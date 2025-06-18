//
// Created by xabdomo on 6/18/25.
//

#ifndef PY_SAFE_WRAPPER_HPP
#define PY_SAFE_WRAPPER_HPP
#include <functional>



namespace SafeWrapper {
    void execute(const std::function<void()> &);
}



#endif //PY_SAFE_WRAPPER_HPP
