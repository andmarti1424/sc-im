#include <curses.h>
#include "maps.h"
#include "yank.h"
#include "marks.h"
#include "cmds.h"
#include "undo.h"
#include "buffer.h"
#include "stdout.h"
#include "conf.h" // for conf parameters
#include <stdlib.h>

void	syncref(register struct enode *e);
//void	unspecial(FILE *f, char *str, int delim);
//extern	char *scext;
//extern	char *ascext;
//extern	char *tbl0ext;
//extern	char *tblext;
//extern	char *latexext;
//extern	char *slatexext;
//extern	char *texext;
//extern	struct go_save gs;
//int macrofd;
//int cslop;
//int curcol;
//struct impexfilt *filt = NULL; /* root of list of impex filters */

extern unsigned int shall_quit;
char insert_edit_submode;
struct ent * freeents = NULL; // keep deleted ents around before sync_refs

// mark_ent_as_deleted (free_ents en sc original):
// This structure is used to keep ent structs around before they
// are actualy deleted (memory freed) to allow the sync_refs routine a chance to fix the
// variable references.
// it sets is_deleted flag of an ent.
void mark_ent_as_deleted(register struct ent *p) {
    if (p == NULL) return;
    //p->flags |= iscleared;
    p->flags |= is_deleted;

    p->next = freeents;     /* put this ent on the front of freeents */
    freeents = p;

    return;
}

// flush_saved: iterates throw freeents (ents marked as deleted)
// calls clearent for freeing ents contents memory
// and free ent pointer. this function should always be called
// at exit. this is mandatory, just in case we want to UNDO any changes.
void flush_saved() {
    register struct ent *p;
    register struct ent *q;

    p = freeents;
    while (p != NULL) {
        (void) clearent(p);
        q = p->next;
        free(p);
        p = q;        
    }
    freeents = NULL;
    return;
}

// sync_refs and syncref are used to REMOVE references to
// deleted struct ents.
// Note that the deleted structure must still
// be hanging around before the call, but not referenced
// by an entry in tbl.
// MEJORAR: No debiera recorrerse toda la tabla.
void sync_refs() {
    int i, j;
    register struct ent *p;
    // sync_ranges();
    for (i=0; i <= maxrow; i++)
	for (j=0; j <= maxcol; j++)
	    if ( (p = *ATBL(tbl, i, j)) && p->expr ) {
               syncref(p->expr);
               //info("%d %d %d", i, j, ++k);
            }
    return;
}

void syncref(register struct enode *e) {
    if ( e == (struct enode *) 0 ) {
    //if ( e == NULL || e->op == ERR_ ) {
	return;
    } else if (e->op & REDUCE) {
 	e->e.r.right.vp = lookat(e->e.r.right.vp->row, e->e.r.right.vp->col);
 	e->e.r.left.vp = lookat(e->e.r.left.vp->row, e->e.r.left.vp->col);
    } else {
	switch (e->op) {
	    case 'v':
		//if (e->e.v.vp->flags & iscleared) {
		if (e->e.v.vp->flags & is_deleted) {
		    e->op = ERR_;
                    //info("%d %d", e->e.v.vp->row, e->e.v.vp->col);
		    e->e.o.left = NULL;
		    e->e.o.right = NULL;
		} else if (e->e.v.vp->flags & may_sync)
		    e->e.v.vp = lookat(e->e.v.vp->row, e->e.v.vp->col);
		break;
	    case 'k':
		break;
	    case '$':
		break;
	    default:
		syncref(e->e.o.right);
		syncref(e->e.o.left);
		break;
	}
    }
    return;
}

// Delete a column
void deletecol() {
    int r, c, i;
    int cs;
    struct ent **pp;
    struct ent **qq;
    struct ent * p;
    struct ent * q;
    struct ent * obuf = NULL;
    char buf[50];
 
    if (any_locked_cells(0, curcol, maxrow, curcol)) {
	info("Locked cells encountered. Nothing changed");
	return;
    }

    // mark ent of column to erase with isdeleted flag
    for (r = 0; r <= maxrow; r++) {

        pp = ATBL(tbl, r, curcol);
        if ( *pp != NULL ) {
            mark_ent_as_deleted(*pp);
            //clearent(*pp);
            //free(*pp);
            *pp = NULL;
        }
    }

    // copio referencias de celdas de columna derecha a la de la izquierda (la que se borra)
    for (r = 0; r <= maxrow; r++) {
        for (c = curcol; c < maxcol; c++) {
            pp = ATBL(tbl, r, c);

            // nota: pp[1] = ATBL(tbl, r, c+1);
            if ( pp[1] != NULL ) pp[1]->col--;
            pp[0] = pp[1];
        }

        // libero memoria de última columna (pudiera tmb blanquear ent con "cleanent").
        //pp = ATBL(tbl, r, maxcol);
        //*pp = (struct ent *) 0;
    }

    // corrijo precision y ancho de columnas
    for (i = curcol; i < maxcols - 2; i++) {
        fwidth[i] = fwidth[i+1];
        precision[i] = precision[i+1];
        realfmt[i] = realfmt[i+1];
        //col_hidden[i] = col_hidden[i+1];
    }

    for (; i < maxcols - 1; i++) {
        fwidth[i] = DEFWIDTH;
        precision[i] = DEFPREC;
        realfmt[i] = DEFREFMT;
        //col_hidden[i] = FALSE;
    }

    maxcol--;
    sync_refs();
    //flush_saved(); // we have to flush_saved only at exit.
    //this is because we have to keep ents in case we want to UNDO
    modflg++;
    return;
}

/*
void colshow_op() {
    register int i,j;
    for (i = 0; i < maxcols; i++)
	if (col_hidden[i]) 
	    break;
    for(j = i; j < maxcols; j++)
	if (!col_hidden[j])
	    break;
    j--;
    if (i >= maxcols) {
	info("No hidden columns to show");
    } else {
	(void) sprintf(line,"show %s:", coltoa(i));
	(void) sprintf(line + strlen(line),"%s",coltoa(j));
	linelim = strlen(line);
    }
}

void rowshow_op() {
    register int i,j;
    for (i = 0; i < maxrows; i++)
	if (row_hidden[i]) 
	    break;
    for(j = i; j < maxrows; j++)
	if (!row_hidden[j]) {
	    break;
	}
    j--;

    if (i >= maxrows) {
	info("No hidden rows to show");
    } else {
	(void) sprintf(line,"show %d:%d", i, j);
        linelim = strlen(line);
    }
}
*/

