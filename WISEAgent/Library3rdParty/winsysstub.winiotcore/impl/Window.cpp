#if defined(WIN_IOT)

#include "detail/CpuArchitech.h"
#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
BOOL
WINAPI
IsWindowVisible(
    _In_ HWND hWnd)
{
    return FALSE;
}

extern __declspec(dllexport)
DWORD
WINAPI
GetWindowThreadProcessId(
    _In_ HWND hWnd,
    _Out_opt_ LPDWORD lpdwProcessId)
{
    if (lpdwProcessId) { *lpdwProcessId = 0x3100; }

    return 0x3000;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // WIN_IOT
