#include "EBO.h"

EBO::EBO(const std::vector<GLuint>& indices)
    : count(static_cast<GLsizei>(indices.size()))
{
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(),
                 GL_STATIC_DRAW);
}

void EBO::Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); }

// Note: never unbind an EBO while a VAO is bound - the VAO stores the
// element buffer binding, so unbinding it would erase it from the VAO.
void EBO::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

void EBO::Delete()
{
    if (ID != 0) { glDeleteBuffers(1, &ID); ID = 0; }
}
