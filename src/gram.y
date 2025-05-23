%{
#include <stdlib.h>
#include <string.h>

#include "sc.h"
#include "cmds/cmds.h"
#include "interp.h"
#include "macros.h"
#include "actions/sort.h"
#include "actions/filter.h"
#include "maps.h"
#include "marks.h"
#include "xmalloc.h" // for scxfree
#include "actions/hide_show.h"
#include "cmds/cmds_normal.h"
#include "conf.h"
#include "pipe.h"
#include "main.h"
#include "file.h"
#include "tui.h"
#include "undo.h"
#include "yank.h"
#include "graph.h"
#include "utils/dictionary.h"
#include "trigger.h"
#include "actions/shift.h"
#include "clipboard.h"
#include "actions/plot.h"
#include "actions/subtotal.h"
#include "actions/freeze.h"
#include "sheet.h"
#include "vmtbl.h"
#include "cmds/cmds_command.h"

void yyerror(char *err);               // error routine for yacc (gram.y)
int yylex();
extern struct session * session;

#ifdef USELOCALE
#include <locale.h>
#endif

#ifndef MSDOS
#include <unistd.h>
#endif

#define ENULL (struct enode *) 0
%}

%union {
    int ival;
    double fval;
    struct ent_ptr ent;
    struct enode * enode;
    char * sval;
    struct range_s rval;
}

%type <ent> var
%type <fval> num
%type <rval> range
%type <rval> var_or_range
%type <sval> strarg
%type <enode> e term expr_list
%token <sval> STRING
%token <ival> COL
%token <ival> NUMBER
%token <fval> FNUMBER
%token <rval> RANGE

%token <rval> VAR
%token <sval> WORD
%token <sval> MAPWORD
%token <sval> PLUGIN

/*
 * When adding new commands, make sure that any commands that may take
 * COL as an argument precede S_FORMAT in the %token list.  All other
 * commands must come after S_FORMAT.  This is necessary so that range
 * names can be less than three letters without being parsed as column
 * names.
 */

%token S_SHOW
%token S_HIDE
%token S_SHOWROW
%token S_HIDEROW
%token S_SHOWCOL
%token S_HIDECOL
%token S_FREEZE
%token S_UNFREEZE
%token S_MARK
%token S_AUTOFIT
%token S_PAD
/*
token S_INSERTCOL
token S_OPENCOL
token S_YANKCOL
*/
%token S_DELETECOL
%token S_DATEFMT
%token S_SUBTOTAL
%token S_RSUBTOTAL
%token S_FORMAT
%token S_FMT
%token S_LET
%token S_LABEL
%token S_LEFTSTRING
%token S_RIGHTSTRING
%token S_LEFTJUSTIFY
%token S_RIGHTJUSTIFY
%token S_CENTER
%token S_SORT
%token S_FILTERON
%token S_GOTO
%token S_GOTOB
%token S_CCOPY
%token S_CPASTE
%token S_PLOT
%token S_LOCK
%token S_UNLOCK
%token S_DEFINE
%token S_UNDEFINE
%token S_DETAIL
%token S_EVAL
%token S_SEVAL
%token S_ERROR
%token S_FILL
%token S_STRTONUM
/*
 token S_ADDNOTE
 token S_DELNOTE
 token S_GET
 token S_PUT
 token S_MERGE
 token S_WRITE
 token S_TBL
 token S_COPY
 token S_MOVE
 token S_ERASE
 token S_YANK
 token S_ABBREV
 token S_UNABBREV
 token S_FRAME
 token S_FRAMETOP
 token S_FRAMEBOTTOM
 token S_FRAMELEFT
 token S_FRAMERIGHT
 token S_UNFRAME
 token S_VALUE
 token S_MDIR
 token S_AUTORUN
 token S_FKEY
 token S_SCEXT
 token S_ASCEXT
 token S_TBL0EXT
 token S_TBLEXT
 token S_LATEXEXT
 token S_SLATEXEXT
 token S_TEXEXT
 token S_UP
 token S_DOWN
 token S_LEFT
 token S_RIGHT
 token S_ENDUP
 token S_ENDDOWN
 token S_ENDLEFT
 token S_ENDRIGHT
 token S_SELECT
 token S_INSERTROW
 token S_OPENROW
*/
%token S_DELETEROW
/*
 token S_YANKROW
 token S_PULL
 token S_PULLMERGE
 token S_PULLROWS
 token S_PULLCOLS
 token S_PULLXCHG
 token S_PULLTP
 token S_PULLFMT
 token S_PULLCOPY
 token S_WHEREAMI
 token S_FGETNUM
 token S_GETFRAME
 token S_GETRANGE
 token S_QUERY
 token S_GETKEY
 token S_REDRAW
 token S_STATUS
 token S_RUN
 token S_PLUGIN
 token S_PLUGOUT
*/
%token S_VALUEIZEALL
%token S_SHIFT
%token S_GETNUM
%token S_YANKAREA
%token S_PASTEYANKED
%token S_GETSTRING
%token S_GETEXP
%token S_GETFMT
%token S_GETFORMAT
%token S_RECALC
%token S_EXECUTE
%token S_QUIT
%token S_EXPORT
%token S_REBUILD_GRAPH
%token S_PRINT_GRAPH
%token S_SYNCREFS
%token S_REDO
%token S_UNDO
%token S_IMAP
%token S_EMAP
%token S_CMAP
%token S_NEWSHEET
%token S_NEXTSHEET
%token S_PREVSHEET
%token S_DELSHEET
%token S_MOVETOSHEET
%token S_RENAMESHEET
%token S_NMAP
%token S_VMAP
%token S_INOREMAP
%token S_ENOREMAP
%token S_CNOREMAP
%token S_NNOREMAP
%token S_VNOREMAP
%token S_NUNMAP
%token S_IUNMAP
%token S_EUNMAP
%token S_CUNMAP
%token S_VUNMAP
%token S_COLOR
%token S_CELLCOLOR
%token S_UNFORMAT
%token S_REDEFINE_COLOR
%token S_DEFINE_COLOR
%token S_SET
%token S_FCOPY
%token S_FSUM
%token S_TRIGGER
%token S_UNTRIGGER

%token S_OFFSCR_SC_COLS
%token S_OFFSCR_SC_ROWS
%token S_NB_FROZEN_ROWS
%token S_NB_FROZEN_COLS
%token S_NB_FROZEN_SCREENROWS
%token S_NB_FROZEN_SCREENCOLS

%token K_AUTOBACKUP
%token K_NOAUTOBACKUP
%token K_AUTOCALC
%token K_NOAUTOCALC
%token K_DEBUG
%token K_NODEBUG
%token K_TRG
%token K_NOTRG
%token K_EXTERNAL_FUNCTIONS
%token K_NOEXTERNAL_FUNCTIONS
%token K_EXEC_LUA
%token K_NOEXEC_LUA
%token K_HALF_PAGE_SCROLL
%token K_NOHALF_PAGE_SCROLL
%token K_NOCURSES
%token K_CURSES
%token K_NUMERIC
%token K_NONUMERIC
%token K_NUMERIC_DECIMAL
%token K_NONUMERIC_DECIMAL
%token K_NUMERIC_ZERO
%token K_NONUMERIC_ZERO
%token K_OVERLAP
%token K_NOOVERLAP
%token K_INPUT_BAR_BOTTOM
%token K_IGNORE_HIDDEN
%token K_NOIGNORE_HIDDEN
%token K_INPUT_EDIT_MODE
%token K_UNDERLINE_GRID
%token K_TRUNCATE
%token K_NOTRUNCATE
%token K_AUTOWRAP
%token K_NOAUTOWRAP
%token K_QUIET
%token K_NOQUIET
%token K_QUIT_AFTERLOAD
%token K_NOQUIT_AFTERLOAD
%token K_XLSX_READFORMULAS
%token K_NOXLSX_READFORMULAS
%token K_DEFAULT_COPY_TO_CLIPBOARD_CMD
%token K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD
%token K_COPY_TO_CLIPBOARD_DELIMITED_TAB
%token K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB
%token K_COPY_TO_CLIPBOARD_WYSIWYG
%token K_NOCOPY_TO_CLIPBOARD_WYSIWYG
%token K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD
%token K_IMPORT_DELIMITED_TO_TEXT
%token K_IGNORECASE
%token K_NOIGNORECASE
%token K_TM_GMTOFF
%token K_COMMAND_TIMEOUT
%token K_MAPPING_TIMEOUT
%token K_NEWLINE_ACTION
%token K_SHOW_CURSOR
%token K_NOSHOW_CURSOR
%token K_ERROR
%token K_INVALID
%token K_FIXED
%token K_SUM
%token K_PROD
%token K_AVG
%token K_STDDEV
%token K_COUNT
%token K_ROWS
%token K_COLS
%token K_ABS
%token K_FROW
%token K_FCOL
%token K_ACOS
%token K_ASIN
%token K_ATAN
%token K_ATAN2
%token K_CEIL
%token K_COS
%token K_EXP
%token K_FABS
%token K_FLOOR
%token K_HYPOT
%token K_LN
%token K_LOG
%token K_PI
%token K_POW
%token K_SIN
%token K_SQRT
%token K_TAN
%token K_DTR
%token K_RTD
%token K_MAX
%token K_MIN
%token K_RND
%token K_ROUND
%token K_IF
%token K_PV
%token K_FV
%token K_PMT
%token K_HOUR
%token K_MINUTE
%token K_SECOND
%token K_MONTH
%token K_DAY
%token K_YEAR
%token K_NOW
%token K_DATE
%token K_DTS
%token K_TTS
%token K_FMT
%token K_REPLACE
%token K_SUBSTR
%token K_UPPER
%token K_LOWER
%token K_CAPITAL
%token K_STON
%token K_SLEN
%token K_EQS
%token K_EXT
%token K_EVALUATE
%token K_SEVALUATE
%token K_LUA
%token K_NVAL
%token K_SVAL
%token K_LOOKUP
%token K_HLOOKUP
%token K_VLOOKUP
%token K_INDEX
%token K_STINDEX
%token K_GETENT
/*
token K_AUTO
token K_AUTOINSERT
token K_AUTOWRAP
token K_CSLOP
token K_BYROWS
token K_BYCOLS
token K_OPTIMIZE
token K_ITERATIONS
token K_NUMERIC
token K_PRESCALE
token K_EXTFUN
token K_CELLCUR
token K_TOPROW
token K_COLOR
token K_COLORNEG
token K_COLORERR
*/
%token K_TBLSTYLE
%token K_TBL
%token K_LATEX
%token K_SLATEX
%token K_TEX
%token K_FRAME
%token K_RNDTOEVEN
%token K_FILENAME
%token K_MYROW
%token K_MYCOL
%token K_LASTROW
%token K_LASTCOL
%token K_COLTOA
%token K_CRACTION
%token K_CRROW
%token K_CRCOL
%token K_ROWLIMIT
%token K_COLLIMIT
%token K_PAGESIZE
%token K_ERR
%token K_REF
%token K_LOCALE
%token K_SET8BIT
%token K_ASCII
%token K_CHR
%token K_FACT


