//
// Created by xabdomo on 6/20/25.
//

#include "py_object.hpp"

void PyLiveObject::init(const py::dict &kwargs)  {
    object = module.attr("__init__")(**kwargs);
}

py::object PyLiveObject::call(const py::dict &kwargs) const {
    return object.attr("__call__")(**kwargs);
}

std::string PyLiveObject::repr() const {
    return object.attr("__repr__")().cast<std::string>();
}


