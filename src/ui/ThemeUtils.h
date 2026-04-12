#pragma once

#include "imgui.h"

inline bool isLightTheme()
{
    const ImVec4& bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    return (bg.x + bg.y + bg.z) / 3.0f > 0.5f;
}
