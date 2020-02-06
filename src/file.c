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
 * \file file.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a brief file description.
 */

#include <pwd.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <sys/wait.h>

#ifndef NO_WORDEXP
#include <wordexp.h>
#endif

#include "conf.h"
#include "maps.h"
#include "yank.h"
#include "cmds.h"
#include "file.h"
#include "marks.h"
#include "lex.h"
#include "format.h"
#include "interp.h"
#include "utils/string.h"
#include "utils/dictionary.h"
#include "cmds_edit.h"
#include "xmalloc.h"
#include "y.tab.h"
#include "xlsx.h"
#include "xls.h"
#include "tui.h"

extern struct ent * freeents;
extern int yyparse(void);

#ifdef HAVE_PTHREAD
#include <pthread.h>
extern pthread_t fthread;
extern int pthread_exists;
#endif

/**
 * \brief Erase the database (tbl, etc.)
 *
 * \return none
 */

void erasedb() {
    int  r, c;

    for (c = 0; c <= maxcol; c++) {
        fwidth[c] = DEFWIDTH;
        precision[c] = DEFPREC;
        realfmt[c] = DEFREFMT;
    }

    for (r = 0; r <= maxrow; r++) {
        register struct ent ** pp = ATBL(tbl, r, 0);
        for (c = 0; c++ <= maxcol; pp++)
            if (*pp != NULL) {
                //(*pp)->next = freeents;    /* save [struct ent] for reuse */
                //freeents = *pp;

                clearent(*pp);
            }
    }

    for (c = 0; c < COLFORMATS; c++) {
        if (colformat[c] != NULL)
            scxfree(colformat[c]);
        colformat[c] = NULL;
    }

    maxrow = 0;
    maxcol = 0;

    clean_range();

    calc_order = BYROWS;
    prescale = 1.0;
    tbl_style = 0;
    optimize = 0;
    currow = curcol = 0;

    *curfile = '\0';
}

void loadrc(void) {
    char rcpath[PATHLEN];
    char * home;

    if ((home = getenv("HOME"))) {
        memset(rcpath, 0, sizeof(rcpath));
        strncpy(rcpath, home, sizeof(rcpath) - (sizeof("/.scimrc") + 1));
        strcat(rcpath, "/.scimrc");
        (void) readfile(rcpath, 0);
    }
    *curfile = '\0';
}

/**
 * \brief Check if a file exists
 *
 * \details Check if a file exists. Returns 1 if so. Returns 0 otherwise.
 *
 * \param[in] fname file name
 *
 * \return 1 if file exises; 0 otherwise
 */

