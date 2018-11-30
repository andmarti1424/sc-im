/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of SC-IM                                                *
 *                                                                             *
 * SC-IM is a spreadsheet program that is based on SC. The original authors    *
 * of SC are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file dep_graph.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief All the functions used to track cell dependencies
 *
 * \details This file contains all functions used for maintaining a
 * dependence graph that keeps track of all the cells that depends on
 * each other. This is done in a two way relationship.
 *
 * \details A vertex represents an ent and has in "edges", links to
 * other vertex's(ents) which the first vertex depends on.
 *
 * \details The other relationship is "back_edges". This is a pointer
 * to other vertex's and there you will keep linked all the vertex's that
 * use the first vertex in their formulas. In other words, you will keep
 * track of all the vertex's that depends on the first vertex.
 *
 * \details NOTE: an orphan vertex represents an ent that has an enode
 * thats need to be evaluated, but do not depend in another cell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <math.h>

#include "dep_graph.h"
#include "interp.h"
#include "tui.h"    // for show_text
#include "sc.h"
#include "xmalloc.h"   // for scxfree
#include "macros.h"
#include "trigger.h"

extern jmp_buf fpe_save;
extern int cellerror;    /**< is there an error in this cell */

#define CREATE_NEW(type) (type *) malloc(sizeof(type))

#define APPEND_TO_LINKLIST(firstNode, newNode, tempNode) \
   if( firstNode == NULL ) { \
      firstNode = newNode ; \
   } \
   else {      \
      tempNode = firstNode; \
      while (tempNode->next != NULL) { \
         tempNode = tempNode->next; \
      } \
      tempNode->next = newNode; \
   }

graphADT graph; /**< Creates an empty graph, with no vertices. Allocate memory from the heap */

/**
 * \brief TODO Document GraphCreate()
 *
 * \return An empty graph
 */

graphADT GraphCreate() {
   graphADT emptyGraph = (graphCDT *) malloc(sizeof(graphCDT));
   emptyGraph->vertices = NULL;
   return emptyGraph;
}

/**
 * \brief Undefined function
 *
 * This adds the vertex sorted in the list and not at the end. Given a row and
 * column to insert as a new vertex, this function will create a new vertex
 * with those values and add it in order to the list.
 *
 * \param[in] graph
 * \param[in] ent
 *
 * \return a pointer to the new vertex
 */

vertexT * GraphAddVertex(graphADT graph , struct ent * ent) {
    //if (ent == NULL) {
    //    sc_debug("add vertex-  null ent");
    //    return NULL;
    //}
    //sc_debug("will add vertex %d %d ", ent->row, ent->col);
    vertexT * newVertex = (vertexT *) malloc(sizeof(vertexT));
    newVertex->visited = 0;
    newVertex->eval_visited = 0;
    newVertex->ent = ent;
    newVertex->edges = NULL;
    newVertex->back_edges = NULL;
    newVertex->next = NULL;

    vertexT * temp_ant = NULL;
    vertexT * tempNode = NULL;

    // first element added to the list
    if( graph->vertices == NULL) {
        graph->vertices = newVertex;

    // append in first position
    } else if (ent->row < graph->vertices->ent->row || (ent->row == graph->vertices->ent->row && ent->col < graph->vertices->ent->col)) {
        newVertex->next = graph->vertices;
        graph->vertices = newVertex;

    // append in second position or after that
    } else {
        tempNode = graph->vertices;
        temp_ant = tempNode;
        while (tempNode != NULL && (ent->row > tempNode->ent->row || (ent->row == tempNode->ent->row && ent->col > tempNode->ent->col) ) ) {
            temp_ant = tempNode;
            tempNode = temp_ant->next;
        }
        temp_ant->next = newVertex;
        newVertex->next = tempNode;
    }
    //sc_debug("Added vertex %d %d in the graph", ent->row, ent->col) ;
    return newVertex;
}


