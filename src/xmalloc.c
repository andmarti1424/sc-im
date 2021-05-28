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
 * @file xmalloc.c
 * @author Andrés Martinelli <andmarti@gmail.com>
 * @date 2017-07-18
 * @brief TODO Write a tbrief file description.
 *
 * TODO Write a longer file description here.
 */

#include <stdlib.h>

#include "sc.h"
#include "xmalloc.h"
#include "macros.h"
#include "color.h"
#include "conf.h"

#define MAGIC    ((double) 1234567890.12344)

extern void free();
extern int shall_quit;

/**
 * \brief TODO \document scxmalloc()
 *
 * \param[in] n
 *
 * \return pointer to 
 */

char * scxmalloc(unsigned n) {
    //register char *ptr;
    //if ((ptr = malloc(n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
    //*((double *) ptr) = MAGIC;
    //return(ptr + sizeof(double));

    register char *ptr;
    ptr = (char *) malloc(n);
    if (ptr == NULL) fatal("scxmalloc: no memory");
    return (ptr);
}

/**
 * \brief TODO Make sure realloc will do a malloc if needed
 *
 * \param[in] ptr
 * \param[in] n
 *
 * \returns: pointer
 */

char * scxrealloc(char *ptr, unsigned n) {
    //if (ptr == NULL) return(scxmalloc(n));
    //ptr -= sizeof(double);
    //if (*((double *) ptr) != MAGIC) fatal("scxrealloc: storage not scxmalloc'ed");
    //if ((ptr = realloc(ptr, n + sizeof(double))) == NULL) fatal("scxmalloc: no memory");
    //*((double *) ptr) = MAGIC;
    //return(ptr + sizeof(double));

    if (ptr == NULL) return(scxmalloc(n));
    ptr = (char *) realloc(ptr, n);
    if (ptr == NULL) fatal("scxmalloc: no memory");
    return(ptr);
}

/**
 * \brief TODO Documetn scxfree()
 *
 * \param[in] p
 *
 * \returns: none
 */

void scxfree(char *p) {
    //if (p == NULL) fatal("scxfree: NULL");
    //p -= sizeof(double);
    //if (*((double *) p) != MAGIC) fatal("scxfree: storage not malloc'ed");
    //free(p);

    if (p == NULL) fatal("scxfree: NULL");
    free(p);
}

/**
 * \brief TODO Document fatal()
 *
 * \param[in] str
 *
 * \return none
 */

void fatal(char * str) {
    //fprintf(stderr,"%s\n", str);
    sc_error("%s", str);
    shall_quit = 2;
}
