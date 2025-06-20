//
// Created by xabdomo on 6/20/25.
//

#include "image_store.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::unordered_map<std::string, GLTexture*> _cache;


static bool _load(const std::string &name) {
    int width, height, channels;
    unsigned char *data = stbi_load(name.c_str(), &width, &height, &channels, 0);
    if (!data) {
        return false;
    }

    GLTexture *texture = new GLTexture();
    texture->set(data, width, height, channels);
    _cache[name] = texture;
    stbi_image_free(data);
    return true;
}

GLuint ImageStore::idOf(const std::string &name) {
    auto k = textureOf(name);
    if (k != nullptr) return k->id();
    return 0;
}

GLTexture * ImageStore::textureOf(const std::string &name) {
    auto it = _cache.find(name);
    if (it == _cache.end()) {
        if (!_load(name)) return nullptr;
        it = _cache.find(name);
    }
    return it->second;
}

void ImageStore::init() {

}

void ImageStore::destroy() {
    for (auto const& [k, v]: _cache) {
        delete v;
    }

    _cache.clear();
}
