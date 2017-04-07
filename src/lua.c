#ifdef XLUA
/*
  R.Pollak
  Lua Support for sc
  called from sc via "@lua(file,0)" command
  and interface for Lua Triggers

  how to use it see macro.lua or trigger.lua
  At initialization of LUA, init.lua is called, this way global variables could be set.
  Those are then accessible also  when called via @lua cmd or in triggers.
*/

#include <lua.h>                                /* Always include this when calling Lua */
#include <lauxlib.h>                            /* Always include this when calling Lua */
#include <lualib.h>                             /* Prototype for luaL_openlibs(),       */
                                                /* always include this when calling Lua */
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>

#include "sc.h"
#include "cmds.h"
#include "trigger.h"
#include "utils/string.h"
#include "main.h"
#include "tui.h"
#include "conf.h"

extern FILE * fdoutput;

#define LC_NUMBER2(n,v)                     \
    static int l_ ## n(lua_State *L)        \
    {                                       \
        lua_pushnumber(L, v);               \
        return 1;                           \
    }

lua_State *L;

static int l_getnum (lua_State *L) {
    int r,c;
    struct ent **pp;
    struct ent *p;
    c = lua_tointeger(L, 1);      /* get argument */
    r = lua_tointeger(L, 2);
    // sc_debug("getnum !!");
    pp = ATBL(tbl,r,c);

    p = *pp;
    if (p == 0) return 0;
    if (p->flags & is_valid) {
        lua_pushnumber(L, p->v);  /* push result */
        return 1;                 /* number of results */
    } else return 0;
}

static int l_setnum (lua_State *L) {
    int r,c;
    double val;
    //struct ent ** pp;
    struct ent *p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);
    val=lua_tonumber(L,3);
    sc_debug("getnum !!");

    p=lookat(r,c);
    p->v=val;
    p->flags |= is_changed |is_valid;
    p->flags &= ~iscleared;
    modflg++;
    p->cellerror =CELLOK;

    return 0;
}

static int l_setstr (lua_State *L) {
    int r,c;
    char * val;
    //struct ent ** pp;
    struct ent *p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);
    val=(char *) lua_tostring(L,3);
    //sc_debug("setstr !!");

    p=lookat(r,c);
    label(p,val,-1);

    return 0;
}


static int l_getstr (lua_State *L) {
    int r,c;

    //struct ent ** pp;
    struct ent *p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);

    //sc_debug("setstr !!");

    p=lookat(r,c);
    if(p == 0) return 0;
    if(p->label !=0) {
        lua_pushstring(L,p->label);
        return 1;
    }

    return 0;
}

static int l_setform (lua_State *L) {
    int r,c;
    char * val;
    wchar_t buf[BUFFERSIZE];
    r = lua_tointeger(L, 1);  /* get argument */
    c = lua_tointeger(L, 2);
    val = (char *) lua_tostring(L,3);
    swprintf(buf, FBUFLEN, L"LET %s%d=%s", coltoa(c), r, val);
    send_to_interp(buf);
    return 0;
}

static int l_sc (lua_State *L) {
    char * val = (char *) lua_tostring(L,1);
    wchar_t buf[BUFFERSIZE];
    swprintf(buf, FBUFLEN, L"%s", val);
    send_to_interp(buf);
    return 0;
}

static int l_colrow2a(lua_State *L) {
    int c, r;
    char buf[16];

    r = lua_tointeger(L, 1);  /* get argument */
    c = lua_tointeger(L, 2);
    sprintf(buf,"%s%d", coltoa(c),r);
    lua_pushstring(L,buf);
    return 1;
}

static int l_colrow(lua_State *L) {
    char buf[16];
    char *val;
    int c, r;
    int ret, len;
    val = (char *) lua_tostring(L,1);
    sc_debug(" %s ", val);
    ret = sscanf(val,"%49[a-za-Z]%d",buf,&r);
    sc_debug("scanf ret %d",ret);
    len=strlen(buf);
    c = (toupper((int)buf[0])) - 'A';
    if (len == 2)               /* has second char */
    c = ((c + 1) * 26) + ((toupper((int)buf[1])) - 'A');
    lua_pushnumber(L,c);
    lua_pushnumber(L,r);
    return 2;
}

int l_query (lua_State *L) {
    char * val;
    char * ret;

    val = (char *)  lua_tostring(L,1);

    ret = ui_query(val);
    //sc_debug("return of query:%s.\n", ret);
    if (ret == '\0') {
        free(ret);
        return 0;
    }
    lua_pushstring(L,ret);
    free(ret);
    return 1;
}

