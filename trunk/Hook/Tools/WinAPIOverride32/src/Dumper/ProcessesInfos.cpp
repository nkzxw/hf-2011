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

#include "processesinfos.h"


CProcessesInfos::CProcessesInfos(BOOL bShowErrors)
{
    this->bShowErrors=bShowErrors;
    this->dwNbProcesses=0;
    this->dwNbProcessesSuccessfullyRetrieved=0;
    memset(this->sProcessesInfos,0,MAX_PROCESSES);//only until this->sProcessesInfos as a fixed size
}

CProcessesInfos::~CProcessesInfos(void)
{
}

DWORD CProcessesInfos::RetrieveModuleInfos(HANDLE hProcess,HMODULE hModule,MODULEINFO* pmodinfo)
{
    DWORD dwLastError;
    if (!GetModuleInformation(hProcess,hModule,pmodinfo,sizeof(MODULEINFO)))
    {
        dwLastError=GetLastError();
        if (this->bShowErrors)
            CAPIError::ShowError(dwLastError);
        return dwLastError;
    }
    return ERROR_SUCCESS;
}


DWORD CProcessesInfos::RetrieveModulesAndBaseName(HANDLE hProcess,TCHAR* szBaseName,DWORD* pdwNbModules,STRUCT_MODULE_INFOS* psModulesInfos)
{
    DWORD dwLastError;
    HMODULE hMods[MAX_MODULES];
    DWORD dwNeededBytes;
    DWORD dwCnt;

    // Get a list of all the modules in this process.
    if(EnumProcessModules(hProcess, hMods, sizeof(hMods), &dwNeededBytes))
    {
        // get module base name
        if (!GetModuleBaseName(hProcess,hMods[0],szBaseName,MAX_PATH))
        {
            if (this->bShowErrors)
                CAPIError::ShowLastError();
            _tcscpy(szBaseName,_T("Unknown"));
        }
        *pdwNbModules=(dwNeededBytes / sizeof(HMODULE));
        // to implement : if dwNbModules==MAX_MODULES -> may other modules exists

        for (dwCnt= 0; dwCnt<*pdwNbModules; dwCnt++ )
        {
            psModulesInfos[dwCnt].hModule=hMods[dwCnt];
            // Get the full path to the module's file.
            if (!GetModuleFileNameEx( hProcess, hMods[dwCnt], psModulesInfos[dwCnt].szModuleName,sizeof(psModulesInfos[dwCnt].szModuleName)/sizeof(TCHAR)))
            {
                _tcscpy(psModulesInfos[dwCnt].szModuleName,_T("Unknown"));
                if (this->bShowErrors)
                    CAPIError::ShowLastError();
            }
            // get module infos
            this->RetrieveModuleInfos(hProcess,psModulesInfos[dwCnt].hModule,&psModulesInfos[dwCnt].sModinfo);
        }
    }
    else
    {
        dwLastError=GetLastError();
        if (this->bShowErrors)
            CAPIError::ShowError(dwLastError);
        return dwLastError;
    }
    return ERROR_SUCCESS;
}



DWORD CProcessesInfos::RetrieveProcessesInfos()
{
    DWORD aProcesses[MAX_PROCESSES];
    DWORD dwNeededBytes;
    DWORD dwCnt;
    DWORD dwCntSuccess=0;
    DWORD dwLastError;
    HANDLE hProcess;
    this->dwNbProcesses=0;
    this->dwNbProcessesSuccessfullyRetrieved=0;

    if (!EnumProcesses( aProcesses, sizeof(aProcesses), &dwNeededBytes ) )
    {
        dwLastError=GetLastError();
        if (this->bShowErrors)
            CAPIError::ShowError(dwLastError);
        return dwLastError;
    }
    // Calculate how many process identifiers were returned.
    this->dwNbProcesses= dwNeededBytes / sizeof(DWORD);
    // to implement : if dwNbProcesses==MAX_PROCESSES -> may other processes exists

    // Print the name and process identifier for each process.
    for ( dwCnt= 0; dwCnt< dwNbProcesses; dwCnt++ )
    {
        // check special case
        if (aProcesses[dwCnt]==INACTIVE_PROCESSES_PID)
        {
            continue;
        }
        else if (aProcesses[dwCnt]==SYSTEM_PID)
        {
            // _tcscpy(this->sProcessesInfos[dwCntSuccess].szProcessName,"System");
            // dwCntSuccess++;
            continue;
        }

        this->sProcessesInfos[dwCntSuccess].dwProcessID=aProcesses[dwCnt];
        hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                        PROCESS_VM_READ,
                                        FALSE, this->sProcessesInfos[dwCntSuccess].dwProcessID);
        if (!hProcess)
        {
            if (this->bShowErrors)
                CAPIError::ShowLastError();
             continue;
        }
        this->RetrieveModulesAndBaseName(hProcess,
            this->sProcessesInfos[dwCntSuccess].szProcessName,
            &this->sProcessesInfos[dwCntSuccess].dwNbModules,
            (STRUCT_MODULE_INFOS*)&this->sProcessesInfos[dwCntSuccess].sModulesInfos);
        CloseHandle( hProcess );
        dwCntSuccess++;
    }
    this->dwNbProcessesSuccessfullyRetrieved=dwCntSuccess;
    return ERROR_SUCCESS;
}