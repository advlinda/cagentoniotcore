#if defined(WIN_IOT)

#include "detail/CpuArchitech.h"
#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
HANDLE
WINAPI
CreateNamedPipeA(
    _In_     LPCSTR lpName,
    _In_     DWORD dwOpenMode,
    _In_     DWORD dwPipeMode,
    _In_     DWORD nMaxInstances,
    _In_     DWORD nOutBufferSize,
    _In_     DWORD nInBufferSize,
    _In_     DWORD nDefaultTimeOut,
    _In_opt_ LPVOID lpSecurityAttributes)
{
    return reinterpret_cast<HANDLE>(0x1000);
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // WIN_IOT
