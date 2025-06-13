//
// Created by xabdomo on 6/11/25.
//

#include "py_param.hpp"
#include "py_scope.hpp"


std::string ParamTypeAsString(const py::object & t) {
    auto& instance = PyScope::getInstance();
    if (t.is_none()) return "<None>";
    if (py::isinstance<py::function>(t)) return "<func>";
    if (PyScope::isSubclass(t, instance.str_type))   return "string";
    if (PyScope::isSubclass(t, instance.float_type)) return "float";
    if (PyScope::isSubclass(t, instance.int_type))   return "int";
    return "<other>";
}

bool isPrimitive(const py::object & t) {
    auto& instance = PyScope::getInstance();
    if (t.is_none()) return false;
    if (PyScope::isSubclass(t, instance.str_type))   return true;
    if (PyScope::isSubclass(t, instance.float_type)) return true;
    if (PyScope::isSubclass(t, instance.int_type))   return true;
    return false;
}
