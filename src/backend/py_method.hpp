//
// Created by xabdomo on 6/18/25.
//

#ifndef PY_METHOD_HPP
#define PY_METHOD_HPP

#include "../backend/py_scope.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <optional>
#include <string>

namespace py = pybind11;

enum class VisualizationMethod {
    Image = 0,
    Features = 1
};

struct PyMethod : public PyScope::LoadedModule {
    py::object object;

    // Calls: self.set(env)
    void set(const py::object& env) const;

    // Calls: self.prepare(agent)
    void prepare(const py::object& agent) const;

    // Calls: self.onStep(action)
    void onStep(const py::object& action) const;

    // Calls: self.onStepAfter(action)
    void onStepAfter(const py::object& action) const;

    // Calls: self.explain(obs)
    py::object explain(const py::object& obs) const;

    // Calls: self.value(obs)
    double value(const py::object& obs) const;

    // Optional: self.supports(method) -> bool
    bool supports(VisualizationMethod m) const;

    // Optional: self.getVisualizationParamsType(method) -> type | None
    [[nodiscard]] std::optional<py::object> getVisualizationParamsType(VisualizationMethod m) const;

    // Optional: self.getVisualization(method, params) -> np.ndarray | None
    [[nodiscard]] std::optional<py::object> getVisualization(VisualizationMethod m, const py::object& params) const;
};


#endif //PY_METHOD_HPP
