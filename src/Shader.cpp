#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>   // glm::value_ptr - turns a glm vector/matrix into a plain float* for OpenGL

#include <fstream>    // std::ifstream, to open the .vert / .frag files
#include <iostream>   // std::cout / std::cerr, to print status and errors
#include <sstream>    // std::stringstream, to slurp a whole file into one string

// Building a usable shader program is a fixed 5-step recipe in OpenGL:
//   1. read the two GLSL source files off disk into strings
//   2. compile the vertex shader
//   3. compile the fragment shader
//   4. link both compiled shaders into one "program"
//   5. the GPU no longer needs the separate compiled stages, so delete them
// Any step can fail; if it does, ID is left at 0 so the rest of the program
// can tell this Shader is not usable.
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // Step 1: read both files into C++ strings.
    std::string vertexCode, fragmentCode;
    if (!readFile(vertexPath, vertexCode))   return;   // couldn't open the file -> give up, ID stays 0
    if (!readFile(fragmentPath, fragmentCode)) return;

    // glShaderSource wants a raw C string (char*), not a std::string.
    const char* vSrc = vertexCode.c_str();
    const char* fSrc = fragmentCode.c_str();

    // Step 2: compile the vertex shader.
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);  // ask OpenGL for an empty shader "slot"
    glShaderSource(vertexShader, 1, &vSrc, nullptr);         // hand it our GLSL source text
    glCompileShader(vertexShader);                           // ask the GPU driver to compile it
    bool vOk = checkCompile(vertexShader, vertexPath);        // did it compile? (also prints any errors)

    // Step 3: compile the fragment shader. Same pattern as above.
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fSrc, nullptr);
    glCompileShader(fragmentShader);
    bool fOk = checkCompile(fragmentShader, fragmentPath);

    // Step 4: only try to link if BOTH shaders compiled successfully.
    if (vOk && fOk)
    {
        ID = glCreateProgram();          // a "program" is a vertex shader + fragment shader glued together
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);               // actually glue them together
        if (!checkLink(ID))              // linking can still fail even if both compiled fine
        {
            glDeleteProgram(ID);
            ID = 0;                      // 0 means "not usable" - checked elsewhere in the app
        }
    }

    // Step 5: the individual compiled shaders are no longer needed once linked -
    // the program keeps its own copy internally.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (ID != 0)
        std::cout << "Shader OK: " << vertexPath << " + " << fragmentPath << "\n";
}

// Tells OpenGL "use this shader program for every draw call from now on".
void Shader::Activate() const
{
    glUseProgram(ID);
}

// Frees the shader program on the GPU. Safe to call even if it was never
// created (ID == 0) or already deleted.
void Shader::Delete()
{
    if (ID != 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
}

// ---- file loading --------------------------------------------------------

// Opens a text file (e.g. "shaders/default.vert") and dumps its entire
// contents into `out` as one big string. Returns false if the file could
// not be opened - usually because the program was launched from the wrong
// folder, so shaders/ can't be found from where it's looking.
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
    ss << file.rdbuf();   // read the whole file in one go
    out = ss.str();
    return true;
}

// ---- error reporting -----------------------------------------------------

// After glCompileShader, OpenGL doesn't throw a C++ exception on failure -
// it just sets a status flag internally. This function asks for that flag
// and for any log text (warnings or errors) the compiler produced.
bool Shader::checkCompile(GLuint shader, const char* label)
{
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);   // 1 = compiled OK, 0 = failed

    // Ask how long the compiler's message is, then fetch it if there is one.
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

// Same idea as checkCompile, but for the LINK step (gluing the vertex and
// fragment shaders together into one program). A shader can compile fine on
// its own and still fail to link - e.g. if the vertex shader doesn't output
// something the fragment shader expects to read in.
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

// A "uniform" is a variable in the GLSL shader that C++ can set from the
// outside (things like the model matrix, or a colour). To set one, OpenGL
// first needs its numeric "location" inside the compiled program, found by
// name with glGetUniformLocation. That's a string lookup, which is slow to
// do every single frame, so the result is cached here: look it up once per
// name, remember it, and just re-use the cached number after that.
GLint Shader::location(const std::string& name) const
{
    auto it = uniformCache.find(name);
    if (it != uniformCache.end())
        return it->second;   // already looked this one up before - reuse it

    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc == -1)
        // -1 means the name is wrong, OR the uniform isn't actually used
        // anywhere in the shader (the compiler is allowed to strip it out).
        std::cerr << "SHADER WARNING: uniform '" << name << "' not found (or optimised out)\n";

    uniformCache[name] = loc;   // remember it (even if -1) so we don't look it up again
    return loc;
}

// The setXxx family below all do the same three things:
//   1. look up (or reuse) the uniform's location by name
//   2. hand the value to OpenGL in the matching glUniform* call
//   3. that value now applies to whatever gets drawn next with this shader
// Each one just matches a different GLSL type (bool, int, float, vec2/3/4, mat3/4).

void Shader::setBool (const std::string& n, bool  v) const { glUniform1i(location(n), (int)v); }   // GLSL has no real bool uniform, so send it as an int (0 or 1)
void Shader::setInt  (const std::string& n, int   v) const { glUniform1i(location(n), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f(location(n), v); }

// vec2/vec3/vec4: glm::value_ptr turns the glm vector into a plain float*
// that OpenGL's C API can read.
void Shader::setVec2(const std::string& n, const glm::vec2& v) const { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec3(const std::string& n, const glm::vec3& v) const { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec4(const std::string& n, const glm::vec4& v) const { glUniform4fv(location(n), 1, glm::value_ptr(v)); }

// mat3/mat4: same idea, but for matrices (used for model/view/projection).
// The GL_FALSE means "don't transpose it" - glm and OpenGL already agree on
// the same (column-major) matrix layout, so no conversion is needed.
void Shader::setMat3(const std::string& n, const glm::mat3& m) const { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4(const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
