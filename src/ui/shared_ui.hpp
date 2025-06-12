//
// Created by xabdomo on 6/12/25.
//

#ifndef SHARED_UI_HPP
#define SHARED_UI_HPP

#include <vector>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../backend/py_param.hpp"

namespace py = pybind11;

namespace SharedUi {
    struct LoadedModule {
        py::object module;
        std::vector<Param> annotations;
        std::vector<Param> constructor;
    };

    extern std::vector<py::object> modules;
    extern std::vector<LoadedModule> loadedModules;

    void init();
    void pushModule(py::object module);
    void destroy();
};



#endif //SHARED_UI_HPP