/* mark a row as hidden
void hiderow(int arg) {
    register int r1;
    register int r2;

    r1 = currow;
    r2 = r1 + arg - 1;
    if (r1 < 0 || r1 > r2) {
	error("Invalid range");
	return;
    }
    if (r2 >= maxrows-1) {
    	if (!growtbl(GROWROW, arg+1, 0)) {
	    info("You can't hide the last row");
	    return;
	}
    }
    FullUpdate++;
    modflg++;
    while (r1 <= r2)
	row_hidden[r1++] = 1;
}

// mark a column as hidden
void hidecol(int arg) {
    int c1;
    int c2;

    c1 = curcol;
    c2 = c1 + arg - 1;
    if (c1 < 0 || c1 > c2) {
	error ("Invalid range");
	return;
    }
    if (c2 >= maxcols-1) {
    	if ((arg >= ABSMAXCOLS-1) || !growtbl(GROWCOL, 0, arg+1)) {
	    info("You can't hide the last col");
	    return;
	}
    }
    FullUpdate++;
    modflg++;
    while (c1 <= c2)
	col_hidden[c1++] = TRUE;
}

// mark a row as not-hidden
void showrow(int r1, int r2) {
    if (r1 < 0 || r1 > r2) {
	error ("Invalid range");
	return;
    }
    if (r2 > maxrows-1) {
	r2 = maxrows-1;
    }
    FullUpdate++;
    modflg++;
    while (r1 <= r2)
	row_hidden[r1++] = 0;
}

// mark a column as not-hidden
void showcol(int c1, int c2) {
    if (c1 < 0 || c1 > c2) {
	error ("Invalid range");
	return;
    }
    if (c2 > maxcols-1) {
	c2 = maxcols-1;
    }
    FullUpdate++;
    modflg++;
    while (c1 <= c2)
	col_hidden[c1++] = FALSE;
}
*/

// Copy a cell (struct ent).  "special" indicates special treatment when
// merging two cells for the "pm" command, merging formats only for the
// "pf" command, or for adjusting cell references when transposing with
// the "pt" command.  r1, c1, r2, and c2 define the range in which the dr
// and dc values should be used.
void copyent(register struct ent *n, register struct ent *p, int dr, int dc,
             int r1, int c1, int r2, int c2, int special) {
    if (!n || !p) {
	error("internal error");
	return;
    }

    //n->flags = may_sync;

    if (special != 'f') {
	if (special != 'm' || p->flags & is_valid) {
	    n->v = p->v;
	    n->flags |= p->flags & is_valid;
	}
	if (special != 'm' || p->expr) {
	    n->expr = copye(p->expr, dr, dc, r1, c1, r2, c2, special == 't');
	    if (p->flags & is_strexpr)
		n->flags |= is_strexpr;
	    else
		n->flags &= ~is_strexpr;
	}
	if (p->label) {
	    if (n->label) scxfree(n->label);
	    n->label = scxmalloc((unsigned) (strlen(p->label) + 1));
	    (void) strcpy(n->label, p->label);
	    n->flags &= ~is_leftflush;
	    n->flags |= ((p->flags & is_label) | (p->flags & is_leftflush));
	} else if (special != 'm') {
	    n->label = NULL;
	    n->flags &= ~(is_label | is_leftflush);
	}
	n->flags |= p->flags & is_locked;
    }
    if (p->format) {
	if (n->format) scxfree(n->format);
        n->format = scxmalloc((unsigned) (strlen(p->format) + 1));
	(void) strcpy(n->format, p->format);
    } else if (special != 'm' && special != 'f')
	n->format = NULL;
    n->flags |= is_changed;
    n->row = p->row;
    n->col = p->col;
    return;
}

/* add a plugin/mapping pair to the end of the filter list. type is r(ead) or w(rite)
void addplugin(char *ext, char *plugin, char type) {
    struct impexfilt *fp;
    char mesg[PATHLEN];

    if (!plugin_exists(plugin, strlen(plugin), mesg)) {
	error("Cannot find plugin %s", plugin);
	return;
    }
    if (filt == NULL) {
	filt = (struct impexfilt *) scxmalloc((unsigned)sizeof(struct impexfilt));
	fp = filt;
    } else {
	fp = filt;
	while (fp->next != NULL)
	    fp = fp->next;
	fp->next = (struct impexfilt *)scxmalloc((unsigned)sizeof(struct impexfilt));
	fp = fp->next;
    }
    strcpy(fp->plugin, plugin);
    strcpy(fp->ext, ext);
    fp->type = type;
    fp->next = NULL;
    return;
}
*/

int etype(register struct enode *e) {
    if (e == (struct enode *)0)
	return NUM;
    switch (e->op) {
	case UPPER: case LOWER: case CAPITAL:
	case O_SCONST: case '#': case DATE: case FMT: case STINDEX:
	case EXT: case SVAL: case SUBSTR:
	    return (STR);

	case '?':
	case IF:
	    return (etype(e->e.o.right->e.o.left));

	case 'f':
	    return (etype(e->e.o.right));

	case O_VAR: {
	    register struct ent *p;
	    p = e->e.v.vp;
	    if (p->expr) 
		return (p->flags & is_strexpr ? STR : NUM);
	    else if (p->label)
		return (STR);
	    else
		return (NUM);
	    }

	default:
	    return (NUM);
    }
    return;
}

// ignorelock is used when sorting so that locked cells can still be sorted
void erase_area(int sr, int sc, int er, int ec, int ignorelock) {
    int r, c;
    struct ent **pp;

    if (sr > er) {
	r = sr; sr = er; er = r;	
    }

    if (sc > ec) {
	c = sc; sc = ec; ec = c;	
    }

    if (sr < 0)
	sr = 0; 
    if (sc < 0)
	sc = 0;
    checkbounds(&er, &ec);

    // Do a lookat() for the upper left and lower right cells of the range
    // being erased to make sure they are included in the delete buffer so
    // that pulling cells always works correctly even if the cells at one
    // or more edges of the range are all empty.

    (void) lookat(sr, sc);
    (void) lookat(er, ec);

    for (r = sr; r <= er; r++) {
	for (c = sc; c <= ec; c++) {
	    pp = ATBL(tbl, r, c);
	    if (*pp && (!((*pp)->flags & is_locked) || ignorelock)) {
                mark_ent_as_deleted(*pp);
		*pp = NULL;
	    }
	}
    }
    return;
}

/*
char * findplugin(char *ext, char type) {
    struct impexfilt *fp;

    fp = filt;
    if (fp == NULL)
	return (NULL);
    if ((!strcmp(fp->ext, ext)) && (fp->type == type))
	return (fp->plugin);
    while (fp->next != NULL) {
	fp = fp->next;
	if ((!strcmp(fp->ext, ext)) && (fp->type == type))
	    return (fp->plugin);
    }

    return (NULL);
}
*/

