#pragma once

#include "detail/CpuArchitech.h"
#include <windef.h>
#include <WinBase.h>
#include <vector>
#include "detail/Handle.h"

// from TlHelp32.h
#define TH32CS_SNAPHEAPLIST 0x00000001
#define TH32CS_SNAPPROCESS  0x00000002
#define TH32CS_SNAPTHREAD   0x00000004
#define TH32CS_SNAPMODULE   0x00000008
#define TH32CS_SNAPMODULE32 0x00000010
#define TH32CS_SNAPALL      (TH32CS_SNAPHEAPLIST | TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE)
#define TH32CS_INHERIT      0x80000000

typedef struct tagPROCESSENTRY32
{
    DWORD     dwSize;
    DWORD     cntUsage;
    DWORD     th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD     th32ModuleID;           // associated exe
    DWORD     cntThreads;
    DWORD     th32ParentProcessID;    // this process's parent process
    LONG      pcPriClassBase;         // Base priority of process's threads
    DWORD     dwFlags;
    CHAR      szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOOL WINAPI K32EnumProcesses(DWORD* procIds, DWORD cb, DWORD* bytesReturned);

DWORD WINAPI K32GetModuleBaseNameA(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

class SnapShot
{
public:
    explicit SnapShot()
    {
        snapProcess();
    }

    void snapProcess();

    bool process32First(PROCESSENTRY32&);

    bool process32Next(PROCESSENTRY32&);

private:
    using proc_id         = DWORD;
    using proc_id_vec     = std::vector<proc_id>;
    using proc_handle     = mstc::tckernel::Handle;
    using proc_handle_vec = std::vector<proc_handle>;

    void convertIdsToHandles();
    bool process32(PROCESSENTRY32&);

    proc_id_vec     procIds_;
    proc_handle_vec hProcs_;
    size_t          procNext_ = 0;
};
