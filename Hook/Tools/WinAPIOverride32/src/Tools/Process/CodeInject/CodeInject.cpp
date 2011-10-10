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

#include "codeinject.h"

#ifdef _DEBUG
#pragma message (__FILE__ " ")
#pragma message (__FILE__ " ************************************************************")
#pragma message (__FILE__ "    WARNING CODE INJECTION WILL WORK ONLY IN RELEASE MODE")
#pragma message (__FILE__ " ************************************************************")
#pragma message (__FILE__ " ")
#endif

//-----------------------------------------------------------------------------
// Name: RemoteGetModuleHandleThreadProc
// Object: Code injected in remote process to get a module handle
// Parameters :
//     in/out  : CCodeInject::PGETMODULEHANDLE_PARAM lpParameter
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CCodeInject::RemoteGetModuleHandleThreadProc (CCodeInject::PGETMODULEHANDLE_PARAM lpParameter)
{
    lpParameter->bFreeLibraryShoudBeCall=FALSE;
    // try get module handle
    lpParameter->RemotehModule=lpParameter->pGetModuleHandle(lpParameter->pszDllName);
    if (lpParameter->RemotehModule)
        return 0;
    // if library not loaded, just load it
    lpParameter->bFreeLibraryShoudBeCall=TRUE;
    lpParameter->RemotehModule=lpParameter->pLoadLibrary(lpParameter->pszDllName);
 	return 0;
}
DWORD WINAPI CCodeInject::RemoteGetModuleHandleAfterThreadProc (void) {return 0;}

//-----------------------------------------------------------------------------
// Name: RemoteFreeLibraryThreadProc
// Object: Code injected in remote process free a library
// Parameters :
//     in     : CCodeInject::PFREELIBRARY_PARAM lpParameter
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CCodeInject::RemoteFreeLibraryThreadProc (CCodeInject::PFREELIBRARY_PARAM lpParameter)
{
    return lpParameter->pFreeLibrary(lpParameter->hModule);
}
DWORD WINAPI CCodeInject::RemoteFreeLibraryAfterThreadProc (void) {return 0;}

//-----------------------------------------------------------------------------
// Name: RemoteGetProcAddressThreadProc
// Object: Code injected in remote process to get a proc address
// Parameters :
//     in/out  : CCodeInject::PGETPROCADDRESS_PARAM lpParameter
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CCodeInject::RemoteGetProcAddressThreadProc (CCodeInject::PGETPROCADDRESS_PARAM lpParameter)
{
    lpParameter->RemotepFunction=lpParameter->pGetProcAddress(lpParameter->hModule,lpParameter->pszProcName);
    return 0;
}
DWORD WINAPI CCodeInject::RemoteGetProcAddressAfterThreadProc (void) {return 0;}


BOOL CCodeInject::RemoteModuleFoundCallBack(MODULEENTRY* pModuleEntry,PVOID UserParam)
{
	CCodeInject* pCodeInject =(CCodeInject*) UserParam;
	if (_tcscmp(pModuleEntry->szModule,_T("kernel32.dll"))==0)
	{
		pCodeInject->hRemoteKernel32 = pModuleEntry->hModule;
		return FALSE; // stop parsing
	}
	return TRUE; // continue parsing
}