struct enode * copye(register struct enode *e, int Rdelta, int Cdelta, int r1, int c1, int r2, int c2, int transpose) {
    register struct enode *ret;
    static struct enode *range = NULL;

    if (e == (struct enode *)0) {
	ret = (struct enode *)0;

    } else if (e->op & REDUCE) {
	int newrow, newcol;
	//if (freeenodes) {
	//    ret = freeenodes;
	//    freeenodes = ret->e.o.left;
	//} else
	    ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
	ret->op = e->op;
	newrow = e->e.r.left.vf & FIX_ROW ||
		 e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 ||
		 e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?
		 e->e.r.left.vp->row :
		 transpose ? r1 + Rdelta + e->e.r.left.vp->col - c1 :
		 e->e.r.left.vp->row + Rdelta;
	newcol = e->e.r.left.vf & FIX_COL ||
		 e->e.r.left.vp->row < r1 || e->e.r.left.vp->row > r2 ||
		 e->e.r.left.vp->col < c1 || e->e.r.left.vp->col > c2 ?
		 e->e.r.left.vp->col :
		 transpose ? c1 + Cdelta + e->e.r.left.vp->row - r1 :
		 e->e.r.left.vp->col + Cdelta;
	ret->e.r.left.vp = lookat(newrow, newcol);
	ret->e.r.left.vf = e->e.r.left.vf;
	newrow = e->e.r.right.vf & FIX_ROW ||
		 e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 ||
		 e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?
		 e->e.r.right.vp->row :
		 transpose ? r1 + Rdelta + e->e.r.right.vp->col - c1 :
		 e->e.r.right.vp->row + Rdelta;
	newcol = e->e.r.right.vf & FIX_COL ||
		 e->e.r.right.vp->row < r1 || e->e.r.right.vp->row > r2 ||
		 e->e.r.right.vp->col < c1 || e->e.r.right.vp->col > c2 ?
		 e->e.r.right.vp->col :
		 transpose ? c1 + Cdelta + e->e.r.right.vp->row - r1 :
		 e->e.r.right.vp->col + Cdelta;
	ret->e.r.right.vp = lookat(newrow, newcol);
	ret->e.r.right.vf = e->e.r.right.vf;

    } else {
	struct enode *temprange=0;

	//if (freeenodes) {
	//    ret = freeenodes;
	//    freeenodes = ret->e.o.left;
	//} else
	    ret = (struct enode *) scxmalloc((unsigned) sizeof (struct enode));
	ret->op = e->op;
	switch (ret->op) {
	    case SUM:
	    case PROD:
	    case AVG:
	    case COUNT:
	    case STDDEV:
	    case MAX:
	    case MIN:
		temprange = range;
		range = e->e.o.left;
		r1 = 0;
		c1 = 0;
		r2 = maxrow;
		c2 = maxcol;
	}
	switch (ret->op) {
	    case 'v':
		{
		    int newrow, newcol;
		    if (range && e->e.v.vp->row >= range->e.r.left.vp->row &&
				 e->e.v.vp->row <= range->e.r.right.vp->row &&
				 e->e.v.vp->col >= range->e.r.left.vp->col &&
				 e->e.v.vp->col <= range->e.r.right.vp->col) {
			newrow = range->e.r.left.vf & FIX_ROW ?
				e->e.v.vp->row : e->e.v.vp->row + Rdelta;
			newcol = range->e.r.left.vf & FIX_COL ?
				e->e.v.vp->col : e->e.v.vp->col + Cdelta;
		    } else {
			newrow = e->e.v.vf & FIX_ROW ||
				 e->e.v.vp->row < r1 || e->e.v.vp->row > r2 ||
				 e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?
				 e->e.v.vp->row :
				 transpose ? r1 + Rdelta + e->e.v.vp->col - c1 :
				 e->e.v.vp->row + Rdelta;
			newcol = e->e.v.vf & FIX_COL ||
				 e->e.v.vp->row < r1 || e->e.v.vp->row > r2 ||
				 e->e.v.vp->col < c1 || e->e.v.vp->col > c2 ?
				 e->e.v.vp->col :
				 transpose ? c1 + Cdelta + e->e.v.vp->row - r1 :
				 e->e.v.vp->col + Cdelta;
		    }
		    ret->e.v.vp = lookat(newrow, newcol);
		    ret->e.v.vf = e->e.v.vf;
		    break;
		}
	    case 'k':
		ret->e.k = e->e.k;
		break;
	    case 'f':
	    case 'F':
		if ((range && ret->op == 'F') || (!range && ret->op == 'f'))
		    Rdelta = Cdelta = 0;
		ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
		ret->e.o.right = (struct enode *)0;
 		break;
	    case '$':
	    case EXT:
		ret->e.s = scxmalloc((unsigned) strlen(e->e.s)+1);
		(void) strcpy(ret->e.s, e->e.s);
		if (e->op == '$')	/* Drop through if ret->op is EXT */
		    break;
	    default:
		ret->e.o.left = copye(e->e.o.left, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
		ret->e.o.right = copye(e->e.o.right, Rdelta, Cdelta, r1, c1, r2, c2, transpose);
		break;
	}
	switch (ret->op) {
	    case SUM:
	    case PROD:
	    case AVG:
	    case COUNT:
	    case STDDEV:
	    case MAX:
	    case MIN:
		range = temprange;
	}
    }
    return ret;
}

/*
void doend(int rowinc, int colinc) {
    register struct ent *p;
    int r, c;

    if (!loading)
	//remember(0);

    lastrow = currow;
    lastcol = curcol;

    if (VALID_CELL(p, currow, curcol)) {
	r = currow + rowinc;
	c = curcol + colinc;
	if (r >= 0 && r < maxrows && 
	    c >= 0 && c < maxcols &&
	    !VALID_CELL(p, r, c)) {
		currow = r;
		curcol = c;
	}
    }

    if (!VALID_CELL(p, currow, curcol)) {
        switch (rowinc) {
        case -1:
	    while (!VALID_CELL(p, currow, curcol) && currow > 0)
		currow--;
	    break;
        case  1:
	    while (!VALID_CELL(p, currow, curcol) && currow < maxrows-1)
		currow++;
	    break;
        case  0:
            switch (colinc) {
 	    case -1:
	        while (!VALID_CELL(p, currow, curcol) && curcol > 0)
		    curcol--;
	        break;
 	    case  1:
	        while (!VALID_CELL(p, currow, curcol) && curcol < maxcols-1)
		    curcol++;
	        break;
	    }
            break;
        }
	//rowsinrange = 1;
	//colsinrange = fwidth[curcol];
	//if (!loading) remember(1);

	info ("");	// clear line
	return;
    }

    switch (rowinc) {
    case -1:
	while (VALID_CELL(p, currow, curcol) && currow > 0)
	    currow--;
	break;
    case  1:
	while (VALID_CELL(p, currow, curcol) && currow < maxrows-1)
	    currow++;
	break;
    case  0:
	switch (colinc) {
	case -1:
	    while (VALID_CELL(p, currow, curcol) && curcol > 0)
		curcol--;
	    break;
	case  1:
	    while (VALID_CELL(p, currow, curcol) && curcol < maxcols-1)
		curcol++;
	    break;
	}
	break;
    }
    if (!VALID_CELL(p, currow, curcol)) {
        currow -= rowinc;
        curcol -= colinc;
    }
    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];
}
*/

/* Modified 9/17/90 THA to handle more formats */
void doformat(int c1, int c2, int w, int p, int r) {
    register int i;
    int crows = 0;
    int ccols = c2;

    if (c1 >= maxcols && !growtbl(GROWCOL, 0, c1)) c1 = maxcols-1 ;
    if (c2 >= maxcols && !growtbl(GROWCOL, 0, c2)) c2 = maxcols-1 ;
    
    if (w == 0) {
	info("Width too small - setting to 1");
	w = 1;
    }

    if (usecurses && w > COLS - rescol - 2) {
	info("Width too large - Maximum = %d", COLS - rescol - 2);
	w = COLS - rescol - 2;
    }

    if (p > w) {
	info("Precision too large");
	p = w;
    }

    checkbounds(&crows, &ccols);
    if (ccols < c2) {
	error("Format statement failed to create implied column %d", c2);
	return;
    }

    for (i = c1; i <= c2; i++)
	fwidth[i] = w, precision[i] = p, realfmt[i] = r;

    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];

