#include "py_scope.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>
#include "../ui/modules/logger.hpp"

namespace fs = std::filesystem;

class PythonOutputRedirectorMsg
{
public:
    // std::stringstream buffer;
    void write(const std::string &msg)
    {
        if (msg.empty())
            return;
        // buffer << msg;
        Logger::message(msg);
        // flush();
    }

    void flush()
    {
        // Logger::message(buffer.str());
        // buffer.clear();
    }
};

class PythonOutputRedirectorErr
{
public:
    // std::stringstream buffer;
    void write(const std::string &msg)
    {
        if (msg.empty())
            return;
        // buffer << msg;
        Logger::error(msg);
        // flush();
    }

    void flush()
    {
        // Logger::error(buffer.str());
        // buffer.clear();
    }
};

PYBIND11_EMBEDDED_MODULE(python_redirect_msg, m)
{
    py::class_<PythonOutputRedirectorMsg>(m, "Redirector_msg")
        .def(py::init<>())
        .def("write", &PythonOutputRedirectorMsg::write)
        .def("flush", &PythonOutputRedirectorMsg::flush);
}

PYBIND11_EMBEDDED_MODULE(python_redirect_err, m)
{
    py::class_<PythonOutputRedirectorErr>(m, "Redirector_err")
        .def(py::init<>())
        .def("write", &PythonOutputRedirectorErr::write)
        .def("flush", &PythonOutputRedirectorErr::flush);
}

static PyScope *instance = nullptr;

PyScope &PyScope::getInstance()
{
    if (instance == nullptr)
    {
        instance = new PyScope();
    }
    return *instance;
}

void PyScope::clearInstance()
{
    delete instance;
    instance = nullptr;
}

py::module *PyScope::LoadModule(const std::string &module_path)
{
    fs::path path(module_path);
    Logger::info("Attempting to load module: " + module_path);
    if (!fs::exists(path) || fs::is_directory(path))
    {
        Logger::error("File: " + module_path + " doesn't exist or it's a dir.");
        return nullptr;
    }

    auto parent_path = path.parent_path();
    Logger::info("Loading module " + module_path + "...");
    Logger::info("  Include: " + parent_path.string());
    auto name = path.filename().string();
    if (name.find_last_of('.') != std::string::npos)
    {
        name = name.substr(0, name.find_last_of('.'));
    }
    Logger::info("  Name: " + name);

    try
    {
        auto &instance = getInstance();
        auto dummy = instance.sys.attr("path").attr("append")(parent_path.string());
        auto module = py::module_::import(name.c_str());
        if (module.is_none())
        {
            Logger::error("Module '" + name + "' was imported as None.");
            return nullptr;
        }

        instance.pythonModules.push_back(module);
        instance.modulesPaths.push_back(module_path);
        Logger::info("Module: " + name + " loaded successfully.");

        return &instance.pythonModules.back();
    }
    catch (py::error_already_set &e)
    {
        // Print detailed Python exception info
        std::ostringstream oss;
        oss << "Python Import Error: " << name << "\n";
        oss << e.what(); // Contains traceback and message
        Logger::error(oss.str());

        return nullptr;
    }
    catch (const std::exception &e)
    {
        Logger::error("Standard Exception while importing module '" + name + "': " + e.what());
        return nullptr;
    }
    catch (...)
    {
        Logger::error("Unknown error occurred while importing module: " + name);
        return nullptr;
    }
}

std::vector<py::object> PyScope::LoadModuleForClasses(const std::string &path)
{
    auto module = LoadModule(path);
    if (!module)
    {
        return {};
    }

    auto &instance = getInstance();
    auto param_class = instance.param_type;

    std::vector<py::object> result;

    py::dict mod_dict = py::cast<py::dict>(module->attr("__dict__"));

    for (auto item : mod_dict)
    {
        std::string class_name = py::str(item.first);
        pybind11::handle cls = item.second;
        // 1. a class and not abstract
        if (py::hasattr(cls, "__bases__"))
        {
            if (!py::bool_(instance.inspect_isabstrct(cls)))
            {
                result.push_back(py::reinterpret_borrow<py::object>(cls));
            }
            else
            {
                Logger::warning("Skipping abstract class: " + class_name);
            }
            continue;
        }

        // 2. a function
        else if (py::isinstance<py::function>(cls))
        {
            result.push_back(py::reinterpret_borrow<py::object>(cls));
            continue;
        }

        Logger::warning("Skipping element: " + class_name);
    }

    Logger::info("Module contains: " + std::to_string(result.size()) + " class(s).");

    return result;
}

