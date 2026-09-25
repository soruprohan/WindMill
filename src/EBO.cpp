#include "EBO.h"

// Same 3-step upload pattern as VBO (see VBO.cpp)(upload index data onto GPU buffer), just aimed at
// GL_ELEMENT_ARRAY_BUFFER - the special buffer type OpenGL uses for index
// lists rather than vertex data.
EBO::EBO(const std::vector<GLuint>& indices)
    : count(static_cast<GLsizei>(indices.size()))   // remember how many indices, for glDrawElements later
{
    glGenBuffers(1, &ID);                                  // 1. get a fresh buffer ID
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);              // 2. select it as the active index buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,                   // 3. copy the index data into it 
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(),
                 GL_STATIC_DRAW);                           // uploaded once, drawn many times
}

// Selects this buffer as the active index list.
void EBO::Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); }

// IMPORTANT:
// A VAO (see VAO.h) remembers "which index buffer goes with this shape" as
// part of its own state. If you unbind the EBO (select "no index buffer")
// while that VAO is still the active one, the VAO's memory of "use this EBO"
// gets overwritten with "use nothing". The shape would then draw as blank.
// So: only call Unbind() on an EBO AFTER you've already unbound the VAO.
void EBO::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

// Frees the buffer's GPU memory. ID != 0 check makes double-deleting safe.
void EBO::Delete()
{
    if (ID != 0) { glDeleteBuffers(1, &ID); ID = 0; }
}
