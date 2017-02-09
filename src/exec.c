#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> // for wait
#include <errno.h>

#include "macros.h"
#include "conf.h"
#include "color.h"
#include "utils/string.h"
#include "screen.h"
#include "sc.h"
#include "main.h"     // exit_app

int exec_cmd (char * line) {
    int waitres;

    def_prog_mode();
    endwin();

    int my_pipe[2];
    if (pipe(my_pipe) == -1) {
        sc_error("Error creating pipe");
        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
        return -1;
    }

    pid_t child_id = fork();
    if (child_id == -1) {
        sc_error("Fork error");
        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
        return -1;
    }

    if (child_id == 0) {     // we are in the child process
        close(my_pipe[0]);   // child doesn't read
        dup2(my_pipe[1], 1); // redirect stdout

        int argc = 1;
        char * p;
        for (p = line; *p; p++)
            argc += (*p == ' ');
        char ** argv = calloc(argc+1, sizeof(char*));
        int i;
        for (i = 0; i < argc; i++)
            argv[i] = strsep(&line, " ");

        execvp(argv[0], argv);

        free(argv);
        sc_error("Error executing command. ");
        exit_app(-1);

    } else {                 // we are in parent process
        close(my_pipe[1]);   // parent doesn't write
        char reading_buf[2];

        while (read(my_pipe[0], reading_buf, 1) > 0) {
            if (!write(1, reading_buf, 1)) {
                sc_error("Failed to read command output: %s", strerror(errno));
                exit_app(-1);
            }
        }

        close(my_pipe[0]);
        wait(&waitres);
        if (system("echo -n 'Press ENTER to return.'") == -1) {
            sc_error("Failed to show ENTER prompt: %s", strerror(errno));
            exit_app(-1);
        }

        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
    }
    return 0;
}
