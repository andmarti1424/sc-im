#include <string.h>
#include <ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>         // for isprint()
#include <wordexp.h>
#include "sc.h"            // for rescol
#include "color.h"         // for set_ucolor
#include "conf.h"
#include "cmds_command.h"
#include "cmds_edit.h"
#include "cmds.h"
#include "utils/string.h"
#include "utils/dictionary.h"
#include "tui.h"
#include "file.h"
#include "main.h"
#include "interp.h"
#include "hide_show.h"
#include "exec.h"
#include "help.h"
#include "marks.h"
#include "filter.h"
#include "maps.h"
#include "xls.h"
#include "xlsx.h"

#include "cmds_visual.h"

#ifdef UNDO
#include "undo.h"
#endif

extern char * rev;
extern struct dictionary * user_conf_d;

wchar_t inputline[BUFFERSIZE];
wchar_t interp_line[BUFFERSIZE];
int inputline_pos;          // This is the position in window. Some chars has 2 chars width
                            // \-> https://en.wikipedia.org/wiki/Halfwidth_and_fullwidth_forms
int real_inputline_pos;     // This is the real position in inputline

static wchar_t * valid_commands[] = {
L"!",
L"addfilter",
L"autojus",
L"cellcolor",
L"color",
L"e csv",
L"e tab",
L"e txt",
L"e xlsx",
L"e! csv",
L"e! tab",
L"e! txt",
L"e! xlsx",
L"datefmt",
L"delfilter",
L"delfilters",
L"file",
L"fill",
L"filteron",
L"filteroff",
L"format",
L"freeze",
L"h",
L"help",
L"hiddencols",
L"hiddenrows",
L"hidecol",
L"hiderow",
L"i csv",
L"i tab",
L"i xls",
L"i xlsx",
L"i! csv",
L"i! tab",
L"i! xls",
L"i! xlsx",
L"imap",
L"inoremap",
L"int",
L"iunmap",
L"load",
L"load!",
L"lock",
L"unfreeze",
L"unlock",
L"nmap",
L"nnoremap",
L"nunmap",
L"pad",
L"q",
L"q!",
L"quit!",
L"quit",
L"redefine_color",
L"refresh",
L"set",
L"showcol",
L"showfilters",
L"showmaps",
L"showrow",
L"showrows",
L"sort",
L"trigger",
L"untrigger",
L"unformat",
L"version",
L"w",
L"wq",
L"x",
L"valueize",
(wchar_t *) 0
};

