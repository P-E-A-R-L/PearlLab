//
// Created by xabdomo on 6/20/25.
//

#ifndef PY_OBJECT_HPP
#define PY_OBJECT_HPP


#include "py_scope.hpp"

struct PyLiveObject: public PyScope::LoadedModule {
    py::object object;

    // Calls: self.__init__(**kwargs)
    void init(const py::dict& kwargs = py::dict());

    // Calls: self.__call__(**kwargs)
    py::object call(const py::dict& kwargs = py::dict()) const;

    // Calls: self.__repr__()
    std::string repr() const;
};



#endif //PY_OBJECT_HPP
