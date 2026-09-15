#pragma once

#include <glad/glad.h>

#include "VBO.h"

// Vertex Array Object - records which buffer feeds which shader attribute.
class VAO
{
public:
    GLuint ID = 0;

    VAO();

    // One line per vertex attribute.
    void LinkAttrib(VBO& vbo, GLuint layout, GLint numComponents,
                    GLenum type, GLsizei stride, const void* offset) const;

    void Bind() const;
    void Unbind() const;
    void Delete();
};