%right ';'
%left '?' ':'
%left '|'
%left '&'
%nonassoc '<' '=' '>' '!'
%left '+' '-' '#'
%left '*' '/' '%'
%left '^'


%%
command:
         S_LET var_or_range '=' e {
                                  struct roman * roman = session->cur_doc;
                                  struct sheet * sh = roman->cur_sh;
                                  let(roman, sh, $2.left.vp, $4);
                                  }

    |    S_LET var_or_range '='
                                  {
                                  // TODO get this code out of gram.y - reeval cells that depends on $2
                                  extern graphADT graph;
                                  struct roman * roman = session->cur_doc;
                                  struct sheet * sh = roman->cur_sh;
#ifdef UNDO
                                  // here we save in undostruct, all the ents that depends on the deleted one (before change)
                                  ents_that_depends_on_range(sh, $2.left.vp->row, $2.left.vp->col, $2.left.vp->row, $2.left.vp->col);
                                  create_undo_action();
                                  copy_to_undostruct(sh, $2.left.vp->row, $2.left.vp->col, $2.left.vp->row, $2.left.vp->col, UNDO_DEL, HANDLE_DEPS, NULL);
#endif

                                  if (getVertex(graph, sh, lookat(sh, $2.left.vp->row, $2.left.vp->col), 0) != NULL) destroy_vertex(sh, lookat(sh, $2.left.vp->row, $2.left.vp->col));

                                  $2.left.vp->v = (double) 0.0;
                                  if ($2.left.vp->expr && !($2.left.vp->flags & is_strexpr)) {
                                      efree($2.left.vp->expr);
                                      $2.left.vp->expr = NULL;
                                  }
                                  $2.left.vp->cellerror = CELLOK;
                                  $2.left.vp->flags &= ~is_valid;
                                  $2.left.vp->flags |= is_changed;
                                  roman->modflg++;

                                  // clearing the value counts as a write, so run write triggers
                                  if (( $2.left.vp->trigger  ) && (($2.left.vp->trigger->flag & TRG_WRITE) == TRG_WRITE))
                                      do_trigger($2.left.vp, TRG_WRITE);

#ifdef UNDO
                                  // here we save in undostruct, all the ents that depends on the deleted one (after change)
                                  copy_to_undostruct(sh, $2.left.vp->row, $2.left.vp->col, $2.left.vp->row, $2.left.vp->col, UNDO_ADD, HANDLE_DEPS, NULL);
                                  extern struct ent_ptr * deps;
                                  if (deps != NULL) {
                                      free(deps);
                                      deps = NULL;
                                  }
                                  end_undo_action();
#endif
                                  }

    |    S_LABEL var_or_range '=' e       {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, $2.left.vp, $4, 0);
                                          }

    |    S_LEFTSTRING var_or_range '=' e  {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, $2.left.vp, $4, -1);
                                          }
    |    S_RIGHTSTRING var_or_range '=' e {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, $2.left.vp, $4, 1);
                                          }
    |    S_LEFTJUSTIFY var_or_range  {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            ljustify(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col); }
    |    S_RIGHTJUSTIFY var_or_range {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            rjustify(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col); }

    |    S_CENTER var_or_range       {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            center(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col); }
    |    S_FMT var_or_range STRING   {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            format_cell(sh, $2.left.vp, $2.right.vp, $3);
                                            scxfree($3);
                                     }

    |    S_DATEFMT var_or_range STRING {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            dateformat(sh, $2.left.vp, $2.right.vp, $3);
                                            scxfree($3);
                                       }
    |    S_DATEFMT STRING            {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       dateformat(sh, lookat(sh, sh->currow, sh->curcol), lookat(sh, sh->currow, sh->curcol), $2);
                                       scxfree($2); }
/* to be sc compatible */
    |    S_HIDE COL                  { hide_col($2, 1); }        // hide de una unica columna
    |    S_HIDE NUMBER               { hide_row($2, 1); }        // hide de una unica fila
    |    S_SHOW COL                  { show_col($2, 1); }        // show de una unica columna
    |    S_SHOW NUMBER               { show_row($2, 1); }        // show de una unica fila

/* more scripting commands */
    |    S_DELETECOL COL             {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       deletecol(sh, $2, 1);
                                     }
    |    S_DELETEROW NUMBER          {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       deleterow(sh, $2, 1);
                                     }