void do_commandmode(struct block * sb) {

    // If a visual selected range exists
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);


    //-------------------------------------
    // Normal KEY handlers for this MODE
    //-------------------------------------
    if (sb->value == OKEY_BS || sb->value == OKEY_BS2) {  // BS
        if ( ! wcslen(inputline) || ! real_inputline_pos ) return;
        int l = wcwidth(inputline[real_inputline_pos - 1]);
        real_inputline_pos--;
        del_wchar(inputline, real_inputline_pos);
        inputline_pos -= l;
        ui_show_header();

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_wchar(get_line_from_history(commandline_history, commandline_history->pos), real_inputline_pos); // Clean history
#endif
        ui_show_header();
        return;

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) {
            real_inputline_pos--;
            int l = wcwidth(inputline[real_inputline_pos]);
            inputline_pos -= l;
            ui_show_header();
        }
        return;

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        int max = wcswidth(inputline, wcslen(inputline));
        if (inputline_pos < max) {
            int l = wcwidth(inputline[real_inputline_pos++]);
            inputline_pos += l;
            ui_show_header();
        }
        return;

    } else if (sb->value == OKEY_DEL) {    // DEL
        if (inputline_pos > wcswidth(inputline, wcslen(inputline))) return;
        del_wchar(inputline, real_inputline_pos);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_wchar(get_line_from_history(commandline_history, commandline_history->pos), real_inputline_pos); // Clean history
#endif
        ui_show_header();
        return;

#ifdef HISTORY_FILE
    } else if (sb->value == OKEY_UP || sb->value == ctl('p') ||         // UP
               sb->value == OKEY_DOWN || sb->value == ctl('n')) {       // DOWN

        int delta = 0;
        if (sb->value == OKEY_UP || sb->value == ctl('p')) {            // up
            if (commandline_history->len <= - commandline_history->pos + 1) return;
            delta = -1;
        }
        if (sb->value == OKEY_DOWN || sb->value == ctl('n')) {          // down
            if ( - commandline_history->pos == 0) return;
            delta = 1;
        }
        commandline_history->pos += delta;
        wcscpy(inputline, get_line_from_history(commandline_history, commandline_history->pos));
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;
#endif

    } else if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = ':';
        chg_mode('v');
        start_visualmode(currow, curcol, currow, curcol);
        return;








    } else if (sb->value == ctl('r') && get_bufsize(sb) == 2 &&        // C-r      // FIXME ???
        (sb->pnext->value - (L'a' - 1) < 1 || sb->pnext->value > 26)) {
        wchar_t cline [BUFFERSIZE];
        int i, r = get_mark(sb->pnext->value)->row;
        if (r != -1) {
            swprintf(cline, BUFFERSIZE, L"%s%d", coltoa(get_mark(sb->pnext->value)->col), r);
        } else {
            swprintf(cline, BUFFERSIZE, L"%s%d:", coltoa(get_mark(sb->pnext->value)->rng->tlcol), get_mark(sb->pnext->value)->rng->tlrow);
            swprintf(cline + wcslen(cline), BUFFERSIZE, L"%s%d", coltoa(get_mark(sb->pnext->value)->rng->brcol), get_mark(sb->pnext->value)->rng->brrow);
        }
        for(i = 0; i < wcslen(cline); i++) ins_in_line(cline[i]);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            wcscat(sl, cline);                        // Insert into history
        }
#endif
        ui_show_header();
        return;

    } else if (sb->value == ctl('f')) {               // C-f
        wchar_t cline [BUFFERSIZE];
        int i;
        struct ent * p1 = *ATBL(tbl, currow, curcol);
        if (! p1 || ! p1->format) {
            sc_error("cell has no format");
            return;
        }
        swprintf(cline, BUFFERSIZE, L"%s", p1->format);
        for (i = 0; i < wcslen(cline); i++) ins_in_line(cline[i]);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            wcscat(sl, cline);                        // Insert into history
        }
#endif
        ui_show_header();
        return;

    } else if ( sb->value == ctl('w') || sb->value == ctl('b') ||
                sb->value == OKEY_HOME || sb->value == OKEY_END) {
        switch (sb->value) {
        case ctl('w'):
            real_inputline_pos = for_word(1, 0, 1) + 1;   // E
            break;
        case ctl('b'):
            real_inputline_pos = back_word(1);            // B
            break;
        case OKEY_HOME:
            real_inputline_pos = 0;                       // 0
            break;
        case OKEY_END:
            real_inputline_pos = wcslen(inputline);       // $
            break;
        }
        inputline_pos = wcswidth(inputline, real_inputline_pos);
        ui_show_header();
        return;

    } else if (sb->value == '\t') {                  // TAB completion
        int i, clen = (sizeof(valid_commands) / sizeof(char *)) - 1;

        if (! get_comp()) copy_to_curcmd(inputline); // keep original cmd

        for (i = 0; i < clen; i++) {
            if ( ! wcscmp(inputline, valid_commands[i]) ) {
                wcscpy(inputline, get_curcmd());
                continue;
            }
            if ( ! wcsncmp(inputline, valid_commands[i], wcslen(inputline)) 
               ) {
                wcscpy(inputline, valid_commands[i]);
                real_inputline_pos = wcslen(inputline);
                inputline_pos = wcswidth(inputline, real_inputline_pos);
                set_comp(1);
                break;
            }
        }

        // Restore inputline content
        if (i == clen) {
            wcscpy(inputline, get_curcmd());
            real_inputline_pos = wcslen(inputline);
            inputline_pos = wcswidth(inputline, real_inputline_pos);
            set_comp(0);
        }

        ui_show_header();
        return;

    } else if (sc_isprint(sb->value)) {               //  Write new char
        ins_in_line(sb->value);
        ui_show_header();

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            wchar_t * sl = get_line_from_history(commandline_history, 0);
            add_wchar(sl, sb->value, real_inputline_pos-1); // Insert into history
        }
