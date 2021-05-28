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
 * \file buffer.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief buffer functions used by stdin andm mappings
 */

#include <stdlib.h>
#include <wchar.h>

#include "buffer.h"
#include "macros.h"
#include "utils/string.h"


/**
* \brief Create buffer as list of blocks
* \return b
*/
struct block * create_buf() {
    struct block * b = (struct block *) malloc(sizeof(struct block));
    b->value = '\0';
    b->pnext = NULL;
    return b;
}


/**
* \brief Add a wint_t to a buffer
* \param[in] buf
* \param[in] d
* \return none
*/
void addto_buf(struct block * buf, wint_t d) {
    struct block * aux = buf;

    if (buf->value == '\0') {
        buf->value = d;
    } else {
        struct block * b = (struct block *) malloc(sizeof(struct block));
        b->value = d;
        b->pnext = NULL;

        while (aux->pnext != NULL)
            aux = aux->pnext;
        aux->pnext = b;
    }
    return;
}


/**
* \brief Replace the elements of "origen" buffer to "destino" buffer
* \param[in] origen
* \param[in] destino
* \return none
*/
void copybuffer(struct block * origen, struct block * destino) {
    flush_buf(destino);
    int len = get_bufsize(origen);
    int i;
    for (i=0; i < len; i++)
        addto_buf(destino, get_bufval(origen, i));
    return;
}


/**
* \brief Replace the element of a buffer at 'pos' with a '\0'
* \param[in] buf
* \param[in] pos
* \return none
*/
// FIXME
void del_buf (struct block * buf, int pos) {
    int i;
    struct block * ant = buf;
    struct block * cur = buf;
    for (i = 0; i < pos; i++) {
        ant = cur;
        cur = cur->pnext;
    }
    if (ant == cur) {
        cur->value = '\0';
        //buf = cur->pnext; //FIXME
        //free(cur);
    } else {
        ant->pnext = cur->pnext;
        free(cur);
    }
    return;
}

/**
* \brief TODO Document flush_buf()
* \param[in] buf
* \return none
*/
void flush_buf (struct block * buf) {
    if (buf == NULL) return;

    struct block * aux, * np;
    for (aux = buf->pnext; aux != NULL; aux = np) {
        np = aux->pnext;
        free(aux);
    }
    buf->value = '\0';
    buf->pnext = NULL;
    return;
}


/**
* \brief Delete all blocks of a buffer including the initial node
* \details Delete all blocks of a buffer including the initial node
* \param buf
* \return none
*/
void erase_buf (struct block * buf) {
    flush_buf(buf);
    free(buf);
    return;
}


/**
* \brief Get size of buffer (included special chars)
* \param[in] buf
* \return c size of buffer
*/
int get_bufsize(struct block * buf) {
    struct block * b_aux = buf;
    if (b_aux == NULL || b_aux->value == '\0') return 0;
    int c = 0;
    while (b_aux != NULL) {
        c++;
        b_aux = b_aux->pnext;
    }
    return c;
}


/**
* \brief Get printable buffer length (excluded special chars)
* \details Get printable bufferlength, which excludes special characters
* as they should never be printed to a screen.
* \param[in] buf
* \return c printable buffer length
*/
int get_pbuflen(struct block * buf) {
    struct block * b_aux = buf;
    if (b_aux == NULL || b_aux->value == '\0') return 0;
    int c = 0;
    while (b_aux != NULL) {
        if ( !is_idchar(b_aux->value) ) c++;
        b_aux = b_aux->pnext;
    }
    return c;
}


/**
* \brief Return the int value of n block
* \param[in] buf
* \param[in] d
* \return none
*/
int get_bufval(struct block * buf, int d) {
    int i;
    struct block * b_aux = buf;
    for (i = 0; i < d; i++) {
        b_aux = b_aux->pnext;
    }
    return b_aux->value;
}


/**
* \brief Return an int value if found in a buffer
* \details Search a buffer for a given integer value.
* \return 0 if not found, 1 if found
*/
int find_val(struct block * buf, int value) {
    struct block * b_aux = buf;
    while ( b_aux != NULL && b_aux->value != '\0' ) {
        if (b_aux->value == value) return 1;
        b_aux = b_aux->pnext;
    }
    return 0;
}


/**
* \brief Delete the first element in a buffer
* \param[in] buf
* \return none
*/
struct block * dequeue (struct block * buf) {
    if (buf == NULL) return buf;
    struct block * sig;
    if (buf->value == '\0') return buf;

    if (buf->pnext == NULL) {
       buf->value = '\0';
    } else {
        sig = buf->pnext;
        //free(buf);
        buf = sig;
    }
    return buf;
}
