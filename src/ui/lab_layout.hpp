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

    // IDs for docking windows, used to identify docking locations
    extern std::unordered_map<std::string, ImGuiID> IDs;
}

#endif // LAB_LAYOUT_HPP
