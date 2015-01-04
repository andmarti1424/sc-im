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
#include <sys/wait.h> // for wait

#include "maps.h"
#include "yank.h"
#include "cmds.h"
#include "file.h"
#include "marks.h"
#include "lex.h"
#include "format.h"
#include "interp.h"
#include "utils/string.h"
#include "color.h"   // for set_ucolor
#include "xmalloc.h" // for scxfree
#include "y.tab.h"

#define DEFCOLDELIM ':'

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

    for (c = 0; c < COLFORMATS; c++)
        if (colformat[c]) {
            scxfree(colformat[c]);
            colformat[c] = NULL;
        }

    maxrow = 0;
    maxcol = 0;

    clean_range();
    //clean_frange();
    //clean_crange();

    propagation = 10;
    calc_order = BYROWS;
    prescale = 1.0;
    tbl_style = 0;
    //craction = 0;
    //rowlimit = collimit = -1;
    //qbuf = 0;

    //autocalc=showcell=showtop=1;
    //autoinsert=autowrap=optimize=numeric=extfunc=color=colorneg=colorerr=cslop=0;
    //currow=curcol=strow=stcol=0;
    //autocalc=1;
    optimize=extfunc=0;
    currow=curcol=0;

    if (usecurses && has_colors())
        color_set(0, NULL);

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

    // Load $HOME/.scrc if present.
    if ((home = getenv("HOME"))) {
        strcpy(curfile, home);
        strcat(curfile, "/.scimrc");
        if ((c = open(curfile, O_RDONLY)) > -1) {
            close(c);
            (void) readfile(curfile, 0);
        }
    }

    /*
    // Load ./.scimrc if present and $HOME/.scimrc contained `set scrc'.
    if (scrc && strcmp(home, getcwd(curfile, PATHLEN)) &&
        (c = open(".scimrc", O_RDONLY)) > -1) {
    close(c);
    (void) readfile(".scimrc", 0);
    }
     */

    * curfile = '\0';
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
    if (modflg) {
        error("File not saved since last change. Add '!' to force");
        return(1);
    }
    return 0;
}

// This function handles the save file process in formato SC
// returns 0 if OK
// return -1 on error
int savefile() {
    int force_rewrite = 0;
    char name[BUFFERSIZE];

    if (! curfile[0] && strlen(inputline) < 3) { // casos ":w" ":w!" ":x" ":x!"
        error("There is no filename");
        return -1;
    }

    if (inputline[1] == '!') force_rewrite = 1;

    strcpy(name, inputline);

    del_range_chars(name, 0, 1 + force_rewrite);

    if (! force_rewrite && file_exists(name)) {
        error("File already exists. Use \"!\" to force rewrite.");
        return -1;
    }

    if (strlen(inputline) > 2) {
        strcpy(curfile, name);
    }

    if (writefile(curfile, 0, 0, maxrow, maxcol) < 0) {
        error("File could not be saved");
        return -1;
    }
    return 0;
}

// Funcion que graba un archivo
// recibe un rango como parametros y un nombre de archivo a generar
int writefile(char * fname, int r0, int c0, int rn, int cn) {
    register FILE *f;
    char save[PATHLEN];
    char tfname[PATHLEN];
    long namelen;
    char *tpp;
    int pid;

    /* find the extension and mapped plugin if exists
    if ((p = strrchr(fname, '.'))) {
    if ((plugin = findplugin(p+1, 'w')) != NULL) {
        if (!plugin_exists(plugin, strlen(plugin), save + 1)) {
        error("plugin not found");
        return -1;
        }
        *save = '|';
        if ((strlen(save) + strlen(fname) + 20) > PATHLEN) {
        error("Path too long");
        return -1;
        }
        sprintf(save + strlen(save), " %s%d:", coltoa(c0), r0);
        sprintf(save + strlen(save), "%s%d \"%s\"", coltoa(cn), rn, fname);
        // pass it to readfile as an advanced macro
        readfile(save, 0);
        return (0);
    }
    }*/

    if (*fname == '\0') {
        if (isatty(STDOUT_FILENO) || *curfile != '\0')
            fname = curfile;
        else {
            write_fd(stdout, r0, c0, rn, cn);
            return 0;
        }
    }

    if ((tpp = strrchr(fname, '/')) == NULL)
        namelen = pathconf(".", _PC_NAME_MAX);
    else {
        *tpp = '\0';
        namelen = pathconf(fname, _PC_NAME_MAX);
        *tpp = '/';
    }

    (void) strcpy(tfname, fname);
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

    (void) strcpy(save, tfname);
    for (tpp = save; *tpp != '\0'; tpp++)
    if (*tpp == '"') {
        (void) memmove(tpp + 1, tpp, strlen(tpp) + 1);
        *tpp++ = '\\';
    }

    if ((f = openfile(tfname, &pid, NULL)) == NULL) {
        error("Can't create file \"%s\"", save);
        return -1;
    }

    info("Writing file \"%s\"...", save);
    write_fd(f, r0, c0, rn, cn);
    
    closefile(f, pid, 0);

    if (!pid) {
        (void) strcpy(curfile, save);
        modflg = 0;
        info("File \"%s\" written", curfile);
    }

    return 0;
}

