// Windmill Farm - A 3D Countryside Scene with Nested Rotational Hierarchies
// Phase 0: environment check and window skeleton.

#include <glad/glad.h>   // must come before GLFW or any other GL header
#include <GLFW/glfw3.h>

#include <iostream>

// Initial window size. The framebuffer callback keeps the viewport in sync
// with whatever the user resizes it to.
const unsigned int WINDOW_WIDTH  = 1280;
const unsigned int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Windmill Farm";

// Sky blue - the scene's clear colour for every phase from here on.
const float SKY_R = 0.53f;
const float SKY_G = 0.81f;
const float SKY_B = 0.92f;

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void printControls()
{
    std::cout << "\n=============== WINDMILL FARM - CONTROLS ===============\n"
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

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n"
              << "Renderer: " << glGetString(GL_RENDERER) << "\n"
              << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    printControls();

    // ---- Render loop -----------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Phase 3 adds GL_DEPTH_BUFFER_BIT here.

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Shutdown --------------------------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
