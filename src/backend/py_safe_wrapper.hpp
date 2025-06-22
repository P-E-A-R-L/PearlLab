#ifndef PY_SAFE_WRAPPER_HPP
#define PY_SAFE_WRAPPER_HPP
#include <functional>

namespace SafeWrapper
{
    bool execute(const std::function<void()> &);
}

#endif // PY_SAFE_WRAPPER_HPP