    modflg++;
    return;
    
}

void formatcol(int c) {
    /*
    int c, i;
    int mf = modflg;

    int *oldformat;
    info("Current format is %d %d %d", fwidth[curcol], precision[curcol], realfmt[curcol]);
    refresh();
    oldformat = (int *)scxmalloc(arg*3*sizeof(int));
    for (i = 0; i < arg; i++) {
	oldformat[i * 3 + 0] = fwidth[i + curcol];
	oldformat[i * 3 + 1] = precision[i + curcol];
	oldformat[i * 3 + 2] = realfmt[i + curcol];
    }
    c = nmgetch();
    while (c >= 0 && c != ctl('m') && c != 'q' && c != OKEY_ESC &&
	    c != ctl('g') && linelim < 0) {
	if (c >= '0' && c <= '9')
	    for (i = curcol; i < curcol + arg; i++)
		realfmt[i] = c - '0';
	else
    */
    int arg = 1;
    int i;

    switch (c) {
        case '<':
        case 'h':
	case OKEY_LEFT:
	    for (i = curcol; i < curcol + arg; i++) {
		fwidth[i]--;
		if (fwidth[i] < 1)
		    fwidth[i] = 1;
	    }
	    //rowsinrange = 1;
	    //colsinrange = fwidth[curcol];
	    modflg++;
	    break;
	case '>':
	case 'l':
	case OKEY_RIGHT:
	    for (i = curcol; i < curcol + arg; i++) {
		fwidth[i]++;
		if (fwidth[i] > COLS - rescol - 2)
		    fwidth[i] = COLS - rescol - 2;
	    }
	    //rowsinrange = 1;
	    //colsinrange = fwidth[curcol];
	    modflg++;
	    break;
	case '-':
	    for (i = curcol; i < curcol + arg; i++) {
		precision[i]--;
		if (precision[i] < 0)
		    precision[i] = 0;
	    }
	    modflg++;
	    break;
	case '+':
	    for (i = curcol; i < curcol + arg; i++)
		precision[i]++;
	    modflg++;
	    break;
/*
	case ' ':
	    if (arg == 1)
		(void) sprintf(line,
			"format [for column] %s ",
			coltoa(curcol));
	    else {
		(void) sprintf(line,
			"format [for columns] %s:",
			coltoa(curcol));
		(void) sprintf(line+strlen(line), "%s ",
			coltoa(curcol+arg-1));
	    }
	    linelim = strlen(line);
	    //insert_mode();
	    info("Current format is %d %d %d", fwidth[curcol], precision[curcol], realfmt[curcol]);
	    continue;
	case '=':
	    info("Define format type (0-9):");
	    refresh();
	    if ((c = nmgetch()) >= '0' && c <= '9') {
		if (colformat[c-'0']) {
		    (void) sprintf(line, "format %c = \"%s\"", c, colformat[c-'0']);
		    //edit_mode();
		    linelim = strlen(line) - 1;
		} else {
		    (void) sprintf(line, "format %c = \"", c);
		    //insert_mode();
		    linelim = strlen(line);
		}
		info("");
	    } else {
		error("Invalid format type");
		c = -1;
	    }
	    continue;
	case ctl('l'):
	    FullUpdate++;
	    clearok(stdscr, 1);
	    break;
	default:
	    break;
*/
    }
    info("Current format is %d %d %d", fwidth[curcol], precision[curcol], realfmt[curcol]);
//  FullUpdate++;
    update();
/*  refresh();
    if (linelim < 0)
	    if ((c = nmgetch()) == OKEY_ESC || c == ctl('g') || c == 'q') {
		for (i = 0; i < arg; i++) {
		    fwidth[i + curcol] = oldformat[i * 3 + 0];
		    precision[i + curcol] = oldformat[i * 3 + 1];
		    realfmt[i + curcol] = oldformat[i * 3 + 2];
		}
		modflg = mf;
		FullUpdate++;
		update();
	    }
    }
    scxfree((char *)oldformat);
    if (c >= 0) {
	info("");
    }
*/
    return;
}

struct ent * left_limit() {
    return lookat(currow, 0);
}

struct ent * right_limit() {
    register struct ent *p;
    int c = maxcols - 1;
    while (!VALID_CELL(p, currow, c) && c > 0)
        c--;
    return lookat(currow, c);
}

struct ent * goto_top() {
    return lookat(0, curcol);
}

struct ent * goto_bottom() {
    register struct ent *p;
    int r = maxrows - 1;
    while (!VALID_CELL(p, r, curcol) && r > 0)
        r--;
    return lookat(r, curcol);
}

// Insert a single row.  It will be inserted before currow
// if after is 0; after if it is 1.
void insert_row(int after) {
    int	r, c;
    struct ent	**tmprow, **pp;
    int lim = maxrow - currow + 1;

    if (currow > maxrow) maxrow = currow;
    maxrow++;
    lim = maxrow - lim + after;
    if (maxrow >= maxrows && !growtbl(GROWROW, maxrow, 0)) return;

    tmprow = tbl[maxrow];
    for (r = maxrow; r > lim; r--) {
        row_hidden[r] = row_hidden[r-1];
        tbl[r] = tbl[r-1];
        for (c = 0, pp = ATBL(tbl, r, 0); c < maxcols; c++, pp++)
            if (*pp) (*pp)->row = r;
    }
    tbl[r] = tmprow;		// the last row is never used

    modflg++;
    return;
}

