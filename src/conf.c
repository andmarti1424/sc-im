/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
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
 * \file conf.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Configuration functions
 *
 * \details This file contains functions that operate on  the user's configuration
 * dictionary (user_conf_d) and the predefined dictionary (predefined_conf_d).
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "utils/dictionary.h"


const char default_config[] =
	"half_page_scroll=1\n"
	"autocalc=1\n"
	"numeric=0\n"
	"nocurses=0\n"
	"newline_action=j\n"
	"external_functions=0\n"
	"xlsx_readformulas=0\n"
	"import_delimited_as_text=0\n"
	"quit_afterload=0\n"
	"numeric_zero=1\n"
	"numeric_decimal=1\n"
	"filename_with_mode=0\n"
	"overlap=0\n"
	"truncate=0\n"
	"debug=0\n"
	"ignorecase=0\n"
	"trigger=1\n"
	"version=0\n"
	"help=0\n"
	"input_bar_bottom=0\n"
	"underline_grid=0\n"
    #ifdef AUTOBACKUP
	"autobackup=0\n"  // 0:noautobackup, n>0: backup every n in seconds
    #endif

    #ifdef DEFAULT_COPY_TO_CLIPBOARD_CMD
	"default_copy_to_clipboard_cmd=" DEFAULT_COPY_TO_CLIPBOARD_CMD "\n"
    #else
	"default_copy_to_clipboard_cmd=\n"
    #endif

	"copy_to_clipboard_delimited_tab=0\n"

    #ifdef DEFAULT_PASTE_FROM_CLIPBOARD_CMD
	"default_paste_from_clipboard_cmd=" DEFAULT_PASTE_FROM_CLIPBOARD_CMD "\n"
    #else
	"default_paste_from_clipboard_cmd=\n"
    #endif

	"tm_gmtoff=0\n";

/**
 * \brief Populates user_conf_d with default values
 *
 * \details Populates the user's configuration dictionary (user_conf_d) with
 * default values.
 *
 * \return none
 */
// TODO Make this function take a pointer to a dictionary as an
// argument rather than using user_conf_d directly.

void store_default_config_values() {
    char *line = default_config;

    do {
        parse_str(user_conf_d, line, 0);
	line = strchr(line, '\n');
    } while(line && *++line != 0);
	
    // Calculate GMT offset (not on Solaris, doesn't have tm_gmtoff)
    #if defined(USELOCALE) && !defined(__sun)
    time_t t = time(NULL);
    struct tm * lt = localtime(&t);
    char strgmtoff[7];
    sprintf(strgmtoff, "%ld", lt->tm_gmtoff);
    put(user_conf_d, "tm_gmtoff", strgmtoff);
    #endif
}

/**
 * \brief TODO Document get_conf_values()
 *
 * \param[in] salida TODO Document this parameter
 *
 * \return NULL if user_conf_d is NULL; otherwise, salida is returned unchanged
 */

// TODO Change 'salida' to a more descriptive variable name.
// TODO Make this function take a pointer to a dictionary as an
// argument rather than using user_conf_d directly.

char * get_conf_values(char * salida) {
   if (user_conf_d == NULL) return NULL;
   struct nlist * nl;

   salida[0]='\0';

   char *buf = salida;
   for (nl = user_conf_d->list; nl != NULL; nl = nl->next) {
       // ignore version conf variable here so that its not shown in :set command
       if (! strcmp(nl->key, "version")) continue;

       buf += sprintf(buf, "%s=%s\n", nl->key, nl->val);
   }
   return salida;
}

/**
 * \brief Retreive the value of a given key in user_conf_d
 *
 * \details This function will look for a given key in the user_conf_d
 * dictionary. If the key is found it will return the value of that
 * dictionary entry.
 *
 * \param[in] key The key to search for in user_conf_d
 *
 * \return key value
 */
// TODO Make this function take a pointer to a dictionary as an
// argument rather than using user_conf_d directly.

char * get_conf_value(char * key) {
   char * val = get(user_conf_d, key);

   if (val == NULL || val[0] == '\0')
       return get(predefined_conf_d, key);
   else
       return val;
}