/* added for sc-im */
    |    S_HIDECOL COL               {
                                       hide_col($2, 1); }        // hide de una unica columna
    |    S_SHOWCOL COL               {
                                       show_col($2, 1); }        // show de una unica columna
    |    S_HIDEROW NUMBER            {
                                       hide_row($2, 1); }        // hide de una unica fila
    |    S_SHOWROW NUMBER            {
                                       show_row($2, 1); }        // show de una unica fila
    |    S_SHOWCOL COL ':' COL       {
                                       show_col($2, $4-$2+1); }  // show de un rango de columnas
    |    S_SHOWROW NUMBER ':' NUMBER {
                                       show_row($2, $4-$2+1); }  // show de un rango de filas
    |    S_HIDECOL COL ':' COL       {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       int c = sh->curcol, arg;
                                       if ($2 < $4) {
                                            sh->curcol = $2;
                                            arg = $4 - $2 + 1;
                                       } else {
                                            sh->curcol = $4;
                                            arg = $2 - $4 + 1;
                                       }
                                       hide_col($2, arg);        // hide de un rango de columnas
                                       sh->curcol = c < sh->curcol ? c : c < sh->curcol + arg ? sh->curcol : c - arg;
                                     }
    |    S_HIDEROW NUMBER ':' NUMBER {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       int r = sh->currow, arg;      // hide de un rango de filas
                                       if ($2 < $4) {
                                           sh->currow = $2;
                                           arg = $4 - $2 + 1;
                                       } else {
                                           sh->currow = $4;
                                           arg = $2 - $4 + 1;
                                       }
                                       hide_row($2, arg);
                                       sh->currow = r < sh->currow ? r : r < sh->currow + arg ? sh->currow : r - arg;
                                     }

    |    S_VALUEIZEALL               {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       valueize_area(sh, 0, 0, sh->maxrow, sh->maxcol); }

    |    S_SHIFT var_or_range STRING {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       if (strlen($3) != 1 || ($3[0] != 'h' && $3[0] != 'j' && $3[0] != 'k' && $3[0] != 'l')) {
                                           sc_error("wrong parameter for shift command");
                                       } else {
                                           wchar_t wstr[2] = L"";
                                           swprintf(wstr, BUFFERSIZE, L"%c", $3[0]);
                                           shift(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, wstr[0]);
                                       }
                                       scxfree($3);
                                     }

    |    S_MARK COL var_or_range     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       set_cell_mark($2 + 97, sh, $3.left.vp->row, $3.left.vp->col);
                                     }

    |    S_MARK COL var_or_range var_or_range {
                                          struct roman * roman = session->cur_doc;
                                          struct sheet * sh = roman->cur_sh;
                                          srange * sr = create_range('\0', '\0', $3.left.vp, $4.left.vp);
                                          unselect_ranges();
                                          set_range_mark($2 + 97, sh, sr);
                                     }

    |    S_MARK COL STRING var_or_range     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh;
                                       if ((sh = search_sheet(roman, $3)) != NULL ) {
                                           set_cell_mark($2 + 97, sh, $4.left.vp->row, $4.left.vp->col);
                                       }
                                       scxfree($3);
                                     }

    |    S_MARK COL STRING var_or_range var_or_range {
                                          struct roman * roman = session->cur_doc;
                                          struct sheet * sh;
                                          if ((sh = search_sheet(roman, $3)) != NULL ) {
                                              srange * sr = create_range('\0', '\0', $4.left.vp, $5.left.vp);
                                              unselect_ranges();
                                              set_range_mark($2 + 97, sh, sr);
                                          }
                                          scxfree($3);
                                     }
    |    S_FILL var_or_range num num {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       fill(sh, $2.left.vp, $2.right.vp, $3, $4);
                                     }

    |    S_FILL num num              { sc_error("Not enough parameters for fill command"); }

    |    S_FREEZE NUMBER ':' NUMBER  {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, $2, 0), lookat(sh, $4, 0), 1, 'r'); }
    |    S_FREEZE NUMBER             {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, $2, 0), lookat(sh, $2, 0), 1, 'r'); }
    |    S_FREEZE COL ':' COL        {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, $2), lookat(sh, 0, $4), 1, 'c'); }
    |    S_FREEZE COL                {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, $2), lookat(sh, 0, $2), 1, 'c'); }
    |    S_UNFREEZE NUMBER ':' NUMBER{
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, $2, 0), lookat(sh, $4, 0), 0, 'r'); }
    |    S_UNFREEZE NUMBER           {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, $2, 0), lookat(sh, $2, 0), 0, 'r'); }
    |    S_UNFREEZE COL ':' COL      {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, $2), lookat(sh, 0, $4), 0, 'c'); }
    |    S_UNFREEZE COL              {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, $2), lookat(sh, 0, $2), 0, 'c'); }

    |    S_SORT range STRING         { sortrange(session->cur_doc->cur_sh, $2.left.vp, $2.right.vp, $3);
                                       //scxfree($3);
                                       //do not free here
                                     }
    |    S_SUBTOTAL range COL STRING COL {
                                       subtotal($2.left.vp->row, $2.left.vp->col, $2.right.vp->row,
                                                $2.right.vp->col, $3, $4, $5, 0);
                                       scxfree($4);
                                     }
    |    S_RSUBTOTAL range COL STRING COL {
                                       subtotal($2.left.vp->row, $2.left.vp->col, $2.right.vp->row,
                                                 $2.right.vp->col, $3, $4, $5, 1);
                                       scxfree($4);
                                     }
