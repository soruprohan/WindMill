#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

// ---------------------------------------------------------------------------
// Flat placeholder palette. Phase 8 replaces most of these with textures and
// Phase 9 starts lighting them properly.
// ---------------------------------------------------------------------------
namespace
{
    const glm::vec3 C_GRASS   (0.38f, 0.58f, 0.28f);
    const glm::vec3 C_TOWER   (0.84f, 0.80f, 0.72f);
    const glm::vec3 C_HEAD    (0.44f, 0.29f, 0.19f);
    const glm::vec3 C_BLADE   (0.93f, 0.91f, 0.86f);
    const glm::vec3 C_HUB     (0.30f, 0.22f, 0.16f);
    const glm::vec3 C_WALL    (0.90f, 0.84f, 0.72f);
    const glm::vec3 C_ROOF    (0.62f, 0.27f, 0.20f);
    const glm::vec3 C_DOOR    (0.35f, 0.22f, 0.14f);
    const glm::vec3 C_WINDOW  (0.55f, 0.74f, 0.85f);
    const glm::vec3 C_BARK    (0.40f, 0.26f, 0.16f);
    const glm::vec3 C_LEAF_LO (0.18f, 0.42f, 0.20f);
    const glm::vec3 C_LEAF_HI (0.24f, 0.52f, 0.25f);
    const glm::vec3 C_WOOD    (0.55f, 0.38f, 0.22f);
    const glm::vec3 C_WOOD_D  (0.42f, 0.28f, 0.16f);
    const glm::vec3 C_FENCE   (0.74f, 0.60f, 0.42f);
    const glm::vec3 C_METAL   (0.26f, 0.26f, 0.30f);
    const glm::vec3 C_BULB    (1.00f, 0.93f, 0.68f);
    const glm::vec3 C_SUN     (1.00f, 0.90f, 0.45f);

    // Scene dimensions.
    const float GROUND_SIZE  = 60.0f;
    const float FARM_EXTENT  = 18.0f;   // fence boundary, +/- on X and Z

    // Windmill proportions.
    const float TOWER_HEIGHT = 5.0f;
    const float TOWER_DIA    = 1.7f;
    const glm::vec3 HEAD_SIZE(1.5f, 1.3f, 1.9f);
    const float BLADE_LENGTH = 3.8f;
}