int file_exists(const char * fname) {
    FILE * file;
    if ((file = fopen(fname, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

/**
 * \brief Check if file has been modified since last save.
 *
 * \details This function checks if a file suffered mods since it was open.
 *
 * \return 0 if not modified; 1 if modified
 */

int modcheck() {
    if (modflg && ! atoi(get_conf_value("nocurses"))) {
        sc_error("File not saved since last change. Add '!' to force");
        return(1);
    }
    return 0;
}

/**
 * \brief TODO Handle the save file process
 *
 * This funciton handles the save file process in SC-IM format..
 *
 * \return 0 on OK; -1 on error
 */

int savefile() {
    int force_rewrite = 0;
    char name[BUFFERSIZE];
    #ifndef NO_WORDEXP
    size_t len;
    wordexp_t p;
    #endif

    if (! curfile[0] && wcslen(inputline) < 3) { // casos ":w" ":w!" ":x" ":x!"
        sc_error("There is no filename");
        return -1;
    }

    if (inputline[1] == L'!') force_rewrite = 1;

    wcstombs(name, inputline, BUFFERSIZE);
    del_range_chars(name, 0, 1 + force_rewrite);

    #ifndef NO_WORDEXP
    wordexp(name, &p, 0);
    if (p.we_wordc < 1) {
        sc_error("Failed expanding filepath");
        return -1;
    }
    if ((len = strlen(p.we_wordv[0])) >= sizeof(name)) {
        sc_error("File path too long");
        wordfree(&p);
        return -1;
    }
    memcpy(name, p.we_wordv[0], len+1);
    wordfree(&p);
    #endif

    if (! force_rewrite && file_exists(name)) {
        sc_error("File already exists. Use \"!\" to force rewrite.");
        return -1;
    }

    #ifdef AUTOBACKUP
    // check if backup of curfile exists.
    // if it exists, remove it.
    if (strlen(curfile) && backup_exists(curfile)) remove_backup(curfile);

    // check if backup of newfilename exists.
    // if it exists and '!' is set, remove it.
    // if it exists and no '!' is set, return.
    if (!strlen(curfile) && backup_exists(name)) {
        if (!force_rewrite) {
            sc_error("Backup file of %s exists. Use \"!\" to force the write process.", name);
            return -1;
        } else remove_backup(name);
    }
    #endif

    // copy newfilename to curfile
    if (wcslen(inputline) > 2) {
        strcpy(curfile, name);
    }

    // add sc extension if not present
    if (wcslen(inputline) > 2 && str_in_str(curfile, ".") == -1) {
        sprintf(curfile + strlen(curfile), ".sc");


    // treat csv
    } else if (strlen(curfile) > 4 && (! strcasecmp( & curfile[strlen(curfile)-4], ".csv"))) {
        export_delim(curfile, ',', 0, 0, maxrow, maxcol, 1);
        modflg = 0;
        return 0;
    // treat tab
    } else if (strlen(curfile) > 4 && (! strcasecmp( & curfile[strlen(curfile)-4], ".tsv") ||
        ! strcasecmp( & curfile[strlen(curfile)-4], ".tab"))){
        export_delim(curfile, '\t', 0, 0, maxrow, maxcol, 1);
        modflg = 0;
        return 0;
    }
    // save in sc format
    if (writefile(curfile, 0, 0, maxrow, maxcol, 1) < 0) {
        sc_error("File could not be saved");
        return -1;
    }
    return 0;
}

/**
 * \brief Write a file
 *
 * \details Write a file. Receives parameter range and file name.
 *
 * \param[in] fname file name
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] verbose
 *
 * \return 0 on success; -1 on error
 */

int writefile(char * fname, int r0, int c0, int rn, int cn, int verbose) {
    register FILE *f;
    char save[PATHLEN];
    char tfname[PATHLEN];
    int pid;

    (void) strcpy(tfname, fname);

    (void) strcpy(save, tfname);

    if ((f = openfile(tfname, &pid, NULL)) == NULL) {
        if (verbose) sc_error("Can't create file \"%s\"", save);
        return -1;
    }

    if (verbose) sc_info("Writing file \"%s\"...", save);
    write_fd(f, r0, c0, rn, cn);

    closefile(f, pid, 0);

    if (! pid) {
        (void) strcpy(curfile, save);
        modflg = 0;
        if (verbose) sc_info("File \"%s\" written", curfile);
    }

    return 0;
}

/**
 * \brief TODO Document write_fd
 *
 * \param[in] f file pointer
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * 
 * \return none
 */

void write_fd(register FILE *f, int r0, int c0, int rn, int cn) {
    register struct ent **pp;
    int r, c;

    (void) fprintf(f, "# This data file was generated by the Spreadsheet Calculator.  That is SC-IM, the improved one!\n");
    (void) fprintf(f, "# You almost certainly shouldn't edit it.\n\n");
    print_options(f);
    for (c = 0; c < COLFORMATS; c++)
        if (colformat[c])
            (void) fprintf (f, "format %d = \"%s\"\n", c, colformat[c]);

    for (c = c0; c <= cn; c++)
        if (fwidth[c] != DEFWIDTH || precision[c] != DEFPREC || realfmt[c] != DEFREFMT)
            (void) fprintf (f, "format %s %d %d %d\n", coltoa(c), fwidth[c], precision[c], realfmt[c]);

    // new implementation of hidecol. group by ranges
    for (c = c0; c <= cn; c++) {
        int c_aux = c;
        if ( col_hidden[c] && c <= maxcol && ( c == 0 || !col_hidden[c-1] )) {
            while (c_aux <= maxcol && col_hidden[c_aux])
                c_aux++;
            fprintf(f, "hidecol %s", coltoa(c));
            if (c_aux-1 != c) {
                fprintf(f, ":%s\n", coltoa(c_aux-1));
                c = c_aux-1;
            } else
                fprintf(f, "\n");
        }
    }

    // new implementation of hiderow. group by ranges
    for (r = r0; r <= rn; r++) {
        int r_aux = r;
        if ( row_hidden[r] && r <= maxrow && ( r == 0 || !row_hidden[r-1] )) {
            while (r_aux <= maxrow && row_hidden[r_aux])
                r_aux++;
            fprintf(f, "hiderow %d", r);
            if (r_aux-1 != r) {
                fprintf(f, ":%d\n", r_aux-1);
                r = r_aux-1;
            } else
                fprintf(f, "\n");
        }
    }

    write_marks(f);
    write_franges(f);

    write_cells(f, r0, c0, rn, cn, r0, c0);

    for (r = r0; r <= rn; r++) {
        pp = ATBL(tbl, r, c0);
        for (c = c0; c <= cn; c++, pp++)
            if (*pp) {
                // Write ucolors
                if ((*pp)->ucolor != NULL) {

                    char strcolor[BUFFERSIZE];
                    strcolor[0] = 0;

                    char * gap_to_previous = "";
                    // decompile int value of color to its string description
                    if ((*pp)->ucolor->fg != NONE_COLOR && (*pp)->ucolor->fg != DEFAULT_COLOR) {
                        linelim=0;
                        struct enode * e = new((*pp)->ucolor->fg, (struct enode *)0, (struct enode *)0);
                        decompile(e, 0);
                        uppercase(line);
                        del_char(line, 0);
                        sprintf(strcolor, "fg=%.*s", BUFFERSIZE-4, &line[0]);
                        free(e);
                        gap_to_previous = " ";
                    }
                    if ((*pp)->ucolor->bg != NONE_COLOR && (*pp)->ucolor->bg != DEFAULT_COLOR) {
                        linelim=0;
                        struct enode * e = new((*pp)->ucolor->bg, (struct enode *)0, (struct enode *)0);
                        decompile(e, 0);
                        uppercase(line);
                        del_char(line, 0);
                        sprintf(strcolor + strlen(strcolor), "%sbg=%s", gap_to_previous, &line[0]);
                        free(e);
                        gap_to_previous = " ";
                    }

                    if ((*pp)->ucolor->bold)      sprintf(strcolor + strlen(strcolor), "%sbold=1", gap_to_previous);
                    if ((*pp)->ucolor->dim)       sprintf(strcolor + strlen(strcolor), "%sdim=1", gap_to_previous);
                    if ((*pp)->ucolor->reverse)   sprintf(strcolor + strlen(strcolor), "%sreverse=1", gap_to_previous);
                    if ((*pp)->ucolor->standout)  sprintf(strcolor + strlen(strcolor), "%sstandout=1", gap_to_previous);
                    if ((*pp)->ucolor->underline) sprintf(strcolor + strlen(strcolor), "%sunderline=1", gap_to_previous);
                    if ((*pp)->ucolor->blink)     sprintf(strcolor + strlen(strcolor), "%sblink=1", gap_to_previous);

                    // previous implementation
                    //(void) fprintf(f, "cellcolor %s%d \"%s\"\n", coltoa((*pp)->col), (*pp)->row, strcolor);

                    // new implementation
                    // by row, store cellcolors grouped by ranges
                    int c_aux = c;
                    struct ucolor * u = (*pp)->ucolor;
                    struct ucolor * a = NULL;
                    if ( c > 0 && *ATBL(tbl, r, c-1) != NULL)
                         a = (*ATBL(tbl, r, c-1))->ucolor;

                    if ( (u != NULL) && (c <= maxcol) && ( c == 0 || ( a == NULL ) || ( a != NULL && ! same_ucolor( a, u ) ))) {
                        while (c_aux <= maxcol && *ATBL(tbl, r, c_aux) != NULL && same_ucolor( (*ATBL(tbl, r, c_aux))->ucolor, (*pp)->ucolor ))
                            c_aux++;
                        fprintf(f, "cellcolor %s%d", coltoa((*pp)->col), (*pp)->row);
                        if (c_aux-1 != (*pp)->col)
                            fprintf(f, ":%s%d \"%s\"\n", coltoa(c_aux-1), (*pp)->row, strcolor);
                        else
                            fprintf(f, " \"%s\"\n", strcolor);
                    }

                }


                /* if ((*pp)->nrow >= 0) {
                    (void) fprintf(f, "addnote %s ", v_name((*pp)->row, (*pp)->col));
                    (void) fprintf(f, "%s\n", r_name((*pp)->nrow, (*pp)->ncol, (*pp)->nlastrow, (*pp)->nlastcol));
                } */

                // padding
                // previous implementation
                //if ((*pp)->pad)
                //    (void) fprintf(f, "pad %d %s%d\n", (*pp)->pad, coltoa((*pp)->col), (*pp)->row);
                // new implementation
                int r_aux = r;
                if ( (*pp)->pad  && r <= maxrow && ( r == 0 || (*ATBL(tbl, r-1, c) == NULL) ||
                    (*ATBL(tbl, r-1, c) != NULL && ((*ATBL(tbl, r-1, c))->pad != (*pp)->pad)) )) {
                    while (r_aux <= maxrow && *ATBL(tbl, r_aux, c) != NULL && (*pp)->pad == (*ATBL(tbl, r_aux, c))->pad )
                        r_aux++;
                    fprintf(f, "pad %d %s%d", (*pp)->pad, coltoa((*pp)->col), (*pp)->row);
                    if (r_aux-1 != (*pp)->row)
                        fprintf(f, ":%s%d\n", coltoa((*pp)->col), r_aux-1);
                    else
                        fprintf(f, "\n");
                }




            }
    }

    // write locked cells
    // lock should be stored after any other command
    for (r = r0; r <= rn; r++) {
        pp = ATBL(tbl, r, c0);
        for (c = c0; c <= cn; c++, pp++)
            if (*pp) {
                // previous implementation
                //if ((*pp)->flags & is_locked)
                //    (void) fprintf(f, "lock %s%d\n", coltoa((*pp)->col), (*pp)->row);
                // new implementation
                int c_aux = c;
                if ( (*pp)->flags & is_locked && c <= maxcol && ( c == 0 || ( *ATBL(tbl, r, c-1) != NULL && ! ((*ATBL(tbl, r, c-1))->flags & is_locked) ) )) {
                    while (c_aux <= maxcol && *ATBL(tbl, r, c_aux) != NULL && (*ATBL(tbl, r, c_aux))->flags & is_locked )
                        c_aux++;
                    fprintf(f, "lock %s%d", coltoa((*pp)->col), (*pp)->row);
                    if (c_aux-1 != (*pp)->col)
                        fprintf(f, ":%s%d\n", coltoa(c_aux-1), (*pp)->row);
                    else
                        fprintf(f, "\n");
                }
            }
    }

    /*
     * Don't try to combine these into a single fprintf().  v_name() has
     * a single buffer that is overwritten on each call, so the first part
     * needs to be written to the file before making the second call.
     */
    fprintf(f, "goto %s", v_name(currow, curcol));
    //fprintf(f, " %s\n", v_name(strow, stcol));
    fprintf(f, "\n");
}

/**
 * \brief TODO Document write_franges()
 *
 * \param[in] f file pointer
 * 
 * \return none
 */

void write_franges(register FILE *f) {
    if (! freeze_ranges) return;
    if (freeze_ranges->type == 'a') {
        fprintf(f, "freeze %s%d", coltoa(freeze_ranges->tl->col), freeze_ranges->tl->row);
        fprintf(f, ":%s%d\n", coltoa(freeze_ranges->br->col), freeze_ranges->br->row);
    } else if (freeze_ranges->type == 'c' && freeze_ranges->tl->col == freeze_ranges->br->col) {
        fprintf(f, "freeze %s\n", coltoa(freeze_ranges->tl->col));
    } else if (freeze_ranges->type == 'c') {
        fprintf(f, "freeze %s:", coltoa(freeze_ranges->tl->col));
        fprintf(f, "%s\n", coltoa(freeze_ranges->br->col));
    } else if (freeze_ranges->type == 'r' && freeze_ranges->tl->row == freeze_ranges->br->row) {
        fprintf(f, "freeze %d\n", freeze_ranges->tl->row);
    } else if (freeze_ranges->type == 'r') {
        fprintf(f, "freeze %d:%d\n", freeze_ranges->tl->row, freeze_ranges->br->row);
    }
}

/**
 * \brief TODO Document write_marks()
 *
 * \param[in] f file pointer
 *
 * \return none
 */

void write_marks(register FILE *f) {
    int i;
    struct mark * m;

    for ( i='a'; i<='z'; i++ ) {
        m = get_mark((char) i);

        // m->rng should never be NULL if both m->col and m->row are -1 !!
        if ( m->row == -1 && m->col == -1) { // && m->rng != NULL ) {  
            fprintf(f, "mark %c %s%d ", i, coltoa(m->rng->tlcol), m->rng->tlrow);
            fprintf(f, "%s%d\n", coltoa(m->rng->brcol), m->rng->brrow);
        } else if ( m->row != 0 && m->row != 0) { // && m->rng == NULL) {
            fprintf(f, "mark %c %s%d\n", i, coltoa(m->col), m->row);
        }
    }

    return;
}

/**
 * \brief TODO Document write_cells()
 *
 * \param[in] f file pointer
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] dr
 * \param[in[ dc
 *
 * \return none
 */

void write_cells(register FILE *f, int r0, int c0, int rn, int cn, int dr, int dc) {
    register struct ent **pp;
    int r, c;
    //int r, c, mf;
    char *dpointptr;

    //mf = modflg;
    if (dr != r0 || dc != c0) {
        //yank_area(r0, c0, rn, cn);
        rn += dr - r0;
        cn += dc - c0;
        //rs = currow;
        //cs = curcol;
        currow = dr;
        curcol = dc;
    }
    //if (Vopt) valueize_area(dr, dc, rn, cn);
    for (r = dr; r <= rn; r++) {
        pp = ATBL(tbl, r, dc);
        for (c = dc; c <= cn; c++, pp++)
            if (*pp) {
                if ((*pp)->label || (*pp)->flags & is_strexpr) {
                    edits(r, c, 1);
                    (void) fprintf(f, "%s\n", line);
                }
                if ((*pp)->flags & is_valid || (*pp)->expr) {
                    editv(r, c);
                    dpointptr = strchr(line, dpoint);
                    if (dpointptr != NULL)
                        *dpointptr = '.';
                    (void) fprintf(f, "%s\n", line);
                }
                if ((*pp)->format) {
                    editfmt(r, c);
                    (void) fprintf(f, "%s\n",line);
                }
            }
    }
    //modflg = mf;
}

/**
 * \brief Try to open a spreadsheet file.
 *
 * \param[in] fname file name
 * \param[in] eraseflg
 *
 * \return SC_READFILE_SUCCESS if we loaded the file, SC_READFILE_ERROR if we failed,
 * SC_READFILE_DOESNTEXIST if the file doesn't exist.
 */

sc_readfile_result readfile(char * fname, int eraseflg) {
    if (!strlen(fname)) return 0;
    loading = 1;

#ifdef AUTOBACKUP
    // Check if curfile is set and backup exists..
    if (str_in_str(fname, ".scimrc") == -1 && strlen(curfile) &&
    backup_exists(curfile) && strcmp(fname, curfile)) {
        if (modflg) {
            // TODO - force load with '!' ??
            sc_error("There are changes unsaved. Cannot load file: %s", fname);
            loading = 0;
            return SC_READFILE_ERROR;
        }
        remove_backup(curfile);
    }
    // Check if fname is set and backup exists..
    if (backup_exists(fname)) {
        wchar_t msg[BUFFERSIZE];
        swprintf(msg, BUFFERSIZE,
        // TODO - Open backup readonly ??
        L"Backup of %s file exists. Do you want to (E)dit the file and remove the backup, (R)ecover the backup or (Q)uit: ", fname);
        wchar_t t = ui_query_opt(msg, L"qerQER");
        switch (t) {
            case L'q':
            case L'Q':
                loading = 0;
                extern int shall_quit;
                shall_quit = 1;
                return SC_READFILE_ERROR;
                break;
            case L'e':
            case L'E':
                remove_backup(fname);
                break;
            case L'r':
            case L'R':
                ;
                int len = strlen(fname);
                if (!len) return 0;
                char * pstr = strrchr(fname, '/');
                int pos = pstr == NULL ? -1 : pstr - fname;
                char bkpname[len+6];
                strcpy(bkpname, fname);
                add_char(bkpname, '.', pos+1);
                sprintf(bkpname + strlen(bkpname), ".bak");
                remove(fname);
                rename(bkpname, fname);
                break;
        }
    }
#endif

    // Check if file is a correct format
    int len = strlen(fname);
    if (! strcmp( & fname[len-3], ".sc") ||
        (len > 6 && ! strcasecmp( & fname[len-7], ".scimrc"))) {
        // pass

    // If file is an xlsx file, we import it
    } else if (len > 5 && ! strcasecmp( & fname[len-5], ".xlsx")){
        #ifndef XLSX
        sc_error("XLSX import support not compiled in");
        #else
        open_xlsx(fname, "UTF-8");
        strcpy(curfile, fname);
        modflg = 0;
        #endif
        loading = 0;
        return SC_READFILE_SUCCESS;

    // If file is an xls file, we import it
    } else if (len > 4 && ! strcasecmp( & fname[len-4], ".xls")){
        #ifndef XLS
        sc_error("XLS import support not compiled in");
        #else
        open_xls(fname, "UTF-8");
        modflg = 0;
        strcpy(curfile, fname);
        #endif
        loading = 0;
        return SC_READFILE_SUCCESS;

    // If file is an delimited text file, we import it
    } else if (len > 4 && ( ! strcasecmp( & fname[len-4], ".csv") ||
        ! strcasecmp( & fname[len-4], ".tsv") || ! strcasecmp( & fname[len-4], ".tab") ||
        ! strcasecmp( & fname[len-4], ".txt") )){

        char delim = ',';
        if ( ! strcasecmp( & fname[len-4], ".tsv") ||  ! strcasecmp( & fname[len-4], ".tab"))
            delim = '\t';

        if (get_conf_value("txtdelim") != NULL) {
            if (! strcasecmp(get_conf_value("txtdelim"), "\\t")) {
                delim = '\t';
            } else if (! strcasecmp(get_conf_value("txtdelim"), ",")) {
                delim = ',';
            } else if (! strcasecmp(get_conf_value("txtdelim"), ";")) {
                delim = ';';
            }
        }
        import_csv(fname, delim); // csv tsv tab txt delim import
        strcpy(curfile, fname);
        modflg = 0;
        loading = 0;
        return SC_READFILE_SUCCESS;

    } else {
        sc_info("\"%s\" is not a SC-IM compatible file", fname);
        loading = 0;
        return SC_READFILE_ERROR;
    }

    // We open an 'sc' format file
    // open fname for reading
    register FILE * f;
    char save[PATHLEN];
    if (*fname == '\0') fname = curfile;
    (void) strcpy(save, fname);
    f = fopen(save, "r");
    if (f == NULL) {
        loading = 0;
        strcpy(curfile, save);
        return SC_READFILE_DOESNTEXIST;
    } /* */

    if (eraseflg) erasedb();

    while (! brokenpipe && fgets(line, sizeof(line), f)) {
        linelim = 0;
        if (line[0] != '#') (void) yyparse();
    }
    fclose(f);

    loading = 0;
    linelim = -1;
    //EvalAll(); // FIXME: see why this double EvalAll is neccesary to remedy #193
    if (eraseflg) {
        //(void) strcpy(curfile, save);
        cellassign = 0;
    }
    strcpy(curfile, save);
    EvalAll();
    modflg = 0;
    return SC_READFILE_SUCCESS;
}

/**
 * \brief Expand a ~ in path to the user's home directory
 *
 * \param[in] path
 *
 * \return path
 */

char * findhome(char * path) {
    static char * HomeDir = NULL;

    if (* path == '~') {
        char * pathptr;
        char tmppath[PATHLEN];

        if (HomeDir == NULL) {
            HomeDir = getenv("HOME");
            if (HomeDir == NULL)
                HomeDir = "/";
        }
        pathptr = path + 1;
        if ((* pathptr == '/') || (* pathptr == '\0'))
            strcpy(tmppath, HomeDir);
        else {
            struct passwd * pwent;
            char * namep;
            char name[50];

            namep = name;
            while ((*pathptr != '\0') && (*pathptr != '/'))
                *(namep++) = *(pathptr++);
            *namep = '\0';
            if ((pwent = getpwnam(name)) == NULL) {
                (void) sprintf(path, "Can't find user %s", name);
                return (NULL);
            }
            strcpy(tmppath, pwent->pw_dir);
        }
        strcat(tmppath, pathptr);
        strcpy(path, tmppath);
    }
    return (path);
}

/**
 * \brief Open the input or output file
 *
 * \details Open the input or output file, setting up a pipe if needed.
 *
 * \param[in] fname file name
 * \param[in] rpid
 * \param[in] rfd
 *
 * \return file pointer
 */

FILE * openfile(char *fname, int *rpid, int *rfd) {
    int pipefd[4];
    int pid;
    FILE *f;
    char *efname;

    while (*fname && (*fname == ' '))    // Skip leading blanks
        fname++;

    if (*fname != '|') {                 // Open file if not pipe
        *rpid = 0;
        if (rfd != NULL)
            *rfd = 1;                    // Set to stdout just in case

        efname = findhome(fname);
        return (fopen(efname, rfd == NULL ? "w" : "r"));
    }

    fname++;                             // Skip |
    efname = findhome(fname);
    if (pipe(pipefd) < 0 || (rfd != NULL && pipe(pipefd+2) < 0)) {
        sc_error("Can't make pipe to child");
        *rpid = 0;
        return (0);
    }

    //deraw(rfd==NULL);

    if ((pid=fork()) == 0) {             // if child
        (void) close(0);                 // close stdin
        (void) close(pipefd[1]);
        (void) dup(pipefd[0]);           // connect to first pipe
        if (rfd != NULL) {               // if opening for read
            (void) close(1);             // close stdout
            (void) close(pipefd[2]);
            (void) dup(pipefd[3]);       // connect to second pipe
        }
        (void) signal(SIGINT, SIG_DFL);  // reset
        execl("/bin/sh", "sh", "-c", efname, 0, (char *) NULL);
        exit (-127);
    } else {                             // else parent
        *rpid = pid;
        if ((f = fdopen(pipefd[(rfd==NULL?1:2)], rfd==NULL?"w":"r")) == NULL) {
            (void) kill(pid, 9);
            sc_error("Can't fdopen %sput", rfd==NULL?"out":"in");
            (void) close(pipefd[1]);
            if (rfd != NULL)
                (void) close(pipefd[3]);
            *rpid = 0;
            return (0);
        }
    }
    (void) close(pipefd[0]);
    if (rfd != NULL) {
        (void) close(pipefd[3]);
        *rfd = pipefd[1];
    }
    return (f);
}

// close a file opened by openfile(), if process wait for return
/**
 * \brief Close a file opened by openfile()
 *
 * \details Close a file opened by openfile(). If process, wait for return
 *
 * \param[in] f file pointer
 * \param[in] pid
 * \param[in] rfd
 *
 * \return none
 */

void closefile(FILE *f, int pid, int rfd) {
    int temp;
    wint_t wi;

    (void) fclose(f);
    if (pid) {
        while (pid != wait(&temp)) //;
        if (rfd==0) {
            printf("Press any key to continue ");
            fflush(stdout);
#ifdef NCURSES
            cbreak();
#endif
            ui_getch_b(&wi);
        } else {
            close(rfd);
#ifdef NCURSES
            if (! atoi(get_conf_value("nocurses"))) {
                cbreak();
                nonl();
                noecho ();
            }
#endif
        }
    }
    if (brokenpipe) {
        sc_error("Broken pipe");
        brokenpipe = FALSE;
    }
}

/**
 * \brief TODO <brief function description>
 *
 * \param[in] f file pointer
 *
 * \return none
 */

void print_options(FILE *f) {
    if (
        ! optimize &&
        ! rndtoeven &&
        calc_order == BYROWS &&
        prescale == 1.0 &&
        ! atoi(get_conf_value("external_functions")) &&
        tbl_style == 0
       )
    return; // No reason to do this

    (void) fprintf(f, "set");
    if (optimize)              (void) fprintf(f," optimize");
    if (rndtoeven)             (void) fprintf(f, " rndtoeven");
    if (calc_order != BYROWS ) (void) fprintf(f, " bycols");
    if (prescale != 1.0)       (void) fprintf(f, " prescale");
    if ( atoi(get_conf_value("external_functions")) ) (void) fprintf(f, " external_functions");
    if (tbl_style)             (void) fprintf(f, " tblstyle = %s", tbl_style == TBL ? "tbl" : tbl_style == LATEX ? "latex" : tbl_style == SLATEX ? "slatex" : tbl_style == TEX ? "tex" : tbl_style == FRAME ? "frame" : "0" );
    (void) fprintf(f, "\n");
}


/**
 * \brief Import csv to sc
 *
 * \param[in] fname file name
 * \param[in] d
 *
 * \return 0 on success; -1 on error
 */

int import_csv(char * fname, char d) {
    register FILE * f;
    int r = 0, c = 0, cf = 0;
    wchar_t line_interp[FBUFLEN] = L"";
    char * token;

    int quote = 0; // if value has '"'. ex: 12,"1234,450.00",56
    char delim[2] = ""; //strtok receives a char *, not a char
    add_char(delim, d, 0);

    if ((f = fopen(fname , "r")) == NULL) {
        sc_error("Can't read file \"%s\"", fname);
        return -1;
    }

    // Check max length of line
    int max = max_length(f) + 1;
    if (max == 0) {
        sc_error("Can't read file \"%s\"", fname);
        return -1;
    }
    char line_in[max];
    rewind(f);

    // CSV file traversing
    while ( ! feof(f) && (fgets(line_in, sizeof(line_in), f) != NULL) ) {
        // this hack is for importing file that have DOS eol
        int l = strlen(line_in);
        while (l--)
            if (line_in[l] == 0x0d) {
                line_in[l] = '\0';
                break;
            }

        // Split string using the delimiter
        token = xstrtok(line_in, delim);
        c = 0;

        while( token != NULL ) {
            if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
            clean_carrier(token);
            if ( token[0] == '\"' && token[strlen(token)-1] == '\"') {
                quote = 1;
            } else if ( (token[0] == '\"' || quote) && strlen(token) && (token[strlen(token)-1] != '\"' || strlen(token) == 1) ) {
                quote = 1;
                char * next = xstrtok(NULL, delim);

                if (next != NULL) {
                    sprintf(token + strlen(token), "%c%s", d, next);
                    continue;
                }
            }
            if (quote) { // Remove quotes
                del_char(token, 0);
                del_char(token, strlen(token)-1);
            }
            char * st = str_replace (token, "\"", "''"); //replace double quotes inside string

            // number import
            if (isnumeric(st) && strlen(st) && ! atoi(get_conf_value("import_delimited_as_text"))
            ) {
                //wide char
                swprintf(line_interp, BUFFERSIZE, L"let %s%d=%s", coltoa(c), r, st);

            // text import
            } else if (strlen(st)){
                //wide char
                swprintf(line_interp, BUFFERSIZE, L"label %s%d=\"%s\"", coltoa(c), r, st);
            }
            //wide char
            if (strlen(st)) send_to_interp(line_interp);

            if (++c > cf) cf = c;
            quote = 0;
            token = xstrtok(NULL, delim);
            free(st);
        }

        r++;
        if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
    }
    maxrow = r-1;
    maxcol = cf-1;

    auto_justify(0, maxcols, DEFWIDTH);

    fclose(f);

    EvalAll();
    return 0;
}

/**
 * \brief Export to CSV, TAB, or plain TXT
 *
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 *
 * \return none
 */

void do_export(int r0, int c0, int rn, int cn) {
    int force_rewrite = 0;
    char type_export[4] = "";
    char ruta[PATHLEN];
    char linea[BUFFERSIZE];

    if (inputline[1] == L'!') force_rewrite = 1;
    wcstombs(linea, inputline, BUFFERSIZE); // Use new variable to keep command history untouched
    del_range_chars(linea, 0, 1 + force_rewrite); // Remove 'e' or 'e!' from inputline

    // Get format to export
    if (str_in_str(linea, "csv") == 0) {
        strcpy(type_export, "csv");
    } else if (str_in_str(linea, "tab") == 0) {
        strcpy(type_export, "tab");
    } else if (str_in_str(linea, "txt") == 0) {
        strcpy(type_export, "txt");
    } else if (str_in_str(linea, "mkd") == 0) {
        strcpy(type_export, "mkd");
    }

    // Get route and file name to write.
    // Use parameter if any.
    if (strlen(linea) > 4) {   // ex. 'csv '
        del_range_chars(linea, 0, 3); // remove 'csv'
        strcpy(ruta, linea);

    // Use curfile name and '.csv' o '.tab' extension
    // Remove current '.sc' extension if necessary
    } else if (curfile[0]) {
        strcpy(ruta, curfile);
        char * ext = strrchr(ruta, '.');
        if (ext != NULL) del_range_chars(ruta, strlen(ruta) - strlen(ext), strlen(ruta)-1);
        sprintf(ruta + strlen(ruta), ".%s", type_export);

    } else {
        sc_error("No filename specified !");
        return;
    }

    if (! force_rewrite && file_exists(ruta) && strlen(ruta) > 0) {
        sc_error("File %s already exists. Use \"!\" to force rewrite.", ruta);
        return;
    }

    #ifdef AUTOBACKUP
    // check if backup of fname exists.
    // if it exists and '!' is set, remove it.
    // if it exists and curfile = fname, remove it.
    // else return.
    if (( !strcmp(type_export, "csv") || !strcmp(type_export, "tab")) && (strlen(ruta) && backup_exists(ruta))) {
        if (force_rewrite || (strlen(curfile) && !strcmp(curfile, ruta))) {
            remove_backup(ruta);
        } else {
            sc_error("Backup file of %s exists. Use \"!\" to force the write process.", ruta);
            return;
        }
    }
    #endif

    // Call export routines
    if (strcmp(type_export, "csv") == 0) {
        export_delim(ruta, ',', r0, c0, rn, cn, 1);
    } else if (strcmp(type_export, "tab") == 0) {
        export_delim(ruta, '\t', r0, c0, rn, cn, 1);
    } else if (strcmp(type_export, "txt") == 0) {
        export_plain(ruta, r0, c0, rn, cn);
    } else if (strcmp(type_export, "mkd") == 0) {
        export_markdown(ruta, r0, c0, rn, cn);
    }
}


/**
 * \brief Export to md file with markdown table
 *
 * \param[in] fname file name
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 *
 * \return none
 */

void export_markdown(char * fname, int r0, int c0, int rn, int cn) {
    FILE * f;
    int row, col;
    register struct ent ** pp;
    int pid;
    wchar_t out[FBUFLEN] = L"";

    if (fname == NULL)
        f = stdout;
    else {
        sc_info("Writing file \"%s\"...", fname);
        if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
            sc_error ("Can't create file \"%s\"", fname);
            return;
        }
    }

    struct ent * ent = go_end();
    if (rn > ent->row) rn = ent->row;

    char num [FBUFLEN] = "";
    char text[FBUFLEN] = "";
    char formated_s[FBUFLEN] = "";
    char dashline[FBUFLEN] = "";
    int res = -1;
    int align = 1;
    int dash_num;

    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;
            if (*pp) {

                num [0] = '\0';
                text[0] = '\0';
                out [0] = L'\0';
                formated_s[0] = '\0';
                res = -1;
                align = 1;

                if (col == 0) {
                  (void) fprintf (f, "| ");
                } else if (col <= cn) {
                  (void) fprintf (f, " | ");
                }

                // If a numeric value exists
                if ( (*pp)->flags & is_valid) {
                    res = ui_get_formated_value(pp, col, formated_s);
                    // res = 0, indicates that in num we store a date
                    // res = 1, indicates a format is applied in num
                    if (res == 0 || res == 1) {
                        strcpy(num, formated_s);
                    } else if (res == -1) {
                        sprintf(num, "%.*f", precision[col], (*pp)->v);
                    }
                }

                // If a string exists
                if ((*pp)->label) {
                    strcpy(text, (*pp)->label);
                    align = 1;                                // right alignment
                    if ((*pp)->flags & is_label) {            // center alignment
                        align = 0;
                    } else if ((*pp)->flags & is_leftflush) { // left alignment
                        align = -1;
                    } else if (res == 0) {                    // res must ¿NOT? be zero for label to be printed
                        text[0] = '\0';
                    }
                }

                //make header border of dashes with alignment characters
                if (row == 0) {
                  if (col == 0) strcat (dashline, "|");
                  if(align == 0){
                    strcat (dashline, ":");
                  }
                  else {
                    strcat (dashline, "-");
                  }
                  for (dash_num = 0; dash_num < fwidth[col]; dash_num++) {
                    strcat (dashline, "-");
                  }
                  if(align >= 0){
                    strcat (dashline, ":");
                  }
                  else {
                    strcat (dashline, "-");
                  }
                  strcat (dashline, "|");
                }

                pad_and_align (text, num, fwidth[col], align, 0, out);
                (void) fprintf (f, "%ls", out);

            } else {
              (void) fprintf (f, "%*s", fwidth[col], " ");
            }
        }

        (void) fprintf(f," |\n");

        if (row == 0) (void) fprintf(f,"%s\n",dashline);
    }
    closefile(f, pid, 0);

    if (! pid) {
        sc_info("File \"%s\" written", fname);
    }

}
/**
 * \brief Export to plain TXT
 *
 * \param[in] fname file name
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * 
 * \return none
 */

void export_plain(char * fname, int r0, int c0, int rn, int cn) {
    FILE * f;
    int row, col;
    register struct ent ** pp;
    int pid;
    wchar_t out[FBUFLEN] = L"";

    if (fname == NULL)
        f = stdout;
    else {
        sc_info("Writing file \"%s\"...", fname);
        if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
            sc_error ("Can't create file \"%s\"", fname);
            return;
        }
    }

    struct ent * ent = go_end();
    if (rn > ent->row) rn = ent->row;

    char num [FBUFLEN] = "";
    char text[FBUFLEN] = "";
    char formated_s[FBUFLEN] = "";
    int res = -1;
    int align = 1;

    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;

            if (*pp) {

                num [0] = '\0';
                text[0] = '\0';
                out [0] = L'\0';
                formated_s[0] = '\0';
                res = -1;
                align = 1;

                // If a numeric value exists
                if ( (*pp)->flags & is_valid) {
                    res = ui_get_formated_value(pp, col, formated_s);
                    // res = 0, indicates that in num we store a date
                    // res = 1, indicates a format is applied in num
                    if (res == 0 || res == 1) {
                        strcpy(num, formated_s);
                    } else if (res == -1) {
                        sprintf(num, "%.*f", precision[col], (*pp)->v);
                    }
                }

                // If a string exists
                if ((*pp)->label) {
                    strcpy(text, (*pp)->label);
                    align = 1;                                // right alignment
                    if ((*pp)->flags & is_label) {            // center alignment
                        align = 0;
                    } else if ((*pp)->flags & is_leftflush) { // left alignment
                        align = -1;
                    } else if (res == 0) {                    // res must ¿NOT? be zero for label to be printed
                        text[0] = '\0';
                    }
                }

                pad_and_align (text, num, fwidth[col], align, 0, out);
                (void) fprintf (f, "%ls", out);

            } else {
                (void) fprintf (f, "%*s", fwidth[col], " ");
            }
        }
        (void) fprintf(f,"\n");
    }
    if (fname != NULL) {
        closefile(f, pid, 0);
        if (! pid) {
            sc_info("File \"%s\" written", fname);
        }
    }

}

