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
 * \file plot.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <errno.h>

#include "plot.h"
#include "../file.h"
#include "../tui.h"


/**
 * \brief plotedit()
 * \details edit a plot instruction
 * \param[in] s
 * \return none
 */
int plotedit(wchar_t * s) {
#ifdef GNUPLOT
    // edit ~/.scim/plotxxxx (or /usr/local/share/scim/plotxxxx)
    char command[BUFFERSIZE];

    if (! wcscmp(s, L"line") || ! wcscmp(s, L"scatter") ||
        ! wcscmp(s, L"pie")  || ! wcscmp(s, L"bar")) {
        char buffer[PATHLEN + 5];
        char path_out[PATHLEN];
        char type[BUFFERSIZE];
        wcstombs(type, s, BUFFERSIZE);
        sprintf(buffer, "plot_%s", type);
        if (! plugin_exists(buffer, strlen(buffer), path_out)) {
            sc_error("could not load plot template file");
            return -1;
        }
        ui_pause();

        char * editor;
        if (! (editor = getenv("EDITOR")))
            editor = DFLT_EDITOR;
        sprintf(command, "%.*s %.*s", 100, editor, 100, path_out);

        if (system(command) == -1) sc_error("Failed editting plot file - errno:%d", errno);
        ui_resume();
    } else {
        sc_error("error: invalid plot file: %ls", s);
        return -1;
    }
    return 0;
#else
    sc_error("Gnuplot was not installed when building Sc-im. Please rebuild Sc-im.");
    return -1;
#endif
}


/**
 * \brief plot()
 * \param[in] s
 * \param[in] r
 * \param[in] c
 * \param[in] rf
 * \param[in] cf
 * \return none
 */
int plot(char * s, int r, int c, int rf, int cf) {
#ifdef GNUPLOT
    // create tmp file
    char datafile[] = "/tmp/sc-im-plotdataXXXXXX";
    int fd = mkstemp(datafile);
    if (fd == -1) {
        sc_error("Error while creating temp file for plot");
        return -1;
    }

    // export range to temp file in csv format
    export_delim(datafile, ',', r, c, rf, cf, 0);

    // call gnuplot with  ~/.scim/plotline (or /usr/local/share/scim/plotline) and temp data file
    char command[BUFFERSIZE+PATHLEN];
    char buffer[PATHLEN];
    char plug_buffer[PATHLEN];
    sprintf(command, "gnuplot -e \"filename='%s'\"", datafile);
    if (! strcmp(s, "line") || ! strcmp(s, "scatter") ||
        ! strcmp(s, "pie")  || ! strcmp(s, "bar")) {
        sprintf(buffer, "plot_%s", s);
        if (! plugin_exists(buffer, strlen(buffer), plug_buffer)) {
            sc_error("could not load default plotline file");
            return -1;
        }
        sprintf(command + strlen(command), " %s", plug_buffer);

    } else {
        sc_error("plot option not valid");
        return -1;
    }

    ui_pause();

    if (system(command) == -1)
        sc_error("Failed during plot - errno:%d", errno);
    getchar();
    ui_resume();

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(datafile);

    return 0;
#else
    sc_error("Gnuplot was not installed when building Sc-im. Please rebuild Sc-im.");
    return -1;
#endif
}
