#include "py_env.hpp"

std::pair<py::object, py::dict> PyEnv::reset(std::optional<int> seed, std::optional<py::dict> options)
{
    py::gil_scoped_acquire acquire{};

    py::dict kwargs;
    if (seed.has_value())
        kwargs["seed"] = py::int_(seed.value());
    if (options.has_value())
        kwargs["options"] = options.value();

    const py::tuple result = object.attr("reset")(**kwargs);
    return {result[0], result[1].cast<py::dict>()};
}

std::tuple<py::object, py::dict, bool, bool, py::dict> PyEnv::step(const py::object &action)
{
    py::gil_scoped_acquire acquire{};

    const py::tuple result = object.attr("step")(action);
    return {
        result[0],                  // observation
        result[1].cast<py::dict>(), // reward
        result[2].cast<bool>(),     // terminated
        result[3].cast<bool>(),     // truncated
        result[4].cast<py::dict>()  // info
    };
}

std::optional<py::array> PyEnv::render(const std::string &mode)
{
    py::gil_scoped_acquire acquire{};

    py::object result = object.attr("render")(mode);
    if (result.is_none())
        return std::nullopt;
    return result.cast<py::array>();
}

void PyEnv::close() const
{
    py::gil_scoped_acquire acquire{};

    if (py::hasattr(object, "close"))
        object.attr("close")();
}

std::vector<int> PyEnv::seed(std::optional<int> seed_value) const
{
    py::gil_scoped_acquire acquire{};

    if (!py::hasattr(object, "seed"))
        return {};

    py::object result;
    if (seed_value)
        result = object.attr("seed")(seed_value.value());
    else
        result = object.attr("seed")();

    return result.cast<std::vector<int>>();
}

py::object PyEnv::get_observations() const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("get_observations")();
}

std::optional<std::vector<py::object>> PyEnv::get_available_actions() const
{
    py::gil_scoped_acquire acquire{};

    if (!py::hasattr(object, "get_available_actions"))
        return std::nullopt;

    py::list actions = object.attr("get_available_actions")();
    std::vector<py::object> out;
    for (auto item : actions)
    {
        out.push_back(py::reinterpret_borrow<py::object>(item));
    }
    return out;
}

std::optional<py::object> PyEnv::unwrapped() const
{
    py::gil_scoped_acquire acquire{};

    if (!py::hasattr(object, "unwrapped"))
        return std::nullopt;
    return object.attr("unwrapped");
}
