#if defined(WIN_IOT)

#include "detail/ToolHelp.h"
#include <Psapi.h>
#include <assert.h>
#include <stack>
#include "detail/Processes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
HANDLE
WINAPI
CreateToolhelp32Snapshot(
    DWORD dwFlags,
    DWORD th32ProcessID)
{
    if (dwFlags != TH32CS_SNAPPROCESS)
    {
        //assert(false);
        return nullptr;
    }

    try
    {
        return static_cast<HANDLE>(new SnapShot());
    }
    catch (...)
    {
        return nullptr;
    }
}

extern __declspec(dllexport)
BOOL
WINAPI
CloseToolhelp32Snapshot(
    HANDLE hSnapshot)
{
    if (!hSnapshot) { return FALSE; }

    delete static_cast<SnapShot*>(hSnapshot);
    return TRUE;
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32First(
    HANDLE hSnapshot,
    LPVOID lppe)
{
    if (!hSnapshot) { return FALSE; }

    return static_cast<SnapShot*>(hSnapshot)->process32First(*static_cast<PROCESSENTRY32*>(lppe)) != false;
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32Next(
    HANDLE hSnapshot,
    LPVOID lppe)
{
    if (!hSnapshot) { return FALSE; }

    return static_cast<SnapShot*>(hSnapshot)->process32Next(*static_cast<PROCESSENTRY32*>(lppe)) != false;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

void SnapShot::snapProcess()
{
    procIds_.resize(256);
    procNext_ = 0;
    auto proc_id_size = sizeof(proc_id_vec::value_type);

    for (; procIds_.size() <= 4096;)
    {
        DWORD bytesReturn = 0;
        DWORD bytesInput = proc_id_size * procIds_.size();
        ::K32EnumProcesses(procIds_.data(), bytesInput, &bytesReturn);
        if (bytesReturn < bytesInput) {
            procIds_.resize(bytesReturn / proc_id_size);
            convertIdsToHandles();
            return;
        }

        procIds_.resize(procIds_.size() * 2);
    }

    //assert(false);
}

void SnapShot::convertIdsToHandles()
{
    try
    {
        Processes::id2Handle(procIds_, hProcs_);
    }
    catch (...)
    {
    }

    auto size = hProcs_.size();
    procIds_.resize(size);

    // try to book all invalid hProc in reverse order!
    std::stack<decltype(size)> invalidProcs;

    for (decltype(size) i = 0; i < size; ++i)
    {
        if (hProcs_[i] == proc_handle::invalid()) { invalidProcs.push(i); }
    }

    // remove all invalid procId and hProc in reverse order!
    for (; !invalidProcs.empty(); invalidProcs.pop())
    {
        auto i = invalidProcs.top();
        procIds_.erase(procIds_.begin() + i);
        hProcs_.erase(hProcs_.begin() + i);
    }
}

bool SnapShot::process32First(PROCESSENTRY32& ppe)
{
    procNext_ = 0;
    return process32(ppe);
}

bool SnapShot::process32Next(PROCESSENTRY32& ppe)
{
    if (procNext_ >= procIds_.size())
    {
        ::SetLastError(ERROR_NO_MORE_FILES);
        return false;
    }

    ++procNext_;
    return process32(ppe);
}

bool SnapShot::process32(PROCESSENTRY32& ppe)
{
    if (ppe.dwSize < sizeof(ppe))
    {
        //assert(false);
        return false;
    }

    if (procNext_ >= procIds_.size())
    {
        ::SetLastError(ERROR_NO_MORE_FILES);
        return false;
    }

    auto procId = procIds_[procNext_];
    auto hProc = static_cast<HANDLE>(hProcs_[procNext_]);

    ppe.dwSize              = sizeof(ppe);
    ppe.cntUsage            = 0;
    ppe.th32ProcessID       = procId;
    ppe.th32DefaultHeapID   = 0;
    ppe.th32ModuleID        = 0;
    ppe.cntThreads          = 0;
    ppe.th32ParentProcessID = 0;
    ppe.pcPriClassBase      = 0;
    ppe.dwFlags             = 0;
    ppe.szExeFile[0]        = '\0';

    DWORD cb = _countof(ppe.szExeFile);
    if (!::K32GetModuleBaseNameA(hProc, nullptr, ppe.szExeFile, cb)) { return false; }

    return true;
}

#endif // WIN_IOT
