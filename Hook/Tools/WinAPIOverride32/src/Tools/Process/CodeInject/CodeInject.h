/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



This Class has been written thanks to the very good
WinSpy Project Copyright (c) by 2003 Robert Kuster
*/



//////////// WARNING //////////////////////////////
// Code injection will only work in release Mode
//////////// WARNING /////////////////////////////
#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../ModulesParser/ModulesParser.h"
#include "../../String/AnsiUnicodeConvert.h"


class CCodeInject
{
private:
    typedef HMODULE (WINAPI *pfGetModuleHandle)(TCHAR* lpModuleName);
    typedef HMODULE (WINAPI *pfLoadLibrary)(TCHAR* lpModuleName);
    typedef FARPROC (WINAPI *pfGetProcAddress)(HMODULE hModule,CHAR* lpProcName);
    typedef BOOL (WINAPI *pfFreeLibrary)(HMODULE hModule);

    typedef struct tagGETMODULEHANDLE_PARAM
    {
        pfGetModuleHandle   pGetModuleHandle;
        pfLoadLibrary       pLoadLibrary;
        TCHAR               pszDllName[MAX_PATH];// dll name to retreive hmodule
        HMODULE             RemotehModule;// return value of get modulehandle or loadlibrary
        BOOL                bFreeLibraryShoudBeCall;// TRUE if loadlibrary as been called
    }GETMODULEHANDLE_PARAM,*PGETMODULEHANDLE_PARAM;

    typedef struct tagGETPROCADDRESS_PARAM
    {
        pfGetProcAddress    pGetProcAddress;
        HMODULE             hModule;// dll hmodule
        CHAR                pszProcName[MAX_PATH];// function name /!\ GetProcAddress arg is CHAR not TCHAR
        FARPROC             RemotepFunction;// return value of GetProcAddress
    }GETPROCADDRESS_PARAM,*PGETPROCADDRESS_PARAM;

    typedef struct tagFREELIBRARY_PARAM
    {
        pfFreeLibrary       pFreeLibrary;
        HMODULE             hModule;
    }FREELIBRARY_PARAM,*PFREELIBRARY_PARAM;


    DWORD dwPageSize;

    LPVOID       pDataRemote;    // the address (in the remote process) of param data;
    FARPROC      pCodeRemote;    // the address (in the remote process) where ThreadFunc will be copied to;
    HANDLE       hProcess;
	HMODULE      hRemoteKernel32;

    LPVOID       pDataRemoteBackup;
    FARPROC      pCodeRemoteBackup;

    pfLoadLibrary      pLoadLibrary;
    pfGetModuleHandle  pGetModuleHandle;
    pfGetProcAddress   pGetProcAddress;
    pfFreeLibrary      pFreeLibrary;

    void SaveRemotePointers();
    void RestoreRemotePointers();
    void FreeRemoteAllocatedMemory();

    static DWORD WINAPI RemoteGetModuleHandleThreadProc (CCodeInject::PGETMODULEHANDLE_PARAM lpParameter);
    static DWORD WINAPI RemoteFreeLibraryThreadProc (CCodeInject::PFREELIBRARY_PARAM lpParameter);
    static DWORD WINAPI RemoteGetProcAddressThreadProc (CCodeInject::PGETPROCADDRESS_PARAM lpParameter);

	static DWORD WINAPI RemoteGetModuleHandleAfterThreadProc ();
	static DWORD WINAPI RemoteFreeLibraryAfterThreadProc ();
	static DWORD WINAPI RemoteGetProcAddressAfterThreadProc ();

	static BOOL RemoteModuleFoundCallBack(MODULEENTRY* pModuleEntry,PVOID UserParam);
public:
    CCodeInject(DWORD dwProcessID);
    ~CCodeInject(void);
    BOOL SetParameter(PVOID Param,SIZE_T ParamSize);
    BOOL GetParameter(PVOID Param,SIZE_T ParamSize);
    BOOL SetCode(FARPROC ThreadProcStartAdress,SIZE_T ThreadProcCodeSize);
    BOOL Execute();
    BOOL Execute(LPDWORD lpExitCode);
    BOOL Execute(FARPROC RemoteProcFuncAddress,LPDWORD lpExitCode);

    BOOL RemoteGetModuleHandle(TCHAR* pszDllName,HMODULE* phRemoteModule,BOOL* pbFreeLibraryShoudBeCall);
    BOOL RemoteGetProcAddress(HMODULE hRemoteModule,TCHAR* pszProcName,FARPROC* ppFunc);
    BOOL RemoteFreeLibrary(HMODULE hRemoteModule);
};
