#ifndef PY_ENV_HPP
#define PY_ENV_HPP

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <optional>
#include <vector>
#include <string>

#include "py_visualizable.hpp"

namespace py = pybind11;

struct PyEnv : public PyVisualizable
{

    // Call: env.reset(seed=..., options=...)
    std::pair<py::object, py::dict> reset(std::optional<int> seed = std::nullopt,
                                          std::optional<py::dict> options = std::nullopt);

    // Call: env.step(action)
    std::tuple<py::object, py::dict, bool, bool, py::dict> step(const py::object &action);

    // Call: env.render(mode)
    std::optional<py::array> render(const std::string &mode = "human");

    // Call: env.close()
    void close() const;

    // Call: env.seed(seed)
    [[nodiscard]] std::vector<int> seed(std::optional<int> seed_value = std::nullopt) const;

    // Call: env.get_observations()
    [[nodiscard]] py::object get_observations() const;

    // Optional: env.get_available_actions()
    [[nodiscard]] std::optional<std::vector<py::object>> get_available_actions() const;

    // Optional: env.unwrapped
    [[nodiscard]] std::optional<py::object> unwrapped() const;
};

#endif // PY_ENV_HPP
