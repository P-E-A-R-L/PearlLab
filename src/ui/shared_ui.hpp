//
// Created by xabdomo on 6/12/25.
//

#ifndef SHARED_UI_HPP
#define SHARED_UI_HPP

#include <vector>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../backend/py_param.hpp"
#include "../backend/py_scope.hpp"

namespace py = pybind11;

namespace SharedUi {

    extern std::vector<py::object> modules;
    extern std::vector<PyScope::LoadedModule> loadedModules;

    void init();
    void pushModule(const py::object& module);
    void destroy();


    struct Pipeline {

    };
}



#endif //SHARED_UI_HPP