bool PyScope::parseLoadedModule(py::object module, PyScope::LoadedModule &l)
{
    l.module = module;
    l.moduleName = std::string(py::str(module.attr("__module__"))) + std::string(".") + std::string(py::str(module.attr("__qualname__")));

    auto &python = PyScope::getInstance();
    bool isClass = py::hasattr(module, "__bases__");

    if (isClass)
    {
        if (PyScope::isSubclassOrInstance(module, python.pearl_agent_type))
        {
            l.type = PyScope::Agent;
        }
        else if (PyScope::isSubclassOrInstance(module, python.pearl_env_type))
        {
            l.type = PyScope::Environment;
        }
        else if (PyScope::isSubclassOrInstance(module, python.pearl_method_type))
        {
            l.type = PyScope::Method;
        }
        else if (PyScope::isSubclassOrInstance(module, python.pearl_mask_type))
        {
            l.type = PyScope::Mask;
        }
        else
        {
            l.type = PyScope::Other;
        }
    }
    else if (py::isinstance<py::function>(module))
    {
        l.type = PyScope::Function;
    }
    else
    {
        Logger::error("Unknown type for module: " + l.moduleName);
        return false;
    }

    if (l.type != PyScope::Function)
    {
        // a class
        if (hasattr(module, "__annotations__"))
        {
            py::dict dict = module.attr("__annotations__");
            for (const auto &[key, value] : dict)
            {
                std::string attrName = py::str(key);
                if (py::hasattr(value, "typ"))
                {
                    try
                    {
                        auto param = PyScope::parseParamFromAnnotation(value);
                        param.attrName = attrName;
                        l.annotations.push_back(param);
                    }
                    catch (...)
                    {
                        Logger::warning(std::string(py::str(module)) + " : failed to parse annotation for " + attrName);
                    }
                }
            }
        }

        if (hasattr(module, "__init__"))
        {
            py::object init = module.attr("__init__");
            std::unordered_map<std::string, Param> annotations;
            if (hasattr(init, "__annotations__"))
            {
                py::dict init_annotations = init.attr("__annotations__");
                for (auto &[key, value] : init_annotations)
                {
                    std::string attrName = py::str(key);
                    if (py::isinstance(value, python.param_type))
                    {
                        try
                        {
                            auto param = PyScope::parseParamFromAnnotation(value);
                            param.attrName = attrName;
                            annotations.insert({attrName, param});
                        }
                        catch (...)
                        {
                            Logger::warning(std::string(py::str(module)) + " : failed to parse annotation for __init__." + attrName);
                        }
                    }
                    else
                    { // some other type that's not a Param
                        Param param;
                        param.attrName = attrName;
                        param.type = py::reinterpret_borrow<py::object>(value);
                        param.isFilePath = false;
                        param.editable = true;
                        param.hasChoices = false;

                        param.choices = {};
                        param.disc = "None";
                        param.defaultValue = "None";
                        param.rangeStart = "None";
                        param.rangeEnd = "None";

                        annotations.insert({attrName, param});
                    }
                }
            }

            // now load all parameters if we can
            if (hasattr(init, "__code__"))
            {
                for (const auto &name : init.attr("__code__").attr("co_varnames"))
                {
                    std::string attrName = py::str(name);
                    if (attrName == "self")
                        continue; // skip self
                    if (annotations.find(attrName) != annotations.end())
                    {
                        l.constructor.push_back(annotations[attrName]);
                    }
                    else
                    {
                        Param param;
                        param.attrName = attrName;
                        param.type = py::reinterpret_borrow<py::object>(python.object_type);
                        param.isFilePath = false;
                        param.editable = true;
                        param.hasChoices = false;

                        param.choices = {};
                        param.disc = "None";
                        param.defaultValue = "None";
                        param.rangeStart = "None";
                        param.rangeEnd = "None";

                        l.constructor.push_back(param);
                    }
                }
            }
            else
            {
                Logger::warning(std::string(py::str(module)) + " : __init__ has no __code__ attribute, skipping parameter extraction.");
                return false;
            }
        }
    }
    else
    {
        // a function
        std::unordered_map<std::string, Param> annotations;
        if (hasattr(module, "__annotations__"))
        {
            py::dict init_annotations = module.attr("__annotations__");
            for (auto &[key, value] : init_annotations)
            {
                std::string attrName = py::str(key);
                if (py::isinstance(value, python.param_type))
                {
                    try
                    {
                        auto param = PyScope::parseParamFromAnnotation(value);
                        param.attrName = attrName;
                        annotations.insert({attrName, param});
                    }
                    catch (...)
                    {
                        Logger::warning(std::string(py::str(module)) + " : failed to parse annotation for function." + attrName);
                    }
                }
                else
                { // some other type that's not a Param

                    Param param;
                    param.attrName = attrName;
                    param.type = py::reinterpret_borrow<py::object>(value);
                    param.isFilePath = false;
                    param.editable = true;
                    param.hasChoices = false;
                    param.choices = {};
                    param.disc = "None";
                    param.defaultValue = "None";
                    param.rangeStart = "None";
                    param.rangeEnd = "None";

                    annotations.insert({attrName, param});
                }
            }
        }

        // now load all parameters if we can
        if (hasattr(module, "__code__"))
        {
            for (const auto &name : module.attr("__code__").attr("co_varnames"))
            {
                std::string attrName = py::str(name);
                if (attrName == "self")
                    continue; // skip self
                if (annotations.find(attrName) != annotations.end())
                {
                    l.constructor.push_back(annotations[attrName]);
                }
                else
                {
                    Param param;
                    param.attrName = attrName;
                    param.type = py::reinterpret_borrow<py::object>(python.object_type);
                    param.isFilePath = false;
                    param.editable = true;
                    param.hasChoices = false;

                    param.choices = {};
                    param.disc = "None";
                    param.defaultValue = "None";
                    param.rangeStart = "None";
                    param.rangeEnd = "None";

                    l.constructor.push_back(param);
                }
            }
        }
        else
        {
            Logger::warning(std::string(py::str(module)) + " : function has no __code__ attribute, skipping parameter extraction.");
            return false;
        }

        if (annotations.contains("return"))
        {
            l.returnType = py::reinterpret_borrow<py::object>(annotations["return"].type);
        }
        else
        {
            l.returnType = py::reinterpret_borrow<py::object>(python.object_type);
        }
    }

    return true;
}