// fname is the path and name of file
/**
 * \brief TODO Document export_delim
 *
 * \param[in] fname full path of the file
 * \param[in] coldelim
 * \param[in] r0
 * \param[in] c0
 * \param[in] rn
 * \param[in] cn
 * \param[in] verbose
 *
 * \return none
 */

void export_delim(char * fname, char coldelim, int r0, int c0, int rn, int cn, int verbose) {
    FILE * f;
    int row, col;
    register struct ent ** pp;
    int pid;


    if (verbose) sc_info("Writing file \"%s\"...", fname);

    if (fname == NULL)
        f = stdout;
    else {
        if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
            if (verbose) sc_error ("Can't create file \"%s\"", fname);
            return;
        }
    }

    struct ent * ent = go_end();
    if (rn > ent->row) rn = ent->row;

    for (row = r0; row <= rn; row++) {
        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            if (*pp) {
                char * s;
                if ((*pp)->flags & is_valid) {
                    if ((*pp)->cellerror) {
                        (void) fprintf (f, "%*s", fwidth[col], ((*pp)->cellerror == CELLERROR ? "ERROR" : "INVALID"));
                    } else if ((*pp)->format) {
                        char field[FBUFLEN];
                        if (*((*pp)->format) == 'd') {  // Date format
                            time_t v = (time_t) ((*pp)->v);
                            strftime(field, sizeof(field), ((*pp)->format)+1, localtime(&v));
                        } else {                        // Numeric format
                            format((*pp)->format, precision[col], (*pp)->v, field, sizeof(field));
                        }
                        ltrim(field, ' ');
                        unspecial(f, field, coldelim);
                    } else { //eng number format
                        char field[FBUFLEN] = "";
                        (void) engformat(realfmt[col], fwidth[col], precision[col], (*pp)->v, field, sizeof(field));
                        ltrim(field, ' ');
                        unspecial(f, field, coldelim);
                    }
                }
                if ((s = (*pp)->label)) {
                    ltrim(s, ' ');
                    unspecial(f, s, coldelim);
                }
            }
            if (col < cn)
                (void) fprintf(f,"%c", coldelim);
        }
        (void) fprintf(f,"\n");
    }
    if (fname != NULL)
        closefile(f, pid, 0);

    if (! pid && verbose) {
        sc_info("File \"%s\" written", fname);
    }
}

