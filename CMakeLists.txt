cmake_minimum_required(VERSION 3.30)
project(PearlLab)

set(CMAKE_CXX_STANDARD 20)

set(VENV_PATH "/home/xabdomo/tf-env")
set(Python3_EXECUTABLE "${VENV_PATH}/bin/python3")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")
set(IMGUI_NODE_DIR "${CMAKE_SOURCE_DIR}/libs/imgui-node-editor")
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/libs/glad")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)


set(GLAD_SOURCES ${GLAD_DIR}/src/glad.c)

set(IMGUI_NODE_SOURCES
        ${IMGUI_NODE_DIR}/crude_json.cpp
        ${IMGUI_NODE_DIR}/imgui_canvas.cpp
        ${IMGUI_NODE_DIR}/imgui_node_editor.cpp
        ${IMGUI_NODE_DIR}/imgui_node_editor_api.cpp
)

set(PEAR_LAB_SOURCES
        src/ui/font_manager.cpp
        src/ui/font_manager.hpp
        src/ui/lab_layout.cpp
        src/ui/lab_layout.hpp
        src/ui/modules/logger.cpp
        src/ui/modules/logger.hpp
        src/backend/py_scope.cpp
        src/backend/py_scope.hpp
        src/ui/modules/py_module_window.cpp
        src/ui/modules/py_module_window.hpp
        src/backend/py_param.cpp
        src/backend/py_param.hpp
        src/ui/modules/pipeline_graph.cpp
        src/ui/modules/pipeline_graph.hpp
        src/ui/shared_ui.cpp
        src/ui/shared_ui.hpp
        src/ui/modules/objects_panel.cpp
        src/ui/modules/objects_panel.hpp
        src/ui/utility/drawing.h
        src/ui/utility/drawing.cpp
        src/ui/modules/pipeline.cpp
        src/ui/modules/pipeline.hpp
        src/backend/py_agent.cpp
        src/backend/py_agent.hpp
        src/ui/startup_loader.cpp
        src/ui/startup_loader.hpp
)

# deps
find_package(Boost REQUIRED COMPONENTS python)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
find_package(glfw3 REQUIRED)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
        OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(${NUMPY_INCLUDE_DIR})
message(${Python3_INCLUDE_DIRS})

include_directories(${Boost_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIR})
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)
include_directories(${GLAD_DIR}/include)
include_directories(${IMGUI_NODE_DIR})

add_subdirectory(libs/ImGuiFileDialog)
include_directories(libs/ImGuiFileDialog)

add_executable(PearlLab
        src/main.cpp
        ${PEAR_LAB_SOURCES}
        ${IMGUI_SOURCES}
        ${GLAD_SOURCES}
        ${IMGUI_NODE_SOURCES}
)

target_link_libraries(PearlLab
        glfw
        OpenGL::GL
        ImGuiFileDialog
        ${Boost_LIBRARIES}
        ${Python3_LIBRARIES}
        pybind11::module
        yaml-cpp
)
