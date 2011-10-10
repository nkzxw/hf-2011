#pragma once

#include <windows.h>

#define INJLIB_WAITTIMEOUT 30000 // INFINITE can cause DeadLock if host process is in debug mode

#ifdef UNICODE
#define InjectLib InjectLibW
#define EjectLib  EjectLibW
#else
#define InjectLib InjectLibA
#define EjectLib  EjectLibA
#endif   // !UNICODE

BOOL WINAPI InjectLibA (DWORD dwProcessID, LPCSTR lpszLibFile);
BOOL WINAPI InjectLibW (DWORD dwProcessID, LPCWSTR lpszLibFile);
BOOL WINAPI EjectLibA(DWORD dwProcessID, PCSTR pszLibFile);
BOOL WINAPI EjectLibW(DWORD dwProcessID, PCWSTR pszLibFile);