#endif
        return;


    //-------------------------------------
    // CONFIRM A COMMAND PRESSING ENTER
    //-------------------------------------
    } else if (find_val(sb, OKEY_ENTER)) {

        if ( ! wcscmp(inputline, L"refresh")) {
            winchg();

        } else if ( ! wcscmp(inputline, L"help") || ! wcscmp(inputline, L"h") ) {
            help();

        } else if ( ! wcscmp(inputline, L"q!") || ! wcscmp(inputline, L"quit!") ) {
            shall_quit = 2;

        } else if ( ! wcscmp(inputline, L"q") || ! wcscmp(inputline, L"quit") ) {
            shall_quit = 1;

        } else if ( ! wcsncmp(inputline, L"autojus", 7) ) {
            wchar_t cline [BUFFERSIZE];
            wcscpy(cline, inputline);
            int c = curcol, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                cf = sr->brcol;
            }
            if ( p != -1 || ! wcscmp(inputline, L"autojus")) {
                swprintf(cline, BUFFERSIZE, L"autojus %s:", coltoa(c));
                swprintf(cline + wcslen(cline), BUFFERSIZE, L"%s", coltoa(cf));
            }
            send_to_interp(cline);

        } else if ( ! wcsncmp(inputline, L"redefine_color", 14) ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"load", 4) ) {
            char cline [BUFFERSIZE];
            int force_rewrite = 0;
            wordexp_t p;
            wcstombs(cline, inputline, BUFFERSIZE);
            if ( ! wcsncmp(inputline, L"load! ", 6) ) {
                force_rewrite = 1;
                del_range_chars(cline, 4, 4);
            }

            del_range_chars(cline, 0, 4);
            wordexp(cline, &p, 0);
            if ( ! strlen(cline) ) {
                sc_error("Path to file to load is missing !");
            } else if ( p.we_wordc < 1 ) {
                sc_error("Failed expanding filepath");
            } else if ( modflg && ! force_rewrite ) {
                sc_error("Changes were made since last save. Use '!' to force the load");
            } else if ( ! file_exists(p.we_wordv[0])) {
                sc_error("File %s does not exists!", p.we_wordv[0]);
            } else {
                delete_structures();
                create_structures();
                readfile(p.we_wordv[0], 0);
                //EvalAll(); // is it necessary?
                modflg = 0;
                //update(TRUE);
            }
            wordfree(&p);

        } else if ( ! wcsncmp(inputline, L"hiderow ", 8) ||
                    ! wcsncmp(inputline, L"showrow ", 8) ||
                    ! wcsncmp(inputline, L"showcol ", 8) ||
                    ! wcsncmp(inputline, L"hidecol ", 8)
                  ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"showrows", 8) ) {
            if (p == -1) return; // If there is not selected range, go back.
            int r, arg;
            sr = get_range_by_pos(p);
            r = sr->tlrow;
            arg = sr->brcol - sr->tlcol + 1;
            show_row(r, arg);

        // range lock / unlock
        } else if ( ! wcsncmp(inputline, L"lock", 4) || ! wcsncmp(inputline, L"unlock", 6) ||
                    ! wcsncmp(inputline, L"valueize", 8) ) {
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if ( ! wcsncmp(inputline, L"lock", 4) ) lock_cells(lookat(r, c), lookat(rf, cf));
            else if ( ! wcsncmp(inputline, L"unlock", 6) ) unlock_cells(lookat(r, c), lookat(rf, cf));
            else if ( ! wcsncmp(inputline, L"valueize", 8) ) valueize_area(r, c, rf, cf);

        } else if ( ! wcsncmp(inputline, L"datefmt", 7)) {
            wcscpy(interp_line, inputline);

            if (p != -1) { // in case there is a range selected
                int r = currow, c = curcol, rf = currow, cf = curcol;
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                int found = wstr_in_wstr(interp_line, L"\"");
                if (found == -1) return;
                del_range_wchars(cline, 0, found-1);
                swprintf(interp_line, BUFFERSIZE, L"datefmt %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d %ls", coltoa(cf), rf, cline);
            }
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"sort ", 5) ) {
            wcscpy(interp_line, inputline);
            if (p != -1) {
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                int found = wstr_in_wstr(interp_line, L"\"");
                if (found == -1) return;
                del_range_wchars(cline, 0, found-1);
                swprintf(interp_line, BUFFERSIZE, L"sort %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d %ls", coltoa(sr->brcol), sr->brrow, cline);
            }
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"freeze ", 7) ) {
            wcscpy(interp_line, inputline);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"freeze", 6) ) {
            wcscpy(interp_line, inputline);
            if (p != -1) {
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                del_range_wchars(cline, 0, 5);
                swprintf(interp_line, BUFFERSIZE, L"freeze %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d %ls", coltoa(sr->brcol), sr->brrow, cline);
            }
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"unfreeze", 8) ) {
            wcscpy(interp_line, inputline);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"addfilter", 9) ) {
            wchar_t cline [BUFFERSIZE];
            char line [BUFFERSIZE];
            wcscpy(cline, inputline);
            int found = wstr_in_wstr(cline, L"\"");
            if (found == -1) return;
            del_range_wchars(cline, wcslen(cline), wcslen(cline));
            del_range_wchars(cline, 0, found);
            wcstombs(line, cline, BUFFERSIZE);
            add_filter(line);

        } else if ( ! wcsncmp(inputline, L"delfilter ", 10) ) {
            wchar_t cline [BUFFERSIZE];
            char line [BUFFERSIZE];
            wcscpy(cline, inputline);
            del_range_wchars(cline, 0, 9);
            wcstombs(line, cline, BUFFERSIZE);
            int id = atoi(line);
            del_filter(id);

        } else if ( ! wcsncmp(inputline, L"delfilters", 10) ) {
            free_filters();

        } else if ( ! wcsncmp(inputline, L"filteron", 8) ) {
            wcscpy(interp_line, inputline);
            if ( ! wcscmp(inputline, L"filteron") && p == -1) { // If there is no selected range
                sc_error("Please specify a range or select one");
                return;
            } else if (p != -1) {
                wchar_t cline [BUFFERSIZE];
                wcscpy(cline, interp_line);
                swprintf(interp_line, BUFFERSIZE, L"filteron %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(sr->brcol), sr->brrow);
            }
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"filteroff", 9) ) {
            disable_filters();

        } else if ( ! wcsncmp(inputline, L"hiddenrows", 10)) {
            show_hiddenrows();

        } else if ( ! wcsncmp(inputline, L"hiddencols", 10)) {
            show_hiddencols();

        } else if ( ! wcsncmp(inputline, L"showfilters", 11)) {
            show_filters();

        } else if ( ! wcsncmp(inputline, L"int ", 4) ) { // send cmd to interpreter
            wcscpy(interp_line, inputline);
            del_range_wchars(interp_line, 0, 3);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"fill ", 5) ) {
            interp_line[0]=L'\0';
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                swprintf(interp_line, BUFFERSIZE, L"fill %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(cf), rf);
            }

            if (any_locked_cells(r, c, rf, cf)) {
                swprintf(interp_line, BUFFERSIZE, L"");
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }

            if (p != -1)
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", &inputline[5]);
            else
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", inputline);
            send_to_interp(interp_line);

        } else if ( ! wcsncmp(inputline, L"format ", 7) ) {
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                swprintf(interp_line, BUFFERSIZE, L"fmt %s%d:", coltoa(c), r);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d", coltoa(cf), rf);
            } else
                swprintf(interp_line, BUFFERSIZE, L"fmt %s%d", coltoa(c), r);

            if (any_locked_cells(r, c, rf, cf)) {
                sc_error("Locked cells encountered. Nothing changed");
                return;
            }
            int l = wcslen(interp_line);
            swprintf(interp_line + l, BUFFERSIZE, L"%ls", inputline);
            del_range_wchars(interp_line, l, l + 5);
            #ifdef UNDO
            create_undo_action();
            copy_to_undostruct(r, c, rf, cf, 'd');
            #endif
            send_to_interp(interp_line);
            #ifdef UNDO
            copy_to_undostruct(r, c, rf, cf, 'a');
            end_undo_action();
            #endif

        } else if ( ! wcsncmp(inputline, L"cellcolor ", 10) ) {
            #ifdef USECOLORS
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 9);
            swprintf(interp_line, BUFFERSIZE, L"cellcolor ");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = L'\0';
            #endif

        } else if ( ! wcsncmp(inputline, L"unformat", 8) ) {
            #ifdef USECOLORS
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 7);
            swprintf(interp_line, BUFFERSIZE, L"unformat");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = L'\0';
            #endif

        } else if ( ! wcsncmp(inputline, L"color ", 6) ) {
            #ifdef USECOLORS
            char line [BUFFERSIZE];
            wcstombs(line, inputline, BUFFERSIZE);
            del_range_chars(line, 0, 5);
            chg_color(line);
            #else
            sc_error("Color support not compiled in");
            chg_mode('.');
            inputline[0] = '\0';
            #endif

        }  else if ( ! wcsncmp(inputline, L"trigger ", 8) ) {
            interp_line[0]=L'\0';
            wchar_t line [BUFFERSIZE];
            wcscpy(line, inputline);
            del_range_wchars(line, 0, 7);
            swprintf(interp_line, BUFFERSIZE, L"trigger ");
            if (p != -1) {
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s%d:", coltoa(sr->tlcol), sr->tlrow);
                swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d ", coltoa(sr->brcol), sr->brrow);
            }
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%ls", line);
            send_to_interp(interp_line);
        }  else if ( ! wcsncmp(inputline, L"untrigger ", 10) ) {
            wcscpy(interp_line, inputline);
            send_to_interp(interp_line);
        }  else if ( ! wcsncmp(inputline, L"set ", 4) ) {
            wcscpy(interp_line, inputline);
            send_to_interp(interp_line);
            sc_info("Config value changed: %s", line);

        } else if ( ! wcsncmp(inputline, L"pad ", 4) ) {
            int c = curcol, cf = curcol;
            if (p != -1) { // in case there is a range selected
                c = sr->tlcol;
                cf = sr->brcol;
            }
            wcscpy(interp_line, inputline); // pad 5
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L" %s:", coltoa(c)); // pad 5 A:
            swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s", coltoa(cf));  // B
            send_to_interp(interp_line);

        } else if ( ! wcscmp(inputline, L"set") ) {
            char valores[ (get_maxkey_length(user_conf_d) + get_maxvalue_length(user_conf_d) + 1) * user_conf_d->len ];
            get_conf_values(valores);
            show_text(valores);

        } else if ( ! wcscmp(inputline, L"version") ) {
            show_text(rev);

        } else if ( ! wcscmp(inputline, L"showmaps") ) {
            extern int len_maps;
            char valores[MAXMAPITEM * len_maps];
            get_mappings(valores);
            show_text(valores);

        } else if ( ! wcsncmp(inputline, L"nmap", 4) ||
                    ! wcsncmp(inputline, L"imap", 4) ||
                    ! wcsncmp(inputline, L"inoremap", 8) ||
                    ! wcsncmp(inputline, L"nnoremap", 8) ||
                    ! wcsncmp(inputline, L"iunmap", 6) ||
                    ! wcsncmp(inputline, L"nunmap", 6) ) {
            send_to_interp(inputline);

        } else if ( ! wcsncmp(inputline, L"!", 1) ) {
            char line [BUFFERSIZE];
            wcstombs(line, inputline, BUFFERSIZE);
            int found = str_in_str(line, " ");
            if (found == -1) found++;
            del_range_chars(line, 0, found);
            exec_cmd(line);

        } else if ( inputline[0] == L'w' ) {
            if (savefile() == 0 && ! wcscmp(inputline, L"wq")) shall_quit = 1;

        } else if ( ! wcsncmp(inputline, L"file ", 5) ) {

            char cline [BUFFERSIZE];
            wordexp_t p;

            wcstombs(cline, inputline, BUFFERSIZE);
            del_range_chars(cline, 0, 4);
            wordexp(cline, &p, 0);

            if ( p.we_wordc < 1 ) {
                sc_error("Failed to expand filename");
            } else {
                strncpy(curfile, p.we_wordv[0], PATHLEN - 1);
                sc_info("File name set to \"%s\"", curfile);
            }

            wordfree(&p);

        } else if ( ! wcscmp(inputline, L"file") ) {

            if( ! *curfile ) {
                sc_info("Current file has no name");
            } else {
                sc_info("Current file: \"%s\"", curfile);
            }

        } else if ( inputline[0] == L'x' ) {
            if ( savefile() == 0 ) shall_quit = 1;

        } else if ( ! wcscmp(inputline, L"fcopy") ) {
            fcopy();

        } else if ( ! wcscmp(inputline, L"fsum") ) {
            fsum();

        } else if (
                ! wcsncmp(inputline, L"e csv"  , 5) ||
                ! wcsncmp(inputline, L"e! csv" , 6) ||
            ! wcsncmp(inputline, L"e tab"  , 5) ||
            ! wcsncmp(inputline, L"e! tab" , 6) ||
            ! wcsncmp(inputline, L"e txt" , 5) ||
            ! wcsncmp(inputline, L"e! txt" , 6) ) {
                do_export( p == -1 ? 0 : sr->tlrow, p == -1 ? 0 : sr->tlcol,
                p == -1 ? maxrow : sr->brrow, p == -1 ? maxcol : sr->brcol);

        } else if (
                ! wcsncmp(inputline, L"e xlsx"  , 6) ||
                ! wcsncmp(inputline, L"e! xlsx" , 7)) {
                #ifndef XLSX_EXPORT
                sc_error("XLSX export support not compiled in");
                #else
                char linea[BUFFERSIZE];
                char filename[PATHLEN] = "";
                int force_rewrite = 0;
                if (inputline[1] == L'!') force_rewrite = 1;
                wcstombs(linea, inputline, BUFFERSIZE); // Use new variable to keep command history untouched
                del_range_chars(linea, 0, 1 + force_rewrite); // Remove 'e' or 'e!' from inputline

                // Get path and file name to write.
                // Use parameter if any.
                if (strlen(linea) > 5) {   // ex. 'xlsx '
                    del_range_chars(linea, 0, 4); // remove 'xlsx'
                    strcpy(filename, linea);
                    // Use curfile name and '.xlsx' extension
                    // Remove current '.sc' extension if necessary
                } else if (curfile[0]) {
                    strcpy(filename, curfile);
                    char * ext = strrchr(filename, '.');
                    if (ext != NULL) del_range_chars(filename, strlen(filename) - strlen(ext), strlen(filename)-1);
                    sprintf(filename + strlen(filename), ".xlsx");
                } else {
                    sc_error("No filename specified !");
                }

                if (strlen(filename) > 0 && ! force_rewrite && file_exists(filename)) {
                    sc_error("File %s already exists. Use \"!\" to force rewrite.", filename);
                } else if (strlen(filename) && export_xlsx(
                    filename, p == -1 ? 0 : sr->tlrow, p == -1 ? 0 : sr->tlcol,
                    p == -1 ? maxrow : sr->brrow, p == -1 ? maxcol : sr->brcol) == 0)
                    sc_info("File \"%s\" written", filename);
                #endif

        } else if (
            ! wcsncmp(inputline, L"i csv " , 6) ||
            ! wcsncmp(inputline, L"i! csv ", 7) ||
            ! wcsncmp(inputline, L"i xlsx" , 6) ||
            ! wcsncmp(inputline, L"i! xlsx", 7) ||
            ! wcsncmp(inputline, L"i xls " , 6) ||
            ! wcsncmp(inputline, L"i! xls ", 7) ||
            ! wcsncmp(inputline, L"i tab " , 6) ||
            ! wcsncmp(inputline, L"i! tab ", 7) ) {

            int force_rewrite = 0;
            char delim = ',';
            char cline [BUFFERSIZE];
            wcstombs(cline, inputline, BUFFERSIZE);

            if (inputline[1] == '!') {
                force_rewrite = 1;
                del_range_chars(cline, 1, 1);
            }

            if ( ! strncmp(cline, "i csv ", 6) ) { delim = ',';
            } else if ( ! strncmp(cline, "i tab ", 6) ) { delim = '\t';
            } else if ( ! strncmp(cline, "i xls ", 6) ) {
                #ifndef XLS
                sc_error("XLS import support not compiled in");
                chg_mode('.');
                inputline[0] = L'\0';
                //update(TRUE);
                //return;
                #endif
                delim = 'x';
            } else if ( ! strncmp(cline, "i xlsx", 6) ) {
                #ifndef XLSX
                sc_error("XLSX import support not compiled in");
                chg_mode('.');
                inputline[0]= L'\0';
                //update(TRUE);
                //return;
                #endif
                delim = 'y';
            }

            del_range_chars(cline, 0, 5);

            if ( ! strlen(cline) ) {
                sc_error("Path to file to import is missing !");
            } else if ( modflg && ! force_rewrite ) {
                sc_error("Changes were made since last save. Save current file or use '!' to force the import.");
            } else {
                delete_structures();
                create_structures();
                if (delim == 'x') {           // xls import
                #ifdef XLS
                    open_xls(cline, "UTF-8");
                #endif
                } else if (delim == 'y') {    // xlsx import
                #ifdef XLSX
                    del_range_chars(cline, 0, 0);
                    open_xlsx(cline, "UTF-8");
                #endif
                } else {
                    import_csv(cline, delim); // csv or tab delim import
                }
                modflg = 0;
                update(TRUE);
            }
        } else {
            sc_error("COMMAND NOT FOUND !");
        }

#ifdef HISTORY_FILE
        // if exists in history an item with same text to the command typed
        // (counting from the second position) it is moved to the beginning of list.
        // (first element in list means last command executed)
        del_item_from_history(commandline_history, 0);
        int moved = move_item_from_history_by_str(commandline_history, inputline, -1);
        if (! moved) add(commandline_history, inputline);
        commandline_history->pos = 0;
#endif

        chg_mode('.');
        inputline[0]=L'\0';
        set_comp(0); // unmark tab completion
        update(TRUE);
    }
    return;
}

void ins_in_line(wint_t d) {
    add_wchar(inputline, (wchar_t) d, real_inputline_pos++);
    inputline_pos += wcwidth((wchar_t) d);
    return;
}
