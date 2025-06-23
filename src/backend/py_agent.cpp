#include "py_agent.hpp"

py::object PyAgent::predict(const py::object &observation) const {
    py::gil_scoped_acquire acquire{};

    if (!object || object.is_none()) {
        throw std::runtime_error("PyAgent::predict called on null or uninitialized object");
    }

    return object.attr("predict")(observation);
}

py::object PyAgent::get_q_net() const {
    py::gil_scoped_acquire acquire{};

    if (!object || object.is_none()) {
        throw std::runtime_error("PyAgent::get_q_net called on null or uninitialized object");
    }

    return object.attr("get_q_net")();
}

void PyAgent::close() const {
    py::gil_scoped_acquire acquire{};

    if (!object || object.is_none()) {
        throw std::runtime_error("PyAgent::close called on null or uninitialized object");
    }

    if (py::hasattr(object, "close"))
        object.attr("close")();
}

std::optional<py::object> PyAgent::get_observation_space() const {
    py::gil_scoped_acquire acquire{};

    if (!object || object.is_none()) {
        throw std::runtime_error("PyAgent::observation_space called on null or uninitialized object");
    }

    if (!py::hasattr(object, "observation_space"))
        return std::nullopt;

    return object.attr("observation_space");
}

std::optional<py::object> PyAgent::get_action_space() const {
    py::gil_scoped_acquire acquire{};

    if (!object || object.is_none()) {
        throw std::runtime_error("PyAgent::action_space called on null or uninitialized object");
    }

    if (!py::hasattr(object, "action_space"))
        return std::nullopt;

    return object.attr("action_space");
}
