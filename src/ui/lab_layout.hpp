#ifndef LAB_LAYOUT_HPP
#define LAB_LAYOUT_HPP
#include <string>
#include <vector>

#include "project_manager.hpp"
#include <unordered_map>
#include <imgui.h>

namespace LabLayout
{
    extern ProjectManager::ProjectDetails project_details;
    void init(const std::string &project_path);
    void update();
    void render();
    void destroy();

    enum DockLocation {
        TOP_LEFT,
        CENTER_LEFT,
        BOT_LEFT,
        CENTER_TOP,
        CENTER_BOT,
        TOP_RIGHT,
        CENTER_RIGHT,
        BOT_RIGHT,
    };

    // IDs for docking windows, used to identify docking locations
    inline std::unordered_map<std::string, ImGuiID> IDs = {
        {"left_top", 0},
        {"left_mid", 0},
        {"left_bot", 0},
        {"center_top", 0},
        {"center_bot", 0},
        {"right_top", 0},
        {"right_mid", 0},
        {"right_bot", 0}
    };
}

#endif // LAB_LAYOUT_HPP
