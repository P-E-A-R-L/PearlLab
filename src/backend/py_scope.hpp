#ifndef PYSCOPE_HPP
#define PYSCOPE_HPP

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "py_param.hpp"

namespace py = pybind11;

class PyScope
{
public:
    // internal modules
    py::module annotations;
    py::module sys;
    py::module builtins;

    // handy modules
    py::module inspect;
    py::module numbers;
    py::module helper;

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
    py::object isgeneric;

    // output redirectors
    py::module redirect_msg_mod;
    py::module redirect_err_mod;
    py::object redirector_msg;
    py::object redirector_err;

    std::vector<std::string> modulesPaths;
    std::vector<py::module> pythonModules;

    static PyScope &getInstance();
    static void clearInstance();

    static py::module *LoadModule(const std::string &path);
    static std::vector<py::object> LoadModuleForClasses(const std::string &path);
    static void init();

    static bool isSubclassOrInstance(py::handle obj, py::handle base);

    static Param parseParamFromAnnotation(py::handle annotation);

    enum ModuleType
    {
        Agent,
        Environment,
        Method,
        Mask,
        Function,
        Other
    };

    struct LoadedModule
    {
        py::object module;
        py::object returnType; // for functions
        std::string moduleName;
        std::vector<Param> annotations;
        std::vector<Param> constructor;
        ModuleType type = Other; // default to Other
    };

    static bool parseLoadedModule(py::object obj, PyScope::LoadedModule &l);

    template <typename T>
    static ssize_t argmax_impl(py::array_t<T> array)
    {
        py::gil_scoped_acquire acquire{};

        auto buf = array.unchecked();
        if (buf.size() == 0)
            throw std::runtime_error("Input array is empty");

        ssize_t max_index = 0;
        T max_value = *buf.data(0);
        for (ssize_t i = 1; i < buf.size(); ++i)
        {
            if (*buf.data(i) > max_value)
            {
                max_value = *buf.data(i);
                max_index = i;
            }
        }
        return max_index;
    }

    static ssize_t argmax(const py::array &array);

private:
    PyScope();
};

#endif // PYSCOPE_HPP
