/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of SC-IM                                                *
 *                                                                             *
 * SC-IM is a spreadsheet program that is based on SC. The original authors    *
 * of SC are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file main.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief The main file of sc-im
 *
 * \details This is the main file for sc-im.
 *
 * \see Homepage: https://github.com/andmarti1424/sc-im
 *
 * \bug It has been detected that libxls can produce memory leaks. One example
 * is when you try to read a non xls file (e.g. xlsx file).
 *
 * \bug Extended ascii characters are not showing correctly. Compile sc-im
 * against -lncursesw and not -lncurses.
 */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>   // for F_GETFL O_NONBLOCK F_SETFL
#include <locale.h>
#include <wchar.h>
#include <sys/ioctl.h> // for ioctl

#ifndef NO_WORDEXP
#include <wordexp.h>
#endif

#include "main.h"
#include "shift.h"
#include "macros.h"
#include "tui.h"
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
#include "vmtbl.h"   // for growtbl
#include "filter.h"
#include "dep_graph.h"

#ifdef UNDO
#include "undo.h"
#endif

#ifdef XLUA
#include "lua.h"
#endif

int currow = 0; /**< Current row of the selected cell. */
int curcol = 0; /**< Current column of the selected cell. */
int lastrow = 0;
int lastcol = 0;
int maxrows;
int maxcols;
int * fwidth;
int * precision;
int * realfmt;
char * col_hidden;
char * row_hidden;
char line[FBUFLEN];
int modflg; /**< Indicates a change was made since last save */
struct ent *** tbl;
int shall_quit = 0;
unsigned int curmode;
unsigned int lastmode;
int maxrow, maxcol;
char curfile[PATHLEN];
char loadingfile[PATHLEN] = { '\0' };
char * exepath;

int changed;
int cellassign;
int arg = 1;
int brokenpipe = FALSE; /**< Set to true if SIGPIPE is received */
char * ascext;
char * tbl0ext;
char * tblext;
char * latexext;
char * slatexext;
char * texext;
char dpoint = '.'; /**< Default decimal point character */
char thsep = ','; /**< Default thousands separator character */
int linelim = -1;
int calc_order = BYROWS;
int optimize  = 0; /**< Causes numeric expressions to be optimizedv */
int tbl_style = 0; /**< Headers for T command output */
int rndtoeven = 0;
int rowsinrange = 1;
int colsinrange = DEFWIDTH;
double eval_result;
char * seval_result;
FILE * fdoutput;  /**< Output file descriptor (stdout or file) */
int rescol = RESCOL; /**< Columns reserved for row numbers */

struct block * buffer;
struct block * lastcmd_buffer;
struct dictionary * user_conf_d; /**< User's configuration dictionary */
struct dictionary * predefined_conf_d; /**< Predefined configuration dictionary */
struct history * commandline_history;
struct history * insert_history;
char stderr_buffer[1024] = "";
struct timeval startup_tv, current_tv; /**< Runtime timer */

#ifdef AUTOBACKUP
struct timeval lastbackup_tv; /**< Last backup timer */
#ifdef HAVE_PTHREAD
#include <pthread.h>
int pthread_exists = 0; /**< Return status of pthread_create */
pthread_t fthread;
#endif
#endif

void read_stdin();
extern char * rev;

/**
 * \brief The main() function
 * 
 * \details The main() function of sc-im. It is the first function called when
 * the applicaiton is executed.
 *
 * \param[in] argc (argument count) is the number of strings pointed to by argv. This
 * is passed to main() by the system.
 *
 * \param[in] argv (argument vector) is a is a one-dimensional array of strings.
 * Each string is one of the arguments that was passed to the program. The
 * first string is the executable's name. This is passed to main() by
 * the system.
 *
 * \return 0 on success; 1 on some errors; -1 on error
 */

// TODO Document the possible errors. Why are some things -1 while others
// are 1? Look for instances of exit_app(<number>).

