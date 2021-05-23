/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
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
 * \file interp.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for interp.c
 */

#include <time.h>
double finfunc(int fun, double v1, double v2, double v3);
char * dostindex(int minr, int minc, int maxr, int maxc, struct enode * val);
double doindex(int minr, int minc, int maxr, int maxc, struct enode * val);
double dolookup(struct enode * val, int minr, int minc, int maxr, int maxc, int offset, int vflag);
double docount(int minr, int minc, int maxr, int maxc, struct enode * e);
double dosum(int minr, int minc, int maxr, int maxc, struct enode * e);
double doprod(int minr, int minc, int maxr, int maxc, struct enode * e);
double doavg(int minr, int minc, int maxr, int maxc, struct enode * e);
double dostddev(int minr, int minc, int maxr, int maxc, struct enode * e);
double domax(int minr, int minc, int maxr, int maxc, struct enode * e);
double domin(int minr, int minc, int maxr, int maxc, struct enode * e);
double dodts(int e1, int e2, int e3);
double dotts(int hr, int min, int sec);
double dotime(int which, double when);
double doston(char * s);
int    doslen(char * s);
double doeqs(char * s1, char * s2);
struct ent * getent(char * colstr, double rowdoub, int alloc);
struct ent * dogetent(int r, int c);
double donval(char * colstr, double rowdoub);
double dolmax(struct ent * e, struct enode * ep);
double dolmin(struct ent * e, struct enode * ep);
//double eval(register struct enode *e);
double eval(register struct ent * ent, register struct enode * e);
double fn1_eval(double (* fn)(), double arg);
double fn2_eval(double (* fn)(), double arg1, double arg2);
char * docat(register char * s1, register char * s2);
char * dodate(time_t tloc, char * fmtstr);
char * dofmt(char * fmtstr, double v);
char * doext(struct enode * se);
char * doext(struct enode * se);
char * dosval(char * colstr, double rowdoub);
char * dosubstr(char * s, register int v1, register int v2);
char * docase(int acase, char * s);
char * docapital(char * s);
char * seval(register struct ent * ent, register struct enode * se);
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
void moveto(int row, int col, int lastrow, int lastcol, int cornerrow, int cornercol);
void num_search(double n, int firstrow, int firstcol, int lastrow, int lastcol, int errsearch, int flow);
void str_search(char * s, int firstrow, int firstcol, int lastrow, int lastcol, int num, int flow);
void fill(struct ent * v1, struct ent * v2, double start, double inc);
void lock_cells(struct ent * v1, struct ent * v2);
void unlock_cells(struct ent * v1, struct ent * v2);
void let(struct ent * v, struct enode * e);
void slet(struct ent * v, struct enode * se, int flushdir);
void format_cell(struct ent * v1, struct ent * v2, char * s);
int constant(register struct enode * e);
void efree(struct enode * e);
void label(register struct ent * v, register char * s, int flushdir);
void decodev(struct ent_ptr v);
char * coltoa(int col);
void decompile_list(struct enode * p);
void decompile(register struct enode * e, int priority);
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
int dateformat(struct ent * v1, struct ent * v2, char * fmt);
double rint(double d);

void EvalAllVertexs();
void EvalJustOneVertex(register struct ent * p, int rebuild_graph);
double doevaluate(char * s);
char * dosevaluate(char * s);
