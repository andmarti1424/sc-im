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
#include "conf.h"

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

    } else if (sb->value == OKEY_BS || sb->value == OKEY_BS2) {  // BS
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

        enter_cell_content(currow, curcol, ope, content);

        inputline[0] = L'\0';
        inputline_pos = 0;
        real_inputline_pos = 0;
        chg_mode('.');
        clr_header(input_win, 0);

        char * opt = get_conf_value("newline_action");
        switch (opt[0]) {
            case 'j':
                currow = forw_row(1)->row;
                break;
            case 'l':
                curcol = forw_col(1)->col;
                break;
        }
        update(TRUE);
        return;

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
