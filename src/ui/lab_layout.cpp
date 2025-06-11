//
// Created by xabdomo on 6/11/25.
//

#include "lab_layout.hpp"
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

#include "ImGuiFileDialog.h"
#include "../backend/py_scope.hpp"
#include "modules/logger.hpp"
#include "modules/py_module_window.hpp"

namespace LabLayout {
    // hidden because normally not other module would need these functions
    // these are just some demo functions to make the UI .. well .. do something
    // they are to be changed with the actual modules
    void renderEnvironmentsModule();
    void renderAgentsModule();
    void renderMethodsModule();
    void renderPipelineModule();
    void renderPreviewModule();
    void renderEventLogModule();
    void renderParamsModule();

}

extern GLFWwindow* AppWindow;

void LabLayout::renderEnvironmentsModule() {
    ImGui::Begin("Environments");
    ImGui::BulletText("Env1");
    ImGui::BulletText("Env2");
    ImGui::End();
}

void LabLayout::renderAgentsModule() {
    ImGui::Begin("Agents");
    ImGui::BulletText("Agent A");
    ImGui::BulletText("Agent B");
    ImGui::End();
}

void LabLayout::renderMethodsModule() {
    ImGui::Begin("Methods");
    ImGui::BulletText("Method X");
    ImGui::BulletText("Method Y");
    ImGui::End();
}

void LabLayout::renderPipelineModule() {
    ImGui::Begin("Pipeline");
    ImGui::Text("Environment: Env1");
    ImGui::Text("Experiment: TestExp");
    ImGui::Text("Agents: A, B");
    ImGui::Text("Method: X");
    ImGui::End();
}

void LabLayout::renderPreviewModule() {
    ImGui::Begin("Preview");
    ImGui::Text("Tried to write a3a3a3a3a3a3a3a3a but Arabic is not supported :\")");
    ImGui::TextWrapped("Preview area for agents in environment...");
    ImGui::End();
}

void LabLayout::renderEventLogModule() {
    ImGui::Begin("Event Log");
    ImGui::TextColored(ImVec4(1,0,0,1), "[ERROR] Failed to load AgentA");
    ImGui::Text("[INFO] Initialized");
    ImGui::End();
}

void LabLayout::renderParamsModule() {
    static char modelPath[128] = "models/agent_a.model";
    static float noise = 0.1f;
    static float learningRate = 0.01f;

    ImGui::Begin("Object Params");
    ImGui::InputText("Model Path", modelPath, IM_ARRAYSIZE(modelPath));
    ImGui::SliderFloat("Noise", &noise, 0.0f, 1.0f);
    ImGui::SliderFloat("Learning Rate", &learningRate, 0.001f, 1.0f);
    ImGui::End();
}


void LabLayout::init() {
    Logger::init();
    PyScope::init();
    PyModuleWindow::init();
}

void LabLayout::render() {

    ///
    /// MAIN MANU
    ///
    static bool open_modules_debugger = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Module")) {
                ImGuiFileDialog::Instance()->OpenDialog("FileDlgKey", "Select module file", ".py" );
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(AppWindow, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) { /* Undo Action */ }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) { /* Disabled Redo */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) { /* Cut Action */ }
            if (ImGui::MenuItem("Copy", "CTRL+C")) { /* Copy Action */ }
            if (ImGui::MenuItem("Paste", "CTRL+V")) { /* Paste Action */ }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Modules Debugger", nullptr, &open_modules_debugger);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About")) {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar(); // Closes the menu bar
    }

    if (ImGuiFileDialog::Instance()->Display("FileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk()) // If user selects a file
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            auto result = PyScope::LoadModuleForClasses(filePath);
            for (auto obj: result) {
                PyModuleWindow::modules.push_back(obj);
                Logger::info("Loaded module: " + std::string(py::str(obj.attr("__name__"))));
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }


    ///
    /// WINDOW CONTENT
    ///
    static bool p_open = true;
    static bool first_time = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
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
    if (first_time) {
        Logger::info("Initializing Lab Layout...");

        first_time = false;

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
        ImGui::DockBuilderDockWindow("Environments", left_top);
        ImGui::DockBuilderDockWindow("Agents", left_mid);
        ImGui::DockBuilderDockWindow("Methods", left);
        ImGui::DockBuilderDockWindow("Pipeline", left_bot);

        ImGui::DockBuilderDockWindow("Preview", center_top);
        ImGui::DockBuilderDockWindow("Event Log", center_bot);

        ImGui::DockBuilderDockWindow("Object Params", right);

        ImGui::DockBuilderFinish(dockspace_id);


        Logger::info("Done.");
    }

    ImGui::End();

    // Render individual windows
    renderEnvironmentsModule();
    renderAgentsModule();
    renderMethodsModule();
    renderPipelineModule();
    renderPreviewModule();
    //renderEventLogModule();
    renderParamsModule();

    Logger::render();
    if (open_modules_debugger) PyModuleWindow::render();
}

void LabLayout::destroy() {
    PyModuleWindow::destroy();
}