int main (int argc, char ** argv) {
    // Define how the file stream should be buffered. Error if unsuccessful.
    if (setvbuf(stderr, stderr_buffer, _IOFBF, STDERRBUF) != 0) {
        fprintf(stderr, "Error setting stderr buffer\n");
        return -1;
    }

    // set up signals so we can catch them
    signals();

#ifdef USELOCALE
    // Set location dependent information. This is used by some libraries.
    //setlocale(LC_ALL, "");
    setlocale(LC_CTYPE, "");
#endif

    // start configuration dictionaries
    user_conf_d = (struct dictionary *) create_dictionary();
    predefined_conf_d = (struct dictionary *) create_dictionary();
    store_default_config_values(); // Stores default values in user_conf_d

    // Read the main() parameters and replace values in user_conf_d as necessary
    read_argv(argc, argv);

    // check if help is in argv. if so, show usage and quit
    if (atoi((char *) get_conf_value("help"))) // atoi converts string to an int
        show_usage_and_quit();

    // check if version is in argv. if so, show version and quit
    if (atoi((char *) get_conf_value("version"))) // atoi converts string to an int
        show_version_and_quit();

    // create command line history structure
    if (! atoi((char *) get_conf_value("nocurses"))) {
#ifdef HISTORY_FILE
        commandline_history = (struct history *) create_history(':');
        load_history(commandline_history, ':'); // load the command history file
#endif
#ifdef INS_HISTORY_FILE
        insert_history = (struct history *) create_history('=');
        load_history(insert_history, '='); // load the insert history file
#endif
    }

    // create basic structures that will depend on the loaded file
    create_structures();

    // setup the spreadsheet arrays (tbl)
    if (! growtbl(GROWNEW, 0, 0)) return exit_app(1);


    // initiate NCURSES if that is what is wanted
    if (! atoi((char *) get_conf_value("nocurses"))) {
        ui_start_screen();

#ifdef USECOLORS
        if (get_d_colors_param() == NULL) {
            start_default_ucolors();
        /*
         * in case we decide to change colors
         * this creates a dictionary and stores in it
         * the relationship between macros and the keys values
         * that are defined in .sc files
         */
            set_colors_param_dict();
        }
#endif
    }

    /*
     * If the 'output' parameter is defined, SC-IM saves its output to that file.
     * To achieve that, we open the output file and keep it open until exit.
     * otherwise, SC-IM will output to stdout.
     */
    if (get_conf_value("output") != NULL) {
        fdoutput = fopen(get_conf_value("output"), "w+");
        if (fdoutput == NULL) {
            sc_error("Cannot open file: %s.", get_conf_value("output"));
            return exit_app(-1);
        }

        if (! atoi((char *) get_conf_value("nocurses"))) { // WE MUST STOP SCREEN!
            ui_stop_screen();

            // if output is set, nocurses should always be 1 !
            put(user_conf_d, "nocurses", "1");
        }
    }

#ifdef XLUA
    doLuainit();
#endif

    wchar_t stdin_buffer[BUFFERSIZE] = { L'\0' };

    // if there was no file passed to scim executable
    // 1. erase db !
    if (! loadingfile[0]) erasedb();

    // 2. loadrc
    loadrc();

    // 3. check input from stdin (pipeline)
    // and send it to interp
    read_stdin();

    // 4. read sc file passed as argv
    load_sc();

    // change curmode to NORMAL_MODE
    chg_mode('.');

    // initiate ui
    FILE * f;
    if ( ! atoi((char *) get_conf_value("nocurses"))) {
        // we show welcome screen if no spreadsheet was passed to SC-IM
        // and no input was sent throw pipeline
        if ( ! curfile[0] && ! wcslen(stdin_buffer)) {
            ui_do_welcome();
            // show mode and cell's details in status bar
            ui_print_mode();
            ui_show_celldetails();
        } else {
            ui_show_header();
            ui_update(TRUE);
        }
    } else {
        f = fopen("/dev/tty", "rw");
        if (f == NULL) sc_error("fatal error loading stdin");
    }

    // handle input from keyboard
    if (! atoi((char *) get_conf_value("nocurses")))
        buffer = (struct block *) create_buf(); // this should only take place if curses ui

    wchar_t nocurses_buffer[BUFFERSIZE];

    // runtime timer
    gettimeofday(&startup_tv, NULL);

    #ifdef AUTOBACKUP
    //gettimeofday(&lastbackup_tv, NULL);
    lastbackup_tv = (struct timeval) {0};
    #endif

    if (get_conf_value("export_csv")) {
        export_delim(NULL, ',', 0, 0, maxrow, maxcol, 0);
    }

    if (get_conf_value("export_tab")) {
        export_delim(NULL, '\t', 0, 0, maxrow, maxcol, 0);
    }

    if (get_conf_value("export") || get_conf_value("export_txt")) {
        export_plain(NULL, 0, 0, maxrow, maxcol);
    }

    while ( ! shall_quit && ! atoi((char *) get_conf_value("quit_afterload"))) {
        // save current time for runtime timer
        gettimeofday(&current_tv, NULL);

        // autobackup if it is time to do so
        handle_backup();

        // if we are in ncurses
        if (! atoi((char *) get_conf_value("nocurses"))) {
            handle_input(buffer);

        // if we are not in ncurses
        } else if (fgetws(nocurses_buffer, BUFFERSIZE, f) != NULL) {
            sc_debug("Interp will receive: %ls", nocurses_buffer);
            send_to_interp(nocurses_buffer);
        }

        /* shall_quit=1 means :q
           shall_quit=2 means :q! */
        if (shall_quit == 1 && modcheck()) shall_quit = 0;
    }
    if (atoi((char *) get_conf_value("nocurses")) && f != NULL) fclose(f);

    return shall_quit == -1 ? exit_app(-1) : exit_app(0);
}

