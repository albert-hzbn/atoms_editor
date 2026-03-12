#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <vector>
#include <cmath>
#include <iostream>

float rotX = 20.0f;
float rotY = 30.0f;
float zoom = -3.0f;

bool mouseDown=false;
double lastX=0,lastY=0;

void mouse_button_callback(GLFWwindow*,int button,int action,int)
{
    if(button==GLFW_MOUSE_BUTTON_LEFT)
        mouseDown = action==GLFW_PRESS;
}

void cursor_position_callback(GLFWwindow*,double x,double y)
{
    if(mouseDown)
    {
        rotX += (y-lastY)*0.5f;
        rotY += (x-lastX)*0.5f;
    }

    lastX=x;
    lastY=y;
}

void scroll_callback(GLFWwindow*,double,double y)
{
    zoom += y*0.01f;
}

void perspective(float fovy,float aspect,float znear,float zfar,float* m)
{
    float f=1.0f/tan(fovy/2.0f);

    m[0]=f/aspect; m[1]=0; m[2]=0; m[3]=0;
    m[4]=0; m[5]=f; m[6]=0; m[7]=0;
    m[8]=0; m[9]=0; m[10]=(zfar+znear)/(znear-zfar); m[11]=-1;
    m[12]=0; m[13]=0; m[14]=(2*zfar*znear)/(znear-zfar); m[15]=0;
}

void createSphere(std::vector<float>& v,int stacks,int slices)
{
    for(int i=0;i<stacks;i++)
    {
        float lat0=M_PI*(-0.5f+i/(float)stacks);
        float lat1=M_PI*(-0.5f+(i+1)/(float)stacks);

        for(int j=0;j<=slices;j++)
        {
            float lng=2*M_PI*j/(float)slices;

            float x=cos(lng);
            float y=sin(lng);

            float z0=sin(lat0);
            float zr0=cos(lat0);

            float z1=sin(lat1);
            float zr1=cos(lat1);

            v.push_back(x*zr0);
            v.push_back(y*zr0);
            v.push_back(z0);

            v.push_back(x*zr1);
            v.push_back(y*zr1);
            v.push_back(z1);
        }
    }
}

GLuint compileShader(GLenum type,const char* src)
{
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    return s;
}

GLuint createProgram(const char* vs,const char* fs)
{
    GLuint v=compileShader(GL_VERTEX_SHADER,vs);
    GLuint f=compileShader(GL_FRAGMENT_SHADER,fs);

    GLuint p=glCreateProgram();

    glAttachShader(p,v);
    glAttachShader(p,f);

    glBindAttribLocation(p,0,"position");

    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    return p;
}

int main()
{
    glfwInit();

    const char* glsl_version="#version 130";

    GLFWwindow* window=glfwCreateWindow(1280,800,"GLSL Sphere",nullptr,nullptr);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetCursorPosCallback(window,cursor_position_callback);
    glfwSetScrollCallback(window,scroll_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glEnable(GL_DEPTH_TEST);

    std::vector<float> vertices;
    createSphere(vertices,40,40);

    GLuint vao,vbo;

    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size()*sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);

    const char* vs = R"(

    #version 130
    in vec3 position;
    uniform mat4 MVP;

    void main()
    {
        gl_Position = MVP * vec4(position,1.0);
    }

    )";

    const char* fs = R"(

    #version 130
    out vec4 color;

    void main()
    {
        color = vec4(0.8,0.3,0.3,1.0);
    }

    )";

    GLuint program=createProgram(vs,fs);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Controls");
        ImGui::SliderFloat("RotX",&rotX,-180,180);
        ImGui::SliderFloat("RotY",&rotY,-180,180);
        ImGui::SliderFloat("Zoom",&zoom,-10,-1);
        ImGui::End();

        ImGui::Render();

        int w,h;
        glfwGetFramebufferSize(window,&w,&h);

        glViewport(0,0,w,h);

        glClearColor(0.2f,0.2f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        float proj[16];
        perspective(45.0f*M_PI/180.0f,(float)w/h,0.1f,100.0f,proj);

        // Apply simple camera transform
        proj[12] = 0;
        proj[13] = 0;
        proj[14] += zoom;

        glUseProgram(program);

        GLuint mvpLoc=glGetUniformLocation(program,"MVP");
        glUniformMatrix4fv(mvpLoc,1,GL_FALSE,proj);

        glBindVertexArray(vao);

        // draw each strip separately
        int slices = 41;
        for(int i=0;i<40;i++)
        {
            glDrawArrays(GL_TRIANGLE_STRIP,i*slices*2,slices*2);
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}