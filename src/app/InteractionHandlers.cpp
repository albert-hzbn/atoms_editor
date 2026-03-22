#include "app/InteractionHandlers.h"

#include "Camera.h"
#include "app/EditorOps.h"
#include "graphics/Picking.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
bool isInsideSelectionRect(const ImVec2& p, const ImVec2& a, const ImVec2& b)
{
    const float minX = std::min(a.x, b.x);
    const float maxX = std::max(a.x, b.x);
    const float minY = std::min(a.y, b.y);
    const float maxY = std::max(a.y, b.y);
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

bool isCtrlHeld()
{
    return ImGui::GetIO().KeyCtrl;
}

void selectAllInstances(EditorState& state)
{
    state.selectedInstanceIndices.clear();
    for (int i = 0; i < (int)state.sceneBuffers.atomIndices.size(); ++i)
        state.selectedInstanceIndices.push_back(i);
}

void toggleSelectedInstance(EditorState& state, int pickedIndex)
{
    auto it = std::find(
        state.selectedInstanceIndices.begin(),
        state.selectedInstanceIndices.end(),
        pickedIndex);
    if (it != state.selectedInstanceIndices.end())
    {
        state.sceneBuffers.restoreAtomColor(pickedIndex);
        state.selectedInstanceIndices.erase(it);
        return;
    }

    state.selectedInstanceIndices.push_back(pickedIndex);
}

void selectSingleInstance(EditorState& state, int pickedIndex)
{
    clearSelection(state);
    state.selectedInstanceIndices.push_back(pickedIndex);
}

bool isDragLargeEnough(const ImVec2& start, const ImVec2& end)
{
    const float width = std::abs(end.x - start.x);
    const float height = std::abs(end.y - start.y);
    return width >= 4.0f && height >= 4.0f;
}

void drawSelectionRect(ImDrawList* drawList, const ImVec2& start, const ImVec2& end)
{
    const ImVec2 minCorner(std::min(start.x, end.x), std::min(start.y, end.y));
    const ImVec2 maxCorner(std::max(start.x, end.x), std::max(start.y, end.y));
    drawList->AddRectFilled(minCorner, maxCorner, IM_COL32(80, 160, 255, 45));
    drawList->AddRect(minCorner, maxCorner, IM_COL32(80, 160, 255, 220), 0.0f, 0, 1.5f);
}
}

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
    const bool ctrlHeld = isCtrlHeld();

    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !state.selectedInstanceIndices.empty())
        requests.doDeleteSelected = true;

    if (ImGui::IsKeyPressed(ImGuiKey_D) && ctrlHeld && !state.selectedInstanceIndices.empty())
        clearSelection(state);

    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !state.selectedInstanceIndices.empty())
        clearSelection(state);

    if (ImGui::IsKeyPressed(ImGuiKey_A) && ctrlHeld && !state.structure.atoms.empty())
        selectAllInstances(state);

    if (ImGui::IsKeyPressed(ImGuiKey_O) && ctrlHeld)
        state.fileBrowser.openFileDialog();

    if (ImGui::IsKeyPressed(ImGuiKey_S) && ctrlHeld && ImGui::GetIO().KeyShift)
        state.fileBrowser.exportImageDialog();

    if (ImGui::IsKeyPressed(ImGuiKey_S) && ctrlHeld && !ImGui::GetIO().KeyShift)
        state.fileBrowser.saveFileDialog();

    if (ImGui::IsKeyPressed(ImGuiKey_W) && ctrlHeld)
        state.fileBrowser.closeStructure();

    if (ImGui::IsKeyPressed(ImGuiKey_Z) && ctrlHeld && !ImGui::GetIO().KeyShift)
        requests.requestUndo = true;

    if ((ImGui::IsKeyPressed(ImGuiKey_Y) && ctrlHeld) ||
        (ImGui::IsKeyPressed(ImGuiKey_Z) && ctrlHeld && ImGui::GetIO().KeyShift))
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
    const bool isOrthographicProjection = std::abs(projection[3][3] - 1.0f) <= 1e-4f;
    const glm::vec3 rayOrigin = isOrthographicProjection
        ? pickRayOrigin(camera.clickX, camera.clickY, windowWidth, windowHeight, projection, view)
        : cameraPosition;
    glm::vec3 ray = pickRayDir(camera.clickX, camera.clickY, windowWidth, windowHeight, projection, view);
    int pickedIndex = pickAtom(
        rayOrigin,
        ray,
        state.sceneBuffers.atomPositions,
        state.sceneBuffers.atomRadii,
        1.0f);

    if (pickedIndex >= 0)
    {
        if (isCtrlHeld())
            toggleSelectedInstance(state, pickedIndex);
        else
            selectSingleInstance(state, pickedIndex);
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

    if (state.fileBrowser.isBoxSelectModeEnabled())
    {
        camera.pendingRightClick = false;
        return;
    }

    camera.pendingRightClick = false;
    if (!state.selectedInstanceIndices.empty())
        state.contextMenu.open();
}

void handleBoxSelection(
    EditorState& state,
    int windowWidth,
    int windowHeight,
    const glm::mat4& projection,
    const glm::mat4& view,
    ImDrawList* drawList)
{
    if (!state.fileBrowser.isBoxSelectModeEnabled())
        return;

    if (state.sceneBuffers.cpuCachesDisabled)
        return;

    static bool dragging = false;
    static ImVec2 dragStart(0.0f, 0.0f);
    static ImVec2 dragEnd(0.0f, 0.0f);

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mousePos = io.MousePos;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !io.WantCaptureMouse)
    {
        dragging = true;
        dragStart = mousePos;
        dragEnd = mousePos;
    }

    if (dragging && ImGui::IsMouseDown(ImGuiMouseButton_Right))
        dragEnd = mousePos;

    if (dragging)
        drawSelectionRect(drawList, dragStart, dragEnd);

    if (dragging && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        dragging = false;

        if (!isDragLargeEnough(dragStart, dragEnd))
            return;

        const bool additive = io.KeyCtrl;
        if (!additive)
            clearSelection(state);

        for (int i = 0; i < (int)state.sceneBuffers.atomPositions.size(); ++i)
        {
            const glm::vec3& worldPos = state.sceneBuffers.atomPositions[i];
            const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);
            if (clip.w <= 1e-6f)
                continue;

            const glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.z < -1.0f || ndc.z > 1.0f)
                continue;

            const ImVec2 screenPos(
                (ndc.x * 0.5f + 0.5f) * (float)windowWidth,
                (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)windowHeight);

            if (!isInsideSelectionRect(screenPos, dragStart, dragEnd))
                continue;

            auto it = std::find(state.selectedInstanceIndices.begin(), state.selectedInstanceIndices.end(), i);
            if (it == state.selectedInstanceIndices.end())
                state.selectedInstanceIndices.push_back(i);
        }
    }
}