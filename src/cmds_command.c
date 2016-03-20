#include <string.h>
#include <curses.h>
#include <stdlib.h>
#include <ctype.h>         // for isprint()
#include "sc.h"            // for rescol
#include "color.h"         // for set_ucolor
#include "conf.h"
#include "cmds_command.h"
#include "cmds_edit.h"
#include "cmds.h"
#include "utils/string.h"
#include "utils/dictionary.h"
#include "screen.h"
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

char inputline[BUFFERSIZE];
int inputline_pos;
char interp_line[100];

static char * valid_commands[] = {
"!",
"addfilter",
"autojus",
"cellcolor",
"color",
"e csv",
"e tab",
"e txt",
"e! csv",
"e! tab",
"e! txt",
"datefmt",
"delfilter",
"delfilters",
"filteron",
"filteroff",
"format",
"h",
"help",
"hiddencols",
"hiddenrows",
"hidecol",
"hiderow",
"i csv",
"i tab",
"i xls",
"i xlsx",
"i! csv",
"i! tab",
"i! xls",
"i! xlsx",
"imap",
"inoremap",
"int",
"iunmap",
"load",
"load!",
"lock",
"unlock",
"nmap",
"nnoremap",
"nunmap",
"q",
"q!",
"quit!",
"quit",
"refresh",
"set",
"showcol",
"showfilters",
"showmaps",
"showrow",
"showrows",
"sort",
"version",
"w",
"x",
"valueize",
(char *) 0
};

void do_commandmode(struct block * sb) {

    // If a visual selected range exists
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);

    if (sb->value == OKEY_BS) {            // BS
        if ( ! strlen(inputline) || ! inputline_pos) return;
        del_char(inputline, --inputline_pos);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_char(get_line_from_history(commandline_history, commandline_history->pos), inputline_pos); // Clean history
#endif
        show_header(input_win);
        return;

    } else if (sb->value == OKEY_DEL) {    // DEL
        if (inputline_pos > strlen(inputline)) return;
        del_char(inputline, inputline_pos);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0)
            del_char(get_line_from_history(commandline_history, commandline_history->pos), inputline_pos); // Clean history
#endif
        show_header(input_win);
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
        strcpy(inputline, get_line_from_history(commandline_history, commandline_history->pos));
        inputline_pos = strlen(inputline);
        show_header(input_win);
        return;
#endif

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) inputline_pos--;
        show_header(input_win);
        return;

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        if (inputline_pos < strlen(inputline)) inputline_pos++;
        show_header(input_win);
        return;

    } else if (sb->value == ctl('v') ) {  // VISUAL SUBMODE
        visual_submode = ':';
        chg_mode('v');
        start_visualmode(currow, curcol, currow, curcol);
        return;

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

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            char * sl = get_line_from_history(commandline_history, 0);
            strcat(sl, cline);                        // Insert into history
        }
#endif
        show_header(input_win);
        return;

    } else if (sb->value == ctl('f')) { // C-f
        char cline [BUFFERSIZE];
        int i;
        struct ent * p1 = *ATBL(tbl, currow, curcol);
        if (! p1 || ! p1->format) {
            scerror("cell has no format");
            return;
        }
        sprintf(cline, "%s", p1->format);
        for (i = 0; i < strlen(cline); i++) ins_in_line(cline[i]);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            char * sl = get_line_from_history(commandline_history, 0);
            strcat(sl, cline);                        // Insert into history
        }
