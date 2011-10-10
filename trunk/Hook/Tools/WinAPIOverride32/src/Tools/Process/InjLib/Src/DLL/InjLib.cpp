/******************************************************************************
Module:  InjLib.cpp
Notices: Copyright (c) 2000 Jeffrey Richter
******************************************************************************/

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include "injlib.h"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <malloc.h>        // For alloca
#include <TlHelp32.h>
#include <Winternl.h>


typedef NTSTATUS (WINAPI *LPFUN_NtCreateThreadEx)
    (
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN LPVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN BOOL CreateSuspended,
    IN ULONG StackZeroBits,
    IN ULONG SizeOfStackCommit,
    IN ULONG SizeOfStackReserve,
    OUT LPVOID lpBytesBuffer
    );

typedef struct _THREADEX_ID{
    DWORD ProcessId;
    DWORD ThreadId;
} THREADEX_ID, *PTHREADEX_ID;
typedef struct NtCreateThreadExBuffer
{
    ULONG Size;
    ULONG Unknown1;
    ULONG Unknown2;
    PTHREADEX_ID pThreadexId;
    ULONG Unknown4;
    ULONG Unknown5;
    ULONG Unknown6;
    PTEB* pPTEB;// pointer to PTEB (pointer of thread environment block)
    ULONG Unknown8;
}NTCREATETHREADEX_BUFFER;

