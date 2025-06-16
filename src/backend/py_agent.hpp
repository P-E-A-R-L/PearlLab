//
// Created by xabdomo on 6/16/25.
//

#ifndef PY_AGENT_HPP
#define PY_AGENT_HPP
#include "../backend/py_scope.hpp"


struct PyAgent: public PyScope::LoadedModule {
    // I'll implement the actual agent logic here
    py::object object;
};


struct PyEnv: public PyScope::LoadedModule {
    // I'll implement the actual agent logic here
};

struct PyMethod: public PyScope::LoadedModule {

};



#endif //PY_AGENT_HPP
