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
 * \file block.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief functions used for handling input
 */

#include "block.h"
#include <string.h>
#include "utils/string.h"


/**
 * \brief Function that tells if 'b' buffer is inside 'o' buffer
 * \details Find out if the int elements in the 'b' list are inside the
 * ori list.
 * Since the b list could be in the middle of the o list, and not
 * only at the beginning.
 * \param[in] o
 * \param[in] b
 * \return the position in the 'o' list if found. returns -1 if not in block
 */
// TODO: IMPROVE this. Use two while statements in order to create an array
// from a block, and make them work separately. Leave the logic unmodified
int block_in_block (struct block * o, struct block * b) {
    int lori = get_bufsize(o);
    int lbus = get_bufsize(b);

    if (!lori || !lbus || lbus > lori) return -1;

    // Generate arrays from two block lists
    int ori[lori];
    int bus[lbus];

    struct block * aux = o;
    int i = 0;
    while (aux != NULL) {
        ori[i++] = aux->value;
        aux = aux->pnext;
    }

    aux = b;
    i=0;
    while (aux != NULL) {
        bus[i++] = aux->value;
        aux = aux->pnext;
    }

    int encontrado;
    int k;
    i = 0;
    while (i <= lori-lbus) {
        encontrado = 0;
        for (k = 0; k < lbus; k++) {
            if ( ori[i + k] != bus[k] ) {
                encontrado = -1;
                break;
            }
        }
        if (encontrado != -1) return i;
        i++;
    }
    return -1;
}


/**
 * \brief replace_block_in_block()
 *
 * Replace the content of the block list "olist". Replace the nodes of the 'in'
 * list with the nodes of the 'out' list.
 *
 * \param[in] olist
 * \param[in] in
 * \param[in] out
 *
 * \return 0 on success
 * \return -1 on error
 */
int replace_block_in_block (struct block * olist, struct block * in, struct block * out) {
    struct block * ori = olist;

    int lori = get_bufsize(ori);
    int lin = get_bufsize(in);
    int lout = get_bufsize(out);

    if ( ! lin || ! lori || ! lout ) return -1;

    //info("%d %d %d", lori, lin, lout); get_key();

    // Remove the 'in' part of "olist"
    int pos = block_in_block (olist, in);

    // Remove the 'pos' position of the "olist" list
    while (lin--) del_buf(ori, lin+pos);

    // Then add the nodes of the 'out' list to "olist"
    while (out != NULL) {
        int e = out->value;
        if (e != '\\' || out->pnext == NULL || out->pnext->value != '"')
            addto_buf(olist, e);
        out = out->pnext;
    }

    return 0;
}