/**
 * \brief TODO Document unspecial()
 *
 * \details Unxpecial (backquotes - > ") things that are special
 * chars in a table
 *
 * \param[in] f file pointer
 * \param[in] srt string pointer
 * \param[in] delim
 *
 * \return none
 */

void unspecial(FILE * f, char * str, int delim) {
    int backquote = 0;

    if (str_in_str(str, ",") != -1) backquote = 1;
    if (backquote) putc('\"', f);
    while (*str) {
        putc(*str, f);
        str++;
    }
    if (backquote) putc('\"', f);
}

/**
 * \brief Check the mas length of lines in a file
 *
 * \details Check masimum length of lines in a file. Note: 
 * FILE * f shall be opened.
 *
 * \param[in] f file pointer
 * \return file length + 1
 */

int max_length(FILE * f) {
    if (f == NULL) return -1;
    int count = 0, max = 0;
    int c = fgetc(f);

    while (c != EOF) {
        if (c != '\n') {
            count++;
        } else if (count > max) {
            max = count;
            count = 0;
        } else {
            count = 0;
        }
        c = fgetc(f);
    }
    return max + 1;
}

/**
 * \brief TODO Document plugin_exists()
 *
 * \param[in] name
 * \param[in] len
 * \param[in] path
 *
 * \return none
 */

