#include "SphereMesh.h"
#include <cmath>

static void createSphere(std::vector<float>& v,int stacks,int slices)
{
    for(int i=0;i<stacks;i++)
    {
        float lat0=M_PI*(-0.5f+i/(float)stacks);
        float lat1=M_PI*(-0.5f+(i+1)/(float)stacks);

        for(int j=0;j<=slices;j++)
        {
            float lng=2*M_PI*j/(float)slices;

            float x=cos(lng);
            float y=sin(lng);

            float z0=sin(lat0);
            float zr0=cos(lat0);

            float z1=sin(lat1);
            float zr1=cos(lat1);

            v.push_back(x*zr0);
            v.push_back(y*zr0);
            v.push_back(z0);

            v.push_back(x*zr1);
            v.push_back(y*zr1);
            v.push_back(z1);
        }
    }
}

SphereMesh::SphereMesh(int st,int sl)
{
    stacks=st;
    slices=sl;

    std::vector<float> vertices;
    createSphere(vertices,stacks,slices);

    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size()*sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);
}

void SphereMesh::draw()
{
    int stripSize=(slices+1)*2;

    for(int i=0;i<stacks;i++)
        glDrawArrays(GL_TRIANGLE_STRIP,i*stripSize,stripSize);
}