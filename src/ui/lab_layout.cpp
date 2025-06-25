#include "lab_layout.hpp"
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

#include "ImGuiFileDialog.h"
#include "project_manager.hpp"
#include "shared_ui.hpp"
#include "startup_loader.hpp"
#include "../backend/py_scope.hpp"
#include "modules/inspector.hpp"
#include "modules/logger.hpp"
#include "modules/objects_panel.hpp"
#include "modules/pipeline.hpp"
#include "modules/pipeline_graph.hpp"
#include "modules/preview.hpp"
#include "modules/py_module_window.hpp"
#include "utility/image_store.hpp"

namespace LabLayout
{
    // hidden because normally not other module would need these functions
    // these are just some demo functions to make the UI .. well .. do something
    // they are to be changed with the actual modules
    // void renderParamsModule();
    // they are no more :)

    ProjectManager::ProjectDetails project_details;

}

extern GLFWwindow *AppWindow;

void LabLayout::init(const std::string &project_path)
{
    Logger::init();
    PyScope::init();

    project_details = ProjectManager::loadProject(project_path);

    PyModuleWindow::init();
    PipelineGraph::init(project_details.graph_file);
    SharedUi::init();
    ObjectsPanel::init();
    Pipeline::init();
    Preview::init();
    Inspector::init();
}

void LabLayout::update() {
    // before we render any content

    Pipeline ::update();
    Preview  ::update();
    Inspector::update();
}

void LabLayout::render()
{

    ///
    /// MAIN MANU
    ///
    static bool open_modules_debugger = false;
    static bool show_metrics = false;
    static bool menu_reset_window = true;

    static bool window_objects   = true;
    static bool window_logger    = true;
    static bool window_preview   = true;
    static bool window_pipeline  = true;
    static bool window_inspector = true;
    static bool window_graph     = true;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Module"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("FileDlgKey", "Select module file", ".py");
            }

            if (ImGui::MenuItem("Quick Save"))
            {
                if (ProjectManager::saveProject(project_details.project_path))
                {
                    Logger::info("Project saved successfully.");
                }
                else
                {
                    Logger::error("Failed to save project.");
                }
            }

            if (ImGui::MenuItem("Save Project"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("FileDlgKey", "Select module file", ".py");
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                glfwSetWindowShouldClose(AppWindow, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z"))
            { /* Undo Action */
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
            { /* Disabled Redo */
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X"))
            { /* Cut Action */
            }
            if (ImGui::MenuItem("Copy", "CTRL+C"))
            { /* Copy Action */
            }
            if (ImGui::MenuItem("Paste", "CTRL+V"))
            { /* Paste Action */
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::MenuItem("Modules Debugger", nullptr, &open_modules_debugger);
            ImGui::MenuItem("Show Metrics", nullptr, &show_metrics);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Reset Docking")) {
                menu_reset_window = true;
            }

            ImGui::Separator();
            ImGui::MenuItem("Objects"  , nullptr, &window_objects);
            ImGui::MenuItem("Pipeline" , nullptr, &window_pipeline);
            ImGui::MenuItem("Preview"  , nullptr, &window_preview);
            ImGui::MenuItem("Graph"    , nullptr, &window_graph);
            ImGui::MenuItem("Event Log", nullptr, &window_logger);
            ImGui::MenuItem("Inspector", nullptr, &window_inspector);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
            }
            ImGui::EndMenu();
        }


        ImGui::Dummy({50, 1});
        ImGui::SameLine();

        auto& io = ImGui::GetIO();
        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::SameLine();

        ImGui::EndMainMenuBar(); // Closes the menu bar
    }

    if (ImGuiFileDialog::Instance()->Display("FileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk()) // If user selects a file
        {
            py::gil_scoped_acquire acquire{};
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            auto result = PyScope::LoadModuleForClasses(filePath);
            for (auto obj : result)
            {
                SharedUi::pushModule(obj);
                Logger::info("Loaded module: " + std::string(py::str(obj.attr("__name__"))));
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ///
    /// WINDOW CONTENT
    ///
    static bool p_open = true;
    static bool first_run = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DockSpaceMain", &p_open, window_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dockspace_flags);

    // Build default layout once
    if (menu_reset_window)
    {
        Logger::info("Initializing Lab Layout...");

        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any existing layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        Logger::info("Setting up default dock layout...");
        // Split into columns: left, center, right
        ImGuiID left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
        ImGuiID right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
        ImGuiID center = dockspace_id;

        // Split left vertically
        ImGuiID left_top = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.25f, nullptr, &left);
        ImGuiID left_mid = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.33f, nullptr, &left);
        ImGuiID left_bot = left;

        // Split center
        ImGuiID center_top = ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, 0.7f, nullptr, &center);
        ImGuiID center_bot = center;

        Logger::info("Docking windows...");
        // Dock windows
        ImGui::DockBuilderDockWindow("Objects", left_top);
        ImGui::DockBuilderDockWindow("Recipes", left_mid);
        ImGui::DockBuilderDockWindow("Pipeline", left_bot);

        ImGui::DockBuilderDockWindow("Preview", center_top);
        ImGui::DockBuilderDockWindow("Pipeline Graph", center_top);
        ImGui::DockBuilderDockWindow("Event Log", center_bot);

        ImGui::DockBuilderDockWindow("Inspector", right);
        ImGui::DockBuilderFinish(dockspace_id);

        Logger::info("Done.");

        StartupLoader::load_modules();

        menu_reset_window = false;
    }

    ImGui::End();

    // Render individual windows

    if (open_modules_debugger) PyModuleWindow::render();
    if (show_metrics) ImGui::ShowMetricsWindow();

    if (window_logger)   Logger::render();
    if (window_graph)    PipelineGraph::render();
    if (window_objects)  ObjectsPanel::render();
    if (window_pipeline) Pipeline::render();
    if (window_preview)  Preview::render();
    if (window_inspector) Inspector::render();

    if (first_run) {
        menu_reset_window = true; // reset layout on first run
        first_run = false;
    }
}

void LabLayout::destroy()
{
    Pipeline::destroy();
    PyModuleWindow::destroy();
    PipelineGraph::destroy();
    ObjectsPanel::destroy();
    Preview::destroy();
    Inspector::destroy();

    SharedUi::destroy();
    PyScope::clearInstance();
}
