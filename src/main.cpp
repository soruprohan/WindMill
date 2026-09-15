// Windmill Farm - A 3D Countryside Scene with Nested Rotational Hierarchies
//
// Phase 0: window skeleton.
// Phase 1: Shader class loads / compiles / links GLSL from disk.
// Phase 2: VAO / VBO / EBO wrappers, drawing via glDrawElements.
// Phase 3: MVP matrices, depth testing, an animated cube.
// Phase 4: primitive library and the Mesh class.
// Phase 5: the windmill's nested transformation hierarchy.
// Phase 6: the complete scene.

#include <glad/glad.h>   // must come before GLFW or any other GL header
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

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

void processInput(GLFWwindow* window, Scene& scene, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Phase 5 check: yawing the head must swing the blades with it while they
    // keep spinning. Phase 7 folds this into the full control scheme.
    const float yawRate = 60.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_COMMA)  == GLFW_PRESS) scene.AdjustHeadYaw(-yawRate);
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scene.AdjustHeadYaw( yawRate);
}

void printControls()
{
    std::cout << "\n=============== WINDMILL FARM - CONTROLS ===============\n"
              << "  , / .   Yaw the windmill heads\n"
              << "  ESC     Quit\n"
              << "========================================================\n\n";
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

    // Every primitive winds counter-clockwise when seen from outside, so
    // back faces can be discarded. It roughly halves the triangles rasterised
    // and, during development, makes any winding mistake obvious immediately:
    // a wrongly wound face simply disappears.
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

    // ---- Scene -----------------------------------------------------------
    Scene scene;
    printControls();

    float lastFrame = static_cast<float>(glfwGetTime());

    // ---- Render loop -----------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, scene, deltaTime);
        scene.Update(deltaTime);

        glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Activate();

        // Recomputed each frame so resizing keeps the aspect ratio correct.
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect,
                                                0.1f, 200.0f);

        // Placeholder view: a slow orbit so the whole farm can be inspected.
        // Phase 7 replaces this entirely with the Camera class.
        const float orbitRadius = 34.0f;
        const float orbitHeight = 10.0f;
        float orbitAngle = currentFrame * 0.12f;
        glm::vec3 eye(std::sin(orbitAngle) * orbitRadius,
                      orbitHeight,
                      std::cos(orbitAngle) * orbitRadius);
        // Aimed above the horizon rather than straight at the ground, so the
        // windmill heads and the sun stay in frame.
        glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f, 5.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

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
