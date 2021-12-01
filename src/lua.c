/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
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
 * \file lua.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a brief file description.
 */

#ifdef XLUA
/*
 * R.Pollak
 * Lua Support for sc
 * called from sc via "@lua(file,0)" command
 * and interface for Lua Triggers
 *
 * how to use it see macro.lua or trigger.lua
 * At initialization of LUA, init.lua is called, this way global variables could be set.
 * Those are then accessible also  when called via @lua cmd or in triggers.
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
#include "cmds/cmds.h"
#include "trigger.h"
#include "utils/string.h"
#include "main.h"
#include "tui.h"
#include "conf.h"
#include "file.h"

extern struct session * session;
extern FILE * fdoutput;

#define LC_NUMBER2(n,v)                     \
    static int l_ ## n(lua_State *L)        \
    {                                       \
        lua_pushnumber(L, v);               \
        return 1;                           \
    }

lua_State * L = NULL;

/**
 * \brief TODO Document l_getnum()
 *
 * \param[in] L
 *
 * \return number of results; 0 otherwise
 */

static int l_getnum (lua_State *L) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c;
    struct ent **pp;
    struct ent *p;
    c = lua_tointeger(L, 1);      /* get argument */
    r = lua_tointeger(L, 2);
    // sc_debug("getnum !!");
    pp = ATBL(sh, sh->tbl, r, c);

    p = *pp;
    if (p == 0) return 0;
    if (p->flags & is_valid) {
        lua_pushnumber(L, p->v);  /* push result */
        return 1;                 /* number of results */
    } else return 0;
}

/**
 * \brief TODO Document l_setnum()
 *
 * \param[in] L
 *
 * \return 0 on success
 */

static int l_setnum (lua_State *L) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c;
    double val;
    //struct ent ** pp;
    struct ent *p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);
    val=lua_tonumber(L, 3);
    //sc_debug("getnum !!");

    p=lookat(sh, r,c);
    p->v=val;
    p->flags |= is_changed |is_valid;
    p->flags &= ~iscleared;
    roman->modflg++;
    p->cellerror =CELLOK;

    return 0;
}

/**
 * \brief TODO Document l_setstr
 *
 * \param[in] L
 *
 * \return 0 on success
 */

static int l_setstr (lua_State *L) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c;
    char * val;
    //struct ent ** pp;
    struct ent * p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);
    val=(char *) lua_tostring(L, 3);
    //sc_debug("setstr !!");

    p=lookat(sh, r,c);
    label(p,val,-1);

    return 0;
}


/**
 * \brief TODO Document l_getstr
 *
 * \return
 */

static int l_getstr (lua_State *L) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    int r, c;

    //struct ent ** pp;
    struct ent *p;
    c = lua_tointeger(L, 1);  /* get argument */
    r = lua_tointeger(L, 2);

    //sc_debug("setstr !!");

    p = lookat(sh, r, c);
    if (p == 0) return 0;
    if (p->label !=0) {
        lua_pushstring(L, p->label);
        return 1;
    }

    return 0;
}

/**
 * \brief TODO Document l_setform()
 *
 * \return none
 */

static int l_setform (lua_State *L) {
    int r, c;
    char * val;
    wchar_t buf[BUFFERSIZE];
    r = lua_tointeger(L, 1);  /* get argument */
    c = lua_tointeger(L, 2);
    val = (char *) lua_tostring(L,3);
    swprintf(buf, FBUFLEN, L"LET %s%d=%s", coltoa(c), r, val);
    send_to_interp(buf);
    return 0;
}

/**
 * \brief TODO Document l_sc
 *
 * \param[in] L
 * \return none
 */

static int l_sc (lua_State *L) {
    char * val = (char *) lua_tostring(L,1);
    wchar_t buf[BUFFERSIZE];
    swprintf(buf, FBUFLEN, L"%s", val);
    send_to_interp(buf);
    return 0;
}

/**
 * \brief TODO Document l_colrow2a()
 *
 * \param[in] L
 *
 * \return none
 */

static int l_colrow2a(lua_State *L) {
    int c, r;
    char buf[16];

    r = lua_tointeger(L, 2);  /* get argument */
    c = lua_tointeger(L, 1);
    sprintf(buf,"%s%d", coltoa(c),r);
    lua_pushstring(L,buf);
    return 1;
}

/**
 * \brief TODO Document l_colrow()
 *
 * \param[in] L
 *
 * \return none
 */

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

/**
 * \brief TODO Document l_query()
 *
 * \param[in] L
 *
 * \return none
 */

int l_query (lua_State *L) {
    char * val;
    char * ret;

    val = (char *)  lua_tostring(L,1);

    ret = ui_query(val);
    //sc_debug("return of query:%s.\n", ret);
    if (ret != NULL && ret[0] == '\0') {
        free(ret);
        return 0;
    }
    lua_pushstring(L,ret);
    free(ret);
    return 1;
}


LC_NUMBER2(currow, session->cur_doc->cur_sh->currow)
LC_NUMBER2(curcol, session->cur_doc->cur_sh->curcol)
LC_NUMBER2(maxcols, session->cur_doc->cur_sh->maxcols)
LC_NUMBER2(maxrows, session->cur_doc->cur_sh->maxrows)

