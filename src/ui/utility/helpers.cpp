//
// Created by xabdomo on 6/24/25.
//

#include "helpers.hpp"

#include <cstring>

std::pair<char*, size_t> Helpers::toImGuiList(const std::vector<std::string> & names) {
    size_t size = 0;
    for (auto &name : names) {
        size += name.length() + 1;
    }

    const auto data = new char[size + 1];
    size_t offset = 0;
    for (int i = 0; i < names.size(); i++) {
        strcpy(data + offset, names[i].c_str());
        offset += names[i].size() + 1;
    }

    data[size] = '\0'; // Null-terminate the for list

    return {data, size};
}
