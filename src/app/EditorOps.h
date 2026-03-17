#pragma once

#include "UndoRedo.h"
#include "app/EditorState.h"

struct Camera;

EditorSnapshot captureSnapshot(const EditorState& state);

void updateBuffers(EditorState& state);
void updateBuffers(EditorState& state, Structure& structure);

void applySnapshot(EditorState& state, const EditorSnapshot& snapshot);

void applyDefaultView(
    Camera& camera,
    const SceneBuffers& sceneBuffers,
    int viewportWidth,
    int viewportHeight,
    bool fitToStructure);

void clearSelection(EditorState& state);
void deleteSelectedAtoms(EditorState& state);
void refreshSelectionHighlights(EditorState& state);