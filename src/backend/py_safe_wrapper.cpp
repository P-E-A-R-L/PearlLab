//
// Created by xabdomo on 6/18/25.
//

#include "py_safe_wrapper.hpp"
#include <pybind11/pybind11.h>
#include "../ui/modules/logger.hpp"

namespace py = pybind11;

bool SafeWrapper::execute(const std::function<void()> &f) {
    try {
        f();
        return true;
    } catch (const py::error_already_set& e) {
        Logger::error(std::format("[Python]: {}", e.what()));
        py::print(e.trace());

    } catch (const std::exception& e) {
        Logger::error(std::format("[Runtime]: {}", e.what()));
    } catch (...) {
        Logger::error("Unknown exception during Python call");
    }

    return false;
}
