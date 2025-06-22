#ifndef PROJECT_MANAGER_HPP
#define PROJECT_MANAGER_HPP

#include <optional>
#include <string>

namespace ProjectManager
{
    struct ProjectDetails
    {
        std::string project_path; // base path of the project
        std::string graph_file;
        std::string imgui_file;
    };

    ProjectDetails loadProject(const std::string &path);
    bool saveProject(const std::string &path);
}

#endif // PROJECT_MANAGER_HPP
