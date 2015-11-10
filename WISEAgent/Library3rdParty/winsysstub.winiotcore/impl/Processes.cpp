#include "detail/pch.h"
#include <Windows.h>
#include "detail/Processes.h"
#include "detail/Token.h"
#include "detail/Scope.h"

void
Processes::id2Handle(
    const proc_id_vec& procIds, proc_handle_vec& hProcs)
{
    hProcs.resize(0);

    HANDLE procToken = nullptr;
    auto ret = ::OpenProcessToken(::GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &procToken);
    mstc::tckernel::Token token(std::move(procToken));
    if (!ret) { return; }

    bool enableOld = true;
    token.adjustPrivilegeA("SeDebugPrivilege", true, enableOld);

    // adjust privilege back
    auto adjustBack = mstc::base::scope([&enableOld, &token]()
    {
        if (enableOld) { return; }
        token.adjustPrivilegeA("SeDebugPrivilege", false, enableOld);
    });

    for (auto& procId : procIds)
    {
        auto hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
        hProcs.push_back(mstc::tckernel::Handle(std::move(hProc)));
    }
}
