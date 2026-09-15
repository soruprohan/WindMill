#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode, fragmentCode;
    if (!readFile(vertexPath, vertexCode))   return;
    if (!readFile(fragmentPath, fragmentCode)) return;

    const char* vSrc = vertexCode.c_str();
    const char* fSrc = fragmentCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vSrc, nullptr);
    glCompileShader(vertexShader);
    bool vOk = checkCompile(vertexShader, vertexPath);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fSrc, nullptr);
    glCompileShader(fragmentShader);
    bool fOk = checkCompile(fragmentShader, fragmentPath);

    if (vOk && fOk)
    {
        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        if (!checkLink(ID))
        {
            glDeleteProgram(ID);
            ID = 0;
        }
    }

    // The program keeps its own copy once linked.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (ID != 0)
        std::cout << "Shader OK: " << vertexPath << " + " << fragmentPath << "\n";
}

void Shader::Activate() const
{
    glUseProgram(ID);
}

void Shader::Delete()
{
    if (ID != 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
}

// ---- file loading --------------------------------------------------------

bool Shader::readFile(const char* path, std::string& out)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "SHADER ERROR: cannot open '" << path << "'\n"
                  << "  (run the program from the project root so shaders/ resolves)\n";
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

// ---- error reporting -----------------------------------------------------

bool Shader::checkCompile(GLuint shader, const char* label)
{
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        std::string log(logLength, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        // Printed even on success: a warning here often explains a black screen.
        std::cerr << "SHADER LOG (" << label << "):\n" << log << "\n";
    }

    if (!success)
        std::cerr << "SHADER ERROR: '" << label << "' failed to compile\n";

    return success == GL_TRUE;
}

bool Shader::checkLink(GLuint program)
{
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        std::string log(logLength, '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::cerr << "PROGRAM LOG:\n" << log << "\n";
    }

    if (!success)
        std::cerr << "SHADER ERROR: program failed to link\n";

    return success == GL_TRUE;
}

// ---- uniforms ------------------------------------------------------------

GLint Shader::location(const std::string& name) const
{
    auto it = uniformCache.find(name);
    if (it != uniformCache.end())
        return it->second;

    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc == -1)
        std::cerr << "SHADER WARNING: uniform '" << name << "' not found (or optimised out)\n";

    uniformCache[name] = loc;
    return loc;
}

void Shader::setBool (const std::string& n, bool  v) const { glUniform1i(location(n), (int)v); }
void Shader::setInt  (const std::string& n, int   v) const { glUniform1i(location(n), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f(location(n), v); }

void Shader::setVec2(const std::string& n, const glm::vec2& v) const { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec3(const std::string& n, const glm::vec3& v) const { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec4(const std::string& n, const glm::vec4& v) const { glUniform4fv(location(n), 1, glm::value_ptr(v)); }

void Shader::setMat3(const std::string& n, const glm::mat3& m) const { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4(const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
