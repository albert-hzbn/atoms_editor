#include "app/WindowSetup.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"

GLFWwindow* createMainWindow()
{
    if (!glfwInit())
        return nullptr;

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Atoms Editor", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    return window;
}

void configureCameraCallbacks(GLFWwindow* window, Camera& camera)
{
    Camera::instance = &camera;
    glfwSetMouseButtonCallback(window, Camera::mouseButton);
    glfwSetCursorPosCallback(window, Camera::cursor);
    glfwSetScrollCallback(window, Camera::scroll);
}