//-----------------------------------------------------------------------------
// Name: CCodeInject
// Object: constructor
// Parameters :
//     in  : HANDLE hProcess : Handle to a process open with PROCESS_CREATE_THREAD |PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE flags
//                                you can add the PROCESS_VM_READ to retrive data information
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CCodeInject::CCodeInject(DWORD dwProcessID)
{

	this->hProcess = OpenProcess(
									PROCESS_CREATE_THREAD     |   // Required for thread creation
									PROCESS_QUERY_INFORMATION |   // Required by Alpha
									PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
									PROCESS_VM_WRITE          |   // For WriteProcessMemory
									PROCESS_VM_READ,              // For ReadProcessMemory
									FALSE, 
									dwProcessID
									);

	CModulesParser::Parse(dwProcessID,CCodeInject::RemoteModuleFoundCallBack,this);

    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo); 
    this->dwPageSize=siSysInfo.dwPageSize;

    this->pCodeRemote=NULL;
    this->pDataRemote=NULL;
    this->pDataRemoteBackup=NULL;
    this->pCodeRemoteBackup=NULL;

	// do ASLR bypassing
    HMODULE hkernel32=GetModuleHandle(_T("kernel32.dll"));

    // load libraries
    this->pLoadLibrary=(pfLoadLibrary)GetProcAddress(hkernel32,
#if (defined(UNICODE)||defined(_UNICODE))
                        "LoadLibraryW"
#else
                        "LoadLibraryA"
#endif
                        );
	// adjust remote function address according to remote module address
	this->pLoadLibrary =(pfLoadLibrary)( ((SIZE_T)this->pLoadLibrary - (SIZE_T)hkernel32) // RVA
										+ (SIZE_T)this->hRemoteKernel32 // remote base
										);

    this->pGetModuleHandle=(pfGetModuleHandle)GetProcAddress(hkernel32, 
#if (defined(UNICODE)||defined(_UNICODE))
                            "GetModuleHandleW"
#else
                            "GetModuleHandleA"
#endif
                            );
	// adjust remote function address according to remote module address
	this->pGetModuleHandle =(pfGetModuleHandle)( ((SIZE_T)this->pGetModuleHandle - (SIZE_T)hkernel32) // RVA
												+ (SIZE_T)this->hRemoteKernel32 // remote base
												);

    this->pGetProcAddress=(pfGetProcAddress)GetProcAddress(hkernel32, "GetProcAddress");
	// adjust remote function address according to remote module address
	this->pGetProcAddress =(pfGetProcAddress)( ((SIZE_T)this->pGetProcAddress - (SIZE_T)hkernel32) // RVA
											+ (SIZE_T)this->hRemoteKernel32 // remote base
											);


    this->pFreeLibrary=(pfFreeLibrary)GetProcAddress(hkernel32, "FreeLibrary");
	// adjust remote function address according to remote module address
	this->pFreeLibrary =(pfFreeLibrary)( ((SIZE_T)this->pFreeLibrary - (SIZE_T)hkernel32) // RVA
										+ (SIZE_T)this->hRemoteKernel32 // remote base
										);
}

CCodeInject::~CCodeInject(void)
{
    this->FreeRemoteAllocatedMemory();
	if (this->hProcess)
		::CloseHandle(this->hProcess);
}

//-----------------------------------------------------------------------------
// Name: FreeRemoteAllocatedMemory
// Object: Free remote process allocated memory of members pDataRemote and pCodeRemote
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CCodeInject::FreeRemoteAllocatedMemory()
{
    if (this->pDataRemote)
        VirtualFreeEx( this->hProcess, this->pDataRemote, 0, MEM_RELEASE );

    if (this->pCodeRemote)
        VirtualFreeEx( this->hProcess, this->pCodeRemote, 0, MEM_RELEASE );
}

