#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>   // for F_GETFL O_NONBLOCK F_SETFL
#include <locale.h>

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
int lastrow = 0, lastcol = 0;
int maxrows, maxcols;
int * fwidth;
int * precision;
int * realfmt;
char * col_hidden;
char * row_hidden;
char line[FBUFLEN];
int modflg;          // a change was made since last save
struct ent *** tbl;
int shall_quit = 0;
unsigned int curmode = NORMAL_MODE;
int maxrow, maxcol;
char curfile[PATHLEN];
char *exepath;

int changed;
int cellassign;
int arg = 1;
int brokenpipe = FALSE; /* Set to true if SIGPIPE is received */
//char * progname;
//char * mdir;
//char * autorun;
//int skipautorun;
//char * fkey[FKEYS];
//char * scext;
char * ascext;
char * tbl0ext;
char * tblext;
char * latexext;
char * slatexext;
char * texext;
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
FILE * fdoutput; // output file descriptor (stdout or file)

struct block * buffer;
struct block * lastcmd_buffer;
struct dictionary * user_conf_d;
struct dictionary * predefined_conf_d;
struct history * commandline_history;


/*********************************************************************
   MAIN LOOP
 *********************************************************************/
int main (int argc, char ** argv) {

#ifdef USELOCALE
    // pass LC_CTYPE env variable to libraries
    //setlocale(LC_ALL, "");
    setlocale(LC_CTYPE, "");
#endif

    // set up signals so we can catch them
    signals();

    // start configuration dictionaries
    user_conf_d = (struct dictionary *) create_dictionary();
    predefined_conf_d = (struct dictionary *) create_dictionary();
    store_default_config_values();

    // create command line history structure
#ifdef HISTORY_FILE
    if (! atoi(get_conf_value("nocurses"))) {
        commandline_history = (struct history *) create_history(':');
        load_history(commandline_history);
    }
#endif

    // create basic structures that will depend on the loaded file
    create_structures();

    // setup the spreadsheet arrays (tbl)
    if (! growtbl(GROWNEW, 0, 0)) return exit_app(1);

    // we save parameters and use them to replace conf-values in config dictionary !
    read_argv(argc, argv);

    // initiate NCURSES if that is what is wanted
    if (! atoi(get_conf_value("nocurses")))
        start_screen();

#ifdef USECOLORS
    //if (has_colors() && get_d_colors_param() == NULL) {
    if (get_d_colors_param() == NULL) {
            start_default_ucolors();
            // in case we decide to change colors
            // esto crea un diccionario y guarda en él
            // la equivalencia entre las macros y los valores
            // de las claves que se definen en los archivos .sc
            set_colors_param_dict();
    }
#endif


    // If the 'output' parameter is defined, SC-IM saves its output to that file.
    // To achieve that, we open the output file and keep it open until exit.
    // otherwise, SC-IM will output to stdout.
    if (get_conf_value("output") != NULL) {
        fdoutput = fopen(get_conf_value("output"), "w+");
        if (fdoutput == NULL) {
            scerror("Cannot open file: %s.", get_conf_value("output"));
            return exit_app(-1);
        }

        if (! atoi(get_conf_value("nocurses"))) { // WE MUST STOP SCREEN!
            stop_screen();

            // if output is set, nocurses should always be 1 !
            put(user_conf_d, "nocurses", "1");
        }
    }


    // load sc file
    load_sc();

    // handle input from stdin (pipeline) in sc format
    // and send it to interp
    //FILE * f = fopen("/dev/stdin", "r");
    FILE * f = stdin;
    int fd = fileno(f);
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
    char stdin_buffer[BUFFERSIZE] = { '\0' };
    while (f != NULL && fgets(stdin_buffer, BUFFERSIZE, f) != NULL) {
        send_to_interp(stdin_buffer);
    }
    freopen("/dev/tty", "rw", stdin);
    flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);


    // Enable this for message debugging
    // wtimeout(input_win, 1000);

    // initiate gui
    if ( ! atoi(get_conf_value("nocurses"))) {
        start_screen();         // Already done above

        // we show welcome screen if no spreadsheet was passed to SC-IM
        // and no input was sent throw pipeline
        if ( ! curfile[0] && ! strlen(stdin_buffer)) {
            do_welcome();

            // show mode and cell's details in status bar
            show_celldetails(input_win);
            print_mode(input_win);
            wrefresh(input_win);
        } else {
            update(TRUE);
        }
    }

    // handle input from keyboard
    buffer = (struct block *) create_buf(); // TODO: this should only take place if curses ui
    // Enable for debugging purposes
    // wtimeout(input_win, TIMEOUT_CURSES);

    char nocurses_buffer[BUFFERSIZE];

    while ( ! shall_quit && ! atoi(get_conf_value("quit_afterload"))) {
        // if we are in ncurses
        if (! atoi(get_conf_value("nocurses"))) {
            handle_input(buffer);


        // if we are not in ncurses
        } else {
            if (fgets(nocurses_buffer, BUFFERSIZE, stdin) != NULL) {
                printf("Interp will receive: %s", nocurses_buffer);
                send_to_interp(nocurses_buffer);
            }
        }

        /* shall_quit=1 means :q
           shall_quit=2 means :q! */
        if (shall_quit == 1 && modcheck()) shall_quit = 0;
    }

    return shall_quit == -1 ? exit_app(-1) : exit_app(0);
}

/*********************************************************************
   END OF MAIN LOOP
 *********************************************************************/


