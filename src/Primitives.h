#pragma once

#include <glad/glad.h>
#include <vector>

#include "Vertex.h"

// The raw CPU-side output of a generator, before it is uploaded to the GPU.
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

// ---------------------------------------------------------------------------
// Every generator returns a UNIT primitive centred on the origin, fitting
// inside a 1x1x1 box. Nothing here is sized for a particular object: the
// scene scales each one with a model matrix instead.
//
// That convention means glm::scale(vec3(w, h, d)) always produces exactly
// those dimensions, and one generated mesh serves the tower, the trunks, the
// lamp posts and the water wheel hub alike.
//
// Vertex colours are left white. Per-object colour comes from the
// "objectColor" uniform, so the same mesh can be drawn in any colour.
//
// Winding is counter-clockwise viewed from outside on every face, so
// back-face culling can stay enabled.
// ---------------------------------------------------------------------------
namespace Primitives
{
    // Axis-aligned cube, 24 vertices so each face keeps its own normal.
    MeshData makeCube();

    // Flat grid in the XZ plane at y = 0, normal +Y.
    // uvScale > 1 makes a texture tile rather than stretch (used by Phase 8).
    MeshData makePlane(int subdivisions = 1, float uvScale = 1.0f);

    // Y-axis cylinder with flat caps, spanning y = -0.5 .. +0.5.
    MeshData makeCylinder(int segments = 24);

    // Y-axis cone: base disc at y = -0.5, apex at y = +0.5.
    MeshData makeCone(int segments = 24);

    // UV sphere of diameter 1.
    MeshData makeSphere(int sectors = 24, int stacks = 16);

    // Triangular prism: gable cross-section in XY, ridge running along Z.
    // Used for the farmhouse roof.
    MeshData makePrism();
}
