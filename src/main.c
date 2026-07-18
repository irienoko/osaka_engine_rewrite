#include "window.h"
#include "graphics.h"
#include <stdio.h>

#include <lualib.h>
#include <lauxlib.h> 

/* TODO
    look into using  md files insted of obj for cooless factor
    add entity mesh creation
    figure out how texture arrays work
    imbed lua <- hell yeah
*/

/*
    Game loop gonna pass to lua later
    should prop rename to render loop and create a more robust one but this will do for now
*/
lua_State *L;

void GameLoop();
int main()
{
    L = luaL_newstate();

    if(L == NULL)
    {
        printf("lua failed to init exiting");
        return -1;
    }
    luaL_openlibs(L);
    luaL_dofile(L, "scripts/main.lua");
    lua_getglobal(L, "onLoad");
    if (lua_isfunction(L, -1))
    {
        lua_pcall(L, 0, 0, 0);
    }

    Window_Create(640, 480, "osaka engine rewrite");
    Graphics_CreateShaderProgram();
    Window_GameLoop(&GameLoop);

    lua_close(L);

    return 0;
}

void GameLoop()
{
    lua_getglobal(L, "onRun");
    if (lua_isfunction(L, -1))
    {
        lua_pcall(L, 0, 0, 0);
    }
}