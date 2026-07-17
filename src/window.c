#include "../misc/glad.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>

static void error_callback(int error, const char* description);

GLFWwindow *window_handel;
/*
    TODO: use some sort of attrib structure for window settings
*/
void Window_Create(int width, int height, const char *title)
{
    glfwSetErrorCallback(error_callback);
    if(!glfwInit())
    {
        printf("failed to initialise glfw :( quitting");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_handel = glfwCreateWindow(width, height, title, NULL, NULL);
    if(!window_handel)
    {
        printf("failed to create window :( quitting");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window_handel);
    //glfwSetCursorPosCallback(window_handel, mouse_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize OpenGL context\n");
        exit(EXIT_FAILURE);
    }
    glfwSwapInterval(1);
}

GLFWwindow *Window_GetHandel()
{
    return window_handel;
}


void Window_GameLoop( void (*Graphics_ProcessShaders)(), void (*GameLoop)());
{
    int width, height;
    while(!glfwWindowShouldClose(window_handel))
    {
        glfwGetFramebufferSize(window_handel, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.2f, 0.3f,0.3f,1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        (*GameLoop)();
             
        glfwSwapBuffers(window_handel);
        glfwPollEvents();
    }

    glfwDestroyWindow(window_handel);
    glfwTerminate();
}


static void error_callback(int error, const char* description)
{
    fprintf(stderr, ":( Error: %s\n", description);
}