/**
 * \brief TODO Write a brief description
 *
 * \details This looks for a vertex representing a specific ent in
 * a sorted list. We search for a vertex in graph and return it if
 * found. If not found and create flag, we add add the vertex
 * (malloc it) and return it. If not found, it returns NULL.
 *
 * \param[in] graph
 * \param[in] ent
 * \param[in] create
 *
 * \return vertex if found; NULL if not found
 */

vertexT * getVertex(graphADT graph, struct ent * ent, int create) {
   if (graph == NULL || ent == NULL || (graph->vertices == NULL && !create)) return NULL;
   vertexT * temp = graph->vertices;
   //sc_debug("getVertex - looking for %d %d, create:%d", ent->row, ent->col, create);
   //while (temp != NULL && temp->ent != NULL) // temp->ent should not be NULL
   while (temp != NULL
          //in case it was inserted ordered
          && (temp->ent->row < ent->row || (temp->ent->row == ent->row && temp->ent->col <= ent->col)))
                       {
       //sc_debug("this vertex exists: %d %d", temp->ent->row, temp->ent->col);
       if (temp->ent->row == ent->row && temp->ent->col == ent->col) {
           //sc_debug("found vertex: %d %d", temp->ent->row, temp->ent->col);
           return temp;
       }
       temp = temp->next;
   }
   //sc_debug("not found vertex: %d %d", ent->row, ent->col);


   /*
    * if we get to here, there is not vertex representing ent
    * we add it if create is set to true!
    */
   return create ? GraphAddVertex(graph, ent) : NULL;
}


/*
 * This function adds a edge in our graph from the vertex "from" to the vertex "to"
 * should add edges ordered in list ?
 */
/**
 * \brief Add an edge to a graph
 *
 * \details This function adds an edge in out graph from the vertex "from"
 * to the vertex "to". Should add edges ordered in list?
 *
 * \return none
 */

void GraphAddEdge(vertexT * from, vertexT * to) {
   if (from == NULL || to == NULL) {
      sc_info("Error while adding edge: either of the vertices do not exist") ;
      return;
   }
   // do we have to check this here? or shall we handle it outside from the caller?
   markAllVerticesNotVisited(0); // needed to check if edge already exists
   if (GraphIsReachable(from, to, 0)) {
      //sc_info("Error while adding edge: the edge already exists! - %d %d - %d %d", from->ent->row, from->ent->col, to->ent->row, to->ent->col) ;
      return;
   }

   edgeT * newEdge = CREATE_NEW(edgeT) ;
   newEdge->connectsTo = to;
   newEdge->next = NULL;
   edgeT * tempEdge = NULL;
   APPEND_TO_LINKLIST(from->edges, newEdge, tempEdge);
//sc_info("Added the edge from %d %d to %d %d", from->row, from->col, to->row, to->col) ;

   // BACK APPEND
   newEdge = CREATE_NEW(edgeT) ;
   newEdge->connectsTo = from;
   newEdge->next = NULL;
   tempEdge = NULL;
   APPEND_TO_LINKLIST(to->back_edges, newEdge, tempEdge);
//sc_info("Added BACK reference from %d %d to %d %d", to->row, to->col, from->row, from->col) ;
  return;
}

/**
 * \brief Iterate through all verticies and set visited to false
 *
 * Iterate through all verticies and set visited to false
 * evalvisited
 *
 * \return none
 */

void markAllVerticesNotVisited (int eval_visited) {
   vertexT * temp = graph->vertices;
   while (temp != NULL) {
      if (eval_visited) temp->eval_visited = 0;
      else temp->visited = 0;
      temp = temp->next;
   }
   return;
}


/**
 * \brief Prints vertexes
 *
 * \return none
 */

