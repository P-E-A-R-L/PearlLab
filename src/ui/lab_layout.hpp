#ifndef LAB_LAYOUT_HPP
#define LAB_LAYOUT_HPP
#include <string>

#include "project_manager.hpp"

namespace LabLayout
{
    extern ProjectManager::ProjectDetails project_details;
    void init(const std::string &project_path);
    void update();
    void render();
    void destroy();
}

#endif // LAB_LAYOUT_HPP