// Insert a single col. The col will be inserted
// BEFORE CURCOL if after is 0;
// AFTER  CURCOL if it is 1.
void insert_col(int after) {
    int r, c;
    register struct ent **pp;
    int lim = maxcol - curcol - after + 1;
    struct frange *fr;

    if (curcol + after > maxcol)
	maxcol = curcol + after;
    maxcol++;

    if ((maxcol >= maxcols) && !growtbl(GROWCOL, 0, maxcol))
	return;

    for (c = maxcol; c >= curcol + after + 1; c--) {
	fwidth[c] = fwidth[c-1];
	precision[c] = precision[c-1];
	realfmt[c] = realfmt[c-1];
	//col_hidden[c] = col_hidden[c-1];
    }
    for (c = curcol + after; c - curcol - after < 1; c++) {
    	fwidth[c] = DEFWIDTH;
	precision[c] =  DEFPREC;
	realfmt[c] = DEFREFMT;
	//col_hidden[c] = FALSE;
    }
	
    for (r=0; r <= maxrow; r++) {
	pp = ATBL(tbl, r, maxcol);
	for (c = lim; --c >= 0; pp--)
	    if ((pp[0] = pp[-1])) pp[0]->col++;

	pp = ATBL(tbl, r, curcol + after);
	for (c = curcol + after; c - curcol - after < 1; c++, pp++)
	    *pp = (struct ent *) 0;
    }

    curcol += after;
    modflg++;
    return;
}

// delete a row
void deleterow() {
    int rs = maxrow - currow + 1;
    struct ent *p;
    struct ent *obuf = NULL;
    register struct ent **pp;
    int			r, c, i;
    struct ent		**tmprow;


    if (any_locked_cells(currow, 0, currow, maxcol)) {
        info("Locked cells encountered. Nothing changed");

    } else {
        //flush_saved();
        erase_area(currow, 0, currow, maxcol, 0);
        if (currow > maxrow) return;
        r = currow;

        // Rows are dealt with in numrow groups, each group of rows spaced numrow rows apart.
        // save the first row of the group and empty it out
        tmprow = tbl[r];
        pp = ATBL(tbl, r, 0);
        for (c = maxcol + 1; --c >= 0; pp++) {
            if (*pp != NULL) {
                mark_ent_as_deleted(*pp);
                *pp = NULL;
                //clearent(*pp);
                //free(*pp);
            }
        }

        // move the rows, put the deleted, but now empty, row at the end
        for (; r + 1 < maxrows - 1; r++) {
            //row_hidden[r] = row_hidden[r+1];
            tbl[r] = tbl[r + 1];
            pp = ATBL(tbl, r, 0);
            for (c = 0; c < maxcols; c++, pp++)
        	if (*pp) (*pp)->row = r;
        }
        tbl[r] = tmprow;
        
        maxrow--;

        sync_refs();
        //flush_saved(); // we have to flush only at exit. this is in case we want to UNDO
        modflg++;
    }
    return;
}

void ljustify(sr, sc, er, ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
	i = sr;
	sr = er;
	er = i;
    }
    if (sc > ec) {
	i = sc;
	sc = ec;
	ec = i;
    }
    for (i = sr; i <= er; i++) {
	for (j = sc; j <= ec; j++) {
	    p = *ATBL(tbl, i, j);
	    if (p && p->label) {
		p->flags &= ~is_label;
		p->flags |= is_leftflush | is_changed;
		changed++;
		modflg++;
	    }
	}
    }
    return;
}

void rjustify(sr, sc, er, ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
	i = sr;
	sr = er;
	er = i;
    }
    if (sc > ec) {
	i = sc;
	sc = ec;
	ec = i;
    }
    for (i = sr; i <= er; i++) {
	for (j = sc; j <= ec; j++) {
	    p = *ATBL(tbl, i, j);
	    if (p && p->label) {
		p->flags &= ~(is_label | is_leftflush);
		p->flags |= is_changed;
		changed++;
		modflg++;
	    }
	}
    }
    return;
}

void center(sr, sc, er, ec) {
    struct ent *p;
    int i, j;

    if (sr > er) {
	i = sr;
	sr = er;
	er = i;
    }
    if (sc > ec) {
	i = sc;
	sc = ec;
	ec = i;
    }
    for (i = sr; i <= er; i++) {
	for (j = sc; j <= ec; j++) {
	    p = *ATBL(tbl, i, j);
	    if (p && p->label) {
		p->flags &= ~is_leftflush;
		p->flags |= is_label | is_changed;
		changed++;
		modflg++;
	    }
	}
    }
    return;
}

/*
void move_area(int dr, int dc, int sr, int sc, int er, int ec) {
    struct ent *p;
    struct ent **pp;
    int deltar, deltac;
    int r, c;

    if (sr > er) r = sr; sr = er; er = r;
    if (sc > ec) c = sc; sc = ec; ec = c;
    if (sr < 0) sr = 0;
    if (sc < 0) sc = 0;
    checkbounds(&er, &ec);

    r = currow;
    currow = sr;
    c = curcol;
    curcol = sc;

    // First we erase the source range, which puts the cells on the delete
    // buffer stack.
    erase_area(sr, sc, er, ec, 0);

    currow = r;
    curcol = c;
    deltar = dr - sr;
    deltac = dc - sc;

    // Now we erase the destination range, which adds it to the delete buffer
    // stack, but then we flush it off.  We then move the original source
    // range from the stack to the destination range, adjusting the addresses
    // as we go, leaving the stack in its original state.
    erase_area(dr, dc, er + deltar, ec + deltac, 0);
    //flush_saved();
    for (p = delbuf[dbidx]; p; p = p->next) {
        pp = ATBL(tbl, p->row + deltar, p->col + deltac);
        *pp = p;
        p->row += deltar;
        p->col += deltac;
        p->flags &= ~is_deleted;
    }
}
*/

void chg_mode(char strcmd){
    switch (strcmd) {
        case '=': 
            curmode = INSERT_MODE;
            break;
        case '<':
            curmode = INSERT_MODE;
            break;
        case '>':
            curmode = INSERT_MODE;
            break;
        case '\\':
            curmode = INSERT_MODE;
            break;
        case 'E':
            curmode = EDIT_MODE;
            break;
        case 'e':
            curmode = EDIT_MODE;
            break;
        case ':':
            curmode = COMMAND_MODE;
            break;
        case '.':
            curmode = NORMAL_MODE;
            break;
        case 'v':
            curmode = VISUAL_MODE;
            break;
    } 
    return;
}

