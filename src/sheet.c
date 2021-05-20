/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 * sc-im is a spreadsheet program that is based on SC. The original authors    *
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
 * \file sheet.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * some of this code is by Roman Pollak - @roman65536
 * \date 2021-05-20
 * \brief source file to handle sheets
 * \see Homepage: https://github.com/andmarti1424/sc-im
 */

#include <stdlib.h>
#include "sheet.h"

/**
 * \brief new_sheet()
 * \param[doc] roman struct
 * \param[name] sheet name
 * \return sutrct sheet *
 */
struct sheet * new_sheet(struct roman * doc, char * name) {
      struct sheet * sh;
      if ((sh = search_sheet(doc, name)) != 0 ) return sh;

      sh = (struct sheet *) calloc(1, sizeof(struct sheet));
      INSERT(sh, (doc->first_sh), (doc->last_sh), next, prev);
      sh->name = name != NULL ? strdup(name) : NULL;

      sh->tbl = 0;

      sh->currow = 0;     /* current row of the selected cell. */
      sh->curcol = 0;     /* current column of the selected cell. */
      sh->lastrow = 0;    /* row of last selected cell */
      sh->lastcol = 0;    /* col of last selected cell */

      sh->offscr_sc_cols = 0; // off screen spreadsheet rows and columns
      sh->offscr_sc_rows = 0;
      sh->nb_frozen_rows = 0;
      sh->nb_frozen_cols = 0; // total number of frozen rows/cols
      sh->nb_frozen_screenrows = 0; // screen rows occupied by those frozen rows
      sh->nb_frozen_screencols = 0; // screen cols occupied by those frozen columns
      /*
      sh->hash= (void *) calloc(HASH_NR,sizeof(void *));
      sh->nr_hash=HASH_NR;
      sh->maxcol = sh->maxrow = 0;
      sh->ccol = 16;
      sh->crow = 32768;
      objs_cache_init(&sh->cache_ent, sizeof(struct Ent), NULL);
      */

      return sh;
  }

/**
 * \brief search_sheet()
 * \param[doc] roman struct
 * \param[name] sheet name
 * \return struct sheet *
 */
struct sheet * search_sheet(struct roman * doc, char * name) {
      if (doc == NULL || name == NULL || ! strlen(name)) return 0;
      struct sheet * sh;

      for(sh = doc->first_sh; sh != 0; sh = sh->next) {
          if (sh->name == NULL) continue;
          if (! strcmp(name, sh->name)) return sh; 
      }
      return 0;
}


/**
 * \brief get_num_sheets()
 * \param[doc] roman struct
 * \return [int] number of sheets
 */
int get_num_sheets(struct roman * doc) {
      if (doc == NULL) return 0;
      struct sheet * sh;
      int cnt = 0;
      for(sh = doc->first_sh; sh != NULL ; sh = sh->next) {
          cnt++;
      }
      return cnt;
}
