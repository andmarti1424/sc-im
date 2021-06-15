/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
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
 * \file dep_graph.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for dep_graph.c
 */

#include "sc.h"

typedef struct vertexTag {
    struct ent * ent;
    struct sheet * sheet;
    int visited;
    int eval_visited; // just to not to collide with previous. its used in EvalBottomUp.
    struct edgeTag * edges;
    struct edgeTag * back_edges;
    struct vertexTag * next;
} vertexT;

/* For each edge, we need a link to the vertex it connects to and a link to the next edge w.r.t source vertex */
typedef struct edgeTag {
   vertexT * connectsTo;
   struct edgeTag * next;
} edgeT;

typedef struct graphCDT {
  vertexT * vertices;
} graphCDT;

typedef struct graphCDT * graphADT;


graphADT GraphCreate();
vertexT * GraphAddVertex(graphADT graph , struct sheet * sh, struct ent * ent);
vertexT * getVertex(graphADT graph, struct sheet * sh, struct ent * ent, int create);
void GraphAddEdge(vertexT * from, vertexT * to);
void print_vertexs();

void destroy_list_edges(edgeT * e);
void destroy_graph (graphADT graph);
void destroy_vertex(struct sheet * sh, struct ent * ent);
void delete_reference(vertexT * v_cur, vertexT * vc, int back_reference);

void markAllVerticesNotVisited(int eval_visited);
void ents_that_depends_on(struct sheet * sh, struct ent * ent);
void ents_that_depends_on_range(struct sheet * sh, int r1, int c1, int r2, int c2);
void ents_that_depends_on_list(struct ent_ptr * e_ori, int deltar,  int deltac);
int GraphIsReachable(vertexT * src, vertexT * dest, int back_dep);
void rebuild_graph();

void EvalAll();
void EvalBottomUp();
void EvalAllVertexs();
void EvalRange(struct sheet * sh, int tlrow, int tlcol, int brrow, int brcol);
void EvalJustOneVertex(struct sheet * sh, struct ent * p, int rebuild_graph);
