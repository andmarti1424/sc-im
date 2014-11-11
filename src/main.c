#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include "shift.h"
#include "macros.h"
#include "stdout.h"
#include "input.h"
#include "marks.h"
#include "yank.h"
#include "undo.h"
#include "utils/dictionary.h"
#include "history.h"
#include "buffer.h"

int arg = 1;
int currow = 0, curcol = 0;
int maxrows, maxcols;
//int FullUpdate = 0;
int *fwidth;
int *precision;
int *realfmt;
char *col_hidden;
char *row_hidden;
char line[FBUFLEN];
int changed;
int modflg;
int cellassign;
//int numeric;
char *mdir;
char *autorun;
int skipautorun;
char *fkey[FKEYS];
char *scext;
char *ascext;
char *tbl0ext;
char *tblext;
char *latexext;
char *slatexext;
char *texext;
int scrc = 0;
//int showsc, showsr;	/* Starting cell for highlighted range */
int usecurses = TRUE;	/* Use curses unless piping/redirection or using -q */
int brokenpipe = FALSE;	/* Set to true if SIGPIPE is received */
char curfile[PATHLEN];
char dpoint = '.';	/* decimal point */
char thsep = ',';	/* thousands separator */
int  linelim = -1;
int  calc_order = BYROWS;
int  optimize  = 0;	/* Causes numeric expressions to be optimized */
int  tbl_style = 0;	/* headers for T command output */
int  rndtoeven = 0;
//FIXME THIS
//int  showtop   = 1;	/* Causes current cell value display in top line  */
//int  showcell  = 1;	/* Causes current cell to be highlighted	  */
//int  showrange = 0;	/* Causes ranges to be highlighted		  */
//int  showneed  = 0;	/* Causes cells needing values to be highlighted  */
//int  showexpr  = 0;	/* Causes cell exprs to be displayed, highlighted */
//int  shownote  = 0;	/* Causes cells with attached notes to be highlighted					  */
//int  braille   = 0;	/* Be nice to users of braille displays		  */
//int  braillealt = 0;	/* Alternate mode for braille users		  */
//int  autocalc  = 1;	/* 1 to calculate after each update */
//int  autolabel = 1;     /* If room, causes label to be created after a define */
//int  autoinsert = 0;    /* Causes rows to be inserted if craction is non-zero
//                           and the last cell in a row/column of the scrolling
//                           portion of a framed range has been filled	  */
//int  autowrap = 0;      /* Causes cursor to move to next row/column if craction
//                           is non-zero and the last cell in a row/column of
//                           the scrolling portion of a framed range has been
//                           filled
//int  color     = 0;	/* Use color */
//int  colorneg  = 0;	/* Increment color number for cells with negative numbers */
//int  colorerr  = 0;	/* Color cells with errors with color 3 */
//int  numeric_field = 0; /* Started the line editing with a number */
//int  craction = 0;	/* 1 for down, 2 for right */
//int  pagesize = 0;	/* If nonzero, use instead of 1/2 screen height */
//int  rowlimit = -1;
//int  collimit = -1;
int  rowsinrange = 1;
int  colsinrange = DEFWIDTH;
//extern	int lastmx, lastmy;	/* Screen address of the cursor */
//extern	int lastcol, lcols;	/* Spreadsheet Column the cursor was in last */
//extern	int lastendrow;		/* Last bottom row of screen */
//extern	int framerows;		/* Rows in current frame */
//extern	int framecols;		/* Columns in current frame */
//extern	char mode_ind;		/* Mode indicator */
/* a linked list of free [struct ent]'s, uses .next as the pointer */
//struct ent *freeents = NULL;
//extern	int	seenerr;
struct ent ***tbl;
char	*progname;
unsigned int shall_quit = 0;
unsigned int curmode = NORMAL_MODE;
int maxrow, maxcol;

struct block * buffer;
struct block * lastcmd_buffer;
struct dictionary * user_conf_d;
struct dictionary * predefined_conf_d;
struct history * commandline_history;

void signals();

