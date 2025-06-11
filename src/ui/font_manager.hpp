//
// Created by xabdomo on 6/11/25.
//

#ifndef FONTMANAGER_HPP
#define FONTMANAGER_HPP

#include <string>

namespace FontManager {
    void init();
    void pushFont(const std::string& name);
    void popFont();
};



#endif //FONTMANAGER_HPP
