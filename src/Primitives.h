#pragma once

#include <glad/glad.h>
#include <vector>

#include "Vertex.h"

// The result of building one shape, before it's sent to the GPU.
// Just two plain CPU-side lists: the vertices, and which vertices make up
// each triangle (the indices).
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

// ---------------------------------------------------------------------------
// This file has one function per shape: makeCube, makePlane, makeCylinder,
// makeCone, makeSphere, makePrism. Each one builds that shape entirely in
// CPU memory and hands back a MeshData - no GPU calls happen in here at all.
//
// A few rules every shape follows:
//
// 1. Every shape is size 1 (fits inside a 1x1x1 box) and centred on (0,0,0).
//    Need it bigger? Scale it later with a model matrix. This way one
//    "cylinder" shape can become a windmill tower, a tree trunk, or a lamp
//    post - just by scaling it differently each time it's drawn.
//
// 2. Every vertex is coloured white. The actual colour you see comes from a
//    separate "objectColor" value set at draw time, not from the shape data.
//
// 3. Triangles are wound counter-clockwise when you look at them from
//    outside the shape. (This just needs to be consistent - it's what lets
//    the renderer safely skip drawing the backs of faces you can't see.)
// ---------------------------------------------------------------------------
namespace Primitives
{
    // A box. Has 24 vertices (not just 8 corners) because each of the 6
    // flat faces needs its own copy of the corners, so each face can point
    // in its own direction (its "normal").
    MeshData makeCube();

    // A flat square, lying down (normal points straight up).
    // uvScale controls how many times a texture repeats across it instead
    // of stretching once over the whole thing.
    MeshData makePlane(int subdivisions = 1, float uvScale = 1.0f);

    // Same as above, but lets you repeat the texture a different number of
    // times along each direction - useful for a long, thin shape like a river.
    MeshData makePlane(int subdivisions, float uvScaleU, float uvScaleV);

    // A can/tube shape, standing upright, with flat caps top and bottom.
    MeshData makeCylinder(int segments = 24);

    // An ice-cream-cone shape: flat circle at the bottom, pointed tip at top.
    // uvScaleU/V repeat the texture around and up the cone - a mountain
    // needs a lot of repeats so its rock texture doesn't look stretched.
    MeshData makeCone(int segments = 24, float uvScaleU = 1.0f, float uvScaleV = 1.0f);

    // A ball.
    //24 sectors → around the sphere
    // 16 stacks  → from bottom to top
    MeshData makeSphere(int sectors = 24, int stacks = 16);

    // A triangular tube - like a tent, or a house roof shape.
    MeshData makePrism();
}
