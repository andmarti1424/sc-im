#include "cmds_insert.h"
#include "cmds.h"
#include "screen.h"
#include <string.h>
#include <ctype.h>         // for isprint()
#include "buffer.h"
#include "sc.h"            // for rescol
#include "utils/string.h"
#include "marks.h"

void do_insertmode(struct block * sb) {

    if (sb->value == OKEY_LEFT) {          // LEFT
        if (inputline_pos) inputline_pos--;
        show_header(input_win);

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        if (inputline_pos < strlen(inputline)) inputline_pos++;
        show_header(input_win);

    } else if (sb->value == OKEY_BS) {     // BS
        if ( !strlen(inputline) || !inputline_pos ) return;
        del_char(inputline, --inputline_pos);
        show_header(input_win);

    } else if (sb->value == OKEY_DEL) {    // DEL
        if (inputline_pos > strlen(inputline)) return;
        del_char(inputline, inputline_pos);
        show_header(input_win);

    } else if (sb->value == OKEY_TAB) {    // TAB
        chg_mode('e');
        show_header(input_win);

    } else if (find_val(sb, OKEY_ENTER)) { // ENTER
        insert_or_edit_cell(); 

    } else if ( strlen(inputline) < (COLS - 14) && isprint(sb->value)) { //  ESCRIBO UN NUEVO CHAR
        ins_in_line(sb->value);
        show_header(input_win);

    } else if (sb->value == ctl('r') && get_bufsize(sb) == 2 && // C-r
        (sb->pnext->value - ('a' - 1) < 1 || sb->pnext->value > 26)) {
        char cline [BUFFERSIZE];
        int i, r = get_mark(sb->pnext->value)->row;
        if (r != -1) {
            sprintf(cline, "%s%d", coltoa(get_mark(sb->pnext->value)->col), r);
        } else {
            sprintf(cline, "%s%d:", coltoa(get_mark(sb->pnext->value)->rng->tlcol), get_mark(sb->pnext->value)->rng->tlrow);
            sprintf(cline, "%s%s%d", cline, coltoa(get_mark(sb->pnext->value)->rng->brcol), get_mark(sb->pnext->value)->rng->brrow);
        }
        for(i = 0; i < strlen(cline); i++) ins_in_line(cline[i]);
        show_header(input_win);

    } else {
        move(0, rescol + inputline_pos + 1);
        show_header(input_win);
    }

    return;
}