void print_vertexs() {
   char det[BUFFERSIZE] = "";
   if (graph == NULL) {
       strcpy(det, "Graph is empty");
       ui_show_text((char *) &det);
       return;
   }
   vertexT * temp = graph->vertices;
   edgeT * etemp;
   det[0]='\0';
   int msg_size = BUFFERSIZE;
   char * msg = (char *) malloc(msg_size);
   msg[0]='\0';
   strcpy(msg, "Content of graph:\n");

   while (temp != NULL) {
      sprintf(det + strlen(det), "vertex: %d %d vis:%d eval_vis:%d\n", temp->ent->row, temp->ent->col, temp->visited, temp->eval_visited);
      etemp = temp->edges;

      /* check not overflow msg size. if so, just realloc. */
      if (strlen(det) + strlen(msg) > msg_size) {
          //sc_debug("realloc"),
          msg_size += BUFFERSIZE;
          msg = (char *) realloc(msg, msg_size);
      }
      sprintf(msg + strlen(msg), "%s", det);
      det[0]='\0';
      /**/
      while (etemp != NULL) {
          sprintf(det + strlen(det), "    \\-> depends on the following ents: %d %d\n", etemp->connectsTo->ent->row, etemp->connectsTo->ent->col);
          etemp = etemp->next;

          /* check not overflow msg size. if so, just realloc. */
          if (strlen(det) + strlen(msg) > msg_size) {
              msg_size += BUFFERSIZE;
              msg = (char *) realloc(msg, msg_size);
          }
          sprintf(msg + strlen(msg), "%s", det);
          det[0]='\0';
          /**/
      }
      etemp = temp->back_edges;
      while (etemp != NULL) {
          sprintf(det + strlen(det), "(back_edges) edges that depend on that ent: \\-> %d %d\n", etemp->connectsTo->ent->row, etemp->connectsTo->ent->col);
          etemp = etemp->next;

          /* check not overflow msg size. if so, just realloc. */
          if (strlen(det) + strlen(msg) > msg_size) {
              msg_size += BUFFERSIZE;
              msg = (char *) realloc(msg, msg_size);
          }
          sprintf(msg + strlen(msg), "%s", det);
          det[0]='\0';
          /**/
      }
      temp = temp->next;
   }
   ui_show_text((char *) msg);
   free(msg);
   return;
}


/**
 * \brief Destroy a vertex
 *
 * \details This function frees the memory of vertex's edges. This also
 * frees the vertex itself, but only if it has no back_dependences. The
 * only parameter is an int pointer.
 *
 * \param[in] ent
 *
 * \return none
 */

void destroy_vertex(struct ent * ent) {
   if (graph == NULL || ent == NULL) return;
   //sc_debug("destroying vertex %d %d", ent->row, ent->col);

   vertexT * v_prev, * v_cur = graph->vertices;

   // if is in the middle of the list
   if (v_cur->ent->row != ent->row || v_cur->ent->col != ent->col) {
       if (v_cur->ent == NULL) sc_error("ERROR destroying vertex");
       v_prev = v_cur;
       v_cur = v_cur->next;
       while (v_cur != NULL && (v_cur->ent->row < ent->row || (v_cur->ent->row == ent->row && v_cur->ent->col <= ent->col))) {
           if (v_cur->ent->row == ent->row && v_cur->ent->col == ent->col) break;
           v_prev = v_cur;
           v_cur = v_cur->next;
       }
       if (v_cur->ent->row != ent->row || v_cur->ent->col != ent->col) {
           sc_error("Error while destroying a vertex. Vertex not found! Please rebuild graph");
           return;
       }
       v_prev->next = v_cur->next;
   }

   /*FIXME
   // for each edge in back_edges, we look for the reference to the vertex we are deleting, and we erase it!
   edgeT * e2 = v_cur->back_edges;
   while (e2 != NULL) {
    //   sc_debug("back_edge: we follow %d %d", e2->connectsTo->ent->row, e2->connectsTo->ent->col);
       delete_reference(v_cur, e2->connectsTo, 0);
       e2 = e2->next;
   }*/

   // for each edge in edges, we look for the reference to the vertex we are deleting, and we erase it!
   edgeT * e = v_cur->edges;
   if (v_cur->back_edges == NULL)
   while (e != NULL && v_cur->back_edges == NULL) {
    // sc_debug("edge: we follow %d %d", e->connectsTo->ent->row, e->connectsTo->ent->col);
       delete_reference(v_cur, e->connectsTo, 1);

       // delete vertex only if it end up having no edges, no expression, no value, no label....
       if (e->connectsTo->edges == NULL && e->connectsTo->back_edges == NULL && !e->connectsTo->ent->expr && !(e->connectsTo->ent->flags & is_valid) && ! e->connectsTo->ent->label)
           destroy_vertex(e->connectsTo->ent);
//     WARNING: an orphan vertex now represents an ent that has an enode thats
//     need to be evaluated, but do not depend in another cell.
       e = e->next;
   }

   destroy_list_edges(v_cur->edges);
   v_cur->edges = NULL;

   destroy_list_edges(v_cur->back_edges);
   v_cur->back_edges = NULL;


   // if vertex to free was the first one..
   if (graph->vertices && graph->vertices->ent->row == ent->row && graph->vertices->ent->col == ent->col)
       graph->vertices = v_cur->next;

   free(v_cur);
   return;
}