/*
    |    S_GET strarg {
/* This tmp hack is because readfile recurses back through yyparse.
                    char *tmp;
                    tmp = $2;
                    readfile(tmp, 1);
                    scxfree(tmp);
                }
*/
    |    S_AUTOFIT COL ':' COL       { auto_fit(session->cur_doc->cur_sh, $2, $4, DEFWIDTH); }  // auto justificado de columnas
    |    S_AUTOFIT COL               { auto_fit(session->cur_doc->cur_sh, $2, $2, DEFWIDTH); }  // auto justificado de columna

    |    S_PAD NUMBER COL ':' COL  {
                                       pad(session->cur_doc->cur_sh, $2, 0, $3, session->cur_doc->cur_sh->maxrow, $5); }
    |    S_PAD NUMBER COL          {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       pad(sh, $2, 0, $3, sh->maxrow, $3); }
    |    S_PAD NUMBER var_or_range {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       pad(sh, $2, $3.left.vp->row, $3.left.vp->col, $3.right.vp->row, $3.right.vp->col); }
    |    S_GETFORMAT COL           {   getformat($2, fdoutput); }
    |    S_FORMAT COL NUMBER NUMBER NUMBER {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       doformat(sh, $2,$2,$3,$4,$5);
                                       }
    |    S_FORMAT NUMBER NUMBER      {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       dorowformat(sh, $2, $3);
                                     }

    |    S_FILTERON range            { enable_filters($2.left.vp, $2.right.vp);
                                     }
    |    S_GOTO var_or_range var_or_range {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       moveto(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, $3.left.vp->row, $3.left.vp->col);
                                     }
    |    S_GOTO var_or_range     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       moveto(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, -1, -1);
                                 }
    |    S_GOTO num              {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       num_search(sh, $2, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 1); }
    |    S_GOTO STRING           {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $2, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 1); }
                                   //scxfree($2); shall not free here
    |    S_GOTO '#' STRING       {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $3, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 1, 1); }
                                   //scxfree($3); shall not free here
    |    S_GOTO '%' STRING       {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $3, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 2, 1); }
                                   //scxfree($3); shall not free here

    |    S_GOTOB num              {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       num_search(sh, $2, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 0); }

    |    S_GOTOB STRING           {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $2, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 0); }

    |    S_GOTOB '#' STRING       {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $3, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 1, 0); }

    |    S_GOTOB '%' STRING       {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, $3, 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 2, 0); }

 // |    S_GOTO WORD             { /* don't repeat last goto on "unintelligible word" */ ; }

    |    S_CCOPY range           { copy_to_clipboard($2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col); }
    |    S_STRTONUM range        { convert_string_to_number($2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col); }
    |    S_CPASTE                { paste_from_clipboard(); }
    |    S_LOCK var_or_range     {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   lock_cells(sh, $2.left.vp, $2.right.vp);
                                 }
    |    S_UNLOCK var_or_range   {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   unlock_cells(sh, $2.left.vp, $2.right.vp);
                                 }
    |    S_NEWSHEET STRING       {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh;

                                   // do not need to alloc a new 'Sheet1'
                                   // just reuse the just allocated 'Sheet1' in load_file();
                                   if (! strcmp($2, "Sheet1") && (sh = search_sheet(roman, $2)) != NULL && sh->flags & is_allocated) {
                                       sh->flags &= ~is_allocated;
                                       sh->flags |= is_empty;
                                       scxfree($2);
                                       chg_mode('.');

                                   // if a sheet already exists with the name we are trying to create
                                   } else if ((sh = search_sheet(roman, $2)) != NULL ) {
                                       sc_info("sheet already exist with that name");
                                       scxfree($2);
                                       chg_mode('.');

                                   // if a just allocated 'Sheet1' exists, reuse it and do not malloc a new one.
                                   } else if ((sh = search_sheet(roman, "Sheet1")) != NULL && sh->flags & is_allocated) {
                                       sh->flags &= ~is_allocated;
                                       sh->flags |= is_empty;
                                       free(sh->name);
                                       sh->name = $2;
                                       chg_mode('.');
                                       ui_update(TRUE);

                                   // if reached here, now yes malloc a new one
                                   } else {
                                       roman->cur_sh = new_sheet(roman, $2);
                                       growtbl(roman->cur_sh, GROWNEW, 0, 0);
                                       erasedb(roman->cur_sh, 0);
                                       scxfree($2);
                                       roman->modflg++;
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
    |    S_DELSHEET STRING       {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh;
                                   if ((sh = search_sheet(roman, $2)) == NULL ) {
                                       sc_info("No sheet exists with that name");
                                       scxfree($2);
                                   } else if (roman->cur_sh == sh && sh->next == NULL && sh->prev == NULL) {
                                       sc_info("Cannot delete the only sheet of document");
                                       scxfree($2);
                                   } else {
                                       if (roman->cur_sh == sh && sh->next != NULL)
                                           roman->cur_sh = sh->next;
                                       else if (roman->cur_sh == sh)
                                           roman->cur_sh = sh->prev;
                                       delete_sheet(roman, sh, 0);
                                       sh = NULL;
                                       roman->modflg++;
                                       scxfree($2);
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
    |    S_DELSHEET              {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   if (sh->next == NULL && sh->prev == NULL) {
                                       sc_info("Cannot delete the only sheet of document");
                                   } else {
                                       if (roman->cur_sh == sh && sh->next != NULL)
                                           roman->cur_sh = sh->next;
                                       else if (roman->cur_sh == sh)
                                           roman->cur_sh = sh->prev;
                                       delete_sheet(roman, sh, 0);
                                       sh = NULL;
                                       roman->modflg++;
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
    |    S_NEXTSHEET             {
                                   struct roman * roman = session->cur_doc;
                                   if (roman->cur_sh->next != NULL) {
                                       roman->cur_sh = roman->cur_sh->next;
                                   } else if (roman->next != NULL) {
                                       session->cur_doc = roman->next;
                                       session->cur_doc->cur_sh = session->cur_doc->first_sh;
                                   } else {
                                       session->cur_doc = session->first_doc;
                                       session->cur_doc->cur_sh = session->cur_doc->first_sh;
                                   }
                                   chg_mode('.');
                                   ui_update(TRUE);
                                 }
    |    S_PREVSHEET             {
                                   struct roman * roman = session->cur_doc;
                                   if (roman->cur_sh->prev != NULL) {
                                       roman->cur_sh = roman->cur_sh->prev;
                                   } else if (roman->prev != NULL) {
                                       session->cur_doc = roman->prev;
                                       session->cur_doc->cur_sh = session->cur_doc->last_sh;
                                   } else {
                                       session->cur_doc = session->last_doc;
                                       session->cur_doc->cur_sh = session->cur_doc->last_sh;
                                   }
                                   chg_mode('.');
                                   ui_update(TRUE);
                                 }

    |    S_MOVETOSHEET STRING    {
                                   struct sheet * sh;
                                   if ((sh = search_sheet(session->cur_doc, $2)) != NULL )
                                       session->cur_doc->cur_sh = sh;
                                   scxfree($2);
                                 }
    |    S_RENAMESHEET STRING    {
                                   struct sheet * sh = session->cur_doc->cur_sh;
                                   if (sh->name != NULL) free(sh->name);
                                   session->cur_doc->modflg++;
                                   sh->name = $2;
                                   chg_mode('.');
                                   ui_show_header();
                                 }

    |    S_NMAP STRING STRING    {
                                   add_map($2, $3, NORMAL_MODE, 1);
                                   scxfree($2);
                                   scxfree($3);
                                 }

    |    S_IMAP STRING STRING    {
                                   add_map($2, $3, INSERT_MODE, 1);
                                   scxfree($2);
                                   scxfree($3);
                                 }
    |    S_EMAP STRING STRING    {
                                   add_map($2, $3, EDIT_MODE, 1);
                                   scxfree($2);
                                   scxfree($3);
                                 }
    |    S_VMAP STRING STRING    {
                                   add_map($2, $3, VISUAL_MODE, 1);
                                   scxfree($2);
                                   scxfree($3);
                                 }
    |    S_CMAP STRING STRING    {
                                   add_map($2, $3, COMMAND_MODE, 1);
                                   scxfree($2);
                                   scxfree($3);
                                 }
    |    S_NNOREMAP STRING STRING  {
                                   add_map($2, $3, NORMAL_MODE, 0);
                                   scxfree($2);
                                   scxfree($3);
                                   }

    |    S_INOREMAP STRING STRING  {
                                   add_map($2, $3, INSERT_MODE, 0);
                                   scxfree($2);
                                   scxfree($3);
                                   }
    |    S_ENOREMAP STRING STRING  {
                                   add_map($2, $3, EDIT_MODE, 0);
                                   scxfree($2);
                                   scxfree($3);
                                   }
    |    S_VNOREMAP STRING STRING  {
                                   add_map($2, $3, VISUAL_MODE, 0);
                                   scxfree($2);
                                   scxfree($3);
                                   }
    |    S_CNOREMAP STRING STRING  {
                                   add_map($2, $3, COMMAND_MODE, 0);
                                   scxfree($2);
                                   scxfree($3);
                                   }

    |    S_NUNMAP STRING           {
                                   del_map($2, NORMAL_MODE);
                                   scxfree($2);
                                   }

    |    S_IUNMAP STRING           {
                                   del_map($2, INSERT_MODE);
                                   scxfree($2);
                                   }
    |    S_EUNMAP STRING           {
                                   del_map($2, EDIT_MODE);
                                   scxfree($2);
                                   }
    |    S_VUNMAP STRING           {
                                   del_map($2, VISUAL_MODE);
                                   scxfree($2);
                                   }

    |    S_CUNMAP STRING           {
                                   del_map($2, COMMAND_MODE);
                                   scxfree($2);
                                   }
    |    S_COLOR STRING            {
#ifdef USECOLORS
                                   chg_color($2);
#endif
                                   scxfree($2);
                                   }

    |    S_DETAIL var              {
                                   char det[BUFFERSIZE] = "";
                                   struct ent * e = $2.vp;

                                   sprintf(det + strlen(det), "row: %d\n", e->row);
                                   sprintf(det + strlen(det), "col: %d\n", e->col);
                                   sprintf(det + strlen(det), "cellerror: %d\n"   , e->cellerror);
                                   sprintf(det + strlen(det), "flags:\n");
                                   sprintf(det + strlen(det), "is_valid: %d\n"    , e->flags & is_valid );
                                   sprintf(det + strlen(det), "is_deleted: %d\n"  , e->flags & is_deleted);
                                   sprintf(det + strlen(det), "is_changed: %d\n"  , e->flags & is_changed);
                                   sprintf(det + strlen(det), "is_strexpr: %d\n"  , e->flags & is_strexpr);
                                   sprintf(det + strlen(det), "is_leftflush: %d\n", e->flags & is_leftflush);
                                   sprintf(det + strlen(det), "is_locked: %d\n"   , e->flags & is_locked);
                                   sprintf(det + strlen(det), "is_label: %d\n"    , e->flags & is_label);
                                   sprintf(det + strlen(det), "iscleared: %d\n"   , e->flags & iscleared);
                                   sprintf(det + strlen(det), "may_sync: %d\n"    , e->flags & may_sync);
                                   ui_show_text((char *) &det);
                                   }

    |    S_CELLCOLOR var_or_range STRING
                                   {
#ifdef USECOLORS
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   if ( ! get_conf_int("nocurses"))
                                       color_cell(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, $3);
#endif
                                   scxfree($3);
                                   }

    |    S_TRIGGER var_or_range STRING  {
                                          set_trigger(session->cur_doc->cur_sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, $3);
                                          scxfree($3);
                                        }

    |    S_OFFSCR_SC_COLS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->offscr_sc_cols = $2;
                                 }
    |    S_OFFSCR_SC_ROWS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->offscr_sc_rows = $2;
                                 }
    |    S_NB_FROZEN_ROWS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_rows = $2;
                                 }
    |    S_NB_FROZEN_COLS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_cols = $2;
                                 }
    |    S_NB_FROZEN_SCREENROWS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_screenrows = $2;
                                 }
    |    S_NB_FROZEN_SCREENCOLS NUMBER {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_screencols = $2;
                                 }

    |    S_UNTRIGGER var_or_range  {
                                   del_trigger(session->cur_doc->cur_sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col);
                                   }

    |    S_CELLCOLOR STRING        {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses"))
                                       color_cell(sh, sh->currow, sh->curcol, sh->currow, sh->curcol, $2);
#endif
                                   scxfree($2);
                                   }

    |    S_UNFORMAT var_or_range   {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses")) unformat(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col);
#endif
                                   }

    |    S_UNFORMAT                {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses")) unformat(sh, sh->currow, sh->curcol, sh->currow, sh->curcol);
#endif
                                   }

    |    S_REDEFINE_COLOR STRING NUMBER NUMBER NUMBER {
                                         redefine_color($2, $3, $4, $5);
                                         scxfree($2); }

    |    S_DEFINE_COLOR STRING NUMBER NUMBER NUMBER {
                                         define_color($2, $3, $4, $5);
                                         scxfree($2); }

    |    S_FCOPY                   { fcopy(session->cur_doc->cur_sh, ""); }
    |    S_FCOPY strarg            { fcopy(session->cur_doc->cur_sh, $2); }
    |    S_FSUM                    { fsum(session->cur_doc->cur_sh);  }

    |    S_PLOT STRING var_or_range       {
                                     plot($2, $3.left.vp->row, $3.left.vp->col, $3.right.vp->row, $3.right.vp->col);
                                     scxfree($2);
                                   }
/*  |    S_SET STRING              { parse_str(user_conf_d, $2, TRUE);
                                     scxfree($2);
                                   }
*/
    |    S_SET setlist             { //if (! loading) sc_debug("INT: Config value changed");
                                   }
/*
    |    S_DEFINE strarg           {
                                   struct roman * roman = session->cur_doc;
                                   struct ent_ptr arg1, arg2;
                                          arg1.vp = lookat(roman->cur_sh, showsr, showsc);
                                          arg1.vf = 0;
                                          arg2.vp = lookat(roman->cur_sh, roman->cur_sh->currow, roman->cur_sh->curcol);
                                          arg2.vf = 0;
                                          if (arg1.vp == arg2.vp )
                                              add_range($2, arg2, arg2, 0);
                                          else
                                              add_range($2, arg1, arg2, 1);
                                   }
*/
    |    S_DEFINE strarg range     { add_range($2, $3.left, $3.right, 1); }
    |    S_DEFINE strarg var       { add_range($2, $3, $3, 0); }
/*  |    S_DEFINE strarg NUMBER    { info("%s %d", $2, $3); get_key(); } */
    |    S_UNDEFINE var_or_range   { del_range($2.left.vp, $2.right.vp); }

    |    S_EVAL e                  {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     eval_result = eval(sh, NULL, $2, 0);
                                     efree($2);
                                   }
    |    S_EXECUTE STRING          {

                                     inputline[0]=L'\0';

                                     #ifdef HISTORY_FILE
                                            commandline_history = (struct history *) create_history(':');
                                            load_history(commandline_history, ':'); // load the command history file
                                     #endif
                                     #ifdef INS_HISTORY_FILE
                                            insert_history = (struct history *) create_history('=');
                                            load_history(insert_history, '='); // load the insert history file
                                     #endif

                                     (void) swprintf(inputline, BUFFERSIZE, L"%s", $2);

                                     struct block * auxb = (struct block *) create_buf();
                                     addto_buf(auxb, OKEY_ENTER);
                                     do_commandmode(auxb);
                                     flush_buf(auxb);
                                     erase_buf(auxb);
                                     auxb = NULL;
                                     inputline[0]=L'\0';
                                     scxfree($2);
                                   }
    |    S_EXPORT STRING STRING    {
                                     struct roman * roman = session->cur_doc;
                                     swprintf(inputline, BUFFERSIZE, L"e! %s %s", $2, $3);
                                     do_export(0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol);
                                     scxfree($2);
                                     scxfree($3);
                                   }
    |    S_QUIT                    {
                                     printf("quitting. unsaved changes will be lost.\n");
                                     shall_quit = 2; // unsaved changes are lost!
                                   }
    |    S_REBUILD_GRAPH           {
                                     rebuild_graph();
                                     ui_update(FALSE);
                                   }

    |    S_PRINT_GRAPH             { print_vertexs(); }
    |    S_SYNCREFS                { sync_refs(session->cur_doc->cur_sh); }

    |    S_UNDO                    {
#ifdef UNDO
                                     do_undo();
                                     // sync_refs(session->cur_doc->cur_sh);
                                     EvalAll();
                                     ui_update(TRUE);
#endif
                                   }

    |    S_REDO                    {
#ifdef UNDO
                                     do_redo();
                                     // sync_refs(session->cur_doc->cur_sh);
                                     EvalAll();
                                     ui_update(TRUE);
#endif
                                   }

// For scripting and piping
    |    S_RECALC                  {
                                     EvalAll();
                                     //ui_update(1);
                                     //changed = 0;
                                   }
    |    S_GETNUM var_or_range     {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     getnum(sh, $2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, fdoutput);
                                   }

    |    S_GETNUM '{' STRING '}' '!' var_or_range {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, $3)) == NULL )
                                         sh = roman->cur_sh;
                                     getnum(sh, $6.left.vp->row, $6.left.vp->col, $6.right.vp->row, $6.right.vp->col, fdoutput);
                                     scxfree($3);
                                   }
    |    S_GETSTRING var_or_range  { getstring($2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, fdoutput); }

    |    S_GETEXP var_or_range     { getexp($2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, fdoutput); }


    |    S_GETFMT var_or_range     { getfmt($2.left.vp->row, $2.left.vp->col, $2.right.vp->row, $2.right.vp->col, fdoutput); }

    |    S_YANKAREA '{' STRING '}' '!' var_or_range STRING {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, $3)) == NULL )
                                         sh = roman->cur_sh;
                                     yank_area(sh, $6.left.vp->row, $6.left.vp->col, $6.right.vp->row, $6.right.vp->col, $7[0], 1);
                                     scxfree($3);
                                     scxfree($7);
                                   }
    |    S_PASTEYANKED '{' STRING '}' NUMBER STRING {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, $3)) == NULL )
                                         sh = roman->cur_sh;
                                     paste_yanked_ents(sh, $5, $6[0]);
                                     scxfree($3);
                                     scxfree($6);
                                   }
    |    S_SEVAL e                 {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     seval_result = seval(sh, NULL, $2, 0); // always make sure this seval_result is always freed afterwards
                                     efree($2);
                                   }
    |    S_ERROR STRING            { sc_error($2);
                                     //free $2
                                   }

    |    // nothing
    |    error   {
                     sc_error("syntax error: %s", line);
                     YYABORT;

                     //linelim = 0;
                     //yyparse();

                     //line[0]='\0';
                     //linelim = -1;
                     //yyclearin;
                     //yyerrok;
                 };

