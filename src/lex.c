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
#include "sc.h"

#ifdef NONOTIMEOUT
#define	notimeout(a1, a2)
#endif

typedef int bool;
enum { false, true };

#include "y.tab.h"

#ifdef hpux
extern YYSTYPE yylval;
#endif /* hpux */

jmp_buf wakeup;
jmp_buf fpe_buf;

bool decimal = FALSE;

#ifdef SIGVOID
void
#endif
fpe_trap(int signo)
{
#if defined(i386) && !defined(M_XENIX)
    asm("	fnclex");
    asm("	fwait");
#else
#ifdef IEEE_MATH
    (void)fpsetsticky((fp_except)0);	/* Clear exception */
#endif /* IEEE_MATH */
#ifdef PC
    _fpreset();
#endif
#endif
    longjmp(fpe_buf, 1);
}

struct key {
    char *key;
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

static k = 10;

int yylex() {
    char *p = line + linelim;
    int ret=0;
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
	register char *la;	/* lookahead pointer */
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
/*
		struct range *r;
		if (!find_range(tokenst, tokenl, (struct ent *)0, (struct ent *)0, &r)) {
		    yylval.rval.left = r->r_left;
		    yylval.rval.right = r->r_right;
		    if (r->r_is_range)
		        ret = RANGE;
		    else
			ret = VAR;
		} else
		char *path;
                if ((path = scxmalloc((unsigned)PATHLEN)) && plugin_exists(tokenst, tokenl, path)) {
		    strcat(path, p);
		    yylval.sval = path;
		    ret = PLUGIN;
*/
                // NMAP
		if ( tokenst && (strcmp(line, "nmap") > 0 || strcmp(line, "imap") > 0) ) {

                    char * p = tokenst;
                    char * ptr = p;
                    while (*ptr && (
                          (*ptr != ' ')
                          //|| (*ptr != '\t')
                          || (*(ptr-1) == '\\'))) ptr++;
                    ptr = malloc((unsigned) (ptr - p));
	            yylval.sval = ptr;
	            //p++; // quitado por and
       
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
                    if (*p && *p == ' ') *ptr--; // agregado por and
	            *ptr--; // agregado por and
	            if (*ptr) *ptr = '\0';

	            //if (*p) p++;
	            ret = MAPWORD;

/*   
        char * p = tokenst;
        char *ptr;
        ptr = p;
        while (*ptr && ((*ptr != ' ') || (*(ptr-1) == '\\')))
	    ptr++;
        ptr = scxmalloc((unsigned) (ptr-p));
	yylval.sval = ptr;
	//p++; // quitado por and
        
	while ( *p && ((*p != ' ') ||
              ( *(p+1) 
              && *(p-1) == '\\' 
              && *(p+1) != '\0'
              && *(p+1) != '\n'
              )       ) ) *ptr++ = *p++;
        if (*p && *p == ' ') *ptr--; // agregado por and
	*ptr--; // agregado por and
	if (*ptr) *ptr = '\0';
	if (*p) p++;
	ret = MAPWORD;

 */               
                } else {
		    linelim = p-line;
		    yyerror("Unintelligible word");
		}

	    }
	}
    } else if ((*p == '.') || isdigit(*p)) {
#ifdef SIGVOID
	void (*sig_save)();
#else
	int (*sig_save)();
#endif
	double v = 0.0;
	int temp;
	char *nstart = p;

	sig_save = signal(SIGFPE, fpe_trap);
	if (setjmp(fpe_buf)) {
	    (void) signal(SIGFPE, sig_save);
	    yylval.fval = v;
	    error("Floating point exception\n");
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
		if (v > (double)32767 || v < (double)-32768) {
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
        ptr = p+1;	/* "string" or "string\"quoted\"" */
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

    if (len == 2)		/* has second char */
	col = ((col + 1) * 26) + ((toupper(string[1])) - 'A');

    return (col);
}


#ifdef SIMPLE

void initkbd() {}

void kbd_again() {}

void resetkbd() {}

#ifndef VMS

int nmgetch() {
    return (getchar());
}

#else /* VMS */

int nmgetch()
/*
   This is not perfect, it doesn't move the cursor when goraw changes
   over to deraw, but it works well enough since the whole sc package
   is incredibly stable (loop constantly positions cursor).

   Question, why didn't the VMS people just implement cbreak?

   NOTE: During testing it was discovered that the DEBUGGER and curses
   and this method of reading would collide (the screen was not updated
   when continuing from screen mode in the debugger).
*/
{
    short c;
    static int key_id=0;
    int status;
#define VMScheck(a) {if (~(status = (a)) & 1) VMS_MSG (status);}

    if (VMS_read_raw) {
      VMScheck(smg$read_keystroke (&stdkb->_id, &c, 0, 0, 0));
    } else
       c = getchar();

    switch (c) {
	case SMG$K_TRM_LEFT:  c = KEY_LEFT;  break;
	case SMG$K_TRM_RIGHT: c = KEY_RIGHT; break;
	case SMG$K_TRM_UP:    c = ctl('p');  break;
	case SMG$K_TRM_DOWN:  c = ctl('n');  break;
	default:   c = c & A_CHARTEXT;
    }
    return (c);
}

VMS_MSG (int status)
/*
   Routine to put out the VMS operating system error (if one occurs).
*/
{
#include <descrip.h>
   char errstr[81], buf[120];
   $DESCRIPTOR(errdesc, errstr);
   short length;
#define err_out(msg) fprintf (stderr,msg)

/* Check for no error or standard error */

    if (~status & 1) {
	status = status & 0x8000 ? status & 0xFFFFFFF : status & 0xFFFF;
	if (SYS$GETMSG(status, &length, &errdesc, 1, 0) == SS$_NORMAL) {
	    errstr[length] = '\0';
	    (void) sprintf(buf, "<0x%x> %s", status, errdesc.dsc$a_pointer);
	    err_out(buf);
	} else
	    err_out("System error");
    }
}
#endif /* VMS */

#else /*SIMPLE*/

#if defined(BSD42) || defined (SYSIII) || defined(BSD43)

#define N_KEY 4

struct key_map {
    char *k_str;
    int k_val;
    char k_index;
}; 

struct key_map km[N_KEY];

char keyarea[N_KEY*30];

char *tgetstr();
char *getenv();
char *ks;
char ks_buf[20];
char *ke;
char ke_buf[20];

#ifdef TIOCSLTC
struct ltchars old_chars, new_chars;
#endif

char dont_use[] = {
    ctl('['), ctl('a'), ctl('b'), ctl('c'), ctl('e'), ctl('f'), ctl('g'),
    ctl('h'), ctl('i'), ctl('j'),  ctl('l'), ctl('m'), ctl('n'), ctl('p'),
    ctl('q'), ctl('r'), ctl('s'), ctl('t'), ctl('u'), ctl('v'),  ctl('w'),
    ctl('x'), ctl('z'), 0
};

void charout(int c) {
    (void)putchar(c);
}

void initkbd() {
    register struct key_map *kp;
    register i,j;
    char *p = keyarea;
    char *ktmp;
    static char buf[1024]; /* Why do I have to do this again? */

    if (!(ktmp = getenv("TERM"))) {
	(void) fprintf(stderr, "TERM environment variable not set\n");
	exit (1);
    }
    if (tgetent(buf, ktmp) <= 0)
	return;

    km[0].k_str = tgetstr("kl", &p); km[0].k_val = KEY_LEFT;
    km[1].k_str = tgetstr("kr", &p); km[1].k_val = KEY_RIGHT;
    km[2].k_str = tgetstr("ku", &p); km[2].k_val = ctl('p');
    km[3].k_str = tgetstr("kd", &p); km[3].k_val = ctl('n');

    ktmp = tgetstr("ks",&p);
    if (ktmp)  {
	(void) strcpy(ks_buf, ktmp);
	ks = ks_buf;
	tputs(ks, 1, charout);
    }
    ktmp = tgetstr("ke",&p);
    if (ktmp)  {
	(void) strcpy(ke_buf, ktmp);
	ke = ke_buf;
    }

    /* Unmap arrow keys which conflict with our ctl keys   */
    /* Ignore unset, longer than length 1, and 1-1 mapped keys */

    for (i = 0; i < N_KEY; i++) {
	kp = &km[i];
	if (kp->k_str && (kp->k_str[1] == 0) && (kp->k_str[0] != kp->k_val))
	    for (j = 0; dont_use[j] != 0; j++)
	        if (kp->k_str[0] == dont_use[j]) {
		     kp->k_str = (char *)0;
		     break;
		}
    }

#ifdef TIOCSLTC
    (void)ioctl(fileno(stdin), TIOCGLTC, (char *)&old_chars);
    new_chars = old_chars;
    if (old_chars.t_lnextc == ctl('v'))
	new_chars.t_lnextc = -1;
    if (old_chars.t_rprntc == ctl('r'))
	new_chars.t_rprntc = -1;
    (void)ioctl(fileno(stdin), TIOCSLTC, (char *)&new_chars);
#endif
}

void kbd_again() {
    if (ks) 
	tputs(ks, 1, charout);

#ifdef TIOCSLTC
    (void)ioctl(fileno(stdin), TIOCSLTC, (char *)&new_chars);
#endif
}

void resetkbd() {
    if (ke) 
	tputs(ke, 1, charout);

#ifdef TIOCSLTC
    (void)ioctl(fileno(stdin), TIOCSLTC, (char *)&old_chars);
#endif
}

int nmgetch() {
    register int c;
    register struct key_map *kp;
    register struct key_map *biggest;
    register int i;
    int almost;
    int maybe;

    static char dumpbuf[10];
    static char *dumpindex;

#ifdef SIGVOID
    void time_out();
#else
    int time_out();
#endif

    if (dumpindex && *dumpindex)
	return (*dumpindex++);

    c = getchar();
    biggest = 0;
    almost = 0;

    for (kp = &km[0]; kp < &km[N_KEY]; kp++) {
	if (!kp->k_str)
	    continue;
	if (c == kp->k_str[kp->k_index]) {
	    almost = 1;
	    kp->k_index++;
	    if (kp->k_str[kp->k_index] == 0) {
		c = kp->k_val;
		for (kp = &km[0]; kp < &km[N_KEY]; kp++)
		    kp->k_index = 0;
		return (c);
	    }
	}
	if (!biggest && kp->k_index)
	    biggest = kp;
        else if (kp->k_index && biggest->k_index < kp->k_index)
	    biggest = kp;
    }

    if (almost) { 
        (void) signal(SIGALRM, time_out);
        (void) alarm(1);

	if (setjmp(wakeup) == 0) { 
	    maybe = nmgetch();
	    (void) alarm(0);
	    return (maybe);
	}
    }
    
    if (biggest) {
	for (i = 0; i<biggest->k_index; i++) 
	    dumpbuf[i] = biggest->k_str[i];
	if (!almost)
	    dumpbuf[i++] = c;
	dumpbuf[i] = '\0';
	dumpindex = &dumpbuf[1];
	for (kp = &km[0]; kp < &km[N_KEY]; kp++)
	    kp->k_index = 0;
	return (dumpbuf[0]);
    }

    return(c);
}

#endif

#if defined(SYSV2) || defined(SYSV3) || defined(MSDOS)

void initkbd() {
    //keypad(stdscr, TRUE);
    //notimeout(stdscr,TRUE);
}

void kbd_again() {
    //keypad(stdscr, TRUE);
    //notimeout(stdscr,TRUE);
}

void resetkbd() {
    //keypad(stdscr, FALSE);
    //notimeout(stdscr, FALSE);
}

int nmgetch() {
    register int c;

    c = getch();
    switch (c) {
#ifdef KEY_SELECT
	case KEY_SELECT:	c = 'm';	break;
#endif
#ifdef KEY_C1
/* This stuff works for a wyse wy75 in ANSI mode under 5.3.  Good luck. */
/* It is supposed to map the curses keypad back to the numeric equiv. */

/* I had to disable this to make programmable function keys work.  I'm
 * not familiar with the wyse wy75 terminal.  Does anyone know how to
 * make this work without causing problems with programmable function
 * keys on everything else?  - CRM

	case KEY_C1:	c = '0'; break;
	case KEY_A1:	c = '1'; break;
	case KEY_B2:	c = '2'; break;
	case KEY_A3:	c = '3'; break;
	case KEY_F(5):	c = '4'; break;
	case KEY_F(6):	c = '5'; break;
	case KEY_F(7):	c = '6'; break;
	case KEY_F(9):	c = '7'; break;
	case KEY_F(10):	c = '8'; break;
	case KEY_F0:	c = '9'; break;
	case KEY_C3:	c = '.'; break;
	case KEY_ENTER:	c = ctl('m'); break;

 *
 *
 */
#endif
	default:	break;
    }
    return (c);
}

#endif /* SYSV2 || SYSV3 */

#endif /* SIMPLE */

#ifdef SIGVOID
void
#else
int
#endif
time_out(int signo) {
    longjmp(wakeup, 1);
}
