#include "py_method.hpp"

void PyMethod::set(const py::object &env) const
{
    py::gil_scoped_acquire acquire{};

    object.attr("set")(env);
}

void PyMethod::prepare(const py::object &agent) const
{
    py::gil_scoped_acquire acquire{};

    object.attr("prepare")(agent);
}

void PyMethod::onStep(const py::object &action) const
{
    py::gil_scoped_acquire acquire{};

    object.attr("onStep")(action);
}

void PyMethod::onStepAfter(const py::object &action, const py::dict &reward, const bool done, const py::dict &info) const
{
    py::gil_scoped_acquire acquire{};

    object.attr("onStepAfter")(action, reward, done, info);
}

py::object PyMethod::explain(const py::object &obs) const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("explain")(obs);
}

double PyMethod::value(const py::object &obs) const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("value")(obs).cast<double>();
}
