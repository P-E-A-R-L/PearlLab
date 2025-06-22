#ifndef PY_AGENT_HPP
#define PY_AGENT_HPP

#include "../backend/py_scope.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <optional>

#include "py_object.hpp"

namespace py = pybind11;

struct PyAgent : public PyLiveObject
{
    // Predict method: returns np.ndarray (action probabilities)
    py::object predict(const py::object &observation) const;

    // get_q_net: returns any Python object (usually a model)
    py::object get_q_net() const;

    // Optional: close()
    void close() const;

    // Accessors for spaces (may not be needed if passed only at construction)
    std::optional<py::object> get_observation_space() const;

    std::optional<py::object> get_action_space() const;
};

#endif // PY_AGENT_HPP
