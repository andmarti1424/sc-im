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
 * \file sc.h
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Header file for sc.c
 */

#ifndef SC_H_
#define SC_H_

#include <stdio.h>
#include <memory.h>

//#define ATBL(tbl, row, col)    (*(tbl + row) + (col))
extern struct ent ** ATBL(struct ent ***,int ,int );

#define MINROWS      100     /* minimum size at startup */

/* MAX rows size of sheet. Default 65536.   */
/* Can be changed up to 1048576 in Makefile */
#ifndef MAXROWS
#define MAXROWS    65536
#endif

#define MINCOLS       30
#define ABSMAXCOLS   702     /* MAX cols. (ZZ in base 26) */
#define CRROWS         1
#define CRCOLS         2
/* formats for engformat() */
#define REFMTFIX       0
#define REFMTFLT       1
#define REFMTENG       2
#define REFMTDATE      3
#define REFMTLDATE     4
#define DEFWIDTH      10     /* Default column width and precision */
#define DEFPREC        2
#define DEFREFMT      REFMTFIX /* Make default format fixed point  THA 10/14/90 */
#define FKEYS         24     /* Number of function keys available */
#define COLFORMATS    10     /* Number of custom column formats */
#define FBUFLEN     1024     /* buffer size for a single field */
#define PATHLEN     1024     /* maximum path length */
#define MAXCMD       160     /* for ! command and commands that use the pager */
#define STDERRBUF   8192     /* stderr buffer size */

#ifndef DFLT_PAGER
#define    DFLT_PAGER "more" /* more is probably more widespread than less */
#endif                       /* DFLT_PAGER */

#ifndef DFLT_EDITOR
#define    DFLT_EDITOR "vim"
#endif

//#ifndef A_CHARTEXT         /* Should be defined in curses.h */
// #define A_CHARTEXT 0xff
//#endif

#ifndef FALSE
    # define    FALSE   0
    # define    TRUE    1
#endif

/*
 * Some not too obvious things about the flags:
 *    is_valid means there is a valid number in v.
 *    is_locked means that the cell cannot be edited.
 *    is_label set means it points to a valid constant string.
 *    is_strexpr set means expr yields a string expression.
 *    If is_strexpr is not set, and expr points to an expression tree, the
 *        expression yields a numeric expression.
 *    So, either v or label can be set to a constant.
 *        Either (but not both at the same time) can be set from an expression.
 */

#define VALID_CELL(p, r, c) ((p = *ATBL(tbl, r, c)) && ((p->flags & is_valid) || p->label))

// TODO Properly document these structs using doxygen tags. Possibly
// not the same as functions. What is the standar way?

/* info for each cell, only alloc'd when something is stored in a cell */
struct ent {
    double v;             /* v && label are set in EvalAll() */
    char * label;
    struct enode * expr;  /* cell's contents */
    short flags;
    int row, col;
    struct ent * next;    // used for yanklist, freeents list, undo..
    char * format;        /* printf format for this cell */
    char cellerror;       /* error in a cell? */
    struct ucolor * ucolor;
    struct trigger * trigger;
    int pad;              // padding between other cells
};

#define FIX_ROW 1
#define FIX_COL 2

/*
 * ent_ptr holds the row/col # and address type of a cell
 *
 * vf is the type of cell address, 0 non-fixed, or bitwise OR of FIX_ROW or
 *    FIX_COL
 * vp : we just use vp->row or vp->col, vp may be a new cell just for holding
 *    row/col (say in gram.y) or a pointer to an existing cell
 */
struct ent_ptr {
    int vf;
    struct ent * vp;
};

// stores a range (left, right)
struct range {
    struct ent_ptr r_left, r_right;
    char * r_name;
    struct range * r_next, * r_prev;
    int r_is_range;
};

// holds the beginning/ending cells of a range
struct range_s {
    struct ent_ptr left, right;
};

/* stores type of operation this cell will perform */
struct enode {
    int op;
    union {

    struct {        /* other cells use to eval() / seval() */
        struct enode * left, * right;
        char *s;    /* previous value of @ext function in case */
    } o;            /* external functions are turned off */

    int gram_match; /* some compilers (hp9000ipc) need this */
    double k;       /* constant # */
    char * s;       /* string part of a cell */

    struct range_s r;    /* op is on a range */
    struct ent_ptr v;    /* ref. another cell on which this enode depends */
    } e;
};

