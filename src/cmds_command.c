#include "cmds_command.h"
#include "cmds.h"
#include <string.h>
#include <curses.h>
#include "buffer.h"
#include <stdlib.h>
#include "utils/string.h"
#include <ctype.h>         // for isprint()
#include "sc.h"            // for rescol
#include "utils/dictionary.h"

extern char * rev;
extern struct dictionary * user_conf_d;

char inputline[BUFFERSIZE];
int inputline_pos;
char interp_line[100];

void do_commandmode(struct block * sb) {
    

    if (sb->value == OKEY_BS) {            // BS
        if (! strlen(inputline) || ! inputline_pos) {
            show_header(input_win);
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

    } else if (find_val(sb, OKEY_ENTER)) { // ENTER

        if ( strcmp(inputline, "q") == 0 || strcmp(inputline, "quit") == 0 ) {
            shall_quit = 1;

        } else if ( strcmp(inputline, "q!") == 0 || strcmp(inputline, "quit!") == 0 ) {
            shall_quit = 2;

        } else if ( strcmp(inputline, "help") == 0 || strcmp(inputline, "h") == 0 ) {
            help();

        } else if ( strncmp(inputline, "hiderow", 7) == 0 ||
        strncmp(inputline, "showrow", 7) == 0 ||
        strncmp(inputline, "showcol", 7) == 0 ||
        strncmp(inputline, "hidecol", 7) == 0 ||
        strncmp(inputline, "sort ", 5) == 0 ) {
            send_to_interp(inputline); 

        } else if ( strncmp(inputline, "int ", 4) == 0 ) { // send cmd to interpreter
            strcpy(interp_line, inputline);
            del_range_chars(interp_line, 0, 3);
            send_to_interp(interp_line);

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

        } else if ( strncmp(inputline, "e csv", 5) == 0 || strncmp(inputline, "e tab", 5) == 0 ) {
            do_export();

        } else if ( strcmp(inputline, "version") == 0 ) {
            show_text(rev);

        } else if ( strncmp(inputline, "!", 1) == 0 ) {
            del_range_chars(inputline, 0, 0); 
            exec_cmd(inputline);

        } else if ( inputline[0] == 'w' ) {
            savefile();

        } else if ( inputline[0] == 'x' ) {
            if ( savefile() == 0 ) shall_quit = 1;

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
        //clr_header(input_win); // COMENTADO el dia 22/06
        inputline[0]='\0';
        update();

    } else if (isprint(sb->value)) {        //  ESCRIBO UN NUEVO CHAR
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
