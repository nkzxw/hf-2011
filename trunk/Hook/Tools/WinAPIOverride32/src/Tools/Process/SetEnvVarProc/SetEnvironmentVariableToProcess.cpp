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

#include "SetEnvironmentVariableToProcess.h"

//-----------------------------------------------------------------------------
// Name: CSetEnvironmentVariableToProcess
// Object: constructor
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CSetEnvironmentVariableToProcess::CSetEnvironmentVariableToProcess(void)
{
    // get application path
    TCHAR Path[MAX_PATH];
    CStdFileOperations::GetAppPath(this->AppPath,MAX_PATH);

    this->pSetEnvironmentVariables=NULL;
    this->hSetEnvVarProcModule=NULL;
    this->bSetEnvVarProcLoaded=FALSE;

    this->pInjectLib=NULL;
    this->hInjLibModule=NULL;
    this->bInjLibLoaded=FALSE;

    this->HasOwnerShip=FALSE;
    // create mutex
    this->hMutex= ::CreateMutex(NULL, TRUE, CSetEnvironmentVariableToProcess_MUTEX_NAME);
    if (this->hMutex)
    {
        DWORD LastError = ::GetLastError(); 
        // if mutex is already existing
        if (ERROR_ALREADY_EXISTS == LastError)
        {
            // wait until we get ownership
            if (::WaitForSingleObject(hMutex,CSetEnvironmentVariableToProcess_MAX_WAIT)==WAIT_OBJECT_0)
            {
                this->HasOwnerShip=TRUE;
            }
        }
        else
            // mutex was not existing : we have ownership
            this->HasOwnerShip=TRUE;
    }

    if (this->HasOwnerShip)
    {
        // load SetEnvVarProc dll
        _tcscpy(Path,this->AppPath);
        _tcscat(Path,CSetEnvironmentVariableToProcess_SentEnvVarProcLibName);
        this->hSetEnvVarProcModule = ::LoadLibrary(Path);
        if (this->hSetEnvVarProcModule)
        {
            // get SetEnvironmentVariables function address
            this->pSetEnvironmentVariables=(pfSetEnvironmentVariables)::GetProcAddress(this->hSetEnvVarProcModule,SetEnvVarProc_SET_ENVIRONMENT_VARIABLES_EXPORTED_FUNC_NAME);
            if (this->pSetEnvironmentVariables)
                this->bSetEnvVarProcLoaded=TRUE;
        }
         
        if (!this->bSetEnvVarProcLoaded)
        {
            TCHAR Msg[2*MAX_PATH];
            _stprintf(Msg,_T("Error loading %s"),CSetEnvironmentVariableToProcess_SentEnvVarProcLibName);
            ::MessageBox(NULL,Msg,_T("Error"),MB_ICONERROR | MB_OK);
        }
        else
        {
            // load injlib
            _tcscpy(Path,this->AppPath);
            _tcscat(Path,CSetEnvironmentVariableToProcess_INJ_LIB_DLL_NAME);
            this->hInjLibModule = ::LoadLibrary(Path);
            if (this->hInjLibModule)
            {
                // get inlib function address
                this->pInjectLib=(InjectLib)::GetProcAddress(this->hInjLibModule,INJECTLIB_FUNC_NAME);
                if (this->pInjectLib)
                    this->bInjLibLoaded=TRUE;
            }

            if (!this->bInjLibLoaded)
            {
                TCHAR Msg[2*MAX_PATH];
                _stprintf(Msg,_T("Error loading %s"),CSetEnvironmentVariableToProcess_INJ_LIB_DLL_NAME);
                ::MessageBox(NULL,Msg,_T("Error"),MB_ICONERROR | MB_OK);
            }
        }
    }

    // try to adjust privilege (allow tu set env var of more processes)
    CPrivilege Privilege(FALSE);
    Privilege.SetPrivilege(SE_DEBUG_NAME,TRUE);
}

//-----------------------------------------------------------------------------
// Name: ~CSetEnvironmentVariableToProcess
// Object: destructor
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CSetEnvironmentVariableToProcess::~CSetEnvironmentVariableToProcess(void)
{
    if (this->hMutex)
    {
        // release mutex
        if (this->HasOwnerShip)
            ::ReleaseMutex(this->hMutex);

        ::CloseHandle(this->hMutex);
    }

    // free SetEnvVarProc dll
    if (this->hSetEnvVarProcModule)
        ::FreeLibrary(this->hSetEnvVarProcModule);

    // free injlib dll
    if (this->hInjLibModule)
        ::FreeLibrary(this->hInjLibModule);
}

