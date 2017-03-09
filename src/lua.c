#include <lua.h>                                /* Always include this when calling Lua */
#include <lauxlib.h>                            /* Always include this when calling Lua */
#include <lualib.h>                             /* Prototype for luaL_openlibs(), */
                                                /*   always include this when calling Lua */


#include <curses.h>
#include <unistd.h>
#include "sc.h"
#include "cmds.h"



static  lua_State *L;



void bail(lua_State *L, char *msg){
	fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
	exit(1);
}



static int l_getnum (lua_State *L) {
  int r,c;
  struct ent **pp;
  struct ent *p;
      r = lua_tointeger(L, 1);  /* get argument */
      c = lua_tointeger(L, 2);
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
      r = lua_tointeger(L, 1);  /* get argument */
      c = lua_tointeger(L, 2);
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
      r = lua_tointeger(L, 1);  /* get argument */
      c = lua_tointeger(L, 2);
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
      r = lua_tointeger(L, 1);  /* get argument */
      c = lua_tointeger(L, 2);
      val=lua_tostring(L,3);
      //  printf("setstr !!\n");
     
    sprintf(buf,"LET %s%d=%s",coltoa(c),r,val);
     send_to_interpp(buf);
     


      return 0;

 }

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


doLuainit()
{
  
 L = luaL_newstate();                        /* Create Lua state variable */
    luaL_openlibs(L);                           /* Load Lua libraries */

    lua_register(L,"lgetnum",l_getnum);
    lua_register(L,"lsetnum",l_setnum);
    lua_register(L,"lsetform",l_setform);
    lua_register(L,"lsetstr",l_setstr);
    lua_register(L,"lquery",l_query);

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

    lua_pushinteger(L,row);
    lua_pushinteger(L,col);
    lua_pushinteger(L, flags);
    /* BELOW HERE IS THE HELLO WORLD CODE */
    //printf("In C, calling Lua\n");
    if (lua_pcall(L, 3, 0, 0))                  /* Run the function */
	bail(L, "lua_pcall() failed");          /* Error out if Lua file has an error */
    //printf("Back in C again\n");



}
  