glm::mat4 Transform::matrix() const
{
    glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
    m = glm::rotate(m, glm::radians(rotationDeg.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotationDeg.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rotationDeg.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, scale);
    return m;
}

// ---------------------------------------------------------------------------
// Construction: every primitive is generated exactly once here.
// ---------------------------------------------------------------------------
Scene::Scene()
    : cube    (Primitives::makeCube()),
      plane   (Primitives::makePlane(8, 20.0f)),
      cylinder(Primitives::makeCylinder(28)),
      cone    (Primitives::makeCone(28)),
      sphere  (Primitives::makeSphere(28, 18)),
      prism   (Primitives::makePrism())
{
    // --- windmills ---
    windmills.push_back({ glm::vec3(-9.0f, 0.0f, -7.0f), 1.00f, -25.0f,   0.0f });
    windmills.push_back({ glm::vec3( 9.5f, 0.0f, -9.5f), 0.78f,  30.0f,  47.0f });

    // --- farmhouse ---
    house.position    = glm::vec3(1.0f, 0.0f, 7.5f);
    house.rotationDeg = glm::vec3(0.0f, -18.0f, 0.0f);

    // --- water wheel, clear of the house so its shape reads ---
    // y is the axle height; the support posts run from the ground up to it.
    wheelPos = glm::vec3(-14.0f, 2.6f, 8.5f);

    // --- trees: varied positions and scales ---
    const float treeData[8][4] = {
        // x       z      scale   yaw
        { -15.0f,  2.0f,  1.15f,  20.0f },
        { -13.0f, -13.0f, 0.90f, 140.0f },
        {  -4.0f, -15.0f, 1.05f,  70.0f },
        {   6.0f,  13.0f, 0.85f, 200.0f },
        {  14.0f,   4.0f, 1.20f,  10.0f },
        {  15.5f, -15.0f, 0.95f, 250.0f },
        {  -6.0f,  14.5f, 1.00f, 300.0f },
        {  12.0f, -2.0f,  0.72f,  95.0f },
    };
    for (const auto& t : treeData)
    {
        Transform tr;
        tr.position    = glm::vec3(t[0], 0.0f, t[1]);
        tr.scale       = glm::vec3(t[2]);
        tr.rotationDeg = glm::vec3(0.0f, t[3], 0.0f);
        trees.push_back(tr);
    }

    // --- lamp posts ---
    lampPosts = {
        glm::vec3(-5.0f, 0.0f,  2.0f),
        glm::vec3( 5.5f, 0.0f,  2.0f),
        glm::vec3(-5.0f, 0.0f, 13.0f),
        glm::vec3( 5.5f, 0.0f, 13.0f),
    };

    // Bulb positions are recorded once here, not rebuilt every frame.
    // Phase 10 places a point light at each of these.
    const float poleHeight = 3.4f;
    for (const auto& p : lampPosts)
        lampBulbs.push_back(p + glm::vec3(0.0f, poleHeight + 0.22f, 0.0f));

    // --- sun ---
    // High enough to read as an afternoon sun, low enough to stay in frame.
    sunPos = glm::vec3(26.0f, 19.0f, -30.0f);
}

void Scene::Update(float deltaTime)
{
    if (paused) return;

    bladeAngle = std::fmod(bladeAngle + bladeSpeed * deltaTime, 360.0f);
    wheelAngle = std::fmod(wheelAngle + wheelSpeed * deltaTime, 360.0f);
}

void Scene::AdjustHeadYaw(float degrees)
{
    for (auto& w : windmills)
        w.yawDeg += degrees;
}

void Scene::Draw(Shader& shader)
{
    drawGround(shader);
    for (const auto& w : windmills) drawWindmill(shader, w);
    drawHouse(shader, house);
    drawWaterWheel(shader, wheelPos);
    for (const auto& t : trees)     drawTree(shader, t);
    drawFence(shader);
    for (const auto& p : lampPosts) drawLampPost(shader, p);
    drawSun(shader);
}

void Scene::Delete()
{
    cube.Delete();
    plane.Delete();
    cylinder.Delete();
    cone.Delete();
    sphere.Delete();
    prism.Delete();
}

// ---------------------------------------------------------------------------
void Scene::drawGround(Shader& shader)
{
    glm::mat4 m = glm::scale(glm::mat4(1.0f),
                             glm::vec3(GROUND_SIZE, 1.0f, GROUND_SIZE));
    plane.Draw(shader, m, C_GRASS);
}

// ---------------------------------------------------------------------------
// THE WINDMILL - the nested transformation hierarchy this project is built on.
//
//   base                        translate to the windmill's spot
//    +-- tower                  cylinder, offset up by half its height
//    +-- headPivot              translate to tower top, then rotate about Y
//         +-- head housing      cube
//         +-- hubPivot          translate forward, then rotate about Z
//              +-- hub cap      cylinder lying along Z
//              +-- blade x4     each a further 90 degrees about Z
//
// hubPivot is built FROM headPivot, so changing yawDeg swings all four blades
// with the head, while bladeAngle moves only the blades. That parent-child
// inheritance is the whole point of the project.
// ---------------------------------------------------------------------------
void Scene::drawWindmill(Shader& shader, const Windmill& w)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), w.position);
    base = glm::scale(base, glm::vec3(w.scale));

    // --- tower ---
    // A unit cylinder is centred on its origin, so translating up by half the
    // height before scaling is what puts its base on the ground rather than
    // sinking half of it below. The same trick is used for trunks and poles.
    glm::mat4 tower = glm::translate(base, glm::vec3(0.0f, TOWER_HEIGHT * 0.5f, 0.0f));
    tower = glm::scale(tower, glm::vec3(TOWER_DIA, TOWER_HEIGHT, TOWER_DIA));
    cylinder.Draw(shader, tower, C_TOWER);

    // --- head: yaws about its own vertical pivot at the top of the tower ---
    glm::mat4 headPivot = glm::translate(base, glm::vec3(0.0f, TOWER_HEIGHT, 0.0f));
    headPivot = glm::rotate(headPivot, glm::radians(w.yawDeg), glm::vec3(0, 1, 0));
    cube.Draw(shader, glm::scale(headPivot, HEAD_SIZE), C_HEAD);

    // --- hub: inherits the yaw, then adds its own independent spin ---
    glm::mat4 hubPivot = glm::translate(headPivot,
                                        glm::vec3(0.0f, 0.0f, HEAD_SIZE.z * 0.5f));
    hubPivot = glm::rotate(hubPivot, glm::radians(bladeAngle + w.bladePhase),
                           glm::vec3(0, 0, 1));

    // Hub cap: the cylinder's axis is Y, so tip it 90 degrees about X to lie
    // along Z and face the viewer.
    glm::mat4 hub = glm::rotate(hubPivot, glm::radians(90.0f), glm::vec3(1, 0, 0));
    hub = glm::scale(hub, glm::vec3(0.55f, 0.45f, 0.55f));
    cylinder.Draw(shader, hub, C_HUB);

    // --- four blades, each a further 90 degrees around the hub axis ---
    for (int i = 0; i < 4; ++i)
    {
        glm::mat4 blade = glm::rotate(hubPivot, glm::radians(90.0f * i),
                                      glm::vec3(0, 0, 1));
        blade = glm::translate(blade, glm::vec3(0.0f, BLADE_LENGTH * 0.5f, 0.0f));
        blade = glm::scale(blade, glm::vec3(0.22f, BLADE_LENGTH, 0.07f));
        cube.Draw(shader, blade, C_BLADE);
    }
}

