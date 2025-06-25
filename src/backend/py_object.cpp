#include "py_object.hpp"

#include <iostream>

#include "py_safe_wrapper.hpp"
#include "../ui/modules/logger.hpp"

void PyLiveObject::init(const py::dict &kwargs)
{
    py::gil_scoped_acquire acquire{};

    object = module.attr("__init__")(**kwargs);
}

py::object PyLiveObject::call(const py::dict &kwargs) const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("__call__")(**kwargs);
}

std::string PyLiveObject::repr() const
{
    py::gil_scoped_acquire acquire{};

    return object.attr("__repr__")().cast<std::string>();
}

void PyLiveObject::init_members() {
    py::gil_scoped_acquire acquire{};

    m_lock = new std::mutex();

    _private_members_values.clear();
    public_members_values.clear();

    auto& python = PyScope::getInstance();

    std::lock_guard guard(*m_lock);

    for (int i = 0; i < this->annotations.size(); ++i) {
        auto& anno = annotations[i];
        if (!SafeWrapper::execute([&] {
            _private_members_values.push_back(object.attr(anno.attrName.c_str()));

            PyPrimitive primitive{};
            if (PyScope::isSubclassOrInstance(anno.type, python.int_type)) {
                primitive.type = INTEGER;
                primitive.value.i = new int();
                (*primitive.value.i) = object.attr(anno.attrName.c_str()).cast<int>();
            } else if (PyScope::isSubclassOrInstance(anno.type, python.float_type)) {
                primitive.type = FLOAT;
                primitive.value.f = new float();
                (*primitive.value.f) = object.attr(anno.attrName.c_str()).cast<float>();
            } else if (PyScope::isSubclassOrInstance(anno.type, python.str_type)) {
                primitive.type = STRING;
                primitive.value.str = new std::string();
                (*primitive.value.str) = py::str(object.attr(anno.attrName.c_str()));
            } else {
                primitive.type = OTHER;
                primitive.value.str = new std::string();
                (*primitive.value.str) = py::str(object.attr(anno.attrName.c_str()));
            }

            public_members_values.push_back(primitive);
        })) {
            Logger::error("Failed to parse annotations for: " + moduleName);
            public_members_values.clear();
            _private_members_values.clear();
            annotations.clear();
            break;
        }
    }
}

static bool value_changed(PyPrimitive& primitive, py::object& obj) {
    if (primitive.type == OTHER) return false;
    switch (primitive.type) {
        case STRING:
            return *primitive.value.str != std::string(py::str(obj));
        case INTEGER:
            return *primitive.value.i != obj.cast<int>();
        case FLOAT:
            return *primitive.value.f != obj.cast<float>();
        default: ;
            return false;
    }
}

static py::object to_python(PyPrimitive& primitive) {
    switch (primitive.type) {
        case STRING:
            return py::str(*primitive.value.str);
        case INTEGER:
            return py::int_(*primitive.value.i);
        case FLOAT:
            return py::float_(*primitive.value.f);
        default:
            return py::none();
    }
}

static void update_value(PyPrimitive& primitive, py::object& obj) {
    switch (primitive.type) {
        case STRING:
            *primitive.value.str = std::string(py::str(obj));
            return;
        case INTEGER:
            *primitive.value.i   = obj.cast<int>();
            return;
        case FLOAT:
            *primitive.value.f   = obj.cast<float>();
            return;
        default:
            *primitive.value.str = std::string(py::str(obj));
            return;
    }
}

void PyLiveObject::update_members() {
    if (m_lock == nullptr) {
        throw std::runtime_error("Object is not initialized / or deleted");
    }

    py::gil_scoped_acquire acquire{};

    for (int i = 0; i < this->annotations.size(); ++i) {
        auto& anno = annotations[i];
        auto& pub    = public_members_values[i];
        auto& prv        = _private_members_values[i];

        if (!SafeWrapper::execute([&] {
            std::lock_guard guard(*m_lock);
            if (anno.editable) {
                // check if the value changed
                if (value_changed(pub, prv)) {
                    // write value
                    prv = to_python(pub);
                    object.attr(anno.attrName.c_str()) = prv;
                }
            }

            // read back value
            prv = object.attr(anno.attrName.c_str());
            update_value(pub, prv);
        })) {
            Logger::error("Failed to update attr: " + anno.attrName + " <" + this->moduleName + ">");
        }
    }
}

PyLiveObject::PyLiveObject() {
    object = py::none();
}

PyLiveObject::~PyLiveObject() {
    delete m_lock;
    m_lock = nullptr;
    // std::cout << "PyLiveObject::~PyLiveObject()" << std::endl;
}

