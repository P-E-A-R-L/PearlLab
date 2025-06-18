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

void PyMethod::onStepAfter(const py::object& action) const {
    object.attr("onStepAfter")(action);
}

py::object PyMethod::explain(const py::object& obs) const {
    return object.attr("explain")(obs);
}

double PyMethod::value(const py::object& obs) const {
    return object.attr("value")(obs).cast<double>();
}

bool PyMethod::supports(VisualizationMethod m) const {
    return object.attr("supports")(static_cast<int>(m)).cast<bool>();
}

std::optional<py::object> PyMethod::getVisualizationParamsType(VisualizationMethod m) const {
    py::object result = object.attr("getVisualizationParamsType")(static_cast<int>(m));
    if (result.is_none())
        return std::nullopt;
    return result;
}

std::optional<py::object> PyMethod::getVisualization(VisualizationMethod m, const py::object& params) const {
    py::object result = object.attr("getVisualization")(static_cast<int>(m), params);
    if (result.is_none())
        return std::nullopt;

    // Could be a numpy array or a dict — return as generic py::object
    return result;
}
