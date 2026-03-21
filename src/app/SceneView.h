#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;
struct Camera;
struct SceneBuffers;

struct FrameView
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    int windowWidth = 0;
    int windowHeight = 0;

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 lightMVP = glm::mat4(1.0f);
    glm::vec3 lightPosition = glm::vec3(0.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f);
};

bool updateViewport(GLFWwindow* window, FrameView& frame);

void buildFrameView(
    Camera& camera,
    const SceneBuffers& sceneBuffers,
    bool useOrthographicView,
    FrameView& frame);