struct impexfilt {
    char ext[PATHLEN];
    char plugin[PATHLEN];
    char type;
    struct impexfilt * next;
};

/* Use this structure to save the last 'g' command */
struct go_save {
    int g_type;
    double g_n;
    char * g_s;
    int g_row;
    int g_col;
    int g_lastrow;
    int g_lastcol;
    int strow;
    int stcol;
    int stflag;
    int errsearch;
};

/* op values */
#define O_VAR       'v'
#define O_CONST     'k'
#define O_ECONST    'E'      /* constant cell w/ an error */
#define O_SCONST    '$'
#define REDUCE       0200    /* Or'ed into OP if operand is a range */
#define OP_BASE      256
#define ACOS        (OP_BASE + 0)
#define ASIN        (OP_BASE + 1)
#define ATAN        (OP_BASE + 2)
#define CEIL        (OP_BASE + 3)
#define COS         (OP_BASE + 4)
#define EXP         (OP_BASE + 5)
#define FABS        (OP_BASE + 6)
#define FLOOR       (OP_BASE + 7)
#define HYPOT       (OP_BASE + 8)
#define LOG         (OP_BASE + 9)
#define LOG10       (OP_BASE + 10)
#define POW         (OP_BASE + 11)
#define SIN         (OP_BASE + 12)
#define SQRT        (OP_BASE + 13)
#define TAN         (OP_BASE + 14)
#define DTR         (OP_BASE + 15)
#define RTD         (OP_BASE + 16)
#define SUM         (OP_BASE + 17)
#define PROD        (OP_BASE + 18)
#define AVG         (OP_BASE + 19)
#define COUNT       (OP_BASE + 20)
#define STDDEV      (OP_BASE + 21)
#define MAX         (OP_BASE + 22)
#define MIN         (OP_BASE + 23)
#define RND         (OP_BASE + 24)
#define HOUR        (OP_BASE + 25)
#define MINUTE      (OP_BASE + 26)
#define SECOND      (OP_BASE + 27)
#define MONTH       (OP_BASE + 28)
#define DAY         (OP_BASE + 29)
#define YEAR        (OP_BASE + 30)
#define NOW         (OP_BASE + 31)
#define DATE        (OP_BASE + 32)
#define FMT         (OP_BASE + 33)
#define SUBSTR      (OP_BASE + 34)
#define STON        (OP_BASE + 35)
#define EQS         (OP_BASE + 36)
#define EXT         (OP_BASE + 37)
#define ELIST       (OP_BASE + 38)    /* List of expressions */
#define LMAX        (OP_BASE + 39)
#define LMIN        (OP_BASE + 40)
#define NVAL        (OP_BASE + 41)
#define SVAL        (OP_BASE + 42)
#define PV          (OP_BASE + 43)
#define FV          (OP_BASE + 44)
#define PMT         (OP_BASE + 45)
#define STINDEX     (OP_BASE + 46)
#define LOOKUP      (OP_BASE + 47)
#define ATAN2       (OP_BASE + 48)
#define INDEX       (OP_BASE + 49)
#define DTS         (OP_BASE + 50)
#define TTS         (OP_BASE + 51)
#define ABS         (OP_BASE + 52)
#define HLOOKUP     (OP_BASE + 53)
#define VLOOKUP     (OP_BASE + 54)
#define ROUND       (OP_BASE + 55)
#define IF          (OP_BASE + 56)
#define FILENAME    (OP_BASE + 57)
#define MYROW       (OP_BASE + 58)
#define MYCOL       (OP_BASE + 59)
#define LASTROW     (OP_BASE + 60)
#define LASTCOL     (OP_BASE + 61)
#define COLTOA      (OP_BASE + 62)
#define UPPER       (OP_BASE + 63)
#define LOWER       (OP_BASE + 64)
#define CAPITAL     (OP_BASE + 65)
#define NUMITER     (OP_BASE + 66)
#define ERR_        (OP_BASE + 67)
#define PI_         (OP_BASE + 68)
#define REF_        (OP_BASE + 69)
#define SLEN        (OP_BASE + 77)
#define ASCII       (OP_BASE + 78)
#define CHR         (OP_BASE + 79)
#define SET8BIT     (OP_BASE + 80)
#define REPLACE     (OP_BASE + 81)
#define FROW        (OP_BASE + 82)
#define FCOL        (OP_BASE + 83)
#define LUA         (OP_BASE + 84)

