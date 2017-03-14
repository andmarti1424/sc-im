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
#include <lualib.h>                             /* Prototype for luaL_openlibs(), */
                                                /*   always include this when calling Lua */


#include <curses.h>
#include <unistd.h>
#include "sc.h"
#include "cmds.h"
#include "trigger.h"


extern FILE * fdoutput;

#define LC_NUMBER2(n,v)                     \
    static int l_ ## n(lua_State *L)       \
    {                                       \
        lua_pushnumber(L, v);               \
        return 1;                           \
    }


  lua_State *L;



void bail(lua_State *L, char *msg){
	fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
	exit(1);
}



static int l_getnum (lua_State *L) {
  int r,c;
  struct ent **pp;
  struct ent *p;
      c = lua_tointeger(L, 1);  /* get argument */
      r = lua_tointeger(L, 2);
      // printf("getnum !!\n");
      pp=ATBL(tbl,r,c);
     
      
      p=*pp;
      if(p==0) return;
      if (p->flags & is_valid)
	{
      
      lua_pushnumber(L, p->v);  /* push result */
      return 1;  /* number of results */
	} else return 0;

 }

static int l_setnum (lua_State *L) {
  int r,c;
  double val;
  struct ent **pp;
  struct ent *p;
      c = lua_tointeger(L, 1);  /* get argument */
      r = lua_tointeger(L, 2);
      val=lua_tonumber(L,3);
      //printf("getnum !!\n");
      
      p=lookat(r,c);
      p->v=val;
      p->flags |= is_changed |is_valid;
      p->flags &= ~iscleared;
            changed++;
            modflg++;
//            FullUpdate++;
            p->cellerror =CELLOK;

      return 0;

 }



static int l_setstr (lua_State *L) {
  int r,c;
  char * val;
  struct ent **pp;
  struct ent *p;
      c = lua_tointeger(L, 1);  /* get argument */
      r = lua_tointeger(L, 2);
      val=lua_tostring(L,3);
      //  printf("setstr !!\n");
       
      p=lookat(r,c);
	 label(p,val,-1);

      return 0;

 }


static int l_setform (lua_State *L) {
  int r,c;
  char * val;
  struct ent **pp;
  struct ent *p;
  char buf[256];
      c = lua_tointeger(L, 1);  /* get argument */
      r = lua_tointeger(L, 2);
      val=lua_tostring(L,3);
      //  printf("setstr !!\n");
     
    sprintf(buf,"LET %s%d=%s",coltoa(c),r,val);
     send_to_interpp(buf);
     
      return 0;
}

static int l_sc (lua_State *L) {

  char * val;
      
      val=lua_tostring(L,1);
      //  printf("setstr !!\n");
     
      send_to_interpp(val);
     
      return 0;
}


static int l_colrow2a(lua_State *L) {
  int c,r;
  char buf[16];
  
  c = lua_tointeger(L, 1);  /* get argument */
  r = lua_tointeger(L, 2);
  sprintf(buf,"%s%d", coltoa(c),r);
  lua_pushstring(L,buf);
  return 1;
  
}


static int l_colrow(lua_State *L) {

  char buf[16];
  char *val;
  int c,r;
  int ret,len;
  val=lua_tostring(L,1);
  printf("\n %s ", val);
  ret=sscanf(val,"%49[a-za-Z]%d",&buf,&r);
  printf("\scanf ret %d",ret);
  len=strlen(buf);	
  c = (toupper((int)buf[0])) - 'A';
  if (len == 2)               /* has second char */
  c = ((c + 1) * 26) + ((toupper((int)buf[1])) - 'A');
  lua_pushnumber(L,c);
  lua_pushnumber(L,r);
  return 2;
  
}


#if 1

char * query(char * initial_msg) {
    char * hline = (char *) malloc(sizeof(char) * BUFFERSIZE);
    hline[0]='\0';

    // curses is not enabled
    if ( atoi(get_conf_value("nocurses"))) {
        if (strlen(initial_msg)) wprintf(L"%s", initial_msg);

        if (fgetws(hline, BUFFERSIZE-1, stdin) == NULL)
            hline[0]='\0';

        clean_carrier(hline);
        return hline;
    }

    // curses is enabled
    int loading_o;
    if (loading) {
        loading_o=loading;
        loading=0;
        update(0);
        loading=loading_o;
    }
    curs_set(1);

    // show initial message
    if (strlen(initial_msg)) sc_info(initial_msg);

    // ask for input
    wtimeout(input_win, -1);
    wmove(input_win, 0, rescol);
    wclrtoeol(input_win);
    wrefresh(input_win);
    int d = wgetch(input_win);

    while (d != OKEY_ENTER && d != OKEY_ESC) {
        if (d == ERR) {
            d = wgetch(input_win);
            continue;
        }

        if (d == OKEY_BS || d == OKEY_BS2) {
            del_char(hline, strlen(hline) - 1);
        } else {
            sprintf(hline + strlen(hline), "%c", d);
        }

        mvwprintw(input_win, 0, rescol, "%s", hline);
        wclrtoeol(input_win);
        wrefresh(input_win);
        d = wgetch(input_win);
    }
    if (d == OKEY_ESC) hline[0]='\0';

    // go back to spreadsheet
    noecho();
    curs_set(0);
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtoeol(input_win);

    return hline;
}

