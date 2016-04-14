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

// used for wchar_t that has more than 1 column width
int get_real_inputline_pos() {
    int pos;
    int sum = 0;
    for (pos = 0; pos < wcslen(inputline) && sum < inputline_pos; pos++) {
        sum += wcwidth(inputline[pos]);
    }
    return pos;
}

void do_insertmode(struct block * sb) {

    if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = insert_edit_submode;
        chg_mode('v');
        start_visualmode(currow, curcol, currow, curcol);
        return;

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) {
            int l = wcwidth(inputline[real_inputline_pos-1]);
            real_inputline_pos--;
            inputline_pos -= l;
            show_header(input_win);
        }

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        int max = wcswidth(inputline, wcslen(inputline));
        int l = wcwidth(inputline[real_inputline_pos++]);
        if (inputline_pos <= max) inputline_pos += l;
        show_header(input_win);

    } else if (sb->value == OKEY_BS) {     // BS
        if ( ! wcslen(inputline) ) return;

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

//    } else {
//        move(0, rescol + inputline_pos + 1);
//        show_header(input_win);
    }

    return;
}
