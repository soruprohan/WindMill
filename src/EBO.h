#pragma once

#include <glad/glad.h>
#include <vector>

// Element Buffer Object - owns the GPU-side copy of an index array.
//
// instead of listing every triangle's three corners
// as full Vertex data (which repeats shared corners over and over), you list
// the unique vertices once in a VBO and then here just list, per triangle,
// WHICH three vertices (by index/number) it uses. An EBO is the GPU-side
// copy of that index list. Drawing with an EBO is what glDrawElements needs.
class EBO
{
public:
    GLuint ID = 0;
    GLsizei count = 0;   // how many indices there are - glDrawElements needs this number to know how much to draw

    explicit EBO(const std::vector<GLuint>& indices);

    void Bind() const;    // "use this index list" for the next draw call
    void Unbind() const;  // stop using it (careful - see the note in EBO.cpp)
    void Delete();        // frees the GPU memory this buffer used
};
