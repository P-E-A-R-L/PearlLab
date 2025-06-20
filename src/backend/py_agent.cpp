//
// Created by xabdomo on 6/16/25.
//

#include "py_agent.hpp"

py::object PyAgent::predict(const py::object &observation) const {
    return object.attr("predict")(observation);
}

py::object PyAgent::get_q_net() const {
    return object.attr("get_q_net")();
}

void PyAgent::close() const {
    if (py::hasattr(object, "close"))
        object.attr("close")();
}

std::optional<py::object> PyAgent::get_observation_space() const {
    if (!py::hasattr(object, "observation_space"))
        return std::nullopt;
    return object.attr("observation_space");
}

std::optional<py::object> PyAgent::get_action_space() const {
    if (!py::hasattr(object, "action_space"))
        return std::nullopt;
    return object.attr("action_space");
}