//-----------------------------------------------------------------------------
// Name: SetParameter
// Object: Inject parameters data into remote process
// Parameters :
//     in  : PVOID Param : param to the ThreadProc
//           SIZE_T ParamSize : param size
//     out : 
//     return : FALSE on error, TRUE on success
//-----------------------------------------------------------------------------
BOOL CCodeInject::SetParameter(PVOID Param,SIZE_T ParamSize)
{
    SIZE_T NumBytesXferred = 0; // number of bytes written/read to/from the remote process;

    if (IsBadReadPtr(Param,ParamSize))
        return FALSE;

    if (this->pDataRemote)
    {
        VirtualFreeEx( this->hProcess, this->pDataRemote, 0, MEM_RELEASE );
        this->pDataRemote=NULL;
    }

    // 1. Allocate memory in the remote process for Param
    // 2. Write a copy of DataLocal to the allocated memory
    this->pDataRemote = VirtualAllocEx( this->hProcess, 0, ParamSize, MEM_COMMIT, PAGE_READWRITE );
    if (this->pDataRemote==NULL)
        return FALSE;

    if (!WriteProcessMemory( this->hProcess, this->pDataRemote, Param, ParamSize, &NumBytesXferred ))
    {
        VirtualFreeEx( this->hProcess, this->pDataRemote, 0, MEM_RELEASE );
        this->pDataRemote=NULL;
        return FALSE;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetParameter
// Object: read parameters data into remote process
//         Use it to retrieve data after a Remote function call
// Parameters :
//     in  : SIZE_T ParamSize : param size
//     out : PVOID Param : param transmitted to the ThreadProc
//     return : FALSE on error, TRUE on success
//-----------------------------------------------------------------------------
BOOL CCodeInject::GetParameter(PVOID Param,SIZE_T ParamSize)
{
    SIZE_T nbBytesReads=0;
    if (IsBadWritePtr(Param,ParamSize))
        return FALSE;
    return ReadProcessMemory(this->hProcess,this->pDataRemote,Param,ParamSize,&nbBytesReads);
}

//-----------------------------------------------------------------------------
// Name: SetCode
// Object: Inject code into remote process
//         You don't need to use it if you want to call an api or an already loaded code, just 
//         use execute(apiAddress,lpExitCode) instead
// Parameters :
//     in  : FARPROC ThreadProcStartAdress : ThreadProc
//           DWORD ThreadProcCodeSize = ((LPBYTE) AfterThreadProc - (LPBYTE) ThreadProc);
//
//                                      where ThreadProc and AfterThreadProc are like the following
//                        typedef LRESULT     (WINAPI *pfSendMessage)(HWND,UINT,WPARAM,LPARAM);
//                        typedef struct
//                        {
//                            // params we want to transmit
//                            HWND    hwnd;
//
//                            // func pointer
//                            FARPROC	pfunc;
//                        }INJDATA,*PINJDATA;
//                        //static DWORD WINAPI ShowPasswordContentThreadProc (LPVOID lpParameter)
//                        static DWORD WINAPI ShowPasswordContentThreadProc (PINJDATA lpParameter)
//                        {
//                            ((pfSendMessage)(lpParameter->pfunc))(lpParameter->hwnd,(UINT) EM_SETPASSWORDCHAR,'x',0);
// 	                          return 0;
//                        }
//                        // This function marks the memory address after ThreadProc.
//                        static DWORD WINAPI  ShowPasswordContentAfterThreadProc (void) {return 0;}
//
//     out : 
//     return : FALSE on error, TRUE on success
//-----------------------------------------------------------------------------
BOOL CCodeInject::SetCode(FARPROC ThreadProcStartAdress,SIZE_T ThreadProcCodeSize)
{
    // check ThreadProcCodeSize (with some compiler options, memory flags don't
    // work anymore so check it)
    if (ThreadProcCodeSize>this->dwPageSize)
        // the following is not disturbing as at least a memory page is allocated by a VirtualAllocEx call
        // (of course only if code to inject is less than a memory page)
        ThreadProcCodeSize=this->dwPageSize;

    SIZE_T NumBytesXferred = 0; // number of bytes written/read to/from the remote process;

    if (IsBadReadPtr(ThreadProcStartAdress,ThreadProcCodeSize))
        return FALSE;

    if (this->pCodeRemote)
    {
        VirtualFreeEx( this->hProcess, this->pCodeRemote, 0, MEM_RELEASE );
        this->pCodeRemote=NULL;
    }

    // 1. Allocate memory in the remote process for the injected ThreadProc
    // 2. Write a copy of ThreadProc to the allocated memory
    this->pCodeRemote = (FARPROC) VirtualAllocEx( this->hProcess, 0, ThreadProcCodeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE );        
    if (this->pCodeRemote==NULL)
        return FALSE;

    if (!WriteProcessMemory( this->hProcess, this->pCodeRemote, ThreadProcStartAdress, ThreadProcCodeSize, &NumBytesXferred ))
    {
        VirtualFreeEx( this->hProcess, this->pCodeRemote, 0, MEM_RELEASE );
        this->pCodeRemote=NULL;
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Object: execute TreadProc in remote process (blocking call : function will not return until thread as finished)
// Parameters :
//     in  : 
//     out : 
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::Execute()
{
    DWORD dwExitCode=0;
    return this->Execute(&dwExitCode);
}
//-----------------------------------------------------------------------------
// Name: Execute
// Object: execute thread proc in remote process (blocking call : function will not return until thread as finished)
// Parameters :
//     in  : 
//     out : LPDWORD lpExitCode : exit code of TreadProc
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::Execute(LPDWORD lpExitCode)
{
    // check if InjectCode was successfully called before
    if (!this->pCodeRemote)
        return FALSE;
    return this->Execute(this->pCodeRemote,lpExitCode);
}
//-----------------------------------------------------------------------------
// Name: Execute
// Object: execute RemoteProcFuncAddress proc in remote process (blocking call : function will not return until thread as finished)
//         Use this function if you don't have injected code (func already exist in remote process)
//         WARNING A DIRECT CALL WILL WORK ONLY FOR FUNC HAVING 1 PARAMETER AND IF THIS PARAMETER IS A POINTER
//         else you have to do code injection like done in RemoteGetProcAddress, RemoteGetModuleHandleThreadProc
//         RemoteFreeLibrary
// Parameters :
//     in  : FARPROC RemoteProcFunc : remote address of the proc func to execute
//     out : LPDWORD lpExitCode : exit code of TreadProc
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::Execute(FARPROC RemoteProcFuncAddress,LPDWORD lpExitCode)
{
    HANDLE       hThread = NULL; // the handle to the thread executing the remote copy of ThreadProc;
    DWORD        dwThreadId = 0;

    if (IsBadWritePtr(lpExitCode,sizeof(DWORD)))
        return FALSE;

    *lpExitCode=0;

    // Start execution of remote ThreadProc
    hThread = CreateRemoteThread(this->hProcess,
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) RemoteProcFuncAddress,
                                this->pDataRemote,
                                0,
                                &dwThreadId
                                );
    if (!hThread)
        return FALSE;

    // wait for end of thread
    if (WaitForSingleObject(hThread, INFINITE)!=WAIT_OBJECT_0)
    {
        CloseHandle(hThread);
        return FALSE;
    }

    GetExitCodeThread(hThread, lpExitCode);
    CloseHandle(hThread);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SaveRemotePointers
// Object: used for internal function for allocating and freeing temporary 
//              pDataRemote and pCodeRemote
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CCodeInject::SaveRemotePointers()
{
    // save local data
    this->pDataRemoteBackup=this->pDataRemote;
    this->pCodeRemoteBackup=this->pCodeRemote;
    // avoid memory to be free as the test condition is this->pDataRemote
    // if we don't put this after restoration our pointer will point on freed memory
    this->pDataRemote=NULL;
    this->pCodeRemote=NULL;
}

//-----------------------------------------------------------------------------
// Name: FreeRemoteAllocatedMemory
// Object: used for internal function for restoring pDataRemote and pCodeRemote
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CCodeInject::RestoreRemotePointers()
{
    this->pDataRemote=this->pDataRemoteBackup;
    this->pCodeRemote=this->pCodeRemoteBackup;
}

//-----------------------------------------------------------------------------
// Name: RemoteGetModuleHandle
// Object: return the HMODULE of the specified library in the remote process
//          if module as been retrieve by LoadLibrary, bFreeLibraryShoudBeCall is put to TRUE
//          if it has been retrieve by GetModuleHandle, bFreeLibraryShoudBeCall is put to FALSE
//         Use this function if your injected code require a non kernell32|user32 library call
// Parameters :
//     in  : 
//     out : HMODULE* phRemoteModule : module handle in the remote process
//           BOOL* bFreeLibraryShoudBeCall : TRUE if LoadLibrary as been called (and so FreeLibrary should be called)
//                                           FALE if HMODULE as been retreive by GetModuleHandle
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::RemoteGetModuleHandle(TCHAR* pszDllName,HMODULE* phRemoteModule,BOOL* pbFreeLibraryShoudBeCall)
{
    GETMODULEHANDLE_PARAM data;
    SIZE_T ThreadProcCodeSize;
    BOOL bRet=TRUE;

    if (IsBadReadPtr(pszDllName,1))
        return FALSE;
    if (IsBadWritePtr(phRemoteModule,sizeof(HMODULE)))
        return FALSE;
    if (IsBadWritePtr(pbFreeLibraryShoudBeCall,sizeof(BOOL)))
        return FALSE;

    // backup remote pointer pDataRemote and pCodeRemote
    this->SaveRemotePointers();

    // compute code size
    ThreadProcCodeSize = ((LPBYTE) RemoteGetModuleHandleAfterThreadProc - (LPBYTE) RemoteGetModuleHandleThreadProc);
    // fill data struct infos
    data.pLoadLibrary=this->pLoadLibrary;
    data.pGetModuleHandle=this->pGetModuleHandle;
    _tcsncpy(data.pszDllName,pszDllName,MAX_PATH-1);
    data.pszDllName[MAX_PATH-1]=0;
    
    if (!this->SetParameter(&data,sizeof(data)))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }
    if (!this->SetCode((FARPROC)RemoteGetModuleHandleThreadProc,ThreadProcCodeSize))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }

    if (this->Execute())
    {
        if (this->GetParameter(&data,sizeof(data)))
        {
            *phRemoteModule=data.RemotehModule;
            *pbFreeLibraryShoudBeCall=data.bFreeLibraryShoudBeCall;
        }
        else
            bRet=FALSE;
    }
    else
        bRet=FALSE;
   
    // free memory allocated by SetParameter and SetCode
    this->FreeRemoteAllocatedMemory();

    // restore remote pointer pDataRemote and pCodeRemote
    this->RestoreRemotePointers();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RemoteFreeLibrary
// Object: Free library specified by hModule in remote process
//         Use this function if your injected code require a non kernell32|user32 library call
// Parameters :
//     in  : HMODULE hRemoteModule
//     out : 
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::RemoteFreeLibrary(HMODULE hRemoteModule)
{
    FREELIBRARY_PARAM data;
    SIZE_T ThreadProcCodeSize;
    BOOL bRet=TRUE;

    // backup remote pointer pDataRemote and pCodeRemote
    this->SaveRemotePointers();

    // compute code size
    ThreadProcCodeSize = ((LPBYTE) RemoteFreeLibraryAfterThreadProc - (LPBYTE) RemoteFreeLibraryThreadProc);
    // fill data struct infos
    data.hModule=hRemoteModule;
    data.pFreeLibrary=this->pFreeLibrary;
    
    if (!this->SetParameter(&data,sizeof(data)))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }
    if (!this->SetCode((FARPROC)RemoteFreeLibraryThreadProc,ThreadProcCodeSize))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }

    if (!this->Execute())
        bRet=FALSE;
   
    // free memory allocated by SetParameter and SetCode
    this->FreeRemoteAllocatedMemory();

    // restore remote pointer pDataRemote and pCodeRemote
    this->RestoreRemotePointers();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RemoteGetProcAddress
// Object: find proc address in remote process
//         Use this function if your injected code require a non kernell32|user32 library call
// Parameters :
//     in  : HMODULE hModule
//           TCHAR* pszProcName
//     out : FARPROC* ppFunc : proc address in remote process
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CCodeInject::RemoteGetProcAddress(HMODULE hRemoteModule,TCHAR* pszProcName,FARPROC* ppFunc)
{
    GETPROCADDRESS_PARAM data;
    SIZE_T ThreadProcCodeSize;
    BOOL bRet=TRUE;

    if (IsBadReadPtr(pszProcName,1))
        return FALSE;
    if (IsBadWritePtr(ppFunc,sizeof(FARPROC)))
        return FALSE;

    // backup remote pointer pDataRemote and pCodeRemote
    this->SaveRemotePointers();

    // compute code size
    ThreadProcCodeSize = ((LPBYTE) RemoteGetProcAddressAfterThreadProc - (LPBYTE) RemoteGetProcAddressThreadProc);
    // fill data struct infos
    data.hModule=hRemoteModule;
    data.pGetProcAddress=this->pGetProcAddress;

	CAnsiUnicodeConvert::TcharToAnsi(pszProcName,data.pszProcName,MAX_PATH);
    
    if (!this->SetParameter(&data,sizeof(data)))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }
    if (!this->SetCode((FARPROC)RemoteGetProcAddressThreadProc,ThreadProcCodeSize))
    {
        this->FreeRemoteAllocatedMemory();
        this->RestoreRemotePointers();
        return FALSE;
    }

    if (this->Execute())
    {
        if (this->GetParameter(&data,sizeof(data)))
        {
            *ppFunc=data.RemotepFunction;
        }
        else
            bRet=FALSE;
    }
    else
        bRet=FALSE;
   
    // free memory allocated by SetParameter and SetCode
    this->FreeRemoteAllocatedMemory();

    // restore remote pointer pDataRemote and pCodeRemote
    this->RestoreRemotePointers();

    return bRet;
}