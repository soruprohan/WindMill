#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    // Looking straight up or down makes the view matrix degenerate, because
    // the forward vector becomes parallel to `up` and their cross product
    // collapses. Stopping one degree short avoids it entirely.
    const float PITCH_LIMIT = 89.0f;
}

Camera::Camera(const glm::vec3& startPosition, float yawDeg, float pitchDeg)
    : position(startPosition), yaw(yawDeg), pitch(pitchDeg)
{
    updateOrientation();
}

void Camera::updateOrientation()
{
    pitch = std::max(-PITCH_LIMIT, std::min(PITCH_LIMIT, pitch));

    const float y = glm::radians(yaw);
    const float p = glm::radians(pitch);

    orientation = glm::normalize(glm::vec3(std::cos(y) * std::cos(p),
                                           std::sin(p),
                                           std::sin(y) * std::cos(p)));
}

void Camera::Inputs(GLFWwindow* window, float deltaTime)
{
    if (orbitMode) orbitInputs(window, deltaTime);
    else           freeFlyInputs(window, deltaTime);
}

// ---------------------------------------------------------------------------
void Camera::freeFlyInputs(GLFWwindow* window, float deltaTime)
{
    // Every movement is multiplied by deltaTime, so speed is the same whether
    // the machine runs at 30 or 300 frames per second.
    float step = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        step *= 2.5f;

    const glm::vec3 right = glm::normalize(glm::cross(orientation, up));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += step * orientation;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= step * orientation;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= step * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += step * right;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) position += step * up;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) position -= step * up;

    mouseLook(window);
}

// ---------------------------------------------------------------------------
void Camera::mouseLook(GLFWwindow* window)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
    {
        if (!firstClick)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstClick = true;
        }
        return;
    }

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    const double cx = width * 0.5;
    const double cy = height * 0.5;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // On the first frame of the drag, warp to the centre before reading, so
    // wherever the cursor happened to be does not register as a huge jump.
    if (firstClick)
    {
        glfwSetCursorPos(window, cx, cy);
        firstClick = false;
        return;
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    yaw   += static_cast<float>(mouseX - cx) * sensitivity;
    pitch -= static_cast<float>(mouseY - cy) * sensitivity;   // screen Y is down

    updateOrientation();

    // Re-centre so the cursor never reaches the edge of the screen.
    glfwSetCursorPos(window, cx, cy);
}

// ---------------------------------------------------------------------------
void Camera::orbitInputs(GLFWwindow* window, float deltaTime)
{
    orbitAngle += orbitSpeed * deltaTime;
    if (orbitAngle >= 360.0f) orbitAngle -= 360.0f;

    // A/D swing manually on top of the automatic sweep, W/S zoom, E/R rise.
    const float swing = 45.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) orbitAngle -= swing;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) orbitAngle += swing;

    const float zoom = 14.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) orbitRadius -= zoom;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) orbitRadius += zoom;
    orbitRadius = std::max(8.0f, std::min(70.0f, orbitRadius));

    const float rise = 9.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) orbitHeight += rise;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) orbitHeight -= rise;
    orbitHeight = std::max(1.5f, std::min(45.0f, orbitHeight));

    const float a = glm::radians(orbitAngle);
    position = glm::vec3(orbitTarget.x + orbitRadius * std::cos(a),
                         orbitHeight,
                         orbitTarget.z + orbitRadius * std::sin(a));
    orientation = glm::normalize(orbitTarget - position);
}

// ---------------------------------------------------------------------------
void Camera::ToggleOrbit()
{
    orbitMode = !orbitMode;

    if (orbitMode)
    {
        // Enter the orbit where the camera already is, so the view does not jump.
        glm::vec3 d = position - orbitTarget;
        orbitRadius = std::max(8.0f, std::min(70.0f, std::sqrt(d.x * d.x + d.z * d.z)));
        orbitHeight = std::max(1.5f, std::min(45.0f, position.y));
        orbitAngle  = glm::degrees(std::atan2(d.z, d.x));
    }
    else
    {
        // Hand the orbit's current facing back to the free-fly yaw/pitch.
        yaw   = glm::degrees(std::atan2(orientation.z, orientation.x));
        pitch = glm::degrees(std::asin(glm::clamp(orientation.y, -1.0f, 1.0f)));
        updateOrientation();
    }
}

void Camera::ToggleProjection()
{
    orthographic = !orthographic;
}

// ---------------------------------------------------------------------------
glm::mat4 Camera::ViewMatrix() const
{
    return glm::lookAt(position, position + orientation, up);
}

glm::mat4 Camera::ProjectionMatrix(float aspect) const
{
    if (orthographic)
    {
        const float s = orthoScale;
        return glm::ortho(-aspect * s, aspect * s, -s, s, 0.1f, 300.0f);
    }
    return glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 300.0f);
}