ssize_t PyScope::argmax(const py::array &array)
{
    auto dtype = array.dtype();

    if (py::isinstance<py::array_t<float>>(array))
        return argmax_impl<float>(array.cast<py::array_t<float>>());
    else if (py::isinstance<py::array_t<double>>(array))
        return argmax_impl<double>(array.cast<py::array_t<double>>());
    else if (py::isinstance<py::array_t<int>>(array))
        return argmax_impl<int>(array.cast<py::array_t<int>>());
    else if (py::isinstance<py::array_t<long>>(array))
        return argmax_impl<long>(array.cast<py::array_t<long>>());
    else
        throw std::runtime_error("Unsupported data type");
}

PyScope::PyScope() {}

void PyScope::init()
{
    if (instance == nullptr)
    {
        instance = new PyScope();
    }

    Logger::info("Initializing Python ...");
    instance->sys = py::module_::import("sys");
    instance->inspect = py::module_::import("inspect");
    instance->builtins = py::module_::import("builtins");
    instance->numbers = py::module_::import("numbers");

    instance->pythonModules.push_back(instance->sys);
    instance->pythonModules.push_back(instance->inspect);
    instance->pythonModules.push_back(instance->builtins);
    instance->pythonModules.push_back(instance->numbers);

    instance->int_type = instance->builtins.attr("int");
    instance->float_type = instance->builtins.attr("float");
    instance->str_type = instance->builtins.attr("str");
    instance->object_type = instance->builtins.attr("object");
    instance->number_type = instance->numbers.attr("Number");

    instance->inspect_isabstrct = instance->inspect.attr("isabstract");
    instance->issubclass = instance->builtins.attr("issubclass");

    auto dummy = instance->sys.attr("path").attr("append")("./py");

    try
    {
        instance->annotations = py::module_::import("annotations");
        instance->param_type = instance->annotations.attr("Param");

        instance->pythonModules.push_back(instance->annotations);
    }
    catch (...)
    {
        // nothing to do
        Logger::error("[Lab Library] Failed to initialize Python (see errors)");
    }

    try
    {
        instance->helper = py::module_::import("lab_helpers");
        instance->isgeneric = instance->helper.attr("is_generic_type");

        instance->pythonModules.push_back(instance->helper);
    }
    catch (...)
    {
        // nothing to do
        Logger::error("[Lab Library] Failed to initialize Python (see errors)");
    }

    if (instance->annotations.is_none())
    {
        Logger::error("  Failed to import 'py/annotations' module.");
    }

    if (instance->param_type.is_none())
    {
        Logger::error("  Failed to import 'annotations' from 'py/annotations' module.");
    }

    if (instance->helper.is_none())
    {
        Logger::error("  Failed to import 'py/lab_helper' module.");
    }

    if (instance->isgeneric.is_none())
    {
        Logger::error("  Failed to import 'is_generic_type' from 'py/lab_helper' module.");
    }

    try
    {
        instance->pearl_agent = py::module_::import("pearl.agent");
        instance->pearl_env = py::module_::import("pearl.env");
        instance->pearl_method = py::module_::import("pearl.method");
        instance->pearl_mask = py::module_::import("pearl.mask");

        instance->pearl_agent_type = instance->pearl_agent.attr("RLAgent");
        instance->pearl_env_type = instance->pearl_env.attr("RLEnvironment");
        instance->pearl_method_type = instance->pearl_method.attr("ExplainabilityMethod");
        instance->pearl_mask_type = instance->pearl_mask.attr("Mask");

        instance->pythonModules.push_back(instance->pearl_agent);
        instance->pythonModules.push_back(instance->pearl_env);
        instance->pythonModules.push_back(instance->pearl_method);
        instance->pythonModules.push_back(instance->pearl_mask);
    }
    catch (...)
    {
        // nothing to do
        Logger::error("[Pearl Library] Failed to initialize Python (see errors)");
    }

    // Redirect Python output
    // instance->redirect_msg_mod = py::module_::import("python_redirect_msg");
    // instance->redirector_msg = instance->redirect_msg_mod.attr("Redirector_msg")();
    // instance->redirect_err_mod = py::module_::import("python_redirect_err");
    // instance->redirector_err = instance->redirect_err_mod.attr("Redirector_err")();
    // instance->sys.attr("stdout") = instance->redirector_msg;
    // instance->sys.attr("stderr") = instance->redirector_err;

    Logger::info("Done.");
}

