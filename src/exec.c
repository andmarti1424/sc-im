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
 * \file exec.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 28/05/2021
 * \brief file that contains source code for sending a command to the OS
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> // for wait
#include <errno.h>

#include "macros.h"
#include "conf.h"
#include "utils/string.h"
#include "tui.h"
#include "sc.h"
#include "main.h"     // exit_app


/**
 * \brief send a command to the OS
 * \param[in] char * command
 * \return int - 0 on success, -1 on error
 */
int exec_cmd (char * line) {
#ifdef NCURSES
    int waitres;

    ui_pause();

    int my_pipe[2];
    if (pipe(my_pipe) == -1) {
        sc_error("Error creating pipe");
        getchar();
        ui_resume();
        ui_update(TRUE);
        return -1;
    }

    pid_t child_id = fork();
    if (child_id == -1) {
        sc_error("Fork error");
        getchar();
        ui_resume();
        ui_update(TRUE);
        return -1;
    }

    if (child_id == 0) {     // we are in the child process
        close(my_pipe[0]);   // child doesn't read
        dup2(my_pipe[1], 1); // redirect stdout

        int argc = 1;
        char * p;
        for (p = line; *p; p++)
            argc += (*p == ' ');
        char ** argv = calloc(argc+1, sizeof(char*));
        int i;
        for (i = 0; i < argc; i++)
            argv[i] = strsep(&line, " ");

        execvp(argv[0], argv);

        free(argv);
        sc_error("Error executing command. ");
        exit_app(-1);

    } else {                 // we are in parent process
        close(my_pipe[1]);   // parent doesn't write
        char reading_buf[2];

        while (read(my_pipe[0], reading_buf, 1) > 0) {
            if (!write(1, reading_buf, 1)) {
                sc_error("Failed to read command output: %s", strerror(errno));
                exit_app(-1);
            }
        }

        close(my_pipe[0]);
        wait(&waitres);
        if (system("echo -n 'Press ENTER to return.'") == -1) {
            sc_error("Failed to show ENTER prompt: %s", strerror(errno));
            exit_app(-1);
        }

        getchar();
        ui_resume();
        ui_update(TRUE);
    }
#endif
    return 0;
}
