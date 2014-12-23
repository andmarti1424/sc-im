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
#include "range.h"
#include "screen.h"
#include "file.h"
#include "main.h"
#include "undo.h"
#include "hide_show.h"
#include "exec.h"
#include "help.h"

extern char * rev;
extern struct dictionary * user_conf_d;

char inputline[BUFFERSIZE];
int inputline_pos;
char interp_line[100];

static char * valid_commands[] = {
"!",
"color",
"e csv",
"e tab",
"e! csv",
"e! tab",
"format",
"h",
"help",
"hiddencols",
"hiddenrows",
"hidecol",
"hiderow",
"i csv",
"i tab",
"i! csv",
"i! tab",
"int",
"load",
"load!",
"q",
"q!",
"quit!",
"quit",
"set",
"showcol",
"showrow",
"showrows",
"sort",
"version",
"w",
"x",
(char *) 0
};

static int tab_comp = -1; // tab_comp != -1 indicates tab_completation

void do_commandmode(struct block * sb) {

    // si hay un rango seleccionado en modo visual
    int p = is_range_selected();
    struct srange * sr = NULL;
    if (p != -1) sr = get_range_by_pos(p);
    
    if (sb->value == OKEY_BS) {            // BS
        if (! strlen(inputline) || ! inputline_pos) {
            //show_header(input_win);
            return;
        }
        del_char(inputline, --inputline_pos);
        if (commandline_history->pos == 0)
            del_char(get_line_from_history(commandline_history, commandline_history->pos), inputline_pos); // borro en el historial
        show_header(input_win);

    } else if (sb->value == OKEY_DEL) {    // DEL
        if (inputline_pos > strlen(inputline)) return;
        del_char(inputline, inputline_pos);
        if (commandline_history->pos == 0)
            del_char(get_line_from_history(commandline_history, commandline_history->pos), inputline_pos); // borro en el historial
        show_header(input_win);

    } else if (sb->value == OKEY_UP || sb->value == ctl('p')) {     // UP
        if (commandline_history->len > - commandline_history->pos + 1) {
            commandline_history->pos--;
            strcpy(inputline, get_line_from_history(commandline_history, commandline_history->pos));
            inputline_pos = strlen(inputline);
        }
        show_header(input_win);
            
    } else if (sb->value == OKEY_DOWN || sb->value == ctl('n')) {   // DOWN
        if ( - commandline_history->pos) {
            commandline_history->pos++;
            strcpy(inputline, get_line_from_history(commandline_history, commandline_history->pos));
            inputline_pos = strlen(inputline);
        }
        show_header(input_win);

    } else if (sb->value == OKEY_LEFT) {   // LEFT
        if (inputline_pos) inputline_pos--;
        show_header(input_win);

    } else if (sb->value == OKEY_RIGHT) {  // RIGHT
        if (inputline_pos < strlen(inputline)) inputline_pos++;
        show_header(input_win);

    } else if (isprint(sb->value)) {       //  ESCRIBO UN NUEVO CHAR
        ins_in_line(sb->value);
        mvwprintw(input_win, 0, 0 + rescol, ":%s", inputline);
        wmove(input_win, 0, inputline_pos + 1 + rescol);
        wrefresh(input_win);
        
        //if (sb->value < 256 && sb->value > 31 && commandline_history->pos == 0) { // solo si edito el nuevo comando
        if (commandline_history->pos == 0) { // solo si edito el nuevo comando
            ;
            char * sl = get_line_from_history(commandline_history, 0);
            add_char(sl, sb->value, inputline_pos-1); // Inserto en el historial
        }

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

    } else if (sb->value == '\t') {                  // TAB
        int i, clen = (sizeof(valid_commands) / sizeof(char *)) - 1;
    
        for (i = tab_comp + 1; i < clen; i++) {
            if (strncmp(get_line_from_history(commandline_history, 0), valid_commands[i],
            strlen(get_line_from_history(commandline_history, 0))) == 0) {
                strcpy(inputline, valid_commands[i]);
                inputline_pos = strlen(inputline);
                tab_comp = i;
                break;
            }
        }

        // restauro contenido de inputline
        if (i == clen) {
            strcpy(inputline, get_line_from_history(commandline_history, 0));
            inputline_pos = strlen(inputline);
            tab_comp = -1;
        }

        show_header(input_win);
 


    // CONFIRM A COMMAND PRESSING ENTER
    } else if (find_val(sb, OKEY_ENTER)) {

        if ( strcmp(inputline, "q") == 0 || strcmp(inputline, "quit") == 0 ) {
            shall_quit = 1;

        } else if ( strcmp(inputline, "q!") == 0 || strcmp(inputline, "quit!") == 0 ) {
            shall_quit = 2;

        } else if ( strcmp(inputline, "help") == 0 || strcmp(inputline, "h") == 0 ) {
            help();

        } else if ( strncmp(inputline, "load", 4) == 0 ) {
            char cline [BUFFERSIZE];
            int force_rewrite = 0;
            strcpy(cline, inputline);

            if ( strncmp(inputline, "load! ", 6) == 0 ) {
                force_rewrite = 1;
                del_range_chars(cline, 4, 4);
            }

            del_range_chars(cline, 0, 4);
            if ( ! strlen(cline) ) {
                error("Path to file to load is missing !");
            } else if ( modflg && ! force_rewrite ) {
                error("Changes were made since last save. Use '!' to force the load");
            } else {
                delete_structures();
                create_structures();
                readfile(cline, 0);
                //EvalAll(); // es necesario???
                modflg = 0;
                update(); 
            }

        } else if ( strncmp(inputline, "hiderow ", 8) == 0 ||
                    strncmp(inputline, "showrow ", 8) == 0 ||
                    strncmp(inputline, "showcol ", 8) == 0 ||
                    strncmp(inputline, "hidecol ", 8) == 0
                  ) {
            send_to_interp(inputline); 

        } else if ( strncmp(inputline, "showrows", 8) == 0 ) {
            if (p == -1) return; // si no hay un rango seleccionado, regreso.
            int r, arg;
            sr = get_range_by_pos(p);
            r = sr->tlrow;
            arg = sr->brcol - sr->tlcol + 1;
            show_row(r, arg);

        } else if ( strncmp(inputline, "sort ", 5) == 0 ) {
            strcpy(interp_line, inputline);
            if (p != -1) { // si hay un rango seleccionado
                char cline [BUFFERSIZE];
                strcpy(cline, interp_line);
                int found = str_in_str(interp_line, "\"");
                if (found == -1) return;
                del_range_chars(cline, 0, found-1);
                sprintf(interp_line, "sort %s%d:", coltoa(sr->tlcol), sr->tlrow);
                sprintf(interp_line, "%s%s%d %s", interp_line, coltoa(sr->brcol), sr->brrow, cline);
            }
            send_to_interp(interp_line); 

        } else if ( strncmp(inputline, "hiddenrows", 10) == 0 ) {
            show_hiddenrows();
                    
        } else if ( strncmp(inputline, "hiddencols", 10) == 0 ) {
            show_hiddencols();

        } else if ( strncmp(inputline, "int ", 4) == 0 ) { // send cmd to interpreter
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 3);
            send_to_interp(interp_line);

        } else if ( strncmp(inputline, "format ", 7) == 0 ) {
            create_undo_action();
            int r = currow, c = curcol, rf = currow, cf = curcol;
            if (p != -1) {
                c = sr->tlcol;
                r = sr->tlrow;
                rf = sr->brrow;
                cf = sr->brcol;
                sprintf(interp_line, "fmt %s%d:", coltoa(c), r);
                sprintf(interp_line, "%s%s%d", interp_line, coltoa(cf), rf);
            } else
                sprintf(interp_line, "fmt %s%d", coltoa(c), r);
            int l = strlen(interp_line);
            sprintf(interp_line, "%s %s", interp_line, inputline);
            del_range_chars(interp_line, l, l + 6);
            copy_to_undostruct(r, c, rf, cf, 'd');
            send_to_interp(interp_line);
            copy_to_undostruct(r, c, rf, cf, 'a');
            end_undo_action();

        } else if ( strncmp(inputline, "color ", 6) == 0 ) {
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 5);
            chg_color(interp_line);

        } else if ( strncmp(inputline, "set ", 4) == 0 ) {
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 3); 
            parse_str(user_conf_d, interp_line);
            info("Config value changed: %s", interp_line);

        } else if ( strcmp(inputline, "set") == 0 ) {
            int d = user_conf_d->len;
            char valores[20 * d];
            valores[0]='\0';
            get_conf_values(valores);
            show_text(valores);

        } else if ( strcmp(inputline, "version") == 0 ) {
            show_text(rev);

        } else if ( strncmp(inputline, "!", 1) == 0 ) {
            del_range_chars(inputline, 0, 0); 
            exec_cmd(inputline);

        } else if ( inputline[0] == 'w' ) {
            savefile();

        } else if ( inputline[0] == 'x' ) {
            if ( savefile() == 0 ) shall_quit = 1;

        } else if (
            strncmp(inputline, "e csv"  , 5) == 0 ||
            strncmp(inputline, "e! tab" , 6) == 0 ||
            strncmp(inputline, "e! csv" , 6) == 0 ||
            strncmp(inputline, "e tab"  , 5) == 0 ) {
                do_export( p == -1 ? 0 : sr->tlrow, p == -1 ? 0 : sr->tlcol,
                p == -1 ? maxrow : sr->brrow, p == -1 ? maxcol : sr->brcol);

        } else if (
            strncmp(inputline, "i csv " , 6) == 0 ||
            strncmp(inputline, "i! csv ", 7) == 0 ||
            strncmp(inputline, "i tab " , 6) == 0 ||
            strncmp(inputline, "i! tab ", 7) == 0 ) {
            
            int force_rewrite = 0;
            char delim;
            char cline [BUFFERSIZE];
            strcpy(cline, inputline);

            if (inputline[1] == '!') {
                force_rewrite = 1;
                del_range_chars(cline, 1, 1);
            }

            if ( strncmp(cline, "i csv ", 6) == 0) delim = ',';
            else delim = '\t';

            del_range_chars(cline, 0, 5);
            if ( ! strlen(cline) ) {
                error("Path to file to import is missing !");
            } else if ( modflg && ! force_rewrite ) {
                error("Changes were made since last save. Save current file or use '!' to force the import.");
            } else {
                delete_structures();
                create_structures();
                import_csv(cline, delim);
                modflg = 0;
                update(); 
            }

        } else {
            error("COMMAND NOT FOUND !");
        }

        del_item_from_history(commandline_history, 0);
        // si hay en historial algun item con texto igual al del comando que se ejecuta
        // a partir de la posicion 2, se lo coloca al comiendo de la lista (indica que es lo mas reciente ejecutado).
        int moved = move_item_from_history_by_str(commandline_history, inputline, -1);
        if (!moved) add(commandline_history, inputline);
        commandline_history->pos = 0;

        chg_mode('.');
        // clr_header(input_win); // COMENTADO el dia 22/06
        inputline[0]='\0';
        update();

    }
    //show_header(input_win); // NO DESCOMENTAR.
    return;
}

void ins_in_line(int d) {
    //error(">>%d             ", d);
    //if (d < 256 && d > 31) // > 0
        add_char(inputline, (char) d, inputline_pos++);
    return;
}
