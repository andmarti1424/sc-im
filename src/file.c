#include <pwd.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <sys/wait.h>

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
#include "color.h"
#include "xmalloc.h"
#include "y.tab.h"
#include "xlsx.h"
#include "xls.h"
#include "screen.h"

extern struct ent * freeents;

/* erase the database (tbl, etc.) */
void erasedb() {
    int  r, c;
    char * home;

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

    propagation = 10;
    calc_order = BYROWS;
    prescale = 1.0;
    tbl_style = 0;
    optimize = 0;
    currow = curcol = 0;

//    if (usecurses && has_colors())
//        color_set(0, NULL);

/*
    if (mdir) {
        scxfree(mdir);
        mdir = NULL;
    }
    if (autorun) {
        scxfree(autorun);
        autorun = NULL;
    }
    for (c = 0; c < FKEYS; c++)
        if (fkey[c]) {
            scxfree(fkey[c]);
            fkey[c] = NULL;
        }
*/

    // Load $HOME/.scrc if present.
    if ((home = getenv("HOME"))) {
        strcpy(curfile, home);
        strcat(curfile, "/.scimrc");
        if ((c = open(curfile, O_RDONLY)) > -1) {
            close(c);
            (void) readfile(curfile, 0);
        }
    }

    *curfile = '\0';
}

