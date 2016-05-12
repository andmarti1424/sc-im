#include <stdio.h>
#include <stdlib.h>
#include "dep_graph.h"
#include "interp.h"
#include "screen.h"    // for show_text
#include "sc.h"
#include <setjmp.h>
#include "xmalloc.h" // for scxfree
#include <math.h>

extern jmp_buf fpe_save;
extern int cellerror;    /* is there an error in this cell */


/******************************************************************************
 * This file contains all functions used for maintaining a dependence graph
 * that keeps track of all the cells that depends on each other.
 * this is done in a two way relationship.
 *
 * A vertex represents an ent and has in "edges", links to other vertex's(ents) which the first vertex depends on.
 *
 * The other relationship is "back_edges". This is a pointer to other vertex's and
 * there you will keep linked all the vertex's that use the first vertex in their formulas.
 * In other words, you will keep track of all the vertex's that depends on the first vertex.
 *
 * NOTE: an orphan vertex represents an ent that has an enode thats need to be evaluated, but do not depend in another cell.
 *******************************************************************************/
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

/*  LINKS:
 *  https://www.cs.bu.edu/teaching/c/graph/linked/
 *  https://github.com/Chetan496/cpp-algortihms/blob/master/graph.c
 */

graphADT graph;

/* Creates an empty graph, with no vertices. Allocate memory from the heap */
graphADT GraphCreate() {
   graphADT emptyGraph = (graphCDT *) malloc(sizeof(graphCDT));
   emptyGraph->vertices = NULL;
   return emptyGraph;
}

/* this adds the vertex sorted in the list
 * and not at the end
 * given a row & col to insert as a new vertex, this function will create a new vertex with those values
 * and add it order in the list!
 * returns a pointer to the new vertex
 */
vertexT * GraphAddVertex(graphADT graph , struct ent * ent) {
    vertexT * newVertex = (vertexT *) malloc(sizeof(vertexT));
    newVertex->visited = 0;
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
    //scinfo("Added the vertex %d %d in the graph", row, col) ;
    return newVertex;
}


/* This looks for a vertex representing a specific ent in a sorted list
 * we search for a vertex in graph and return it if found.
 * if not found and create flag, we add the vertex (malloc it) and return it
 * else if not found, it returns NULL
 */
vertexT * getVertex(graphADT graph, struct ent * ent, int create) {
   if (graph == NULL) return NULL;
   vertexT * temp = graph->vertices;
   while (temp != NULL && (temp->ent->row < ent->row || (temp->ent->row == ent->row && temp->ent->col <= ent->col))) {
       if (temp->ent->row == ent->row && temp->ent->col == ent->col) return temp;
       temp = temp->next;
   }

   // if we get to here, there is not vertex representing ent
   // we add it if create is set to true!
   return create ? GraphAddVertex(graph, ent) : NULL;
}


