#include "app/InteractionHandlers.h"

#include "Camera.h"
#include "app/EditorOps.h"
#include "graphics/Picking.h"
#include "imgui.h"

#include <algorithm>

FrameActionRequests beginFrameActionRequests(EditorState& state)
{
    FrameActionRequests requests;
    requests.requestMeasureDistance = state.fileBrowser.consumeMeasureDistanceRequest();
    requests.requestMeasureAngle = state.fileBrowser.consumeMeasureAngleRequest();
    requests.requestAtomInfo = state.fileBrowser.consumeAtomInfoRequest();
    requests.requestStructureInfo = state.fileBrowser.consumeStructureInfoRequest();
    return requests;
}

void applyKeyboardShortcuts(EditorState& state, FrameActionRequests& requests)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !state.selectedInstanceIndices.empty())
        requests.doDeleteSelected = true;

    if (ImGui::IsKeyPressed(ImGuiKey_D) && ImGui::GetIO().KeyCtrl && !state.selectedInstanceIndices.empty())
        clearSelection(state);

    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !state.selectedInstanceIndices.empty())
        clearSelection(state);

    if (ImGui::IsKeyPressed(ImGuiKey_A) && ImGui::GetIO().KeyCtrl && !state.structure.atoms.empty())
    {
        state.selectedInstanceIndices.clear();
        for (int i = 0; i < (int)state.sceneBuffers.atomIndices.size(); ++i)
            state.selectedInstanceIndices.push_back(i);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_O) && ImGui::GetIO().KeyCtrl)
        state.fileBrowser.openFileDialog();

    if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl)
        state.fileBrowser.saveFileDialog();

    if (ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
        requests.requestUndo = true;

    if ((ImGui::IsKeyPressed(ImGuiKey_Y) && ImGui::GetIO().KeyCtrl) ||
        (ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift))
    {
        requests.requestRedo = true;
    }
}

void handlePendingAtomPick(
    Camera& camera,
    EditorState& state,
    const glm::vec3& cameraPosition,
    int windowWidth,
    int windowHeight,
    const glm::mat4& projection,
    const glm::mat4& view)
{
    if (!camera.pendingClick)
        return;

    // Picking disabled for large structures (CPU caches not available)
    if (state.sceneBuffers.cpuCachesDisabled)
    {
        camera.pendingClick = false;
        return;
    }

    camera.pendingClick = false;
    glm::vec3 ray = pickRayDir(camera.clickX, camera.clickY, windowWidth, windowHeight, projection, view);
    int pickedIndex = pickAtom(
        cameraPosition,
        ray,
        state.sceneBuffers.atomPositions,
        state.sceneBuffers.atomRadii,
        1.0f);

    if (pickedIndex >= 0)
    {
        bool ctrlHeld = ImGui::GetIO().KeyCtrl;
        if (ctrlHeld)
        {
            auto it = std::find(
                state.selectedInstanceIndices.begin(),
                state.selectedInstanceIndices.end(),
                pickedIndex);
            if (it != state.selectedInstanceIndices.end())
            {
                state.sceneBuffers.restoreAtomColor(pickedIndex);
                state.selectedInstanceIndices.erase(it);
            }
            else
            {
                state.selectedInstanceIndices.push_back(pickedIndex);
            }
        }
        else
        {
            clearSelection(state);
            state.selectedInstanceIndices.push_back(pickedIndex);
        }
    }
    else
    {
        clearSelection(state);
    }
}

void handleRightClick(Camera& camera, EditorState& state)
{
    if (!camera.pendingRightClick)
        return;

    camera.pendingRightClick = false;
    if (!state.selectedInstanceIndices.empty())
        state.contextMenu.open();
}