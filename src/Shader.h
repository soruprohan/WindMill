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
    GLuint ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath);

    void Activate() const;
    void Delete();

    // Uniform setters.
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

    static bool readFile(const char* path, std::string& out);
    static bool checkCompile(GLuint shader, const char* label);
    static bool checkLink(GLuint program);
};