void write_fd(register FILE *f, int r0, int c0, int rn, int cn) {
    register struct ent **pp;
    int r, c;

    (void) fprintf(f, "# This data file was generated by SCIM.\n");
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

    //write_ranges(f);
    //write_franges(f);
    //write_colors(f, 0);
    //write_cranges(f);
    write_marks(f);

    if (mdir) 
        (void) fprintf(f, "mdir \"%s\"\n", mdir);
    if (autorun) 
        (void) fprintf(f, "autorun \"%s\"\n", autorun);

    for (c = 0; c < FKEYS; c++)
        if (fkey[c]) (void) fprintf(f, "fkey %d = \"%s\"\n", c + 1, fkey[c]);

    write_cells(f, r0, c0, rn, cn, r0, c0);

    // write blocked cells
    for (r = r0; r <= rn; r++) {
        pp = ATBL(tbl, r, c0);
        for (c = c0; c <= cn; c++, pp++)
            if (*pp) {
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
    
        if ( m->row == -1 && m->row == -1) { // && m->rng != NULL ) {   // m->rng should not be NULL
            fprintf(f, "mark %c %s%d ", i, coltoa(m->rng->tlcol), m->rng->tlrow);
            fprintf(f, "%s%d\n", coltoa(m->rng->brcol), m->rng->brrow);
        } else if ( m->row != 0 && m->row != 0 ) {
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
    register FILE * f;
    char save[PATHLEN];
    //int tempautolabel;
    //char *p;
    //char *plugin;
    int pid = 0;
    int rfd = STDOUT_FILENO;//, savefd;

    //tempautolabel = autolabel;    /* turn off auto label when */
    //autolabel = 0;            /* reading a file */

    if (*fname == '*' && mdir) { 
       (void) strcpy(save, mdir);
       (void) strcat(save, fname);
    } else {
        if (*fname == '\0')
            fname = curfile;
        (void) strcpy(save, fname);
    }

    /*if ((p = strrchr(fname, '.')) && (fname[0] != '|')) {  // exclude macros
    if ((plugin = findplugin(p+1, 'r')) != NULL) {
        if (!(plugin_exists(plugin, strlen(plugin), save + 1))) {
        error("plugin not found");
        return -1;
        }
        *save = '|';
        if ((strlen(save) + strlen(fname) + 2) > PATHLEN) {
        error("Path too long");
        return -1;
        }
        sprintf(save + strlen(save), " \"%s\"", fname);
        eraseflg = 0;
        // get filename: could be preceded by params if this is a save
        while (p > fname) {
        if (*p == ' ') {
            p++;
            break;
        }
        p--;
        }
        (void) strcpy(curfile, p);
    }
    }*/

    // what is this?
    //if (eraseflg && strcmp(fname, curfile) && modcheck(" first"))
    //    return 0;

    if (fname[0] == '-' && fname[1] == '\0') {
        f = stdin;
        *save = '\0';
    } else {
        if ((f = openfile(save, &pid, &rfd)) == NULL) {
            error("Can't read file \"%s\"", save);
            //autolabel = tempautolabel;
            return 0;
        } else if (eraseflg) {
            info("Reading file \"%s\"", save);
            //refresh();
        }
    }
    if (*fname == '|')
        *save = '\0';

    if (eraseflg) erasedb();

    loading++;
    //savefd = macrofd;
    //macrofd = rfd;
    while (! brokenpipe && fgets(line, sizeof(line), f)) {
        if (line[0] == '|' && pid != 0) {
            line[0] = ' ';
        }
        linelim = 0;
        if (line[0] != '#') (void) yyparse();
    }

    //macrofd = savefd;
    --loading;
    closefile(f, pid, rfd);
    if (f == stdin) {
        freopen("/dev/tty", "r", stdin);
    }
    linelim = -1;
    if (eraseflg) {
        (void) strcpy(curfile, save);
        modflg = 0;
        cellassign = 0;
        if (autorun && !skipautorun)
            (void) readfile(autorun, 0);
        skipautorun = 0;
        EvalAll();
    }
    //autolabel = tempautolabel;
    return 1;
}

// expand a ~ in a path to your home directory
char * findhome(char *path) {
    static    char    *HomeDir = NULL;

    if (*path == '~') {
        char    *pathptr;
        char    tmppath[PATHLEN];

        if (HomeDir == NULL) {
            HomeDir = getenv("HOME");
            if (HomeDir == NULL)
                HomeDir = "/";
        }
        pathptr = path + 1;
        if ((*pathptr == '/') || (*pathptr == '\0'))
            strcpy(tmppath, HomeDir);
        else {
            struct    passwd *pwent;
            char    *namep;
            char    name[50];

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
        chown(tpath, statbuf.st_uid, statbuf.st_gid);

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
#ifdef DOBACKUPS
        if (rfd == NULL && ! backup_file(efname) )
            //(yn_ask("Could not create backup copy.  Save anyway?: (y,n)") != 1))
            return (0);
#endif
        return (fopen(efname, rfd == NULL ? "w" : "r"));
    }

    fname++;                             // Skip | 
    efname = findhome(fname);
    if (pipe(pipefd) < 0 || (rfd != NULL && pipe(pipefd+2) < 0)) {
        error("Can't make pipe to child");
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
            error("Can't fdopen %sput", rfd==NULL?"out":"in");
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
        (void) printf("Press any key to continue ");
        (void) fflush(stdout);
        cbreak();
        (void) nmgetch();
        //goraw();
        //clear();
    } else {
        close(rfd);
        if (usecurses) {
        cbreak();
        nonl();
        noecho ();
        kbd_again();
        //if (color && has_colors())
        //    bkgdset(COLOR_PAIR(1) | ' ');
        }
    }
    }
    if (brokenpipe) {
    error("Broken pipe");
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
        //! numeric &&
        prescale == 1.0 &&
        ! extfunc && //showtop &&
        tbl_style == 0 //&&
        //craction == 0 &&
        //pagesize == 0 &&
        //rowlimit == -1 &&
        //collimit == -1 //&&
        //!color &&
        //!colorneg &&
        //!colorerr
       )
    return;        // No reason to do this

    (void) fprintf(f, "set");
    //if (!autocalc)             (void) fprintf(f," !autocalc");
    //if (autoinsert)            (void) fprintf(f," autoinsert");
    //if (autowrap)              (void) fprintf(f," autowrap");
    //if (cslop)      //       (void) fprintf(f," cslop");
    if (optimize)              (void) fprintf(f," optimize");
    if (rndtoeven)             (void) fprintf(f, " rndtoeven");
    if (propagation != 10)     (void) fprintf(f, " iterations = %d", propagation);
    if (calc_order != BYROWS ) (void) fprintf(f, " bycols");
    //if (numeric)               (void) fprintf(f, " numeric");
    if (prescale != 1.0)       (void) fprintf(f, " prescale");
    if (extfunc)               (void) fprintf(f, " extfun");
    //if (!showtop)            (void) fprintf(f, " !toprow");
    if (tbl_style)             (void) fprintf(f, " tblstyle = %s", tbl_style == TBL ? "tbl" : tbl_style == LATEX ? "latex" : tbl_style == SLATEX ? "slatex" : tbl_style == TEX ? "tex" : tbl_style == FRAME ? "frame" : "0" );
    //if (craction)              (void) fprintf(f, " craction = %d", craction);
    //if (pagesize)              (void) fprintf(f, " pagesize = %d", pagesize);
    //if (rowlimit >= 0)         (void) fprintf(f, " rowlimit = %d", rowlimit);
    //if (collimit >= 0)         (void) fprintf(f, " collimit = %d", collimit);
    //if (color)                 (void) fprintf(f," color");
    //if (colorneg)              (void) fprintf(f," colorneg");
    //if (colorerr)              (void) fprintf(f," colorerr");
    (void) fprintf(f, "\n");
}


// Importación de CSV a SC
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
        error("Can't read file \"%s\"", fname);
        return -1;
    }

    // recorro archivo csv
    while ( ! feof(f) && (fgets(line_in, sizeof(line_in), f) != NULL) ) {        
        // this hack is for importing file that have DOS eol
        int l = strlen(line_in);
        while (l--)
            if (line_in[l] == 0x0d) {
                line_in[l] = '\0';
                break;
            }

        // rompo la cadena por delimitador
        token = xstrtok(line_in, delim);
        c = 0;

        while( token != NULL ) {
            clean_carrier(token);
            if ( (token[0] == '\"' || quote) && (token[strlen(token)-1] != '\"' || strlen(token) == 1) ) {
                quote = 1;
                sprintf(token, "%s%s", token, xstrtok(NULL, ","));
                continue;
            }
            if (quote) { // elimino comillas si vengo de quote
                del_char(token, 0);
                del_char(token, strlen(token)-1);
            }
            if (isnumeric(token)) {
                sprintf(line_interp, "let %s%d=%s", coltoa(c), r, token);
            //} else if (token[0] == '"') {
            //    sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, token);
            } else {
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, token);
                //info("label %s%d=\"%s\"", coltoa(c), r, token);
                //get_key();
            }
            send_to_interp(line_interp);
            c++;
            quote = 0;
            token = xstrtok(NULL, ",");            
        }     
        
        r++;
    }
    
    maxrow = r-1;
    maxcol = c-1;

    //closefile(f, pid, rfd);
    fclose(f);

    EvalAll();

    return 0;
}

