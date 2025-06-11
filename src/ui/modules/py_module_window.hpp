//
// Created by xabdomo on 6/11/25.
//

#ifndef PY_MODULE_WINDOW_HPP
#define PY_MODULE_WINDOW_HPP
#include <vector>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;



namespace PyModuleWindow {
    extern std::vector<py::object> modules;

    void init();
    void render();
    void destroy();
};



#endif //PY_MODULE_WINDOW_HPP
