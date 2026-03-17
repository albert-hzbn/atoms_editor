#pragma once

struct GLFWwindow;
struct Camera;

GLFWwindow* createMainWindow();
void configureCameraCallbacks(GLFWwindow* window, Camera& camera);