// del selected cells
// can be a single cell or a range
void del_selected_cells() {

    // delete range
    if (is_range_selected() != -1) {
       srange * r = get_selected_range();
       yank_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a', 1);

       create_undo_action();
       copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'd');

       erase_area(r->tlrow, r->tlcol, r->brrow, r->brcol, 0);
       modflg++;
       sync_refs();
       //flush_saved(); NO DESCOMENTAR. VER ABAJO.

       copy_to_undostruct(r->tlrow, r->tlcol, r->brrow, r->brcol, 'a');
       end_undo_action();

    // delete cell
    } else {
       yank_area(currow, curcol, currow, curcol, 'e', 1);

       create_undo_action();
       copy_to_undostruct(currow, curcol, currow, curcol, 'd');

       erase_area(currow, curcol, currow, curcol, 0);
       modflg++;
       sync_refs();

       copy_to_undostruct(currow, curcol, currow, curcol, 'a');
       end_undo_action();

/*
       // Para UNDO
       // Save deleted ent
       struct ent * e_prev = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
       cleanent(e_prev);
       copyent(e_prev, lookat(currow, curcol), 0, 0, 0, 0, currow, curcol, 0);
       // Save ents that depends on the deleted one: TODO
       //struct ent * e_prev2 = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
       //cleanent(e_prev2);
       //copyent(e_prev2, lookat(currow+9, curcol+2), 0, 0, 0, 0, currow+9, curcol+2, 0);
       //e_prev->next = e_prev2;
       
       // create undo struct:
       struct undo u;
       u.removed = e_prev;
       u.added = NULL; // Eliminar esta linea, luego de implementar los TODO descriptos

       // delete the ent:
       erase_area(currow, curcol, currow, curcol, 0);
       modflg++;
       // sync refs after the delete
       sync_refs();

       // Save current status of each ent that depends on the deleted one TODO
       struct ent * e_now = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
       cleanent(e_now);
       (void) copyent(e_now, lookat(currow+9, curcol+2), 0, 0, 0, 0, currow+9, curcol+2, 0);
       u.added = e_now;
       
       // Add undo struct to undolist:
       add_to_undolist(&u); // <---------|
*/
       //flush_saved(); // No descomentar. Se debe hacer flush SOLO al salir
    } 

    return;
}

// Change cell content sending inputline to interpreter
void insert_or_edit_cell() {
    char ope[15] = "";
    switch (insert_edit_submode) {
        case '=':
            strcpy(ope, "let");
            break;
        case '<':
            strcpy(ope, "leftstring");
            break;
        case '>':
            strcpy(ope, "rightstring");
            break;
        case '\\':
            strcpy(ope, "label");
            break;
    }
    if (insert_edit_submode != '=') {
        add_char(inputline, '\"', 0);
        add_char(inputline, '\"', strlen(inputline));
    }

    create_undo_action();
    copy_to_undostruct(currow, curcol, currow, curcol, 'd');
    (void) sprintf(line, "%s %s = %s", ope, v_name(currow, curcol), inputline);
    send_to_interp(line); 
    copy_to_undostruct(currow, curcol, currow, curcol, 'a');
    end_undo_action();

    line[0]='\0';
    inputline[0]='\0';
    inputline_pos = 0;
    chg_mode('.');
    clr_header(input_win, 0);

    char * opt = get_conf_value("newline_action");
    switch (opt[0]) {
        case 'j':
            currow = forw_row(1)->row; 
            break;
        case 'l':
            curcol = forw_col(1)->col;
            break;
    }
    update();
    return;
}

// Send command to interpreter
void send_to_interp(char * oper) {
    //info(oper);
    linelim = 0;
    (void) yyparse();
    if (atoi(get_conf_value("autocalc"))) EvalAll();
    return;
}

/* return a pointer to a cell's [struct ent *], creating if needed */
struct ent * lookat(int row, int col) {
    register struct ent **pp;

    checkbounds(&row, &col);
    pp = ATBL(tbl, row, col);
    if ( *pp == NULL ) {
	//*pp = (struct ent *) scxmalloc( (unsigned) sizeof(struct ent) );
	*pp = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
	if (row > maxrow) maxrow = row;
	if (col > maxcol) maxcol = col;
	(*pp)->label = (char *) 0;
	(*pp)->row = row;
	(*pp)->col = col;
	(*pp)->flags = may_sync;
	(*pp)->expr = (struct enode *) 0;
	(*pp)->v = (double) 0.0;
	(*pp)->format = (char *) 0;
	(*pp)->cellerror = CELLOK;
	(*pp)->next = NULL;
    }
    return (*pp);
}

// cleanent: blank an ent
void cleanent(struct ent * p) {
    p->label = (char *) 0;
    p->row = 0;
    p->col = 0;
    p->flags = may_sync;
    p->expr = (struct enode *) 0;
    p->v = (double) 0.0;
    p->format = (char *) 0;
    p->cellerror = CELLOK;
    p->next = NULL;
    return;
}

// clearent: free memory of an ent (its contents)
void clearent(struct ent *v) {
    if (!v) return;

    label(v, "", -1);
    v->v = (double)0;

    if (v->expr) efree(v->expr);
    v->expr = NULL;

    if (v->format) scxfree(v->format);
    v->format = NULL;

    v->flags = ( is_changed | iscleared );
    changed++;
    modflg++;

    return;
}

void scroll_left(int n) {
    while (n--) {
        if (! offscr_sc_cols ) {
            //update;
            break;
        }
        int a = 1;
        int b = 0;
        offscr_sc_cols--;
        while (a != b && curcol) {
            a = offscr_sc_cols;
            calc_offscr_sc_cols();
            b = offscr_sc_cols;
            if (a != b) {
                curcol --;
                offscr_sc_cols = a;
            }
        }
    }
    return;
}

void scroll_right(int n) {
    while (n--) {
        // este while es para que el cursor se desplace a la
        // derecha cuando llegamos a la ultima columna visible en pantalla
        while (curcol < offscr_sc_cols + 1) {
            curcol++;
        }
        offscr_sc_cols++;
    }
    return;
}

void scroll_down(int n) {
    while (n--) {
        if (currow == offscr_sc_rows) {
            forw_row(1);
            unselect_ranges();
        }
        offscr_sc_rows++;
    }
    return;
}

void scroll_up(int n) {
    while (n--) {
        if (offscr_sc_rows)
            offscr_sc_rows--;
        else
            break;
        if (currow == offscr_sc_rows + LINES - RESROW - 1) {
            back_row(1);
            unselect_ranges();
        }
    }
    return;
}













