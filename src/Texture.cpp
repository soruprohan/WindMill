#include "Texture.h"

// stb_image is header-only: exactly one translation unit defines the
// implementation, and this is it.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

Texture::Texture(const char* imagePath, GLuint unit_)
    : unit(unit_)
{
    // OpenGL's texture origin is the bottom-left corner; image files store
    // rows top-down. Without this flip every texture arrives upside-down.
    stbi_set_flip_vertically_on_load(true);

    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(imagePath, &width, &height, &channels, 0);
    if (!pixels)
    {
        std::cerr << "TEXTURE ERROR: cannot load '" << imagePath << "' - "
                  << stbi_failure_reason() << "\n"
                  << "  (run the program from the project root so textures/ resolves)\n";
        return;
    }

    GLenum format = GL_RGB;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else
    {
        std::cerr << "TEXTURE ERROR: '" << imagePath << "' has "
                  << channels << " channels, which is not handled\n";
        stbi_image_free(pixels);
        return;
    }

    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Trilinear when minified so distant ground does not shimmer, plain
    // linear when magnified.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Rows of a 3-channel image are not necessarily 4-byte aligned.
    glPixelStorei(GL_UNPACK_ALIGNMENT, (channels == 4) ? 4 : 1);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    // The GPU has its own copy now.
    stbi_image_free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Texture OK: " << imagePath
              << "  (" << width << "x" << height << ", " << channels << "ch)\n";
}

void Texture::Bind() const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind() const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Delete()
{
    if (ID != 0)
    {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}
