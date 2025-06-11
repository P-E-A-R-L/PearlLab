#include <iostream>

#define GLFW_INCLUDE_NONE

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <yaml-cpp/yaml.h>

#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "backend/py_scope.hpp"
#include "ui/font_manager.hpp"
#include "ui/lab_layout.hpp"

GLFWwindow* AppWindow;

int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    auto width = 1280;
    auto height = 720;

    try {
        YAML::Node config = YAML::LoadFile("./config.yaml");
        if (config["window"]) {
            width = config["window"]["width"].as<int>();
            height = config["window"]["height"].as<int>();
        }
        auto venv = config["venv"].as<std::string>();
        std::string path = getenv("PATH");
        setenv("PATH", (venv + "/bin:" + path).c_str(), 1);
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading YAML: " << e.what() << "\n";
    }

    // Initialize python interpreter
    pybind11::scoped_interpreter guard{};

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(width, height, "Pearl Lab", nullptr, nullptr);
    AppWindow = window;
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader (e.g., glad, glew, or manual)
    // Example for glad:
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
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

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        static bool _first_render = true;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::NewFrame();

        if (_first_render) {
            LabLayout::init();
            _first_render = false;
        }

        LabLayout::render(); // render the main app

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    LabLayout::destroy();
    PyScope::clearInstance();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
