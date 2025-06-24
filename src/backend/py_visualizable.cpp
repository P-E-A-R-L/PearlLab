#include "py_visualizable.hpp"

bool PyVisualizable::supports(VisualizationMethod m) const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("supports")(static_cast<int>(m)).cast<bool>();
}

std::optional<py::object> PyVisualizable::getVisualizationParamsType(VisualizationMethod m) const
{
    py::gil_scoped_acquire acquire{};

    py::object result = object.attr("getVisualizationParamsType")(static_cast<int>(m));
    if (result.is_none())
        return std::nullopt;
    return result;
}

std::optional<py::object> PyVisualizable::getVisualization(VisualizationMethod m, const py::object &params) const
{
    py::gil_scoped_acquire acquire{};

    py::object result = object.attr("getVisualization")(static_cast<int>(m), params);
    if (result.is_none())
        return std::nullopt;

    // Could be a numpy array or a dict — return as generic py::object
    return result;
}

std::vector<VisualizationMethod> PyVisualizable::getSupportedMethods() const
{
    py::gil_scoped_acquire acquire{};

    std::vector<VisualizationMethod> methods;
    for (int i = 0; i < static_cast<int>(VisualizationMethod::HEAT_MAP) + 1; ++i)
    {
        if (supports(static_cast<VisualizationMethod>(i)))
        {
            methods.push_back(static_cast<VisualizationMethod>(i));
        }
    }
    return methods;
}