extern graphADT graph;

/**
 * \brief Creates the structures used by the program.
 * 
 * \return none
 */

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

    // init calc chain graph
    graph = GraphCreate();
}

/**
 * \brief TODO Document read_stdin()
 *
 * \return none
 */

void read_stdin() {
    //sc_debug("reading stdin from pipeline");
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    fd_set savefds = readfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FILE * f = stdin;
    //FILE * f = fopen("/dev/tty", "rw");
    wchar_t stdin_buffer[BUFFERSIZE] = { L'\0' };

    if (select(1, &readfds, NULL, NULL, &timeout)) {
        //sc_debug("there is data");
        while (f != NULL && fgetws(stdin_buffer, BUFFERSIZE, f) != NULL) {
            sc_debug("Interp will receive: %ls", stdin_buffer);
            send_to_interp(stdin_buffer);
        }
        fflush(f);
    } else {
        //sc_debug("there is NO data");
    }
    readfds = savefds;
    if (f != NULL) fclose(f);

    if ( ! freopen("/dev/tty", "rw", stdin)) {
        perror(NULL);
        exit(-1);
    }
    //sc_debug("finish reading");
}

/**
 * \brief Delete basic structures that depend on the loaded files.
 *
 * \return none
 */

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

    // free calc chain graph
    destroy_graph(graph);

    // Free ents of tbl
    erasedb();
}

/**
 * \brief Cleans things up just before exiting the program.
 *
 * \param[in] status
 * \param[out] status
 *
 * \return status is returned unchanged
 */

int exit_app(int status) {

    // free history
    if (! atoi((char *) get_conf_value("nocurses"))) {

#ifdef HISTORY_FILE
        if (! save_history(commandline_history, "w")) sc_error("Could not save commandline history");
        if (commandline_history != NULL) destroy_history(commandline_history);
#endif

#ifdef INS_HISTORY_FILE
        if (! save_history(insert_history, "a")) sc_error("Could not save input mode history");
        if (insert_history != NULL) destroy_history(insert_history);
#endif
    }

    // wait for autobackup thread to finish, just in case
    #if defined(AUTOBACKUP) && defined(HAVE_PTHREAD)
    if (pthread_exists) pthread_join (fthread, NULL);
    #endif

    // remove backup file
#ifdef AUTOBACKUP
    if (strlen(curfile) && backup_exists(curfile)) remove_backup(curfile);
#endif

    // erase structures
    delete_structures();

    // Free mappings
    del_maps();

    // Erase stdin
    erase_buf(buffer);

    // stop CURSES screen
    if (! atoi((char *) (get_conf_value("nocurses"))))
        ui_stop_screen();

    // close fdoutput
    if (get_conf_value("output") != NULL && get_conf_value("output")[0] != '\0' && fdoutput != NULL) {
        fclose(fdoutput);
    }

    // delete user and predefined config dictionaries
    destroy_dictionary(predefined_conf_d);
    destroy_dictionary(user_conf_d);

    return status;
}

/**
 * \brief Read command line parameters and store them in a dictionary 
 *
 * \details Read parameters passed to SC-IM executable and
 * store them in user_conf dictionary.
 *
 * \param[in] argc (argument count) is the number of strings pointed to by
 * argv.
 * \param[in] argv (argument vector) is a one-dimensional array of strings.
 * Each string is one of the arguments that was passed to the program. The
 * first string is the executable's name.
 *
 * \return none
 */

void read_argv(int argc, char ** argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if ( ! strncmp(argv[i], "--", 2) ) {       // it was passed a parameter
            char *dup = strdup(argv[i]);
            char *rest = dup;
            char *name = strsep(&rest, "=");
            if (rest) {
                put(user_conf_d, &name[2], rest);  // --parameter=value
            } else {
                put(user_conf_d, &name[2], "1");   // --parameter
            }
            free(dup);
        } else {                                   // it was passed a file
            strncpy(loadingfile, argv[i], PATHLEN-1);
        }
    }
    exepath = argv[0];
    return;
}

/**
 * \brief Attempt to load a file
 *
 * \return none
 */

