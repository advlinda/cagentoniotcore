#pragma once

#include "detail/CpuArchitech.h"
#include <windef.h>
#include <WinBase.h>
#include <assert.h>
#include <utility>
#include <stdexcept>
#include <detail/UniqueResource.h>

namespace mstc
{
namespace tckernel
{
class HandleTraits : public mstc::base::ResourceTraits<HANDLE>
{
public:
    // compile error - C3249 : illegal statement or sub-expression for 'constexpr'
    //  static constexpr HANDLE invalid() noexcept
    static const HANDLE invalid() noexcept
    {
        return nullptr;
    }
    static exception_ptr destroy(HANDLE h) noexcept
    {
        auto ret = ::CloseHandle(h);
        if (ret) { return nullptr; }

        try
        {
            throw std::runtime_error("close process error");
        }
        catch (...)
        {
            return std::current_exception();
        }
    }
}; // HandleTraits

class Handle : public mstc::base::UniqueResource<HandleTraits>
{
public:
    using base_type = mstc::base::UniqueResource<HandleTraits>;
    using base_type::base_type;
    using base_type::operator =;
}; // Handle
} // tckernel
} // mstc
