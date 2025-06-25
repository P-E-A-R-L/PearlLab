#ifndef PREVIEW_HPP
#define PREVIEW_HPP
#include <map>

#include "pipeline.hpp"
#include "../utility/gl_texture.hpp"

namespace Preview
{
    void init();
    void update();
    void render();
    void onStart();
    void onStop();
    void destroy();
};

#endif // PREVIEW_HPP
