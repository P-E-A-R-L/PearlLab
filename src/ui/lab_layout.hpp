#ifndef LAB_LAYOUT_HPP
#define LAB_LAYOUT_HPP
#include <string>
#include <vector>

#include "project_manager.hpp"

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

    void dockWindow(const std::string& name, DockLocation location);
}

#endif // LAB_LAYOUT_HPP
