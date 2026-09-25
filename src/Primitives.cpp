#include "Primitives.h"

#include <glm/gtc/constants.hpp>   // glm::pi<float>(), glm::two_pi<float>()

#include <cmath>   // std::cos, std::sin

namespace
{
    const glm::vec3 WHITE(1.0f, 1.0f, 1.0f);   // every vertex starts white; see Primitives.h rule 2

    // Most shapes are built from flat 4-cornered faces (quads), but a GPU
    // only draws triangles. This helper takes 4 vertices that were just
    // added (starting at index `base`) and writes the 6 indices needed to
    // split that square into 2 triangles.
    inline void addQuad(std::vector<GLuint>& indices, GLuint base)
    {
        indices.insert(indices.end(), { base + 0, base + 1, base + 2,
                                        base + 2, base + 3, base + 0 });
    }
}

namespace Primitives
{

// ---------------------------------------------------------------------------
// CUBE
//
// A cube has 6 flat faces. Each face needs to point in its own direction,
// so each face gets its own 4 corner vertices - even though geometrically
// the corners are shared between faces. That's why this is 24 vertices,
// not 8: 6 faces x 4 corners each.
// ---------------------------------------------------------------------------
MeshData makeCube()
{
    MeshData m;
    const float h = 0.5f;   // half the cube's size - corners sit at +/- h

    // One row per face: which way it points (normal), and its 4 corners,
    // listed in an order that winds counter-clockwise seen from outside.
    struct Face { glm::vec3 normal; glm::vec3 corner[4]; };

    const Face faces[6] = {
        { {0,0,1},  { {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h} } },  // front  (+Z)
        { {0,0,-1}, { { h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h} } },  // back   (-Z)
        { {-1,0,0}, { {-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h} } },  // left   (-X)
        { {1,0,0},  { { h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h} } },  // right  (+X)
        { {0,-1,0}, { {-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h} } },  // bottom (-Y)
        { {0,1,0},  { {-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h} } },  // top    (+Y)
    };

    // Same 4 texture corners for every face: bottom-left, bottom-right,
    // top-right, top-left.
    const glm::vec2 uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    m.vertices.reserve(24);
    m.indices.reserve(36);   // 6 faces x 2 triangles x 3 corners each

    for (int f = 0; f < 6; ++f)
    {
        GLuint base = static_cast<GLuint>(m.vertices.size()); //stores the base index for each face
        for (int c = 0; c < 4; ++c)
            m.vertices.push_back({ faces[f].corner[c], faces[f].normal, uv[c], WHITE });
        addQuad(m.indices, base);
    }
    return m;
}

// ---------------------------------------------------------------------------
// PLANE
//
// A flat grid lying on the ground (the XZ plane), facing straight up.
// `subdivisions` is how many small squares it's cut into per side - for a
// flat ground it can just be 1, but more subdivisions gives you more
// points to work with if you ever wanted to bend the surface.
// ---------------------------------------------------------------------------

// Two-argument version: same repeat amount both ways. Just forwards to the
// real version below.
MeshData makePlane(int subdivisions, float uvScale)
{
    return makePlane(subdivisions, uvScale, uvScale);
}

MeshData makePlane(int subdivisions, float uvScaleU, float uvScaleV)
{
    if (subdivisions < 1) subdivisions = 1;

    MeshData m;
    const int n = subdivisions;               // how many little squares per side
    const glm::vec3 up(0.0f, 1.0f, 0.0f);      // every point on a flat plane faces straight up

    // Walk a grid of (n+1) x (n+1) points, from corner to corner.
    for (int i = 0; i <= n; ++i)
    {
        float v = static_cast<float>(i) / static_cast<float>(n);   // 0 at one edge, 1 at the other
        for (int j = 0; j <= n; ++j)
        {
            float u = static_cast<float>(j) / static_cast<float>(n);
            m.vertices.push_back({ glm::vec3(u - 0.5f, 0.0f, v - 0.5f),   // -0.5 centres it on the origin
                                   up,
                                   glm::vec2(u * uvScaleU, v * uvScaleV),
                                   WHITE });
        }
    }

    // Now connect each little square of 4 neighbouring grid points into
    // 2 triangles.
    const int row = n + 1;   // how many points per row of the grid
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            GLuint a = static_cast<GLuint>(i * row + j);   // this square's 4 corners:
            GLuint b = a + 1;                              //   a b
            GLuint c = a + row;                            //   c d
            GLuint d = c + 1;
            m.indices.insert(m.indices.end(), { a, c, b, b, c, d });   // wound CCW seen from above
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// CYLINDER
//
// Think of it as 3 separate pieces glued together: the curved side wall,
// a flat circle capping the bottom, and a flat circle capping the top.
// These pieces can't share vertices even where they touch, because a wall
// vertex needs to point sideways (outward) while a cap vertex needs to
// point straight up or down - one vertex can only store one direction.
// ---------------------------------------------------------------------------
MeshData makeCylinder(int segments)
{
    if (segments < 3) segments = 3;   // need at least a triangle-ish shape

    MeshData m;
    const float r = 0.5f;    // radius
    const float h = 0.5f;    // half height (top is at +h, bottom at -h)
    const float twoPi = glm::two_pi<float>();   // a full circle, in radians (like 360 degrees)

    // --- the curved side wall ---
    // Walk all the way around the circle. At each step, place one point on
    // the bottom rim and one directly above it on the top rim.
    GLuint sideBase = static_cast<GLuint>(m.vertices.size());
    for (int i = 0; i <= segments; ++i)   // "<=" so the last point lands back on the first, closing the seam
    {
        float t     = static_cast<float>(i) / static_cast<float>(segments);   // 0 to 1 around the circle
        float theta = t * twoPi;                                              // 0 to 360 degrees, in radians
        float x = std::cos(theta);
        float z = std::sin(theta);
        glm::vec3 n(x, 0.0f, z);   // points straight outward from the centre - already length 1

        m.vertices.push_back({ glm::vec3(x * r, -h, z * r), n, glm::vec2(t, 0.0f), WHITE });   // bottom rim point
        m.vertices.push_back({ glm::vec3(x * r,  h, z * r), n, glm::vec2(t, 1.0f), WHITE });   // top rim point
    }
    // Stitch each pair of neighbouring (bottom, top) points into a quad.
    for (int i = 0; i < segments; ++i)
    {
        GLuint b0 = sideBase + static_cast<GLuint>(i * 2);   // this step's bottom point
        GLuint t0 = b0 + 1;                                   // this step's top point
        GLuint b1 = b0 + 2;                                   // next step's bottom point
        GLuint t1 = b0 + 3;                                   // next step's top point
        m.indices.insert(m.indices.end(), { b0, t0, t1, t1, b1, b0 });
    }

    // --- the two flat caps ---
    // A cap is a "fan" of triangles: one centre point, surrounded by a ring
    // of rim points, with a triangle between the centre and every pair of
    // neighbouring rim points - like slicing a pizza.
    for (int cap = 0; cap < 2; ++cap)
    {
        float      y = (cap == 0) ? -h : h;                                    // bottom cap or top cap
        glm::vec3  n = (cap == 0) ? glm::vec3(0, -1, 0) : glm::vec3(0, 1, 0);   // faces down or up

        GLuint centre = static_cast<GLuint>(m.vertices.size());
        m.vertices.push_back({ glm::vec3(0.0f, y, 0.0f), n, glm::vec2(0.5f, 0.5f), WHITE });

        for (int i = 0; i <= segments; ++i)
        {
            float theta = (static_cast<float>(i) / segments) * twoPi;
            float x = std::cos(theta);
            float z = std::sin(theta);
            m.vertices.push_back({ glm::vec3(x * r, y, z * r), n,
                                   glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f), WHITE });
        }

        for (int i = 0; i < segments; ++i)
        {
            GLuint a = centre + 1 + static_cast<GLuint>(i);
            GLuint b = a + 1;
            // Bottom and top caps need their triangles wound in opposite
            // order, because you're looking at them from opposite sides.
            if (cap == 0) m.indices.insert(m.indices.end(), { centre, a, b });
            else          m.indices.insert(m.indices.end(), { centre, b, a });
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// CONE
//
// Same idea as the cylinder's side wall, except instead of a top RING of
// points, every "top" point is the same single point - the tip (apex).
// Each one still needs its own copy though, because the cone's surface is
// slanted, so the direction it faces (its normal) is different at every
// angle around the tip - one shared tip vertex could only face one way.
//
// The normal is NOT simply "pointing straight out sideways" like the
// cylinder's - because the surface leans inward as it rises, the normal
// leans outward and slightly down-to-up too. The formula below is the
// correct tilted direction for a cone of height H and base radius r.
// ---------------------------------------------------------------------------
MeshData makeCone(int segments, float uvScaleU, float uvScaleV)
{
    if (segments < 3) segments = 3;

    MeshData m;
    const float r = 0.5f;    // base radius
    const float H = 1.0f;    // full height
    const float h = 0.5f;    // half height (base at -h, tip at +h)
    const float twoPi = glm::two_pi<float>();

    // The tilted outward-and-up direction the cone's surface faces at a
    // given angle around the circle.
    auto sideNormal = [&](float theta) {
        return glm::normalize(glm::vec3(H * std::cos(theta), r, H * std::sin(theta)));
    };

    // --- the sloped side ---
    // Walk around the base circle. At each step, place one point on the
    // base rim and one at the tip - both carrying this step's own normal.
    GLuint sideBase = static_cast<GLuint>(m.vertices.size());
    for (int i = 0; i <= segments; ++i)
    {
        float t     = static_cast<float>(i) / static_cast<float>(segments);
        float theta = t * twoPi;
        glm::vec3 n = sideNormal(theta);

        m.vertices.push_back({ glm::vec3(std::cos(theta) * r, -h, std::sin(theta) * r),
                               n, glm::vec2(t * uvScaleU, 0.0f), WHITE });       // base rim point
        m.vertices.push_back({ glm::vec3(0.0f, h, 0.0f),
                               n, glm::vec2(t * uvScaleU, uvScaleV), WHITE });   // tip (same spot, own normal)
    }
    for (int i = 0; i < segments; ++i)
    {
        GLuint b0 = sideBase + static_cast<GLuint>(i * 2);   // base point, this angle
        GLuint t0 = b0 + 1;                                   // tip point,  this angle
        GLuint b1 = b0 + 2;                                   // base point, next angle
        // Only 1 triangle here, not 2 - a cone slice is a triangle already
        // (2 base corners + the shared tip), there's no 4th corner to make a quad.
        m.indices.insert(m.indices.end(), { b0, t0, b1 });
    }

    // --- the flat base circle --- (a fan, same technique as the cylinder's cap)
    const glm::vec3 down(0.0f, -1.0f, 0.0f);
    GLuint centre = static_cast<GLuint>(m.vertices.size());
    m.vertices.push_back({ glm::vec3(0.0f, -h, 0.0f), down, glm::vec2(0.5f, 0.5f), WHITE });
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (static_cast<float>(i) / segments) * twoPi;
        float x = std::cos(theta);
        float z = std::sin(theta);
        m.vertices.push_back({ glm::vec3(x * r, -h, z * r), down,
                               glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f), WHITE });
    }
    for (int i = 0; i < segments; ++i)
    {
        GLuint a = centre + 1 + static_cast<GLuint>(i);
        m.indices.insert(m.indices.end(), { centre, a, a + 1 });
    }
    return m;
}

// ---------------------------------------------------------------------------
// SPHERE
//
// Built like a globe: horizontal rings of points from the north pole down
// to the south pole (like lines of latitude), each ring split into points
// going around (like lines of longitude). Because it's centred at (0,0,0),
// each point's outward direction (normal) is just that point's own
// position, shrunk down to length 1.
// ---------------------------------------------------------------------------
MeshData makeSphere(int sectors, int stacks)
{
    if (sectors < 3) sectors = 3;   // "sectors" = slices around (longitude)
    if (stacks  < 2) stacks  = 2;   // "stacks"  = rings top to bottom (latitude)

    MeshData m;
    const float r = 0.5f;
    const float pi    = glm::pi<float>();
    const float twoPi = glm::two_pi<float>();

    // One ring of points per "stack", from the top pole down to the bottom.
    for (int i = 0; i <= stacks; ++i)
    {
        float v   = static_cast<float>(i) / static_cast<float>(stacks);   // 0 at top, 1 at bottom
        float phi = pi * 0.5f - v * pi;         // angle above/below the equator: +90 deg at the very top
        float y   = r * std::sin(phi);          // height of this ring
        float xz  = r * std::cos(phi);          // radius of this ring (widest at the equator, ~0 at the poles)

        for (int j = 0; j <= sectors; ++j)
        {
            float u     = static_cast<float>(j) / static_cast<float>(sectors);   // 0 to 1 around the ring
            float theta = u * twoPi;
            glm::vec3 p(xz * std::cos(theta), y, xz * std::sin(theta));
            m.vertices.push_back({ p, glm::normalize(p), glm::vec2(u, 1.0f - v), WHITE });
        }
    }

    // Connect each ring to the ring below it.
    const int row = sectors + 1;   // points per ring
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < sectors; ++j)
        {
            GLuint k1 = static_cast<GLuint>(i * row + j);       // point on the upper ring
            GLuint k2 = k1 + static_cast<GLuint>(row);          // point directly below it, on the lower ring

            // Right at the very top and bottom, the ring has shrunk down to
            // a single point (the pole), so one of the two triangles in a
            // normal quad would have zero width there. Skip that one.
            if (i != 0)
                m.indices.insert(m.indices.end(), { k1, k1 + 1, k2 });
            if (i != stacks - 1)
                m.indices.insert(m.indices.end(), { k1 + 1, k2 + 1, k2 });
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// PRISM (used as the farmhouse roof)
//
// Picture a triangle (like the letter A) drawn flat, then stretched
// backward to make a 3D tent/roof shape. The triangle has a flat bottom and
// a peak in the middle; stretching it back gives you: 2 sloped rectangles
// (the roof panels), a flat rectangle underneath, and 2 triangular ends.
// ---------------------------------------------------------------------------
MeshData makePrism()
{
    MeshData m;
    const float h = 0.5f;

    // The 6 corners of the shape: l/r = left/right, b = base (bottom),
    // ap = apex (the peak), f/b = front/back.
    const glm::vec3 lbf(-h, -h,  h), apf(0.0f, h,  h), rbf( h, -h,  h);  // front triangle (+Z side)
    const glm::vec3 lbb(-h, -h, -h), apb(0.0f, h, -h), rbb( h, -h, -h);  // back triangle  (-Z side)

    // Which way each sloped roof panel faces. (0.5 in the Y here comes from
    // the roof's slope - a 45-degree-ish tilt outward and upward.)
    const glm::vec3 nLeft  = glm::normalize(glm::vec3(-1.0f, 0.5f, 0.0f));
    const glm::vec3 nRight = glm::normalize(glm::vec3( 1.0f, 0.5f, 0.0f));

    const glm::vec2 uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    // Small helper just for this function: given 4 corners (in order) and
    // which way they face, add them as one flat rectangular face.
    auto quad = [&](const glm::vec3& a, const glm::vec3& b,
                    const glm::vec3& c, const glm::vec3& d, const glm::vec3& n)
    {
        GLuint base = static_cast<GLuint>(m.vertices.size());
        const glm::vec3 corner[4] = { a, b, c, d };
        for (int i = 0; i < 4; ++i)
            m.vertices.push_back({ corner[i], n, uv[i], WHITE });
        addQuad(m.indices, base);
    };

    quad(lbf, apf, apb, lbb, nLeft);                            // left roof panel
    quad(apf, rbf, rbb, apb, nRight);                           // right roof panel
    quad(lbb, rbb, rbf, lbf, glm::vec3(0.0f, -1.0f, 0.0f));     // flat underside

    // The two triangular end walls (front and back) - just 3 corners each,
    // written out directly since there's no quad() shortcut for a triangle.
    GLuint base = static_cast<GLuint>(m.vertices.size());
    const glm::vec3 nFront(0.0f, 0.0f, 1.0f);
    m.vertices.push_back({ lbf, nFront, glm::vec2(0.0f, 0.0f), WHITE });
    m.vertices.push_back({ rbf, nFront, glm::vec2(1.0f, 0.0f), WHITE });
    m.vertices.push_back({ apf, nFront, glm::vec2(0.5f, 1.0f), WHITE });
    m.indices.insert(m.indices.end(), { base, base + 1, base + 2 });

    base = static_cast<GLuint>(m.vertices.size());
    const glm::vec3 nBack(0.0f, 0.0f, -1.0f);
    m.vertices.push_back({ rbb, nBack, glm::vec2(0.0f, 0.0f), WHITE });
    m.vertices.push_back({ lbb, nBack, glm::vec2(1.0f, 0.0f), WHITE });
    m.vertices.push_back({ apb, nBack, glm::vec2(0.5f, 1.0f), WHITE });
    m.indices.insert(m.indices.end(), { base, base + 1, base + 2 });

    return m;
}

} // namespace Primitives
