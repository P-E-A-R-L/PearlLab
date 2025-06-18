//
// Created by xabdomo on 6/18/25.
//

#include "py_safe_wrapper.hpp"
#include <pybind11/pybind11.h>
#include "../ui/modules/logger.hpp"

namespace py = pybind11;

void SafeWrapper::execute(const std::function<void()> &f) {
    try {
        f();
    } catch (const py::error_already_set& e) {
        Logger::error(std::format("Python exception occurred: {}", e.what()));
        if (e.matches(PyExc_TypeError)) {
            Logger::error("TypeError in Python call");
        } else if (e.matches(PyExc_RuntimeError)) {
            Logger::error("RuntimeError in Python call");
        }
        // Print full traceback, if needed
        py::print(e.trace());

    } catch (const std::exception& e) {
        Logger::error(std::format("Standard exception during Python call: {}", e.what()));
    } catch (...) {
        Logger::error("Unknown exception during Python call");
    }
}
