//
// Created by xabdomo on 6/11/25.
//

#include "py_scope.hpp"
#include <filesystem>
#include <iostream>

#include "../ui/modules/logger.hpp"

namespace fs = std::filesystem;


static PyScope* instance = nullptr;

PyScope & PyScope::getInstance() {
    if (instance == nullptr) {
        instance = new PyScope();
    }
    return *instance;
}

void PyScope::clearInstance() {
    delete instance;
    instance = nullptr;
}

py::module* PyScope::LoadModule(const std::string &module_path) {
    fs::path path(module_path);
    Logger::info("Attempting to load module: " + module_path);
    if (!fs::exists(path) || fs::is_directory(path)) {
        Logger::error("File: " + module_path + " doesn't exist or it's a dir.");
        return nullptr;
    }

    auto parent_path = path.parent_path();
    Logger::info("Loading module " + module_path + "...");
    Logger::info("  Include: " + parent_path.string());
    auto name = path.filename().string();
    if (name.find_last_of('.') != std::string::npos) {
        name = name.substr(0, name.find_last_of('.'));
    }
    Logger::info("  Name: " + name);

    try {
        auto& instance = getInstance();
        auto dummy = instance.sys.attr("path").attr("append")(parent_path.string());
        auto module = py::module_::import(name.c_str());
        if (module.is_none()) {
            Logger::error("Module '" + name + "' was imported as None.");
            return nullptr;
        }
        instance.pythonModules.push_back(module);
        Logger::info("Module: " + name + " loaded successfully.");
        return &instance.pythonModules.back();

    } catch (py::error_already_set& e) {
        // Print detailed Python exception info
        std::ostringstream oss;
        oss << "Python Import Error: " << name << "\n";
        oss << e.what();  // Contains traceback and message
        Logger::error(oss.str());

        return nullptr;
    } catch (const std::exception& e) {
        Logger::error("Standard Exception while importing module '" + name + "': " + e.what());
        return nullptr;

    } catch (...) {
        Logger::error("Unknown error occurred while importing module: " + name);
        return nullptr;
    }

}

std::vector<py::object> PyScope::LoadModuleForClasses(const std::string &path) {
    auto module = LoadModule(path);
    if (!module) {
        return {};
    }

    auto& instance = getInstance();
    auto param_class = instance.param_type;

    std::vector<py::object> result;

    py::dict mod_dict = py::cast<py::dict>(module->attr("__dict__"));

    for (auto item : mod_dict) {
        std::string class_name = py::str(item.first);
        pybind11::handle cls = item.second;
        // 1. Must be a class
        if (!py::hasattr(cls, "__bases__"))
            continue;

        // Passed all checks
        result.push_back(py::reinterpret_borrow<py::object>(cls));
    }

    Logger::info("Module contains: " + std::to_string(result.size()) + " class(s).");

    return result;
}

PyScope::PyScope() {}

void PyScope::init() {
    if (instance == nullptr) {
        instance = new PyScope();
    }

    Logger::info("Initializing Python ...");
    instance->sys = py::module_::import("sys");
    instance->builtins     = py::module_::import("builtins");
    instance->int_type   = instance->builtins.attr("int");
    instance->float_type   = instance->builtins.attr("float");
    instance->str_type   = instance->builtins.attr("str");

    auto dummy = instance->sys.attr("path").attr("append")("./py");

    try {
        instance->annotations = py::module_::import("annotations");
        instance->param_type = instance->annotations.attr("Param");

        instance->pythonModules.push_back(instance->sys);
        instance->pythonModules.push_back(instance->builtins);
        instance->pythonModules.push_back(instance->annotations);

        Logger::info("Done.");
    } catch (...) {
        // nothing to do
        Logger::error("Failed to initialize Python (see errors)");
    }

    if (instance->annotations.is_none()) {
        Logger::error("  Failed to import 'py/annotations' module.");
    }

    if (instance->param_type.is_none()) {
        Logger::error("  Failed to import 'annotations' from 'py/annotations' module.");
    }


}

Param PyScope::parseParamFromAnnotation(py::handle value) {
    std::string typ = py::str(value.attr("typ").attr("__name__"));
    bool editable = py::bool_(value.attr("editable"));
    std::string rangeStart = py::str(value.attr("range_start"));
    std::string rangeEnd = py::str(value.attr("range_end"));
    std::string isFilePath = py::str(value.attr("isFilePath"));
    auto choices = value.attr("choices");
    std::string def = py::str(value.attr("default"));
    std::string disc = py::str(value.attr("disc"));

    ParamType o_type;

    if (typ == "int")
        o_type = INT;
    else if (typ == "float")
        o_type = FLOAT;
    else if (typ == "str")
        o_type = STRING;
    else
        o_type = OTHER;

    std::vector<std::string> choice_list;
    bool hasChoices = true;
    if (o_type != OTHER) {
        if (!choices.is_none()) {
            for (auto val: choices) {
                choice_list.push_back(py::str(val));
            }
        } else {
            hasChoices = false;
        }
    }

    Param result;
    result.type         = o_type;
    result.choices      = choice_list;
    result.hasChoices   = hasChoices;
    result.editable     = editable;
    result.isFilePath   = false;
    result.rangeStart   = "None"; // maybe use optional ? idk tbh ...
    result.rangeEnd     = "None";
    result.defaultValue = "None";
    result.disc         = disc;

    if (o_type != OTHER) {
        result.rangeStart   = rangeStart;
        result.rangeEnd     = rangeEnd;
        result.isFilePath   = (isFilePath == "True");
        result.defaultValue = def;
    }

    return result;
}
