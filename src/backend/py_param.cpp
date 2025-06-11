//
// Created by xabdomo on 6/11/25.
//

#include "py_param.hpp"

std::string ParamTypeAsString(ParamType t) {
    if (t == STRING) return "string";
    if (t == FLOAT) return "float";
    if (t == INT) return "int";
    return "<other>";
}
