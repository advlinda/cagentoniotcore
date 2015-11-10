#pragma once

#include "detail/Handle.h"

namespace mstc
{
namespace tckernel
{
class Token : public Handle
{
public:
    using base_type = Handle;
    using base_type::base_type;
    using base_type::operator =;

    void adjustPrivilegeA(const char* privilege, bool enableNew, bool& enableOld);
}; // Token
} // tckernel
} // mstc
