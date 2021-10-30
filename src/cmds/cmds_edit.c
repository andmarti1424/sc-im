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
 * \file cmds_edit.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 */

#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "cmds.h"
#include "cmds_edit.h"
#include "../sc.h"    // for rescol
#include "../tui.h"
#include "../buffer.h"
#include "../utils/string.h"
#include "../interp.h"
#include "../marks.h"
#include "../conf.h"
#include "../history.h"

// this macro is used to determinate a word over a WORD
#define istext(a)    (iswalnum(a) || ((a) == L'_'))

#ifdef INS_HISTORY_FILE
extern char ori_insert_edit_submode;
#endif

static wint_t wi; /**< char read from stdin */
extern struct session * session;

/**
 * \brief TODO Document do_editmode()
 *
 * \param[in] sb
 *
 * \return none
 */

void do_editmode(struct block * sb) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int pos;
    int initial_position;

    if (sb->value == L'h' || sb->value == OKEY_LEFT) {         // LEFT
        if (real_inputline_pos) {
            real_inputline_pos--;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
        }
        ui_show_header();
        return;

    } else if (sb->value == L'l' || sb->value == OKEY_RIGHT) { // RIGHT
        if (real_inputline_pos < wcslen(inputline)-1) {
            real_inputline_pos++;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
        }
        ui_show_header();
        return;

    } else if (sb->value == L' ' && ( wcslen(inputline) < (COLS - 14) ) ) {         // SPACE
        add_wchar(inputline, L' ', real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'^') {                                   // ^
        pos = first_nonblank_char();
        if (pos == -1) return;
        real_inputline_pos = pos;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'g') {                                   // ^
        if (ui_getch_b(&wi) == -1 || wi != L'_') return;
        pos = last_nonblank_char();
        if (pos == -1) return;
        real_inputline_pos = pos;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'0' || sb->value == OKEY_HOME) {         // 0
        inputline_pos = 0;
        real_inputline_pos = 0;
        ui_show_header();
        return;

    } else if (sb->value == L'$' || sb->value == OKEY_END) {          // $
        inputline_pos = wcswidth(inputline, wcslen(inputline)) - 1;
        real_inputline_pos = wcslen(inputline) - 1;
        ui_show_header();
        return;

    } else if (sb->value == L'I') {         // I
        inputline_pos = 0;
        real_inputline_pos = 0;
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L'i' ||  sb->value == L'=') {         // i o =
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L'x') {         // x
        del_back_char();
        ui_show_header();
        return;

    } else if (sb->value == L'X') {         // X
        del_for_char();
        ui_show_header();
        return;

    } else if (sb->value == L'r') {         // r
        //curs_set(1);
        if (ui_getch_b(&wi) != -1) inputline[real_inputline_pos] = wi;
        //curs_set(2);
        ui_show_header();
        return;

    } else if (find_val(sb, OKEY_ENTER) || sb->value == OKEY_ESC) {  // ENTER or ESC
        char ope[BUFFERSIZE] = "";
        wchar_t content[BUFFERSIZE] = L"";
        wcscpy(content, inputline);

        switch (insert_edit_submode) {
            case '=':
                strcpy(ope, "let");
                break;
            case '<':
                strcpy(ope, "leftstring");
                break;
            case '>':
                strcpy(ope, "rightstring");
                break;
            case '\\':
                strcpy(ope, "label");
                break;
        }

        if (content[0] == L'"') {
            del_wchar(content, 0);
        } else if (insert_edit_submode != '=' && content[0] != L'"') {
            add_wchar(content, L'\"', 0);
            add_wchar(content, L'\"', wcslen(content));
        }

        enter_cell_content(sh, sh->currow, sh->curcol, ope, content);

        inputline[0] = L'\0';
        inputline_pos = 0;
        real_inputline_pos = 0;
        chg_mode('.');
        ui_clr_header(1);

        char * opt = get_conf_value("newline_action");
        switch (opt[0]) {
            case 'j':
                sh->currow = forw_row(sh, 1)->row;
                break;
            case 'l':
                sh->curcol = forw_col(sh, 1)->col;
                break;
        }
        ui_update(TRUE);
        return;

    } else if (sb->value == L'a') {         // a
        real_inputline_pos++;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L'A') {         // A
        real_inputline_pos = wcslen(inputline);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L'D') {         // D
        inputline[real_inputline_pos] = L'\0';
        if (real_inputline_pos == wcslen(inputline) && real_inputline_pos && sb->value == L'D') real_inputline_pos--;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'C') {         // C
        inputline[real_inputline_pos] = L'\0';
        inputline_pos = wcswidth(inputline, real_inputline_pos);
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L's') {         // s
        del_back_char();
#ifdef INS_HISTORY_FILE
        ori_insert_edit_submode = insert_edit_submode;
        add(insert_history, L"");
#endif
        chg_mode(insert_edit_submode);
        ui_show_header();
        return;

    } else if (sb->value == L'f') {         // f
        if (ui_getch_b(&wi) == -1) return;
        pos = look_for((wchar_t) wi); // this returns real_inputline_pos !
        if (pos != -1) {
            real_inputline_pos = pos;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
            ui_show_header();
        }
        return;

    } else if (sb->value == L't') {         // t
        if (ui_getch_b(&wi) == -1) return;
        int initial_position = inputline_pos;
        if (inputline_pos < wcswidth(inputline, wcslen(inputline))) inputline_pos++ ;
        pos = look_for((wchar_t) wi); // this returns real_inputline_pos !
        if (pos != -1) {
            real_inputline_pos = pos - 1;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
        } else {
            inputline_pos = initial_position;
        }
        ui_show_header();
        return;

    } else if (sb->value == L'F') {         // F
        if (ui_getch_b(&wi) == -1) return;
        pos = look_back((wchar_t) wi); // this returns real_inputline_pos !
        if (pos != -1) {
            real_inputline_pos = pos;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
            ui_show_header();
        }
        return;

    } else if (sb->value == L'T') {         // T
        if (ui_getch_b(&wi) == -1) return;
        int initial_position = inputline_pos;
        if (inputline_pos) inputline_pos--;
        pos = look_back((wchar_t) wi); // this returns real_inputline_pos !
        if (pos != -1) {
            real_inputline_pos = pos + 1;
            inputline_pos = wcswidth(inputline, real_inputline_pos);
        } else {
            inputline_pos = initial_position;
        }
        ui_show_header();
        return;

    } else if (sb->value == L'w') {         // w
        real_inputline_pos = for_word(0, 0, 0);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'W') {         // W
        real_inputline_pos = for_word(0, 0, 1);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'e') {         // e
        real_inputline_pos = for_word(1, 0, 0);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'E') {         // E
        real_inputline_pos = for_word(1, 0, 1);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'b') {         // b
        real_inputline_pos = back_word(0);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'B') {         // B
        real_inputline_pos = back_word(1);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == L'R') {         // R
        //curs_set(1);
        if (ui_getch_b(&wi) == -1) return;
        wint_t c = wi;
        while (c != OKEY_ENTER && c != -1) {
            if (iswprint(c)) {
                inputline[real_inputline_pos] = c;
                ++real_inputline_pos;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                ui_show_header();
            }
            if (ui_getch_b(&wi) == -1) return;
            c = wi;
        }
        //curs_set(2);


    } else if (sb->value == L'd' || sb->value == L'c') {         // d or c
        wint_t c, d;
        if (ui_getch_b(&wi) != -1) {
            c = wi;
            switch (c) {
            case L'$':
                pos = wcswidth(inputline, wcslen(inputline)) - 1;
                del_range_wchars(inputline, real_inputline_pos, pos);
                if (real_inputline_pos == wcslen(inputline) && real_inputline_pos && sb->value == L'd') real_inputline_pos--;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'f':
                if (ui_getch_b(&wi) == -1) return;
                d = wi;
                pos = look_for((wchar_t) d); // this returns real_inputline_pos !
                if (pos != -1) del_range_wchars(inputline, real_inputline_pos, pos);
                break;

            case L't':
                if (ui_getch_b(&wi) == -1) return;
                initial_position = inputline_pos;
                if (inputline_pos < wcswidth(inputline, wcslen(inputline))) inputline_pos++ ;
                pos = look_for((wchar_t) wi);
                if (pos != -1) {
                    del_range_wchars(inputline, real_inputline_pos, pos - 1);
                    inputline_pos = wcswidth(inputline, real_inputline_pos);
                } else {
                    inputline_pos = initial_position;
                }
                break;

            case L'0':                     // 0
                del_range_wchars(inputline, 0, real_inputline_pos-1);
                real_inputline_pos = 0;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'^':
                pos = first_nonblank_char();
                if (pos == -1) return;
                del_range_wchars(inputline, pos, real_inputline_pos-1);
                real_inputline_pos = pos;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'g':
                if (ui_getch_b(&wi) == -1 || wi != L'_') return;
                pos = last_nonblank_char();
                if (pos == -1) return;
                del_range_wchars(inputline, real_inputline_pos, pos);
                real_inputline_pos = pos;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'e':                     // de or ce
                //OK
                del_range_wchars(inputline, real_inputline_pos, for_word(1, 0, 0));
                break;

            case L'E':                     // dE or cE
                del_range_wchars(inputline, real_inputline_pos, for_word(1, 0, 1));
                break;

            case L'w':                     // dw or cw
                del_range_wchars(inputline, real_inputline_pos, for_word(0, 1, 0) - 1);
                if (real_inputline_pos == wcslen(inputline) && real_inputline_pos && sb->value == L'd') real_inputline_pos--;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'W':                     // dW or cW
                del_range_wchars(inputline, real_inputline_pos, for_word(0, 1, 1) - 1);
                if (real_inputline_pos == wcslen(inputline) && real_inputline_pos && sb->value == L'd') real_inputline_pos--;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'b':                     // db or cb
                d = back_word(0);
                del_range_wchars(inputline, d, real_inputline_pos-1);
                real_inputline_pos = d;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'B':                     // dB or cB
                d = back_word(1);
                del_range_wchars(inputline, d, real_inputline_pos-1);
                real_inputline_pos = d;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'F':
                if (ui_getch_b(&wi) == -1) return;
                pos = look_back((wchar_t) wi);
                if (pos != -1) del_range_wchars(inputline, pos, real_inputline_pos-1);
                real_inputline_pos = pos;
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                break;

            case L'T':
                if (ui_getch_b(&wi) == -1) return;
                initial_position = inputline_pos;
                if (inputline_pos) inputline_pos--;
                pos = look_back((wchar_t) wi);

                if (pos != -1) {
                    del_range_wchars(inputline, pos+1, real_inputline_pos-1);
                    real_inputline_pos = pos;
                    inputline_pos = wcswidth(inputline, real_inputline_pos);
                } else {
                    inputline_pos = initial_position;
                }
                break;

            case L'l':                     // dl or cl
            case OKEY_RIGHT:
                del_back_char();
                break;

            case L'h':                     // dh or ch
            case OKEY_LEFT:
                del_for_char();
                break;

            case L'a':
                if (ui_getch_b(&wi) == -1) return;
                d = wi;
                if ( d == L'W' ) {     // daW or caW
                    c = ( real_inputline_pos && inputline[real_inputline_pos-1] == L' ' ) ? real_inputline_pos : back_word(1);
                    del_range_wchars(inputline, c, for_word(0, 1, 1) - 1);
                    real_inputline_pos = (wcslen(inputline) > real_inputline_pos) ? c : wcslen(inputline)-2;
                    inputline_pos = wcswidth(inputline, real_inputline_pos);
                 } else if ( d == L'w' ) { // daw or caw
                     d = ( real_inputline_pos && ! istext( inputline[real_inputline_pos-1]) ) ? real_inputline_pos : back_word(0);
                     del_range_wchars(inputline, d, for_word(0, 1, 0) - 1);
                     real_inputline_pos = (wcslen(inputline) > real_inputline_pos) ? d : wcslen(inputline)-2;
                     inputline_pos = wcswidth(inputline, real_inputline_pos);
                 }
                 break;
             }
             if (sb->value == L'c') {
#ifdef INS_HISTORY_FILE
                 ori_insert_edit_submode = insert_edit_submode;
                 add(insert_history, L"");
#endif
                 chg_mode(insert_edit_submode);
             }
        }

    }
    ui_show_header();
    return;
}

