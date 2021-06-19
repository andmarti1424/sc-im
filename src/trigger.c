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
 * \file trigger.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 * \details This is general trigger support. The idea behind triggers is
 * that whenever a value is changed, triggers will be called on Write. And
 * whenever a value will be evaluated a READ trigger will be fired. ent
 * structure is extended with trigger structure.
 *
 * Triggers need mode,type,file,function flags
 *   mode - can be R,W,RW
 *   type - C|LUA (later even SH)
 *   file - File name of the Trigger commands file
 *   function - Which function should be called for both Lua and C
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>      // for atoi
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "tui.h"
#include "undo.h"
#include "conf.h"
#include "cmds/cmds.h"
#include "trigger.h"
#include "file.h"

#ifdef XLUA
#include "lua.h"
#endif

extern struct session * session;

/**
 * \brief TODO Document set_trigger()
 *
 * \param[in] r
 * \param[in] c
 * \param[in] rf
 * \param[in] cf
 * \param[in] str
 *
 * \return none
 */

void set_trigger(struct sheet * sh, int r, int c, int rf, int cf, char * str) {
    struct roman * roman = session->cur_doc;
    if (any_locked_cells(sh, r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    // parse detail
    // Create key-value dictionary for the content of the string
    struct dictionary * d = create_dictionary();

    // Remove quotes
    if (str[0] == '"') del_char(str, 0);
    if (str[strlen(str)-1] == '"') del_char(str, strlen(str)-1);

    parse_str(d, str, TRUE);

    if (
        (get(d,"mode") == NULL) ||
        (get(d,"type") == NULL) ||
        (get(d,"file") == NULL) ||
        (get(d,"function") == NULL )) {
            sc_error("One of the values specified is wrong. Please parameters of the trigger to be set.");
            destroy_dictionary(d);
            return;
    }

    struct ent * n;
    int i, j;
    for (i = r; i <= rf; i++) {
        for (j = c; j <= cf; j++) {
            // action
            n = lookat(sh, i, j);
            if (n->trigger == NULL)
                n->trigger = (struct trigger *) malloc(sizeof(struct trigger));
            else {
                free(n->trigger->file);
                free(n->trigger->function);
            }
            n->trigger->file = strdup(get(d,"file"));
            n->trigger->function = strdup(get(d,"function"));
            int tmp;
            if (strcmp(get(d,"mode"), "R") == 0) tmp = TRG_READ;
            if (strcmp(get(d,"mode"), "W") == 0) tmp = TRG_WRITE;
            if (strcmp(get(d,"mode"), "RW")== 0) tmp = TRG_READ | TRG_WRITE;
#ifdef XLUA
            if (strcmp(get(d,"type"), "LUA")== 0) tmp |= TRG_LUA;
#endif
            if (strcmp(get(d,"type"), "C")== 0) {
                char * error;
                char buffer[PATHLEN];
                char buffer1[PATHLEN];
                tmp |= TRG_C;
                sprintf(buffer, "module/%s", n->trigger->file);

                if (plugin_exists(buffer, strlen(buffer), buffer1))
                    n->trigger->handle = dlopen(buffer1, RTLD_LAZY);
                if (! n->trigger->handle) {
                    sc_error("Trigger could not be set: %s", dlerror());
                    return;
                }
                n->trigger->c_function = dlsym(n->trigger->handle, n->trigger->function);
                if ((error = dlerror()) != NULL)  {
                    sc_error("Trigger could not be set: %s.", error);
                    return;
                }
            }
            n->trigger->flag = tmp;
            if (! roman->loading) sc_info("Trigger was set");
        }
    }
    destroy_dictionary(d);
    return;
}

/**
 * \brief TODO Document del_trigger()
 *
 * \param[in] r
 * \param[in] c
 * \param[in] rf
 * \param[in] cf
 *
 * \return none
 */

void del_trigger(struct sheet * sh, int r, int c, int rf, int cf ) {
    if (any_locked_cells(sh, r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    struct ent * n;
    int i, j;
    for (i = r; i <= rf; i++) {
        for (j = c; j <= cf; j++) {
           // action
           n = lookat(sh, i, j);
           if (n->trigger != NULL ) {
               if ((n->trigger->flag & TRG_C) == TRG_C) {
                   dlclose(n->trigger->handle);
               }
               free(n->trigger->file);
               free(n->trigger->function);
               free(n->trigger);
               n->trigger = NULL;
           }
        }
    }
    return;
}

static int in_trigger = 0;

/**
 * \brief TODO Document do_trigger()
 *
 * \param[in] p
 * \param[in] rw
 *
 * \return none
 */

void do_trigger(struct ent *p , int rw) {
    struct trigger * trigger = p->trigger;
    if(in_trigger) return;
    in_trigger = 1;

#ifdef XLUA
    if ((trigger->flag & TRG_LUA ) == TRG_LUA) {
        sc_info("%d %d", p->row, p->col);
        doLuaTrigger_cell(p,rw);
    }
#endif

    if ((trigger->flag & TRG_C ) == TRG_C) {
        do_C_Trigger_cell(p,rw);
    }
    in_trigger = 0;
    return;
}

/**
 * \brief TODO Document do_C_Trigger_cell()
 *
 * \param[in] p
 * \param[in] rw
 *
 * \return none
 */

void do_C_Trigger_cell(struct ent * p, int rw) {
    int status;
    int (*function)(struct ent *, int );

    function = p->trigger->c_function;
    if ( (status = (*function)(p,rw ))) {
        sc_info("Trigger reported error code: %d", status);
    }
    return;
}
