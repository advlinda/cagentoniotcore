#if defined(WIN_IOT)

#include "detail/CpuArchitech.h"
#include <windef.h>
#include <WinBase.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
HANDLE
WINAPI
CreateToolhelp32Snapshot(
    DWORD dwFlags,
    DWORD th32ProcessID
    )
{
    return reinterpret_cast<HANDLE>(0x5000);
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32First(
    HANDLE hSnapshot,
    LPVOID lppe
    )
{
    ::SetLastError(ERROR_NO_MORE_FILES);

    return FALSE;
}

extern __declspec(dllexport)
BOOL
WINAPI
Process32Next(
    HANDLE hSnapshot,
    LPVOID lppe
    )
{
    ::SetLastError(ERROR_NO_MORE_FILES);

    return FALSE;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // WIN_IOT