int plugin_exists(char * name, int len, char * path) {
    FILE * fp;
    static char * HomeDir;
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        strcpy((char *) path, cwd);
        strcat((char *) path, "/");
        strncat((char *) path, name, len);
        if ((fp = fopen((char *) path, "r"))) {
            fclose(fp);
            return 1;
        }
    }
    if ((HomeDir = getenv("HOME"))) {
        strcpy((char *) path, HomeDir);
        strcat((char *) path, "/.scim/");
        strncat((char *) path, name, len);
        if ((fp = fopen((char *) path, "r"))) {
            fclose(fp);
            return 1;
        }
    }
    strcpy((char *) path, HELP_PATH);
    strcat((char *) path, "/");
    strncat((char *) path, name, len);
    if ((fp = fopen((char *) path, "r"))) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/**
 * \brief TODO Document do_autobackup()
 *
 * \return none
 */

void * do_autobackup() {
    int len = strlen(curfile);
    //if (loading || ! len) return (void *) -1;
    //if (! len || ! modflg) return (void *) -1;
    if (! len) return (void *) -1;

    char * pstr = strrchr(curfile, '/');
    int pos = pstr == NULL ? -1 : pstr - curfile;
    char name[PATHLEN] = {'\0'};
    char namenew[PATHLEN] = {'\0'};
    strcpy(name, curfile);
    add_char(name, '.', pos+1);
    sprintf(name + strlen(name), ".bak");
    sprintf(namenew, "%.*s.new", PATHLEN-5, name);
    //if (atoi(get_conf_value("debug"))) sc_info("doing autobackup of file:%s", name);

    // create new version
    if (! strcmp(&name[strlen(name)-7], ".sc.bak")) {
        register FILE * f;
        if ((f = fopen(namenew , "w")) == NULL) return (void *) -1;
        write_fd(f, 0, 0, maxrow, maxcol);
        fclose(f);
    } else if (! strcmp(&name[strlen(name)-8], ".csv.bak")) {
        export_delim(namenew, ',', 1, 0, maxrow, maxcol, 0);
#ifdef XLSX_EXPORT
    } else if (! strcmp(&name[strlen(name)-9], ".xlsx.bak")) {
        export_delim(namenew, ',', 0, 0, maxrow, maxcol, 0);
        export_xlsx(namenew, 0, 0, maxrow, maxcol);
#endif
    } else if (! strcmp(&name[strlen(name)-8], ".tab.bak") || ! strcmp(&name[strlen(name)-8], ".tsv.bak")) {
        export_delim(namenew, '\t', 0, 0, maxrow, maxcol, 0);
    }

    // delete if exists name
    remove(name);

    // rename name.new to name
    rename(namenew, name);

    return (void *) 0;
}

