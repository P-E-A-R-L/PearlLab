//
// Created by xabdomo on 6/18/25.
//

#ifndef GL_TEXTURE_HPP
#define GL_TEXTURE_HPP


#include <pybind11/numpy.h>
#include <stdexcept>
#include <GL/gl.h>

class GLTexture {
public:
    GLTexture();
    ~GLTexture();

    void set(const pybind11::array_t<float> &data, const int width, const int height, const int channels);
    void set(const std::vector<unsigned char> &data, const int width, const int height, const int channels);
    void set(const unsigned char* data, const int width, const int height, const int channels);
    void update(const pybind11::array_t<float> &data) const;
    void update(const std::vector<unsigned char> &data) const;
    void update(unsigned char* data) const;
    void bind(GLenum texture_unit = GL_TEXTURE0) const;
    void unbind() const;
    void destroy();

    GLuint id() const { return texture_id_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    GLuint texture_id_;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    GLenum format_ = GL_RGB;

    static void check_array_shape(const pybind11::array_t<float>& arr, bool allow_batch);
};



#endif //GL_TEXTURE_HPP