// function that checks if a file exists.
// returns 1 if so. returns 0 otherwise.
int file_exists(const char * fname) {
    FILE * file;
    if ((file = fopen(fname, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

// This function checks if a file suffered mods since it was open
int modcheck() {
    if (modflg && ! atoi(get_conf_value("nocurses"))) {
        scerror("File not saved since last change. Add '!' to force");
        return(1);
    }
    return 0;
}

// This function handles the save file process in SC-IM format
// returns 0 if OK
// return -1 on error
int savefile() {
    int force_rewrite = 0;
    char name[BUFFERSIZE];

    if (! curfile[0] && strlen(inputline) < 3) { // casos ":w" ":w!" ":x" ":x!"
        scerror("There is no filename");
        return -1;
    }

    if (inputline[1] == '!') force_rewrite = 1;

    strcpy(name, inputline);

    del_range_chars(name, 0, 1 + force_rewrite);

    if (! force_rewrite && file_exists(name)) {
        scerror("File already exists. Use \"!\" to force rewrite.");
        return -1;
    }

    if (strlen(inputline) > 2) {
        strcpy(curfile, name);
    }

    if (writefile(curfile, 0, 0, maxrow, maxcol) < 0) {
        scerror("File could not be saved");
        return -1;
    }
    return 0;
}

// Write a file
// receives parameter range and file name
int writefile(char * fname, int r0, int c0, int rn, int cn) {
    register FILE *f;
    char save[PATHLEN];
    char tfname[PATHLEN];
    //long namelen;
    //char * tpp;
    int pid;

/*
    if (*fname == '\0') {
        if (isatty(STDOUT_FILENO) || *curfile != '\0')
            fname = curfile;
        else {
            write_fd(stdout, r0, c0, rn, cn);
            return 0;
        }
    }
*/

/*
    if ((tpp = strrchr(fname, '/')) == NULL)
        namelen = pathconf(".", _PC_NAME_MAX);
    else {
        *tpp = '\0';
        namelen = pathconf(fname, _PC_NAME_MAX);
        *tpp = '/';
    }
*/

    (void) strcpy(tfname, fname);
/*
    for (tpp = tfname; *tpp != '\0'; tpp++)
        if (*tpp == '\\' && *(tpp + 1) == '"')
            (void) memmove(tpp, tpp + 1, strlen(tpp));

        if (scext != NULL) {
            if (strlen(tfname) > 3 && !strcmp(tfname + strlen(tfname) - 3, ".sc"))
                tfname[strlen(tfname) - 3] = '\0';
            else if (strlen(tfname) > strlen(scext) + 1 &&
            tfname[strlen(tfname) - strlen(scext) - 1] == '.' &&
            ! strcmp(tfname + strlen(tfname) - strlen(scext), scext))
                tfname[strlen(tfname) - strlen(scext) - 1] = '\0';
            tfname[namelen - strlen(scext) - 1] = '\0';
            strcat(tfname, ".");
            strcat(tfname, scext);
        }
*/

    (void) strcpy(save, tfname);
/*
    for (tpp = save; *tpp != '\0'; tpp++)
    if (*tpp == '"') {
        (void) memmove(tpp + 1, tpp, strlen(tpp) + 1);
        *tpp++ = '\\';
    }
*/

    if ((f = openfile(tfname, &pid, NULL)) == NULL) {
        scerror("Can't create file \"%s\"", save);
        return -1;
    }

    scinfo("Writing file \"%s\"...", save);
    write_fd(f, r0, c0, rn, cn);

    closefile(f, pid, 0);

    if (! pid) {
        (void) strcpy(curfile, save);
        modflg = 0;
        scinfo("File \"%s\" written", curfile);
    }

    return 0;
}

void write_fd(register FILE *f, int r0, int c0, int rn, int cn) {
    register struct ent **pp;
    int r, c;

    (void) fprintf(f, "# This data file was generated by SC-IM.\n");
    (void) fprintf(f, "# You almost certainly shouldn't edit it.\n\n");
    print_options(f);
    for (c = 0; c < COLFORMATS; c++)
        if (colformat[c])
            (void) fprintf (f, "format %d = \"%s\"\n", c, colformat[c]);
    for (c = c0; c <= cn; c++)
        if (fwidth[c] != DEFWIDTH || precision[c] != DEFPREC || realfmt[c] != DEFREFMT)
            (void) fprintf (f, "format %s %d %d %d\n", coltoa(c), fwidth[c], precision[c], realfmt[c]);
    for (c = c0; c <= cn; c++)
        if (col_hidden[c])
            (void) fprintf(f, "hide %s\n", coltoa(c));
    for (r = r0; r <= rn; r++)
        if (row_hidden[r])
            (void) fprintf(f, "hide %d\n", r);

    // padding
    for (c = c0; c <= cn; c++) {
        pp = ATBL(tbl, 0, c);
        if (*pp && (*pp)->pad)
            (void) fprintf(f, "pad %d %s\n", (*pp)->pad, coltoa(c));
    }

    //write_ranges(f);
    write_marks(f);

    /*
    if (mdir)
        (void) fprintf(f, "mdir \"%s\"\n", mdir);
    if (autorun)
        (void) fprintf(f, "autorun \"%s\"\n", autorun);

    for (c = 0; c < FKEYS; c++)
        if (fkey[c]) (void) fprintf(f, "fkey %d = \"%s\"\n", c + 1, fkey[c]);
    */

    write_cells(f, r0, c0, rn, cn, r0, c0);

    for (r = r0; r <= rn; r++) {
        pp = ATBL(tbl, r, c0);
        for (c = c0; c <= cn; c++, pp++)
            if (*pp) {
                // Write ucolors
                if ((*pp)->ucolor != NULL) {

                    char strcolor[BUFFERSIZE];

                    // decompile int value of color to its string description
                    linelim=0;
                    struct enode * e = new((*pp)->ucolor->fg, (struct enode *)0, (struct enode *)0);
                    decompile(e, 0);
                    uppercase(line);
                    if (line[0] == '@') del_char(line, 0); // FIXME THIS !!!
                    sprintf(strcolor, "fg=%s", &line[0]);
                    free(e);
                    linelim=0;
                    e = new((*pp)->ucolor->bg, (struct enode *)0, (struct enode *)0);
                    decompile(e, 0);
                    uppercase(line);
                    if (line[0] == '@') del_char(line, 0); // FIXME THIS !!!
                    sprintf(strcolor + strlen(strcolor), " bg=%s", &line[0]);
                    free(e);

                    if ((*pp)->ucolor->bold)      sprintf(strcolor + strlen(strcolor), " bold=1");
                    if ((*pp)->ucolor->dim)       sprintf(strcolor + strlen(strcolor), " dim=1");
                    if ((*pp)->ucolor->reverse)   sprintf(strcolor + strlen(strcolor), " reverse=1");
                    if ((*pp)->ucolor->standout)  sprintf(strcolor + strlen(strcolor), " standout=1");
                    if ((*pp)->ucolor->underline) sprintf(strcolor + strlen(strcolor), " underline=1");
                    if ((*pp)->ucolor->blink)     sprintf(strcolor + strlen(strcolor), " blink=1");

                    // previous implementation
                    //(void) fprintf(f, "cellcolor %s%d \"%s\"\n", coltoa((*pp)->col), (*pp)->row, strcolor);

                    // new implementation
                    // by row, store cellcolors grouped by ranges
                    int c_aux = c;
                    struct ucolor * u = (*pp)->ucolor;
//                  struct ucolor * a = (*ATBL(tbl, r, c-1)) == NULL ? NULL : (*ATBL(tbl, r, c-1))->ucolor;
                    struct ucolor * a = NULL;
                    if ( c > 0 && *ATBL(tbl, r, c-1) != NULL)
                         a = (*ATBL(tbl, r, c-1))->ucolor;

                    if ( (u != NULL) && (c <= maxcol) && ( c == 0 || ( a == NULL ) || ( ! same_ucolor( NULL, NULL) ))) {
                        while (c_aux < maxcol && *ATBL(tbl, r, c_aux) != NULL && same_ucolor( (*ATBL(tbl, r, c_aux))->ucolor, (*pp)->ucolor ))
                            c_aux++;
                        fprintf(f, "cellcolor %s%d", coltoa((*pp)->col), (*pp)->row);
                        fprintf(f, ":%s%d \"%s\"\n", coltoa(c_aux-1), (*pp)->row, strcolor);
                    }

                }

                // write blocked cells
                // lock should be stored after any other command
                if ((*pp)->flags & is_locked)
                    (void) fprintf(f, "lock %s%d\n", coltoa((*pp)->col), (*pp)->row);


                /*if ((*pp)->nrow >= 0) {
                    (void) fprintf(f, "addnote %s ", v_name((*pp)->row, (*pp)->col));
                    (void) fprintf(f, "%s\n", r_name((*pp)->nrow, (*pp)->ncol, (*pp)->nlastrow, (*pp)->nlastcol));
                }*/
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

void write_cells(register FILE *f, int r0, int c0, int rn, int cn, int dr, int dc) {
    register struct ent **pp;
    int r, c, mf;
    char *dpointptr;

    mf = modflg;
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
                if ((*pp)->flags & is_valid) {
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
    /*if (dr != r0 || dc != c0) {
        currow = rs;
        curcol = cs;
        flush_saved();
    }*/
    modflg = mf;
}

int readfile(char * fname, int eraseflg) {

    // Check if file is a correct format
    int len = strlen(fname);
    if (! strcmp( & fname[len-3], ".sc") ||
        (len > 6 && ! strcasecmp( & fname[len-7], ".scimrc"))) {
      // pass

    // If file is an xlsx file, we import it
    } else if (len > 5 && ! strcasecmp( & fname[len-5], ".xlsx")){
        #ifndef XLSX
        if (! atoi(get_conf_value("nocurses")))
            scerror("XLSX import support not compiled in");
        #else
        open_xlsx(fname, "UTF-8");
        #endif
        //*curfile = '\0';
        modflg = 0;
        return 1;

    // If file is an xls file, we import it
    } else if (len > 4 && ! strcasecmp( & fname[len-4], ".xls")){
        #ifndef XLS
        scerror("XLS import support not compiled in");
        #else
        open_xls(fname, "UTF-8");
        #endif
        //*curfile = '\0';
        modflg = 0;
        return 1;

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

        modflg = 0;
        return 1;

    } else if (! atoi(get_conf_value("nocurses"))) {
        scinfo("\"%s\" is not a SC-IM compatible file", fname);
        return -1;
    }

    register FILE * f;
    char save[PATHLEN];
    int pid = 0;
    int rfd = STDOUT_FILENO;//, savefd;

    //if (*fname == '*' && mdir) {
    //   (void) strcpy(save, mdir);
    //   (void) strcat(save, fname);
    //} else {
        if (*fname == '\0')
            fname = curfile;
        (void) strcpy(save, fname);
    //}

    if (fname[0] == '-' && fname[1] == '\0') {
        f = stdin;
        *save = '\0';
    } else {
        if ((f = openfile(save, &pid, &rfd)) == NULL) {
            scerror("Can't read file \"%s\"", save);
            return 0;
        } else if (eraseflg) {
            scinfo("Reading file \"%s\"", save);
        }
    }
    if (*fname == '|')
        *save = '\0';

    if (eraseflg) erasedb();

    loading++;
    while (! brokenpipe && fgets(line, sizeof(line), f)) {
        if (line[0] == '|' && pid != 0) {
            line[0] = ' ';
        }
        linelim = 0;
        if (line[0] != '#') (void) yyparse();
    }

    --loading;
    closefile(f, pid, rfd);
    if (f == stdin) {
        (void) freopen("/dev/tty", "r", stdin);
    }
    linelim = -1;
    if (eraseflg) {
        (void) strcpy(curfile, save);
        modflg = 0;
        cellassign = 0;
        /*if (autorun && ! skipautorun)
            (void) readfile(autorun, 0);
        skipautorun = 0;
        */
        EvalAll();
    }
    return 1;
}

// expand a ~ in a path to your home directory
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

// make a backup copy of a file, use the same mode and name in the format
//[path/]file~
// return 1 if we were successful, 0 otherwise
int backup_file(char *path) {
    struct stat statbuf;
    struct utimbuf timebuf;
    char fname[PATHLEN];
    char tpath[PATHLEN];
    char buf[BUFSIZ];
    char *tpp;
    int infd, outfd;
    int count;
    mode_t oldumask;

    // tpath will be the [path/]file ---> [path/]file~
    strcpy(tpath, path);
    if ((tpp = strrchr(tpath, '/')) == NULL)
        tpp = tpath;
    else
        tpp++;
    strcpy(fname, tpp);
    (void) sprintf(tpp, "%s~", fname);

    if (stat(path, &statbuf) == 0) {
        if ((infd = open(path, O_RDONLY, 0)) < 0)
            return (0);

        oldumask = umask(0);
        outfd = open(tpath, O_TRUNC|O_WRONLY|O_CREAT, statbuf.st_mode);
        umask(oldumask);
        if (outfd < 0)
            return (0);
        (void) chown(tpath, statbuf.st_uid, statbuf.st_gid);

        while ((count = read(infd, buf, sizeof(buf))) > 0) {
            if (write(outfd, buf, count) != count) {
                count = -1;
                break;
            }
        }
        close(infd);
        close(outfd);

        // copy access and modification times from original file
        timebuf.actime = statbuf.st_atime;
        timebuf.modtime = statbuf.st_mtime;
        utime(tpath, &timebuf);

        return ((count < 0) ? 0 : 1);
    } else if (errno == ENOENT)
        return (1);
    return (0);
}

// Open the input or output file, setting up a pipe if needed
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
        scerror("Can't make pipe to child");
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
            scerror("Can't fdopen %sput", rfd==NULL?"out":"in");
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
void closefile(FILE *f, int pid, int rfd) {
    int temp;

    (void) fclose(f);
    if (pid) {
        while (pid != wait(&temp)) //;
        if (rfd==0) {
            printf("Press any key to continue ");
            fflush(stdout);
            cbreak();
            get_key();
            //goraw();
            //clear();
        } else {
            close(rfd);
            if (! atoi(get_conf_value("nocurses"))) {
                cbreak();
                nonl();
                noecho ();
                //kbd_again();
            //if (color && has_colors())
            //    bkgdset(COLOR_PAIR(1) | ' ');
            }
        }
    }
    if (brokenpipe) {
        scerror("Broken pipe");
        brokenpipe = FALSE;
    }
}

void print_options(FILE *f) {
    if (
        // autocalc &&
        //! autoinsert &&
        //! autowrap &&
        //! cslop &&
        ! optimize &&
        ! rndtoeven &&
        propagation == 10 &&
        calc_order == BYROWS &&
        prescale == 1.0 &&
        ! atoi(get_conf_value("external_functions")) &&
        tbl_style == 0
       )
    return; // No reason to do this

    (void) fprintf(f, "set");
    if (optimize)              (void) fprintf(f," optimize");
    if (rndtoeven)             (void) fprintf(f, " rndtoeven");
    if (propagation != 10)     (void) fprintf(f, " iterations = %d", propagation);
    if (calc_order != BYROWS ) (void) fprintf(f, " bycols");
    if (prescale != 1.0)       (void) fprintf(f, " prescale");
    if ( atoi(get_conf_value("external_functions")) ) (void) fprintf(f, " extfun");
    if (tbl_style)             (void) fprintf(f, " tblstyle = %s", tbl_style == TBL ? "tbl" : tbl_style == LATEX ? "latex" : tbl_style == SLATEX ? "slatex" : tbl_style == TEX ? "tex" : tbl_style == FRAME ? "frame" : "0" );
    (void) fprintf(f, "\n");
}


// Import: CSV to SC
int import_csv(char * fname, char d) {

    register FILE * f;
    //int pid = 0;
    //int rfd = STDOUT_FILENO;
    int r = 0, c = 0;
    char line_in[FBUFLEN];
    char line_interp[FBUFLEN] = "";

    char * token;

    int quote = 0; // if value has '"'. ex: 12,"1234,450.00",56
    char delim[2] = ""; //strtok receives a char *, not a char
    add_char(delim, d, 0);

    //if ((f = openfile(fname, & pid, & rfd)) == NULL) {
    if ((f = fopen(fname , "r")) == NULL) {
        scerror("Can't read file \"%s\"", fname);
        return -1;
    }
    loading = 1;

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
                    sprintf(token + strlen(token), "%s", next);
                    continue;
                }
            }
            if (quote) { // Remove quotes
                del_char(token, 0);
                del_char(token, strlen(token)-1);
            }
            char * st = str_replace (token, "\"", "''"); //replace double quotes inside string
            // Now every value gets imported as text!
            //if (isnumeric(st) && strlen(st) && token[strlen(st)-1] != '-' && token[strlen(st)-1] != '.') { // FIXME do a proper isnumeric function !!
            //    sprintf(line_interp, "let %s%d=%s", coltoa(c), r, st);
            //} else {
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, st);
            //}
            send_to_interp(line_interp);
            c++;
            quote = 0;
            token = xstrtok(NULL, delim);
            free(st);
        }

        r++;
        if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
    }
    //scdebug("END");

    maxrow = r-1;
    maxcol = c-1;

    // to do a test:
    //sprintf(line_interp, "let %s%d=%s", coltoa(maxcol), maxrow, "0");
    //send_to_interp(line_interp);

    auto_justify(0, maxcol, DEFWIDTH);

    //closefile(f, pid, rfd);
    fclose(f);
    loading = 0;

    EvalAll();

    return 0;
}

// Export to CSV, TAB or plain TXT
void do_export(int r0, int c0, int rn, int cn) {
    int force_rewrite = 0;
    char type_export[4] = "";
    char ruta[PATHLEN];
    char linea[BUFFERSIZE];

    if (inputline[1] == '!') force_rewrite = 1;
    strcpy(linea, inputline); // Use new variable to keep command history untouched
    del_range_chars(linea, 0, 1 + force_rewrite); // Remove 'e' or 'e!' from inputline

    // Get format to export
    if (str_in_str(linea, "csv") == 0) {
        strcpy(type_export, "csv");
    } else if (str_in_str(linea, "tab") == 0) {
        strcpy(type_export, "tab");
    } else if (str_in_str(linea, "txt") == 0) {
        strcpy(type_export, "txt");
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
        scerror("No filename specified !");
        return;
    }

    if (! force_rewrite && file_exists(ruta) && strlen(ruta) > 0) {
        scerror("File %s already exists. Use \"!\" to force rewrite.", ruta);
        return;
    }

    // Call export routines
    if (strcmp(type_export, "csv") == 0) {
        export_delim(ruta, ',', r0, c0, rn, cn);
    } else if (strcmp(type_export, "tab") == 0) {
        export_delim(ruta, '\t', r0, c0, rn, cn);
    } else if (strcmp(type_export, "txt") == 0) {
        export_plain(ruta, r0, c0, rn, cn);
    }
}

// fname is the path and name of file
void export_plain(char * fname, int r0, int c0, int rn, int cn) {
    FILE * f;
    int row, col;
    register struct ent ** pp;
    int pid;
    char out[FBUFLEN] = "";

    scinfo("Writing file \"%s\"...", fname);

    if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
        scerror ("Can't create file \"%s\"", fname);
        return;
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
                out [0] = '\0';
                formated_s[0] = '\0';
                res = -1;
                align = 1;

                // If a numeric value exists
                if ( (*pp)->flags & is_valid) {
                    res = get_formated_value(pp, col, formated_s);
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
                (void) fprintf (f, "%s", out);

            } else {
                (void) fprintf (f, "%*s", fwidth[col], " ");
            }
        }
        (void) fprintf(f,"\n");
    }
    closefile(f, pid, 0);

    if (! pid) {
        scinfo("File \"%s\" written", fname);
    }

}

// fname is the path and name of file
void export_delim(char * fname, char coldelim, int r0, int c0, int rn, int cn) {
    FILE * f;
    int row, col;
    register struct ent ** pp;
    int pid;

    scinfo("Writing file \"%s\"...", fname);

    if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
        scerror ("Can't create file \"%s\"", fname);
        return;
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
    closefile(f, pid, 0);

    if (! pid) {
        scinfo("File \"%s\" written", fname);
    }
}

/* unspecial (backquote -> ") things that are special chars in a table */
void unspecial(FILE *f, char *str, int delim) {
    int backquote = 0;     

    if (str_in_str(str, ",") != -1) backquote = 1;
    if (backquote) putc('\"', f);
    while (*str) {
        putc(*str, f); 
        str++;
    }
    if (backquote) putc('\"', f);
}