// moves curcol back one displayed column
struct ent * back_col(int arg) {
    int c = curcol;
    while (--arg >= 0) {
	if (c)
	    c--;
	else {
            info ("At column A");
            break; 
       }
	while( col_hidden[c] && c )
	    c--;
    }
    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];
    return lookat(currow, c);
}

/* moves curcol forward one displayed column */
struct ent * forw_col(int arg) {
    int c = curcol;
    while (--arg >= 0) {
	if (c < maxcols - 1)
	    c++;
	else
        if (! growtbl(GROWCOL, 0, arg))	/* get as much as needed */
            break;
	else
            c++;
	while (col_hidden[c] && (c < maxcols - 1))
            c++;
    }
    //rowsinrange = 1;
    //colsinrange = fwidth[curcol];
    return lookat(currow, c);
}

/* moves currow forward one displayed row */
struct ent * forw_row(int arg) {
    int r = currow;
    while (arg--) {
	if (r < maxrows - 1)
	    r++;
	else {
            if (!growtbl(GROWROW, arg, 0)) {
                error("cant grow");
                break;
	    } else
                r++;
            }
	    while (row_hidden[r] && (r < maxrows - 1)) {
	        r++;
            }
    }
    return lookat(r, curcol);
}

/* moves currow backward one displayed row */
struct ent * back_row(int arg) {
    int r = currow;
    while (arg--) {
	if (r) r--;
	else { 
            info("At row zero");
            break;
        }
	while (row_hidden[r] && r)
	    r--;
    }
    return lookat(r, curcol);
}

struct ent * go_home() {
    return lookat(0, 0);
}

