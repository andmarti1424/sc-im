#include <string.h>
#include <ncursesw/curses.h>
//#include <ctype.h> // for isalnum isprint
#include "cmds.h"
#include "screen.h"
#include "buffer.h"
#include "sc.h"    // for rescol
#include "cmds_edit.h"
#include "utils/string.h"
#include "interp.h"
#include "marks.h"

// this macro is used to determinate a word over a WORD
#define istext(a)    (iswalnum(a) || ((a) == L'_'))

#include <wchar.h>
#include <wctype.h>
wint_t get_key() {
     static wint_t wi;                      // char read from stdin

     wtimeout(input_win, -1);
     move(0, rescol + inputline_pos + 1);
     wget_wch(input_win, & wi);             // return value of wget_wch()
     wtimeout(input_win, TIMEOUT_CURSES);
     if ( wi != OKEY_ESC ) return wi;
     return -1;
}

// REVISED
void do_editmode(struct block * sb) {
    if (sb->value == L'h' || sb->value == OKEY_LEFT) {         // LEFT
        inputline_pos = back_char();

    } else if (sb->value == L'l' || sb->value == OKEY_RIGHT) { // RIGHT
        inputline_pos = for_char();

    } else if (sb->value == L'x') {         // x
        del_back_char();

    } else if (sb->value == L'X') {         // X
        del_for_char();

    } else if (sb->value == L' ' && ( wcslen(inputline) < (COLS - 14) ) ) {         // SPACE
        add_wchar(inputline, L' ', inputline_pos);

    } else if (sb->value == L'r') {         // r
        curs_set(1);
        wint_t c = get_key();
        if (c != -1) inputline[inputline_pos] = c;
        curs_set(2);

    } else if (sb->value == L'R') {         // R
        curs_set(1);
        wint_t c = get_key();
        while (c != OKEY_ENTER && c != -1) {
            if (iswprint(c)) {
                inputline[inputline_pos] = c;
                ++inputline_pos;

                mvwprintw(input_win, 0, 1 + rescol, "%ls", inputline);
                wmove(input_win, 0, inputline_pos + 1 + rescol);
                wrefresh(input_win);
            }
            c = get_key();
        }
        curs_set(2);

    } else if (sb->value == L'f') {         // f
        wint_t c = get_key();
        if (c != -1) inputline_pos = look_for(c);

    } else if (sb->value == L'd' || sb->value == L'c') {         // d or c
        wint_t c, d;
        if ( (c = get_key()) != -1 ) {
             switch (c) {
             case L'e':                     // de or ce
                 del_range_wchars(inputline, inputline_pos, for_word(1, 0, 0));
                 break;

             case L'E':                     // dE or cE
                 del_range_wchars(inputline, inputline_pos, for_word(1, 0, 1));
                 break;

             case L'w':                     // dw or cw
                 del_range_wchars(inputline, inputline_pos, for_word(0, 1, 0) - 1);
                 if (inputline_pos == wcslen(inputline) && inputline_pos) inputline_pos--;
                 break;

             case L'W':                     // dW or cW
                 del_range_wchars(inputline, inputline_pos, for_word(0, 1, 1) - 1);
                 if (inputline_pos == wcslen(inputline) && inputline_pos) inputline_pos--;
                 break;

             case L'b':                     // db or cb
                 d = back_word(0);
                 del_range_wchars(inputline, d, inputline_pos-1);
                 inputline_pos = d;
                 break;

             case L'B':                     // dB or cB
                 d = back_word(1);
                 del_range_wchars(inputline, d, inputline_pos-1);
                 inputline_pos = d;
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
                 if ( (d = get_key()) == L'W' ) {     // daW or caW
                     c = ( inputline_pos && inputline[inputline_pos-1] == ' ' ) ? inputline_pos : back_word(1);
                     del_range_wchars(inputline, c, for_word(0, 1, 1) - 1);
                     inputline_pos = (wcslen(inputline) > inputline_pos) ? c : wcslen(inputline)-2;
                 } else if ( d == L'w' ) { // daw or caw
                     d = ( inputline_pos && ! istext( inputline[inputline_pos-1]) ) ? inputline_pos : back_word(0);
                     del_range_wchars(inputline, d, for_word(0, 1, 0) - 1);
                     inputline_pos = (wcslen(inputline) > inputline_pos) ? d : wcslen(inputline)-2;
                 }
                 break;
             }
             if (sb->value == L'c') chg_mode(insert_edit_submode);
        }

    } else if (find_val(sb, OKEY_ENTER)) {  // ENTER
        insert_or_edit_cell(); 
        return;

    } else if (sb->value == L'$') {         // $
        inputline_pos = wcslen(inputline) - 1;

    } else if (sb->value == L'w') {         // w
        inputline_pos = for_word(0, 0, 0);

    } else if (sb->value == L'W') {         // W
        inputline_pos = for_word(0, 0, 1);

    } else if (sb->value == L'e') {         // e
        inputline_pos = for_word(1, 0, 0);

    } else if (sb->value == L'E') {         // E
        inputline_pos = for_word(1, 0, 1);

    } else if (sb->value == L'b') {         // b
        inputline_pos = back_word(0);

    } else if (sb->value == L'B') {         // B
        inputline_pos = back_word(1);

    } else if (sb->value == L'0') {         // 0
        inputline_pos = 0;

    } else if (sb->value == L'a') {         // a
        inputline_pos++;
        chg_mode(insert_edit_submode);

    } else if (sb->value == L'i' ||  sb->value == L'=') {         // i o =
        chg_mode(insert_edit_submode);

    } else if (sb->value == L's') {         // s
        if (inputline_pos <= wcslen(inputline)) del_wchar(inputline, inputline_pos);
        chg_mode(insert_edit_submode);

    } else if (sb->value == L'A') {         // A
        inputline_pos = wcslen(inputline);
        chg_mode(insert_edit_submode);

    } else if (sb->value == L'I') {         // I
        inputline_pos = 0;
        chg_mode(insert_edit_submode);

    } else if (sb->value == L'D') {         // D
        inputline_pos = 0;
        inputline[0] = L'\0';
        chg_mode(insert_edit_submode);
    }

    show_header(input_win);
    return;
}

