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

    void set(pybind11::array_t<float> data, int width, int height, int channels);
    void set(std::vector<unsigned char> data, int width, int height, int channels);
    void update(pybind11::array_t<float> data);
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
