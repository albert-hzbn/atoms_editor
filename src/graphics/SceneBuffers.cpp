#include "SceneBuffers.h"

void SceneBuffers::init(GLuint sphereVAO)
{
    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &colorVBO);
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    // Wire instance buffers into the sphere VAO so the atom draw call
    // picks them up as instanced attributes.
    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    // Wire the line VBO into the dedicated line VAO.
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);
}

void SceneBuffers::upload(const StructureInstanceData& data)
{
    atomCount     = data.positions.size();
    orbitCenter   = atomCount > 0 ? data.orbitCenter : glm::vec3(0.0f);
    boxLines      = data.boxLines;
    atomPositions = data.positions;
    atomColors    = data.colors;
    atomIndices   = data.atomIndices;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 data.positions.size() * sizeof(glm::vec3),
                 data.positions.empty() ? nullptr : data.positions.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 data.colors.size() * sizeof(glm::vec3),
                 data.colors.empty() ? nullptr : data.colors.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 data.boxLines.size() * sizeof(glm::vec3),
                 data.boxLines.empty() ? nullptr : data.boxLines.data(),
                 GL_STATIC_DRAW);
}

void SceneBuffers::highlightAtom(int idx, glm::vec3 color)
{
    if (idx < 0 || (size_t)idx >= atomCount)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    (GLintptr)(idx * (GLintptr)sizeof(glm::vec3)),
                    sizeof(glm::vec3),
                    &color);
}

void SceneBuffers::restoreAtomColor(int idx)
{
    if (idx < 0 || (size_t)idx >= atomColors.size())
        return;
    glm::vec3 orig = atomColors[idx];
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    (GLintptr)(idx * (GLintptr)sizeof(glm::vec3)),
                    sizeof(glm::vec3),
                    &orig);
}