/* flag values */
#define is_valid      0001
#define is_changed    0002
#define is_strexpr    0004
#define is_leftflush  0010
#define is_deleted    0020
#define is_locked     0040
#define is_label      0100
#define iscleared     0200
#define may_sync      0400
/* cell error (1st generation (ERROR) or 2nd+ (INVALID)) */
#define CELLOK        0
#define CELLERROR     1
#define CELLINVALID   2
#define CELLREF       3
/* calculation order */
#define BYCOLS        1
#define BYROWS        2
/* tblprint style output for: */
#define TBL           1       /* 'tbl' */
#define LATEX         2       /* 'LaTeX' */
#define TEX           3       /* 'TeX' */
#define SLATEX        4       /* 'SLaTeX' (Scandinavian LaTeX) */
#define FRAME         5       /* tblprint style output for FrameMaker */
/* Types for etype() */
#define NUM           1
#define STR           2
#define GROWAMT       30      /* default minimum amount to grow */
#define GROWNEW       1       /**< first time table */
#define GROWROW       2       /* add rows */
#define GROWCOL       3       /* add columns */
#define GROWBOTH      4       /* grow both */

extern int currow, curcol;
extern int maxrow, maxcol;
extern struct ent *** tbl;     // data table ref. in vmtbl.c and ATBL()
extern char curfile[];
extern int arg;
extern int lastrow, lastcol;
extern int gmyrow, gmycol;    // globals used for @myrow, @mycol cmds
extern int rescol;            // columns reserved for row numbers
extern int maxrows, maxcols;  // # cells currently allocated
extern int rowsinrange;       // Number of rows in target range of a goto
extern int colsinrange;       // Number of cols in target range of a goto
extern int * fwidth;
extern int * precision;
extern int * realfmt;
extern char *colformat[10];
extern char *col_hidden;
extern char *row_hidden;
extern char line[FBUFLEN];
extern int linelim;
extern int changed;
extern int dbidx;
extern int qbuf;              // buffer no. specified by `"' command
extern int showsc, showsr;    // starting cell of highlighted range
extern int cellassign;
extern int macrofd;
extern int cslop;
extern int usecurses;
extern int brokenpipe;        // Set to true if SIGPIPE is received
extern int modflg;
extern char * mdir;
extern char * autorun;
extern int skipautorun;
extern char * fkey[FKEYS];
extern char * scext;
extern int repct;
extern int calc_order;
extern double prescale;
extern int propagation;
//extern int autocalc;
//extern int autolabel;
//extern int autoinsert;
//extern int autowrap;
//extern int showcell;
//extern int showtop;
//extern int craction;
extern int optimize;
extern int color;
extern int numeric;
extern int colorneg;
extern int colorerr;
extern int rndtoeven;
extern int tbl_style;
//extern int pagesize;    // If nonzero, use instead of 1/2 screen height
//extern int rowlimit;
//extern int collimit;
extern int loading;

extern struct enode *copye(register struct enode *e, int Rdelta, int Cdelta, int r1, int c1, int r2, int c2, int transpose);
extern char dpoint;    // country-dependent decimal point from locale
extern char thsep;    // country-dependent thousands separator from locale
extern char * coltoa(int col);
extern char * findplugin(char *ext, char type);
extern char * findhome(char *path);
extern char * r_name(int r1, int c1, int r2, int c2);
extern char * scxmalloc(unsigned n);
extern char * scxrealloc(char *ptr, unsigned n);
//extern char * seval(register struct enode * se);
extern char * seval(register struct ent * ent, register struct enode * se);
extern char * v_name(int row, int col);
//extern double eval(register struct enode *e);
extern double eval(register struct ent * ent, register struct enode * e);
extern struct enode *new(int op, struct enode *a1, struct enode *a2);
extern struct enode *new_const(int op, double a1);
extern struct enode *new_range(int op, struct range_s a1);
extern struct enode *new_str(char *s);
extern struct enode *new_var(int op, struct ent_ptr a1);
extern struct ent *lookat(int row, int col);
extern void EvalAll();
extern void checkbounds(int *rowp, int *colp);
extern void clearent(struct ent *v);
extern void closefile(FILE *f, int pid, int rfd);
extern void colshow_op();
extern struct colorpair *cpairs[8];
extern void editexp(int row, int col);
extern void efree(struct enode *e);
extern void label(register struct ent *v, register char *s, int flushdir);

extern double eval_result;
extern char * seval_result;
#endif // SC_H_