/**
 * \brief TODO Write brief function description
 *
 * \details For each edge in edges, we look for the references to the
 * vertex we are deleting and we erase it!
 *
 * \details v_cur is the reference.
 *
 * \details If back_reference is set, the delete is done over the
 * back_edges list. If not, it is done over edges list.
 *
 * \param[in] v_cur
 * \param[in] tc
 * \param[in] back_reference
 *
 * \return none
 */

void delete_reference(vertexT * v_cur, vertexT * vc, int back_reference) {
    if (v_cur == NULL || vc == NULL) return;
//  sc_debug("we follow %d %d", vc->ent->row, vc->ent->col);

    // If v_cur is in the first position of back_edge list of vc
    if (back_reference && vc->back_edges->connectsTo == v_cur) {
        edgeT * e_cur = vc->back_edges;
        vc->back_edges = e_cur->next;
        free(e_cur);
        return;
    // If v_cur is in the first position of edge list of vc
    } else if (! back_reference && vc->edges->connectsTo == v_cur) {
        edgeT * e_cur = vc->edges;
        vc->edges = e_cur->next;
        free(e_cur);
        return;
    }

    // If v_cur is not in the first position
    edgeT * eb = back_reference ? vc->back_edges : vc->edges;
    edgeT * e_prev = eb;
    edgeT * e_cur = eb->next;
    while (e_cur != NULL && e_cur->connectsTo != v_cur) {
        e_prev = e_cur;
        e_cur = e_cur->next;
    }
    if (e_cur != NULL && e_cur->connectsTo == v_cur) {
        e_prev->next = e_cur->next;
        free(e_cur);
    }
    return;
}


/**
 * \brief Free memory of an edge and its linked edges
 *
 * \return none
 */

void destroy_list_edges(edgeT * e) {
    if (e == NULL) return;
    edgeT * e_next, * e_cur = e;

    while (e_cur != NULL) {
        e_next = e_cur->next;
        e_cur->connectsTo = NULL;
        free(e_cur);
        e_cur = e_next;
    }
    return;
}


/**
 * \brief Free the memory of a graph
 *
 * \param[in] graph
 *
 * \return none
 */

void destroy_graph(graphADT graph) {
    if (graph == NULL) return;

    vertexT * v_next, * v_cur = graph->vertices;
    while (v_cur != NULL) {
        v_next = v_cur->next;
        if (v_cur->edges != NULL) destroy_list_edges(v_cur->edges);
        if (v_cur->back_edges != NULL) destroy_list_edges(v_cur->back_edges);
        free(v_cur);
        v_cur = v_next;
    }
    free(graph);
    return;
}


/**
 * \brief Document rebuild_graph()
 * \details Rebuild entire graph and eval from top left to bottom right.
 * \return none
 */

