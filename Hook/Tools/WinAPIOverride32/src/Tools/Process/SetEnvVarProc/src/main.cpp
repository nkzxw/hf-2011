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
*/

//-----------------------------------------------------------------------------
// Object: set environment variable for process not supporting WM_SETTINGCHANGE, like the services.exe process
//         SendMessageTimeout(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)_T("Environment"),SMTO_ABORTIFHUNG,50000,&dwResult);
//-----------------------------------------------------------------------------
#include <windows.h> 
#include <memory.h> 

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../ExportedDefinesAndStructs.h"

#define SHARED_MEMORY_SIZE 0x2000 
#define SHARED_MEMORY_NAME _T("SetEnvVarProcSharedMemory")

typedef struct tagInternalEnvVarDefinition 
{
    DWORD NameLen;// size in byte count including '\0'
    WCHAR* Name;
    DWORD ValueLen;// size in byte count including '\0'
    WCHAR* Value;
}INTERNAL_ENV_VAR_DEFINITION,*PINTERNAL_ENV_VAR_DEFINITION;

static LPVOID g_pSharedMemory = NULL; // pointer to shared memory
HANDLE g_hMapObject = NULL;  // handle to file mapping

BOOL bFirstLoadedDll=FALSE;
void DataProviderCode();
void DataConsumerCode();

BOOL WINAPI DllMain(HINSTANCE hModule,DWORD dwReason,LPVOID lpvReserved) 
{ 
    UNREFERENCED_PARAMETER(lpvReserved); 
    
    switch (dwReason) 
    { 
    case DLL_PROCESS_ATTACH: 
        {
            ::DisableThreadLibraryCalls(hModule);

            // make shared memory accessible for all account. 
            // --> we can inject dll into processes running under other users accounts
            SECURITY_DESCRIPTOR sd={0};
            sd.Revision=SECURITY_DESCRIPTOR_REVISION;
            sd.Control=SE_DACL_PRESENT;
            sd.Dacl=NULL; // assume everyone access
            SECURITY_ATTRIBUTES SecAttr={0};
            SecAttr.bInheritHandle=FALSE;
            SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
            SecAttr.lpSecurityDescriptor=&sd;

            // Create a named file mapping object.
            g_hMapObject = ::CreateFileMapping( 
                                            INVALID_HANDLE_VALUE, // use paging file
                                            &SecAttr,             // security attributes
                                            PAGE_READWRITE,       // read/write access
                                            0,                    // size: high 32-bits
                                            SHARED_MEMORY_SIZE,   // size: low 32-bits
                                            SHARED_MEMORY_NAME    // name of map object
                                            );
            if (g_hMapObject == NULL) 
                return FALSE; 

            // The first process to attach initializes memory.
            bFirstLoadedDll = (::GetLastError() != ERROR_ALREADY_EXISTS); 

            // Get a pointer to the file-mapped shared memory.
            g_pSharedMemory = ::MapViewOfFile( 
                                            g_hMapObject,     // object to map view of
                                            FILE_MAP_WRITE, // read/write access
                                            0,              // high offset:  map from
                                            0,              // low offset:   beginning
                                            0               // default: map entire file
                                            );
            if (g_pSharedMemory == NULL) 
            {
                ::CloseHandle(g_hMapObject);
                return FALSE; 
            }

            // Initialize memory if this is the first process.
            if (bFirstLoadedDll)
            {
                DataProviderCode();
                // keep dll into memory
                return TRUE;
            }
            else
            {
                // apply changes
                DataConsumerCode();

                // by returning FALSE, DLL_PROCESS_DETACH is not always called,
                // release handle here

                // Unmap shared memory from the process's address space.
                if (g_pSharedMemory)
                    ::UnmapViewOfFile(g_pSharedMemory); 

                // Close the process's handle to the file-mapping object.
                if (g_hMapObject)
                    ::CloseHandle(g_hMapObject);

                // free dll as soon as DataConsumerCode is executed in injected process
                return FALSE;
            }
        }
        break; 

    case DLL_PROCESS_DETACH: 
        if (bFirstLoadedDll)
        {
            // Unmap shared memory from the process's address space.
            if (g_pSharedMemory)
                ::UnmapViewOfFile(g_pSharedMemory); 

            // Close the process's handle to the file-mapping object.
            if (g_hMapObject)
                ::CloseHandle(g_hMapObject);
        }

        break; 

    default: 
        break; 
    } 

    return TRUE; 
} 

