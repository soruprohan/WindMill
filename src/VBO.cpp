#include "VBO.h"

// Uploading data to the GPU is always the same three-step dance in OpenGL:
//   1. ask for a buffer "name"           (glGenBuffers)
//   2. select it as the current buffer   (glBindBuffer)
//   3. copy your data into it            (glBufferData)
// Every buffer class in this project (VBO, EBO) follows this exact pattern.
VBO::VBO(const std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &ID);                          // 1. get a fresh buffer ID from OpenGL
    glBindBuffer(GL_ARRAY_BUFFER, ID);              // 2. "select" this buffer as the active vertex buffer,GL_ARRAY_BUFFER means This buffer is going to contain vertex attributes.
    glBufferData(GL_ARRAY_BUFFER,                   // 3. copy the vertex data into it
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),   // total size in bytes
                 vertices.data(),                   // pointer to the actual data in CPU memory
                 GL_STATIC_DRAW);                   // hint: "uploaded once, drawn many times, never changes"
}

// Selects / deselects this buffer as the active GL_ARRAY_BUFFER. Binding to
// 0 in Unbind() is OpenGL's way of saying "nothing is selected".
void VBO::Bind()   const { glBindBuffer(GL_ARRAY_BUFFER, ID); }
void VBO::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

// Frees the buffer's memory on the GPU. Checks ID != 0 first so calling
// Delete() twice (or on a VBO that failed to create) does nothing harmful.
void VBO::Delete()
{
    if (ID != 0) { glDeleteBuffers(1, &ID); ID = 0; }
}
