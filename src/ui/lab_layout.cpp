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

    { // setup theme
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.15f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.48f, 0.26f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.37f, 0.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.21f, 0.16f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.45f, 0.26f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.00f, 1.00f, 0.40f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.48f, 0.26f, 0.98f, 0.52f);
        colors[ImGuiCol_Button]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.2f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 1.00f, 1.00f, 0.3f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.34f, 0.06f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.40f, 0.26f, 0.98f, 0.50f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.20f, 0.58f, 0.73f);
        colors[ImGuiCol_TabSelected]            = ImVec4(0.29f, 0.20f, 0.68f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TabDimmed]              = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

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
    /// MAIN MENU
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
            if (Pipeline::PipelineState::experimentState != Pipeline::STOPPED) {
                ImGui::BeginDisabled();
            }

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

            if (Pipeline::PipelineState::experimentState != Pipeline::STOPPED) {
                ImGui::EndDisabled();
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
            if (Pipeline::PipelineState::experimentState != Pipeline::STOPPED) {
                ImGui::BeginDisabled();
            }

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

            if (Pipeline::PipelineState::experimentState != Pipeline::STOPPED) {
                ImGui::EndDisabled();
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
            ImGui::MenuItem("Inspector (s)", nullptr, &window_inspector);

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

        // Split left vertically
        ImGuiID right_top = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.25f, nullptr, &right);
        ImGuiID right_mid = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.33f, nullptr, &right);
        ImGuiID right_bot = right;

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

void LabLayout::dockWindow(const std::string &name, DockLocation location) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    if (ImGui::DockBuilderGetNode(dockspace_id)) {
        // Split into columns: left, center, right
        ImGuiID left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
        ImGuiID right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
        ImGuiID center = dockspace_id;

        // Split left vertically
        ImGuiID left_top = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.25f, nullptr, &left);
        ImGuiID left_mid = ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.33f, nullptr, &left);
        ImGuiID left_bot = left;

        // Split left vertically
        ImGuiID right_top = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.25f, nullptr, &right);
        ImGuiID right_mid = ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.33f, nullptr, &right);
        ImGuiID right_bot = right;

        // Split center
        ImGuiID center_top = ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, 0.7f, nullptr, &center);
        ImGuiID center_bot = center;

        auto dock = center;
        switch (location) {
            case TOP_LEFT:
                dock = left_top;
                break;
            case CENTER_LEFT:
                dock = left_mid;
                break;
            case BOT_LEFT:
                dock = left_bot;
                break;
            case TOP_RIGHT:
                dock = right_top;
                break;
            case CENTER_RIGHT:
                dock = right_mid;
                break;
            case BOT_RIGHT:
                dock = right_bot;
                break;
            case CENTER_TOP:
                dock = center_top;
                break;
            case CENTER_BOT:
                dock = center_bot;
                break;
        }

        ImGui::DockBuilderDockWindow(name.c_str(), dock);

        ImGui::DockBuilderFinish(dockspace_id);
    }
}
