#include "window.h"
#include "graphics.h"
#include <stdio.h>

/*
    Game loop gonna pass to lua later
    should prop rename to render loop and create a more robust one but this will do for now
*/
void GameLoop()
{
    
}

int main()
{
    Window_Create(640, 480, "osaka engine rewrite");
    Graphics_CreateShaderProgram();
    Window_GameLoop(&GameLoop);
    return 0;
}