term:   var                       {
                                    if ($1.vf & GET_ENT)
                                        $$ = $1.expr;
                                    else {
                                       $1.sheet = NULL;
                                       $$ = new_var(O_VAR, $1);
                                       $$->e.r.left.expr = NULL;
                                       $$->e.r.right.expr = NULL;
                                    }
                                  }

        | '{' STRING '}' '!' var  {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh;
                                    if ((sh = search_sheet(roman, $2)) != NULL) {
                                        struct ent_ptr ep;
                                        ep.vf = $5.vf;
                                        ep.vp = lookat(sh, $5.vp->row, $5.vp->col);
                                        ep.sheet = sh;
                                        $$ = new_var(O_VAR, ep);
                                        $$->e.r.left.expr = NULL;
                                        $$->e.r.right.expr = NULL;
                                        scxfree($2);
                                    } else {
                                        //sc_debug("not sheet found");
                                        $$ = NULL;
                                        scxfree($2);
                                    }
                                  }

        | '@' K_FIXED term        { $$ = new('f', $3, ENULL); }

        | '(' '@' K_FIXED ')' term
                                  { $$ = new('F', $5, ENULL); }

        | '@' K_SUM '(' var_or_range ')'
                                  { $$ = new(SUM, new_range(REDUCE | SUM, $4), ENULL); }
        | '@' K_SUM  '(' range ',' e ')'
                                  { $$ = new(SUM, new_range(REDUCE | SUM, $4), $6); }
        | '@' K_PROD '(' var_or_range ')'
                                  { $$ = new(PROD, new_range(REDUCE | PROD, $4), ENULL); }
        | '@' K_PROD  '(' range ',' e ')'
                                  { $$ = new(PROD, new_range(REDUCE | PROD, $4), $6); }
        | '@' K_AVG '(' var_or_range ')'
                                  { $$ = new(AVG, new_range(REDUCE | AVG, $4), ENULL); }
        | '@' K_AVG  '(' range ',' e ')'
                                  { $$ = new(AVG, new_range(REDUCE | AVG, $4), $6); }
        | '@' K_STDDEV '(' var_or_range ')'
                                  { $$ = new(STDDEV, new_range(REDUCE | STDDEV, $4), ENULL); }
        | '@' K_STDDEV  '(' range ',' e ')'
                                  { $$ = new(STDDEV, new_range(REDUCE | STDDEV, $4), $6); }
        | '@' K_COUNT '(' var_or_range ')'
                                  { $$ = new(COUNT, new_range(REDUCE | COUNT, $4), ENULL); }
        | '@' K_COUNT  '(' range ',' e ')'
                                  { $$ = new(COUNT, new_range(REDUCE | COUNT, $4), $6); }
        | '@' K_MAX '(' var_or_range ')'
                                  { $$ = new(MAX, new_range(REDUCE | MAX, $4), ENULL); }
        | '@' K_MAX  '(' range ',' e ')'
                                  { $$ = new(MAX, new_range(REDUCE | MAX, $4), $6); }
        | '@' K_MAX '(' e ',' expr_list ')'
                                  { $$ = new(LMAX, $6, $4); }
        | '@' K_MIN '(' var_or_range ')'
                                  { $$ = new(MIN, new_range(REDUCE | MIN, $4), ENULL); }
        | '@' K_MIN  '(' range ',' e ')'
                                  { $$ = new(MIN, new_range(REDUCE | MIN, $4), $6); }
        | '@' K_MIN '(' e ',' expr_list ')'
                                  { $$ = new(LMIN, $6, $4); }
        | '@' K_ROWS '(' var_or_range ')'
                                  { $$ = new_range(REDUCE | 'R', $4); }
        | '@' K_COLS '(' var_or_range ')'
                                  { $$ = new_range(REDUCE | 'C', $4); }
        | '@' K_ABS '(' e ')'     { $$ = new(ABS, $4, ENULL); }
        | '@' K_FROW '(' e ')'    { $$ = new(FROW, $4, ENULL); }
        | '@' K_FCOL '(' e ')'    { $$ = new(FCOL, $4, ENULL); }
        | '@' K_ACOS '(' e ')'    { $$ = new(ACOS, $4, ENULL); }
        | '@' K_ASIN '(' e ')'    { $$ = new(ASIN, $4, ENULL); }
        | '@' K_ATAN '(' e ')'    { $$ = new(ATAN, $4, ENULL); }
        | '@' K_ATAN2 '(' e ',' e ')'
                                  { $$ = new(ATAN2, $4, $6); }
        | '@' K_CEIL '(' e ')'    { $$ = new(CEIL, $4, ENULL); }
        | '@' K_COS '(' e ')'     { $$ = new(COS, $4, ENULL); }
        | '@' K_EXP '(' e ')'     { $$ = new(EXP, $4, ENULL); }
        | '@' K_FABS '(' e ')'    { $$ = new(FABS, $4, ENULL); }
        | '@' K_FLOOR '(' e ')'   { $$ = new(FLOOR, $4, ENULL); }
        | '@' K_HYPOT '(' e ',' e ')'
                                  { $$ = new(HYPOT, $4, $6); }
        | '@' K_LN '(' e ')'      { $$ = new(LOG, $4, ENULL); }
        | '@' K_LOG '(' e ')'     { $$ = new(LOG10, $4, ENULL); }
        | '@' K_POW '(' e ',' e ')'
                                  { $$ = new(POW, $4, $6); }
        | '@' K_SIN '(' e ')'     { $$ = new(SIN, $4, ENULL); }
        | '@' K_SQRT '(' e ')'    { $$ = new(SQRT, $4, ENULL); }
        | '@' K_TAN '(' e ')'     { $$ = new(TAN, $4, ENULL); }
        | '@' K_DTR '(' e ')'     { $$ = new(DTR, $4, ENULL); }
        | '@' K_RTD '(' e ')'     { $$ = new(RTD, $4, ENULL); }
        | '@' K_RND '(' e ')'     { $$ = new(RND, $4, ENULL); }
        | '@' K_ROUND '(' e ',' e ')'
                                  { $$ = new(ROUND, $4, $6); }
        | '@' K_IF  '(' e ',' e ',' e ')'
                                  { $$ = new(IF,  $4,new(',',$6,$8)); }
        | '@' K_PV  '(' e ',' e ',' e ')'
                                  { $$ = new(PV,  $4,new(':',$6,$8)); }
        | '@' K_FV  '(' e ',' e ',' e ')'
                                  { $$ = new(FV,  $4,new(':',$6,$8)); }
        | '@' K_PMT '(' e ',' e ',' e ')'
                                  { $$ = new(PMT, $4,new(':',$6,$8)); }
        | '@' K_HOUR '(' e ')'    { $$ = new(HOUR, $4, ENULL); }
        | '@' K_MINUTE '(' e ')'  { $$ = new(MINUTE, $4, ENULL); }
        | '@' K_SECOND '(' e ')'  { $$ = new(SECOND, $4, ENULL); }
        | '@' K_MONTH '(' e ')'   { $$ = new(MONTH, $4, ENULL); }
        | '@' K_DAY '(' e ')'     { $$ = new(DAY, $4, ENULL); }
        | '@' K_YEAR '(' e ')'    { $$ = new(YEAR, $4, ENULL); }
        | '@' K_NOW               { $$ = new(NOW, ENULL, ENULL);}
        | '@' K_DTS '(' e ',' e ',' e ')'
                                  { $$ = new(DTS, $4, new(',', $6, $8));}
        | NUMBER '.' NUMBER '.' NUMBER
                                  { $$ = new(DTS, new_const(O_CONST, (double) $1),
                                         new(',', new_const(O_CONST, (double) $3),
                                         new_const(O_CONST, (double) $5)));}
        | '@' K_TTS '(' e ',' e ',' e ')'
                                  { $$ = new(TTS, $4, new(',', $6, $8));}
        | '@' K_STON '(' e ')'    { $$ = new(STON, $4, ENULL); }
        | '@' K_SLEN '(' e ')'    { $$ = new(SLEN, $4, ENULL); }
        | '@' K_EQS '(' e ',' e ')'
                                  { $$ = new(EQS, $4, $6); }
        | '@' K_DATE '(' e ')'    { $$ = new(DATE, $4, ENULL); }
        | '@' K_DATE '(' e ',' e ')'
                                  { $$ = new(DATE, $4, $6); }
        | '@' K_FMT  '(' e ',' e ')'
                                  { $$ = new(FMT, $4, $6); }
        | '@' K_UPPER '(' e ')'   { $$ = new(UPPER, $4, ENULL); }
        | '@' K_LOWER '(' e ')'   { $$ = new(LOWER, $4, ENULL); }
        | '@' K_CAPITAL '(' e ')' { $$ = new(CAPITAL, $4, ENULL); }
        | '@' K_INDEX  '(' range ',' e ')'
                                  { $$ = new(INDEX, new_range(REDUCE | INDEX, $4), $6); }
        | '@' K_INDEX  '(' e ',' range ')'
                                  { $$ = new(INDEX, new_range(REDUCE | INDEX, $6), $4); }
        | '@' K_INDEX  '(' range ',' e ',' e ')'
                                  { $$ = new(INDEX, new_range(REDUCE | INDEX, $4), new(',', $6, $8)); }
        | '@' K_LOOKUP  '(' range ',' e ')'
                                  { $$ = new(LOOKUP, new_range(REDUCE | LOOKUP, $4), $6); }
        | '@' K_LOOKUP  '(' e ',' range ')'
                                  { $$ = new(LOOKUP, new_range(REDUCE | LOOKUP, $6), $4); }
        | '@' K_HLOOKUP  '(' range ',' e ',' e ')'
                                  { $$ = new(HLOOKUP, new_range(REDUCE | HLOOKUP, $4), new(',', $6, $8)); }
        | '@' K_HLOOKUP  '(' e ',' range ',' e ')'
                                  { $$ = new(HLOOKUP, new_range(REDUCE | HLOOKUP, $6), new(',', $4, $8)); }
        | '@' K_VLOOKUP  '(' range ',' e ',' e ')'
                                  { $$ = new(VLOOKUP, new_range(REDUCE | VLOOKUP, $4), new(',', $6, $8)); }
        | '@' K_VLOOKUP  '(' e ',' range ',' e ')'
                                  { $$ = new(VLOOKUP, new_range(REDUCE | VLOOKUP, $6), new(',', $4, $8)); }
        | '@' K_STINDEX  '(' range ',' e ')'
                                  { $$ = new(STINDEX, new_range(REDUCE | STINDEX, $4), $6); }
        | '@' K_STINDEX  '(' e ',' range ')'
                                  { $$ = new(STINDEX, new_range(REDUCE | STINDEX, $6), $4); }
        | '@' K_STINDEX  '(' range ',' e ',' e ')'
                                  { $$ = new(STINDEX, new_range(REDUCE | STINDEX, $4), new(',', $6, $8)); }
        | '@' K_EXT  '(' e ',' e ')'
                                  { $$ = new(EXT, $4, $6); }
        | '@' K_LUA  '(' e ',' e ')'
                                  {
                                  #ifdef XLUA
                                  $$ = new(LUA, $4, $6);
                                  #endif
                                  }
        | '@' K_MYROW             { $$ = new(MYROW, ENULL, ENULL);}
        | '@' K_MYCOL             { $$ = new(MYCOL, ENULL, ENULL);}
        | '@' K_LASTROW           { $$ = new(LASTROW, ENULL, ENULL);}
        | '@' K_LASTCOL           { $$ = new(LASTCOL, ENULL, ENULL);}
        | '@' K_NVAL '(' e ',' e ')'
                                  { $$ = new(NVAL, $4, $6); }
        | '@' K_SVAL '(' e ',' e ')'
                                  { $$ = new(SVAL, $4, $6); }
        | '@' K_REPLACE '(' e ',' e ',' e ')'
                                  { $$ = new(REPLACE, $4, new(',', $6, $8)); }

        | '@' K_EVALUATE '(' e ')'  { $$ = new(EVALUATE, $4, ENULL); }
        | '@' K_SEVALUATE '(' e ')' { $$ = new(SEVALUATE, $4, ENULL); }
        | '@' K_SUBSTR '(' e ',' e ',' e ')'
                                  { $$ = new(SUBSTR, $4, new(',', $6, $8)); }
        |       '(' e ')'         { $$ = $2; }
        |       '+' term          { $$ = $2; }
    //    |       '-' term          { $$ = new('m', $2, ENULL); }
        |       NUMBER            { $$ = new_const(O_CONST, (double) $1); }
        |       FNUMBER           { $$ = new_const(O_CONST, $1); }
        | '@'   K_PI              { $$ = new(PI_, ENULL, ENULL); }
        |       STRING            { $$ = new_str($1); }
        |       '~' term          { $$ = new('!', $2, ENULL); }
        |       '!' term          { $$ = new('!', $2, ENULL); }
        | '@' K_FILENAME '(' e ')'
                                  { $$ = new(FILENAME, $4, ENULL); }
        | '@' K_COLTOA '(' e ')'  { $$ = new(COLTOA, $4, ENULL);}
        | '@' K_ASCII '(' e ')'   { $$ = new(ASCII, $4, ENULL); }
        | '@' K_SET8BIT '(' e ')' { $$ = new(SET8BIT, $4, ENULL); }
        | '@' K_CHR '(' e ')'     { $$ = new(CHR, $4, ENULL);}
        | '@' K_ERR               { $$ = new(ERR_, ENULL, ENULL); }
        |     K_ERR               { $$ = new(ERR_, ENULL, ENULL); }
        | '@' K_REF               { $$ = new(REF_, ENULL, ENULL); }
        |     K_REF               { $$ = new(REF_, ENULL, ENULL); }
        | '@' K_FACT '(' e ')'    { $$ = new(FACT, $4, ENULL); }
        ;

