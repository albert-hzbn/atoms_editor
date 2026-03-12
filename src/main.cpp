#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.h"
#include "SphereMesh.h"
#include "Shader.h"
#include "MathUtils.h"
#include "Controls.h"

#include <iostream>
#include <cmath>

int main()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    const char* glsl_version = "#version 130";

    GLFWwindow* window = glfwCreateWindow(1280, 800, "GLSL Sphere", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed\n";
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    Camera camera;
    Camera::instance = &camera;

    glfwSetMouseButtonCallback(window, Camera::mouseButton);
    glfwSetCursorPosCallback(window, Camera::cursor);
    glfwSetScrollCallback(window, Camera::scroll);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glEnable(GL_DEPTH_TEST);

    SphereMesh sphere(500, 500);

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

    GLuint program = createProgram(vs, fs);

    if (!program)
    {
        std::cerr << "Shader program failed\n";
        return -1;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawControls(camera);

        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        glViewport(0, 0, w, h);

        glClearColor(0.2f,0.2f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float proj[16];
        perspective(45.0f * M_PI / 180.0f, (float)w / h, 0.1f, 100.0f, proj);

        proj[14] += camera.zoom;

        glUseProgram(program);

        GLuint mvpLoc = glGetUniformLocation(program, "MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, proj);

        glBindVertexArray(sphere.vao);
        sphere.draw();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}