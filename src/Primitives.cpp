#include "Primitives.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace
{
    const glm::vec3 WHITE(1.0f, 1.0f, 1.0f);

    inline void addQuad(std::vector<GLuint>& indices, GLuint base)
    {
        indices.insert(indices.end(), { base + 0, base + 1, base + 2,
                                        base + 2, base + 3, base + 0 });
    }
}

namespace Primitives
{

// ---------------------------------------------------------------------------
// Cube
// ---------------------------------------------------------------------------
MeshData makeCube()
{
    MeshData m;
    const float h = 0.5f;

    struct Face { glm::vec3 normal; glm::vec3 corner[4]; };

    const Face faces[6] = {
        { {0,0,1},  { {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h} } },  // +Z
        { {0,0,-1}, { { h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h} } },  // -Z
        { {-1,0,0}, { {-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h} } },  // -X
        { {1,0,0},  { { h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h} } },  // +X
        { {0,-1,0}, { {-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h} } },  // -Y
        { {0,1,0},  { {-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h} } },  // +Y
    };

    const glm::vec2 uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    m.vertices.reserve(24);
    m.indices.reserve(36);

    for (int f = 0; f < 6; ++f)
    {
        GLuint base = static_cast<GLuint>(m.vertices.size());
        for (int c = 0; c < 4; ++c)
            m.vertices.push_back({ faces[f].corner[c], faces[f].normal, uv[c], WHITE });
        addQuad(m.indices, base);
    }
    return m;
}

// ---------------------------------------------------------------------------
// Plane
// ---------------------------------------------------------------------------
MeshData makePlane(int subdivisions, float uvScale)
{
    if (subdivisions < 1) subdivisions = 1;

    MeshData m;
    const int n = subdivisions;               // quads per side
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    for (int i = 0; i <= n; ++i)
    {
        float v = static_cast<float>(i) / static_cast<float>(n);
        for (int j = 0; j <= n; ++j)
        {
            float u = static_cast<float>(j) / static_cast<float>(n);
            m.vertices.push_back({ glm::vec3(u - 0.5f, 0.0f, v - 0.5f),
                                   up,
                                   glm::vec2(u * uvScale, v * uvScale),
                                   WHITE });
        }
    }

    const int row = n + 1;
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            GLuint a = static_cast<GLuint>(i * row + j);
            GLuint b = a + 1;
            GLuint c = a + row;
            GLuint d = c + 1;
            // CCW seen from +Y.
            m.indices.insert(m.indices.end(), { a, c, b, b, c, d });
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// Cylinder
//
// Side wall and caps do not share vertices: the wall normal points outward
// while the cap normal points along +/-Y, and one vertex cannot carry both.
// ---------------------------------------------------------------------------
MeshData makeCylinder(int segments)
{
    if (segments < 3) segments = 3;

    MeshData m;
    const float r = 0.5f;
    const float h = 0.5f;                       // half height
    const float twoPi = glm::two_pi<float>();

    // --- side wall: two rings, segments+1 vertices each for the UV seam ---
    GLuint sideBase = static_cast<GLuint>(m.vertices.size());
    for (int i = 0; i <= segments; ++i)
    {
        float t     = static_cast<float>(i) / static_cast<float>(segments);
        float theta = t * twoPi;
        float x = std::cos(theta);
        float z = std::sin(theta);
        glm::vec3 n(x, 0.0f, z);                // already unit length

        m.vertices.push_back({ glm::vec3(x * r, -h, z * r), n, glm::vec2(t, 0.0f), WHITE });
        m.vertices.push_back({ glm::vec3(x * r,  h, z * r), n, glm::vec2(t, 1.0f), WHITE });
    }
    for (int i = 0; i < segments; ++i)
    {
        GLuint b0 = sideBase + static_cast<GLuint>(i * 2);
        GLuint t0 = b0 + 1;
        GLuint b1 = b0 + 2;
        GLuint t1 = b0 + 3;
        m.indices.insert(m.indices.end(), { b0, t0, t1, t1, b1, b0 });
    }

    // --- caps: a centre vertex plus a ring, stitched as a triangle fan ---
    for (int cap = 0; cap < 2; ++cap)
    {
        float      y = (cap == 0) ? -h : h;
        glm::vec3  n = (cap == 0) ? glm::vec3(0, -1, 0) : glm::vec3(0, 1, 0);

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
            if (cap == 0) m.indices.insert(m.indices.end(), { centre, a, b });
            else          m.indices.insert(m.indices.end(), { centre, b, a });
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// Cone
//
// The side normal is not normalize(x, 0, z): the surface slopes, so the normal
// tilts upward. For base radius r and height H the outward normal at angle
// theta is normalize(H*cos, r, H*sin).
//
// The apex is emitted once per triangle rather than shared, because the
// normal there differs for every surrounding face.
// ---------------------------------------------------------------------------
MeshData makeCone(int segments)
{
    if (segments < 3) segments = 3;

    MeshData m;
    const float r = 0.5f;
    const float H = 1.0f;                       // full height
    const float h = 0.5f;                       // half height
    const float twoPi = glm::two_pi<float>();

    auto sideNormal = [&](float theta) {
        return glm::normalize(glm::vec3(H * std::cos(theta), r, H * std::sin(theta)));
    };

    // --- side ---
    for (int i = 0; i < segments; ++i)
    {
        float t0 = static_cast<float>(i)     / segments;
        float t1 = static_cast<float>(i + 1) / segments;
        float a0 = t0 * twoPi;
        float a1 = t1 * twoPi;
        float am = (a0 + a1) * 0.5f;

        glm::vec3 p0(std::cos(a0) * r, -h, std::sin(a0) * r);
        glm::vec3 p1(std::cos(a1) * r, -h, std::sin(a1) * r);
        glm::vec3 apex(0.0f, h, 0.0f);

        GLuint base = static_cast<GLuint>(m.vertices.size());
        m.vertices.push_back({ p0,   sideNormal(a0), glm::vec2(t0, 0.0f), WHITE });
        m.vertices.push_back({ apex, sideNormal(am), glm::vec2((t0 + t1) * 0.5f, 1.0f), WHITE });
        m.vertices.push_back({ p1,   sideNormal(a1), glm::vec2(t1, 0.0f), WHITE });
        m.indices.insert(m.indices.end(), { base, base + 1, base + 2 });
    }

    // --- base disc ---
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
// Sphere
//
// Centred on the origin, so the normal is simply the normalised position.
// ---------------------------------------------------------------------------
MeshData makeSphere(int sectors, int stacks)
{
    if (sectors < 3) sectors = 3;
    if (stacks  < 2) stacks  = 2;

    MeshData m;
    const float r = 0.5f;
    const float pi    = glm::pi<float>();
    const float twoPi = glm::two_pi<float>();

    for (int i = 0; i <= stacks; ++i)
    {
        float v   = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = pi * 0.5f - v * pi;         // +90 deg at the top
        float y   = r * std::sin(phi);
        float xz  = r * std::cos(phi);

        for (int j = 0; j <= sectors; ++j)
        {
            float u     = static_cast<float>(j) / static_cast<float>(sectors);
            float theta = u * twoPi;
            glm::vec3 p(xz * std::cos(theta), y, xz * std::sin(theta));
            m.vertices.push_back({ p, glm::normalize(p), glm::vec2(u, 1.0f - v), WHITE });
        }
    }

    const int row = sectors + 1;
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < sectors; ++j)
        {
            GLuint k1 = static_cast<GLuint>(i * row + j);       // upper ring
            GLuint k2 = k1 + static_cast<GLuint>(row);          // lower ring

            // The pole rings collapse to a point, so one triangle of each
            // quad there would be degenerate. Skip it.
            if (i != 0)
                m.indices.insert(m.indices.end(), { k1, k1 + 1, k2 });
            if (i != stacks - 1)
                m.indices.insert(m.indices.end(), { k1 + 1, k2 + 1, k2 });
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// Triangular prism (gable roof)
//
// Cross-section in XY: base corners at x = +/-0.5, y = -0.5, apex at (0, +0.5).
// Extruded along Z from -0.5 to +0.5, so the ridge runs along Z.
// ---------------------------------------------------------------------------
MeshData makePrism()
{
    MeshData m;
    const float h = 0.5f;

    const glm::vec3 lbf(-h, -h,  h), apf(0.0f, h,  h), rbf( h, -h,  h);  // front (+Z)
    const glm::vec3 lbb(-h, -h, -h), apb(0.0f, h, -h), rbb( h, -h, -h);  // back  (-Z)

    // Slopes are perpendicular to the edge (0.5, 1, 0), hence the 0.5 in Y.
    const glm::vec3 nLeft  = glm::normalize(glm::vec3(-1.0f, 0.5f, 0.0f));
    const glm::vec3 nRight = glm::normalize(glm::vec3( 1.0f, 0.5f, 0.0f));

    const glm::vec2 uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    auto quad = [&](const glm::vec3& a, const glm::vec3& b,
                    const glm::vec3& c, const glm::vec3& d, const glm::vec3& n)
    {
        GLuint base = static_cast<GLuint>(m.vertices.size());
        const glm::vec3 corner[4] = { a, b, c, d };
        for (int i = 0; i < 4; ++i)
            m.vertices.push_back({ corner[i], n, uv[i], WHITE });
        addQuad(m.indices, base);
    };

    quad(lbf, apf, apb, lbb, nLeft);                            // left slope
    quad(apf, rbf, rbb, apb, nRight);                           // right slope
    quad(lbb, rbb, rbf, lbf, glm::vec3(0.0f, -1.0f, 0.0f));     // underside

    // Gable ends.
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