int l_query (lua_State *L) {
    char * val;
    char * ret;

    val = lua_tostring(L,1);

    ret = query(val);
    printf("return of query:%s.\n", ret);
    /*
    if (ret == NULL)  return 0;
    lua_pushstring(L,ret);
    free(ret);
                        replace that with...
    */
    if (ret == '\0') {
        free(ret);
        return 0;
    }
    lua_pushstring(L,ret);
    free(ret);
    return 1;
}

#else

char * query(char * initial_msg) {

   int loading_o;
   
    if(loading) {
		loading_o=loading;
		loading=0;
   		update(0);
		loading=loading_o;
		}
    curs_set(1);
    char * hline = malloc(sizeof(char) * BUFFERSIZE);
    hline[0]='\0';

    // show initial message
    if (strlen(initial_msg)) sc_info(initial_msg);

    // ask for input
    wmove(input_win, 0, rescol);
    wclrtoeol(input_win);
    wrefresh(input_win);
    wtimeout(input_win, -1);
    int d = wgetch(input_win);
    while (d != OKEY_ENTER && d != OKEY_ESC) {
        if (d == OKEY_BS || d == OKEY_BS2) {
            del_char(hline, strlen(hline) - 1);
       } else {
            sprintf(hline + strlen(hline), "%c", d);
        }

        mvwprintw(input_win, 0, rescol, "%s", hline);
        wclrtoeol(input_win);
        wrefresh(input_win);
        d = wgetch(input_win);
    }
    if (d == OKEY_ESC) hline[0]='\0';

    // go back to spreadsheet
    noecho();
    curs_set(0);
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtoeol(input_win);

    if (strlen(hline)) return hline;
    else return NULL;
}


static int l_query (lua_State *L) {
  char * val;
  char * ret;
  val=lua_tostring(L,1);

  //  goraw();
  ret=query(val);
  // deraw(0);
  if(ret == NULL)  return 0;
  lua_pushstring(L,ret);
  free(ret);

  return 1;
}


#endif

LC_NUMBER2(currow,currow)
LC_NUMBER2(curcol,curcol)
LC_NUMBER2(maxcols,maxcols)
LC_NUMBER2(maxrows,maxrows)



static const luaL_reg sclib[] =
{
 { "lgetnum", l_getnum },
 { "lsetnum", l_setnum },
 { "lsetform", l_setform },
 { "lsetstr", l_setstr },
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
  


doLuainit()
{
  
 L = luaL_newstate();                        /* Create Lua state variable */
    luaL_openlibs(L);                           /* Load Lua libraries */


 luaL_register(L, "sc", sclib);

    if (luaL_loadfile(L, "init.lua")) /* Load but don't run the Lua script */
	bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */
      if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

    
    
}    

doLuaclose()
{
  lua_close(L);       
}

char *doLUA( struct enode *se)
{

    
    char * cmd;

    cmd=seval(NULL,se->e.o.left);

    if (luaL_loadfile(L, cmd)) /* Load but don't run the Lua script */
	bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */

    /* ABOVE HERE IS HELLO WORLD CODE */

    if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

    /*
//    lua_getglobal(L, "tellme");                 /* Tell what function to run */

   
    return 0;


  

}


doLuaTriger()
{



   if (luaL_loadfile(L, "trigger.lua")) /* Load but don't run the Lua script */
     return;
     //bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */


   if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

   
    lua_getglobal(L, "trigger");                 /* Tell what function to run */

    /* BELOW HERE IS THE HELLO WORLD CODE */
    //printf("In C, calling Lua\n");
    if (lua_pcall(L, 0, 0, 0))                  /* Run the function */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //printf("Back in C again\n");



}


doLuaTriger2(int row, int col, int flags)
{



   if (luaL_loadfile(L, "trigger.lua")) /* Load but don't run the Lua script */
     return;
     //bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */


   if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

   
    lua_getglobal(L, "trigger_cell");                 /* Tell what function to run */

    lua_pushinteger(L,col);
    lua_pushinteger(L,row);
    lua_pushinteger(L, flags);
    /* BELOW HERE IS THE HELLO WORLD CODE */
    //printf("In C, calling Lua\n");
    if (lua_pcall(L, 3, 0, 0))                  /* Run the function */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //printf("Back in C again\n");

}

/*
Lua trigger on a particular cell
we assume file and function ist correct other lua throught an error
*/ 
doLuaTrigger_cell(struct ent *p, int flags)
{
  int row,col;
  struct trigger *trigger=p->trigger;

  row=p->row;
  col=p->col;
 

   if (luaL_loadfile(L, trigger->file)) /* Load but don't run the Lua script */
     return;
     //bail(L, "luaL_loadfile() failed");      /* Error out if file can't be read */


   if (lua_pcall(L, 0, 0, 0))                  /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
        bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */

   
    lua_getglobal(L, trigger->function);                 /* Tell what function to run */

    lua_pushinteger(L,col);
    lua_pushinteger(L,row);
    lua_pushinteger(L, flags);
    /* BELOW HERE IS THE HELLO WORLD CODE */
    //printf("In C, calling Lua\n");
    if (lua_pcall(L, 3, 0, 0))                  /* Run the function */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //printf("Back in C again\n");



}
  
