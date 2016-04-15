#include <sys/types.h>
#include <string.h>

#ifdef IEEE_MATH
 #include <ieeefp.h>
#endif /* IEEE_MATH */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "lex.h"
#include "sc.h"
#include "conf.h"

typedef int bool;
enum { false, true };

#include "y.tab.h"

jmp_buf wakeup;
jmp_buf fpe_buf;

bool decimal = FALSE;

void fpe_trap(int signo) {
#if defined(i386)
    asm("    fnclex");
    asm("    fwait");
#else
 #ifdef IEEE_MATH
    (void)fpsetsticky((fp_except)0);    /* Clear exception */
 #endif /* IEEE_MATH */
#endif
    longjmp(fpe_buf, 1);
}

struct key {
    char * key;
    int val;
};

struct key experres[] = {
#include "experres.h"
    { 0, 0 }
};

struct key statres[] = {
#include "statres.h"
    { 0, 0 }
};

#include "macros.h"
#include "screen.h"
#include "range.h"
#include "color.h" // for set_ucolor

int yylex() {
    char * p = line + linelim;
    int ret = 0;
    static int isfunc = 0;
    static bool isgoto = 0;
    static bool colstate = 0;
    static int dateflag;
    static char *tokenst = NULL;
    static int tokenl;

    while (isspace(*p)) p++;
    if (*p == '\0') {
        isfunc = isgoto = 0;
        ret = -1;
    } else if (isalpha(*p) || (*p == '_')) {
        register char *la;    /* lookahead pointer */
        register struct key *tblp;

        if ( !tokenst ) {
            tokenst = p;
            tokenl = 0;
        }
        /*
         *  This picks up either 1 or 2 alpha characters (a column) or
         *  tokens made up of alphanumeric chars and '_' (a function or
         *  token or command or a range name)
         */
        while (isalpha(*p) && isascii(*p)) {
            p++;
            tokenl++;
        }
        la = p;
        while (isdigit(*la) || (*la == '$'))
            la++;
        /*
         * A COL is 1 or 2 char alpha with nothing but digits following
         * (no alpha or '_')
         */
        if (!isdigit(*tokenst) && tokenl && tokenl <= 2 && (colstate ||
            (isdigit(*(la-1)) && !(isalpha(*la) || (*la == '_'))))) {
            ret = COL;
            yylval.ival = atocol(tokenst, tokenl);
        } else {
            while (isalpha(*p) || (*p == '_') || isdigit(*p)) {
                p++;
                tokenl++;
            }
            ret = WORD;
            if (!linelim || isfunc) {
                if (isfunc) isfunc--;
                for (tblp = linelim ? experres : statres; tblp->key; tblp++)
                    if (((tblp->key[0]^tokenst[0])&0137)==0
                        && tblp->key[tokenl]==0) {
                    int i = 1;
                    while (i<tokenl && ((tokenst[i]^tblp->key[i])&0137)==0)
                        i++;
                    if (i >= tokenl) {
                        ret = tblp->val;
                        colstate = (ret <= S_FORMAT);
                        if (isgoto) {
                            isfunc = isgoto = 0;
                            if (ret != K_ERROR && ret != K_INVALID)
                                ret = WORD;
                            }
                            break;
                        }
                    }
            }

            if (ret == WORD) {
                if (tokenst && ( ! strncmp(line, "nmap", 4) || ! strncmp(line, "imap", 4) )) {
                    char * p = tokenst;
                    char * ptr = p;
                    while (*ptr && (
                          (*ptr != ' ')
                          //|| (*ptr != '\t')
                          || (*(ptr-1) == '\\'))) ptr++;
                    ptr = malloc((unsigned) (ptr - p));
                    yylval.sval = ptr;

                    while ( *p && (
                      (*p != ' ') ||
                      (*p != '\t') ||
                      ( *(p+1)
                      && *(p-1)
                      && *(p-1) == '\\'
                      && *(p+1) != '\0'
                      && *(p+1) != '\n'
                      && *(p+1) != '\r'
                      )       ) ) *ptr++ = *p++;
                    if (*p && *p == ' ') (*ptr)--;
                    (*ptr)--;
                    if (*ptr) *ptr = '\0';

                    ret = MAPWORD;

                } else {
                    struct range * r;
                    if (!find_range(tokenst, tokenl, (struct ent *)0, (struct ent *)0, &r)) {
                        yylval.rval.left = r->r_left;
                        yylval.rval.right = r->r_right;
                        if (r->r_is_range)
                            ret = RANGE;
                        else
                            ret = VAR;
                    } else {
                        linelim = p-line;
                        yyerror("Unintelligible word");
                    }
                }
            }
        } // 117
    } else if ((*p == '.') || isdigit(*p)) { // 89
        void (*sig_save)();
        double v = 0.0;
        int temp;
        char *nstart = p;

        sig_save = signal(SIGFPE, fpe_trap);
        if (setjmp(fpe_buf)) {
            (void) signal(SIGFPE, sig_save);
            yylval.fval = v;
            sc_error("Floating point exception\n");
            isfunc = isgoto = 0;
            tokenst = NULL;
            return FNUMBER;
        }

        if (*p=='.' && dateflag) {  /* .'s in dates are returned as tokens. */
            ret = *p++;
            dateflag--;
        } else {
            if (*p != '.') {
                tokenst = p;
                tokenl = 0;
                do {
                    v = v*10.0 + (double) ((unsigned) *p - '0');
                    tokenl++;
                } while (isdigit(*++p));
                if (dateflag) {
                    ret = NUMBER;
                    yylval.ival = (int)v;
                /*
                 *  If a string of digits is followed by two .'s separated by
                 *  one or two digits, assume this is a date and return the
                 *  .'s as tokens instead of interpreting them as decimal
                 *  points.  dateflag counts the .'s as they're returned.
                 */
                } else if (*p=='.' && isdigit(*(p+1)) && (*(p+2)=='.' ||
                    (isdigit(*(p+2)) && *(p+3)=='.'))) {
                    ret = NUMBER;
                    yylval.ival = (int)v;
                    dateflag = 2;
                } else if (*p == 'e' || *p == 'E') {
                    while (isdigit(*++p)) /* */;
                        if (isalpha(*p) || *p == '_') {
                            linelim = p - line;
                            return (yylex());
                        } else
                            ret = FNUMBER;
                } else if (isalpha(*p) || *p == '_') {
                    linelim = p - line;
                    return (yylex());
                }
            }
            if ((!dateflag && *p=='.') || ret == FNUMBER) {
                ret = FNUMBER;
                yylval.fval = strtod(nstart, &p);
                if (!finite(yylval.fval))
                    ret = K_ERR;
                else
                    decimal = TRUE;
            } else {
                /* A NUMBER must hold at least MAXROW and MAXCOL */
                /* This is consistent with a short row and col in struct ent */
                //if (v > (double)32767 || v < (double)-32768) {
                //if (v > (double) INT_MAX || v < (double) INT_MIN) {
                if (v > (double) MAXROWS || v < (double) -MAXROWS) {
                    ret = FNUMBER;
                    yylval.fval = v;
                } else {
                    temp = (int)v;
                    if((double)temp != v) {
                        ret = FNUMBER;
                        yylval.fval = v;
                    } else {
                        ret = NUMBER;
                        yylval.ival = temp;
                    }
                }
            }
        }
        (void) signal(SIGFPE, sig_save);
    } else if (*p=='"') {
        char *ptr;
        ptr = p+1;    /* "string" or "string\"quoted\"" */
        while (*ptr && ((*ptr != '"') || (*(ptr-1) == '\\')))
            ptr++;
        ptr = scxmalloc((unsigned)(ptr-p));
        yylval.sval = ptr;
        p++;
        while (*p && ((*p != '"') || (*(p-1) == '\\' && *(p+1) != '\0' && *(p+1) != '\n')))
            *ptr++ = *p++;
        *ptr = '\0';
        if (*p)
            p++;
        ret = STRING;
 
    } else if (*p=='[') {
        while (*p && *p!=']')
            p++;
        if (*p)
            p++;
        linelim = p-line;
        tokenst = NULL;
        return yylex();

/*
    } else if (tokenl ==  3) {
                    //k++;
            //mvprintw(0, 5, "$$%d", tokenl);
                    //k++;
            //mvprintw(0, 25, "$$%s", tokenst);
            //ret = STRING;
            //yylval.sval = tokenst;

        yylval.sval = "HOLA";
        ret = MAPWORD;
        ret = STRING;
*/
    } else {
        ret = *p++;
    }
    linelim = p-line;
    if (!isfunc) isfunc = ((ret == '@') + (ret == S_GOTO) - (ret == S_SET));
    if (ret == S_GOTO) isgoto = TRUE;
    tokenst = NULL;
    return ret;
}