#endif
        show_header(input_win);
        return;

    } else if (isprint(sb->value)) {       //  Write new char
        ins_in_line(sb->value);
        mvwprintw(input_win, 0, 0 + rescol, ":%s", inputline);
        wmove(input_win, 0, inputline_pos + 1 + rescol);
        wrefresh(input_win);

#ifdef HISTORY_FILE
        if (commandline_history->pos == 0) {          // Only if editing the new command
            char * sl = get_line_from_history(commandline_history, 0);
            add_char(sl, sb->value, inputline_pos-1); // Insert into history
        }
#endif
        return;

    } else if ( sb->value == ctl('w') || sb->value == ctl('b') ||
                sb->value == OKEY_HOME || sb->value == OKEY_END) {
        switch (sb->value) {
        case ctl('w'):
            inputline_pos = for_word(1, 0, 1) + 1;   // E
            break;
        case ctl('b'):
            inputline_pos = back_word(1);            // B
            break;
        case OKEY_HOME:
            inputline_pos = 0;                       // 0
            break;
        case OKEY_END:
            inputline_pos = strlen(inputline);       // $
            break;
        }
        wmove(input_win, 0, inputline_pos + 1 + rescol);
        wrefresh(input_win);
        return;

    } else if (sb->value == '\t') {                  // TAB completion
        int i, clen = (sizeof(valid_commands) / sizeof(char *)) - 1;

        if (! get_comp()) copy_to_curcmd(inputline); // keep original cmd

        for (i = 0; i < clen; i++) {
            if ( ! strcmp(inputline, valid_commands[i]) ) {
                strcpy(inputline, get_curcmd());
                continue;
            }
            if ( ! strncmp(inputline, valid_commands[i], strlen(inputline)) 
               ) {
                strcpy(inputline, valid_commands[i]);
                inputline_pos = strlen(inputline);
                set_comp(1);
                break;
            }
        }

        // Restore inputline content
        if (i == clen) {
            //strcpy(inputline, get_line_from_history(commandline_history, 0));
            strcpy(inputline, get_curcmd());
            inputline_pos = strlen(inputline);
            set_comp(0);
        }

        show_header(input_win);
        return;

    // CONFIRM A COMMAND PRESSING ENTER
    } else if (find_val(sb, OKEY_ENTER)) {

        if ( ! strcmp(inputline, "q") || ! strcmp(inputline, "quit") ) {
            shall_quit = 1;

        } else if ( ! strcmp(inputline, "refresh")) {
            winchg();

        } else if ( ! strcmp(inputline, "q!") || ! strcmp(inputline, "quit!") ) {
            shall_quit = 2;

        } else if ( ! strcmp(inputline, "help") || ! strcmp(inputline, "h") ) {
            help();

        } else if ( ! strncmp(inputline, "autojus", 7) ) {
            char cline [BUFFERSIZE];
            strcpy(cline, inputline);
            int c = curcol, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                cf = sr->brcol;
            }
            if ( p != -1 || ! strcmp(inputline, "autojus")) {
                sprintf(cline, "autojus %s:", coltoa(c));
                sprintf(cline + strlen(cline), "%s", coltoa(cf));
            }
            send_to_interp(cline); 

        } else if ( ! strncmp(inputline, "load", 4) ) {
            char cline [BUFFERSIZE];
            int force_rewrite = 0;
            strcpy(cline, inputline);

            if ( ! strncmp(inputline, "load! ", 6) ) {
                force_rewrite = 1;
                del_range_chars(cline, 4, 4);
            }

            del_range_chars(cline, 0, 4);
            if ( ! strlen(cline) ) {
                scerror("Path to file to load is missing !");
            } else if ( modflg && ! force_rewrite ) {
                scerror("Changes were made since last save. Use '!' to force the load");
            } else {
                delete_structures();
                create_structures();
                readfile(cline, 0);
                //EvalAll(); // is it necessary?
                modflg = 0;
                update(TRUE); 
            }

        } else if ( ! strncmp(inputline, "hiderow ", 8) ||
                    ! strncmp(inputline, "showrow ", 8) ||
                    ! strncmp(inputline, "showcol ", 8) ||
                    ! strncmp(inputline, "hidecol ", 8)
                  ) {
            send_to_interp(inputline); 

        } else if ( ! strncmp(inputline, "showrows", 8) ) {
            if (p == -1) return; // If there is not selected range, go back.
            int r, arg;
            sr = get_range_by_pos(p);
            r = sr->tlrow;
            arg = sr->brcol - sr->tlcol + 1;
            show_row(r, arg);

        // range lock / unlock
        } else if ( ! strncmp(inputline, "lock", 4) || ! strncmp(inputline, "unlock", 6) ||
                    ! strncmp(inputline, "valueize", 8) ) {
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }
            if ( ! strncmp(inputline, "lock", 4) ) lock_cells(lookat(r, c), lookat(rf, cf));
            else if ( ! strncmp(inputline, "unlock", 6) ) unlock_cells(lookat(r, c), lookat(rf, cf));
            else if ( ! strncmp(inputline, "valueize", 8) ) valueize_area(r, c, rf, cf);

        } else if ( ! strncmp(inputline, "datefmt", 7)) {
            strcpy(interp_line, inputline);

            if (p != -1) { // in case there is a range selected
                int r = currow, c = curcol, rf = currow, cf = curcol;
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                char cline [BUFFERSIZE];
                strcpy(cline, interp_line);
                int found = str_in_str(interp_line, "\"");
                if (found == -1) return;
                del_range_chars(cline, 0, found-1);
                sprintf(interp_line, "datefmt %s%d:", coltoa(c), r);
                sprintf(interp_line + strlen(interp_line), "%s%d %s", coltoa(cf), rf, cline);
            }
            send_to_interp(interp_line);

        } else if ( ! strncmp(inputline, "sort ", 5) ) {
            strcpy(interp_line, inputline);
            if (p != -1) {
                char cline [BUFFERSIZE];
                strcpy(cline, interp_line);
                int found = str_in_str(interp_line, "\"");
                if (found == -1) return;
                del_range_chars(cline, 0, found-1);
                sprintf(interp_line, "sort %s%d:", coltoa(sr->tlcol), sr->tlrow);
                sprintf(interp_line + strlen(interp_line), "%s%d %s", coltoa(sr->brcol), sr->brrow, cline);
            }
            send_to_interp(interp_line); 

        } else if ( ! strncmp(inputline, "addfilter", 9) ) {
            char cline [BUFFERSIZE];
            strcpy(cline, inputline);
            int found = str_in_str(cline, "\"");
            if (found == -1) return;
            del_range_chars(cline, strlen(cline), strlen(cline));
            del_range_chars(cline, 0, found);
            add_filter(cline);

        } else if ( ! strncmp(inputline, "delfilter ", 10) ) {
            char cline [BUFFERSIZE];
            strcpy(cline, inputline);
            del_range_chars(cline, 0, 9);
            int id = atoi(cline);
            del_filter(id);

        } else if ( ! strncmp(inputline, "delfilters", 10) ) {
            free_filters();

        } else if ( ! strncmp(inputline, "filteron", 8) ) { //FIXME
            strcpy(interp_line, inputline);
            if ( ! strcmp(inputline, "filteron") && p == -1) { // If there is no selected range
                scerror("Please specify a range or select one");
                return;
            } else if (p != -1) {
                char cline [BUFFERSIZE];
                strcpy(cline, interp_line);
                sprintf(interp_line, "filteron %s%d:", coltoa(sr->tlcol), sr->tlrow);
                sprintf(interp_line + strlen(interp_line), "%s%d", coltoa(sr->brcol), sr->brrow);
            }
            send_to_interp(interp_line); 

        } else if ( ! strncmp(inputline, "filteroff", 9) ) {
            disable_filters();

        } else if ( ! strncmp(inputline, "hiddenrows", 10)) {
            show_hiddenrows();

        } else if ( ! strncmp(inputline, "hiddencols", 10)) {
            show_hiddencols();

        } else if ( ! strncmp(inputline, "showfilters", 11)) {
            show_filters();

        } else if ( ! strncmp(inputline, "int ", 4) ) { // send cmd to interpreter
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 3);
            send_to_interp(interp_line);

        } else if ( ! strncmp(inputline, "format ", 7) ) {
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                sprintf(interp_line, "fmt %s%d:", coltoa(c), r);
                sprintf(interp_line +strlen(interp_line), "%s%d", coltoa(cf), rf);
            } else
                sprintf(interp_line, "fmt %s%d", coltoa(c), r);

            if (any_locked_cells(r, c, rf, cf)) {
                scerror("Locked cells encountered. Nothing changed");
                return;
            }
            int l = strlen(interp_line);
            sprintf(interp_line + l, "%s", inputline);
            del_range_chars(interp_line, l, l + 6);
            #ifdef UNDO
            create_undo_action();
            copy_to_undostruct(r, c, rf, cf, 'd');
            #endif
            send_to_interp(interp_line);
            #ifdef UNDO
            copy_to_undostruct(r, c, rf, cf, 'a');
            end_undo_action();
            #endif

        } else if ( ! strncmp(inputline, "cellcolor ", 10) ) {
            #ifdef USECOLORS
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
            }

            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 9);
            color_cell(r, c, rf, cf, interp_line);
            #else
            scerror("Color support not compiled in");
            chg_mode('.');
            inputline[0]='\0';
            update(TRUE);
            return;
            #endif

        } else if ( ! strncmp(inputline, "color ", 6) ) {
            #ifdef USECOLORS
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 5);
            chg_color(interp_line);
            #else
            scerror("Color support not compiled in");
            chg_mode('.');
            inputline[0]='\0';
            update(TRUE);
            return;
            #endif

        } else if ( ! strncmp(inputline, "set ", 4) ) {
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 3); 
            parse_str(user_conf_d, interp_line);
            scinfo("Config value changed: %s", interp_line);

        } else if ( ! strcmp(inputline, "set") ) {
            int d = user_conf_d->len;
            char valores[20 * d];
            get_conf_values(valores);
            show_text(valores);

        } else if ( ! strcmp(inputline, "version") ) {
            show_text(rev);

        } else if ( ! strcmp(inputline, "showmaps") ) {
            extern int len_maps;
            char valores[MAXMAPITEM * len_maps];
            get_mappings(valores);
            show_text(valores);

        } else if ( ! strncmp(inputline, "nmap", 4) ||
                    ! strncmp(inputline, "imap", 4) ||
                    ! strncmp(inputline, "inoremap", 8) ||
                    ! strncmp(inputline, "nnoremap", 8) ||
                    ! strncmp(inputline, "iunmap", 6) ||
                    ! strncmp(inputline, "nunmap", 6) ) {
            send_to_interp(inputline);

        } else if ( ! strncmp(inputline, "!", 1) ) {
            del_range_chars(inputline, 0, 0);
            exec_cmd(inputline);

        } else if ( inputline[0] == 'w' ) {
            savefile();

        } else if ( inputline[0] == 'x' ) {
            if ( savefile() == 0 ) shall_quit = 1;

        } else if (
            ! strncmp(inputline, "e csv"  , 5) ||
            ! strncmp(inputline, "e! csv" , 6) ||
            ! strncmp(inputline, "e tab"  , 5) ||
            ! strncmp(inputline, "e! tab" , 6) ||
            ! strncmp(inputline, "e txt" , 5) ||
            ! strncmp(inputline, "e! txt" , 6) ) {
                do_export( p == -1 ? 0 : sr->tlrow, p == -1 ? 0 : sr->tlcol,
                p == -1 ? maxrow : sr->brrow, p == -1 ? maxcol : sr->brcol);

        } else if (
            ! strncmp(inputline, "i csv " , 6) ||
            ! strncmp(inputline, "i! csv ", 7) ||
            ! strncmp(inputline, "i xlsx" , 6) ||
            ! strncmp(inputline, "i! xlsx", 7) ||
            ! strncmp(inputline, "i xls " , 6) ||
            ! strncmp(inputline, "i! xls ", 7) ||
            ! strncmp(inputline, "i tab " , 6) ||
            ! strncmp(inputline, "i! tab ", 7) ) {

            int force_rewrite = 0;
            char delim = ',';
            char cline [BUFFERSIZE];
            strcpy(cline, inputline);

            if (inputline[1] == '!') {
                force_rewrite = 1;
                del_range_chars(cline, 1, 1);
            }

            if ( ! strncmp(cline, "i csv ", 6) ) { delim = ',';
            } else if ( ! strncmp(cline, "i tab ", 6) ) { delim = '\t';
            } else if ( ! strncmp(cline, "i xls ", 6) ) {
                #ifndef XLS
                scerror("XLS import support not compiled in");
                chg_mode('.');
                inputline[0]='\0';
                update(TRUE);
                return;
                #endif
                delim = 'x';
            } else if ( ! strncmp(cline, "i xlsx", 6) ) {
                #ifndef XLSX
                scerror("XLSX import support not compiled in");
                chg_mode('.');
                inputline[0]='\0';
                update(TRUE);
                return;
                #endif
                delim = 'y';
            }

            del_range_chars(cline, 0, 5);

            if ( ! strlen(cline) ) {
                scerror("Path to file to import is missing !");
            } else if ( modflg && ! force_rewrite ) {
                scerror("Changes were made since last save. Save current file or use '!' to force the import.");
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
            scerror("COMMAND NOT FOUND !");
        }

#ifdef HISTORY_FILE
        del_item_from_history(commandline_history, 0);
        // si hay en historial algun item con texto igual al del comando que se ejecuta
        // a partir de la posicion 2, se lo coloca al comiendo de la lista (indica que es lo mas reciente ejecutado).
        int moved = move_item_from_history_by_str(commandline_history, inputline, -1);
        if (!moved) add(commandline_history, inputline);
        commandline_history->pos = 0;
#endif

        chg_mode('.');
        // clr_header(input_win); // COMENTADO el dia 22/06
        inputline[0]='\0';
        set_comp(0); // unmark tab completion
        update(TRUE);
    }
    //show_header(input_win); // NO DESCOMENTAR.
    return;
}

void ins_in_line(int d) {
    //scerror(">>%d             ", d);
    //if (d < 256 && d > 31) // > 0
        add_char(inputline, (char) d, inputline_pos++);
    return;
}
