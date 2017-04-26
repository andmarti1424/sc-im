#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <errno.h>

#include "plot.h"
#include "file.h"
#include "tui.h"

int plot(char * s, int r, int c, int rf, int cf) {
    // create tmp file
    char datafile[] = "/tmp/sc-im-plotdataXXXXXX";
    int fd = mkstemp(datafile);
    if (fd == -1) {
        sc_error("Error while creating temp file for plot");
        return -1;
    }

    // export range to temp file in csv format
    export_delim(datafile, ',', r, c, rf, cf);

    // call gnuplot with  ~/.scim/plotline (or /usr/local/share/scim/plotline) and temp data file
    char command[BUFFERSIZE];
    sprintf(command, "gnuplot -e \"filename='%s'\"", datafile);
    if (! strcmp(s, "line")) {
        char buffer[PATHLEN];
        char buffer1[PATHLEN];
        sprintf(buffer, "plotline");
        if (! plugin_exists(buffer, strlen(buffer), buffer1)) {
            sc_error("could not load default plotline file");
            return -1;
        }
        /* sprintf(command + strlen(command), " %s", "~/.scim/plotline"); */
        sprintf(command + strlen(command), " %s", buffer1);

    } else {
        sc_error("plot option not valid");
        return -1;
    }
    //sc_debug(command);

    def_prog_mode();
    endwin();
    if (system(command) == -1)
        sc_error("Failed during plot - errno:%d", errno);
    getchar();
    reset_prog_mode();
    refresh();
    ui_update(TRUE);

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(datafile);

    return 0;
}
