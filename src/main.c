#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "shift.h"
#include "macros.h"
#include "screen.h"
#include "input.h"
#include "marks.h"
#include "maps.h"
#include "yank.h"
#include "file.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "history.h"
#include "conf.h"
#include "buffer.h"
#include "cmds.h"
#include "color.h"   // for set_ucolor
#include "vmtbl.h"   // for growtbl
#include "filter.h"

#ifdef UNDO
#include "undo.h"
#endif

int currow = 0, curcol = 0;
int maxrows, maxcols;
int * fwidth;
int * precision;
int * realfmt;
char * col_hidden;
char * row_hidden;
char line[FBUFLEN];
int modflg;
struct ent ***tbl;
char *progname;
unsigned int shall_quit = 0;
unsigned int curmode = NORMAL_MODE;
int maxrow, maxcol;

int changed;
int cellassign;
int arg = 1;
char * mdir;
char * autorun;
int skipautorun;
char * fkey[FKEYS];
char * scext;
char * ascext;
char * tbl0ext;
char * tblext;
char * latexext;
char * slatexext;
char * texext;
int scrc = 0;
int usecurses = TRUE;   /* Use curses unless piping/redirection or using -q */
int brokenpipe = FALSE; /* Set to true if SIGPIPE is received */
char curfile[PATHLEN];
char dpoint = '.';      /* decimal point */
char thsep = ',';       /* thousands separator */
int linelim = -1;
int calc_order = BYROWS;
int optimize  = 0;     /* Causes numeric expressions to be optimized */
int tbl_style = 0;     /* headers for T command output */
int rndtoeven = 0;
int rowsinrange = 1;
int colsinrange = DEFWIDTH;
double eval_result;
char * seval_result;

struct block * buffer;
struct block * lastcmd_buffer;
struct dictionary * user_conf_d;
struct dictionary * predefined_conf_d;
struct history * commandline_history;

int main (int argc, char ** argv) {

    // catch up signals
    signals();

    // inicializo NCURSES
    start_screen();        
    // esto habilitarlo para debug de mensajes:    wtimeout(input_win, 1000);

    // setup the spreadsheet arrays
    if (! growtbl(GROWNEW, 0, 0)) return exit_app(1);

    // start configuration dictionaries
    user_conf_d = (struct dictionary *) create_dictionary();
    predefined_conf_d = (struct dictionary *) create_dictionary();
    store_default_config_values();
    
    // create command line history structure
#if defined HISTORY_FILE
    commandline_history = (struct history *) create_history(':');
    load_history(commandline_history);
#endif

    // create basic structures that will depend on the loaded file
    create_structures();

    // load spreasdsheet passed as argv
    char * revi;
    if ((revi = strrchr(argv[0], '/')) != NULL)
        progname = revi + 1;
    else {
        progname = argv[0];
    }
    load_sc(argc, argv);      

    // we show welcome screen if not spreadsheet was pass to SCIM
    if ( ! curfile[0] ) {
        do_welcome();
        // show mode and cell's details in status bar
        show_celldetails(input_win);
        print_mode(input_win);
        wrefresh(input_win);
    } else {
        update(); 
    }

    // manejo stdin
    shall_quit = 0;
    buffer = (struct block *) create_buf();

    // esto habilitarlo para debug:  wtimeout(input_win, TIMEOUT_CURSES);

    while ( ! shall_quit ) {
        handle_input(buffer);

        // shall_quit=1 implica :q
        // shall_quit=2 implica :q!
        if (shall_quit == 1 && modcheck()) shall_quit = 0;
    }
    return shall_quit == -1 ? exit_app(-1) : exit_app(0);
}



void create_structures() {

    // inicializo array de marcas
    create_mark_array();

    // create last command buffer
    lastcmd_buffer = (struct block *) create_buf();

    // create yank list structure
    init_yanklist();
}    



