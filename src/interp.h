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
 * \file interp.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for interp.c
 */

struct ent * getent(struct sheet * sh, char * colstr, double rowdoub, int alloc);
double eval(struct sheet * sh, struct ent * ent, struct enode * e);
double fn1_eval(double (* fn)(), double arg);
double fn2_eval(double (* fn)(), double arg1, double arg2);
char * seval(struct sheet * sh, struct ent * ent, struct enode * se);
void setiterations(int i);
void EvalAll();
struct enode * new(int op, struct enode * a1, struct enode * a2);
struct enode * new_var(int op, struct ent_ptr a1);
struct enode * new_range(int op, struct range_s a1);
struct enode * new_const(int op, double a1);
struct enode * new_str(char * s);
void copy(struct ent * dv1, struct ent * dv2, struct ent * v1, struct ent * v2);
void copydbuf(int deltar, int deltac);
void eraser(struct ent * v1, struct ent * v2);
void yankr(struct ent * v1, struct ent * v2);
void mover(struct ent * d, struct ent * v1, struct ent * v2);
void g_free();
void go_last();
void go_previous();
void moveto(struct sheet * sh, int row, int col, int lastrow_, int lastcol_, int cornerrow, int cornercol);
void num_search(struct sheet * sh, double n, int firstrow, int firstcol, int lastrow, int lastcol, int errsearch, int flow);
void str_search(struct sheet * sh, char * s, int firstrow, int firstcol, int lastrow, int lastcol, int num, int flow);
void fill(struct sheet * sh, struct ent * v1, struct ent * v2, double start, double inc);
void lock_cells(struct sheet * sh, struct ent * v1, struct ent * v2);
void unlock_cells(struct sheet * sh, struct ent * v1, struct ent * v2);
void let(struct roman * roman, struct sheet * sh, struct ent * v, struct enode * e);
void slet(struct roman * roman, struct sheet * sh, struct ent * v, struct enode * se, int flushdir);
void format_cell(struct sheet * sh, struct ent * v1, struct ent * v2, char *s);
int constant(struct enode * e);
void efree(struct enode * e);
void label(struct ent * v, char * s, int flushdir);
void decodev(struct ent_ptr v);
char * coltoa(int col);
void decompile_list(struct enode * p);
void decompile(struct enode * e, int priority);
void index_arg(char * s, struct enode * e);
void two_arg_index(char * s, struct enode * e);
void list_arg(char * s, struct enode * e);
void one_arg(char * s, struct enode * e);
void two_arg(char * s, struct enode * e);
void three_arg(char * s, struct enode * e);
void range_arg(char * s, struct enode * e);
void editfmt(struct sheet * sh, int row, int col);
void editv(struct sheet * sh, int row, int col);
void editexp(struct sheet * sh, int row, int col);
void edits(struct sheet * sh, int row, int col, int saveinfile);
int dateformat(struct sheet * sh, struct ent *v1, struct ent *v2, char * fmt);

extern void EvalAllVertexs();
extern void EvalJustOneVertex(struct sheet * sh, struct ent * p, int rebuild_graph);

/* g_type can be: */
#define G_NONE 0          /* Starting value - must be 0 */
#define G_NUM  1
#define G_STR  2
#define G_NSTR 3
#define G_XSTR 4
#define G_CELL 5
