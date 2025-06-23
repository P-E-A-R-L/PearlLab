#include "py_object.hpp"

void PyLiveObject::init(const py::dict &kwargs)
{
    py::gil_scoped_acquire acquire{};

    object = module.attr("__init__")(**kwargs);
}

py::object PyLiveObject::call(const py::dict &kwargs) const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("__call__")(**kwargs);
}

std::string PyLiveObject::repr() const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("__repr__")().cast<std::string>();
}