void create_structures() {

    // initiate mark array
    create_mark_array();

    // create last command buffer
    lastcmd_buffer = (struct block *) create_buf();

    // create yank list structure
    init_yanklist();

    /* Assign NULL to colformats
    int c;
    for (c = 0; c < COLFORMATS; c++)
        colformat[c] = NULL;
    */
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

    // free history
#ifdef HISTORY_FILE
    if (! atoi(get_conf_value("nocurses")) && save_history(commandline_history) != 0 ) {
        scerror("Cannot save command line history");
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

    // stop CURSES screen
    if (! atoi(get_conf_value("nocurses")))
        stop_screen();

    // close fdoutput
    if (get_conf_value("output") != '\0' && fdoutput != NULL) {
       fclose(fdoutput);
    }

    // delete user and predefined config dictionaries
    destroy_dictionary(predefined_conf_d);
    destroy_dictionary(user_conf_d);

    return status;
}



// we read parameters passed to SC-IM executable
// and store them in user_conf dictionary
void read_argv(int argc, char ** argv) {
    int i, j;
    for (i = 1; i < argc; i++) {
        if ( ! strncmp(argv[i], "--", 2) ) {       // it was passed a parameter
            char ** s = split(argv[i], '=', 0);
            if (s[1] != NULL)
                put(user_conf_d, &s[0][2], s[1]);  // --parameter=value
            else
                put(user_conf_d, &s[0][2], "1");   // --parameter

            for (j=0; s[j]; j++) {
                free(s[j]);
            }
            free(s);
            s = NULL;
        } else {                                   // it was passed a file
            strcpy(curfile, argv[i]);
        }
    }
    exepath = argv[0];
}



// we try to load a file
void load_sc() {
    if (! curfile[0]) {                            // there was no file passed to scim executable
        erasedb();
    } else {
        if (! readfile(curfile, 1) && ! atoi(get_conf_value("nocurses"))) {
            scinfo("New file: \"%s\"", curfile);     // file passed to scim executable does not exists
        }
        EvalAll();                                 // we eval formulas
    }
}



// set the calculation order 
void setorder(int i) {
    if ((i == BYROWS) || (i == BYCOLS)) calc_order = i;
}



void nopipe() {
    scdebug("brokenpipe!");
    brokenpipe = TRUE;
}



// setup signals catched by SC-IM
void signals() {
    void sig_int();
    void sig_abrt();
    void sig_term();
    void nopipe();
    void winchg();

    //signal(SIGINT, sig_int); // FIXME - sig. Commented to reduce annoyance while testing
    signal(SIGABRT, sig_abrt);
    signal(SIGTERM, sig_term); // kill
    signal(SIGPIPE, nopipe);
    //(void) signal(SIGALRM, time_out);
    signal(SIGWINCH, winchg);
    //(void) signal(SIGBUS, doquit);
    //(void) signal(SIGFPE, doquit);
}



void sig_int() {
    scerror("Got SIGINT. Press «:q<Enter>» to quit Scim");
}



void sig_abrt() {
    scerror("Error !!! Quitting SC-IM.");
    shall_quit = -1; // error !
}



void sig_term() {
    scerror("Got SIGTERM signal. Quitting SC-IM.");
    shall_quit = 2;
}



// SIGWINCH signal !!!!
// resize of terminal
void winchg() {
    endwin();
    start_screen();
    clearok(stdscr, TRUE);
    update(TRUE);
    flushinp();
    show_header(input_win);
    show_celldetails(input_win);
    wrefresh(input_win);
    update(TRUE);
    //signal(SIGWINCH, winchg);
}



#include <stdlib.h>
#include <stdarg.h>
#include "conf.h"

void scerror(char * s, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, s);
    vsprintf (t, s, args);
    if ( ! atoi(get_conf_value("nocurses"))) {
#ifdef USECOLORS
        set_ucolor(input_win, &ucolors[ERROR_MSG]);
#endif
        mvwprintw(input_win, 1, 0, "%s", t);
        wclrtoeol(input_win);
        wrefresh(input_win);
    } else if (get_conf_value("output") != NULL && fdoutput != NULL) {
        fprintf(fdoutput, "%s\n", t);
    } else
        printf("%s\n", t);
    va_end(args);
    return;
}

void scinfo(char * s, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, s);
    vsprintf (t, s, args);
    if ( ! atoi(get_conf_value("nocurses"))) {
#ifdef USECOLORS
        set_ucolor(input_win, &ucolors[INFO_MSG]);
#endif
        mvwprintw(input_win, 1, 0, "%s", t);
        wclrtoeol(input_win);
        wrefresh(input_win);
    } else if (get_conf_value("output") != NULL && fdoutput != NULL) {
        fprintf(fdoutput, "%s\n", t);
    } else
        printf("%s\n", t);
    va_end(args);
    return;
}

void scdebug(char * s, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, s);
    vsprintf (t, s, args);
    if ( ! atoi(get_conf_value("nocurses"))) {
#ifdef USECOLORS
        set_ucolor(input_win, &ucolors[INFO_MSG]);
#endif
        mvwprintw(input_win, 1, 0, "%s", t);
        wclrtoeol(input_win);
        wtimeout(input_win, -1);
        wgetch(input_win);
        wtimeout(input_win, TIMEOUT_CURSES);
        wrefresh(input_win);
    } else if (get_conf_value("output") != NULL && fdoutput != NULL) {
        fprintf(fdoutput, "%s\n", t);
    } else {
        printf("%s\n", t);
    }
    va_end(args);
    return;
}
