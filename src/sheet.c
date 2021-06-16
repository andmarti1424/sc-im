/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
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
 * \file sheet.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * some of this code is by Roman Pollak - @roman65536
 * \date 2021-05-20
 * \brief source file to handle sheets
 * \see Homepage: https://github.com/andmarti1424/sc-im
 */

#include <stdlib.h>
#include "sheet.h"
#include "file.h"
#include "yank.h"
#include "marks.h"
#include "graph.h"

int id_sheet = 0;

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
      sh->name = strdup(name);

      sh->tbl = NULL;

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

      sh->maxcol = 0;
      sh->maxrow = 0;
      sh->id = id_sheet++; /* an id of sheet must be kept so that we can insert vertex's ordered
                              (and look them up) in the dependency graph. */
      /*
      sh->hash= (void *) calloc(HASH_NR,sizeof(void *));
      sh->nr_hash=HASH_NR;
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
      if (doc == NULL || name == NULL || ! strlen(name)) return NULL;
      struct sheet * sh;

      for(sh = doc->first_sh; sh != 0; sh = sh->next) {
          if (sh->name == NULL) continue;
          if (! strcmp(name, sh->name)) return sh;
      }
      return NULL;
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

/**
 * \brief free_session(): free memory of a session calling delete_doc
 * \param[doc] roman struct
 * \return void
 */
void free_session(struct session * session) {
    while (session != NULL) {
        struct roman * r_aux, * r = session->first_doc;
        // traverse romans
        while (r != NULL) {
            r_aux = r->next;
            delete_doc(session, r);
            r = r_aux;
        }
        // free session
        free(session);
        session = NULL;
    }
    return;
}


/**
 * \brief delete_doc()
 * \details delete content of a doc and free its memory calling delete_sheet()
 * \param[doc] struct roman *
 * \param[sh] struct sheet *
 * \return void
 */
void delete_doc(struct session * session, struct roman * doc) {
    // remove the link of doc to session
    REMOVE(doc, (session->first_doc), (session->last_doc), next, prev);

    // traverse doc sheets
    struct sheet * s_aux, * sh = doc->first_sh;
    while (sh != NULL) {
        s_aux = sh->next;
        delete_sheet(doc, sh, 1);
        sh = s_aux;
    }
    if (doc->name != NULL) {
        free(doc->name);
        doc->name = NULL;
    }
    free(doc);
    return;
}

/**
 * \brief delete_sheet()
 * \details delete content of a sheet and free its memory
 * \param[doc] struct roman *
 * \param[sh] struct sheet *
 * \param[flg_free] int
 * \return void
 */
void delete_sheet(struct roman * roman, struct sheet * sh, int flg_free) {
    REMOVE(sh, (roman->first_sh), (roman->last_sh), next, prev);

    // mark '->sheet to NULL' in all ents on yanklist that refers to this sheet
    struct ent_ptr * yl = get_yanklist();
    while (yl != NULL) {
        if (yl->sheet == sh) yl->sheet = NULL;
        yl = yl->next;
    }

    // reset marks that links to the sheet we are deleting
    clean_marks_by_sheet(sh);

    // free sheet
    erasedb(sh, flg_free); // clear sh and free on if flg_free is true

    // if we are deleting a sheet not at exit
    // we need to update references of ents of other sheets
    // that could refer to this one by rebuilding the graph.
    if (! flg_free) rebuild_graph();

    for (int row = 0; sh->tbl != NULL && row < sh->maxrows; row++) {
        if (sh->tbl[row] != NULL) {
            free(sh->tbl[row]);
            sh->tbl[row] = NULL;
        }
    }

    free(sh->tbl);
    free(sh->fwidth);
    free(sh->precision);
    free(sh->realfmt);
    free(sh->col_hidden);
    free(sh->col_frozen);
    free(sh->row_hidden);
    free(sh->row_frozen);
    free(sh->row_format);

    if (sh->name != NULL) {
        free(sh->name);
        sh->name = NULL;
    }
    free(sh);
    sh = NULL;
    return;
}

