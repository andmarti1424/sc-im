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
 * \file conf.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Configuration functions
 *
 * \details This file contains functions that operate on  the user's configuration
 * dictionary (user_conf_d).
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "sc.h"
#include "utils/dictionary.h"


const char default_config[] =
    "half_page_scroll=1\n"
    "autocalc=1\n"
    "numeric=0\n"
    "nocurses=0\n"
    "newline_action=0\n"
    "external_functions=0\n"
    "exec_lua=1\n"
    "xlsx_readformulas=0\n"
    "import_delimited_as_text=0\n"
    "quit_afterload=0\n"
    "quiet=0\n"
    "numeric_zero=1\n"
    "numeric_decimal=1\n"
    "overlap=0\n"
    "truncate=0\n"
    "autowrap=0\n"
    "debug=0\n"
    "ignorecase=0\n"
    "show_cursor=0\n"
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

    "command_timeout=3000\n"
    "mapping_timeout=1500\n"

#ifdef DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD
    "default_open_file_under_cursor_cmd=" DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD "\n"
#else
    "default_open_file_under_cursor_cmd=\n"
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
    parse_str(user_conf_d, default_config, 0);

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
 * \brief Retreive the string value of a given key in user_conf_d
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

char * get_conf_value(const char * key) {
   return get(user_conf_d, key);
}

/**
 * \brief Retreive the integer value of a given key in user_conf_d
 *
 * \details This function will look for a given key in the user_conf_d
 * dictionary. If the key is found it will return the value of that
 * dictionary entry, or 0 otherwise.
 *
 * \param[in] key The key to search for in user_conf_d
 *
 * \return key value
 */
// TODO Make this function take a pointer to a dictionary as an
// argument rather than using user_conf_d directly.

int get_conf_int(const char * key) {
   return get_int(user_conf_d, key);
}


/* \brief change_config_parameter
 * parameter[in] char * cmd
 * return int:
 * 0 if config parameter changed
 * 1 if config parameter was valid but previous and new values are the same
 * -1 on error
 */
#include <wchar.h>
#include <string.h>
#include "macros.h"
#include "cmds/cmds.h"
#include "utils/string.h"
#include "tui.h"
int change_config_parameter(wchar_t * inputline) {
    extern wchar_t interp_line[BUFFERSIZE];

    // remove "set "
    wchar_t line [BUFFERSIZE];
    wcscpy(line, inputline);
    del_range_wchars(line, 0, 3);

    // parse value
    wchar_t * l;
    if ((l = wcschr(line, L' ')) != NULL) l[0] = L'\0';
    if ((l = wcschr(line, L'=')) != NULL) l[0] = L'\0';

    // check a proper config parameter exists
    char oper[BUFFERSIZE];
    wcstombs(oper, line, BUFFERSIZE);
    // sent garbage after "set "..
    if (! strlen(oper)) {
        sc_error("Invalid command: \'%ls\'", inputline);
        return -1;
    }
    char * value_bef = malloc(sizeof(char)*90);
    value_bef[0] = '\0';
    char * key = malloc(sizeof(char)*90);
    key[0] = '\0';
    char * value_aft = malloc(sizeof(char)*90);
    value_aft[0] = '\0';

    strcpy(key, oper);
    char * s_aux = get_conf_value(key);
    if (s_aux != NULL) strcpy(value_bef, get_conf_value(key));
    if ((! value_bef || ! strlen(value_bef)) && strlen(oper) > 2 && ! wcsncmp(inputline, L"set no", 6)) {
        s_aux = get_conf_value(&oper[2]);
        if (s_aux != NULL) {
            strcpy(value_bef, s_aux);
            strcpy(key, &oper[2]);
        }
    }

    if (! value_bef || ! strlen(value_bef)) {
        sc_error("Invalid config variable: \'%s\'", oper);
        free(value_aft);
        free(value_bef);
        free(key);
        return -1;
    }

    // we try to change config value
    wcscpy(interp_line, inputline);
    send_to_interp(interp_line);
    s_aux = get_conf_value(key);
    if (s_aux != NULL) strcpy(value_aft, s_aux);
    // check it was changed
    if (! strcmp(value_bef, value_aft)) { sc_info("Config variable \'%s\' unchanged. Current value is \'%s\'", key, value_aft);
        free(value_aft);
        free(value_bef);
        free(key);
        return 1;
    }
    // inform so
    sc_info("Config variable \'%s\' changed. Value \'%s\' to \'%s\'", key, value_bef, value_aft);
    free(value_aft);
    free(value_bef);
    free(key);
    return 0;
}
