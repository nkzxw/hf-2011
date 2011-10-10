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

#pragma once
#include <windows.h>
#include "../Tools/APIError/APIError.h"
#include <Psapi.h> // require psapi.lib
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

// psapi lib is requiered
#pragma comment(lib,"psapi")

/////////////// defines ///////////////
#define MAX_PROCESSES 1024
#define MAX_MODULES   1024

#define INACTIVE_PROCESSES_PID 0
#define SYSTEM_PID             4
/////////////// structs ///////////////
typedef struct _STRUCT_MODULE_INFOS
{
    HMODULE hModule;
    TCHAR  szModuleName[MAX_PATH];
    MODULEINFO sModinfo;
}STRUCT_MODULE_INFOS,*PSTRUCT_MODULE_INFOS;

typedef struct _STRUCT_PROCESS_INFOS
{
    DWORD dwProcessID;
    TCHAR  szProcessName[MAX_PATH];
    DWORD dwNbModules;
    STRUCT_MODULE_INFOS sModulesInfos[MAX_MODULES];
}STRUCT_PROCESS_INFOS,*PSTRUCT_PROCESS_INFOS;



/////////////// class ///////////////
class CProcessesInfos
{
public:
    CProcessesInfos(BOOL bShowErrors);
    ~CProcessesInfos(void);

    DWORD RetrieveProcessesInfos();
    DWORD RetrieveModulesAndBaseName(HANDLE hProcess,TCHAR* szBaseName,DWORD* pdwNbModules,STRUCT_MODULE_INFOS* psModulesInfos);
    DWORD RetrieveModuleInfos(HANDLE hProcess,HMODULE hModule,MODULEINFO* pmodinfo);

    BOOL bShowErrors;

    DWORD dwNbProcesses;
    DWORD dwNbProcessesSuccessfullyRetrieved;
    STRUCT_PROCESS_INFOS sProcessesInfos[MAX_PROCESSES];
};

