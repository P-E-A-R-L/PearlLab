//
// Created by xabdomo on 6/11/25.
//

#ifndef PY_PARAM_HPP
#define PY_PARAM_HPP

#include <string>
#include <vector>

enum ParamType {
    INT,
    FLOAT,
    STRING,
    OTHER
};

struct Param {
    std::string attrName;
    ParamType type;
    bool editable;
    std::string rangeStart;
    std::string rangeEnd;
    bool isFilePath;
    bool hasChoices;
    std::vector<std::string> choices;
    std::string defaultValue;
    std::string disc;
};

std::string ParamTypeAsString(ParamType);

#endif //PY_PARAM_HPP