void rebuild_graph() {
    destroy_graph(graph);
    graph = GraphCreate();
    int i, j;
    struct ent * p;

    for (i = 0; i <= maxrow; i++)
        for (j = 0; j <= maxcol; j++)

        if ((p = *ATBL(tbl, i, j)) && p->expr) {
            EvalJustOneVertex(p, i, j, 1);
            //sc_debug("Expr %d %d", i, j);
        } else if ((p = *ATBL(tbl, i, j)) && p->flags & is_valid && getVertex(graph, p, 0) == NULL) {
            GraphAddVertex(graph, p);
            //sc_debug("Val %d %d", i, j);
        }
    return;
}


/**
 * \brief Document All_vertexs_of_edges_visited
 * \details Used in EvalBottomUp and GraphIsReachable
 * \return int
 */
int All_vertexs_of_edges_visited(struct edgeTag * e, int eval_visited) {
    struct edgeTag * edges = e;
    while (edges != NULL) {
        //sc_debug("1 r:%d c:%d visited:%d", edges->connectsTo->ent->row, edges->connectsTo->ent->col, edges->connectsTo->eval_visited);
        if (eval_visited && ! edges->connectsTo->eval_visited) return 0;
        else if (!eval_visited && ! edges->connectsTo->visited) return 0;
        edges = edges->next;
    }
    return 1;
}


/**
 * \brief EvalBottomUp
 * New Eval implementation. It operates bottom up.
 * Replaces EvalAllVertexs()
 * \return none
 */
void EvalBottomUp() {
    //print_vertexs();
    vertexT * temp = graph->vertices;
    struct ent * p;

    markAllVerticesNotVisited(1);
    int evalDone = 0;

    while (temp != NULL) {
        //sc_debug("analizo %d %d", temp->ent->row, temp->ent->col);
        if ( ! temp->eval_visited && (temp->edges == NULL || All_vertexs_of_edges_visited(temp->edges, 1))) {
            //sc_debug("visito %d %d", temp->ent->row, temp->ent->col);

            if ((p = *ATBL(tbl, temp->ent->row, temp->ent->col)) && p->expr) {
                EvalJustOneVertex(temp->ent, temp->ent->row, temp->ent->col, 0);
            }
            temp->eval_visited = 1;
            evalDone = 1;
        }
        temp = temp->next;
        if (temp == NULL) {
            //sc_debug("temp is null. evaldone: %d", evalDone);
            if (! evalDone) return;
            evalDone = 0;
            temp = graph->vertices;
        }
    }
    return;
}


/**
 * \brief Eval vertexs of graph
 * \return none
 */
void EvalAll() {
    //EvalAllVertexs();
    EvalBottomUp();
    return;
}


/**
 * \brief TODO Document EvalAllVertexs
 * \details Eval all vertexs of graph in the order that they were added
 * \return none
 */

void EvalAllVertexs() {
    struct ent * p;

    //(void) signal(SIGFPE, eval_fpe);
    vertexT * temp = graph->vertices;
    //int i = 0;
    while (temp != NULL) {
        //sc_debug("Evaluating cell %d %d: %d", temp->ent->row, temp->ent->col, ++i);
        if ((p = *ATBL(tbl, temp->ent->row, temp->ent->col)) && p->expr)
            EvalJustOneVertex(p, temp->ent->row, temp->ent->col, 0);
        temp = temp->next;
    }
    //(void) signal(SIGFPE, exit_app);
}

/**
 * \brief Evaluate just one vertex
 *
 * \param[in] p
 * \param[in] i
 * \param[in] j
 * \param[in] rebuild_graph
 *
 * \return none
 */