void load_sc() {
    char name[PATHLEN];
    #ifdef NO_WORDEXP
    size_t len;
    #else
    int c;
    wordexp_t p;
    #endif

    #ifdef NO_WORDEXP
    if ((len = strlen(loadingfile)) >= sizeof(name)) {
        sc_info("File path too long: '%s'", loadingfile);
        return;
    }
    memcpy(name, loadingfile, len+1);
    #else
    wordexp(loadingfile, &p, 0);
    for (c=0; c < p.we_wordc; c++) {
        if (c) sprintf(name + strlen(name), " ");
        sprintf(name + strlen(name), "%s", p.we_wordv[c]);
    }
    wordfree(&p);
    #endif

    if (strlen(name) != 0) {
        sc_readfile_result result = readfile(name, 0);
        if (!atoi((char *) get_conf_value("nocurses"))) {
            if (result == SC_READFILE_DOESNTEXIST) {
                // It's a new record!
                sc_info("New file: \"%s\"", name);
            } else if (result == SC_READFILE_ERROR) {
                sc_info("\"%s\" is not a SC-IM compatible file", name);
            }
        }
    }
}

/**
 * \brief Set the calculation order
 *
 * \return none
 */

void setorder(int i) {
    if ((i == BYROWS) || (i == BYCOLS)) calc_order = i;
    return;
}

/**
 * \brief Set signals catched by sc-im
 *
 * \return none
 */

void signals() {
    void sig_int();
    void sig_abrt();
    void sig_term();
    void nopipe();
    void sig_winchg();
    void sig_tstp();
    void sig_cont();

    signal(SIGINT, sig_int);
    signal(SIGABRT, sig_abrt);
    signal(SIGTERM, sig_term); // kill
    signal(SIGPIPE, nopipe);
    //(void) signal(SIGALRM, time_out);
    signal(SIGWINCH, sig_winchg);
    //(void) signal(SIGBUS, doquit);
    //(void) signal(SIGFPE, doquit);
    signal(SIGTSTP, sig_tstp);
    signal(SIGCONT, sig_cont);
    return;
}

/**
 * \brief Handles the SIGPIPE signal
 *
 * \return none
 */

// TODO Possibly rename this function to sig_nopipe() for consistency
// with the other signal functions.

void nopipe() {
    sc_error("brokenpipe!");
    brokenpipe = TRUE;
    return;
}

/**
 * \brief Handles the SIGTSTP signal
 *
 * \return none
 */

void sig_tstp() {
    //sc_info("Got SIGTSTP.");
    def_prog_mode();
    endwin();
    signal(SIGTSTP, SIG_DFL);  /* set handler to default */
    kill(getpid(), SIGTSTP);   /* call the default handler */
}


/**
 * \brief Handles the SIGCONT signal
 *
 * \return none
 */

void sig_cont() {
    signal(SIGTSTP, sig_tstp); /* set handler back to this */
    sig_winchg();
    reset_prog_mode();
    refresh();
    ui_update(TRUE);
    //sc_info("Got SIGCONT.");
}

/**
 * \brief Handles the SIGINT signal
 *
 * \return none
 */

void sig_int() {
    if ( ! atoi((char *) get_conf_value("debug")))
        sc_error("Got SIGINT. Press «:q<Enter>» to quit SC-IM");
    else
        shall_quit = 2;
    return;
}

/**
 * \brief Handles the SIGABRT signal
 *
 * \return none
 */

void sig_abrt() {
    sc_error("Error !!! Quitting SC-IM.");
    shall_quit = -1; // error !
    return;
}

/**
 * \brief Handles the SIGABRT signal
 *
 * \return none
 */

void sig_term() {
    sc_error("Got SIGTERM signal. Quitting SC-IM.");
    shall_quit = 2;
    return;
}

/**
 * \brief Send the version number to standard output and quit.
 *
 * \return none
 */

// TODO Split this into two commands. One prints the version number
// the other prints the version number along with the other information.

