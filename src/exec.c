#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> // for wait

#include "macros.h"
#include "conf.h"
#include "color.h"
#include "utils/string.h"
#include "screen.h"
#include "sc.h"

int exec_cmd (char * line) {
    int waitres;

    def_prog_mode();
    endwin();

    int my_pipe[2];
    if (pipe(my_pipe) == -1) {
        scerror("Error creating pipe");
        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
        return -1;
    }

    pid_t child_id = fork();
    if (child_id == -1) {
        scerror("Fork error");
        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
        return -1;
    }

    if (child_id == 0) {     // we are in the child process
        close(my_pipe[0]);   // child doesn't read
        dup2(my_pipe[1], 1); // redirect stdout

        char * l = line;
        int j;
        l = rtrim(ltrim(line, ' '), ' ');
        char ** param = split(l, ' ', 1);
        execvp(param[0], param);

        for (j=0; param[j]; j++) {
            free(param[j]);
        }
        free(param);
        param = NULL;

        printf("Error executing command. ");
        exit(-1);

    } else {                 // we are in parent process
        close(my_pipe[1]);   // parent doesn't write
        char reading_buf[2];

        while (read(my_pipe[0], reading_buf, 1) > 0)
            write(1, reading_buf, 1);
        
        close(my_pipe[0]);
        wait(&waitres);
        system("echo -n 'Press ENTER to return.'");

        getchar();
        reset_prog_mode();
        refresh();
        update(TRUE);
    }
    return 0;
}
