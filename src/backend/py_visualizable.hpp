#ifndef PY_VISUALIZABLE_HPP
#define PY_VISUALIZABLE_HPP

#include <pybind11/pybind11.h>
#include <optional>

#include "py_object.hpp"
#include "visualization_method.hpp"

namespace py = pybind11;

struct PyVisualizable : public PyLiveObject
{

    // Optional: self.supports(method) -> bool
    bool supports(VisualizationMethod m) const;

    // Optional: self.getVisualizationParamsType(method) -> type | None
    [[nodiscard]] std::optional<py::object> getVisualizationParamsType(VisualizationMethod m) const;

    // Optional: self.getVisualization(method, params) -> np.ndarray | None
    [[nodiscard]] std::optional<py::object> getVisualization(VisualizationMethod m, const py::object &params = py::none()) const;

    std::vector<VisualizationMethod> getSupportedMethods() const;
};

#endif // PY_VISUALIZABLE_HPP
