#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Free-fly and orbit camera, plus the perspective/orthographic toggle.
//
// Both modes keep `position` and `orientation` up to date, so the view matrix
// is built the same way either way and Phase 9 can read the eye position for
// its specular term without caring which mode is active.
class Camera
{
public:
    Camera(const glm::vec3& startPosition, float yawDeg, float pitchDeg);

    void Inputs(GLFWwindow* window, float deltaTime);

    glm::mat4 ViewMatrix() const;
    glm::mat4 ProjectionMatrix(float aspect) const;

    void ToggleOrbit();
    void ToggleProjection();

    const glm::vec3& Position() const { return position; }
    bool OrbitMode()    const { return orbitMode; }
    bool Orthographic() const { return orthographic; }

    float speed       = 11.0f;   // units per second
    float sensitivity = 0.09f;   // degrees per pixel of cursor movement
    float fovDeg      = 45.0f;

private:
    void  updateOrientation();
    void  freeFlyInputs(GLFWwindow* window, float deltaTime);
    void  orbitInputs(GLFWwindow* window, float deltaTime);
    void  mouseLook(GLFWwindow* window);

    glm::vec3 position;
    glm::vec3 orientation{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };

    // Yaw and pitch are the source of truth, not the orientation vector.
    // Clamping a stored pitch is exact; repeatedly rotating a vector and
    // testing the result afterwards is not.
    float yaw;
    float pitch;

    bool orbitMode    = false;
    bool orthographic = false;
    bool firstClick   = true;

    // Orbit mode.
    glm::vec3 orbitTarget{ 0.0f, 4.0f, 0.0f };
    float orbitRadius = 34.0f;
    float orbitHeight = 11.0f;
    float orbitAngle  = 0.0f;
    float orbitSpeed  = 14.0f;   // degrees per second

    float orthoScale = 14.0f;    // half-height of the ortho box
};
