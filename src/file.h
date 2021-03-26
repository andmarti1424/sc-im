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
 * \file file.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for file.c
 */

#include "freeze.h"

void erasedb();
void loadrc(void);
int modcheck();
int savefile();
int writefile(char *fname, int r0, int c0, int rn, int cn, int verbose);
void write_fd(register FILE *f, int r0, int c0, int rn, int cn);
void write_cells(register FILE *f, int r0, int c0, int rn, int cn, int dr, int dc);
void write_marks(register FILE *f);
void write_franges(register FILE *f);

typedef enum {
    SC_READFILE_ERROR = 0,
    SC_READFILE_SUCCESS = 1,
    SC_READFILE_DOESNTEXIST = 2
} sc_readfile_result;
sc_readfile_result readfile(char *fname, int eraseflg);

int file_exists(const char * fname);
char * findhome(char *path);
int backup_file(char *path);
FILE * openfile(char *fname, int *rpid, int *rfd);
void closefile(FILE *f, int pid, int rfd);
void print_options(FILE *f);
int import_csv(char * fname, char d);
void do_export(int r0, int c0, int rn, int cn);
void export_delim(char * fname, char coldelim, int r0, int c0, int rn, int cn, int verbose);
void export_plain(char * fname, int r0, int c0, int rn, int cn);
void export_markdown(char * fname, int r0, int c0, int rn, int cn);
void export_latex(char * fname, int r0, int c0, int rn, int cn, int verbose);
void unspecial(FILE * f, char * str, int delim);
int max_length(FILE * f);
int count_lines(FILE * f);
int plugin_exists(char * name, int len, char * path);
void * do_autobackup();
void handle_backup();
void remove_backup(char * file);
int backup_exists(char * file);
