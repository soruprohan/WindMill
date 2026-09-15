#include "Mesh.h"

#include <cstddef>   // offsetof

namespace
{
    // The EBO constructor binds GL_ELEMENT_ARRAY_BUFFER, and in a core profile
    // there is no default vertex array object - that binding is only legal,
    // and only recorded, while a VAO is bound.
    //
    // Member initialisers run before the constructor body, so the VAO has to
    // be bound from inside the initialiser list. This helper does that and
    // passes the vertices straight through.
    const std::vector<Vertex>& bindThenPass(const VAO& vao,
                                            const std::vector<Vertex>& vertices)
    {
        vao.Bind();
        return vertices;
    }
}

Mesh::Mesh(const MeshData& data)
    : vao(), vbo(bindThenPass(vao, data.vertices)), ebo(data.indices)
{
    const GLsizei stride = sizeof(Vertex);
    vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, position));
    vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, normal));
    vao.LinkAttrib(vbo, 2, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, texCoord));
    vao.LinkAttrib(vbo, 3, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, color));

    // Unbind the VAO first: unbinding the EBO while the VAO is still bound
    // would erase the element buffer from it.
    vao.Unbind();
    vbo.Unbind();
    ebo.Unbind();
}

void Mesh::Draw(Shader& shader, const glm::mat4& model, const glm::vec3& color) const
{
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);

    vao.Bind();
    glDrawElements(GL_TRIANGLES, ebo.count, GL_UNSIGNED_INT, 0);
    vao.Unbind();
}

void Mesh::Delete()
{
    vao.Delete();
    vbo.Delete();
    ebo.Delete();
}