void EvalJustOneVertex(register struct ent * p, int i, int j, int rebuild_graph) {

    gmyrow=i; gmycol=j;

    if (p->flags & is_strexpr) {
        char * v;
        if (setjmp(fpe_save)) {
            sc_error("Floating point exception %s", v_name(i, j));
            cellerror = CELLERROR;
            v = "";
        } else {
            cellerror = CELLOK;
            v = rebuild_graph ? seval(p, p->expr) : seval(NULL, p->expr);
        }
        p->cellerror = cellerror;
        if ( !v && !p->label) /* Everything's fine */
            return;
        if ( !p->label || !v || strcmp(v, p->label) != 0 || cellerror) {
            p->flags |= is_changed;
        }
        if (p->label)
            scxfree(p->label);
        p->label = v;
    } else {
        double v;
        if (setjmp(fpe_save)) {
            sc_error("Floating point exception %s", v_name(i, j));
            cellerror = CELLERROR;
            v = (double) 0.0;
        } else {
            cellerror = CELLOK;
            v = rebuild_graph ? eval(p, p->expr) : eval(NULL, p->expr);

            if (cellerror == CELLOK && ! isfinite(v))
                cellerror = CELLERROR;
        }
        if ((cellerror != p->cellerror) || (v != p->v)) {
            p->cellerror = cellerror;
            p->v = v;
            p->flags |= is_changed | is_valid;
            if (( p->trigger  ) && ((p->trigger->flag & TRG_WRITE) == TRG_WRITE))
                do_trigger(p, TRG_WRITE);
        }
    }
}


/* TODO Incorporate this comment into the functions and variables below.
 * the folowing functions and variables are used for ent_that_depends_on function.
 * the last is used to get the list of ents that depends on an specific ent
 * the result is saved in a list of ents.
 */
struct ent_ptr * deps = NULL;
int dep_size = 0;

/**
 * \brief TODO Document ents_that_depends_on()
 *
 * \param[in] ent
 *
 * \return none
 */

void ents_that_depends_on (struct ent * ent) {
   if (graph == NULL) return;
   vertexT * v = getVertex(graph, ent, 0);
   if (v == NULL || v->visited) return;

   struct edgeTag * edges = v->back_edges;
   while (edges != NULL) {
       // TODO only add ent if it does not exists in deps ??
       deps = (struct ent_ptr *) realloc(deps, sizeof(struct ent_ptr) * (++dep_size));
       deps[0].vf = dep_size; // we always keep size of list in the first position !
       deps[dep_size-1].vp = lookat(edges->connectsTo->ent->row, edges->connectsTo->ent->col);
       ents_that_depends_on(edges->connectsTo->ent);
       edges->connectsTo->visited = 1;
       edges = edges->next;
   }
   return;
}

/**
 * \brief TODO Write brief function description
 *
 * \details This method returns if a vertex called dest is reachable
 * from the vertex called src (if back_dep is set to false). If back_dep
 * is set to true, the relationship is evaluated in the opposite way.
 *
 * \param[in] src
 * \param[out] dest
 * \param[in] back_dep
 *
 * \return
 */
// TODO List the returns of this function

int GraphIsReachable(vertexT * src, vertexT * dest, int back_dep) {
   if (src == dest) {
       return 1;
   } else if (src->visited) {
       return 0;
   } else {
       // visit all edges of vertexT * src
       src->visited = 1;

       edgeT * tempe;
       if ( !back_dep )
           tempe = src->edges;
       else
           tempe = src->back_edges;

       while (tempe != NULL) {
          if ( ! GraphIsReachable(tempe->connectsTo, dest, back_dep)) {
             tempe = tempe->next;
          } else {
             return 1;
          }
       }
   }
   return 0;
}

/**
 * \brief TODO Document ents_that_depends_on_range()
 *
 * \details Checks dependency of a range of ents. Keep the ends
 * in "deps" lists.
 *
 * \param[in] r1
 * \param[in] c1
 * \param[in] r2
 * \param[in] c2
 *
 * \return none
 */

void ents_that_depends_on_range (int r1, int c1, int r2, int c2) {
        if (graph == NULL) return;

        int r, c;
        struct ent * p;

        // at this point deps must be NULL
        deps = NULL;
        dep_size = 0;

        for (r = r1; r <= r2; r++) {
            for (c = c1; c <= c2; c++) {
                markAllVerticesNotVisited(0);
                p = *ATBL(tbl, r, c);
                if (p == NULL) continue;
                ents_that_depends_on(p);
            }
        }
        return;
}
