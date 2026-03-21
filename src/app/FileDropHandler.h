#pragma once

struct GLFWwindow;
struct EditorState;

void installDropFileCallback(GLFWwindow* window, EditorState& state);
void processDroppedFiles(EditorState& state);
