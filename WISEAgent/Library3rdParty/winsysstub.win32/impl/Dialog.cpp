#if defined(WIN_IOT)

#include "detail/CpuArchitech.h"
#include <windef.h>
#include <iostream>

#ifndef MB_OK
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#define MB_CANCELTRYCONTINUE        0x00000006L
#endif // MB_OK

#ifndef MB_TYPEMASK
#define MB_TYPEMASK                 0x0000000FL
#define MB_ICONMASK                 0x000000F0L
#define MB_DEFMASK                  0x00000F00L
#define MB_MODEMASK                 0x00003000L
#define MB_MISCMASK                 0x0000C000L
#endif // MB_TYPEMASK

#ifndef IDOK
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
#define IDCLOSE             8
#define IDHELP              9
#define IDTRYAGAIN          10
#define IDCONTINUE          11
#endif // IDOK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern __declspec(dllexport)
int
WINAPI
MessageBoxA(
    _In_opt_ HWND hWnd,
    _In_opt_ LPCSTR lpText,
    _In_opt_ LPCSTR lpCaption,
    _In_ UINT uType)
{
    auto choice = uType & MB_TYPEMASK;
    if (choice == MB_OK) { return IDOK; }

    auto oldFlags = std::cout.flags();

    std::cout << "Problematic MessageBoxA(," << lpText << ", " << lpCaption <<
        ", 0x" << std::hex << std::uppercase << uType << ")" << std::endl;

    switch (choice)
    {
    case MB_OKCANCEL:
        return IDCANCEL; // ??
        break;
    case MB_ABORTRETRYIGNORE:
        return IDABORT; // ??
        break;
    case MB_YESNOCANCEL:
        return IDCANCEL; // ??
        break;
    case MB_YESNO:
        return IDNO; // ??
        break;
    case MB_RETRYCANCEL:
        return IDCANCEL; // ??
        break;
    case MB_CANCELTRYCONTINUE:
        return IDCANCEL; // ??
        break;
    default:
        std::cout << "Problematic choice:0x" << std::hex <<
            std::uppercase << choice << std::endl;
        return IDCANCEL; // ??
        break;
    }
	
    std::cout.flags(oldFlags);
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // WIN_IOT
