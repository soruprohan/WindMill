#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

// Loads a vertex/fragment shader pair from disk, compiles and links them,
// and reports any compile or link error through the GL info log.
class Shader
{
public:
    GLuint ID = 0; //ID = handle/number that OpenGL gives us 0 means: No valid program has been created yet.

    Shader(const char* vertexPath, const char* fragmentPath);

    void Activate() const;
    void Delete();

    // Uniform setters.These functions make it easier to send data from C++ to GLSL.
    void setBool (const std::string& name, bool  value) const;
    void setInt  (const std::string& name, int   value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2 (const std::string& name, const glm::vec2& v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setVec4 (const std::string& name, const glm::vec4& v) const;
    void setMat3 (const std::string& name, const glm::mat3& m) const;
    void setMat4 (const std::string& name, const glm::mat4& m) const;

private:
    // glGetUniformLocation is a lookup by string every call; cache the results.
    mutable std::unordered_map<std::string, GLint> uniformCache;

    GLint location(const std::string& name) const;

    static bool readFile(const char* path, std::string& out); // Reads a shader file from disk.For example: default.vert and puts the source code into a C++ string.
    static bool checkCompile(GLuint shader, const char* label); //Checks whether a shader compiled successfully.
    static bool checkLink(GLuint program); //Checks whether the vertex shader and fragment shader successfully linked into one program.
};
