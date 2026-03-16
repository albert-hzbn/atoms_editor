#include "ImGuiSetup.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void initImGui(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.0f;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.WindowPadding  = ImVec2(10, 10);
    style.FramePadding   = ImVec2(6, 4);
    style.ItemSpacing    = ImVec2(8, 6);
    style.ScrollbarSize  = 18.0f;
    style.GrabMinSize    = 10.0f;

    style.Colors[ImGuiCol_Header]        = ImVec4(0.12f, 0.55f, 0.76f, 0.52f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.50f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.60f, 0.90f, 0.60f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.06f, 0.53f, 0.83f, 0.80f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void shutdownImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
