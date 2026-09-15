#pragma once

#include <glad/glad.h>
#include <vector>

// Element Buffer Object - owns the GPU-side copy of an index array.
class EBO
{
public:
    GLuint ID = 0;
    GLsizei count = 0;   // number of indices, handy for glDrawElements

    explicit EBO(const std::vector<GLuint>& indices);

    void Bind() const;
    void Unbind() const;
    void Delete();
};
