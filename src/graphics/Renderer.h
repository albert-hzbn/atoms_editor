#pragma once

#include "ShadowMap.h"
#include "SphereMesh.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <cstddef>

// Owns the three GLSL programs and exposes per-pass draw methods.
struct Renderer
{
    GLuint atomProgram   = 0;
    GLuint shadowProgram = 0;
    GLuint lineProgram   = 0;

    // Compile and link all shader programs.
    void init();

    // Render all atoms into the shadow map (depth-only pass).
    void drawShadowPass(const ShadowMap& shadow,
                        const SphereMesh& sphere,
                        const glm::mat4& lightMVP,
                        size_t atomCount);

    // Render all atoms into the colour buffer with shadow sampling.
    void drawAtoms(const glm::mat4& projection,
                   const glm::mat4& view,
                   const glm::mat4& lightMVP,
                   const ShadowMap& shadow,
                   const SphereMesh& sphere,
                   size_t atomCount);

    // Render the bounding-box / lattice wireframe.
    void drawBoxLines(const glm::mat4& projection,
                      const glm::mat4& view,
                      GLuint lineVAO,
                      size_t lineVertexCount);
};
