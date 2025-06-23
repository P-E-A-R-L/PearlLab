#include "py_safe_wrapper.hpp"
#include <pybind11/pybind11.h>
#include "../ui/modules/logger.hpp"

namespace py = pybind11;

bool SafeWrapper::execute(const std::function<void()> &f){
    try{
        py::gil_scoped_acquire acquire{};
        f();
        return true;
    } catch (const py::error_already_set &e) {
        // Logger::error(std::format("[Python]: {}", e.what()));
        Logger::error("[Python]: " + std::string(e.what()));
        try {
            py::print(e.trace());
        } catch (...) { // for the love of god, sometimes it crashes and idk how ...
        }
    } catch (const std::exception &e) {
        // Logger::error(std::format("[Runtime]: {}", e.what()));
        Logger::error("[Runtime]: " + std::string(e.what()));
    } catch (...) {
        Logger::error("Unknown exception during Python call");
    }

    return false;
}