int main (int argc, char **argv) {

    // catch up signals
    signals();

    // inicializo NCURSES
    start_stdout();        
    //esto habilitarlo para debug de mensajes:        wtimeout(input_win, 1000);

    // setup the spreadsheet arrays
    if (!growtbl(GROWNEW, 0, 0)) return exit_app(1);

    // start configuration dictionaries
    user_conf_d = (struct dictionary *) create_dictionary();
    predefined_conf_d = (struct dictionary *) create_dictionary();
    store_default_config_values();
    
    // inicializo array de marcas
    create_mark_array();

    // create last command buffer
    lastcmd_buffer = (struct block *) create_buf();

    // create yank list structure
    init_yanklist();
    
    // create command line history structure
    commandline_history = (struct history *) create_history(':');
    load_history(commandline_history);

    // cargo la planilla pasada como parametro
    char * revi;
    if ((revi = strrchr(argv[0], '/')) != NULL)
        progname = revi + 1;
    else {
        progname = argv[0];
    }
    load_sc(argc, argv);      

    // doy la bienvenida en caso de no haber pasado ninguna planilla como param
    if (argv[1] == NULL) {
        do_welcome();
        // muestro modo y detalle de celda en barra de estado
        show_celldetails(input_win);
        print_mode(input_win);
        wrefresh(input_win);
    } else {
        update(); 
    }

    // manejo stdin
    shall_quit = 0;
    buffer = (struct block *) create_buf();
    //Esto habilitarlo para debug:     wtimeout(input_win, TIMEOUT_CURSES);
    while (! shall_quit) {
          handle_input(buffer);
          // shall_quit=1 implica :q
          // shall_quit=2 implica :q!
          if (shall_quit == 1 && modcheck()) shall_quit = 0;
    }

    return exit_app(0);
}

int exit_app(int status) {

    // delete user and predefined config dictionaries
    destroy_dictionary(predefined_conf_d);
    destroy_dictionary(user_conf_d);

    // free history
    if (save_history(commandline_history) != 0) {
        error("Cannot save command line history");
    }

    if (commandline_history != NULL) destroy_history(commandline_history);

    // Free mappings
    del_maps();

    // Free ranges
    free_ranges();

    // Free marks array
    free_marks_array();
  
    // Free yanklist
    free_yanklist();

    // Free undo list - from start of list
    clear_undo_list();

    // free deleted ents
    flush_saved();

    // Erase stdin and last_command buffer
    erase_buf(buffer);
    erase_buf(lastcmd_buffer);

    // Free ents of tbl
    erasedb();

    //timeout(-1);

    // stop CURSES stdout
    stop_stdout();

    return status;
}

void load_sc(int argc, char **argv) {
    if (optind < argc && !strcmp(argv[optind], "--"))
	optind++;
    if (optind < argc && argv[optind][0] != '|' && strcmp(argv[optind], "-"))
	(void) strcpy(curfile, argv[optind]);

    if (optind < argc) {
	if (! readfile(argv[optind], 1) && (optind == argc - 1)) {
	    info("New file: \"%s\"", curfile);
	}
        EvalAll(); // TODO. es necesario???
	optind++;
    } else {
        erasedb();
    }

    while (optind < argc) {
	(void) readfile(argv[optind], 0);
	optind++;
    }
} 

// Returns 1 if cell is locked, 0 otherwise
int locked_cell(int r, int c) {
    struct ent *p = *ATBL(tbl, r, c);
    if (p && (p->flags & is_locked)) {
	info("Cell %s%d is locked", coltoa(c), r) ;
	return(1);
    }
    return(0);
}

// Check if area contains locked cells
int any_locked_cells(int r1, int c1, int r2, int c2) {
    int r, c;
    struct ent *p ;

    for (r = r1; r <= r2; r++)
	for (c = c1; c <= c2; c++) {
	    p = *ATBL(tbl, r, c);
	    if (p && (p->flags & is_locked))
		return(1);
	}
    return(0);
}
// set the calculation order 
void setorder(int i) {
    if ((i == BYROWS) || (i == BYCOLS)) calc_order = i;
}

void nopipe() {
    brokenpipe = TRUE;
}



void signals() {
#ifdef SIGVOID
    //void doquit();
    //void time_out();
    void sig_int();
    //void nopipe();
    void winchg();
#else
    //int doquit();
    //int time_out();
    int sig_int();
    //int nopipe();
    int winchg();
#endif

    // FIXME esto anda pero es molesto para probar. 
    //signal(SIGINT, sig_int);

    //signal(SIGTERM, sig_int); // kill
    //(void) signal(SIGPIPE, nopipe);
    //(void) signal(SIGALRM, time_out);
    signal(SIGWINCH, winchg);
    //(void) signal(SIGBUS, doquit);
    //(void) signal(SIGFPE, doquit);
}

// sig_int
#ifdef SIGVOID
void
#else
int
#endif
sig_int() {
    error("Got SIGINT. Press «:q<Enter>» to quit Scim");
}

// resize of terminal
#ifdef SIGVOID
void
#else
int
#endif
winchg() {
    endwin();
    start_stdout();
    clearok(stdscr, TRUE);
    update(); 
    flushinp();
    show_header(input_win);
    show_celldetails(input_win);
    wrefresh(input_win);
    update(); 
    //signal(SIGWINCH, winchg);
}
