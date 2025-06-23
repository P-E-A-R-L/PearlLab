//
// Created by xabdomo on 6/23/25.
//


#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <thread>
#include <chrono>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

int main() {
    py::scoped_interpreter guard{}; // Start the Python interpreter

    py::print("Hello from external thread");

    // py::gil_scoped_release release1{};

    std::thread([] {
        py::gil_scoped_acquire acquire{};
        py::print("Hello, world! from 1");
    }).detach();

    std::thread([] {
        py::gil_scoped_acquire acquire{};
        py::print("Hello, world! from 2");
    }).detach();

    py::gil_scoped_release release2{};

    while (true) {
        // nothing
    }
}