// include for DLL import

#pragma once
#include <windows.h>

// exported func name
#define INJECTLIBA_FUNC_NAME "_InjectLibA@8"
#define EJECTLIBA_FUNC_NAME "_EjectLibA@8"
#define INJECTLIBW_FUNC_NAME "_InjectLibW@8"
#define EJECTLIBW_FUNC_NAME "_EjectLibW@8"


typedef BOOL (__stdcall *InjectLibA) (DWORD dwProcessID, LPCSTR lpszLibFile);
typedef BOOL (__stdcall *InjectLibW) (DWORD dwProcessID, LPCWSTR lpszLibFile);
typedef BOOL (__stdcall *EjectLibA)  (DWORD dwProcessID, PCSTR pszLibFile);
typedef BOOL (__stdcall *EjectLibW)  (DWORD dwProcessID, PCWSTR pszLibFile);

#if (defined(UNICODE)||defined(_UNICODE))
    #define INJECTLIB_FUNC_NAME     INJECTLIBW_FUNC_NAME
    #define EJECTLIB_FUNC_NAME      EJECTLIBW_FUNC_NAME
    #define InjectLib               InjectLibW
    #define EjectLib                EjectLibW
#else
    #define INJECTLIB_FUNC_NAME     INJECTLIBA_FUNC_NAME
    #define EJECTLIB_FUNC_NAME      EJECTLIBA_FUNC_NAME
    #define InjectLib               InjectLibA
    #define EjectLib                EjectLibA
#endif
