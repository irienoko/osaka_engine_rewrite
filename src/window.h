#ifndef CC_WINDOW_H
#define CC_WINDOW_H

#include <GLFW/glfw3.h>


void Window_Create(int width, int height, const char *title);
GLFWwindow *Window_GetHandel();

/*Super basic Gameloop -- might change later*/
void Window_GameLoop(void (*GameLoop)());


#endif