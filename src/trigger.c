/*
 * R.Pollak
 * This is general trigger support, Idea behind triggers, is when ever a value is changed triggers will be called on Write
 * and when ever a value will be evaluated a READ trigger will be fired
 * ent structure is extended with trigger structure
 *
 * Triggers need mode,type,file,function flags
 *   mode - can be R,W,RW
 *   type - C|LUA (later even SH)
 *   file - File name of the Trigger commands file
 *   function - Which function should be called for both Lua and C
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>      // for atoi
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

#include "sc.h"
#include "macros.h"
#include "utils/dictionary.h"
#include "utils/string.h"
#include "range.h"
#include "tui.h"
#include "undo.h"
#include "conf.h"
#include "cmds.h"
#include "trigger.h"

#ifdef XLUA
#include "lua.h"
#endif

void set_trigger(int r, int c, int rf, int cf, char * str) {
    if (any_locked_cells(r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    // parse detail
    // Create key-value dictionary for the content of the string
    struct dictionary * d = create_dictionary();

    // Remove quotes
    if (str[0] == '"') del_char(str, 0);
    if (str[strlen(str)-1] == '"') del_char(str, strlen(str)-1);

    parse_str(d, str);

    if (
        (get(d,"mode") == NULL) ||
        (get(d,"type") == NULL) ||
        (get(d,"file") == NULL) ||
        (get(d,"function") == NULL )) {
            sc_error("One of the values specified is wrong. Please parameters of the trigger to be set.");
            destroy_dictionary(d);
            return;
    }

    struct ent * n;
    int i, j;
    for (i = r; i <= rf; i++) {
        for (j = c; j <= cf; j++) {
            // action
            n = lookat(i, j);
            if (n->trigger == NULL)
                n->trigger = (struct trigger *) malloc(sizeof(struct trigger));
            else {
                free(n->trigger->file);
                free(n->trigger->function);
            }
            n->trigger->file = strdup(get(d,"file"));
            n->trigger->function = strdup(get(d,"function"));
            int tmp;
            if (strcmp(get(d,"mode"), "R") == 0) tmp = TRG_READ;
            if (strcmp(get(d,"mode"), "W") == 0) tmp = TRG_WRITE;
            if (strcmp(get(d,"mode"), "RW")== 0) tmp = TRG_READ | TRG_WRITE;
#ifdef XLUA
            if (strcmp(get(d,"type"), "LUA")== 0) tmp |= TRG_LUA;
#endif
            if (strcmp(get(d,"type"), "C")== 0) {
                char * error;
                char buffer[PATHLEN];
                char buffer1[PATHLEN];
                tmp |= TRG_C;
                sprintf(buffer, "module/%s", n->trigger->file);

                if(plugin_exists(buffer,strlen(buffer),buffer1))
                    n->trigger->handle=dlopen(buffer1,RTLD_LAZY);
                if(!n->trigger->handle) {
                    fputs (dlerror(), stderr);
                    exit(1);
                }
                n->trigger->c_function = dlsym(n->trigger->handle, n->trigger->function);
                if ((error = dlerror()) != NULL)  {
                    fputs(error, stderr);
                    exit(1);
                }
            }
            n->trigger->flag = tmp;
        }
    }
    destroy_dictionary(d);
    return;
}

void del_trigger(int r, int c, int rf, int cf ) {
    if (any_locked_cells(r, c, rf, cf)) {
        sc_error("Locked cells encountered. Nothing changed");
        return;
    }

    struct ent * n;
    int i, j;
    for (i = r; i <= rf; i++) {
        for (j = c; j <= cf; j++) {
           // action
           n = lookat(i, j);
           if (n->trigger != NULL ) {
               if ((n->trigger->flag & TRG_C) == TRG_C) {
                   dlclose(n->trigger->handle);
               }
               free(n->trigger->file);
               free(n->trigger->function);
               free(n->trigger);
               n->trigger = NULL;
           }
        }
    }
    return;
}

static int in_trigger = 0;

void do_trigger( struct ent *p , int rw) {
    struct trigger * trigger = p->trigger;
    if(in_trigger) return;
    in_trigger = 1;
#ifdef XLUA
    if ((trigger->flag & TRG_LUA ) == TRG_LUA)
        doLuaTrigger_cell(p,rw);
#endif
    if ((trigger->flag & TRG_C ) == TRG_C)
        do_C_Trigger_cell(p,rw);
    in_trigger = 0;
    return;
}

void do_C_Trigger_cell(struct ent * p, int rw) {
    int (*function)(struct ent *, int );

    function = p->trigger->c_function;
    printf ("%d\n", (*function)(p,rw ));
    return;
}

int plugin_exists(char * name, int len, char * path) {
    FILE * fp;
    static char * HomeDir;

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
