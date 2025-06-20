//
// Created by xabdomo on 6/18/25.
//

#include "py_method.hpp"

void PyMethod::set(const py::object& env) const {
    object.attr("set")(env);
}

void PyMethod::prepare(const py::object& agent) const {
    object.attr("prepare")(agent);
}

void PyMethod::onStep(const py::object& action) const {
    object.attr("onStep")(action);
}

void PyMethod::onStepAfter(const py::object &action, const py::dict& reward, const bool done, const py::dict &info) const {
    object.attr("onStepAfter")(action, reward, done, info);
}

py::object PyMethod::explain(const py::object& obs) const {
    return object.attr("explain")(obs);
}

double PyMethod::value(const py::object& obs) const {
    return object.attr("value")(obs).cast<double>();
}

