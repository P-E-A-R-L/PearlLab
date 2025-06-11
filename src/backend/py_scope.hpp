//
// Created by xabdomo on 6/11/25.
//

#ifndef PYSCOPE_HPP
#define PYSCOPE_HPP
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "py_param.hpp"

namespace py = pybind11;

class PyScope {
public:
    py::module annotations;
    py::object param_class;
    py::module sys;
    std::vector<py::module> modules;
    std::vector<py::module> pythonModules;

    static PyScope& getInstance();
    static void clearInstance();

    static py::module* LoadModule(const std::string& path);
    static std::vector<py::object> LoadModuleForClasses(const std::string& path);
    static void init();

    static Param parseParamFromAnnotation(py::handle annotation);
private:
    PyScope();
};



#endif //PYSCOPE_HPP