// ---------------------------------------------------------------------------
void Scene::drawHouse(Shader& shader, const Transform& t)
{
    const glm::vec3 bodySize(6.0f, 3.0f, 4.6f);
    const glm::vec3 roofSize(6.7f, 2.1f, 5.1f);

    glm::mat4 base = t.matrix();

    // Body.
    glm::mat4 body = glm::translate(base, glm::vec3(0.0f, bodySize.y * 0.5f, 0.0f));
    cube.Draw(shader, glm::scale(body, bodySize), C_WALL);

    // Roof: the prism's ridge runs along Z, so it slopes over the 6-unit width.
    glm::mat4 roof = glm::translate(base,
                        glm::vec3(0.0f, bodySize.y + roofSize.y * 0.5f, 0.0f));
    prism.Draw(shader, glm::scale(roof, roofSize), C_ROOF);

    // Door and windows sit on the front face (+Z), pushed out by 0.01 so they
    // do not z-fight with the wall they are coplanar with.
    const float face = bodySize.z * 0.5f + 0.01f;

    glm::mat4 door = glm::translate(base, glm::vec3(0.0f, 1.05f, face));
    cube.Draw(shader, glm::scale(door, glm::vec3(1.1f, 2.1f, 0.12f)), C_DOOR);

    for (float x : { -2.0f, 2.0f })
    {
        glm::mat4 win = glm::translate(base, glm::vec3(x, 1.95f, face));
        cube.Draw(shader, glm::scale(win, glm::vec3(1.1f, 0.95f, 0.12f)), C_WINDOW);
    }
}

// ---------------------------------------------------------------------------
// The water wheel is a second, simpler hierarchy: one pivot, and every paddle
// and spoke hangs off it.
// ---------------------------------------------------------------------------
void Scene::drawWaterWheel(Shader& shader, const glm::vec3& pos)
{
    const int   paddles = 10;
    const float radius  = 2.3f;
    const float width   = 1.3f;                        // axial, along Z
    const float step    = 360.0f / static_cast<float>(paddles);

    // Chord between neighbouring rim points, so the rim segments meet.
    const float chord = 2.0f * radius * std::sin(glm::radians(step * 0.5f));

    // Support posts. These are NOT on the pivot - they hold the axle up while
    // the wheel turns inside them.
    for (float z : { -(width * 0.5f + 0.3f), width * 0.5f + 0.3f })
    {
        glm::mat4 post = glm::translate(glm::mat4(1.0f),
                             glm::vec3(pos.x, pos.y * 0.5f, pos.z + z));
        post = glm::scale(post, glm::vec3(0.32f, pos.y, 0.32f));
        cube.Draw(shader, post, C_WOOD_D);
    }

    glm::mat4 pivot = glm::translate(glm::mat4(1.0f), pos);
    pivot = glm::rotate(pivot, glm::radians(-wheelAngle), glm::vec3(0, 0, 1));

    // Axle, lying along Z like the windmill hub.
    glm::mat4 hub = glm::rotate(pivot, glm::radians(90.0f), glm::vec3(1, 0, 0));
    hub = glm::scale(hub, glm::vec3(0.65f, width + 0.9f, 0.65f));
    cylinder.Draw(shader, hub, C_WOOD_D);

    for (int i = 0; i < paddles; ++i)
    {
        float a = step * static_cast<float>(i);
        glm::mat4 arm = glm::rotate(pivot, glm::radians(a), glm::vec3(0, 0, 1));

        // Spoke: offset out by half its length before scaling, so it runs from
        // the hub to the rim instead of straddling the centre.
        glm::mat4 spoke = glm::translate(arm, glm::vec3(0.0f, radius * 0.5f, 0.0f));
        spoke = glm::scale(spoke, glm::vec3(0.14f, radius, 0.14f));
        cube.Draw(shader, spoke, C_WOOD);

        // Rim segment, sitting halfway between two spokes. After the rotation
        // the cube's local X is tangential, so scaling X by the chord closes
        // the ring into a polygon.
        glm::mat4 rim = glm::rotate(pivot, glm::radians(a + step * 0.5f),
                                    glm::vec3(0, 0, 1));
        rim = glm::translate(rim, glm::vec3(0.0f, radius, 0.0f));
        rim = glm::scale(rim, glm::vec3(chord * 1.02f, 0.16f, width));
        cube.Draw(shader, rim, C_WOOD);

        // Paddle, standing proud of the rim.
        glm::mat4 paddle = glm::translate(arm, glm::vec3(0.0f, radius + 0.3f, 0.0f));
        paddle = glm::scale(paddle, glm::vec3(0.5f, 0.75f, width));
        cube.Draw(shader, paddle, C_WOOD_D);
    }
}