extern "C" __declspec(dllexport) BOOL WINAPI InjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {

   BOOL fOk = FALSE; // Assume that the function fails
   HANDLE hProcess = NULL, hThread = NULL;
   PWSTR pszLibFileRemote = NULL;

   __try {
      // Get a handle for the target process.
      hProcess = OpenProcess(
         PROCESS_QUERY_INFORMATION |   // Required by Alpha
         PROCESS_CREATE_THREAD     |   // For CreateRemoteThread
         PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
         PROCESS_VM_WRITE,             // For WriteProcessMemory
         FALSE, dwProcessId);
      if (hProcess == NULL) __leave;

      // Calculate the number of bytes needed for the DLL's pathname
      int cch = 1 + lstrlenW(pszLibFile);
      int cb  = cch * sizeof(WCHAR);

      // Allocate space in the remote process for the pathname
      pszLibFileRemote = (PWSTR) 
         VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
      if (pszLibFileRemote == NULL) __leave;

      // Copy the DLL's pathname to the remote process's address space
      if (!WriteProcessMemory(hProcess, pszLibFileRemote, 
         (PVOID) pszLibFile, cb, NULL)) __leave;

      // Get the real address of LoadLibraryW in Kernel32.dll
      PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
      if (pfnThreadRtn == NULL) __leave;

      // Create a remote thread that calls LoadLibraryW(DLLPathname)
      hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
      if (hThread == NULL) // can occur for vista / seven cross session
      {
          HMODULE hModNtDll = ::GetModuleHandle(_T("ntdll.dll"));
          if (!hModNtDll)
              __leave;
          LPFUN_NtCreateThreadEx funNtCreateThreadEx = (LPFUN_NtCreateThreadEx) ::GetProcAddress(hModNtDll, "NtCreateThreadEx");
          if (!funNtCreateThreadEx)
              __leave;

          //setup and initialize the buffer
          NtCreateThreadExBuffer ntbuffer;

          memset (&ntbuffer,0,sizeof(NtCreateThreadExBuffer));
          PTEB pTEB = 0;
          THREADEX_ID ThreadExId = {0};

          ntbuffer.Size = sizeof(NtCreateThreadExBuffer);
          ntbuffer.Unknown1 = 0x10003;
          ntbuffer.Unknown2 = 0x8;
          ntbuffer.pThreadexId = &ThreadExId;
          ntbuffer.Unknown4 = 0;
          ntbuffer.Unknown5 = 0x10004;
          ntbuffer.Unknown6 = 4;
          ntbuffer.pPTEB = &pTEB;
          ntbuffer.Unknown8 = 0;

          // Finally execute remote thread 'pRemoteFunction' into remote process using NtCreateThreadEx function. 
          // Here one can use 'LoadLibrary' function address instead of 'pRemoteFunction' thread to implement 'DLL Injection' technique.
          NTSTATUS status = funNtCreateThreadEx(
              &hThread,
              0x1FFFFF,// full rights
              NULL,
              hProcess,
              (LPTHREAD_START_ROUTINE) pfnThreadRtn,
              pszLibFileRemote,
              FALSE, //start instantly
              NULL,
              NULL,
              NULL,
              &ntbuffer
              );

      }
      if (hThread == NULL)
          __leave;

      // Wait for the remote thread to terminate
      if (WaitForSingleObject(hThread, INJLIB_WAITTIMEOUT)!=WAIT_OBJECT_0)
        fOk=FALSE;
      else
        fOk = TRUE; // Everything executed successfully
   }
   __finally { // Now, we can clean everthing up

      // Free the remote memory that contained the DLL's pathname
      if (pszLibFileRemote != NULL) 
         VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

      if (hThread  != NULL) 
         CloseHandle(hThread);

      if (hProcess != NULL) 
         CloseHandle(hProcess);
   }

   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


extern "C" __declspec(dllexport) BOOL WINAPI InjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {

   // Allocate a (stack) buffer for the Unicode version of the pathname
   PWSTR pszLibFileW = (PWSTR) 
      _alloca((lstrlenA(pszLibFile) + 1) * sizeof(WCHAR));

   // Convert the ANSI pathname to its Unicode equivalent
   wsprintfW(pszLibFileW, L"%S", pszLibFile);

   // Call the Unicode version of the function to actually do the work.
   return(InjectLibW(dwProcessId, pszLibFileW));
}


///////////////////////////////////////////////////////////////////////////////


extern "C" __declspec(dllexport) BOOL WINAPI EjectLibW(DWORD dwProcessId, PCWSTR pszLibFile) {

   BOOL fOk = FALSE; // Assume that the function fails
   HANDLE hthSnapshot = NULL;
   HANDLE hProcess = NULL, hThread = NULL;

   __try {
      // Grab a new snapshot of the process
      hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
      if (hthSnapshot == NULL) __leave;

      // Get the HMODULE of the desired library
      MODULEENTRY32W me = { sizeof(me) };
      BOOL fFound = FALSE;
      BOOL fMoreMods = Module32FirstW(hthSnapshot, &me);
      for (; fMoreMods; fMoreMods = Module32NextW(hthSnapshot, &me)) {
         fFound = (lstrcmpiW(me.szModule,  pszLibFile) == 0) || 
                  (lstrcmpiW(me.szExePath, pszLibFile) == 0);
         if (fFound) break;
      }
      if (!fFound) __leave;

      // Get a handle for the target process.
      hProcess = OpenProcess(
         PROCESS_QUERY_INFORMATION |   // Required by Alpha
         PROCESS_CREATE_THREAD     | 
         PROCESS_VM_OPERATION,  // For CreateRemoteThread
         FALSE, dwProcessId);
      if (hProcess == NULL) __leave;

      // Get the real address of LoadLibraryW in Kernel32.dll
      PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "FreeLibrary");
      if (pfnThreadRtn == NULL) __leave;

      // Create a remote thread that calls FreeLibrary(hModule)
      hThread = CreateRemoteThread(hProcess, NULL, 0, 
         pfnThreadRtn, me.modBaseAddr, 0, NULL);

      if (hThread == NULL) // can occur for vista / seven cross session
      {
          HMODULE hModNtDll = ::GetModuleHandle(_T("ntdll.dll"));
          if (!hModNtDll)
              __leave;
          LPFUN_NtCreateThreadEx funNtCreateThreadEx = (LPFUN_NtCreateThreadEx) ::GetProcAddress(hModNtDll, "NtCreateThreadEx");
          if (!funNtCreateThreadEx)
              __leave;

          //setup and initialize the buffer
          NtCreateThreadExBuffer ntbuffer;

          memset (&ntbuffer,0,sizeof(NtCreateThreadExBuffer));
          PTEB pTEB = 0;
          THREADEX_ID ThreadexId = {0};

          ntbuffer.Size = sizeof(NtCreateThreadExBuffer);
          ntbuffer.Unknown1 = 0x10003;
          ntbuffer.Unknown2 = 0x8;
          ntbuffer.pThreadexId = &ThreadexId;
          ntbuffer.Unknown4 = 0;
          ntbuffer.Unknown5 = 0x10004;
          ntbuffer.Unknown6 = 4;
          ntbuffer.pPTEB = &pTEB;
          ntbuffer.Unknown8 = 0;

          // Finally execute remote thread 'pRemoteFunction' into remote process using NtCreateThreadEx function. 
          // Here one can use 'LoadLibrary' function address instead of 'pRemoteFunction' thread to implement 'DLL Injection' technique.
          NTSTATUS status = funNtCreateThreadEx(
              &hThread,
              0x1FFFFF,// full rights
              NULL,
              hProcess,
              (LPTHREAD_START_ROUTINE) pfnThreadRtn,
              me.modBaseAddr,
              FALSE, //start instantly
              NULL,
              NULL,
              NULL,
              &ntbuffer
              );

      }

      if (hThread == NULL) __leave;

      // Wait for the remote thread to terminate
      if (WaitForSingleObject(hThread, INJLIB_WAITTIMEOUT)!=WAIT_OBJECT_0)
        fOk=FALSE;
      else
        fOk = TRUE; // Everything executed successfully
   }
   __finally { // Now we can clean everything up

      if (hthSnapshot != NULL) 
         CloseHandle(hthSnapshot);

      if (hThread     != NULL) 
         CloseHandle(hThread);

      if (hProcess    != NULL) 
         CloseHandle(hProcess);
   }

   return(fOk);
}


///////////////////////////////////////////////////////////////////////////////


extern "C" __declspec(dllexport) BOOL WINAPI EjectLibA(DWORD dwProcessId, PCSTR pszLibFile) {

   // Allocate a (stack) buffer for the Unicode version of the pathname
   PWSTR pszLibFileW = (PWSTR) 
      _alloca((lstrlenA(pszLibFile) + 1) * sizeof(WCHAR));

   // Convert the ANSI pathname to its Unicode equivalent
   wsprintfW(pszLibFileW, L"%S", pszLibFile);

   // Call the Unicode version of the function to actually do the work.
   return(EjectLibW(dwProcessId, pszLibFileW));
}




//////////////////////////////// End of File //////////////////////////////////
