//
// Created by xabdomo on 6/18/25.
//

#ifndef PY_METHOD_HPP
#define PY_METHOD_HPP

#include "../backend/py_scope.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <optional>

#include "py_visualizable.hpp"

namespace py = pybind11;


struct PyMethod : public PyVisualizable {

    // Calls: self.set(env)
    void set(const py::object& env) const;

    // Calls: self.prepare(agent)
    void prepare(const py::object& agent) const;

    // Calls: self.onStep(action)
    void onStep(const py::object& action) const;

    // Calls: self.onStepAfter(action)
    void onStepAfter(const py::object& action, const py::dict& reward, const bool done, const py::dict& info) const;

    // Calls: self.explain(obs)
    py::object explain(const py::object& obs) const;

    // Calls: self.value(obs)
    double value(const py::object& obs) const;
};


#endif //PY_METHOD_HPP
