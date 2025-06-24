#ifndef PY_OBJECT_HPP
#define PY_OBJECT_HPP

#include "py_scope.hpp"

enum PrimitiveType {
    STRING,
    FLOAT,
    INTEGER,
    OTHER    // will be represented as string, but will not be editable
};

union PrimitiveValue
{
    std::string    *str;
    float          *f;
    int            *i;
};

struct PyPrimitive {
    PrimitiveValue value;
    PrimitiveType type;
};


struct PyLiveObject : public PyScope::LoadedModule
{
    py::object  object;
    std::mutex* m_lock = nullptr;

    // Calls: self.__init__(**kwargs)
    void init(const py::dict &kwargs = py::dict());

    // Calls: self.__call__(**kwargs)
    py::object call(const py::dict &kwargs = py::dict()) const;

    // Calls: self.__repr__()
    std::string repr() const;

    std::vector<py::object> _private_members_values;
    std::vector<PyPrimitive> public_members_values;

    void init_members();
    void update_members();

    PyLiveObject() = default;
    PyLiveObject(const PyLiveObject&) = delete;

    virtual ~PyLiveObject();
};

#endif // PY_OBJECT_HPP
