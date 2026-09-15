#pragma once

#include <glm/glm.hpp>

#include "EBO.h"
#include "Primitives.h"
#include "Shader.h"
#include "Texture.h"
#include "VAO.h"
#include "VBO.h"

// Owns one VAO/VBO/EBO triple and draws it with a given model matrix.
//
// A primitive is generated and uploaded ONCE at startup, then redrawn many
// times with different model matrices. Geometry is never regenerated inside
// the render loop.
class Mesh
{
public:
    explicit Mesh(const MeshData& data);

    // colour feeds the "objectColor" uniform, so one mesh serves every object
    // that shares its shape regardless of colour. texture is optional: pass
    // nullptr for surfaces that stay flat-coloured, such as the sun and the
    // lamp bulbs. colour is still what shows when textures are toggled off.
    void Draw(Shader& shader, const glm::mat4& model,
              const glm::vec3& color = glm::vec3(1.0f),
              const Texture* texture = nullptr) const;

    void Delete();

    GLsizei IndexCount() const { return ebo.count; }

    // Holds GL handles, so copying it would double-delete them.
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

private:
    // Declaration order is the construction order, and it matters here:
    // see the comment in Mesh.cpp.
    VAO vao;
    VBO vbo;
    EBO ebo;
};