/**
 * \brief Check if it is time to do an autobackup
 *
 * \return none
 */

void handle_backup() {
    #ifdef AUTOBACKUP
    extern struct timeval lastbackup_tv; // last backup timer
    extern struct timeval current_tv; //runtime timer

    int autobackup = atoi(get_conf_value ("autobackup"));
    if (autobackup && autobackup > 0 && (current_tv.tv_sec - lastbackup_tv.tv_sec > autobackup || (lastbackup_tv.tv_sec == 0 && lastbackup_tv.tv_usec == 0))) {
        #ifdef HAVE_PTHREAD
            if (pthread_exists) pthread_join (fthread, NULL);
            pthread_exists = (pthread_create(&fthread, NULL, do_autobackup, NULL) == 0) ? 1 : 0;
        #else
            do_autobackup();
        #endif
        gettimeofday(&lastbackup_tv, NULL);
    }
    #endif
    return;
}

/**
 * \brief Remove autobackup file
 *
 * \details Remove autobackup file. Used when quitting or when loading
 * a new file.
 *
 * \param[in] file file pointer
 * \return none
 */

void remove_backup(char * file) {
    int len = strlen(file);
    if (!len) return;
    char * pstr = strrchr(file, '/');
    int pos = pstr == NULL ? -1 : pstr - file;
    char name[len+6];
    strcpy(name, file);
    add_char(name, '.', pos+1);
    sprintf(name + strlen(name), ".bak");
    remove(name);
    return;
}

/**
 * \brief TODO Document backup_exists()
 *
 * \param[in] file file pointer
 *
 * \return none
 */

int backup_exists(char * file) {
    int len = strlen(file);
    if (!len) return 0;
    char * pstr = strrchr(file, '/');
    int pos = pstr == NULL ? -1 : pstr - file;
    char name[len+6];
    strcpy(name, file);
    add_char(name, '.', pos+1);
    sprintf(name + strlen(name), ".bak");
    FILE * fp;
    if ((fp = fopen((char *) name, "r"))) {
        fclose(fp);
        return 1;
    }
    return 0;
}
