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
    // internal modules
    py::module annotations;
    py::module sys;
    py::module builtins;

    // handy modules
    py::module inspect;
    py::module numbers;

    // pearl models
    py::module pearl_agent;
    py::module pearl_env;
    py::module pearl_method;
    py::module pearl_mask;

    // lab types
    py::object param_type;

    // primitive types
    py::object int_type;
    py::object float_type;
    py::object str_type;
    py::object number_type;
    py::object object_type;

    // pearl types
    py::object pearl_agent_type;
    py::object pearl_env_type;
    py::object pearl_method_type;
    py::object pearl_mask_type;

    // handy functions
    py::object inspect_isabstrct;
    py::object issubclass;


    // output redirectors
    py::module redirect_msg_mod;
    py::module redirect_err_mod;
    py::object redirector_msg;
    py::object redirector_err;

    std::vector<py::module> modules;
    std::vector<py::module> pythonModules;

    static PyScope& getInstance();
    static void clearInstance();

    static py::module* LoadModule(const std::string& path);
    static std::vector<py::object> LoadModuleForClasses(const std::string& path);
    static void init();

    static bool isSubclassOrInstance(py::handle obj, py::handle base);

    static Param parseParamFromAnnotation(py::handle annotation);
private:
    PyScope();
};



#endif //PYSCOPE_HPP
