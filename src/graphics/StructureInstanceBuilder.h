#pragma once

#include "StructureLoader.h"

#include <glm/glm.hpp>
#include <vector>

struct StructureInstanceData
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float>     scales;
    std::vector<glm::vec3> boxLines;
    std::vector<int>       atomIndices; // maps each instance to its index in Structure::atoms
    glm::vec3 orbitCenter = glm::vec3(0.0f, 0.0f, 0.0f);
};

StructureInstanceData buildStructureInstanceData(
    const Structure& structure,
    bool useTransformMatrix,
    const int (&transformMatrix)[3][3],
    const std::vector<float>& elementRadii
);
