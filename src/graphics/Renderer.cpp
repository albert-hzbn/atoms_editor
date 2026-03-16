#include "Renderer.h"
#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Shader sources
// ---------------------------------------------------------------------------

static const char* kAtomVS = R"(
    #version 130

    in vec3 position;
    in vec3 instancePos;
    in vec3 instanceColor;
    in float instanceScale;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 lightMVP;

    out vec3 fragColor;
    out vec4 FragPosLight;

    void main()
    {
        vec3 worldPos = position * instanceScale + instancePos;
        gl_Position   = projection * view * vec4(worldPos, 1.0);
        FragPosLight  = lightMVP * vec4(worldPos, 1.0);
        fragColor     = instanceColor;
    }
)";

static const char* kAtomFS = R"(
    #version 130

    in vec3 fragColor;
    in vec4 FragPosLight;

    uniform sampler2D shadowMap;

    out vec4 color;

    float computeShadow(vec4 pos)
    {
        vec3 proj = pos.xyz / pos.w;
        proj = proj * 0.5 + 0.5;

        if (proj.x < 0.0 || proj.x > 1.0 ||
            proj.y < 0.0 || proj.y > 1.0 ||
            proj.z < 0.0 || proj.z > 1.0)
            return 0.0;

        float closest = texture(shadowMap, proj.xy).r;
        float current = proj.z;
        float bias    = 0.003;

        return (current - bias > closest) ? 1.0 : 0.0;
    }

    void main()
    {
        float shadow  = computeShadow(FragPosLight);
        float ambient = 0.25;
        vec3 lighting = (ambient + (1.0 - ambient) * (1.0 - shadow)) * fragColor;
        color = vec4(lighting, 1.0);
    }
)";

static const char* kShadowVS = R"(
    #version 130

    in vec3 position;
    in vec3 instancePos;
    in float instanceScale;

    uniform mat4 lightMVP;

    void main()
    {
        vec3 worldPos    = position * instanceScale + instancePos;
        gl_Position      = lightMVP * vec4(worldPos, 1.0);
    }
)";

static const char* kShadowFS = R"(
    #version 130
    void main() {}
)";

static const char* kLineVS = R"(
    #version 130

    in vec3 position;

    uniform mat4 projection;
    uniform mat4 view;

    void main()
    {
        gl_Position = projection * view * vec4(position, 1.0);
    }
)";

static const char* kLineFS = R"(
    #version 130

    uniform vec3 uColor;
    out vec4 color;

    void main()
    {
        color = vec4(uColor, 1.0);
    }
)";

// ---------------------------------------------------------------------------
// Renderer implementation
// ---------------------------------------------------------------------------

void Renderer::init()
{
    atomProgram   = createProgram(kAtomVS,   kAtomFS);
    shadowProgram = createProgram(kShadowVS, kShadowFS);
    lineProgram   = createProgram(kLineVS,   kLineFS);
}

void Renderer::drawShadowPass(const ShadowMap& shadow,
                               const SphereMesh& sphere,
                               const glm::mat4& lightMVP,
                               size_t atomCount)
{
    beginShadowPass(shadow);

    glUseProgram(shadowProgram);
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightMVP"),
                       1, GL_FALSE, glm::value_ptr(lightMVP));

    glBindVertexArray(sphere.vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, sphere.vertexCount, (GLsizei)atomCount);

    endShadowPass();
}

void Renderer::drawAtoms(const glm::mat4& projection,
                          const glm::mat4& view,
                          const glm::mat4& lightMVP,
                          const ShadowMap& shadow,
                          const SphereMesh& sphere,
                          size_t atomCount)
{
    glUseProgram(atomProgram);

    glUniformMatrix4fv(glGetUniformLocation(atomProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(atomProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(atomProgram, "lightMVP"),
                       1, GL_FALSE, glm::value_ptr(lightMVP));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow.depthTexture);
    glUniform1i(glGetUniformLocation(atomProgram, "shadowMap"), 0);

    glBindVertexArray(sphere.vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, sphere.vertexCount, (GLsizei)atomCount);
}

void Renderer::drawBoxLines(const glm::mat4& projection,
                             const glm::mat4& view,
                             GLuint lineVAO,
                             size_t lineVertexCount)
{
    if (lineVertexCount == 0)
        return;

    glUseProgram(lineProgram);

    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(glGetUniformLocation(lineProgram, "uColor"), 0.85f, 0.85f, 0.85f);

    glLineWidth(2.0f);
    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, (GLsizei)lineVertexCount);
}
