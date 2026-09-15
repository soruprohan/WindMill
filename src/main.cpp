// Windmill Farm - A 3D Countryside Scene with Nested Rotational Hierarchies
//
// Phase 0: window skeleton.
// Phase 1: Shader class loads / compiles / links GLSL from disk.
// Phase 2: VAO / VBO / EBO wrappers, drawing via glDrawElements.
// Phase 3: MVP matrices, depth testing, an animated cube.
// Phase 4: primitive library and the Mesh class.
// Phase 5: the windmill's nested transformation hierarchy.
// Phase 6: the complete scene.
// Phase 7: camera, projection toggle and the full control scheme.
// Phase 8: textures.

#include <glad/glad.h>   // must come before GLFW or any other GL header
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <iostream>

#include "Camera.h"
#include "Scene.h"
#include "Shader.h"

const unsigned int WINDOW_WIDTH  = 1280;
const unsigned int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Windmill Farm";

// Sky blue.
const float SKY_R = 0.53f;
const float SKY_G = 0.81f;
const float SKY_B = 0.92f;

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Fires only on the frame a key goes down, so a toggle does not flicker while
// the key is held.
static bool pressedOnce(GLFWwindow* window, int key, bool& wasDown)
{
    const bool down = glfwGetKey(window, key) == GLFW_PRESS;
    const bool fired = down && !wasDown;
    wasDown = down;
    return fired;
}

void printControls()
{
    std::cout <<
        "\n================= WINDMILL FARM - CONTROLS =================\n"
        "  MOVEMENT (free-fly camera)\n"
        "    W / S        Forward / backward\n"
        "    A / D        Strafe left / right\n"
        "    E / R        Rise / descend\n"
        "    LEFT SHIFT   Move faster (hold)\n"
        "    RIGHT MOUSE  Look around (hold; cursor hides)\n"
        "\n"
        "  VIEW\n"
        "    F            Toggle orbit camera / free-fly\n"
        "                   in orbit: W/S zoom, A/D swing, E/R height\n"
        "    P            Toggle perspective / orthographic\n"
        "\n"
        "  SCENE\n"
        "    T            Toggle textures on / off\n"
        "    SPACE        Pause / resume all animation\n"
        "    + / -        Blade speed up / down\n"
        "    , / .        Yaw the windmill heads\n"
        "\n"
        "    ESC          Quit\n"
        "===========================================================\n\n";
}

int main()
{
    // ---- GLFW init and window hints -------------------------------------
    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          WINDOW_TITLE, nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // glad must be loaded *after* the context is current, never before.
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialise GLAD\n";
        glfwTerminate();
        return -1;
    }
    // Any gl... call is safe from here on.

    glfwSwapInterval(1);   // vsync

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Without this, far faces paint over near ones and solids look
    // inside-out. The matching GL_DEPTH_BUFFER_BIT in glClear is just as
    // important - half the fix on its own does nothing.
    glEnable(GL_DEPTH_TEST);

    // Every primitive winds counter-clockwise when seen from outside, so back
    // faces can be discarded. During development it also makes any winding
    // mistake obvious: a wrongly wound face simply disappears.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n"
              << "Renderer: " << glGetString(GL_RENDERER) << "\n"
              << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    // ---- Shader ----------------------------------------------------------
    Shader shader("shaders/default.vert", "shaders/default.frag");
    if (shader.ID == 0)
    {
        std::cerr << "Aborting: shader program is not usable\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    shader.Activate();
    shader.setInt("diffuse0", 0);   // every texture binds to unit 0 for now

    // ---- Scene and camera -------------------------------------------------
    Scene  scene;
    Camera camera(glm::vec3(0.0f, 7.0f, 30.0f), -90.0f, -8.0f);

    bool texturesOn = true;

    // Previous-frame key states, for the edge-triggered toggles.
    bool wasF = false, wasP = false, wasT = false, wasSpace = false;

    printControls();

    float lastFrame = static_cast<float>(glfwGetTime());

    // ---- Render loop -----------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ---- input --------------------------------------------------------
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        camera.Inputs(window, deltaTime);

        if (pressedOnce(window, GLFW_KEY_F, wasF))
        {
            camera.ToggleOrbit();
            std::cout << "Camera: " << (camera.OrbitMode() ? "orbit" : "free-fly") << "\n";
        }
        if (pressedOnce(window, GLFW_KEY_P, wasP))
        {
            camera.ToggleProjection();
            std::cout << "Projection: "
                      << (camera.Orthographic() ? "orthographic" : "perspective") << "\n";
        }
        if (pressedOnce(window, GLFW_KEY_T, wasT))
        {
            texturesOn = !texturesOn;
            std::cout << "Textures: " << (texturesOn ? "on" : "off") << "\n";
        }
        if (pressedOnce(window, GLFW_KEY_SPACE, wasSpace))
        {
            scene.paused = !scene.paused;
            std::cout << "Animation: " << (scene.paused ? "paused" : "running") << "\n";
        }

        // Blade speed: held, so it ramps smoothly rather than stepping.
        const float speedStep = 45.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
            scene.bladeSpeed += speedStep;
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
            scene.bladeSpeed -= speedStep;
        scene.bladeSpeed = std::max(-360.0f, std::min(360.0f, scene.bladeSpeed));

        // Windmill head yaw - the Phase 5 hierarchy check.
        const float yawRate = 60.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_COMMA)  == GLFW_PRESS) scene.AdjustHeadYaw(-yawRate);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scene.AdjustHeadYaw( yawRate);

        // ---- update -------------------------------------------------------
        scene.Update(deltaTime);

        // ---- render -------------------------------------------------------
        glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Activate();

        // Recomputed each frame so resizing keeps the aspect ratio correct.
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        const float aspect = (fbHeight > 0)
                           ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight)
                           : 1.0f;

        shader.setMat4("projection", camera.ProjectionMatrix(aspect));
        shader.setMat4("view", camera.ViewMatrix());
        shader.setBool("useTexture", texturesOn);

        scene.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Shutdown --------------------------------------------------------
    scene.Delete();
    shader.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
