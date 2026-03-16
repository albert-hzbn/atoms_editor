#pragma once

#include "StructureInstanceBuilder.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

// Owns the GPU buffers for instanced atom rendering and the bounding-box lines.
struct SceneBuffers
{
    GLuint instanceVBO = 0;
    GLuint colorVBO    = 0;
    GLuint lineVAO     = 0;
    GLuint lineVBO     = 0;

    size_t         atomCount   = 0;
    glm::vec3      orbitCenter = glm::vec3(0.0f);
    std::vector<glm::vec3> boxLines;

    // Allocate GPU objects and wire instance attributes into sphereVAO.
    void init(GLuint sphereVAO);

    // Upload a new StructureInstanceData set to the GPU and cache derived data.
    void upload(const StructureInstanceData& data);
};