// delete basic structures that depend on the loaded file
void delete_structures() {

    // Free marks array
    free_marks_array();
    
    // Free yanklist
    free_yanklist();

    // Erase last_command buffer
    erase_buf(lastcmd_buffer);

    // Free ranges
    free_ranges();

    // Free filters
    free_filters();

    // Free undo list - from start of list
    #ifdef UNDO
    clear_undo_list();
    #endif

    // free deleted ents
    flush_saved();

    // Free ents of tbl
    erasedb();
}



int exit_app(int status) {

    // delete user and predefined config dictionaries
    destroy_dictionary(predefined_conf_d);
    destroy_dictionary(user_conf_d);

    // free history
#if defined HISTORY_FILE
    if (save_history(commandline_history) != 0) {
        error("Cannot save command line history");
    }

    if (commandline_history != NULL) destroy_history(commandline_history);
#endif

    // erase structures
    delete_structures();

    // Free mappings
    del_maps();

    // Erase stdin
    erase_buf(buffer);

    //timeout(-1);

    // stop CURSES stdout
    stop_screen();

    return status;
}



// we read parameters passed to scim executable
// and store them in user_conf dictionary
// then we load the file
void load_sc(int argc, char ** argv) {
    // we read parameters
    int i;
    for (i = 1; i<argc; i++) {
        if ( ! strncmp(argv[i], "--", 2) ) {       // it was passed a parameter
            char ** s = split(argv[i], '=', 0);
            if (s[1] != NULL)
                put(user_conf_d, &s[0][2], s[1]);  // --parameter=value
            else
                put(user_conf_d, &s[0][2], "1");   // --parameter
        } else {                                   // it was passed a file
            strcpy(curfile, argv[i]);
        }
    }

    // we load the file
    if (! curfile[0]) {                        // there was no file passed to scim executable
        erasedb();
    } else {
        if (! readfile(curfile, 1)) {
            info("New file: \"%s\"", curfile); // file passed to scim executable does not exists
        }
        EvalAll();                             // we eval formulas
    }


    /* old routine:
    if (optind < argc && ! strcmp(argv[optind], "--"))
        optind++;
    if (optind < argc && argv[optind][0] != '|' && strcmp(argv[optind], "-"))
        (void) strcpy(curfile, argv[optind]);

    if (optind < argc) {
        if (! readfile(argv[optind], 1) && (optind == argc - 1)) {
            info("New file: \"%s\"", curfile);
        }
        EvalAll(); // evaluo fórmulas
        optind++;
    } else {
        erasedb();
    }

    while (optind < argc) {
        (void) readfile(argv[optind], 0);
        optind++;
    }
    */
} 



// set the calculation order 
void setorder(int i) {
    if ((i == BYROWS) || (i == BYCOLS)) calc_order = i;
}



void nopipe() {
    brokenpipe = TRUE;
}



// setup signals catched by SCIM
void signals() {
    void sig_int();
    void sig_abrt();
    void sig_term();
    //void nopipe();
    void winchg();
    
    //signal(SIGINT, sig_int); // FIXME - sig. linea se comenta porque es molesto para probar. 
    signal(SIGABRT, sig_abrt);
    signal(SIGTERM, sig_term); // kill
    //(void) signal(SIGPIPE, nopipe);
    //(void) signal(SIGALRM, time_out);
    signal(SIGWINCH, winchg);
    //(void) signal(SIGBUS, doquit);
    //(void) signal(SIGFPE, doquit);
}



void sig_int() {
    error("Got SIGINT. Press «:q<Enter>» to quit Scim");
}



void sig_abrt() {
    error("Error !!! Quitting SCIM.");
    shall_quit = -1; // error !
}



void sig_term() {
    error("Got SIGTERM signal. Quitting SCIM.");
    shall_quit = 2;
}



// SIGWINCH signal - resize of terminal
void winchg() {
    endwin();
    start_screen();
    clearok(stdscr, TRUE);
    update(); 
    flushinp();
    show_header(input_win);
    show_celldetails(input_win);
    wrefresh(input_win);
    update(); 
    //signal(SIGWINCH, winchg);
}
