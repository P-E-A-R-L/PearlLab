//
// Created by xabdomo on 6/26/25.
//

#ifndef SCROLLABLE_PLOT_HPP
#define SCROLLABLE_PLOT_HPP
#include <vector>


namespace Widgets {
    void PlotLinesScrollable(const char* label, const std::vector<float>& data, int n_visible = 100, int scroll_speed = 5);
}



#endif //SCROLLABLE_PLOT_HPP
