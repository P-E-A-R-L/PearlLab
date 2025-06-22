#ifndef IMAGE_STORE_HPP
#define IMAGE_STORE_HPP
#include <string>
#include <GL/gl.h>

#include "gl_texture.hpp"

namespace ImageStore
{
    GLuint idOf(const std::string &name);
    GLTexture *textureOf(const std::string &name);

    void init();
    void destroy();
}

#endif // IMAGE_STORE_HPP
