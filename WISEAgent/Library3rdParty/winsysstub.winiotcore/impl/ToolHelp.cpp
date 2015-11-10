#if defined(WIN_IOT)

#include "detail/ToolHelp.h"
#include <Psapi.h>
#include <assert.h>
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
        assert(false);
        return nullptr;
    }

    return static_cast<HANDLE>(new SnapShot());
}

extern __declspec(dllexport)
void
WINAPI
CloseToolhelp32Snapshot(
    HANDLE hSnapshot)
{
    delete static_cast<SnapShot*>(hSnapshot);
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32First(
    HANDLE hSnapshot,
    LPVOID lppe)
{
    return static_cast<SnapShot*>(hSnapshot)->process32First(*static_cast<PROCESSENTRY32*>(lppe)) != false;
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32Next(
    HANDLE hSnapshot,
    LPVOID lppe)
{
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

    for (; procIds_.size() < 65536;)
    {
        DWORD bytesReturn = 0;
        DWORD bytesInput = proc_id_size * procIds_.size();
        ::K32EnumProcesses(procIds_.data(), bytesInput, &bytesReturn);
        if (bytesReturn < bytesInput) {
            procIds_.resize(bytesReturn / proc_id_size);
            Processes::id2Handle(procIds_, hProcs_);
            return;
        }

        procIds_.resize(procIds_.size() * 2);
    }

    assert(false);
}

bool SnapShot::process32First(PROCESSENTRY32& ppe)
{
    procNext_ = 0;
    return process32(ppe);
}

bool SnapShot::process32Next(PROCESSENTRY32& ppe)
{
    ++procNext_;
    return process32(ppe);
}

bool SnapShot::process32(PROCESSENTRY32& ppe)
{
    if (ppe.dwSize < sizeof(ppe))
    {
        assert(false);
        return false;
    }

    if (procNext_ == procIds_.size())
    {
        ::SetLastError(ERROR_NO_MORE_FILES);
        return false;
    }

    if (procNext_ > procIds_.size())
    {
        assert(false);
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
