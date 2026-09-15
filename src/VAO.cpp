#include "VAO.h"

VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

void VAO::LinkAttrib(VBO& vbo, GLuint layout, GLint numComponents,
                     GLenum type, GLsizei stride, const void* offset) const
{
    vbo.Bind();
    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    vbo.Unbind();
}

void VAO::Bind()   const { glBindVertexArray(ID); }
void VAO::Unbind() const { glBindVertexArray(0); }

void VAO::Delete()
{
    if (ID != 0) { glDeleteVertexArrays(1, &ID); ID = 0; }
}
