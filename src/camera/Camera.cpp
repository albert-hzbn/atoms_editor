#include "Camera.h"

Camera* Camera::instance=nullptr;

void Camera::mouseButton(GLFWwindow*,int button,int action,int)
{
    if(button==GLFW_MOUSE_BUTTON_LEFT)
        instance->mouseDown = action==GLFW_PRESS;
}

void Camera::cursor(GLFWwindow*,double x,double y)
{
    if(instance->mouseDown)
    {
        instance->rotX += (y-instance->lastY)*0.5f;
        instance->rotY += (x-instance->lastX)*0.5f;
    }

    instance->lastX=x;
    instance->lastY=y;
}

void Camera::scroll(GLFWwindow*,double,double y)
{
    instance->zoom += y*0.001f;
}