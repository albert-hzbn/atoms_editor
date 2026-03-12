#pragma once
#include <vector>
#include <GL/glew.h>

class SphereMesh
{
public:
    GLuint vao,vbo;
    int slices;
    int stacks;

    SphereMesh(int stacks,int slices);
    void draw();
};