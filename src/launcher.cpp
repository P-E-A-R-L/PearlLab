//
// Created by xabdomo on 6/22/25.
//

#define GLFW_INCLUDE_NONE

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ui/font_manager.hpp"
#include "ImGuiFileDialog.h"
#include <string>

#include "ui/utility/image_store.hpp"

static bool _run_application         = false;
static bool _application_initialized = false;
static std::string project_path             = "";

void OpenProject(const std::string& folderPath) {
    _run_application = true;
    project_path     = folderPath;
}

GLFWwindow* AppWindow;

void ShowLauncherUI() {
    // Get full viewport size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screenSize = viewport->Size;

    // Position and size the window to fill the entire screen
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // square corners
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // Fullscreen, no decoration window
    ImGui::Begin("Launcher", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Calculate button position
    float buttonWidth = 200.0f;
    float buttonHeight = 40.0f;

    float centerX = (screenSize.x - buttonWidth) * 0.5f;
    float centerY = screenSize.y * 0.60f;

    ImGui::SetCursorPos(ImVec2(centerX, centerY));
    if (ImGui::Button("Open Project", ImVec2(buttonWidth, buttonHeight))) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseProjectDir",
            "Select Project Folder",
            nullptr // No filter -> folder picker
        );
    }

    centerX = (screenSize.x - 360) * 0.5f;
    centerY = screenSize.y * 0.1f;
    ImGui::SetCursorPos(ImVec2(centerX, centerY));
    ImGui::Image(ImageStore::idOf("./assets/icons/pearl.png"), {360, 360});

    // Display file dialog
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
    if (ImGuiFileDialog::Instance()->Display("ChooseProjectDir",
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoSavedSettings)) {

        // After user selects a folder
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string folderPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            OpenProject(folderPath);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();
    ImGui::PopStyleVar(2); // Pop rounding and border size
}


// application entry point
int application_init(const std::string& project_path);
int application_loop();
int application_destroy();

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) return -1 ;

    auto width  = 600;
    auto height = 600;

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(width, height, "Pearl Lab Launcher", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    AppWindow = window;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync


    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Init font manager
    FontManager::init();
    ImageStore::init();

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::NewFrame();

        // LabLayout::render(); // render the main app
        if (_run_application) {
            if (!_application_initialized) {
                _application_initialized = true;
                application_init(project_path);
            }

            application_loop();
        } else {
            ShowLauncherUI();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (_application_initialized) application_destroy();

    ImageStore::destroy();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}