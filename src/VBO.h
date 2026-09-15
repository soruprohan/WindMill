#pragma once

#include <glad/glad.h>
#include <vector>

#include "Vertex.h"

// Vertex Buffer Object - owns the GPU-side copy of a vertex array.
class VBO
{
public:
    GLuint ID = 0;

    explicit VBO(const std::vector<Vertex>& vertices);

    void Bind() const;
    void Unbind() const;
    void Delete();
};
