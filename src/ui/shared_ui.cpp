//
// Created by xabdomo on 6/12/25.
//

#include "shared_ui.hpp"

#include <set>

#include "../backend/py_scope.hpp"
#include "modules/logger.hpp"

namespace SharedUi {
    std::vector<py::object> modules{};
    std::vector<LoadedModule> loadedModules{};

    static std::set<std::string> loadedModuleNames;

    void init() {

    }

    void pushModule(py::object module) {
        LoadedModule l;
        l.module = module;
        l.moduleName = std::string(py::str(module.attr("__module__"))) + std::string(".") + std::string(py::str(module.attr("__qualname__")));

        if (loadedModuleNames.find(l.moduleName) != loadedModuleNames.end()) {
            return; // already loaded this module
        }
        loadedModuleNames.insert(l.moduleName);
        auto& python = PyScope::getInstance();

        if (PyScope::isSubclass(module, python.pearl_agent_type)) {
            l.type = SharedUi::Agent;
        } else if (PyScope::isSubclass(module, python.pearl_env_type)) {
            l.type = SharedUi::Environment;
        } else if (PyScope::isSubclass(module, python.pearl_method_type)) {
            l.type = SharedUi::Method;
        } else if (PyScope::isSubclass(module, python.pearl_mask_type)) {
            l.type = SharedUi::Mask;
        } else {
            l.type = SharedUi::Other;
        }

        if (hasattr(module, "__annotations__")) {
            py::dict dict = module.attr("__annotations__");
            for (const auto& [key, value] : dict) {
                std::string attrName = py::str(key);
                if (py::hasattr(value, "typ")) {
                    try {
                        auto param = PyScope::parseParamFromAnnotation(value);
                        param.attrName = attrName;
                        l.annotations.push_back(param);
                    } catch (...) {
                        Logger::warning(std::string(py::str(module)) + " : failed to parse annotation for " + attrName);
                    }
                }
            }
        }

        if (hasattr(module, "__init__")) {
            py::object init = module.attr("__init__");
            auto py_instance = PyScope::getInstance();
            std::unordered_map<std::string, Param> annotations;
            if (hasattr(init, "__annotations__")) {
                py::dict init_annotations = init.attr("__annotations__");
                for (const auto& [key, value] : init_annotations) {
                    std::string attrName = py::str(key);
                    if (py::isinstance(value, py_instance.param_type)) {
                        try {
                            auto param = PyScope::parseParamFromAnnotation(value);
                            param.attrName = attrName;
                            annotations.insert({attrName, param});
                        } catch (...) {
                            Logger::warning(std::string(py::str(module)) + " : failed to parse annotation for __init__." + attrName);
                        }
                    } else { // some other type that's not a Param
                        ParamType o_type = OTHER;

                        if (py::isinstance(value, py_instance.int_type)) { // integer primitive
                            o_type = INT;
                        }

                        if (py::isinstance(value, py_instance.float_type)) { // integer primitive
                            o_type = FLOAT;
                        }

                        if (py::isinstance(value, py_instance.str_type)) { // integer primitive
                            o_type = STRING;
                        }

                        Param param;
                        param.attrName = attrName;
                        param.type = o_type;
                        param.isFilePath = false;
                        param.editable = true;
                        param.hasChoices = false;

                        param.choices = {};
                        param.disc         = "None";
                        param.defaultValue = "None";
                        param.rangeStart   = "None";
                        param.rangeEnd     = "None";

                        annotations.insert({attrName, param});
                    }
                }
            }

            // now load all parameters if we can
            if (hasattr(init, "__code__")) {
                for (const auto& name: init.attr("__code__").attr("co_varnames")) {
                    std::string attrName = py::str(name);
                    if (attrName == "self") continue; // skip self
                    if (annotations.find(attrName) != annotations.end()) {
                        l.constructor.push_back(annotations[attrName]);
                    } else {
                        Param param;
                        param.attrName = attrName;
                        param.type = OTHER;
                        param.isFilePath = false;
                        param.editable = true;
                        param.hasChoices = false;

                        param.choices = {};
                        param.disc         = "None";
                        param.defaultValue = "None";
                        param.rangeStart   = "None";
                        param.rangeEnd     = "None";

                        l.constructor.push_back(param);
                    }
                }
            } else {
                Logger::warning(std::string(py::str(module)) + " : __init__ has no __code__ attribute, skipping parameter extraction.");
                return;
            }
        }

        loadedModules.push_back(l);
        modules.push_back(module);
    }

    void destroy() {
        modules.clear();
        loadedModules.clear();
    }
}

