#pragma once

#include <GL/glew.h>

class CylinderMesh
{
public:
    GLuint vao = 0;
    GLuint vbo = 0;
    int slices = 0;
    int vertexCount = 0;

    explicit CylinderMesh(int slices);
};