#pragma once

#include <glad/glad.h>
#include <vector>

#include "Vertex.h"

// Vertex Buffer Object - owns the GPU-side copy of a vertex array.
//
// Plain-language version: your list of Vertex structs (positions, normals,
// UVs, colours) starts out living in normal CPU/RAM memory, as a
// std::vector. The graphics card can't read that directly - it needs its
// own copy sitting in GPU memory. A VBO is that copy: upload the vertices
// once when the shape is created, and the GPU just re-reads them from its
// own memory every time something is drawn. Nothing is re-uploaded per frame.
class VBO
{
public:
    GLuint ID = 0;   // the "name" OpenGL gave this buffer; 0 = doesn't exist yet

    // Uploads `vertices` to the GPU immediately. explicit stops the compiler
    // from silently converting some other vector into a VBO by accident.
    explicit VBO(const std::vector<Vertex>& vertices);

    void Bind() const;    // "use this buffer" - future GL calls act on it
    void Unbind() const;  // "stop using it" - unselects it
    void Delete();        // frees the GPU memory this buffer used
};