// ---------------------------------------------------------------------------
void Scene::drawTree(Shader& shader, const Transform& t)
{
    const float trunkHeight = 2.2f;
    const float trunkDia    = 0.45f;

    glm::mat4 base = t.matrix();

    glm::mat4 trunk = glm::translate(base, glm::vec3(0.0f, trunkHeight * 0.5f, 0.0f));
    trunk = glm::scale(trunk, glm::vec3(trunkDia, trunkHeight, trunkDia));
    cylinder.Draw(shader, trunk, C_BARK);

    // Two overlapping cones give the canopy some shape.
    glm::mat4 lower = glm::translate(base, glm::vec3(0.0f, trunkHeight + 1.05f, 0.0f));
    cone.Draw(shader, glm::scale(lower, glm::vec3(2.7f, 2.5f, 2.7f)), C_LEAF_LO);

    glm::mat4 upper = glm::translate(base, glm::vec3(0.0f, trunkHeight + 2.35f, 0.0f));
    cone.Draw(shader, glm::scale(upper, glm::vec3(1.9f, 2.1f, 1.9f)), C_LEAF_HI);
}

// ---------------------------------------------------------------------------
void Scene::drawFence(Shader& shader)
{
    const float e = FARM_EXTENT;

    const glm::vec3 nw(-e, 0.0f, -e), ne(e, 0.0f, -e);
    const glm::vec3 se( e, 0.0f,  e), sw(-e, 0.0f, e);

    drawFenceRun(shader, nw, ne, 6);   // back
    drawFenceRun(shader, ne, se, 6);   // right
    drawFenceRun(shader, sw, nw, 6);   // left

    // Front run is split to leave a gateway in the middle.
    drawFenceRun(shader, se, glm::vec3( 3.0f, 0.0f, e), 3);
    drawFenceRun(shader, glm::vec3(-3.0f, 0.0f, e), sw, 3);
}

void Scene::drawFenceRun(Shader& shader, const glm::vec3& from,
                         const glm::vec3& to, int spans)
{
    if (spans < 1) return;

    const float postHeight = 1.35f;
    const glm::vec3 dir  = to - from;
    const glm::vec3 step = dir / static_cast<float>(spans);
    const float spanLen  = glm::length(step);

    // Rotation about Y that lines the cube's local +Z up with the run.
    const float yaw = std::atan2(dir.x, dir.z);

    for (int i = 0; i <= spans; ++i)
    {
        glm::vec3 p = from + step * static_cast<float>(i);
        glm::mat4 post = glm::translate(glm::mat4(1.0f),
                             p + glm::vec3(0.0f, postHeight * 0.5f, 0.0f));
        post = glm::scale(post, glm::vec3(0.18f, postHeight, 0.18f));
        cube.Draw(shader, post, C_FENCE);
    }

    for (int i = 0; i < spans; ++i)
    {
        glm::vec3 mid = from + step * (static_cast<float>(i) + 0.5f);
        for (float y : { 0.55f, 1.05f })
        {
            glm::mat4 rail = glm::translate(glm::mat4(1.0f),
                                            mid + glm::vec3(0.0f, y, 0.0f));
            rail = glm::rotate(rail, yaw, glm::vec3(0, 1, 0));
            rail = glm::scale(rail, glm::vec3(0.07f, 0.14f, spanLen));
            cube.Draw(shader, rail, C_FENCE);
        }
    }
}

// ---------------------------------------------------------------------------
void Scene::drawLampPost(Shader& shader, const glm::vec3& pos)
{
    const float poleHeight = 3.4f;

    glm::mat4 base = glm::translate(glm::mat4(1.0f), pos);

    glm::mat4 pole = glm::translate(base, glm::vec3(0.0f, poleHeight * 0.5f, 0.0f));
    pole = glm::scale(pole, glm::vec3(0.22f, poleHeight, 0.22f));
    cylinder.Draw(shader, pole, C_METAL);

    glm::mat4 bulb = glm::translate(base, glm::vec3(0.0f, poleHeight + 0.22f, 0.0f));
    sphere.Draw(shader, glm::scale(bulb, glm::vec3(0.55f)), C_BULB);
}

// ---------------------------------------------------------------------------
void Scene::drawSun(Shader& shader)
{
    glm::mat4 m = glm::translate(glm::mat4(1.0f), sunPos);
    m = glm::scale(m, glm::vec3(4.0f));
    sphere.Draw(shader, m, C_SUN);
}
