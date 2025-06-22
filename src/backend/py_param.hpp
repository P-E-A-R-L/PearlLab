#ifndef PY_PARAM_HPP
#define PY_PARAM_HPP

#include <string>
#include <vector>

#include <pybind11/pybind11.h>

namespace py = pybind11;

struct Param
{
    std::string attrName;
    py::object type = py::none();
    bool editable;
    std::string rangeStart;
    std::string rangeEnd;
    bool isFilePath;
    bool hasChoices;
    std::vector<std::string> choices;
    std::string defaultValue;
    std::string disc;
};

std::string ParamTypeAsString(const py::object &);
bool isPrimitive(const py::object &);

#endif // PY_PARAM_HPP
