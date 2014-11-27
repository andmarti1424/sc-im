#include "cmds_insert.h"
#include "cmds.h"
#include "stdout.h"
#include <string.h>
#include "buffer.h"
#include "sc.h"            // for rescol

void do_insertmode(struct block * sb) {

    if (sb->value == OKEY_LEFT) {          // LEFT
        if (inputline_pos) inputline_pos--;
        show_header(input_win);

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        if (inputline_pos < strlen(inputline)) inputline_pos++;
        show_header(input_win);

    } else if (sb->value == OKEY_BS) {     // BS
        if ( !strlen(inputline) || !inputline_pos ) { show_header(main_win); return; }
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

    } else {
        move(0, rescol + inputline_pos + 1);
        show_header(input_win);
    }

    return;
}
