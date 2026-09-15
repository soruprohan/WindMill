#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Mesh.h"
#include "Shader.h"

// Placement data for one object. Keeping position/rotation/scale as data in a
// vector, rather than matrices hardcoded inline, means repositioning the scene
// is a data edit and not a code edit.
struct Transform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotationDeg{ 0.0f };   // applied Y, then X, then Z
    glm::vec3 scale{ 1.0f };

    glm::mat4 matrix() const;
};

// One windmill instance. Each carries its own head yaw and blade phase, so
// two windmills side by side do not turn in lockstep.
struct Windmill
{
    glm::vec3 position{ 0.0f };
    float scale      = 1.0f;
    float yawDeg     = 0.0f;   // head rotation about Y
    float bladePhase = 0.0f;   // starting blade angle offset
};

class Scene
{
public:
    Scene();

    void Update(float deltaTime);
    void Draw(Shader& shader);
    void Delete();

    // Phase 5 asks for head yaw on a key, to show the blades swing with the
    // head while still spinning independently.
    void AdjustHeadYaw(float degrees);

    // Phases 9 and 10 need exactly these positions for the lights.
    const std::vector<glm::vec3>& LampBulbPositions() const { return lampBulbs; }
    glm::vec3 SunPosition() const { return sunPos; }

    // Animation state. Phase 7 binds keys to these.
    float bladeSpeed = 60.0f;    // degrees per second
    float wheelSpeed = 35.0f;
    bool  paused     = false;

private:
    void drawGround(Shader& shader);
    void drawWindmill(Shader& shader, const Windmill& w);
    void drawHouse(Shader& shader, const Transform& t);
    void drawWaterWheel(Shader& shader, const glm::vec3& pos);
    void drawTree(Shader& shader, const Transform& t);
    void drawFence(Shader& shader);
    void drawFenceRun(Shader& shader, const glm::vec3& from,
                      const glm::vec3& to, int spans);
    void drawLampPost(Shader& shader, const glm::vec3& pos);
    void drawSun(Shader& shader);

    // Generated once at startup, redrawn many times with different matrices.
    Mesh cube, plane, cylinder, cone, sphere, prism;

    // Animation angles, advanced in Update().
    float bladeAngle = 0.0f;
    float wheelAngle = 0.0f;

    std::vector<Windmill>  windmills;
    std::vector<Transform> trees;
    std::vector<glm::vec3> lampPosts;
    std::vector<glm::vec3> lampBulbs;   // world-space, computed once at layout

    Transform house;
    glm::vec3 wheelPos{ 0.0f };
    glm::vec3 sunPos{ 0.0f };
};
