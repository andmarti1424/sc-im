#include "cmds_insert.h"
#include "cmds.h"
#include "screen.h"
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include "buffer.h"
#include "sc.h"            // for rescol
#include "utils/string.h"
#include "marks.h"
#include "cmds_visual.h"

void do_insertmode(struct block * sb) {

    if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = insert_edit_submode;
        chg_mode('v');
        start_visualmode(currow, curcol, currow, curcol);
        return;

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) {
            real_inputline_pos--;
            int l = wcwidth(inputline[real_inputline_pos]);
            inputline_pos -= l;
            show_header(input_win);
        }

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        int max = wcswidth(inputline, wcslen(inputline));
        if (inputline_pos < max) {
            int l = wcwidth(inputline[real_inputline_pos++]);
            inputline_pos += l;
            show_header(input_win);
        }

    } else if (sb->value == OKEY_BS) {     // BS
        if ( ! wcslen(inputline) || ! real_inputline_pos ) return;

        int l = wcwidth(inputline[real_inputline_pos - 1]);
        real_inputline_pos--;
        del_wchar(inputline, real_inputline_pos);
        inputline_pos -= l;
        show_header(input_win);

    } else if (sb->value == OKEY_DEL) {    // DEL
        int max = wcswidth(inputline, wcslen(inputline));
        if (inputline_pos > max) return;
        del_wchar(inputline, real_inputline_pos);
        show_header(input_win);

    } else if (sb->value == OKEY_TAB) {    // TAB
        chg_mode('e');
        show_header(input_win);

    } else if (find_val(sb, OKEY_ENTER)) { // ENTER
        insert_or_edit_cell();

    } else if (sb->value == OKEY_HOME) {   // HOME
        real_inputline_pos = 0;
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        show_header(input_win);
        return;

    } else if (sb->value == OKEY_END) {    // END
        real_inputline_pos = wcslen(inputline);
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        show_header(input_win);
        return;

    // Write new char !!
    } else if ( wcslen(inputline) < (COLS - 16) && sc_isprint(sb->value)) {
        //DEBUG sc_info("2: %d %lc", sb->value, sb->value);
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
            sprintf(cline + strlen(cline), "%s%d", coltoa(get_mark(sb->pnext->value)->rng->brcol), get_mark(sb->pnext->value)->rng->brrow);
        }
        for(i = 0; i < strlen(cline); i++) ins_in_line(cline[i]);
        show_header(input_win);

    }
    return;
}
