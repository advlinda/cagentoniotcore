#pragma once

#include <utility>
#include <exception>

namespace mstc
{
namespace base
{
template <typename T>
struct ResourceTraits
{
    using R = T;
    using exception_ptr = std::exception_ptr;

    constexpr static R invalid() noexcept
    {
        return nullptr;
    }

    // Traits should provide its own destroy
    //  static exception_ptr destroy(R r) noexcept
    //  {
    //      return nullptr;
    //  }
};

// Traits : resource traits require two functions
//          R is resource type
//          static R invalid()
//          static exception_ptr destroy()
template <typename Traits>
class UniqueResource
{
public:
    using traits_type = Traits;
    using R = typename traits_type::R;
    using exception_ptr = std::exception_ptr;

    UniqueResource(UniqueResource&& rhs) noexcept :
        res_{ rhs.detach() } {}

    explicit UniqueResource(R&& res = traits_type::invalid()) noexcept :
        res_{ std::move(res) }
    {
        // res might (not) be clean up by above statement
        res = traits_type::invalid(); // ensure to clean up
    }

    ~UniqueResource()
    {
        destroy();
    }

    UniqueResource& operator =(R&& res) noexcept
    {
        destroy();
        res_ = std::move(res);
        // res might (not) be clean up by above statement
        res = traits_type::invalid(); // ensure to clean up
        return *this;
    }

    UniqueResource& operator =(UniqueResource&& rhs) noexcept
    {
        if (this != &rhs)
        {
            attach(rhs.detach());
        }
        return *this;
    }

    const R& get() const noexcept { return res_; }

    operator const R&() const noexcept { return res_; }

    exception_ptr attach(R&& res) noexcept
    {
        auto ret = destroy();
        res_ = std::move(res);
        // res might (not) be clean up by above statement
        res = traits_type::invalid();
        return ret;
    }

    R detach() noexcept
    {
        auto res = res_;
        res_ = traits_type::invalid();
        return res;
    }

    exception_ptr destroy() noexcept
    {
        if (res_ == traits_type::invalid()) { return nullptr; }

        auto ret = traits_type::destroy(res_);
        res_ = traits_type::invalid();
        return ret;
    }

    void swap(UniqueResource& rhs) noexcept
    {
        using std;
        swap(res_, rhs.res_);
    }

    friend void swap(UniqueResource& lhs, UniqueResource& rhs) noexcept
    {
        lhs.swap(rhs);
    }

protected:
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator =(const UniqueResource&) = delete;

    R res_;
}; // UniqueResource

template <typename Traits>
void swap(UniqueResource<Traits>& lhs, UniqueResource<Traits>& rhs) noexcept
{
    lhs.swap(rhs);
} // swap

template <typename Traits>
bool operator ==(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() == rhs.get();
} // ==

template <typename Traits>
bool operator !=(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() != rhs.get();
} // !=

template <typename Traits>
bool operator <(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() < rhs.get();
} // <

template <typename Traits>
bool operator <=(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() <= rhs.get();
} // <=

template <typename Traits>
bool operator >(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() > rhs.get();
} // <

template <typename Traits>
bool operator >=(const UniqueResource<Traits>& lhs, const UniqueResource<Traits>& rhs) noexcept
{
    return lhs.get() >= rhs.get();
} // <
} // base
} // mstc