/* This function adds a edge in our graph from the vertex "from" to the vertex "to" */
// should add edges ordered in list ?
void GraphAddEdge(vertexT * from, vertexT * to) {
   if (from == NULL || to == NULL) {
      sc_info("Error while adding edge: either of the vertices do not exist") ;
      return;
   }
   // do we have to check this here? or shall we handle it outside from the caller?
   markAllVerticesNotVisited(); // needed to check if edge already exists
   if (GraphIsReachable(from, to, 0)) {
      //sc_info("Error while adding edge: the edge already exists!") ;
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



// iterate through all the vertices. Set visited = false
void markAllVerticesNotVisited () {
   vertexT * temp = graph->vertices;
   while (temp != NULL) {
      temp->visited = 0;
      temp = temp->next;
   }
   return;
}


// print vertices
void print_vertexs() {
   char det[20000] = ""; // TODO: - improve: malloc, remalloc and free dinamically
   if (graph == NULL) {
       strcpy(det, "Graph is empty");
       show_text((char *) &det);
       return;
   }

   vertexT * temp = graph->vertices;
   edgeT * etemp;
   det[0]='\0';
   while (temp != NULL) {
      sprintf(det + strlen(det), "%d %d\n", temp->ent->row, temp->ent->col);
      etemp = temp->edges;
      while (etemp != NULL) {
          sprintf(det + strlen(det), "    \\-> depends on the following ents: %d %d\n", etemp->connectsTo->ent->row, etemp->connectsTo->ent->col);
          etemp = etemp->next;
      }
      etemp = temp->back_edges;
      while (etemp != NULL) {
          sprintf(det + strlen(det), "edges that depend on that ent: \\-> %d %d\n", etemp->connectsTo->ent->row, etemp->connectsTo->ent->col);
          etemp = etemp->next;
      }

      temp = temp->next;
   }
   show_text((char *) &det);
   return;
}


/* this function frees the memory of vertex's edges.
 * this also frees the vertex itself, but only if it has no back_dependences.
 * the only parameter is an ent pointer.
 */
void destroy_vertex(struct ent * ent) {
   if (graph == NULL || ent == NULL) return;

   vertexT * v_prev, * v_cur = graph->vertices;

   // if is in the middle of the list
   if (v_cur->ent->row != ent->row || v_cur->ent->col != ent->col) {
       if (v_cur->ent == NULL) sc_error("ERROR");
       v_prev = v_cur;
       v_cur = v_cur->next;
       while (v_cur != NULL && (v_cur->ent->row < ent->row || (v_cur->ent->row == ent->row && v_cur->ent->col <= ent->col))) {
           if (v_cur->ent->row == ent->row && v_cur->ent->col == ent->col) break;
           v_prev = v_cur;
           v_cur = v_cur->next;
       }
       if (v_cur->ent->row != ent->row || v_cur->ent->col != ent->col) {
           sc_debug("Error while destroying a vertex. Vertex not found! Please rebuild graph");
           return;
       }
       v_prev->next = v_cur->next;
   }

   // for each edge in back_edges, we look for the reference to the vertex we are deleting, and we erase it!
   edgeT * e2 = v_cur->back_edges;
   while (e2 != NULL) {
    //   sc_debug("back_edge: we follow %d %d", e2->connectsTo->ent->row, e2->connectsTo->ent->col);
       delete_reference(v_cur, e2->connectsTo, 0);
       e2 = e2->next;
   }

   // for each edge in edges, we look for the reference to the vertex we are deleting, and we erase it!
   edgeT * e = v_cur->edges;
   while (e != NULL) {
    //   sc_debug("edge: we follow %d %d", e->connectsTo->ent->row, e->connectsTo->ent->col);
       delete_reference(v_cur, e->connectsTo, 1);

       if (e->connectsTo->edges == NULL && e->connectsTo->back_edges == NULL && !e->connectsTo->ent->expr) destroy_vertex(e->connectsTo->ent);
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


// for each edge in edges, we look for the reference to the vertex we are deleting and we erase it!
// v_cur is the reference
// if back_reference is set, the delete is done over the back_edges list
// if not, it is done over edges list.
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
        e_prev = eb;
        e_cur = eb->next;
    }
    if (e_cur != NULL && e_cur->connectsTo == v_cur) {
        e_prev->next = e_cur->next;
        free(e_cur);
    }
    return;
}


// this free memory of an edge and its linked edges
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


// this free memory of a graph
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



void rebuild_graph() {
    destroy_graph(graph);
    graph = GraphCreate();
    int i, j;
    struct ent * p;

    for (i = 0; i <= maxrow; i++)
        for (j = 0; j <= maxcol; j++)
        if ((p = *ATBL(tbl, i, j)) && p->expr)
            EvalJustOneVertex(p, i, j, 1);
    return;
}


void EvalAll() {
    // Eval vertexs of graph
    EvalAllVertexs();
    return;
}

void EvalAllVertexs() {
    struct ent * p;

    //(void) signal(SIGFPE, eval_fpe);
    vertexT * temp = graph->vertices;
    //int i = 0;
    while (temp != NULL) {
        //sc_info("%d %d %d", temp->ent->row, temp->ent->col, i++);
        if ((p = *ATBL(tbl, temp->ent->row, temp->ent->col)) && p->expr)
            EvalJustOneVertex(p, temp->ent->row, temp->ent->col, 0);
        temp = temp->next;
    }
    //(void) signal(SIGFPE, exit_app);
}

// Evaluate just one vertex
void EvalJustOneVertex(register struct ent * p, int i, int j, int rebuild_graph) {
    //sc_debug("EvalJustOneVertex: %d %d", i, j);
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
            //(*chgct)++;
            p->flags |= is_changed;
            changed++;
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

            if (cellerror == CELLOK && ! finite(v))
                cellerror = CELLERROR;
        }
        if ((cellerror != p->cellerror) || (v != p->v)) {
            p->cellerror = cellerror;
            p->v = v;
            //if (! cellerror)    /* don't keep eval'ing an error */
            //    (*chgct)++;
            p->flags |= is_changed | is_valid;
            changed++;
        }
    }
}




/**********************************************************************************
 * the folowing functions and variables are used for ent_that_depends_on function.
 * the last is used to get the list of ents that depends on an specific ent
 * the result is saved in a list of ents.
 **********************************************************************************/
struct ent_ptr * deps = NULL;
int dep_size = 0;

void ents_that_depends_on (struct ent * ent) {
   if (graph == NULL) {
       return;
   } else {
       vertexT * v = getVertex(graph, ent, 0);
       if (v->visited) return;

       struct edgeTag * edges = v->back_edges;
       while (edges != NULL) {
           deps = (struct ent_ptr *) realloc(deps, sizeof(struct ent_ptr) * (++dep_size));
           deps[0].vf = dep_size; // we always keep size of list in the first position !
           deps[dep_size-1].vp = lookat(edges->connectsTo->ent->row, edges->connectsTo->ent->col);
           ents_that_depends_on(edges->connectsTo->ent);
           edges->connectsTo->visited = 1;
           edges = edges->next;
       }
   }
   return;
}

// This method returns if a vertex called dest is reachable from the vertex called src
// (if back_dep is set to false).
// if back_dep is set to true, the relationship is evaluated in the opposite way.
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
/*******************************************************************/


