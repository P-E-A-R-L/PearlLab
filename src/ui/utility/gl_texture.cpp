//
// Created by xabdomo on 6/18/25.
//

#include "gl_texture.hpp"
#include <iostream>

namespace py = pybind11;

GLTexture::GLTexture() {
    glGenTextures(1, &texture_id_);
}

GLTexture::~GLTexture() {
    destroy();
}

void GLTexture::set(py::array_t<float> data, int width, int height, int channels) {
    width_ = width;
    height_ = height;
    channels_ = channels;

    switch (channels) {
        case 1: format_ = GL_RED; break;
        case 2: format_ = GL_RG; break;
        case 3: format_ = GL_RGB; break;
        case 4: format_ = GL_RGBA; break;
        default: throw std::runtime_error("Unsupported channel count");
    }

    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, format_, width_, height_, 0, format_, GL_FLOAT, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unbind();
}

void GLTexture::set(std::vector<unsigned char> data, int width, int height, int channels) {
    width_ = width;
    height_ = height;
    channels_ = channels;

    switch (channels) {
        case 1: format_ = GL_RED; break;
        case 2: format_ = GL_RG; break;
        case 3: format_ = GL_RGB; break;
        case 4: format_ = GL_RGBA; break;
        default: throw std::runtime_error("Unsupported channel count");
    }

    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, format_, width_, height_, 0, format_, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unbind();
}

void GLTexture::update(py::array_t<float> data) {
    bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format_, GL_FLOAT, data.data());
    unbind();
}

void GLTexture::bind(GLenum texture_unit) const {
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void GLTexture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::destroy() {
    if (texture_id_) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
}