// Exportación a CSV y TAB

void do_export(int r0, int c0, int rn, int cn) {
    int force_rewrite = 0;
    char type_export[4] = "";
    char ruta[PATHLEN];
    char linea[BUFFERSIZE];

    if (inputline[1] == '!') force_rewrite = 1;
    strcpy(linea, inputline); // copio a una nueva variable para no afectar el historial de comandos
    del_range_chars(linea, 0, 1 + force_rewrite); // elimino 'e ' o 'e! ' del inputline

    // obtengo el tipo de formato al cual se exportará la planilla
    if (str_in_str(linea, "csv") == 0) {
        strcpy(type_export, "csv");
    } else if (str_in_str(linea, "tab") == 0) {
        strcpy(type_export, "tab");
    } 

    // luego obtengo la ruta y denominación del archivo a grabar.
    // si se ingresa una como parametro, se la toma.
    if (strlen(linea) > 4) {   // 'csv '
        del_range_chars(linea, 0, 3); // elimino 'csv '
        strcpy(ruta, linea);

    // si no se ingresa una, se toma el nombre de curfile y se le agrega el tipo de extensión (csv o tab)
    // se verifica si el nombre actual termina con ".sc" y se lo quita si es necesario.
    } else if (curfile[0]) {
        strcpy(ruta, curfile);
        char * ext = strrchr(ruta, '.');
        if (ext != NULL) del_range_chars(ruta, strlen(ruta) - strlen(ext), strlen(ruta)-1);
        sprintf(ruta, "%s.%s", ruta, type_export);

    } else {
        error("No filename specified !");
        return;
    }

    if (! force_rewrite && file_exists(ruta) && strlen(ruta) > 0) {
        error("File %s already exists. Use \"!\" to force rewrite.", ruta);
        return;
    }

    // llamo a las rutinas de exportacion
    if (strcmp(type_export, "csv") == 0) {
        export_delim(ruta, ',', r0, c0, rn, cn);
    } if (strcmp(type_export, "tab") == 0) {
        export_delim(ruta, '\t', r0, c0, rn, cn);
    }
}

