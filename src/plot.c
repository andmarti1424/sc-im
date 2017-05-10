#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <errno.h>

#include "plot.h"
#include "file.h"
#include "tui.h"

int plotedit(wchar_t * s) {
#ifdef GNUPLOT
    // edit ~/.scim/plotxxxx (or /usr/local/share/scim/plotxxxx)
    char command[BUFFERSIZE];

    if (! wcscmp(s, L"line") || ! wcscmp(s, L"scatter") ||
        ! wcscmp(s, L"pie")  || ! wcscmp(s, L"bar")) {
        char buffer[PATHLEN];
        char path_out[PATHLEN];
        char type[BUFFERSIZE];
        wcstombs(type, s, BUFFERSIZE);
        sprintf(buffer, "plot_%s", type);
        if (! plugin_exists(buffer, strlen(buffer), path_out)) {
            sc_error("could not load plot template file");
            return -1;
        }
        ui_pause();

        char * editor;
        if (! (editor = getenv("EDITOR")))
            editor = DFLT_EDITOR;
        sprintf(command, "%s %s", editor, path_out);

        if (system(command) == -1) sc_error("Failed editting plot file - errno:%d", errno);
        ui_resume();
    } else {
        sc_error("error: invalid plot file: %ls", s);
        return -1;
    }
    return 0;
#else
    sc_error("Gnuplot was not installed when building Sc-im. Please rebuild Sc-im.");
    return -1;
#endif
}

int plot(char * s, int r, int c, int rf, int cf) {
#ifdef GNUPLOT
    // create tmp file
    char datafile[] = "/tmp/sc-im-plotdataXXXXXX";
    int fd = mkstemp(datafile);
    if (fd == -1) {
        sc_error("Error while creating temp file for plot");
        return -1;
    }

    // export range to temp file in csv format
    export_delim(datafile, ',', r, c, rf, cf, 0);

    // call gnuplot with  ~/.scim/plotline (or /usr/local/share/scim/plotline) and temp data file
    char command[BUFFERSIZE];
    char buffer[PATHLEN];
    char buffer1[PATHLEN];
    sprintf(command, "gnuplot -e \"filename='%s'\"", datafile);

    if (! strcmp(s, "line") || ! strcmp(s, "scatter") ||
        ! strcmp(s, "pie")  || ! strcmp(s, "bar")) {
        sprintf(buffer, "plot_%s", s);
        if (! plugin_exists(buffer, strlen(buffer), buffer1)) {
            sc_error("could not load default plotline file");
            return -1;
        }
        sprintf(command + strlen(command), " %s", buffer1);

    } else {
        sc_error("plot option not valid");
        return -1;
    }

    ui_pause();

    if (system(command) == -1)
        sc_error("Failed during plot - errno:%d", errno);
    getchar();
    ui_resume();

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(datafile);

    return 0;
#else
    sc_error("Gnuplot was not installed when building Sc-im. Please rebuild Sc-im.");
    return -1;
#endif
}
