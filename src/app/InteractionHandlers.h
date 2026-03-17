#pragma once

#include "app/EditorState.h"

#include <glm/glm.hpp>

struct Camera;

struct FrameActionRequests
{
    bool doDeleteSelected = false;
    bool requestMeasureDistance = false;
    bool requestMeasureAngle = false;
    bool requestAtomInfo = false;
    bool requestStructureInfo = false;
    bool requestUndo = false;
    bool requestRedo = false;
};

FrameActionRequests beginFrameActionRequests(EditorState& state);
void applyKeyboardShortcuts(EditorState& state, FrameActionRequests& requests);

void handlePendingAtomPick(
    Camera& camera,
    EditorState& state,
    const glm::vec3& cameraPosition,
    int windowWidth,
    int windowHeight,
    const glm::mat4& projection,
    const glm::mat4& view);

void handleRightClick(Camera& camera, EditorState& state);