void show_version_and_quit() {
    put(user_conf_d, "nocurses", "1");
    sc_info("Sc-im - %s", rev);
#ifdef NCURSES
    sc_info("-DNCURSES");
#endif
#ifdef MAXROWS
    sc_info("-DMAXROWS %d", MAXROWS);
#endif
#ifdef UNDO
    sc_info("-DUNDO");
#endif
#ifdef XLS
    sc_info("-DXLS");
#endif
#ifdef XLSX
    sc_info("-DXLSX");
#endif
#ifdef XLSX_EXPORT
    sc_info("-DXLSX_EXPORT");
#endif
#ifdef XLUA
    sc_info("-DXLUA");
#endif
#ifdef DEFAULT_COPY_TO_CLIPBOARD_CMD
    sc_info("-DDEFAULT_COPY_TO_CLIPBOARD_CMD=\"%s\"", DEFAULT_COPY_TO_CLIPBOARD_CMD);
#endif
#ifdef DEFAULT_PASTE_FROM_CLIPBOARD_CMD
    sc_info("-DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\"%s\"", DEFAULT_PASTE_FROM_CLIPBOARD_CMD);
#endif
#ifdef USELOCALE
    sc_info("-DUSELOCALE");
#endif
#ifdef USECOLORS
    sc_info("-DUSECOLORS");
#endif
#ifdef _XOPEN_SOURCE_EXTENDED
    sc_info("-D_XOPEN_SOURCE_EXTENDED");
#endif
#ifdef _GNU_SOURCE
    sc_info("-D_GNU_SOURCE");
#endif
#ifdef SNAME
    sc_info("-DSNAME=\"%s\"", SNAME);
#endif
#ifdef HELP_PATH
    sc_info("-DHELP_PATH=\"%s\"", HELP_PATH);
#endif
#ifdef LIBDIR
    sc_info("-DLIBDIR=\"%s\"", LIBDIR);
#endif
#ifdef DFLT_PAGER
    sc_info("-DDFLT_PAGER=\"%s\"", DFLT_PAGER);
#endif
#ifdef HISTORY_FILE
    sc_info("-DHISTORY_FILE=\"%s\"", HISTORY_FILE);
#endif
#ifdef INS_HISTORY_FILE
    sc_info("-DINS_HISTORY_FILE=\"%s\"", INS_HISTORY_FILE);
#endif
#ifdef HAVE_PTHREAD
    sc_info("-DHAVE_PTHREAD");
#endif
#ifdef AUTOBACKUP
    sc_info("-DAUTOBACKUP");
#endif
    put(user_conf_d, "quit_afterload", "1");
}

/**
 * \brief Print usage message to stdout text and quit
 *
 * \return none
 */

// NOTE this is a quick and dirty command to search for arguments used in the sources (macOS 10.14)
// grep "get_conf_value(\"" -r ./src/*.c | grep get_conf_value |sed 's/"//g' |sed 's/.*get_conf_value(//g'|cut -d ')' -f1 |sort|uniq|sed 's/^/--/g'
void show_usage_and_quit(){
  put(user_conf_d, "nocurses", "1");
  printf("\
\nSC-IM - SC Improved\
\n\
\nUsage: sc-im [arguments] [file]          specified file\
\n   or: sc-im [arguments] -               read text from stdin\
\n\
\nArguments:\
\n\
\n  --autocalc                  Set variable 'autocalc'.\
\n  --copy_to_clipboard_delimited_tab  Set variable 'copy_to_clipboard_delimited_tab'\
\n  --debug                     Set variable 'debug'\
\n  --default_copy_to_clipboard_cmd=COMMAND  set variable 'default_copy_from_clipboard_cmd'\
\n  --default_paste_from_clipboard_cmd=COMMAND  set variable 'default_paste_from_clipboard_cmd'\
\n  --export_csv                Export to csv without interaction\
\n  --export_tab                Export to tab without interaction\
\n  --export_txt                Export to txt without interaction\
\n  --external_functions        Set variable 'external_functions'\
\n  --half_page_scroll          Set variable 'half_page_scroll'\
\n  --ignorecase                Set variable 'ignorecase'\
\n  --import_delimited_as_text Import text as\
\n  --newline_action={j or l}   Set variable 'newline_action'\
\n  --nocurses                  Run interactive but without ncurses interface.\
\n  --numeric                   Set variable 'numeric'\
\n  --numeric_decimal           Set variable 'numeric_decimal'\
\n  --output=FILE               Save the results in FILE\
\n  --overlap                   Set variable 'overlap variable'\
\n  --quit_afterload            Quit after loading all the files\
\n  --sheet=SHEET               Open SHEET when loading xlsx file. Default is 1.\
\n  --tm_gmtoff={seconds}       set gmt offset used for converting datetimes to localtime.\
\n  --txtdelim={\",\" or \";\" or \"\\t\"}  Sets delimiter when opening a .tab of .scv file");
#ifdef XLSX
  printf("\n\
\n  --xlsx_readformulas         Set variable 'xlsx_readformulas'");
#endif
  printf("\n\
\n  --version                   Print version information and exit\
\n  --help                      Print Help (this message) and exit\n");
    put(user_conf_d, "quit_afterload", "1");
}