/* expressions */
e:       e '+' e                  { $$ = new('+', $1, $3); }
    |    e '-' e                  { $$ = new('-', $1, $3); }
    |    e '*' e                  { $$ = new('*', $1, $3); }
    |    e '/' e                  { $$ = new('/', $1, $3); }
    |    e '%' e                  { $$ = new('%', $1, $3); }
    |      '-' e                  { $$ = new('m', $2, ENULL); }
    |    e '^' e                  { $$ = new('^', $1, $3); }
    |    term
    |    e '?' e ':' e            { $$ = new('?', $1, new(':', $3, $5)); }
    |    e ';' e                  { $$ = new(';', $1, $3); }
    |    e '<' e                  { $$ = new('<', $1, $3); }
    |    e '=' e                  { $$ = new('=', $1, $3); }
    |    e '>' e                  { $$ = new('>', $1, $3); }
    |    e '&' e                  { $$ = new('&', $1, $3); }
    |    e '|' e                  { $$ = new('|', $1, $3); }
    |    e '<' '=' e              { $$ = new('!', new('>', $1, $4), ENULL); }
    /* |    e '!' '=' e              { $$ = new('!', new('=', $1, $4), ENULL); } */
    |    e '<' '>' e              { $$ = new('!', new('=', $1, $4), ENULL); }
    |    e '>' '=' e              { $$ = new('!', new('<', $1, $4), ENULL); }
    |    e '#' e                  { $$ = new('#', $1, $3); }
    ;

