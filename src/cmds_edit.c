#include "cmds_edit.h"
#include "cmds.h"
#include "stdout.h"
#include <string.h>
#include <curses.h>
#include "buffer.h"
#include "sc.h"  // for rescol

// this macro is used to determinate a word over a WORD
#define istext(a)    (isalnum(a) || ((a) == '_'))

int get_key() {
     wtimeout(input_win, -1);
     int c; 
     move(0, rescol + inputline_pos + 1);
     c = wgetch(input_win);
     wtimeout(input_win, TIMEOUT_CURSES);
     if ( c != OKEY_ESC ) return c;
     return -1;
}

void do_editmode(struct block * sb) {

    if (sb->value == 'h') {                // LEFT
        inputline_pos = back_char();

    } else if (sb->value == 'l') {         // RIGHT
        inputline_pos = for_char();

    } else if (sb->value == 'x') {         // x
        del_back_char();

    } else if (sb->value == 'X') {         // X
        del_for_char();

    } else if (sb->value == ' ' && ( strlen(inputline) < (COLS - 14) ) ) {         // SPACE
        add_char(inputline, ' ', inputline_pos);

    } else if (sb->value == 'r') {         // r
        curs_set(1);
        int c = get_key();
        if (c != -1) inputline[inputline_pos] = c;
        curs_set(2);

    } else if (sb->value == 'R') {         // R
        curs_set(1);
        int c = get_key();
        while (c != OKEY_ENTER && c != -1) {
            if (isprint(c)) {
                inputline[inputline_pos] = c;
                ++inputline_pos;
                show_header(main_win);
            }
            c = get_key();
        }
        curs_set(2);

    } else if (sb->value == 'f') {         // f
        int c = get_key();
        if (c != -1) inputline_pos = look_for(c);

    } else if (sb->value == 'd' || sb->value == 'c') {         // d or c
        int c, d;
        if ( (c = get_key()) != -1 ) {
             switch (c) {
             case 'e':                     // de or ce
                 del_range_chars(inputline, inputline_pos, for_word(1, 0, 0));
                 break;

             case 'E':                     // dE or cE
                 del_range_chars(inputline, inputline_pos, for_word(1, 0, 1));
                 break;

             case 'w':                     // dw or cw
                 del_range_chars(inputline, inputline_pos, for_word(0, 1, 0) - 1);
                 if (inputline_pos == strlen(inputline) && inputline_pos) inputline_pos--;
                 break;

             case 'W':                     // dW or cW
                 del_range_chars(inputline, inputline_pos, for_word(0, 1, 1) - 1);
                 if (inputline_pos == strlen(inputline) && inputline_pos) inputline_pos--;
                 break;

             case 'b':                     // db or cb
                 d = back_word(0);
                 del_range_chars(inputline, d, inputline_pos-1);
                 inputline_pos = d;
                 break;

             case 'B':                     // dB or cB
                 d = back_word(1);
                 del_range_chars(inputline, d, inputline_pos-1);
                 inputline_pos = d;
                 break;
  
             case 'l':                     // dl or cl
             case OKEY_RIGHT:
                 del_back_char();
                 break;

             case 'h':                     // dh or ch
             case OKEY_LEFT:
                 del_for_char();
                 break;

             case 'a':
                 if ( (d = get_key()) == 'W' ) {     // daW or caW
                     c = ( inputline_pos && inputline[inputline_pos-1] == ' ' ) ? inputline_pos : back_word(1);
                     del_range_chars(inputline, c, for_word(0, 1, 1) - 1);
                     inputline_pos = (strlen(inputline) > inputline_pos) ? c : strlen(inputline)-2;
                 } else if ( d == 'w' ) { // daw or caw
                     d = ( inputline_pos && ! istext( inputline[inputline_pos-1]) ) ? inputline_pos : back_word(0);
                     del_range_chars(inputline, d, for_word(0, 1, 0) - 1);
                     inputline_pos = (strlen(inputline) > inputline_pos) ? d : strlen(inputline)-2;
                 }
                 break;
             }
             if (sb->value == 'c') chg_mode(insert_edit_submode);
        }

    } else if (find_val(sb, OKEY_ENTER)) { // ENTER
        insert_or_edit_cell(); 
        return;
 
    } else if (sb->value == '$') {         // $
        inputline_pos = strlen(inputline) - 1;

    } else if (sb->value == 'w') {         // w
        inputline_pos = for_word(0, 0, 0);

    } else if (sb->value == 'W') {         // W
        inputline_pos = for_word(0, 0, 1);

    } else if (sb->value == 'e') {         // e
        inputline_pos = for_word(1, 0, 0);

    } else if (sb->value == 'E') {         // E
        inputline_pos = for_word(1, 0, 1);

    } else if (sb->value == 'b') {         // b
        inputline_pos = back_word(0);

    } else if (sb->value == 'B') {         // B
        inputline_pos = back_word(1);

    } else if (sb->value == '0') {         // 0
        inputline_pos = 0;

    } else if (sb->value == 'a') {         // a
        inputline_pos++;
        chg_mode(insert_edit_submode);

    } else if (sb->value == 'i' ||  sb->value == '=') {         // i o =
        chg_mode(insert_edit_submode);

    } else if (sb->value == 's') {         // s
        if (inputline_pos <= strlen(inputline)) del_char(inputline, inputline_pos);
        chg_mode(insert_edit_submode);

    } else if (sb->value == 'A') {         // A
        inputline_pos = strlen(inputline);
        chg_mode(insert_edit_submode);

    } else if (sb->value == 'I') {         // I
        inputline_pos = 0;
        chg_mode(insert_edit_submode);

    } else if (sb->value == 'D') {         // D
        inputline_pos = 0;
        inputline[0] = '\0';
        chg_mode(insert_edit_submode);
    }

    show_header(input_win);
    return;
}