//-----------------------------------------------------------------------------
// Name: DataProviderCode
// Object: FirstLoadedDll code (data provider)
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void DataProviderCode()
{
    // initialization comes here
    
    PBYTE pBuffer=(PBYTE)g_pSharedMemory;

    // reset number of env var
    memset(pBuffer,0,sizeof(DWORD));
}


//-----------------------------------------------------------------------------
// Name: DataConsumerCode
// Object: secondly loaded dll code (data consumer)
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void DataConsumerCode()
{
    PBYTE pBuffer=(PBYTE)g_pSharedMemory;

    // read number of env var
    DWORD NbEnvVar=0;
    memcpy(&NbEnvVar,pBuffer,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    INTERNAL_ENV_VAR_DEFINITION* pEnvVarDef;

    // read environment variables name and value from shared memory and apply them
    for (DWORD Cnt=0;Cnt<NbEnvVar;Cnt++)
    {
        pEnvVarDef=(INTERNAL_ENV_VAR_DEFINITION*)pBuffer;
        // remember that Name and Value are relative to g_pSharedMemory (see SetEnvironmentVariables)
        pEnvVarDef->Name=(WCHAR*)((PBYTE)g_pSharedMemory+(ULONG_PTR)pEnvVarDef->Name);
        pEnvVarDef->Value=(WCHAR*)((PBYTE)g_pSharedMemory+(ULONG_PTR)pEnvVarDef->Value);
        ::SetEnvironmentVariableW(pEnvVarDef->Name,pEnvVarDef->Value);

        pBuffer+=sizeof(INTERNAL_ENV_VAR_DEFINITION)+pEnvVarDef->NameLen+pEnvVarDef->ValueLen;
    }
}


//-----------------------------------------------------------------------------
// Name: SetEnvironmentVariables
// Object: write environment variables name and value to shared memory
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE if SHARED_MEMORY_SIZE is not enough
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetEnvironmentVariables(ENV_VAR_DEFINITIONW* EnvVarArray,DWORD NbEnvVar)
{
    PBYTE pBuffer=(PBYTE)g_pSharedMemory;

    // reset number of env var
    memset(pBuffer,0,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    INTERNAL_ENV_VAR_DEFINITION EnvVarDef;
    for (DWORD Cnt=0;Cnt<NbEnvVar;Cnt++)
    {
        EnvVarDef.NameLen=(DWORD)(wcslen(EnvVarArray[Cnt].Name)+1)*sizeof(WCHAR); // include '\0'
        EnvVarDef.ValueLen=(DWORD)(wcslen(EnvVarArray[Cnt].Value)+1)*sizeof(WCHAR); // include '\0'

        // check available memory
        if ((PBYTE)g_pSharedMemory+SHARED_MEMORY_SIZE<pBuffer+sizeof(INTERNAL_ENV_VAR_DEFINITION)+EnvVarDef.NameLen+EnvVarDef.ValueLen)
        {
            // set number of env var written
            memcpy(g_pSharedMemory,&Cnt,sizeof(DWORD));

            return FALSE;
        }

        EnvVarDef.Name=(WCHAR*)(pBuffer+sizeof(INTERNAL_ENV_VAR_DEFINITION));
        memcpy(EnvVarDef.Name,EnvVarArray[Cnt].Name,EnvVarDef.NameLen);
        
        EnvVarDef.Value=(WCHAR*)(pBuffer+sizeof(INTERNAL_ENV_VAR_DEFINITION)+EnvVarDef.NameLen);
        memcpy(EnvVarDef.Value,EnvVarArray[Cnt].Value,EnvVarDef.ValueLen);

        // make name and value relative to pBuffer begin as g_pSharedMemory
        // as not the same value in different processes
        EnvVarDef.Name=(WCHAR*)((PBYTE)EnvVarDef.Name-(ULONG_PTR)g_pSharedMemory);
        EnvVarDef.Value=(WCHAR*)((PBYTE)EnvVarDef.Value-(ULONG_PTR)g_pSharedMemory);
        memcpy(pBuffer,&EnvVarDef,sizeof(INTERNAL_ENV_VAR_DEFINITION));
        pBuffer+=sizeof(INTERNAL_ENV_VAR_DEFINITION);

        pBuffer+=EnvVarDef.NameLen+EnvVarDef.ValueLen;
    }

    // write real number of env var at least
    memcpy(g_pSharedMemory,&NbEnvVar,sizeof(DWORD));

    return TRUE;
}