expr_list:     e                  { $$ = new(ELIST, ENULL, $1); }
    |    expr_list ',' e          { $$ = new(ELIST, $1, $3); }
    ;

range:   var ':' var              {
                                    $$.left = $1;
                                    $$.right = $3;
                                  }
    |    RANGE                    { $$ = $1; }
    ;

var:

          COL NUMBER               {
                                    struct roman * roman = session->cur_doc;
                                    $$.vp = lookat(roman->cur_sh, $2, $1);
                                    $$.vf = 0;
                                  }
    |    '$' COL NUMBER           {
                                    struct roman * roman = session->cur_doc;
                                    $$.vp = lookat(roman->cur_sh, $3, $2);
                                    $$.vf = FIX_COL;
                                  }
    |    COL '$' NUMBER           {
                                    struct roman * roman = session->cur_doc;
                                    $$.vp = lookat(roman->cur_sh, $3, $1);
                                    $$.vf = FIX_ROW;
                                  }
    |    '$' COL '$' NUMBER       {
                                    struct roman * roman = session->cur_doc;
                                    $$.vp = lookat(roman->cur_sh, $4, $2);
                                    $$.vf = FIX_ROW | FIX_COL;
                                  }


    | '@' K_GETENT '(' e ',' e ')' {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh = roman->cur_sh;
                                    $$.vp = lookat(sh, eval(sh, NULL, $4, 0), eval(sh, NULL, $6, 0));
                                    $$.vf = GET_ENT;
                                    if ($$.expr != NULL) efree($$.expr);
                                    $$.expr = new(GETENT, $4, $6);
                                  }

    |    VAR                      {
                                    $$ = $1.left;
                                  }

    ;

var_or_range:   range             { $$ = $1; }
    |    var                      { $$.left = $1; $$.right = $1; }
    ;

num:     NUMBER                   { $$ = (double) $1; }
    |    FNUMBER                  { $$ = $1; }
    |    '-' num                  { $$ = -$2; }
    |    '+' num                  { $$ = $2; }
    ;

strarg:  STRING                   { $$ = $1; }
    |    var                      {
                                    char *s, *s1;
                                    s1 = $1.vp->label;
                                    if (!s1)
                                    s1 = "NULL_STRING";
                                    s = scxmalloc((unsigned)strlen(s1)+1);
                                    (void) strcpy(s, s1);
                                    $$ = s;
                                  }
    ;

/* allows >=1 'setitem's to be listed in the same 'set' command */
setlist :
    |   setlist setitem
;