/**
 * \brief Looks (forward) for a char in inputline
 *
 * \param[in] cb
 *
 * \return position; -1 otherwise
 */

int look_for(wchar_t cb) {
    int c, cpos = inputline_pos;
    while (++cpos < wcslen(inputline))
        if ((c = inputline[cpos]) && c == cb) return cpos;
    //if (cpos > 0 && cpos == wcslen(inputline)) return real_inputline_pos;
    return -1;
}

/**
 * \brief Looks (backwards) for a char in inputline
 *
 * \param[in] cb
 *
 * \return position; -1 otherwise
 */

int look_back(wchar_t cb) {
    int c, cpos = inputline_pos;
    while (--cpos >= 0)
        if ((c = inputline[cpos]) && c == cb) return cpos;
    return -1;
}

/**
 * \brief Looks for a first non blank char in inputline
 *
 * \return position if found; -1 otherwise
 */
int first_nonblank_char() {
    int cpos = -1;
    while (++cpos < wcslen(inputline))
        if (inputline[cpos] && inputline[cpos] != L' ') return cpos;
    return -1;
}


/**
 * \brief Looks for the last non blank char in inputline
 *
 * \return position if found; -1 otherwise
 */
int last_nonblank_char() {
    int cpos = wcslen(inputline);
    while (--cpos > 0)
        if (inputline[cpos] && inputline[cpos] != L' ') return cpos;
    return -1;
}