struct ent * go_end() {
    int r = currow, c = curcol;
    int raux = r, caux = c;
    register struct ent *p;
    do {
        if (c < maxcols - 1)
            c++;
        else {
            if (r < maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if  (VALID_CELL(p, r, c) ) { raux = r; caux = c; }
    } while ( r < maxrows || c < maxcols );
    if ( ! VALID_CELL(p, r, c) ) return lookat(raux, caux);
}

// if ticks a cell, returns struct ent *
// if ticks a range, return struct ent * to top left cell
struct ent * tick(char c) {
    //tick cell
    int r = get_mark(c)->row;
    if (r != -1)
        return lookat(r, get_mark(c)->col);

    // tick range
    if (curmode != VISUAL_MODE) {
        get_mark(c)->rng->selected = 1; 
        return lookat(get_mark(c)->rng->tlrow, get_mark(c)->rng->tlcol);
    }
    return NULL;
}

struct ent * go_forward() {
    int r = currow, c = curcol;
    int r_ori = r, c_ori = c;
    register struct ent * p;
    do {
        if (c < maxcols - 1) {
            c++;
        } else {
            if (r < maxrows - 1) {
                r++;
                c = 0;
            } else break;
        }
        if (VALID_CELL(p, r, c) ) {
            return lookat(r, c);
        }
    } while (r < maxrows || c < maxcols);

    if ( ! VALID_CELL(p, r, c) )
        return lookat(r_ori, c_ori);

}

struct ent * go_backward() {
    int r = currow, c = curcol;
    int r_ori = r, c_ori = c;
    register struct ent *p;
    do {
        if (c) 
            c--;
        else {
            if (r) {
                r--;
                c = maxcols - 1;
            } else break;
        }
        if  (VALID_CELL(p, r, c) )
            return lookat(r, c);
    } while ( currow || curcol );

    if ( ! VALID_CELL(p, r, c) )
        return lookat(r_ori, c_ori);
}

struct ent * vert_top() {
    return currow < LINES - RESROW - 1 ? lookat(0, curcol) : lookat(offscr_sc_rows, curcol);
}

struct ent * vert_middle() {
    int bottom = offscr_sc_rows + LINES - RESROW - 2;
    if (bottom > maxrow) bottom = maxrow;
    return lookat( ((currow < LINES - RESROW - 1 ? 0 : offscr_sc_rows) + bottom) / 2, curcol);
}

struct ent * vert_bottom() {
    int c = offscr_sc_rows + LINES - RESROW - 2;
    if (c > maxrow) c = maxrow;
    return lookat(c, curcol);
}

struct ent * go_bol() {
    return lookat(currow, offscr_sc_cols);
}

struct ent * go_eol() {
    return lookat(currow, offscr_sc_cols + calc_offscr_sc_cols() - 1);
}

struct ent * horiz_middle() {
    int i;
    int ancho = rescol;
    int visibles = calc_offscr_sc_cols();
    for (i = offscr_sc_cols; i < offscr_sc_cols + visibles; i++) {
        ancho += fwidth[i];
        if (ancho >= (COLS-rescol)/2) {
            return lookat(currow, i);
        }
    }
}






// Funcion que indica si el contenido completo de un buffer
// forma un comando valido.
// res = 0 or NO_CMD : buf has no command
// res = 1 or EDITION_CMD : buf has a command
// res = 2 or MOVEMENT_CMD : buf has a movement command or a command that do not
// change cell content, and should not be considered by the '.' command
int is_single_command (struct block * buf, long timeout) {
    if (buf->value == '\0') return NO_CMD;
    int res = NO_CMD;
    int bs = get_bufsize(buf);

    if (curmode == COMMAND_MODE) {
        res = MOVEMENT_CMD;

    } else if (curmode == INSERT_MODE) {
        res = MOVEMENT_CMD;

    } else if (curmode == EDIT_MODE && bs == 1) {
        res = MOVEMENT_CMD;

    } else if (curmode == NORMAL_MODE && bs == 1) {
        // commands for changing mode
        if (buf->value == ':') res = MOVEMENT_CMD;
        else if (buf->value == '\\') res = MOVEMENT_CMD;
        else if (buf->value == '<') res = MOVEMENT_CMD;
        else if (buf->value == '>') res = MOVEMENT_CMD;
        else if (buf->value == '=') res = MOVEMENT_CMD;
        else if (buf->value == 'e') res = MOVEMENT_CMD;
        else if (buf->value == 'E') res = MOVEMENT_CMD;
        else if (buf->value == 'v') res = MOVEMENT_CMD;

        // movement commands
        else if (buf->value == 'j') res = MOVEMENT_CMD;
        else if (buf->value == 'k') res = MOVEMENT_CMD;
        else if (buf->value == 'h') res = MOVEMENT_CMD;
        else if (buf->value == 'l') res = MOVEMENT_CMD;
        else if (buf->value == '0') res = MOVEMENT_CMD;
        else if (buf->value == '$') res = MOVEMENT_CMD;
        else if (buf->value == OKEY_HOME) res = MOVEMENT_CMD;
        else if (buf->value == OKEY_END) res = MOVEMENT_CMD;
        else if (buf->value == '#') res = MOVEMENT_CMD;
        else if (buf->value == '^') res = MOVEMENT_CMD;
        else if (buf->value == OKEY_LEFT) res = MOVEMENT_CMD;
        else if (buf->value == OKEY_RIGHT) res = MOVEMENT_CMD;
        else if (buf->value == OKEY_DOWN) res = MOVEMENT_CMD;
        else if (buf->value == OKEY_UP) res = MOVEMENT_CMD;
        else if (buf->value == ctl('f')) res = MOVEMENT_CMD;
        else if (buf->value == ctl('b')) res = MOVEMENT_CMD;
        else if (buf->value == ctl('a')) res = MOVEMENT_CMD;
        else if (buf->value == 'G') res = MOVEMENT_CMD;
        else if (buf->value == 'H') res = MOVEMENT_CMD;
        else if (buf->value == 'M') res = MOVEMENT_CMD;
        else if (buf->value == 'L') res = MOVEMENT_CMD;
        else if (buf->value == ctl('y')) res = MOVEMENT_CMD;
        else if (buf->value == ctl('e')) res = MOVEMENT_CMD;
        else if (buf->value == ctl('l')) res = MOVEMENT_CMD;
        else if (buf->value == 'w') res = MOVEMENT_CMD;
        else if (buf->value == 'b') res = MOVEMENT_CMD;

        else if (buf->value == 'x') res = EDITION_CMD;  // cuts a cell
        else if (buf->value == 'u') res = EDITION_CMD;  // undo
        else if (buf->value == ctl('r')) res = EDITION_CMD; // redo
        else if (buf->value == '@') res = EDITION_CMD; // EvallAll 
        else if (buf->value == '{') res = EDITION_CMD;
        else if (buf->value == '}') res = EDITION_CMD;
        else if (buf->value == '|') res = EDITION_CMD;
        else if (buf->value == 'p') res = EDITION_CMD; // paste yanked cells below or right
        else if (buf->value == 'P') res = EDITION_CMD; // paste yanked cells above or left

        else if (isdigit(buf->value) && atoi(get_conf_value("numeric")) ) res = MOVEMENT_CMD; // repeat last command

        else if (buf->value == '.') res = MOVEMENT_CMD; // repeat last command
        else if (buf->value == 'y' && is_range_selected() != -1) res = EDITION_CMD; // yank range
        else if (buf->value == 't') res = EDITION_CMD; // PARA PRUEBA

    } else if (curmode == NORMAL_MODE) {

        if (buf->value == 'g' && bs == 2 && (
             buf->pnext->value == 'M' || buf->pnext->value == 'g' ||
             buf->pnext->value == 'G' ||
             buf->pnext->value == '0' || buf->pnext->value == '$')) res = MOVEMENT_CMD;
        

        else if (buf->value == 'g' && bs > 2 && timeout >= COMPLETECMDTIMEOUT) res = MOVEMENT_CMD; // goto cell
                                                 // TODO add validation: buf->pnext->value debe ser letra


        else if (buf->value == 'y' && bs == 2 &&    // yank cell
            ( buf->pnext->value == 'y' || buf->pnext->value == 'r' ||
              buf->pnext->value == 'c') ) res = EDITION_CMD;

        else if (buf->value == 'm' && bs == 2 &&    // mark
            ((buf->pnext->value - ('a' - 1)) < 1 || buf->pnext->value > 26)) res = MOVEMENT_CMD;

        else if (buf->value == 'z' && bs == 2 &&    // scrolling
            ( buf->pnext->value == 'h' || buf->pnext->value == 'l' ||
              buf->pnext->value == 'z' || buf->pnext->value == 'm' ||
              buf->pnext->value == 'H' || buf->pnext->value == 'L')
            ) res = MOVEMENT_CMD;

        else if (buf->value == 'd' && bs == 2 &&    // cuts a cell
            buf->pnext->value == 'd') res = EDITION_CMD;

        else if (buf->value == '\'' && bs == 2 &&   // tick 
            ((buf->pnext->value - ('a' - 1)) < 1 || buf->pnext->value > 26)) res = MOVEMENT_CMD;

        else if (buf->value == 's' && bs == 2 &&    // shift cell down or up
            ( buf->pnext->value == 'j' || buf->pnext->value == 'k' ||
              buf->pnext->value == 'h' || buf->pnext->value == 'l')) res = EDITION_CMD;

        else if (buf->value == 'i' && bs == 2 &&    // Insert row or column
            ( buf->pnext->value == 'r' || buf->pnext->value == 'c' )) res = EDITION_CMD;

        else if (buf->value == 'd' && bs == 2 &&    // Delete row or column
            ( buf->pnext->value == 'r' || buf->pnext->value == 'c' )) res = EDITION_CMD;

        else if (buf->value == 'r' && bs == 3 &&    // Create range with two marks
            //  FIXME add validation
            ((buf->pnext->value - ('a' - 1)) < 1 || buf->pnext->value > 26) &&
            ((buf->pnext->pnext->value - ('a' - 1)) < 1 || buf->pnext->pnext->value > 26)) res = EDITION_CMD;

        else if (buf->value == 'f' && bs == 2 &&    // Format col
            ( buf->pnext->value == '>' || buf->pnext->value == '<'              ||
            buf->pnext->value == 'h'   || buf->pnext->value == OKEY_LEFT        ||
            buf->pnext->value == 'l'   || buf->pnext->value == OKEY_RIGHT       ||
            buf->pnext->value == '-'   || buf->pnext->value == '+' )) res = EDITION_CMD;

    } else if (curmode == VISUAL_MODE && bs == 1) {
        if (buf->value == 'j'          || buf->value == OKEY_DOWN || buf->value == 'k'      || buf->value == OKEY_UP    ||
            buf->value == 'h'          || buf->value == OKEY_LEFT || buf->value == 'l'      || buf->value == OKEY_RIGHT ||
            buf->value == '$'          || buf->value == '0'       || buf->value == '#'      || buf->value == '^'        ||
            buf->value == 'y'          || buf->value == 'x'       || buf->value == 'w'      || buf->value == 'b'        ||
            buf->value == 'H'          || buf->value == 'M'       || buf->value == 'L'      || buf->value == 'G'        ||
            buf->value == ctl('f')     || buf->value == ctl('b')  || buf->value == ctl('a')
        )
                res = MOVEMENT_CMD;

    } else if (curmode == VISUAL_MODE && bs == 2) {
            if ((buf->value == '\'')   ||
                (buf->value == 'd' && buf->pnext->value == 'd')  ||
                (buf->value == 's' && ( buf->pnext->value == 'h' || buf->pnext->value == 'j' || buf->pnext->value == 'k' || buf->pnext->value == 'l' ))
               )
                res = MOVEMENT_CMD;
    }

    return res;
}
