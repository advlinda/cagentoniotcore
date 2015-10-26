#if defined(WIN_IOT)

#include "detail/CpuArchitech.h"
#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
HANDLE
WINAPI
RegisterEventSourceA(
    _In_opt_ LPCSTR lpUNCServerName,
    _In_     LPCSTR lpSourceName)
{
    return reinterpret_cast<HANDLE>(0x4000);
}

extern __declspec(dllexport)
BOOL
WINAPI
ReportEventA(
    _In_     HANDLE     hEventLog,
    _In_     WORD       wType,
    _In_     WORD       wCategory,
    _In_     DWORD      dwEventID,
    _In_opt_ PSID       lpUserSid,
    _In_     WORD       wNumStrings,
    _In_     DWORD      dwDataSize,
    _In_reads_opt_(wNumStrings) LPCSTR *lpStrings,
    _In_reads_bytes_opt_(dwDataSize) LPVOID lpRawData)
{
    return TRUE;
}

extern __declspec(dllexport)
BOOL
WINAPI
DeregisterEventSource(
    _In_ HANDLE hEventLog)
{
    return TRUE;
}

extern __declspec(dllexport)
HANDLE
WINAPI
OpenEventLogA(
    _In_opt_ LPCSTR lpUNCServerName,
    _In_     LPCSTR lpSourceName
    )
{
    return reinterpret_cast<HANDLE>(0x5100);
}

extern __declspec(dllexport)
BOOL
WINAPI
CloseEventLog(
    _In_ HANDLE hEventLog
    )
{
    return TRUE;
}

extern __declspec(dllexport)
BOOL
WINAPI
GetNumberOfEventLogRecords(
    _In_  HANDLE hEventLog,
    _Out_ PDWORD NumberOfRecords
    )
{
    if (NumberOfRecords) { *NumberOfRecords = 0; }

    return TRUE;
}

extern __declspec(dllexport)
BOOL
WINAPI
GetOldestEventLogRecord(
    _In_  HANDLE hEventLog,
    _Out_ PDWORD OldestRecord
    )
{
    if (OldestRecord) { *OldestRecord = 0; }

    return TRUE;
}

extern __declspec(dllexport)
BOOL
WINAPI
ReadEventLogA(
    _In_  HANDLE     hEventLog,
    _In_  DWORD      dwReadFlags,
    _In_  DWORD      dwRecordOffset,
    _Out_writes_bytes_to_(nNumberOfBytesToRead, *pnBytesRead) LPVOID     lpBuffer,
    _In_  DWORD      nNumberOfBytesToRead,
    _Out_ DWORD      *pnBytesRead,
    _Out_ DWORD      *pnMinNumberOfBytesNeeded
    )
{
    if (pnBytesRead) { *pnBytesRead = 0; }
    if (pnMinNumberOfBytesNeeded) { *pnMinNumberOfBytesNeeded= 0; }

    return TRUE;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // WIN_IOT