/* things that you can 'set' */
setitem :
    K_OVERLAP '=' NUMBER          {  if ($3 == 0) parse_str(user_conf_d, "overlap=0", TRUE);
                                     else         parse_str(user_conf_d, "overlap=1", TRUE); }
    |    K_OVERLAP                {               parse_str(user_conf_d, "overlap=1", TRUE); }
    |    K_INPUT_BAR_BOTTOM '=' NUMBER   {  if ($3 == 0) parse_str(user_conf_d, "input_bar_bottom=0", TRUE);
                                     else         parse_str(user_conf_d, "input_bar_bottom=1", TRUE);
                                                  ui_mv_bottom_bar(); }
    |    K_INPUT_BAR_BOTTOM       {               parse_str(user_conf_d, "input_bar_bottom=1", TRUE);
                                                  ui_mv_bottom_bar();
                                  }

    |    K_INPUT_EDIT_MODE '=' NUMBER   {  if ($3 == 0) parse_str(user_conf_d, "input_edit_mode=0", TRUE);
                                     else         parse_str(user_conf_d, "input_edit_mode=1", TRUE);
                                                  ui_mv_bottom_bar(); }
    |    K_INPUT_EDIT_MODE        {               parse_str(user_conf_d, "input_edit_mode=1", TRUE);
                                                  ui_mv_bottom_bar();
                                  }

    |    K_UNDERLINE_GRID '=' NUMBER {            if ($3 == 0) parse_str(user_conf_d, "underline_grid=0", TRUE);
                                                  else parse_str(user_conf_d, "underline_grid=1", TRUE); }
    |    K_UNDERLINE_GRID         {               parse_str(user_conf_d, "underline_grid=1", TRUE);
                                  }

    |    K_NOOVERLAP              {               parse_str(user_conf_d, "overlap=0", TRUE); }

    |    K_TRUNCATE '=' NUMBER    {  if ($3 == 0) parse_str(user_conf_d, "truncate=0", TRUE);
                                     else         parse_str(user_conf_d, "truncate=1", TRUE); }
    |    K_TRUNCATE               {               parse_str(user_conf_d, "truncate=1", TRUE); }
    |    K_NOTRUNCATE             {               parse_str(user_conf_d, "truncate=0", TRUE); }

    |    K_AUTOWRAP '=' NUMBER    {  if ($3 == 0) parse_str(user_conf_d, "autowrap=0", TRUE);
                                     else         parse_str(user_conf_d, "autowrap=1", TRUE); }
    |    K_AUTOWRAP               {               parse_str(user_conf_d, "autowrap=1", TRUE); }
    |    K_NOAUTOWRAP             {               parse_str(user_conf_d, "autowrap=0", TRUE); }

    |    K_AUTOBACKUP             {               parse_str(user_conf_d, "autobackup=1", TRUE); }
    |    K_AUTOBACKUP '=' NUMBER  {
                                                  char cmd[MAXCMD];
                                                  sprintf(cmd, "autobackup=%d", $3);
                                                  parse_str(user_conf_d, cmd, TRUE); }
    |    K_NOAUTOBACKUP           {               parse_str(user_conf_d, "autobackup=0", TRUE); }
    |    K_AUTOCALC               {               parse_str(user_conf_d, "autocalc=1", TRUE); }
    |    K_AUTOCALC '=' NUMBER    {  if ($3 == 0) parse_str(user_conf_d, "autocalc=0", TRUE);
                                     else         parse_str(user_conf_d, "autocalc=1", TRUE); }
    |    K_NOAUTOCALC             {               parse_str(user_conf_d, "autocalc=0", TRUE); }
    |    K_DEBUG                  {               parse_str(user_conf_d, "debug=1", TRUE); }
    |    K_DEBUG '=' NUMBER       {  if ($3 == 0) parse_str(user_conf_d, "debug=0", TRUE);
                                     else         parse_str(user_conf_d, "debug=1", TRUE); }
    |    K_NODEBUG                {               parse_str(user_conf_d, "debug=0", TRUE); }
    |    K_TRG                    {               parse_str(user_conf_d, "trigger=1", TRUE); }
    |    K_TRG '=' NUMBER         {  if ($3 == 0) parse_str(user_conf_d, "trigger=0", TRUE);
                                     else         parse_str(user_conf_d, "trigger=1", TRUE); }
    |    K_NOTRG                  {               parse_str(user_conf_d, "trigger=0", TRUE); }
    |    K_EXTERNAL_FUNCTIONS     {               parse_str(user_conf_d, "external_functions=1", TRUE); }
    |    K_EXTERNAL_FUNCTIONS '=' NUMBER {
                                     if ($3 == 0) parse_str(user_conf_d, "external_functions=0", TRUE);
                                     else         parse_str(user_conf_d, "external_functions=1", TRUE); }
    |    K_NOEXTERNAL_FUNCTIONS   {               parse_str(user_conf_d, "external_functions=0", TRUE); }
    |    K_EXEC_LUA               {               parse_str(user_conf_d, "exec_lua=1", TRUE); }
    |    K_EXEC_LUA '=' NUMBER    {
                                     if ($3 == 0) parse_str(user_conf_d, "exec_lua=0", TRUE);
                                     else         parse_str(user_conf_d, "exec_lua=1", TRUE); }
    |    K_NOEXEC_LUA             {               parse_str(user_conf_d, "exec_lua=0", TRUE); }
    |    K_HALF_PAGE_SCROLL       {               parse_str(user_conf_d, "half_page_scroll=1", TRUE); }
    |    K_HALF_PAGE_SCROLL '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "half_page_scroll=0", TRUE);
                                     else         parse_str(user_conf_d, "half_page_scroll=1", TRUE); }
    |    K_NOHALF_PAGE_SCROLL     {               parse_str(user_conf_d, "half_page_scroll=0", TRUE); }

    |    K_IGNORE_HIDDEN          {               parse_str(user_conf_d, "ignore_hidden=1", TRUE); }
    |    K_IGNORE_HIDDEN    '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "ignore_hidden=0", TRUE);
                                     else         parse_str(user_conf_d, "ignore_hidden=1", TRUE); }
    |    K_NOIGNORE_HIDDEN        {               parse_str(user_conf_d, "ignore_hidden=0", TRUE); }

    |    K_QUIET '=' NUMBER       {
                                     if ($3 == 0) parse_str(user_conf_d, "quiet=0", TRUE);
                                     else         parse_str(user_conf_d, "quiet=1", TRUE); }
    |    K_QUIET                  {               parse_str(user_conf_d, "quiet=1", TRUE); }
    |    K_NOQUIET                {               parse_str(user_conf_d, "quiet=0", TRUE); }
    |    K_QUIT_AFTERLOAD         {               parse_str(user_conf_d, "quit_afterload=1", TRUE); }
    |    K_QUIT_AFTERLOAD '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "quit_afterload=0", TRUE);
                                     else         parse_str(user_conf_d, "quit_afterload=1", TRUE); }
    |    K_NOQUIT_AFTERLOAD       {               parse_str(user_conf_d, "quit_afterload=0", TRUE); }
    |    K_XLSX_READFORMULAS      {               parse_str(user_conf_d, "xlsx_readformulas=1", TRUE); }
    |    K_XLSX_READFORMULAS '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "xlsx_readformulas=0", TRUE);
                                     else         parse_str(user_conf_d, "xlsx_readformulas=1", TRUE); }
    |    K_NOXLSX_READFORMULAS    {               parse_str(user_conf_d, "xlsx_readformulas=0", TRUE); }
    |    K_NOCURSES               {  if (! session->cur_doc->loading) parse_str(user_conf_d, "nocurses=1", TRUE); }
    |    K_NOCURSES '=' NUMBER    {  if (! session->cur_doc->loading) {
                                         if ($3 == 0) parse_str(user_conf_d, "nocurses=0", TRUE);
                                         else         parse_str(user_conf_d, "nocurses=1", TRUE); }
                                     }
    |    K_CURSES                 {  if (! session->cur_doc->loading) parse_str(user_conf_d, "nocurses=0", TRUE); }
    |    K_NUMERIC                {               parse_str(user_conf_d, "numeric=1", TRUE); }
    |    K_NUMERIC '=' NUMBER     {  if ($3 == 0) parse_str(user_conf_d, "numeric=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric=1", TRUE); }
    |    K_NONUMERIC              {               parse_str(user_conf_d, "numeric=0", TRUE); }
    |    K_IGNORECASE             {               parse_str(user_conf_d, "ignorecase=1", TRUE); }
    |    K_IGNORECASE '=' NUMBER  {  if ($3 == 0) parse_str(user_conf_d, "ignorecase=0", TRUE);
                                     else         parse_str(user_conf_d, "ignorecase=1", TRUE); }
    |    K_NOIGNORECASE           {               parse_str(user_conf_d, "ignorecase=0", TRUE); }
    |    K_NUMERIC_DECIMAL        {               parse_str(user_conf_d, "numeric_decimal=1", TRUE); }
    |    K_NUMERIC_DECIMAL '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "numeric_decimal=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric_decimal=1", TRUE); }
    |    K_NONUMERIC_DECIMAL      {               parse_str(user_conf_d, "numeric_decimal=0", TRUE); }
    |    K_NUMERIC_ZERO           {               parse_str(user_conf_d, "numeric_zero=1", TRUE); }
    |    K_NUMERIC_ZERO '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "numeric_zero=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric_zero=1", TRUE); }
    |    K_NONUMERIC_ZERO         {               parse_str(user_conf_d, "numeric_zero=0", TRUE); }
    |    K_NEWLINE_ACTION         {               parse_str(user_conf_d, "newline_action=0", TRUE); }
    |    K_NEWLINE_ACTION '=' WORD {
                                  char * s = (char *) $3;
                                  if (s[0] =='j') parse_str(user_conf_d, "newline_action=j", TRUE);
                                  else if (s[0] =='l')
                                  parse_str(user_conf_d, "newline_action=l", TRUE);
                                  }
    |    K_DEFAULT_COPY_TO_CLIPBOARD_CMD '=' strarg {
                                  char cmd[MAXCMD];
                                  char * s = (char *) $3;
                                  sprintf(cmd, "default_copy_to_clipboard_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }
    |    K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD '=' strarg {
                                  char cmd[MAXCMD];
                                  char * s = (char *) $3;
                                  sprintf(cmd, "default_paste_from_clipboard_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }

    |    K_IMPORT_DELIMITED_TO_TEXT {      parse_str(user_conf_d, "import_delimited_to_text=1", TRUE); }

    |    K_IMPORT_DELIMITED_TO_TEXT '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "import_delimited_to_text=0", TRUE);
                                     else         parse_str(user_conf_d, "import_delimited_to_text=1", TRUE); }

    |    K_COPY_TO_CLIPBOARD_DELIMITED_TAB {      parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=1", TRUE); }

    |    K_COPY_TO_CLIPBOARD_DELIMITED_TAB '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=0", TRUE);
                                     else         parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=1", TRUE); }
    |    K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB {    parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=0", TRUE); }
    |    K_COPY_TO_CLIPBOARD_WYSIWYG {            parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=1", TRUE); }
    |    K_COPY_TO_CLIPBOARD_WYSIWYG '=' NUMBER
                                  {  if ($3 == 0) parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=0", TRUE);
                                     else         parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=1", TRUE); }
    |    K_NOCOPY_TO_CLIPBOARD_WYSIWYG {          parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=0", TRUE); }
    |    K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD '=' strarg {
                                  char cmd[MAXCMD];
                                  char * s = (char *) $3;
                                  sprintf(cmd, "default_open_file_under_cursor_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }

    |    K_NEWLINE_ACTION '=' NUMBER {
                                     if ($3 == 0) parse_str(user_conf_d, "newline_action=0", TRUE); }
    |    K_COMMAND_TIMEOUT        {               parse_str(user_conf_d, "command_timeout=3000", TRUE); }
    |    K_COMMAND_TIMEOUT '=' num   {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "command_timeout=%d", (int) $3);
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                     }
    |    K_MAPPING_TIMEOUT        {               parse_str(user_conf_d, "mapping_timeout=1500", TRUE); }
    |    K_MAPPING_TIMEOUT '=' num   {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "mapping_timeout=%d", (int) $3);
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                     }
    |    K_TM_GMTOFF              {               parse_str(user_conf_d, "tm_gmtoff=-10800", TRUE); }
    |    K_TM_GMTOFF '=' num      {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "tm_gmtoff=%d", (int) $3);
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                  }
    |    K_SHOW_CURSOR '=' NUMBER {  if ($3 == 0) parse_str(user_conf_d, "show_cursor=0", TRUE);
                                     else         parse_str(user_conf_d, "show_cursor=1", TRUE); }
    |    K_SHOW_CURSOR            {               parse_str(user_conf_d, "show_cursor=1", TRUE); }
    |    K_NOSHOW_CURSOR          {               parse_str(user_conf_d, "show_cursor=0", TRUE); }

    ;
