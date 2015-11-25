#pragma once

#include <utility>
#include <exception>

namespace mstc
{
namespace base
{
// Traits : resource traits require two functions
//          static R invalid()
//          static void destroy()
// R : resource type
template <typename Traits, typename R>
class UniqueResource
{
public:
    using exception_ptr = std::exception_ptr;

    UniqueResource(UniqueResource&& rhs) noexcept :
        res_{ rhs.detach() } {}

    explicit UniqueResource(R&& res = invalid()) noexcept :
        res_{ std::move(res) }
    {
        // res might (not) be clean up by above statement
        res = invalid(); // ensure to clean up
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
        res = invalid(); // ensure to clean up
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
        res = invalid();
        return ret;
    }

    R detach() noexcept
    {
        auto res = res_;
        res_ = invalid();
        return res;
    }

    exception_ptr destroy() noexcept
    {
        if (res_ == invalid()) { return nullptr; }

        auto ret = Traits::destroy(res_);
        res_ = invalid();
        return ret;
    }
	
    static const R invalid() noexcept
    {
        return Traits::invalid();
    }

	friend void swap(UniqueResource& lhs, UniqueResource& rhs) noexcept
	{
		std::swap(lhs, rhs);
	}

protected:
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator =(const UniqueResource&) = delete;

    R res_;
}; // UniqueResource
} // base
} // mstc
