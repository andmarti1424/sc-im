/*******************************************************************************
 * Copyright (c) 2013-2025, Andrés G. Martinelli <andmarti@gmail.com>          *
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
 *    This product includes software developed by Andrés G. Martinelli         *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés G. Martinelli nor the                     *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRÉS G. MARTINELLI ''AS IS'' AND ANY         *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRÉS G. MARTINELLI BE LIABLE FOR ANY        *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file buffer.h
 * \author Andrés G. Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for buffer.c
 */

#include <wchar.h>
#include <wctype.h>

// Block of buffer
struct block {
  wint_t value;
  struct block * pnext;
};

struct block * create_buf();
void addto_buf(struct block * buf, wint_t d);
void copybuffer(struct block * origen, struct block * destino);
void del_buf (struct block * buf, int pos);
void flush_buf (struct block * buf);
void erase_buf (struct block * buf);
int get_bufsize(struct block * buf);
int get_pbuflen(struct block * buf);
int get_bufval(struct block * buf, int d);
int find_val(struct block * buf, int value);
struct block * dequeue (struct block * buf);
