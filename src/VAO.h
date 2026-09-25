#pragma once

#include <glad/glad.h>

#include "VBO.h"

// Vertex Array Object - records which buffer feeds which shader attribute.
//
// Plain-language version: a VBO is just raw bytes on the GPU - OpenGL
// doesn't automatically know that byte 0 is a position, byte 12 is a
// normal, and so on. A VAO is the "instruction sheet" that says: "attribute
// slot 0 (position) comes from this buffer, starting at this byte, repeating
// every N bytes; slot 1 (normal) comes from here; ..." Once that sheet is
// set up, binding the VAO alone is enough for the GPU to correctly read
// every attribute for a draw call - you don't have to explain it again.
class VAO
{
public:
    GLuint ID = 0;   // the "name" OpenGL gave this vertex array; 0 = doesn't exist yet

    VAO();   // asks OpenGL for a fresh, empty VAO

    // Tells the VAO "attribute number `layout` (matches `layout(location = N)`
    // in the shader) should read `numComponents` values of `type` from
    // `vbo`, spaced `stride` bytes apart, starting at byte `offset`."
    // One call per field of Vertex (position, normal, texCoord, color).
    void LinkAttrib(VBO& vbo, GLuint layout, GLint numComponents,
                    GLenum type, GLsizei stride, const void* offset) const;

    void Bind() const;    // "use this instruction sheet" for the next draw call
    void Unbind() const;  // stop using it
    void Delete();        // frees the GPU memory this VAO used
};