// looks for a char in inputline
int look_for(char cb) {
    int c, cpos = inputline_pos;
    while (++cpos < strlen(inputline))
        if ((c = inputline[cpos]) && c == cb) return cpos;
    if (cpos > 0 && cpos == strlen(inputline)) return inputline_pos;
}

// move backwards a word
int back_word(int big_word) {
    int c, cpos = inputline_pos;
    if (inputline[cpos-1] == ' ' ) cpos--;

    while (cpos)
        if ((c = inputline[--cpos]) && c == ' ' ) return cpos+1;
        else if ( istext( inputline [cpos] ) && ! istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
        else if ( ! istext( inputline [cpos] ) && istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
    return cpos;
}

// end_of_word is used for moving forward to end of a WORD
// big_word looks for ' ', else looks for istext.
// delete 1 is used when typing dw command
int for_word(int end_of_word, int delete, int big_word) {
    int c, d, cpos = inputline_pos;

    if (! end_of_word) { // w or W
        while ( ++cpos < strlen(inputline) ) 
            if ( ! istext( inputline[cpos - 1]) && inputline[cpos] != ' ' && ! big_word ) return cpos; //agregado big_word el 08/06
            else if ( inputline[cpos] == ' ' ) return ++cpos;
            else if ( ! istext( inputline [cpos] ) && istext( inputline[cpos - 1] ) && ! big_word ) return cpos;
    } else {             // e or E
        if ( inputline[cpos+1] == ' ' ) cpos += 2;
        else if ( ! istext( inputline [cpos+1] ) && istext( inputline[cpos] ) && ! big_word ) cpos++;
        while ( ++cpos < strlen(inputline) ) 
            if ( ( inputline[cpos] == ' ' ) || ( ! istext( inputline [cpos] )
            && istext( inputline[cpos - 1] ) && ! big_word ) ) return --cpos;
    }

    if (cpos > 0 && cpos >= strlen(inputline)) return strlen(inputline) - 1 + delete;

    return 0;
}

int for_char() {
    if (inputline_pos < strlen(inputline) - 1)
        return ++inputline_pos;
    return inputline_pos;
}

int back_char() {
    if (inputline_pos) return --inputline_pos;
    return inputline_pos;
}

void del_back_char() {
    if (inputline_pos > strlen(inputline)) return;
    del_char(inputline, inputline_pos);
    if (inputline_pos == strlen(inputline) && strlen(inputline)) inputline_pos--;
}

void del_for_char() {
    if ( !strlen(inputline) || !inputline_pos ) { show_header(main_win); return; }
    del_char(inputline, --inputline_pos);
}

// return 1 OK
// return 0 error
int start_edit_mode(struct block * buf, char type) {
    chg_mode(buf->value);

    line[0]='\0';
    inputline[0]='\0';
    inputline_pos = 0;

    struct ent * p1 = lookat(currow, curcol);

    if (type == 'v') { // && p1->flags & is_valid) {   // numeric value
        if (( ! (p1->expr) ) ) {                       // || (p1->flags & is_strexpr)) {
            (void) sprintf(inputline, "%s%.15g", inputline, p1->v);
        } else {                   // expression
            linelim = 0;
            editexp(currow, curcol);
            linelim = -1;
            (void) sprintf(inputline, "%s", line);
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
        (void) sprintf(inputline, "%s", line);
    }

    return 1;
}
