
#include "Controls.h"
#include "imgui.h"

void drawControls(Camera& camera)
{
    ImGui::Begin("Controls");

    ImGui::SliderFloat("RotX",&camera.rotX,-180,180);
    ImGui::SliderFloat("RotY",&camera.rotY,-180,180);
    ImGui::SliderFloat("Zoom",&camera.zoom,-10,-1);

    ImGui::End();
}