bool PyScope::isSubclassOrInstance(py::handle obj, py::handle base)
{
    if (obj.is_none() && base.is_none())
    {
        return true; // both are None, considered as same type
    }

    if (getInstance().isgeneric(obj).cast<bool>())
        return true; // just treat any generic type as OBJECT

    if (getInstance().isgeneric(base).cast<bool>())
        return true; // just treat any generic type as OBJECT

    if (py::isinstance<py::type>(obj))
    {
        return getInstance().issubclass(obj, base).cast<bool>();
    }

    return py::isinstance(obj, base);
    // std::cout << std::string(py::str(obj)) << " " <<  std::string(py::str(base)) << std::endl;
}

Param PyScope::parseParamFromAnnotation(py::handle value)
{
    py::object typ = value.attr("typ");
    bool editable = py::bool_(value.attr("editable"));
    std::string rangeStart = py::str(value.attr("range_start"));
    std::string rangeEnd = py::str(value.attr("range_end"));
    std::string isFilePath = py::str(value.attr("isFilePath"));
    auto choices = value.attr("choices");
    std::string def = py::str(value.attr("default"));
    std::string disc = py::str(value.attr("disc"));

    std::vector<std::string> choice_list;
    bool hasChoices = true;
    if (isPrimitive(typ))
    {
        if (!choices.is_none())
        {
            for (auto val : choices)
            {
                choice_list.push_back(py::str(val));
            }
        }
        else
        {
            hasChoices = false;
        }
    }

    Param result;
    result.type = typ;
    result.choices = choice_list;
    result.hasChoices = hasChoices;
    result.editable = editable;
    result.isFilePath = false;
    result.rangeStart = "None"; // maybe use optional ? idk tbh ...
    result.rangeEnd = "None";
    result.defaultValue = "None";
    result.disc = disc;

    if (isPrimitive(typ))
    {
        result.rangeStart = rangeStart;
        result.rangeEnd = rangeEnd;
        result.isFilePath = (isFilePath == "True");
        result.defaultValue = def;
    }

    return result;
}