// fname indica la ruta y denominación del archivo
void export_delim(char * fname, char coldelim, int r0, int c0, int rn, int cn) {
    FILE *f;    
    int row, col;
    register struct ent **pp;    
    int pid;
    
    info("Writing file \"%s\"...", fname);

    if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
        error ("Can't create file \"%s\"", fname);
        return;
    }

    struct ent * ent = go_end();
    if (rn > ent->row) rn = ent->row;
    //if (cn > ent->col) cn = ent->col;

    for (row = r0; row <= rn; row++) {        
        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            if (*pp) {
                char * s;
                if ((*pp)->flags & is_valid) {
                    if ((*pp)->cellerror) {
                        (void) fprintf (f, "%*s", fwidth[col], ((*pp)->cellerror == CELLERROR ? "ERROR" : "INVALID"));
                    } else if ((*pp)->format) {
                        char field[FBUFLEN];
                        if (*((*pp)->format) == 'd') {  // formato fecha
                            time_t v = (time_t) ((*pp)->v);
                            strftime(field, sizeof(field), ((*pp)->format)+1, localtime(&v));
                        } else {                        // formato numerico
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
        info("File \"%s\" written", fname);
    }
}

/* unspecial (backquote -> ") things that are special chars in a table */
void unspecial(FILE *f, char *str, int delim) {
    int backquote = 0;     
    if (str_in_str(str, ",") != -1) backquote = 1;
    if (backquote) putc('\"', f);

    //if (*str == '\\') str++; /* delete wheeling string operator, OK? */
    while (*str) {
        //if (*str != ' ') //elimino los espacios???
        putc(*str, f); 
        //if (((tbl_style == LATEX) || (tbl_style == SLATEX) || (tbl_style == TEX)) &&
        //((*str == delim) || (*str == '$') || (*str == '#') || (*str == '%') || (*str == '{') || (*str == '}') || (*str == '&')))
        //    putc('\\', f);
        str++;
    }
    if (backquote) putc('\"', f);
}

/*
void printfile(char *fname, int r0, int c0, int rn, int cn) {
    FILE *f;
    static char *pline = NULL;        // only malloc once, malloc is slow
    static unsigned fbufs_allocated = 0;
    int plinelim;
    int pid;
    int fieldlen, nextcol;
    long namelen;
    int row, col;
    register struct ent **pp;
    char file[256];
    char path[1024];
    char *tpp;

    if (fname) {
    // printfile will be the [path/]file ---> [path/]file.out
    if (*fname == '\0') {
        strcpy(path, curfile);

        if ((tpp = strrchr(path, '/')) == NULL) {
        namelen = pathconf(".", _PC_NAME_MAX);
        tpp = path;
        } else {
        *tpp = '\0';
        namelen = pathconf(path, _PC_NAME_MAX);
        *tpp = '/';
        tpp++;
        }
        strcpy(file, tpp);

        if (!strcmp(file + strlen(file) - 3, ".sc"))
        file[strlen(file) - 3] = '\0';
        else if (scext != NULL && file[strlen(file) - strlen(scext) - 1] == '.'
            && !strcmp(file + strlen(file) - strlen(scext), scext))
        file[strlen(file) - strlen(scext)] = '\0';

        if (ascext == NULL)
        file[namelen - 4] = '\0';
        else
        file[namelen - strlen(ascext) - 1] = '\0';
        sprintf(tpp, "%s.%s", file, ascext == NULL ? "asc" : ascext);
        fname = path;
    }

    //if ((strcmp(fname, curfile) == 0) &&
     // !yn_ask("Confirm that you want to destroy the data base: (y,n)")) {
     // return;
    }//

    if ((f = openfile(fname, &pid, NULL)) == (FILE *)0) {
        error("Can't create file \"%s\"", fname);
        return;
    }
    } else
    f = stdout;

    if (!pline && (pline = scxmalloc((unsigned)(FBUFLEN *
        ++fbufs_allocated))) == (char *)NULL) {
    error("Malloc failed in printfile()");
    return;
    }

    for (row = r0; row <= rn; row++) {
    int c = 0;

    if (row_hidden[row])
        continue;

    pline[plinelim=0] = '\0';
    for (pp = ATBL(tbl, row, col=c0); col<=cn;
            pp += nextcol-col, col = nextcol, c += fieldlen) {

        nextcol = col+1;
        if (col_hidden[col]) {
        fieldlen = 0;
        continue;
        }

        fieldlen = fwidth[col];
        if (*pp) {
        char *s;

        // dynamically allocate pline, making sure we are not 
        // attempting to write 'out of bounds'.
        while (c > (fbufs_allocated * FBUFLEN)) {
            if ((pline = scxrealloc ((char *)pline, 
                (unsigned)(FBUFLEN * ++fbufs_allocated))) == NULL) {
            error ("Realloc failed in printfile()");
            return;
            }
        }          
        while (plinelim<c) pline[plinelim++] = ' ';
        plinelim = c;
        if ((*pp)->flags&is_valid) {
            while(plinelim + fwidth[col] > 
              (fbufs_allocated * FBUFLEN)) {
              if((pline = ((char *)scxrealloc
                   ((char *)pline, 
                    (unsigned)(FBUFLEN * ++fbufs_allocated))))
             == NULL) {
            error("Realloc failed in printfile()");
            return;
              }
            }
            if ((*pp)->cellerror)
            (void) sprintf(pline+plinelim, "%*s",
                fwidth[col], ((*pp)->cellerror == CELLERROR ?
                "ERROR " : "INVALID "));
            else {
              char *cfmt;

              cfmt = (*pp)->format ? (*pp)->format :
                (realfmt[col] >= 0 && realfmt[col] < COLFORMATS &&
                colformat[realfmt[col]]) ?
                colformat[realfmt[col]] : 0;
              if (cfmt) {
                   char field[FBUFLEN];

            if (*cfmt == ctl('d')) {
                time_t v = (time_t) ((*pp)->v);
                strftime(field, sizeof(field), cfmt + 1,
                    localtime(&v));
                sprintf(pline+plinelim, "%-*s", fwidth[col],
                    field);
            } else {
                format(cfmt, precision[col], (*pp)->v, field,
                    sizeof(field));
                (void) sprintf(pline+plinelim, "%*s", fwidth[col],
                    field);
            }
              } else {
                   char field[FBUFLEN];
            (void) engformat(realfmt[col], fwidth[col],
                                             precision[col], (*pp) -> v,
                                             field, sizeof(field));
            (void) sprintf(pline+plinelim, "%*s", fwidth[col],
                       field);
              }
            }
            plinelim += strlen(pline+plinelim);
        }
        if ((s = (*pp)->label)) {
            int slen;
            char *start, *last;
            register char *fp;
            struct ent *nc;

             // Figure out if the label slops over to a blank field.
             // A string started with backslash is defining repetition
             // char
            slen = strlen(s);
            if (*s == '\\' && *(s+1) != '\0')
            slen = fwidth[col];
            while (slen > fieldlen && nextcol <= cn &&
                !((nc = lookat(row,nextcol))->flags & is_valid) &&
                !(nc->label)) {
            
                    if (!col_hidden[nextcol])
                 fieldlen += fwidth[nextcol];

            nextcol++;
            }
            if (slen > fieldlen)
            slen = fieldlen;
            
            while(c + fieldlen + 2 > (fbufs_allocated * FBUFLEN)) {
              if((pline = ((char *)scxrealloc
                   ((char *)pline, 
                    (unsigned)(FBUFLEN * ++fbufs_allocated))))
             == NULL) {
            error ("scxrealloc failed in printfile()");
            return;
              }
            }          

            // Now justify and print
            start = (*pp)->flags & is_leftflush ? pline + c
                    : pline + c + fieldlen - slen;
            if( (*pp)->flags & is_label )
            start = pline + (c + ((fwidth[col]>slen)?(fwidth[col]-slen)/2:0));
            last = pline + c + fieldlen;
            fp = plinelim < c ? pline + plinelim : pline + c;
            while (fp < start)
            *fp++ = ' ';
            if( *s == '\\' && *(s+1)!= '\0' ) {
            char *strt;
            strt = ++s;

            while(slen--) {
                *fp++ = *s++; if( *s == '\0' ) s = strt;
            }
            }
            else
            while (slen--)
            *fp++ = *s++;

            if (!((*pp)->flags & is_valid) || fieldlen != fwidth[col])
            while(fp < last)
                *fp++ = ' ';
            if (plinelim < fp - pline)
            plinelim = fp - pline;
        }
        }
    }
    pline[plinelim++] = '\n';
    pline[plinelim] = '\0';
    (void) fputs (pline, f);
    }

    if (fname) closefile(f, pid, 0);
}
*/

