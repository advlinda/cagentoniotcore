#pragma once

#include <utility>
#include <exception>

namespace mstc
{
namespace base
{
// T : resource traits require two functions
//     static HLOCAL invalid()
//     static void destroy()
// R : resource type
template <typename T, typename R>
class UniqueResource : public T
{
public:
    using exception_ptr = std::exception_ptr;

    UniqueResource(UniqueResource&& rhs) noexcept :
    res_{ rhs.detach() } {}

    explicit UniqueResource(R&& res = T::invalid()) noexcept :
        res_{ std::move(res) }
    {
        // res might (not) be clean up by above statement
        res = T::invalid(); // ensure to clean up
    }

    ~UniqueResource()
    {
        destroy();
    }

    UniqueResource& operator =(R&& res) noexcept
    {
        destroy();
        res_ = res;
        // res might (not) be clean up by above statement
        res = T::invalid(); // ensure to clean up
        return *this;
    }

    UniqueResource& operator =(UniqueResource&& rhs) noexcept
    {
        destroy();
        res_ = rhs.detach();
        return *this;
    }

    const R& get() const noexcept { return res_; }

    operator const R&() const noexcept { return res_; }

    exception_ptr attach(R&& res) noexcept
    {
        auto ret = destroy();
        res_ = std::move(res);
        // res might (not) be clean up by above statement
        res = T::invalid();
        return ret;
    }

    R detach() noexcept
    {
        auto res = res_;
        res_ = T::invalid();
        return res;
    }

    exception_ptr destroy() noexcept
    {
        if (res_ == T::invalid()) { return nullptr; }

        auto ret = T::destroy(res_);
        res_ = T::invalid();
        return ret;
    }

protected:
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator =(const UniqueResource&) = delete;

    R res_;
}; // UniqueResource
} // base
} // mstc
