#pragma once

#include <utility>

namespace mstc
{
namespace base
{
template <typename F>
class ScopeF
{
public:
    explicit ScopeF(F&& f) noexcept : f_(std::move(f)) {}

    ~ScopeF()
    {
        (f_)();
    }

protected:
    F f_;
}; // ScopeF

// factory function
template <typename F> ScopeF<F> scope(F&& f) noexcept
{
    return ScopeF<F>(std::move(f));
}

} // base
} // mstc