//-----------------------------------------------------------------------------
// Name: IsObjectStateOk
// Object: check if object construction is ok (mutex state + function loaded)
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if all is ok
//-----------------------------------------------------------------------------
BOOL CSetEnvironmentVariableToProcess::IsObjectStateOk()
{
    return (this->HasOwnerShip 
            && this->bSetEnvVarProcLoaded
            && this->bInjLibLoaded
            );
}

//-----------------------------------------------------------------------------
// Name: SetEnvironmentVariables
// Object: write environment variables array to shared memory
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetEnvironmentVariableToProcess::SetEnvironmentVariables(ENV_VAR_DEFINITION* EnvVarArray,DWORD NbEnvVar)
{
    if (!this->IsObjectStateOk())
        return FALSE;

#if (defined(UNICODE)||defined(_UNICODE))

    return this->pSetEnvironmentVariables(EnvVarArray,NbEnvVar);

#else

    BOOL bRet;
    DWORD Cnt;
    ENV_VAR_DEFINITIONW* pEnvVarDefinitionW=new ENV_VAR_DEFINITIONW[NbEnvVar];

    for (Cnt=0;Cnt<NbEnvVar;Cnt++)
    {
        CAnsiUnicodeConvert::AnsiToUnicode(EnvVarArray[Cnt].Name,&pEnvVarDefinitionW[Cnt].Name);
        CAnsiUnicodeConvert::AnsiToUnicode(EnvVarArray[Cnt].Value,&pEnvVarDefinitionW[Cnt].Value);
    }

    bRet=this->pSetEnvironmentVariables(pEnvVarDefinitionW,NbEnvVar);

    for (Cnt=0;Cnt<NbEnvVar;Cnt++)
    {
        free(pEnvVarDefinitionW[Cnt].Name);
        free(pEnvVarDefinitionW[Cnt].Value);
    }
    delete pEnvVarDefinitionW;

    return bRet;

#endif
}

//-----------------------------------------------------------------------------
// Name: ApplyChanges
// Object: set environment variables for specified process(es)
//         Notice : applies to all processes with same name
// Parameters :
//     in  : TCHAR* ProcessName : process name
//           BOOL bFullPath : TRUE if ProcessName contain full process path
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetEnvironmentVariableToProcess::ApplyChanges(TCHAR* ProcessName,BOOL bFullPath)
{
    if (!this->IsObjectStateOk())
        return FALSE;

    BOOL bRet=TRUE;
    BOOL bFunctionRet;
    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 

    // Walk the process list of the system
    if (!Process32First(hSnap, &pe32))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    do 
    {
        if (bFullPath)
        {
            if (_tcsicmp(pe32.szExeFile,ProcessName)==0)
            {
                bFunctionRet = this->ApplyChanges(pe32.th32ProcessID);
                bRet = bRet && bFunctionRet;
            }
        }
        else
        {
            if (_tcsicmp(CStdFileOperations::GetFileName(pe32.szExeFile),ProcessName)==0)
            {
                bFunctionRet = this->ApplyChanges(pe32.th32ProcessID);
                bRet = bRet && bFunctionRet;
            }
        }
    } 
    while (Process32Next(hSnap, &pe32)); 

    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: ApplyChanges
// Object: set environment variables for specified process
// Parameters :
//     in  : DWORD ProcessId : process id
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetEnvironmentVariableToProcess::ApplyChanges(DWORD ProcessId)
{
    if (!this->IsObjectStateOk())
        return FALSE;

    // inject SetEnvVarProc to specified process
    // as it wont be the first loaded dll, dll in specified process will act like a data consumer
    // and dll will be auto unloaded

    // Notice : full dll path MUST be provided to this->pInjectLib
    TCHAR Path[MAX_PATH];
    _tcscpy(Path,this->AppPath);
    _tcscat(Path,CSetEnvironmentVariableToProcess_SentEnvVarProcLibName);
    // Notice : injlib return when library has been loaded (or after a timeout)
    return this->pInjectLib(ProcessId,Path);
}