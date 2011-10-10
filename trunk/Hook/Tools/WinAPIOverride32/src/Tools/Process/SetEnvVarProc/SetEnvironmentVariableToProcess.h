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
// Object: class helper for SetEnvVarProc dll
//-----------------------------------------------------------------------------

/*
example of use
CSetEnvironmentVariableToProcess SetEnv;
CSetEnvironmentVariableToProcess::ENV_VAR_DEFINITION EnvVarDef[2] = { 
                                                                      {_T("COR_ENABLE_PROFILING"),_T("0")},
                                                                      {_T("COR_PROFILER"),_T("")}
                                                                     };
if (SetEnv.SetEnvironmentVariables(EnvVarDef,2))
    SetEnv.ApplyChanges(_T("services.exe"),FALSE);
*/

#pragma once
#include "ExportedDefinesAndStructs.h"
#include "../../String/AnsiUnicodeConvert.h"
#include "../InjLib/InjLib.h"
#include "../../File/StdFileOperations.h"
#include "../../Privilege/Privilege.h"

#include <Tlhelp32.h>
#include <stdio.h>

#define CSetEnvironmentVariableToProcess_MUTEX_NAME _T("CSetEnvironmentVariableToProcess")
#define CSetEnvironmentVariableToProcess_MAX_WAIT 300000 // 5 min
#define CSetEnvironmentVariableToProcess_SentEnvVarProcLibName _T("SetEnvVarProc.dll")
#define CSetEnvironmentVariableToProcess_INJ_LIB_DLL_NAME _T("InjLib.dll")

class CSetEnvironmentVariableToProcess
{
private:
    TCHAR AppPath[MAX_PATH];
    BOOL HasOwnerShip;
    HANDLE hMutex;

    BOOL bSetEnvVarProcLoaded;
    HMODULE hSetEnvVarProcModule;
    pfSetEnvironmentVariables pSetEnvironmentVariables;

    BOOL bInjLibLoaded;
    HMODULE hInjLibModule;
    InjectLib pInjectLib;

    BOOL IsObjectStateOk();
    
    
public:
    CSetEnvironmentVariableToProcess(void);
    ~CSetEnvironmentVariableToProcess(void);

#if (defined(UNICODE)||defined(_UNICODE))
    typedef ENV_VAR_DEFINITIONW ENV_VAR_DEFINITION;
    typedef PENV_VAR_DEFINITIONW PENV_VAR_DEFINITION;
#else
    typedef struct tagEnvVarDefinition 
    {
        CHAR* Name;
        CHAR* Value;
    }ENV_VAR_DEFINITION,*PENV_VAR_DEFINITION;
#endif

    BOOL SetEnvironmentVariables(ENV_VAR_DEFINITION* EnvVarArray,DWORD NbEnvVar);
    BOOL ApplyChanges(TCHAR* ProcessName,BOOL bFullPath);// applies to all processes with same name
    BOOL ApplyChanges(DWORD ProcessId);
};
