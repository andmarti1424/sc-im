/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
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
 * \file macros.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for cros.c
 */

#define BUFFERSIZE      1024   // must be higher than MAX_IB_LEN
#define MAX_IB_LEN       512   // max input bar length

#define MAX_MULTIPLIER   100
#define MAXSC            15    // MAXSC is the max length of a special key expressed as a string. ex. <C-a>
#define MAXMAPITEM       (MAXSC * 20) // max length of mapping part (in / out)

#define TIMEOUT_CURSES   300   // ms  curses input timeout
#define COMPLETECMDTIMEOUT     (get_conf_int("command_timeout")/4)     // used for goto cell
#define ESC_DELAY        25    // Escape timeout
#define RESCOL           4     // columns reserved for row numbers
#define RESROW           2     // rows reserved for prompt, error, and column numbers
#define RESCOLHEADER     1     // number of row to show column header. always 1. just to make code cleaner

#define NORMAL_MODE      0x01
#define INSERT_MODE      0x02
#define EDIT_MODE        0x04
#define COMMAND_MODE     0x08
#define VISUAL_MODE      0x16

#define ctl(x)           ((x) & 0x1f)
#define uncl(x)           (0x60 | ((x) & 0x1f))
#define OKEY_ESC         '\033'
#define OKEY_TAB         '\011'
#define OKEY_ENTER       10
#define OKEY_SPACE       L' '
#define OKEY_LEFT        0x104
#define OKEY_RIGHT       0x105
#define OKEY_DOWN        0x102
#define OKEY_UP          0x103
#define OKEY_DEL         0x14a
#define OKEY_BS          0x107
#define OKEY_BS2         0x7f   // some BSDs, Linux distros, SSH/tmux configs
#define OKEY_HOME        0x106
#define OKEY_END         0x168
#define OKEY_PGUP        0x153
#define OKEY_PGDOWN      0x152
#define OKEY_F(x)        KEY_F(x)

//#define metak(x) ((x) | 0x80)
#define LEFT             0
#define RIGHT            1

// used for is_single_command function
#define NO_CMD           0
#define EDITION_CMD      1
#define MOVEMENT_CMD     2

#define HEADINGS          0
#define WELCOME           1
#define CELL_SELECTION    2
#define CELL_SELECTION_SC 3
#define NUMB              4
#define STRG              5
#define DATEF             6
#define EXPRESSION        7
#define INFO_MSG          8
#define ERROR_MSG         9
#define MODE              10
#define CELL_ID           11
#define CELL_FORMAT       12
#define CELL_CONTENT      13
#define INPUT             14
#define NORMAL            15
#define CELL_ERROR        16
#define CELL_NEGATIVE     17
#define DEFAULT           18
#define DEBUG_MSG         19
#define VALUE_MSG         20
#define GRID_EVEN         21
#define GRID_ODD          22
#define HEADINGS_ODD      23
#define HELP_HIGHLIGHT    24

void ui_sc_msg(char * s, int type, ...);
#define sc_error(x, ...)     ui_sc_msg(x, ERROR_MSG, ##__VA_ARGS__)
#define sc_debug(x, ...)     ui_sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define sc_info(x, ...)      ui_sc_msg(x, INFO_MSG, ##__VA_ARGS__)
#define sc_value(x, ...)     ui_sc_msg(x, VALUE_MSG, ##__VA_ARGS__)

#define RUNTIME ((current_tv.tv_sec - startup_tv.tv_sec) * 1000L + (current_tv.tv_usec - startup_tv.tv_usec) / 1000L)