LC_NUMBER2(currow,currow)
LC_NUMBER2(curcol,curcol)
LC_NUMBER2(maxcols,maxcols)
LC_NUMBER2(maxrows,maxrows)

static const luaL_reg sclib[] = {
    { "lgetnum", l_getnum },
    { "lsetnum", l_setnum },
    { "lsetform", l_setform },
    { "lsetstr", l_setstr },
    { "lgetstr", l_getstr },
    { "lquery", l_query },
    { "currow", l_currow },
    { "curcol", l_curcol },
    { "maxcols", l_maxcols },
    { "maxrows", l_maxrows },
    { "a2colrow", l_colrow},
    { "colrow2a", l_colrow2a},
    { "sc", l_sc},
    {NULL,NULL}
};

void doLuainit() {
    char buffer[PATHLEN];
    char buffer1[PATHLEN];

    L = luaL_newstate();                            /* Create Lua state variable */
    luaL_openlibs(L);                               /* Load Lua libraries */

    sprintf(buffer,"lua/init.lua");
    if(plugin_exists(buffer,strlen(buffer),buffer1)) {
        if (luaL_loadfile(L, buffer1)) {            /* Load but don't run the Lua script */
            fprintf(stderr, "\nWarning :\n  Couldn't load init.lua: %s\n\n", lua_tostring(L,-1));
            return;
        }
        if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
            fprintf(stderr, "\nFATAL ERROR:\n  Couldn't initialized Lua: %s\n\n", lua_tostring(L,-1));
    }
    luaL_register(L, "sc", sclib);                  /* Load SC specific LUA commands after init.lua exec*/

    return;
}

void doLuaclose() {
    lua_close(L);
    return;
}

char * doLUA( struct enode * se) {
    char * cmd;
    char buffer[PATHLEN];
    char buffer1[PATHLEN];
    cmd = seval(NULL, se->e.o.left);

    sprintf(buffer,"lua/%s",cmd);
    if(plugin_exists(buffer,strlen(buffer),buffer1)) {
        if (luaL_loadfile(L, buffer1))              /* Load but don't run the Lua script */
            ui_bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */

        if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
            ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

       /* Tell what function to run */
       //    lua_getglobal(L, "tellme");
    }
    if (cmd != NULL) free(cmd);
    return 0;
}

void doLuaTriger() {
    if (luaL_loadfile(L, "trigger.lua"))         /* Load but don't run the Lua script */
        return;
    //ui_bail(L, "luaL_loadfile() failed");         /* Error out if file can't be read */

    if (lua_pcall(L, 0, 0, 0))                   /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        ui_bail(L, "lua_pcall() failed");           /* Error out if Lua file has an error */


    lua_getglobal(L, "trigger");                 /* Tell what function to run */

    //sc_debug("In C, calling Lua");
    if (lua_pcall(L, 0, 0, 0))                  /* Run the function */
        ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //sc_debug("Back in C again");
    return;
}

void doLuaTriger2(int row, int col, int flags) {
    if (luaL_loadfile(L, "trigger.lua"))        /* Load but don't run the Lua script */
        return;
    //ui_bail(L, "luaL_loadfile() failed");        /* Error out if file can't be read */

    if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

    lua_getglobal(L, "trigger_cell");           /* Tell what function to run */

    lua_pushinteger(L,col);
    lua_pushinteger(L,row);
    lua_pushinteger(L, flags);
    //sc_debug("In C, calling Lua");
    if (lua_pcall(L, 3, 0, 0))                  /* Run the function */
        ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //sc_debug("Back in C again");
    return;
}

/*
Lua trigger on a particular cell
we assume file and function ist correct other lua throught an error
*/
void doLuaTrigger_cell(struct ent *p, int flags) {
    int row,col;
    struct trigger *trigger = p->trigger;
    char buffer[PATHLEN];
    char buffer1[PATHLEN];

    row = p->row;
    col = p->col;

    sprintf(buffer,"lua/%s",trigger->file);
    if(plugin_exists(buffer,strlen(buffer),buffer1)) {
        if (luaL_loadfile(L, buffer1))        /* Load but don't run the Lua script */
            return;
        //ui_bail(L, "luaL_loadfile() failed");        /* Error out if file can't be read */


        if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
            ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

        lua_getglobal(L, trigger->function);        /* Tell what function to run */

        lua_pushinteger(L,col);
        lua_pushinteger(L,row);
        lua_pushinteger(L, flags);
        //sc_debug("In C, calling Lua");
        if (lua_pcall(L, 3, 0, 0))                  /* Run the function */
            ui_bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
        //sc_debug("Back in C again");
   }
   return;
}
#endif