/**
 * \brief Move backward one word
 *
 * \param[in] big_word
 *
 * \return position
 */

int back_word(int big_word) {
    int c, cpos = real_inputline_pos;
    if (inputline[cpos-1] == L' ' ) cpos--;

    while (cpos)
        if ((c = inputline[--cpos]) && c == L' ' ) return cpos+1;
        else if ( istext( inputline [cpos] ) && ! istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
        else if ( ! istext( inputline [cpos] ) && istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
    return cpos;
}

/**
 * \brief Move to the end of a word
 *
 * \details Used for moving forward to the end of a WORD. big_word
 * looks for ' ', else looks for istext.
 *
 * \param[in] end_of_word
 * \param[in] delete
 * \param[in] big_word
 *
 * \return position; 0 otherwise
 */

int for_word(int end_of_word, int delete, int big_word) {
    int cpos = real_inputline_pos;

    if (! end_of_word) { // w or W
        while ( ++cpos < wcslen(inputline) ) 
            if ( ! istext( inputline[cpos - 1]) && inputline[cpos] != L' ' && ! big_word ) return cpos;
            else if ( inputline[cpos] == L' ' ) return ++cpos;
            else if ( ! istext( inputline [cpos] ) && istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
    } else {             // e or E
        if ( inputline[cpos+1] == L' ' ) cpos += 2;
        else if ( ! istext( inputline [cpos+1] ) && istext( inputline[cpos] ) && ! big_word ) cpos++;
        while ( ++cpos < wcslen(inputline) )
            if ( ( inputline[cpos] == L' ' ) || ( ! istext( inputline [cpos] )
            && istext( inputline[cpos - 1] ) && ! big_word ) ) return --cpos;
    }

    if (cpos > 0 && cpos >= wcslen(inputline)) return wcslen(inputline) - 1 + delete;
    return 0;
}


/**
 * \brief del_back_char()
 * \return none
 */
void del_back_char() {      // x   DEL
    int max = wcswidth(inputline, wcslen(inputline));
    if (inputline_pos > max) return;
    int l = wcwidth(inputline[real_inputline_pos]);
    del_wchar(inputline, real_inputline_pos);
    if (real_inputline_pos == wcslen(inputline) && wcslen(inputline)) {
        inputline_pos -= l;
        real_inputline_pos--;
    } else if (! wcslen(inputline)) {
        inputline_pos = 0;
    }
    return;
}


/**
 * \brief del_for_char()
 * \return none
 */
void del_for_char() {       // X    BS
    if ( ! wcslen(inputline) || ! real_inputline_pos ) return;
    int l = wcwidth(inputline[real_inputline_pos - 1]);
    del_wchar(inputline, real_inputline_pos-1);
    real_inputline_pos--;
    inputline_pos -= l;
    return;
}


/**
 * \brief start_edit_mode()
 * \returns: 1 on success; 0 on error
 */
int start_edit_mode(struct block * buf, char type) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    chg_mode(buf->value);

    line[0]='\0';
    inputline[0]=L'\0';

    struct ent * p1 = lookat(sh, sh->currow, sh->curcol);

    if (type == 'v') { // && p1->flags & is_valid) {   // numeric value
        if (( ! (p1->expr) ) ) {                       // || (p1->flags & is_strexpr)) {
            (void) swprintf(inputline, BUFFERSIZE, L"%.15g", p1->v);
        } else {              // expression
            linelim = 0;
            editexp(sh, sh->currow, sh->curcol);
            linelim = -1;
            (void) swprintf(inputline, BUFFERSIZE, L"%s", line);
        }
        insert_edit_submode='=';

    } else if (type == 's') { // string value
        if (! ((p1)->label || (p1)->flags & is_strexpr)) {
            return 0;
        }
        if (p1->flags & is_label) {
            insert_edit_submode='\\';
        } else if (p1->flags & is_leftflush) {
            insert_edit_submode='<';
        } else {
            insert_edit_submode='>';
        }
        linelim = 0;
        edits(sh, sh->currow, sh->curcol, 0);
        linelim = -1;
        if ((p1)->flags & is_strexpr) swprintf(inputline + wcslen(inputline), BUFFERSIZE, L"\"");
        (void) swprintf(inputline + wcslen(inputline), BUFFERSIZE, L"%s", line);
    }
    inputline_pos = wcswidth(inputline, wcslen(inputline)) - 1;
    real_inputline_pos = wcslen(inputline) - 1;
    //ui_show_header();
    return 1;
}
