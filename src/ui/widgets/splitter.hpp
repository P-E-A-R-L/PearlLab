//
// Created by xabdomo on 6/24/25.
//

#ifndef SPLITTER_HPP
#define SPLITTER_HPP
#include <functional>
#include <vector>


namespace Splitter {
    void Vertical(std::vector<float>& sizes, std::function<void(int)> renderFn, float splitterHeight = 4.0f);
    void Horizontal(std::vector<float>& sizes, std::function<void(int)> renderFn, float splitterWidth  = 4.0f);
}

#endif //SPLITTER_HPP
