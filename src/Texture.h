#pragma once

#include <glad/glad.h>

// Loads an image from disk into a GL texture with mipmaps and GL_REPEAT wrap.
class Texture
{
public:
    // unit is the texture unit this texture binds to (GL_TEXTURE0 + unit).
    Texture(const char* imagePath, GLuint unit = 0);

    void Bind() const;
    void Unbind() const;
    void Delete();

    bool Valid() const { return ID != 0; }

    GLuint ID = 0;

    // Holds a GL handle, so copying it would double-delete.
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

private:
    GLuint unit = 0;
};
