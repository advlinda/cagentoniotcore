#include "detail/pch.h"
#include <Windows.h>
#include "detail/Token.h"

void
mstc::tckernel::Token::adjustPrivilegeA(
    const char* privilege, bool enableNew, bool& enableOld)
{
    TOKEN_PRIVILEGES tp = { 1 };

    auto ret = ::LookupPrivilegeValueA(nullptr, privilege, &tp.Privileges[0].Luid);
    if (!ret)
    {
        throw std::runtime_error("lookup privilege value error");
    }

    if (enableNew) { tp.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED; }

    TOKEN_PRIVILEGES tpOld = { 1 };
    DWORD            cbOld = sizeof(tpOld);

    ret = ::AdjustTokenPrivileges(res_, FALSE, &tp, sizeof(tp), &tpOld, &cbOld);
    if (!ret)
    {
        throw std::runtime_error("adjust token privilege error");
    }

    enableOld = (tpOld.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) ? true : false;
}
