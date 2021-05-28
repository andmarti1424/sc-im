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
 * \file color.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for color.c
 */

#include "sc.h"
#include "macros.h"

#include <math.h>
#define RGB(r, g, b)    r*999/255, g*999/255, b*999/255

#define N_INIT_PAIRS      28

#define HEADINGS          0
#define WELCOME           1
#define CELL_SELECTION    2
#define CELL_SELECTION_SC 3
#define NUMB              4
#define STRG              5
#define DATEF             6
#define EXPRESSION        7
#define INFO_MSG          8
#define ERROR_MSG         9
#define MODE              10
#define CELL_ID           11
#define CELL_FORMAT       12
#define CELL_CONTENT      13
#define INPUT             14
#define NORMAL            15
#define CELL_ERROR        16
#define CELL_NEGATIVE     17
#define DEFAULT           18
#define DEBUG_MSG         19
#define VALUE_MSG         20
#define GRID_EVEN         21
#define GRID_ODD          22
#define HEADINGS_ODD      23
#define HELP_HIGHLIGHT    24
#define SHEET             25
#define CURRENT_SHEET     26
#define FILENM            27

struct ucolor {
    int fg;
    int bg;
    int bold;
    int italic;
    int dim;
    int reverse;
    int standout;
    int underline;
    int blink;
};

struct custom_color {
    int number; // this is the custom color number. since we max have 24 user custom colors.
    char * name;
    int r;
    int g;
    int b;

    /* TODO we actually could store rgb values in just one int
       int rgb = ((r & 0x0ff) << 16) | ((g & 0x0ff) << 8) | (b & 0x0ff);
       int red = (rgb >> 16) & 0x0ff;
       int green = (rgb >> 8) & 0x0ff;
       int blue = (rgb) & 0x0ff;
     */

    struct custom_color * p_next;
};


extern struct ucolor ucolors[N_INIT_PAIRS];

struct dictionary * get_d_colors_param();
void start_default_ucolors();
void color_cell(struct sheet * sh, int r, int c, int rf, int cf, char * str);
void unformat(struct sheet * sh, int r, int c, int rf, int cf);
void set_colors_param_dict();
void free_colors_param_dict();
void chg_color(char * str);
int same_ucolor(struct ucolor * u, struct ucolor * v);
int redefine_color(char * color, int r, int g, int b);

int define_color(char * color, int r, int g, int b);
int free_custom_colors();
struct custom_color * get_custom_color(char * name);
int count_custom_colors();
struct custom_color * get_custom_color_by_number(int number);
