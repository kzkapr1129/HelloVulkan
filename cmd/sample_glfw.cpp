#include "sample_glfw.h"

#include <iostream>
#include <GLFW/glfw3.h>

const uint32_t screenWidth = 640;
const uint32_t screenHeight = 480;

int SampleGLFW::execute() {
    if (!glfwInit())
        return -1;

    GLFWwindow* window;
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window){
        const char* err;
        glfwGetError(&err);
        std::cout << err << std::endl;
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
