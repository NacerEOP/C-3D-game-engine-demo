#pragma once
#include <glad/glad.h>

class DebugQuad {
public:
    DebugQuad();
    ~DebugQuad();

    // Render using the supplied shader program and bound texture ID
    void render(unsigned int shaderProgram, unsigned int textureID);

private:
    unsigned int m_vao;
    unsigned int m_vbo;
};
