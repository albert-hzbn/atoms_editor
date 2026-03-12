#include "Shader.h"

static GLuint compile(GLenum type,const char* src)
{
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    return s;
}

GLuint createProgram(const char* vs,const char* fs)
{
    GLuint v=compile(GL_VERTEX_SHADER,vs);
    GLuint f=compile(GL_FRAGMENT_SHADER,fs);

    GLuint p=glCreateProgram();

    glAttachShader(p,v);
    glAttachShader(p,f);

    glBindAttribLocation(p,0,"position");

    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    return p;
}