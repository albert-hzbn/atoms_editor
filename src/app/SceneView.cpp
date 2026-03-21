#include "app/SceneView.h"

#include "Camera.h"
#include "graphics/SceneBuffers.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
float estimateSceneRadius(const SceneBuffers& sceneBuffers)
{
    float maxRadius = 0.0f;

    if (!sceneBuffers.atomPositions.empty())
    {
        for (size_t i = 0; i < sceneBuffers.atomPositions.size(); ++i)
        {
            const float radius = (i < sceneBuffers.atomRadii.size()) ? sceneBuffers.atomRadii[i] : 0.0f;
            const float distance = glm::length(sceneBuffers.atomPositions[i] - sceneBuffers.orbitCenter) + radius;
            maxRadius = std::max(maxRadius, distance);
        }
    }
    else if (!sceneBuffers.boxLines.empty())
    {
        for (const glm::vec3& point : sceneBuffers.boxLines)
        {
            const float distance = glm::length(point - sceneBuffers.orbitCenter);
            maxRadius = std::max(maxRadius, distance);
        }
    }

    return std::max(maxRadius, 1.0f);
}
}

bool updateViewport(GLFWwindow* window, FrameView& frame)
{
    glfwGetFramebufferSize(window, &frame.framebufferWidth, &frame.framebufferHeight);
    if (frame.framebufferWidth == 0 || frame.framebufferHeight == 0)
    {
        glfwSwapBuffers(window);
        return false;
    }

    glfwGetWindowSize(window, &frame.windowWidth, &frame.windowHeight);
    if (frame.windowWidth == 0)
        frame.windowWidth = frame.framebufferWidth;
    if (frame.windowHeight == 0)
        frame.windowHeight = frame.framebufferHeight;

    return true;
}

void buildFrameView(
    Camera& camera,
    const SceneBuffers& sceneBuffers,
    bool useOrthographicView,
    FrameView& frame)
{
    const float aspect = (float)frame.framebufferWidth / (float)frame.framebufferHeight;
    const float verticalFov = glm::radians(45.0f);
    const float sceneRadius = estimateSceneRadius(sceneBuffers);
    const float depthPadding = std::max(10.0f, sceneRadius * 0.25f);

    if (useOrthographicView)
    {
        const float halfHeight = std::max(0.1f, camera.distance * std::tan(verticalFov * 0.5f));
        const float halfWidth = halfHeight * aspect;
        const float depthRange = std::max(1000.0f, camera.distance + sceneRadius + depthPadding);
        frame.projection = glm::ortho(
            -halfWidth,
            halfWidth,
            -halfHeight,
            halfHeight,
            -depthRange,
            depthRange);
    }
    else
    {
        const float nearestSurface = camera.distance - sceneRadius;
        const float nearClip = (nearestSurface > 0.0f)
            ? std::max(0.01f, nearestSurface * 0.25f)
            : 0.01f;
        const float farClip = std::max(nearClip + 100.0f, camera.distance + sceneRadius + depthPadding);

        frame.projection = glm::perspective(
            verticalFov,
            aspect,
            nearClip,
            farClip);
    }

    const float yaw = glm::radians(camera.yaw);
    const float pitch = glm::radians(camera.pitch);

    const glm::vec3 cameraOffset(
        camera.distance * std::cos(pitch) * std::sin(yaw),
        camera.distance * std::sin(pitch),
        camera.distance * std::cos(pitch) * std::cos(yaw));

    frame.cameraPosition = sceneBuffers.orbitCenter + cameraOffset;
    frame.view = glm::lookAt(frame.cameraPosition, sceneBuffers.orbitCenter, glm::vec3(0, 1, 0));

    const glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
    frame.lightPosition = sceneBuffers.orbitCenter + glm::vec3(40.0f, 40.0f, 40.0f);
    frame.lightMVP = lightProjection * glm::lookAt(
        frame.lightPosition,
        sceneBuffers.orbitCenter,
        glm::vec3(0, 1, 0));
}
