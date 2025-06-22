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

extern GLFWwindow* AppWindow;

int application_init(const std::string& project_path) {

    auto width = 1280;
    auto height = 720;

    GLFWwindow* window = AppWindow;

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


    glfwSetWindowTitle(window, "Pearl Lab");
    glfwSetWindowSize(window, width, height);

    pybind11::initialize_interpreter();
    return 0;
}


int application_loop() {
    static bool _first_render = true;

    if (_first_render) {
        LabLayout::init();
        _first_render = false;
    }

    LabLayout::render(); // render the main app
    return 0;
}

int application_destroy() {
    LabLayout::destroy();
    pybind11::finalize_interpreter();
    return 0;
}
