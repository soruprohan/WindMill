// Windmill Farm - A 3D Countryside Scene with Nested Rotational Hierarchies
//
// Phase 0: window skeleton.
// Phase 1: Shader class loads / compiles / links GLSL from disk.
// Phase 2: VAO / VBO / EBO wrappers, drawing via glDrawElements.
// Phase 3: MVP matrices, depth testing, and an animated 24-vertex cube.

#include <glad/glad.h>   // must come before GLFW or any other GL header
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>       // offsetof
#include <iostream>
#include <vector>

#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Vertex.h"

const unsigned int WINDOW_WIDTH  = 1280;
const unsigned int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Windmill Farm";

// Sky blue - the clear colour for the scene from here on.
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

// ---------------------------------------------------------------------------
// A unit cube centred on the origin, built as 24 vertices - four per face - so
// that every face carries its own normal and its own texture coordinates.
// Sharing 8 corner vertices would force one averaged normal per corner, which
// is wrong for a hard-edged shape.
//
// Winding is counter-clockwise when each face is viewed from outside.
// Phase 4 moves this into Primitives::makeCube().
// ---------------------------------------------------------------------------
static void makeCube(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
    const float h = 0.5f;   // half extent

    struct Face { glm::vec3 normal, color; glm::vec3 corner[4]; };

    const Face faces[6] = {
        // +Z front (red)
        { {0,0,1}, {0.85f, 0.25f, 0.25f},
          { {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h} } },
        // -Z back (green)
        { {0,0,-1}, {0.30f, 0.75f, 0.35f},
          { { h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h} } },
        // -X left (blue)
        { {-1,0,0}, {0.25f, 0.45f, 0.85f},
          { {-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h} } },
        // +X right (yellow)
        { {1,0,0}, {0.90f, 0.80f, 0.25f},
          { { h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h} } },
        // -Y bottom (magenta)
        { {0,-1,0}, {0.75f, 0.30f, 0.70f},
          { {-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h} } },
        // +Y top (cyan)
        { {0,1,0}, {0.30f, 0.80f, 0.80f},
          { {-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h} } },
    };

    const glm::vec2 uv[4] = { {0,0}, {1,0}, {1,1}, {0,1} };

    vertices.clear();
    indices.clear();
    vertices.reserve(24);
    indices.reserve(36);

    for (int f = 0; f < 6; ++f)
    {
        GLuint base = static_cast<GLuint>(vertices.size());

        for (int c = 0; c < 4; ++c)
            vertices.push_back({ faces[f].corner[c], faces[f].normal,
                                 uv[c], faces[f].color });

        // Two triangles per face.
        indices.insert(indices.end(), { base + 0, base + 1, base + 2,
                                        base + 2, base + 3, base + 0 });
    }
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

    // Without this, far faces paint over near ones and the cube looks
    // inside-out. The matching GL_DEPTH_BUFFER_BIT in glClear is just as
    // important - half the fix on its own does nothing.
    glEnable(GL_DEPTH_TEST);

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

    // ---- Geometry --------------------------------------------------------
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    makeCube(vertices, indices);

    VAO vao;
    vao.Bind();

    VBO vbo(vertices);
    EBO ebo(indices);
    ebo.Bind();   // the VAO records this binding

    const GLsizei stride = sizeof(Vertex);
    vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, position));
    vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, normal));
    vao.LinkAttrib(vbo, 2, 2, GL_FLOAT, stride, (void*)offsetof(Vertex, texCoord));
    vao.LinkAttrib(vbo, 3, 3, GL_FLOAT, stride, (void*)offsetof(Vertex, color));

    vao.Unbind();
    vbo.Unbind();
    // The EBO is unbound only after the VAO is unbound. Doing it while the
    // VAO is still bound would erase the element buffer from the VAO.
    ebo.Unbind();

    printControls();

    // ---- Render loop -----------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Activate();

        // Recomputed each frame so resizing keeps the aspect ratio correct.
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect,
                                                0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.5f, 4.0f),   // eye
                                     glm::vec3(0.0f, 0.0f, 0.0f),   // target
                                     glm::vec3(0.0f, 1.0f, 0.0f));  // up

        // Spin on a tilted axis so several faces come into view.
        float t = (float)glfwGetTime();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), t * glm::radians(45.0f),
                                      glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        vao.Bind();
        glDrawElements(GL_TRIANGLES, ebo.count, GL_UNSIGNED_INT, 0);
        vao.Unbind();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Shutdown --------------------------------------------------------
    vao.Delete();
    vbo.Delete();
    ebo.Delete();
    shader.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
