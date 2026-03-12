#pragma once
#include <GLFW/glfw3.h>

class Camera
{
public:
    float rotX = 20.0f;
    float rotY = 30.0f;
    float zoom = -1.5f;

    bool mouseDown=false;
    double lastX=0,lastY=0;

    static Camera* instance;

    static void mouseButton(GLFWwindow*,int button,int action,int);
    static void cursor(GLFWwindow*,double x,double y);
    static void scroll(GLFWwindow*,double,double y);
};