static const luaL_Reg sclib[] = {
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

static int registerLuaFuncs(lua_State *L) {
#if LUA_VERSION_NUM >= 502
    luaL_newlib(L, sclib);                          /* Load SC specific LUA commands after init.lua exec*/
#endif
    return 1;
}

/**
 * \brief TODO Document doLuainit()
 *
 * \return none
 */

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


#if LUA_VERSION_NUM >= 502
    luaL_requiref(L, "sc", registerLuaFuncs, 1);    /* sc = require("sc") */
#else
    luaL_register(L, "sc", sclib);                  /* Load SC specific LUA commands after init.lua exec*/
#endif

    return;
}

/**
 * \brief TODO Document doLuaclose()
 *
 * \return none
 */

void doLuaclose() {
    if (L != NULL) lua_close(L);
    L = NULL;
    return;
}

/**
 * \brief TODO Document doLUA()
 *
 * \param[in] se
 *
 * \return none
 */

char * doLUA(struct sheet * sh, struct enode * se, int type) {
    if ( ! get_conf_int("exec_lua")) return 0;
    char * cmd;
    char buffer[PATHLEN];
    char buffer1[PATHLEN];
    cmd = seval(sh, NULL, se->e.o.left);

    sprintf(buffer, "lua/%s", cmd);
    if (plugin_exists(buffer, strlen(buffer), buffer1)) {
        if (luaL_loadfile(L, buffer1))              /* Load but don't run the Lua script */
            ui_bail(L, "luaL_loadfile() failed");   /* Error out if file can't be read */

        if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
            ui_bail(L, "lua_pcall() failed");       /* Error out if Lua file has an error */

       /* Tell what function to run */
       //    lua_getglobal(L, "tellme");
    }
    if (cmd != NULL) free(cmd);
    return 0;
}

/**
 * \brief TODO Document duLuaTriger
 *
 * \return none
 */

void doLuaTriger() {
    if ( ! get_conf_int("exec_lua")) return;
    if (luaL_loadfile(L, "trigger.lua"))         /* Load but don't run the Lua script */
        return;
    //ui_bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */

    if (lua_pcall(L, 0, 0, 0))                   /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        ui_bail(L, "lua_pcall() failed");        /* Error out if Lua file has an error */


    lua_getglobal(L, "trigger");                 /* Tell what function to run */

    //sc_debug("In C, calling Lua");
    if (lua_pcall(L, 0, 0, 0))                   /* Run the function */
        ui_bail(L, "lua_pcall() failed");        /* Error out if Lua file has an error */
    //sc_debug("Back in C again");
    return;
}

/**
 * \brief TODO Document doLuaTriger2()
 *
 * \param[in] row
 * \param[in] col
 * \param[in] flags
 *
 * \return none
 */

void doLuaTriger2(int row, int col, int flags) {
    if ( ! get_conf_int("exec_lua")) return;
    if (luaL_loadfile(L, "trigger.lua"))         /* Load but don't run the Lua script */
        return;
    //ui_bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */

    if (lua_pcall(L, 0, 0, 0))                   /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        ui_bail(L, "lua_pcall() failed");        /* Error out if Lua file has an error */

    lua_getglobal(L, "trigger_cell");            /* Tell what function to run */

    lua_pushinteger(L,col);
    lua_pushinteger(L,row);
    lua_pushinteger(L, flags);
    //sc_debug("In C, calling Lua");
    if (lua_pcall(L, 3, 0, 0))                   /* Run the function */
        ui_bail(L, "lua_pcall() failed");        /* Error out if Lua file has an error */
    //sc_debug("Back in C again");
    return;
}

/*
 * Lua trigger on a particular cell
 * we assume file and function ist correct other lua throught an error
 */
/**
 * \brief TODO Document doLuaTrigger_cell()
 *
 * \details Lua trigger on a particular cell. We assume file and
 * function ist correct. Otherwise, Lua throes an error
 *
 * \return: none
 */

void doLuaTrigger_cell(struct ent *p, int flags) {
    if ( ! get_conf_int("exec_lua")) return;
    int row,col;
    struct trigger *trigger = p->trigger;
    char buffer[PATHLEN];
    char buffer1[PATHLEN];

    row = p->row;
    col = p->col;

    sprintf(buffer,"lua/%s",trigger->file);
    if(plugin_exists(buffer,strlen(buffer),buffer1)) {
        if (luaL_loadfile(L, buffer1))           /* Load but don't run the Lua script */
            return;
        //ui_bail(L, "luaL_loadfile() failed");  /* Error out if file can't be read */


        if (lua_pcall(L, 0, 0, 0))               /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
            ui_bail(L, "lua_pcall() failed");    /* Error out if Lua file has an error */

        lua_getglobal(L, trigger->function);     /* Tell what function to run */

        lua_pushinteger(L,col);
        lua_pushinteger(L,row);
        lua_pushinteger(L, flags);
        //sc_debug("In C, calling Lua");
        if (lua_pcall(L, 3, 0, 0))               /* Run the function */
            ui_bail(L, "lua_pcall() failed");    /* Error out if Lua file has an error */
        //sc_debug("Back in C again");
   }
   return;
}
#endif