// looks for a char in inputline
// REVISED
int look_for(char cb) {
    int c, cpos = inputline_pos;
    while (++cpos < wcslen(inputline))
        if ((c = inputline[cpos]) && c == cb) return cpos;
    if (cpos > 0 && cpos == wcslen(inputline)) return inputline_pos;
    return -1;
}

// move backwards a word
// REVISED
int back_word(int big_word) {
    int c, cpos = inputline_pos;
    if (inputline[cpos-1] == L' ' ) cpos--;

    while (cpos)
        if ((c = inputline[--cpos]) && c == L' ' ) return cpos+1;
        else if ( istext( inputline [cpos] ) && ! istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
        else if ( ! istext( inputline [cpos] ) && istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
    return cpos;
}

// end_of_word is used for moving forward to end of a WORD
// big_word looks for ' ', else looks for istext.
// delete 1 is used when typing dw command
// REVISED
int for_word(int end_of_word, int delete, int big_word) {
    int cpos = inputline_pos;

    if (! end_of_word) { // w or W
        while ( ++cpos < wcslen(inputline) ) 
            if ( ! istext( inputline[cpos - 1]) && inputline[cpos] != L' ' && ! big_word ) return cpos; //agregado big_word el 08/06
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

// REVISED
int for_char() {
    if (inputline_pos < wcslen(inputline) - 1)
        return ++inputline_pos;
    return inputline_pos;
}

// REVISED
int back_char() {
    if (inputline_pos) return --inputline_pos;
    return inputline_pos;
}

// REVISED
void del_back_char() {
    if (inputline_pos > wcslen(inputline)) return;
    del_wchar(inputline, inputline_pos);
    if (inputline_pos == wcslen(inputline) && wcslen(inputline)) inputline_pos--;
}

// REVISED
void del_for_char() {
    if ( ! wcslen(inputline) || ! inputline_pos ) return;
    del_wchar(inputline, --inputline_pos);
}

// REVISED
// return 1 OK
// return 0 on error
int start_edit_mode(struct block * buf, char type) {
    chg_mode(buf->value);

    line[0]='\0';
    inputline[0]=L'\0';

    struct ent * p1 = lookat(currow, curcol);

    if (type == 'v') { // && p1->flags & is_valid) {   // numeric value
        if (( ! (p1->expr) ) ) {                       // || (p1->flags & is_strexpr)) {
            (void) swprintf(inputline, BUFFERSIZE, L"%.15g", p1->v);
        } else {              // expression
            linelim = 0;
            editexp(currow, curcol);
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
        edits(currow, curcol, 0);
        linelim = -1;
        (void) swprintf(inputline, BUFFERSIZE, L"%s", line);
    }
    inputline_pos = wcslen(inputline) - 1;

    return 1;
}