/*
* This is a very simpleminded test for plugins:  does the file merely exist
* in the plugin directories.  Perhaps should test for it being executable
*/
int plugin_exists(char *name, int len, char *path) {
    FILE *fp;
    static char *HomeDir;

    if ((HomeDir = getenv("HOME"))) {
        strcpy((char *)path, HomeDir);
        strcat((char *)path, "/.sc/plugins/");
        strncat((char *)path, name, len);
        if ((fp = fopen((char *)path, "r"))) {
            fclose(fp);
            return 1;
        }
    }
    strcpy((char *)path, LIBDIR);
    strcat((char *)path, "/plugins/");
    strncat((char *)path, name, len);
    if ((fp = fopen((char *)path, "r"))) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/*
 * Given a token string starting with a symbolic column name and its valid
 * length, convert column name ("A"-"Z" or "AA"-"ZZ") to a column number (0-N).
 * Never mind if the column number is illegal (too high).  The procedure's name
 * and function are the inverse of coltoa().
 * 
 * Case-insensitivity is done crudely, by ignoring the 040 bit.
 */

int atocol(char *string, int len) {
    register int col;

    col = (toupper(string[0])) - 'A';

    if (len == 2)        /* has second char */
        col = ((col + 1) * 26) + ((toupper(string[1])) - 'A');

    return (col);
}
