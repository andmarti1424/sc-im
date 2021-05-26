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
 * \file function.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 24/05/2021
 * \brief Header file for function.c
 */


double finfunc(int fun, double v1, double v2, double v3);
char * dostindex(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * val);
double doindex(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * val);
double dolookup(struct sheet * sh, struct enode * val, int minr, int minc, int maxr, int maxc, int offset, int vflag);
double docount(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double dosum(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double doprod(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double doavg(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double dostddev(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double domax(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double domin(struct sheet * sh, int minr, int minc, int maxr, int maxc, struct enode * e);
double dodts(int e1, int e2, int e3);
double dotts(int hr, int min, int sec);
double dotime(int which, double when);
double doston(char * s);
int    doslen(char * s);
double doeqs(char * s1, char * s2);
struct ent * dogetent(int r, int c);
double donval(struct sheet * sh, char * colstr, double rowdoub);
double dolmax(struct sheet * sh, struct ent * e, struct enode * ep);
double dolmin(struct sheet * sh, struct ent * e, struct enode * ep);
char * docat(char * s1, char * s2);
#include <time.h>
char * dodate(time_t tloc, char * fmtstr);
char * dofmt(char * fmtstr, double v);
char * doext(struct sheet * sh, struct enode * se);
char * dosval(struct sheet * sh, char * colstr, double rowdoub);
char * dosubstr(char * s, int v1, int v2);
char * docase(int acase, char * s);
char * docapital(char * s);
double doevaluate(char * s);
char * dosevaluate(char * s);
double rint(double d);
double dorow(struct enode * ep);
double docol(struct enode * ep);
double doascii(char * s);
char * doreplace(char * source, char * old, char * new_);
char * dochr(double ascii);

#define dtr(x) ((x)*(M_PI/(double)180.0))
#define rtd(x) ((x)*(180.0/(double)M_PI))
#define ISVALID(s,r,c)    ((r)>=0 && (r) < s->maxrows && (c) >=0 && (c) < s->maxcols)
