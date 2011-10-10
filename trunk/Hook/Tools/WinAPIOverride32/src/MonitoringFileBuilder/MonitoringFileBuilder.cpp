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
// Object: manages monitoring files creation and update
//-----------------------------------------------------------------------------

#include "monitoringfilebuilder.h"

//-----------------------------------------------------------------------------
// Name: CMonitoringFileBuilder
// Object: constructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CMonitoringFileBuilder::CMonitoringFileBuilder(HINSTANCE hInstance,HWND hWndParent)
{
    this->hInstance=hInstance;
    this->hWndParent=hWndParent;

    // get app full path
    GetModuleFileName(GetModuleHandle(NULL),this->CurrentDirectory,MAX_PATH);
    // get working directory
    TCHAR* psz=_tcsrchr(this->CurrentDirectory,'\\');
    if (psz)
    {
        // we have found '\'
        // make CurrentDirectory end after '\'
        psz++;
        *psz=0;
    }
    else
        _tcscat(this->CurrentDirectory,_T("\\"));

    // forge proxy filename
    _tcscpy(this->ProxyFileName,this->CurrentDirectory);
    _tcscat(this->ProxyFileName,PROXY_FILE_NAME);

    // empty this->pszFuncDefinition
    *this->pszFuncDefinition=0;
    // assume pszFuncDefinition will be ended using _tcsncpy
    this->pszFuncDefinition[MAX_CONFIG_FILE_LINE_SIZE-1]=0;

    this->bGenerateFirstBytesAnalysisInformation=FALSE;
    this->bCheckHookAvaibility=FALSE;
    this->bGenerateFirstBytesAnalysisInformation=TRUE;
    this->OnlineMSDN=FALSE;
    this->TryToCall=FALSE;
    this->bUseProxy=FALSE;

    this->pOnlineMSDNSearch=new COnlineMSDNSearch();
    this->pOnlineMSDNSearch->SetUserMessageInformationCallBack(CMonitoringFileBuilder::StaticReportUserMessageInfo,this);

    this->bCancelCurrentOperation=FALSE;
    this->UserMessageInformationCallBackUserParam=NULL;
    this->UserMessageInformationCallBack=NULL;

    this->PercentCompletedCallBack=NULL;
    this->PercentCompletedCallBackUserParam=NULL;
    this->pCurrentExportPE=NULL;

    this->hOutputFile = 0;
    *this->tOutputFileName=0;

    this->hCancelEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
}

CMonitoringFileBuilder::~CMonitoringFileBuilder(void)
{
    
    CloseHandle(this->hCancelEvent);
    delete this->pOnlineMSDNSearch;
}

//-----------------------------------------------------------------------------
// Name: CancelCurrentOperation
// Object: cancel current operation (generation or update)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::CancelCurrentOperation()
{
    SetEvent(this->hCancelEvent);
    this->bCancelCurrentOperation=TRUE;
    this->pOnlineMSDNSearch->Cancel();
}
//-----------------------------------------------------------------------------
// Name: ReportUserMessageInfo
// Object: report an error message
// Parameters :
//     in  : TCHAR* Message : message to report
//           CMonitoringFileBuilder::tagUserMessagesTypes MessageType : message type
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::StaticReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType,LPVOID UserParam)
{
    ((CMonitoringFileBuilder*)UserParam)->ReportUserMessageInfo(Message,MessageType);
}

//-----------------------------------------------------------------------------
// Name: ReportUserMessageInfo
// Object: report an error message
// Parameters :
//     in  : TCHAR* Message : message to report
//           CMonitoringFileBuilder::tagUserMessagesTypes MessageType : message type
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::ReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType)
{
    // if user message callback is defined
    if (this->UserMessageInformationCallBack)
        // call it
        this->UserMessageInformationCallBack(Message,MessageType,this->UserMessageInformationCallBackUserParam);
}
//-----------------------------------------------------------------------------
// Name: SetUserMessageInformationCallBack
// Object: set the error message callback
// Parameters :
//     in  : tagUserMessageInformationCallBack CallBack : callback called for each message information
//           LPVOID UserParam : user parameter
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::SetUserMessageInformationCallBack(tagUserMessageInformationCallBack CallBack,LPVOID UserParam)
{
    this->UserMessageInformationCallBackUserParam=UserParam;
    this->UserMessageInformationCallBack=CallBack;
}

//-----------------------------------------------------------------------------
// Name: SetPercentCompletedCallback
// Object: set the percent completed callback
// Parameters :
//     in  : tagPercentCompletedCallBack CallBack : callback called each time percent completed changes
//           LPVOID UserParam : user parameter
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::SetPercentCompletedCallback(tagPercentCompletedCallBack CallBack,LPVOID UserParam)
{
    this->PercentCompletedCallBack=CallBack;
    this->PercentCompletedCallBackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetWorkingFile
// Object: update the fields pszFileToMonitor, tWorkingDir and tDllName
// Parameters :
//     in  : TCHAR* pszFile : name of file we are going to work with
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::SetWorkingFile(TCHAR* pszFile)
{
    TCHAR* pTChar;
    this->pszFileToMonitor=pszFile;

    _tcscpy(this->tWorkingDir,pszFile);
    pTChar=_tcsrchr(this->tWorkingDir,'\\');
    if (pTChar)
        *pTChar=0;
    else
        *this->tWorkingDir=0;

    // check if file (generally dll) belongs to windows default dll directories 
    // if it is the case, remove path from pszFileToMonitor to keep only filename
    pTChar=_tcsrchr(this->pszFileToMonitor,'\\');
    if (pTChar)
    {
        DWORD Size=(DWORD)(pTChar-this->pszFileToMonitor);
        TCHAR pszWinDirectory[MAX_PATH];

        // check if file exists in SYSTEM32 path
        *pszWinDirectory=0;
        GetSystemDirectory(pszWinDirectory,MAX_PATH);
        if (_tcsnicmp(pszWinDirectory,this->pszFileToMonitor,Size)==0)
        {
            // keep only file name
            _tcscpy(this->tDllName,pTChar+1);
        }
        else
        {
            // check if file exists in Windows directory
            *pszWinDirectory=0;
            GetWindowsDirectory(pszWinDirectory,MAX_PATH);
            if (_tcsnicmp(pszWinDirectory,this->pszFileToMonitor,Size)==0)
            {
                // keep only file name
                _tcscpy(this->tDllName,pTChar+1);
            }
            else
                _tcscpy(this->tDllName,this->pszFileToMonitor);
        }
    }
}

BOOL CMonitoringFileBuilder::CreateMonitoringFile(IN TCHAR* FilePath)
{
    *this->tOutputFileName = 0;

    // create monitoring file
    if (!CTextFile::CreateTextFile(FilePath,&this->hOutputFile))
        return FALSE;

    _tcscpy(this->tOutputFileName,FilePath);
    return TRUE;
}

BOOL CMonitoringFileBuilder::GenerateNETMonitoring(TCHAR* pszFileToMonitor,TCHAR* pszMonitoringFile,BOOL FullGeneration)
{
    // fill this->tWorkingDir
    this->SetWorkingFile(pszFileToMonitor);

    // reset the cancel var
    this->bCancelCurrentOperation=FALSE;

    CPE Pe(pszFileToMonitor);
    Pe.Parse(FALSE,FALSE);
    if (!Pe.IsNET())
    {
        TCHAR sz[2*MAX_PATH];
        _stprintf(sz,_T("%s is not a .Net assembly"),pszFileToMonitor);
        this->ReportUserMessageInfo(sz,USER_MESSAGE_ERROR);
        return FALSE;
    }
    CNetMonitoringFileGenerator NetMonitoringFileGenerator(pszFileToMonitor);
    if (!NetMonitoringFileGenerator.ParseAssembly())
    {
        TCHAR sz[2*MAX_PATH];
        _stprintf(sz,_T("Error parsing .Net assembly %s"),pszFileToMonitor);
        this->ReportUserMessageInfo(sz,USER_MESSAGE_ERROR);
        return FALSE;
    }
    if (!FullGeneration)
    {
        // show the function selection dialog
        CFunctionsSelectionUI FunctionsSelectionUI(&NetMonitoringFileGenerator);
        if (FunctionsSelectionUI.Show(this->hInstance,this->hWndParent)!=IDOK)
            return FALSE;
    }
    return NetMonitoringFileGenerator.GenerateMonitoringFile(pszMonitoringFile);
}

BOOL CMonitoringFileBuilder::GenerateCOMTypeLibraryMonitoring(TCHAR* TypeLibraryPath)
{
    CCOMMonitoringFileGenerator COMMonitoringFileGenerator(
                                                            this->PercentCompletedCallBack,
                                                            this->PercentCompletedCallBackUserParam,
                                                            (CCOMMonitoringFileGenerator::tagUserMessageInformationCallBack)this->UserMessageInformationCallBack,
                                                            this->UserMessageInformationCallBackUserParam
                                                            );

    TCHAR OutputDirectory[MAX_PATH];
    CStdFileOperations::GetAppPath(OutputDirectory,MAX_PATH);
    _tcscat(OutputDirectory,COM_MONITORING_FILES_PATH);
    return COMMonitoringFileGenerator.GenerateCOMTypeLibraryMonitoring(TypeLibraryPath,OutputDirectory,this->hCancelEvent);
}
BOOL CMonitoringFileBuilder::GenerateCOMAllRegisteredTypeLibrariesMonitoring()
{
    ResetEvent(this->hCancelEvent);

    CCOMMonitoringFileGenerator COMMonitoringFileGenerator( this->PercentCompletedCallBack,
                                                            this->PercentCompletedCallBackUserParam,
                                                            (CCOMMonitoringFileGenerator::tagUserMessageInformationCallBack)this->UserMessageInformationCallBack,
                                                            this->UserMessageInformationCallBackUserParam
                                                            );

    TCHAR OutputDirectory[MAX_PATH];
    CStdFileOperations::GetAppPath(OutputDirectory,MAX_PATH);
    _tcscat(OutputDirectory,COM_MONITORING_FILES_PATH);
    return COMMonitoringFileGenerator.GenerateCOMAllRegisteredTypeLibrariesMonitoring(OutputDirectory,this->hCancelEvent);
}

//-----------------------------------------------------------------------------
// Name: GenerateExportMonitoring
// Object: generate an monitoring file for export table
// Parameters :
//     in  : TCHAR* pszFileToMonitor : file to monitor
//           TCHAR* pszMonitoringFile : generated monitoring file name
//           BOOL FullGeneration : TRUE for full generation, FALSE for partial generation
//           BOOL bCheckOnly : TRUE if we are only doing a check on the file
//           BOOL Update : TRUE if we are doing an update
//           TCHAR* pszOldMonitoringFile : old monitoring file (used only for updates)
//           HINSTANCE hInstance : module instance containing FunctionSelection dialog resource
//           HWND hWndParent : parent window handle
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::GenerateExportMonitoring(TCHAR* pszFileToMonitor,TCHAR* pszMonitoringFile,
                                                        BOOL FullGeneration,
                                                        BOOL bCheckOnly,
                                                        BOOL Update,
                                                        TCHAR* pszOldMonitoringFile)
{
    CLinkListItem* pItemFunction;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunction;
    BYTE PercentCompleted;
    DWORD NbFunctions;
    DWORD NbFunctionsParsed;
    CLinkList ListCheckExportTableResult(sizeof(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT));
    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT CheckExportResult;
    BOOL bDoChecking;
    TCHAR pszMsg[2*MAX_PATH];
    
    // flag to know if we have to do function checking
    bDoChecking=this->bCheckHookAvaibility||this->bGenerateFirstBytesAnalysisInformation||bCheckOnly;

    // set proxy file if required
    if (this->bUseProxy)
    {
        if (!this->pOnlineMSDNSearch->SetProxyFile(this->ProxyFileName))
            return FALSE;
    }
    // fill class members from args
    this->Update=Update;
    this->pszOldMonitoringFile=pszOldMonitoringFile;
    this->bImportTable=FALSE;

    // parse export table
    CPE pe(pszFileToMonitor);
    if (!pe.Parse(TRUE,FALSE))
        return FALSE;

    CPE UnstubDllPe;

    // fill this->tWorkingDir
    if (this->DllStub.IsStubDll( CStdFileOperations::GetFileName(pszFileToMonitor) ) )
    {
        TCHAR RealModule[MAX_PATH];
        TCHAR RealDllName[MAX_PATH];
        if (!this->DllStub.GetRealModuleNameFromStubName(_T(""), CStdFileOperations::GetFileName(pszFileToMonitor),RealDllName))
            return FALSE;

        if (!::GetSystemDirectory(RealModule,MAX_PATH))
        {
            return FALSE;
        }
        _tcscat(RealModule,_T("\\"));
        _tcscat(RealModule,RealDllName);
        this->SetWorkingFile(RealModule);
        UnstubDllPe.Parse(RealModule,TRUE,FALSE,FALSE);
        this->pCurrentExportPE=&UnstubDllPe;
    }
    else
    {
        this->SetWorkingFile(pszFileToMonitor);
        this->pCurrentExportPE=&pe;
    }

    // reset the cancel var
    this->bCancelCurrentOperation=FALSE;

    // reset the Internet connection error flag
    this->pOnlineMSDNSearch->bInternetConnectionError=FALSE;

    

    if (!bCheckOnly)
    {
        // create monitoring file
        if (!this->CreateMonitoringFile(pszMonitoringFile))
        {
            CAPIError::ShowLastError();
            this->pCurrentExportPE=NULL;
            return FALSE;
        }

        // write monitoring file header
        this->WriteHeader();
    }


    // check if lib is a microsoft one
    this->bMicrosoftDll=this->IsMicrosoftLibrary(this->pszFileToMonitor);

    // check for full generation only after having checked exported functions
    // as this method modify pe
    if (!FullGeneration)
    {
        // show the function selection dialog
        CFunctionsSelectionUI FunctionsSelectionUI(&pe,TRUE);
        if (FunctionsSelectionUI.Show(this->hInstance,this->hWndParent)!=IDOK)
        {
            if (!bCheckOnly)
                CloseHandle(this->hOutputFile);
            this->pCurrentExportPE=NULL;
            return FALSE;
        }
    }

    // if we have to check functions
    if (bDoChecking)
    {
        this->ReportUserMessageInfo(_T("Analysing hook availability...\r\n"),USER_MESSAGE_INFORMATION);
        if (!CHookAvailabilityCheck::CheckExportTable(this->tWorkingDir,
            this->pszFileToMonitor,
            &ListCheckExportTableResult,
            TRUE))
        {
            _tcscpy(pszMsg,_T("Error checking export table of "));
            _tcscat(pszMsg,this->tDllName);
            MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            if (!bCheckOnly)
                CloseHandle(this->hOutputFile);
            this->pCurrentExportPE=NULL;
            return FALSE;
        }
    }


    // reset percent completed
    PercentCompleted=0;
    // reset number of function parsed
    NbFunctionsParsed=0;
    // get number of functions to parse
    NbFunctions=pe.pExportTable->GetItemsCount();
    // update the percent graph
    if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
        this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);

    HANDLE hFileToMonitor;
    // open file to check if FirstBytesCanExecuteAnyWhere
    hFileToMonitor=CreateFile(pszFileToMonitor,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);

    TCHAR FindStackSizeByCallApplicationName[MAX_PATH];
    if (this->TryToCall)
    {
        _tcscpy(FindStackSizeByCallApplicationName,this->CurrentDirectory);
        _tcscat(FindStackSizeByCallApplicationName,FindStackSizeByCall_APPLICATION_NAME);
        CBinResource::ExtractBinResource(this->hInstance,EXE_RESOURCE_SECTION_NAME,IDR_EXE_FIND_STACK_BY_CALL,FindStackSizeByCallApplicationName);
    }
    
    // for each exported function
    for (pItemFunction=pe.pExportTable->Head;pItemFunction;pItemFunction=pItemFunction->NextItem)
    {
        // retrieve func infos
        pExportFunction=(CPE::EXPORT_FUNCTION_ITEM*)pItemFunction->ItemData ;
        // fill this->tFunctionName field
        _tcscpy(this->tFunctionName,pExportFunction->FunctionName);

        // get ordinal value
        this->FunctionOrdinal=pExportFunction->ExportedOrdinal;

        if (!bCheckOnly)
        {
            // if ordinal only
            if (*this->tFunctionName==0)
                _stprintf(pszMsg,_T("\r\nGenerating monitoring data for function ordinal 0x%.4X ...\r\n"),this->FunctionOrdinal);
            else
                _stprintf(pszMsg,_T("\r\nGenerating monitoring data for function %s ...\r\n"),this->tFunctionName);
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);
        }

        if (bDoChecking)
        {
            // set default values to CheckExportResult
            CheckExportResult.ExportTableCheckedOk=FALSE;
            CheckExportResult.FunctionFirstByteCheckResult.CheckResult=CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE;
            CheckExportResult.FunctionFirstByteCheckResult.FirstBytesCanBeExecutedAnyWhereResult.FirstBytesCanBeExecutedAnyWhereResult=CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO;
            CheckExportResult.FunctionFirstByteCheckResult.FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace=0;

            // generate monitoring information for function and check first bytes
            this->FunctionAnalysisAndGeneration(this->tFunctionName,
                                                &ListCheckExportTableResult,
                                                FALSE,// if first bytes analysis has already been done
                                                !bCheckOnly,
                                                !bCheckOnly,
                                                &CheckExportResult);
        }
        else
            // generate monitoring information for function
            this->GenerateFunctionMonitoring(pszFileToMonitor,NULL);


        // check if user has canceled operation after each GenerateFunctionMonitoring call
        if (this->bCancelCurrentOperation)
        {
            // report info User cancel
            this->ReportUserMessageInfo(_T("Operation canceled by user\r\n"),USER_MESSAGE_INFORMATION);
            
            // go out of for
            break;
        }

        // update percent
        NbFunctionsParsed++;
        PercentCompleted=(BYTE)((NbFunctionsParsed*100)/NbFunctions);
        // update the percent graph
        if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
            this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);

    }
    if (this->TryToCall)
    {
        ::DeleteFile(FindStackSizeByCallApplicationName);
    }

    // close file to monitor
    CloseHandle(hFileToMonitor);

    if (!bCheckOnly)
        // close monitoring file
        CloseHandle(this->hOutputFile);

    this->pCurrentExportPE=NULL;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GenerateImportMonitoring
// Object: generate an monitoring file for import table
// Parameters :
//     in  : TCHAR* pszFileToMonitor : file to monitor
//           TCHAR* pszMonitoringFile : generated monitoring file name
//           BOOL FullGeneration : TRUE for full generation, FALSE for partial generation
//           BOOL bCheckOnly : TRUE if we are only doing a check on the file
//           BOOL Update : TRUE if we are doing an update
//           TCHAR* pszOldMonitoringFile : old monitoring file (used only for updates)
//           HINSTANCE hInstance : module instance containing FunctionSelection dialog resource
//           HWND hWndParent : parent window handle
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::GenerateImportMonitoring(TCHAR* pszFileToMonitor,TCHAR* pszMonitoringFile,
                                                        BOOL FullGeneration,
                                                        BOOL bCheckOnly,
                                                        BOOL Update,
                                                        TCHAR* pszOldMonitoringFile
                                                        )
{
    TCHAR pszMsg[2*MAX_PATH];
    TCHAR tDllFullPath[MAX_PATH];
    CLinkListItem* pItemLibrary;
    CLinkListItem* pItemFunction;
    BYTE PercentCompleted;
    DWORD NbFunctions;
    DWORD NbFunctionsParsed;
    BOOL bParseLibSuccess;
    CPE* ppeLib;
    CLinkList ListCheckExportTableResult(sizeof(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT));
    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT CheckExportResult;
    BOOL bDoChecking;

    CPE::IMPORT_LIBRARY_ITEM* pImportLibrary;
    CPE::IMPORT_FUNCTION_ITEM* pImportFunction;

    // flag to know if we have to do function checking
    bDoChecking=this->bCheckHookAvaibility||this->bGenerateFirstBytesAnalysisInformation||bCheckOnly;

    // set proxy file if required
    if (this->bUseProxy)
    {
        if (!this->pOnlineMSDNSearch->SetProxyFile(this->ProxyFileName))
            return FALSE;
    }
    // fill class members from args
    this->Update=Update;
    this->pszOldMonitoringFile=pszOldMonitoringFile;
    this->bImportTable=TRUE;

    // fill this->tWorkingDir
    this->SetWorkingFile(pszFileToMonitor);

    // reset the cancel var
    this->bCancelCurrentOperation=FALSE;

    // reset the Internet connection error flag
    this->pOnlineMSDNSearch->bInternetConnectionError=FALSE;

    // parse import table
    CPE pe(pszFileToMonitor);
    if (!pe.Parse(FALSE,TRUE))
        return FALSE;

    // if partial generation
    if (!FullGeneration)
    {
        // show the function selection dialog
        CFunctionsSelectionUI FunctionsSelectionUI(&pe,FALSE);
        if (FunctionsSelectionUI.Show(this->hInstance,this->hWndParent)!=IDOK)
            return FALSE;
    }


    if (!bCheckOnly)
    {
        // create monitoring file
        if (!this->CreateMonitoringFile(pszMonitoringFile))
        {
            CAPIError::ShowLastError();
            return FALSE;
        }
        // write monitoring file header
        this->WriteHeader();
    }

    // reset percent completed
    PercentCompleted=0;
    // reset number of function parsed
    NbFunctionsParsed=0;
    // get full number of functions (imported funcs for each imported lib)
    NbFunctions=0;
    for (pItemLibrary=pe.pImportTable->Head;pItemLibrary;pItemLibrary=pItemLibrary->NextItem)
    {
        // retrieve library name
        pImportLibrary=(CPE::IMPORT_LIBRARY_ITEM*)pItemLibrary->ItemData;
        NbFunctions+=pImportLibrary->pFunctions->GetItemsCount();
    }
    // update the percent graph
    if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
        this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);

    TCHAR FindStackSizeByCallApplicationName[MAX_PATH];
    if (this->TryToCall)
    {
        _tcscpy(FindStackSizeByCallApplicationName,this->CurrentDirectory);
        _tcscat(FindStackSizeByCallApplicationName,FindStackSizeByCall_APPLICATION_NAME);
        CBinResource::ExtractBinResource(this->hInstance,EXE_RESOURCE_SECTION_NAME,IDR_EXE_FIND_STACK_BY_CALL,FindStackSizeByCallApplicationName);
    }

    // for each imported library
    for (pItemLibrary=pe.pImportTable->Head;pItemLibrary;pItemLibrary=pItemLibrary->NextItem)
    {
        // retrieve library name
        pImportLibrary=(CPE::IMPORT_LIBRARY_ITEM*)pItemLibrary->ItemData;

        // get real library name
        if (this->DllStub.IsStubDll(pImportLibrary->LibraryName))
        {
            if (!this->DllStub.GetRealModuleNameFromStubName(CStdFileOperations::GetFileName(pszFileToMonitor),pImportLibrary->LibraryName,this->tDllName))
            {
                // display report for library monitoring generation
                _stprintf(pszMsg,_T("Error finding reference for stub imported library %s ...\r\n"),pImportLibrary->LibraryName);
                this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_ERROR);
            }
        }
        else
            _tcscpy(this->tDllName,pImportLibrary->LibraryName);

        // if not only checking
        if (!bCheckOnly)
        {
            // display report for library monitoring generation
            _stprintf(pszMsg,_T("\r\nGenerating monitoring data for imported library %s ...\r\n"),this->tDllName);
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);
        }

        this->bMicrosoftDll=FALSE;
        ppeLib=NULL;
        bParseLibSuccess=FALSE;
        this->pCurrentExportPE=NULL;
        if (CDllFinder::FindDll(pszFileToMonitor,this->tWorkingDir,this->tDllName,tDllFullPath,FALSE))
        {
            // check if lib is a microsoft one and show a friendly message
            this->bMicrosoftDll=this->IsMicrosoftLibrary(tDllFullPath);
            
            // parse export table of imported library
            ppeLib=new CPE(tDllFullPath);
            bParseLibSuccess=ppeLib->Parse(TRUE,FALSE);

            this->pCurrentExportPE=ppeLib;
        }
        else
        {
            // update percent
            NbFunctionsParsed+=pImportLibrary->pFunctions->GetItemsCount();
            PercentCompleted=(BYTE)((NbFunctionsParsed*100)/NbFunctions);
            // update the percent graph
            if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
                this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);
            
            // work on next library
            continue;
        }

        // if we have to check functions
        if (bDoChecking)
        {
            // clear list if it contains already used data
            ListCheckExportTableResult.RemoveAllItems();

            _stprintf(pszMsg,_T("Analysing hook availability for %s...\r\n"),this->tDllName);
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);
            // check the export table of the imported library (we don't do first bytes analysis for each func now, we will do it later)
            if (!CHookAvailabilityCheck::CheckExportTable(this->tWorkingDir,
                                                      //  this->tDllName,
                                                        ppeLib,
                                                        &ListCheckExportTableResult,
                                                        FALSE))
            {
                _tcscpy(pszMsg,_T("Error checking export table of "));
                _tcscat(pszMsg,this->tDllName);

                this->pCurrentExportPE=NULL;
                if (ppeLib)
                    delete ppeLib;
                
                this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_ERROR);

                // update percent
                NbFunctionsParsed+=pImportLibrary->pFunctions->GetItemsCount();
                PercentCompleted=(BYTE)((NbFunctionsParsed*100)/NbFunctions);
                // update the percent graph
                if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
                    this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);

                // work on next library
                continue;
            }
        }


        // for each imported function
        for (pItemFunction=pImportLibrary->pFunctions->Head;pItemFunction;pItemFunction=pItemFunction->NextItem)
        {
            // retrieve func infos
            pImportFunction=(CPE::IMPORT_FUNCTION_ITEM*)pItemFunction->ItemData ;

            // get function name from import table
            _tcscpy(this->tFunctionName,pImportFunction->FunctionName);

            // get ordinal value
            this->FunctionOrdinal=pImportFunction->Ordinal;

            if (!bCheckOnly)
            {
                // if ordinal only
                if (*this->tFunctionName==0)
                    _stprintf(pszMsg,_T("\r\nGenerating monitoring data for function ordinal 0x%.8X ...\r\n"),this->FunctionOrdinal);
                else
                    _stprintf(pszMsg,_T("\r\nGenerating monitoring data for function %s ...\r\n"),this->tFunctionName);
                this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);
            }

            if (bDoChecking)
            {
                // set default values to CheckExportResult
                CheckExportResult.ExportTableCheckedOk=FALSE;
                CheckExportResult.FunctionFirstByteCheckResult.CheckResult=CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE;
                CheckExportResult.FunctionFirstByteCheckResult.FirstBytesCanBeExecutedAnyWhereResult.FirstBytesCanBeExecutedAnyWhereResult=CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO;
                CheckExportResult.FunctionFirstByteCheckResult.FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace=0;

                // generate monitoring information for function and check first bytes
                this->FunctionAnalysisAndGeneration(this->tFunctionName,
                                                    &ListCheckExportTableResult,
                                                    TRUE,// first bytes analysis has not been done
                                                    !bCheckOnly,
                                                    !bCheckOnly,
                                                    &CheckExportResult);
            }
            else
                // generate monitoring information for function
                this->GenerateFunctionMonitoring(pszFileToMonitor,NULL);

            // check if operation is ask to be canceled
            if (this->bCancelCurrentOperation)
            {
                // let user info be report by other while
                // go out of for
                break;
            }

            // update percent
            NbFunctionsParsed++;
            PercentCompleted=(BYTE)((NbFunctionsParsed*100)/NbFunctions);
            // update the percent graph
            if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
                this->PercentCompletedCallBack(PercentCompleted,this->PercentCompletedCallBackUserParam);

        }

        if (ppeLib)
            delete ppeLib;

        if (this->bCancelCurrentOperation)
        {
            // report info User cancel
            this->ReportUserMessageInfo(_T("Operation canceled by user\r\n"),USER_MESSAGE_INFORMATION);

            // go out of for
            break;
        }

    }
    if (this->TryToCall)
    {
        ::DeleteFile(FindStackSizeByCallApplicationName);
    }

    if (!bCheckOnly)
        // close monitoring file
        CloseHandle(this->hOutputFile);

    this->pCurrentExportPE=NULL;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteHeader
// Object: write a small header containing version informations in the generated monitoring file
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::WriteHeader()
{
    BOOL bVersionInfoPresent;
    TCHAR* psz;
    TCHAR pszComments[4*MAX_PATH];
    CVersion Version;
    DWORD dwWrittenBytes;
    TCHAR pszImportOrExport[MAX_PATH];

    if (this->bImportTable)
        _tcscpy(pszImportOrExport,_T("imports"));
    else
        _tcscpy(pszImportOrExport,_T("exports"));

    // get version
    bVersionInfoPresent=Version.Read(this->pszFileToMonitor);

    // retrieve file short name
    psz=_tcsrchr(this->pszFileToMonitor,'\\');
    if (psz)
        psz++;
    else
        psz=this->pszFileToMonitor;

    // write monitoring file comments
    if (bVersionInfoPresent)
        _stprintf(pszComments,
        _T(";      Monitoring file generated for %s table of %s v%s by MonitoringFileBuilder\r\n\r\n"),
        pszImportOrExport,
        psz,
        Version.ProductVersion);
    else
        _stprintf(pszComments,
        _T(";      Monitoring file generated for %s table of %s by MonitoringFileBuilder\r\n\r\n"),
        pszImportOrExport,
        psz);
    WriteFile(this->hOutputFile,pszComments,(DWORD)_tcslen(pszComments)*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteFuncDefWithFirstBytesAnalysisUpdate
// Object: Write content of this->pszFuncDefinition in this->hOutputFile, 
//          updating if necessary FirstBytesAnalysis
// Parameters :
//     in  : CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult : checking result of the function
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::WriteFuncDefWithFirstBytesAnalysis(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult)
{
    FUNCTION_DISABLE_STATE OldState=FUNCTION_DISABLE_STATE_ENABLE;
    TCHAR* pszNotDisabledLine;

    // search for a disable state
    pszNotDisabledLine=this->pszFuncDefinition;
    while((*pszNotDisabledLine==';')||(*pszNotDisabledLine=='!')||(*pszNotDisabledLine==' '))
    {
        if (*pszNotDisabledLine==';')
            OldState=FUNCTION_DISABLE_STATE_DISABLED;
        if (*pszNotDisabledLine=='!')
            OldState=FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED;
        pszNotDisabledLine++;
    }
    // if a disable state has been found, remove it
    if (pszNotDisabledLine!=this->pszFuncDefinition)
        _tcscpy(this->pszFuncDefinition,pszNotDisabledLine);

    this->WriteFuncDefWithFirstBytesAnalysis(pFunctionCheckResult,OldState);
}

//-----------------------------------------------------------------------------
// Name: WriteFuncDefWithFirstBytesAnalysisUpdate
// Object: Write content of this->pszFuncDefinition in this->hOutputFile, 
//          updating if necessary FirstBytesAnalysis
// Parameters :
//     in  : CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult : checking result of the function
//           FUNCTION_DISABLE_STATE OldFunctionDisableState : old disable state to translate
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::WriteFuncDefWithFirstBytesAnalysis(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult,
                                                                FUNCTION_DISABLE_STATE OldFunctionDisableState)
{
    DWORD dwWrittenBytes;
    FUNCTION_DISABLE_STATE CurrentState=FUNCTION_DISABLE_STATE_ENABLE;

    // remove old "|FirstBytesCanExecuteAnywhere" or "|FirstBytesCanExecuteAnywhere=" option
    this->RemoveFirstByteCanExecuteAnywhereOptions(this->pszFuncDefinition);

    // analyze first bytes of function
    if (pFunctionCheckResult)
    {
        if (!pFunctionCheckResult->FunctionFirstByteCheckResult.FunctionInExecutableSection)
        {
            // disable hook by default
            WriteFile(this->hOutputFile,_T(";"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
            CurrentState=FUNCTION_DISABLE_STATE_DISABLED;
        }
        else if (!pFunctionCheckResult->ExportTableCheckedOk)
        {
            // disable hook by default
            WriteFile(this->hOutputFile,_T(";"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
            CurrentState=FUNCTION_DISABLE_STATE_DISABLED;
        }
        else
        {
            switch(pFunctionCheckResult->FunctionFirstByteCheckResult.CheckResult)
            {
            case CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE:
                // temporary disable hook by default (monitoring wizard can re-enable it)
                WriteFile(this->hOutputFile,_T("!"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
                CurrentState=FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED;
                break;
            case CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE:
                // disable hook by default
                WriteFile(this->hOutputFile,_T(";"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
                CurrentState=FUNCTION_DISABLE_STATE_DISABLED;
                break;
            default:
                if (pFunctionCheckResult->HasAnotherFunctionAlreadyTheSameAddress)
                {
                    // temporary disable hook by default (monitoring wizard can re-enable it)
                    WriteFile(this->hOutputFile,_T("!"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
                    CurrentState=FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED;
                }
                break;
            }
        }

        if (CurrentState==FUNCTION_DISABLE_STATE_ENABLE)
        {
            // forward old state
            switch(OldFunctionDisableState)
            {
            case FUNCTION_DISABLE_STATE_DISABLED:
                WriteFile(this->hOutputFile,_T(";"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
                break;
            case FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED:
                WriteFile(this->hOutputFile,_T("!"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
                break;
            }
            if (OldFunctionDisableState!=FUNCTION_DISABLE_STATE_ENABLE)
            {
                TCHAR pszMsg[2*MAX_PATH];
                _stprintf(pszMsg,_T("Function %s disabled state translated even if it seems to be hookable\r\n"),this->tFunctionName);
                this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
            }
        }

        // write function definition
        WriteFile(this->hOutputFile,this->pszFuncDefinition,(DWORD)_tcslen(this->pszFuncDefinition)*sizeof(TCHAR),&dwWrittenBytes,NULL);

        if (this->bGenerateFirstBytesAnalysisInformation)
        {
            // if function can be hooked
            if ((pFunctionCheckResult->ExportTableCheckedOk==TRUE)
                &&(pFunctionCheckResult->FunctionFirstByteCheckResult.CheckResult!=CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE)
                )
                this->AddFirstByteCanExecuteAnywhereOptionsToFile(&pFunctionCheckResult->FunctionFirstByteCheckResult);
        }
    }
    else
        // write function definition
        WriteFile(this->hOutputFile,this->pszFuncDefinition,(DWORD)_tcslen(this->pszFuncDefinition)*sizeof(TCHAR),&dwWrittenBytes,NULL);

    // new line
    WriteFile(this->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: GenerateFunctionMonitoring
// Object: write a monitoring line in the monitoring file for the specified functions
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::GenerateFunctionMonitoring(TCHAR* ImportingModuleName,CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult)
{
    CAtlRegExp<CAtlRECharTraitsA> RegExpr;
    CAtlREMatchContext<CAtlRECharTraitsA> MatchContext;
    const CAtlREMatchContext<CAtlRECharTraitsA>::RECHAR* pszMatchContextStart;
    const CAtlREMatchContext<CAtlRECharTraitsA>::RECHAR* pszMatchContextEnd;

    char pszLibName[MAX_PATH];
    char pszDllFuncName[MAX_PATH];

    BOOL bIsExecutable=TRUE;
    if (pFunctionCheckResult)
        bIsExecutable=pFunctionCheckResult->FunctionFirstByteCheckResult.FunctionInExecutableSection;

    char  UnDecoratedFuncName[2*MAX_PATH];
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR* tpsz;
#endif
    BOOL   bSuccessfullyUndecorated;
    DWORD  StackSize=0;
    BOOL   bCppDecoratedFunc;
    BOOL bCdeclOrUnknownCalllingConvention;
    DWORD dwWrittenBytes;
    char* psz;
    TCHAR* pszDecoratedFunctionCallingConvention = _T("");
    TCHAR* pszParameters=NULL;
    TCHAR* pszReturn=NULL;

    TCHAR ForwardedFunctionName[MAX_PATH];
    TCHAR ForwardedLibraryName[MAX_PATH];
    TCHAR* pRealLibraryName;
    TCHAR* pRealFunctionName;
    BOOL bFuncParametersFound;
    BOOL bDescriptionFound;

	pRealLibraryName = 0;
	pRealFunctionName = 0;
	if (pFunctionCheckResult)
	{
		if (pFunctionCheckResult->ForwardedFunction)
		{
			CHookAvailabilityCheck::DecodeForwardedName(pFunctionCheckResult->ForwardedFunctionName,ForwardedLibraryName,ForwardedFunctionName);
			CHookAvailabilityCheck::GetLastChildForwardedName(ImportingModuleName,this->tWorkingDir,this->tDllName,ForwardedLibraryName,ForwardedFunctionName);
			pRealLibraryName = ForwardedLibraryName;
			pRealFunctionName = ForwardedFunctionName;
		}
	}
    if ( (pRealLibraryName == 0) || (pRealFunctionName == 0) )
    {
        pRealLibraryName = this->tDllName;
        pRealFunctionName = this->tFunctionName;
    }

    // convert lib and func name to ascii
#if (defined(UNICODE)||defined(_UNICODE))
    CAnsiUnicodeConvert::UnicodeToAnsi(pRealLibraryName,&psz);
    strcpy(pszLibName,psz);
    free(psz);

    CAnsiUnicodeConvert::UnicodeToAnsi(pRealFunctionName,&psz);
    strcpy(pszDllFuncName,psz);
    free(psz);
#else
    strcpy(pszLibName,pRealLibraryName);
    strcpy(pszDllFuncName,pRealFunctionName);
#endif

    if (*pRealFunctionName==0)// if ordinal function only
    {
        BOOL bUnknownNumberOfArgs=TRUE;
        // if find args by call
        if (this->TryToCall && bIsExecutable)
        {
            this->ReportUserMessageInfo(_T("Calling function to find number of arguments\r\n"),USER_MESSAGE_INFORMATION);
            if (this->FindStackSizeByCall(pRealLibraryName,pRealFunctionName,&StackSize))
            {
                // if (StackSize==0) cdecl convention or no args func
                if (StackSize||this->bMicrosoftDll) // Microsoft dll are quite never in cdecl --> if empty stack size suppose it's a no args func
                    bUnknownNumberOfArgs=FALSE;
            }
        }
        if (bUnknownNumberOfArgs)
        {
            // we know absolutely nothing -> signal it
            WriteFile(this->hOutputFile,_T("\r\n; Unknown number of parameters\r\n"),34*sizeof(TCHAR),&dwWrittenBytes,NULL);

            // put "()" in pszParameters
            pszParameters=new TCHAR[3];
            _tcscpy(pszParameters,_T("()"));
        }
        else
        {
            // compute number of parameters from stack reserved size
            DWORD NbParams=StackSize/sizeof(PBYTE);
            if (NbParams==0)
            {
                // put "()" in pszParameters
                pszParameters=new TCHAR[3];
                _tcscpy(pszParameters,_T("()"));
            }
            else
            {
                // add all param with the unknown type

                // alloc _tcslen(_T("("))+_tcslen(_T("UNKNOWN"))*NbParams+_tcslen(_T(","))*(NbParams-1)+_tcslen(_T("("))+\0
                pszParameters=new TCHAR[(2+NbParams*8)*sizeof(TCHAR)];

                // add (
                _tcscpy(pszParameters,_T("("));

                // for each param
                for(DWORD cnt=0;cnt<NbParams;cnt++)
                {
                    if (cnt)
                        _tcscat(pszParameters,_T(","));
                    _tcscat(pszParameters,_T("UNKNOWN"));
                }

                // add )
                _tcscat(pszParameters,_T(")"));
            }
        }

        // forge DLL_ORDINAL@0xthis->FunctionOrdinal@this->tDllName|0xthis->FunctionOrdinal@this->tDllName(UNKNOWN,UNKNOWN,...)
        _stprintf(this->pszFuncDefinition,_T("DLL_ORDINAL@0x%.4X@%s|0x%X@%s%s"),
                                            this->FunctionOrdinal,
                                            this->tDllName,
                                            this->FunctionOrdinal,
                                            this->tDllName,
                                            pszParameters);

        delete[] pszParameters;

        this->WriteFuncDefWithFirstBytesAnalysis(pFunctionCheckResult);
        return TRUE;
    }



    // if we are updating a monitoring file, 
    // try to find description in old monitoring file first 
    if (this->Update)
    {
        bDescriptionFound=this->FindFuncDescriptionInMonitoringFile(this->pszOldMonitoringFile,pRealLibraryName,pRealFunctionName);

        if (bDescriptionFound)
        {
            this->ReportUserMessageInfo(_T("Definition successfully retrieved from old monitoring file\r\n"),USER_MESSAGE_INFORMATION);
            // write func definition with first bytes analysis update 
            this->WriteFuncDefWithFirstBytesAnalysis(pFunctionCheckResult);
            return TRUE;
        }
    }

    //Language, Calling Convention        Name in .OBJ file       void func (void)

    //C, __cdecl                          _name                   _func
    //C, __stdcall                        _name@nn                _func@0
    //C, __fastcall                       @name@nn                @func@0
    //C++, __cdecl                        ?name@@decoration       ?func@@YAXXZ
    //C++, __stdcall                      ?name@@decoration       ?func@@YGXXZ
    //C++, __fastcall                     ?name@@decoration       ?func@@YIXXZ
    //C++, __thiscall                     ??0name@@decoration

    bCppDecoratedFunc=FALSE;
    bCdeclOrUnknownCalllingConvention=FALSE;
    bSuccessfullyUndecorated=FALSE;
    bFuncParametersFound=FALSE;
    pszMatchContextStart=0;
    pszMatchContextEnd=0;

    // if C, __stdcall
    RegExpr.Parse( "^_{[^@]+}@{([0-9]+)}$" );
    if (RegExpr.Match(pszDllFuncName,&MatchContext))
    {
        MatchContext.GetMatch(0, &pszMatchContextStart, &pszMatchContextEnd);
        memcpy(UnDecoratedFuncName,pszMatchContextStart,(pszMatchContextEnd-pszMatchContextStart)*sizeof(TCHAR));
        UnDecoratedFuncName[pszMatchContextEnd-pszMatchContextStart]=0;
        MatchContext.GetMatch(1, &pszMatchContextStart, &pszMatchContextEnd);
        StackSize=atoi(pszMatchContextStart);
        pszDecoratedFunctionCallingConvention = _T("__stdcall ");
    }

    else
    {
        // if C, __fastcall
        RegExpr.Parse( "^@{[^@]+}@{([0-9]+)}$" );
        if (RegExpr.Match(pszDllFuncName,&MatchContext))
        {
            MatchContext.GetMatch(0, &pszMatchContextStart, &pszMatchContextEnd);
            memcpy(UnDecoratedFuncName,pszMatchContextStart,(pszMatchContextEnd-pszMatchContextStart)*sizeof(TCHAR));
            UnDecoratedFuncName[pszMatchContextEnd-pszMatchContextStart]=0;
            MatchContext.GetMatch(1, &pszMatchContextStart, &pszMatchContextEnd);
            StackSize=atoi(pszMatchContextStart);
            pszDecoratedFunctionCallingConvention = _T("__fastcall ");
        }
        else
        {
            // if C++
            RegExpr.Parse( "^\?+{[^@]+}@+.*$" );
            if (RegExpr.Match(pszDllFuncName,&MatchContext))
            {
                bCppDecoratedFunc=TRUE;
            }
            else // C, cdecl or unknown calling convention
            {
                bCdeclOrUnknownCalllingConvention=TRUE;
                StackSize=0;
                strcpy(UnDecoratedFuncName,pszDllFuncName);
            }
        }
    }


    if (bCppDecoratedFunc)
    {
        
        // write comment for c++ call undecorated func name and params + (this pointer put is ECX)
        if (UnDecorateSymbolName(pszDllFuncName,UnDecoratedFuncName,2*MAX_PATH,UNDNAME_COMPLETE))
        {
            bSuccessfullyUndecorated=TRUE;
            // ;
            WriteFile(this->hOutputFile,_T("\r\n;"),3*sizeof(TCHAR),&dwWrittenBytes,NULL);
            // fully undecorated func name and params
#if (defined(UNICODE)||defined(_UNICODE))
            TCHAR* pszUnicode;
            CAnsiUnicodeConvert::AnsiToUnicode(UnDecoratedFuncName,&pszUnicode);
            WriteFile(this->hOutputFile,pszUnicode,(DWORD)_tcslen(pszUnicode)*sizeof(TCHAR),&dwWrittenBytes,NULL);
            free(pszUnicode);
#else
            WriteFile(this->hOutputFile,UnDecoratedFuncName,(DWORD)_tcslen(UnDecoratedFuncName)*sizeof(TCHAR),&dwWrittenBytes,NULL);
#endif
            // new line
            WriteFile(this->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);

            // find calling convention
            if (strstr(UnDecoratedFuncName, " __cdecl "))
                pszDecoratedFunctionCallingConvention = _T("__cdecl ");
            else if (strstr(UnDecoratedFuncName, " __stdcall "))
                pszDecoratedFunctionCallingConvention = _T("__stdcall ");
            else if (strstr(UnDecoratedFuncName, " __fastcall "))
                pszDecoratedFunctionCallingConvention = _T("__fastcall ");
            else if (strstr(UnDecoratedFuncName, " __thiscall "))
                pszDecoratedFunctionCallingConvention = _T("__thiscall ");
        }
    }

    // find parameters for a non cpp func and put them in pszParameters
    if (!bCppDecoratedFunc)
    {
        // if import table is parsed
        if (bImportTable)
        {
            // check if func is already defined in monitoring file folder
            if (this->FindFuncDescriptionInMonitoringFileFolder(pRealLibraryName,pRealFunctionName))
            {
                // we get full function definition in pszFuncDefinition
                // write func definition with first bytes analysis update 
                this->WriteFuncDefWithFirstBytesAnalysis(pFunctionCheckResult);

                return TRUE;
            }
        }

        if (!bFuncParametersFound)
        {
            // if microsoft dll and user want to use online msdn, try to retrieve parameters from MSDN
            if (this->bMicrosoftDll&&this->OnlineMSDN)
            {
                // if we successfully retrieve parameters from online msdn
                if (this->pOnlineMSDNSearch->Search(pszLibName,UnDecoratedFuncName,&pszParameters,&pszReturn))
                    bFuncParametersFound=TRUE;
            }

            // if function not already defined,or not a Microsoft one, or Microsoft one and online msdn retrieval error
            if (!bFuncParametersFound)
            {
                // if cdecl like calling convention
                if (bCdeclOrUnknownCalllingConvention)
                {
                    // if find args by call
                    if (this->TryToCall && bIsExecutable)
                    {
                        this->ReportUserMessageInfo(_T("Calling function to find number of arguments\r\n"),USER_MESSAGE_INFORMATION);
                        if (this->FindStackSizeByCall(pRealLibraryName,pRealFunctionName,&StackSize))
                        {
                            // if (StackSize==0) cdecl convention or no args func
                            if (StackSize||this->bMicrosoftDll) // Microsoft dll are quite never in cdecl --> if empty stack size suppose it's a no args func
                                bCdeclOrUnknownCalllingConvention=FALSE;
                        }
                    }
                }
                if (bCdeclOrUnknownCalllingConvention)
                {
                    // we know absolutely nothing -> signal it
                    WriteFile(this->hOutputFile,_T("\r\n; Unknown number of parameters\r\n"),34*sizeof(TCHAR),&dwWrittenBytes,NULL);

                    // put "()" in pszParameters
                    pszParameters=new TCHAR[3];
                    _tcscpy(pszParameters,_T("()"));
                }
                else
                {
                    // compute number of parameters from stack reserved size
                    DWORD NbParams=StackSize/sizeof(PBYTE);
                    if (NbParams==0)
                    {
                        // put "()" in pszParameters
                        pszParameters=new TCHAR[3];
                        _tcscpy(pszParameters,_T("()"));
                    }
                    else
                    {
                        // add all param with the unknown type

                        // alloc _tcslen(_T("("))+_tcslen(_T("UNKNOWN"))*NbParams+_tcslen(_T(","))*(NbParams-1)+_tcslen(_T("("))+\0
                        pszParameters=new TCHAR[(2+NbParams*8)*sizeof(TCHAR)];

                        // add (
                        _tcscpy(pszParameters,_T("("));

                        // for each param
                        for(DWORD cnt=0;cnt<NbParams;cnt++)
                        {
                            if (cnt)
                                _tcscat(pszParameters,_T(","));
                            _tcscat(pszParameters,_T("UNKNOWN"));
                        }

                        // add )
                        _tcscat(pszParameters,_T(")"));
                    } // NbParams !=0
                }// !bCdeclOrUnknownCalllingConvention
            }// !bFuncParametersFound second
        }// !bFuncParametersFound first
    }// if (!bCppDecoratedFunc)


    // forge libname|return [calling convention] funcname(Type1,Type2)

    // copy dll name
    _tcscpy(this->pszFuncDefinition,this->tDllName);
    // add "|"
    _tcscat(this->pszFuncDefinition,_T("|"));
    // add return 
    if (pszReturn)
    {
        // add return type
        _tcscat(this->pszFuncDefinition,pszReturn);

        // add space
        if (pszReturn[_tcslen(pszReturn)-1]!=' ')
            _tcscat(this->pszFuncDefinition,_T(" "));

        // free pszReturn as it's non more use
        delete pszReturn;
        pszReturn=NULL;
    }

    // add calling convention if decorated function
    _tcscat(this->pszFuncDefinition,pszDecoratedFunctionCallingConvention);
    
    // add decorated function name
    _tcscat(this->pszFuncDefinition,pRealFunctionName);

    // add arguments
    if (bCppDecoratedFunc)
    {
        if (bSuccessfullyUndecorated)
        {
            psz=strchr(UnDecoratedFuncName,'(');
            if (psz)
            {
#if(defined(UNICODE)||defined(_UNICODE))
                CAnsiUnicodeConvert::AnsiToUnicode(psz,&tpsz);
                _tcscat(this->pszFuncDefinition,tpsz);
                free(tpsz);
#else
                _tcscat(this->pszFuncDefinition,psz);
#endif
            }
            else
                _tcscat(this->pszFuncDefinition,_T("()"));
        }
        else
            _tcscat(this->pszFuncDefinition,_T("()"));

        // add "|DisplayName=UndecoratedFunctionName"
        if (UnDecorateSymbolName(pszDllFuncName,UnDecoratedFuncName,2*MAX_PATH,UNDNAME_NAME_ONLY))
        {
            _tcscat(this->pszFuncDefinition,_T("|"));
            _tcscat(this->pszFuncDefinition,OPTION_DISPLAY_NAME);
#if(defined(UNICODE)||defined(_UNICODE))
            CAnsiUnicodeConvert::AnsiToUnicode(UnDecoratedFuncName,&tpsz);
            _tcscat(this->pszFuncDefinition,tpsz);
            free(tpsz);
#else
            _tcscat(this->pszFuncDefinition,UnDecoratedFuncName);
#endif
        }
    }
    else
    {
        if (pszParameters)
        {
            // add pszParameters
            _tcscat(this->pszFuncDefinition,pszParameters);
            delete[] pszParameters;
            pszParameters=NULL;
        }
    }

    this->WriteFuncDefWithFirstBytesAnalysis(pFunctionCheckResult);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsMicrosoftLibrary
// Object: check if library is a microsoft one
// Parameters :
//     in  : TCHAR* FileName : name of library
//     out :
//     return : TRUE if microsoft one, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::IsMicrosoftLibrary(TCHAR* FileName)
{
    CVersion Version;
    if(Version.Read(FileName))
    {
        _tcslwr(Version.CompanyName);
        // if Microsoft file
        if (_tcsstr(Version.CompanyName,_T("microsoft")))
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: FindFuncDescriptionInMonitoringFileFolder
// Object: search function description in the monitoring folder inside pszLibName.txt monitoring file
//          on success, func full definition is put in pszFuncDefinition member
// Parameters :
//     in  : TCHAR* pszLibName : library name
//           TCHAR* pszDecoratedFuncName : decorated function name
//     out :
//     return : TRUE if found, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::FindFuncDescriptionInMonitoringFileFolder(TCHAR* pszLibName,TCHAR* pszDecoratedFuncName)
{
    if (!pszLibName)
        return FALSE;
    // search for a libname.txt file like user32.txt, kernel32.txt
    TCHAR pszLibNameLocal[MAX_PATH];
    TCHAR pszMonitoringFileName[MAX_PATH];
    TCHAR* psz;
    HANDLE hFile;

    // make a local copy to avoid changing pszLibName content
    _tcscpy(pszLibNameLocal,pszLibName);

    // put in lower case for comparison
    _tcslwr(pszLibNameLocal);

    // get dll short name
    psz=_tcsrchr(pszLibNameLocal,'\\');
    if (psz)
    {
        // point after last '\'
        _tcscpy(pszLibNameLocal,psz+1);
    }

    // replace .dll extension by .txt
    psz=_tcsrchr(pszLibNameLocal,'.');
    if (psz)
    {
        psz++;
        *psz=0;
    }
    _tcscat(pszLibNameLocal,_T("txt"));

    // try to open "monitoring files/LibName.txt"

    // make full path name
    _tcscpy(pszMonitoringFileName,this->CurrentDirectory);
    _tcscat(pszMonitoringFileName,MONITORING_FILES_PATH);
    _tcscat(pszMonitoringFileName,pszLibNameLocal);


    // check if search named is not the current written file (example foo.exe importing foo.dll both monitoring files will be autonamed foo.txt)
    if (_tcsicmp(this->tOutputFileName,pszMonitoringFileName)==0)
    {
        return FALSE;
    }

    TCHAR pszMsg[4*MAX_PATH];
    _stprintf(pszMsg,_T("Searching definition for function %s (module %s) in %s...\r\n"),
                    pszDecoratedFuncName,
                    pszLibName,
                    pszMonitoringFileName);
    this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);


    // check if file exists locally to avoid an error msgbox
    hFile = CreateFile(pszMonitoringFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        this->ReportUserMessageInfo(_T("Not Found\r\n"),USER_MESSAGE_INFORMATION);
        return FALSE;
    }
    CloseHandle(hFile);

    return this->FindFuncDescriptionInMonitoringFile(pszMonitoringFileName,pszLibName,pszDecoratedFuncName);
}

//-----------------------------------------------------------------------------
// Name: FindFuncDescriptionInMonitoringFile
// Object: search function description in the monitoring file pszMonitoringFileName
//          on success, func full definition is put in pszFuncDefinition member
// Parameters :
//     in  : TCHAR* pszMonitoringFileName : monitoring file name in which to search
//           TCHAR* pszLibName : libname to search
//           TCHAR* pszDecoratedFuncName : decorated func name to search
//     out :
//     return : TRUE if found, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::FindFuncDescriptionInMonitoringFile(TCHAR* pszMonitoringFileName,TCHAR* pszLibName,TCHAR* pszDecoratedFuncName)
{
    // create cancel event
    this->hMonitoringFileParsingCancelEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    this->bFuncDefinitionFoundInMonitoringFiles=FALSE;

    this->pszSearchedFunc=pszDecoratedFuncName;
    this->pszSearchedLib=pszLibName;
    this->pszSearchedLibFileName = _tcsdup(CStdFileOperations::GetFileName(pszLibName));
    // put pszSearchedLib in uppercase to make non case sensitive search
    _tcsupr(this->pszSearchedLibFileName);

    CTextFile::ParseLines(pszMonitoringFileName,this->hMonitoringFileParsingCancelEvent,
                          CMonitoringFileBuilder::MonitoringFileLineStaticCallBack,this);

    free(this->pszSearchedLibFileName);
    CloseHandle(this->hMonitoringFileParsingCancelEvent);

    if (this->bFuncDefinitionFoundInMonitoringFiles)
        this->ReportUserMessageInfo(_T("Successfully Found\r\n"),USER_MESSAGE_INFORMATION);
    else
        this->ReportUserMessageInfo(_T("Not Found\r\n"),USER_MESSAGE_INFORMATION);

    return this->bFuncDefinitionFoundInMonitoringFiles;
}

//-----------------------------------------------------------------------------
// Name: MonitoringFileLineStaticCallBack
// Object: callback for each line read for CTextFile::ParseLines method
// Parameters :
//     in  : TCHAR* Line : line content
//           DWORD dwLineNumber : line number
//           LPVOID UserParam : associated CMonitoringFileBuilder object
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::MonitoringFileLineStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    ((CMonitoringFileBuilder*)UserParam)->MonitoringFileLineCallBack(FileName,Line,dwLineNumber);
    return TRUE;
}

void CMonitoringFileBuilder::MonitoringFileLineCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(dwLineNumber);
    TCHAR* psz;
    TCHAR* psz2;

    CTrimString::TrimString(Line);

    // if empty line
    if (*Line==0)
        // continue parsing
        return;

    // search even in commented line in case of disabled functions. 
    // This avoid to take definition again in msdn and translate the disabled function to generated file

    // search |
    psz=_tcschr(Line,'|');
    // if not found continue file parsing
    if (!psz)
        return;

    // copy Line before modifying it (in case it's the good one)
    // don't use _tcsncpy(this->pszFuncDefinition,Line,MAX_CONFIG_FILE_LINE_SIZE-1); as new lib path may differ
    TCHAR* pszNewFuncDef = this->pszFuncDefinition;
    // if disabled description
    if ( (*Line == ';') || (*Line == '!') )
    {
        // translate state
        *pszNewFuncDef = *Line;
        pszNewFuncDef++;
    }
    _sntprintf(pszNewFuncDef,MAX_CONFIG_FILE_LINE_SIZE-2,_T("%s%s"),this->pszSearchedLib,psz);
    pszNewFuncDef[MAX_CONFIG_FILE_LINE_SIZE-2]=0;


    /////////////////
    // check lib name
    /////////////////
    *psz=0;
    // put line in uppercase for non case sensitive compare
    _tcsupr(Line);
    // search lib name
    psz2=_tcsstr(Line,this->pszSearchedLibFileName);
    // if lib name not found
    if (!psz2)
        return;

    // if lib name is found, assume that is the correct lib name
    // at this point we could search for kernel32.dll and Line contains mykernel32.dll
    // so assume that there's no character before psz2 or 
    // previous character is ' ' or ';' or '!' (monitoring file supported syntax)
    // or previous character is path delimiter '/' or '\\'
    if (psz2>Line) // if not first char
    {
        // get previous char
        psz2--;
        // check previous char
        if ((*psz2!=' ')&&(*psz2!=';')&&(*psz2!='!')&&(*psz2!='/')&&(*psz2!='\\'))
            return;
    }

    /////////////////
    // search function name
    /////////////////

    // point after | --> remove library name
    Line=psz+1;

    // search (
    psz=_tcschr(Line,'(');
    // if not found continue file parsing
    if (!psz)
        return;

    // ends Line string
    *psz=0;

    // remove space between func name and (
    CTrimString::TrimString(Line);


    // split function name from any keyword (return type in function name if any or WINAPI, __cdecl)
    // in case of template we can have 
    // "public: __thiscall std::allocator<class CGenericQuickWayToFolderObject *>::allocator<class CGenericQuickWayToFolderObject *>"
    // "public: class std::vector<class CFolderMenu::CFolderPopUpMenuInfos * _Off,class std::allocator<class CFolderMenu::CFolderPopUpMenuInfos *> >::iterator __thiscall std::vector<class CFolderMenu::CFolderPopUpMenuInfos *,class std::allocator<class CFolderMenu::CFolderPopUpMenuInfos *> >::iterator::operator+(int)const"

    // in case of template
    if (_tcschr(Line,'<'))
    {
        int Depth=0;
        psz = NULL;
        int Index=(int)(_tcslen(Line)-1);

        for (int Cnt=Index;Cnt>=0;Cnt--)
        {
            switch(Line[Cnt])
            {
            case '>':
                Depth++;
                break;
            case '<':
                Depth--;
                break;
            case ' ':
                if (Depth==0)
                {
                    // point to first space not in template < >
                    psz=Line+Cnt;

                    // go out of for
                    Cnt=0;
                }
                break;
            }
        }
    }
    else
    {
        // search last space in function name
        psz=_tcsrchr(Line,' ');
    }

    // if we found it
    if (psz)
        // func name is after this space
        Line=psz+1;
    // else Line already points to func name

    // at this point we have assume that Line only contains func name 
    // so just compare Line and searched func name
    if (_tcscmp(this->pszSearchedFunc,Line)==0)
    {
        // if match, cancel reading
        this->bFuncDefinitionFoundInMonitoringFiles=TRUE;
        SetEvent(this->hMonitoringFileParsingCancelEvent);
    }
}




//-----------------------------------------------------------------------------
// Name: FunctionAnalysisAndGeneration
// Object:  Display function analysis result and generate information
// Parameters :
//     in  : TCHAR* FunctionName : Name of the function
//           CLinkList* pLinkListExportResult : Link list of an export table checking
//           BOOL bDoFirstBytesAnalysis : TRUE to do first bytes analysis (if not already done)
//           BOOL bGenerateFunctionMonitoring : TRUE to generate function description
//           BOOL bWriteReportToFile : TRUE to write report to output file
//     out : CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pCheckExportResult
//             the export check result associated to the function
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::FunctionAnalysisAndGeneration(TCHAR* FunctionName,
                                                           CLinkList* pLinkListExportResult,
                                                           BOOL bDoFirstBytesAnalysis,
                                                           BOOL bGenerateFunctionMonitoring,
                                                           BOOL bWriteReportToFile,
                                                           OUT CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pCheckExportResult)
{
    CLinkListItem* pItemExportResult;

    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pTmpCheckExportResult;
    // find function result in LinkListExportResult list
    
    for(pItemExportResult=pLinkListExportResult->Head;pItemExportResult;pItemExportResult=pItemExportResult->NextItem)
    {
        pTmpCheckExportResult=(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT*)pItemExportResult->ItemData;

        if (_tcscmp(FunctionName,pTmpCheckExportResult->FunctionName)==0)
        {
            // do first bytes checking now in case of import table and update the LinkListExportResult item data
            if (bDoFirstBytesAnalysis)
            {
                if (*FunctionName==0)// if ordinal function only
                    CHookAvailabilityCheck::FullCheck(this->tWorkingDir,this->pCurrentExportPE,0,this->FunctionOrdinal,TRUE,&pTmpCheckExportResult->FunctionFirstByteCheckResult);
                else
                    CHookAvailabilityCheck::FullCheck(this->tWorkingDir,this->pCurrentExportPE,FunctionName,0,FALSE,&pTmpCheckExportResult->FunctionFirstByteCheckResult);
                    
            }

            // add report for result checking if required
            if (this->bCheckHookAvaibility||this->bGenerateFirstBytesAnalysisInformation)
                this->ReportCheckResult(pItemExportResult,bWriteReportToFile);

            if (bGenerateFunctionMonitoring)
            {
                TCHAR CurrentFileName[MAX_PATH];
                this->pCurrentExportPE->GetFileName(CurrentFileName);
                this->GenerateFunctionMonitoring(CurrentFileName,pTmpCheckExportResult);
            }

            // copy check result (doing it only now to allow to change data into LinkListExportResult item)
            memcpy(pCheckExportResult,pTmpCheckExportResult,sizeof(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT));

            return TRUE;
        }
    }

        
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: ReportCheckResult
// Object:  report potential warning for an item according to the export table check
// Parameters :
//     in  : CLinkListItem* pExportTableResultItem : Item of an ExportTableResult list
//           BOOL bWriteReportToFile : TRUE if you want to write report to monitoring file
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::ReportCheckResult(CLinkListItem* pExportTableResultItem,BOOL bWriteReportToFile)
{
    TCHAR pszMsg[5*MAX_PATH];
    TCHAR pszMsg2[MAX_PATH];
    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pExportResult;
    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pExportResult2;
    CLinkListItem* pItem;
    DWORD dwWrittenBytes=0;


    // get associated structure
    pExportResult=(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT*)pExportTableResultItem->ItemData;

    // if forwarded function
    if (pExportResult->ForwardedFunction)
    {
        TCHAR ForwardedDllName[MAX_PATH];
        TCHAR ForwardedFunctionName[MAX_PATH];

        if (CHookAvailabilityCheck::DecodeForwardedName(pExportResult->ForwardedFunctionName,ForwardedDllName,ForwardedFunctionName))
        {
            CHookAvailabilityCheck::GetLastChildForwardedName(NULL,this->tWorkingDir,this->tDllName,ForwardedDllName,ForwardedFunctionName);
            if (bWriteReportToFile)
            {
                // write to file
                _stprintf(pszMsg,_T("\r\n;%s() in %s is a forwarded function for %s() in %s\r\n"),
                            this->tFunctionName,
                            this->tDllName,
                            ForwardedFunctionName,
                            ForwardedDllName);
            
                WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);
            }
            // signal it to user
            _stprintf(pszMsg,_T("Function %s is a forwarded function for %s() in %s\r\n"),
                            this->tFunctionName,
                            ForwardedFunctionName,
                            ForwardedDllName);

            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
        }
    }

    // if multiples functions point to the same address
    if (pExportResult->HasAnotherFunctionAlreadyTheSameAddress)
    {
        // already defined with another name
        if (bWriteReportToFile)
        {
            // write to file
            _stprintf(pszMsg,_T("\r\n;%s() in %s has the same entry point as %s\r\n"),
                    this->tFunctionName,
                    this->tDllName,
                    pExportResult->FirstFunctionWithSameAddressName);
        
            WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        }
        // signal it to user
        _stprintf(pszMsg,_T("Function %s has the same entry point as %s\r\n"),
                this->tFunctionName,
                pExportResult->FirstFunctionWithSameAddressName);

        this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
    }

    if (!pExportResult->FunctionFirstByteCheckResult.FunctionInExecutableSection)
    {
        // current func cannot be hooked
        if (bWriteReportToFile)
        {
            // write to file
            _stprintf(pszMsg,_T("\r\n;%s() in %s shouldn't be hooked: Address is not in an executable section, it can be an exported variable\r\n"),
                this->tFunctionName,
                this->tDllName);

            WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        }

        // signal it to user
        _stprintf(pszMsg,_T("Function %s can't be hooked: Address is not in an executable section, it can be an exported variable\r\n"),
            this->tFunctionName);
        this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);

    }
    // if export checking fails
    else if (!pExportResult->ExportTableCheckedOk)
    {
        
        if (pExportResult->FunctionFirstByteCheckResult.CheckResult==CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE)
        {
            _stprintf(pszMsg,_T("Export checking avoid a disasm analysis hole for function %s\r\n"),pExportResult->FunctionName);
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);
        }

        // find function that will crash 
        // as pExportTableResultItem is sorted by ascending RVA,
        // this is the next function in pExportTableResultItem with a different address
        pExportResult2=NULL;
        for(pItem=pExportTableResultItem->NextItem;pItem;pItem=pItem->NextItem)
        {
            pExportResult2=(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT*)pItem->ItemData;
            if (pExportResult2->RVA!=pExportResult->RVA)
                break;
        }
        *pszMsg2=0;
        if (pExportResult2)
            _stprintf(pszMsg2,_T(" If you try to hook it, calls to function %s will make your process crash"),pExportResult2->FunctionName);

        // current func cannot be hooked
        if (bWriteReportToFile)
        {
            // write to file
            _stprintf(pszMsg,_T("\r\n;%s() in %s can't be hooked: It's size is less than %u bytes.%s\r\n"),
                    this->tFunctionName,
                    this->tDllName,
                    HOOK_SIZE,
                    pszMsg2);

            WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);

        }

        // signal it to user
        _stprintf(pszMsg,_T("Function %s can't be hooked: It's size is less than %u bytes.%s\r\n"),
                this->tFunctionName,
                HOOK_SIZE,
                pszMsg2);
        
        this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
    }
    else // check first byte analysis result
    {
        switch(pExportResult->FunctionFirstByteCheckResult.CheckResult)
        {
        case CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE:
            if (bWriteReportToFile)
            {
                // write to file
                _stprintf(pszMsg,_T("\r\n;%s() in %s can't be hooked: It's size is less than %u bytes\r\n"),
                        this->tFunctionName,
                        this->tDllName,
                        HOOK_SIZE);

                WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);
            }

            // signal it to user
            _stprintf(pszMsg,_T("Function %s can't be hooked: It's size is less than %u bytes\r\n"),
                    this->tFunctionName,
                    HOOK_SIZE);

            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);

            break;

        case CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE:
            if (bWriteReportToFile)
            {
                // write to file
                _stprintf(pszMsg,_T("\r\n;Warning: function %s() in %s may can't be hooked: It's size seems to be less than %u bytes --> you have to do a manual checking \r\n"),
                        this->tFunctionName,
                        this->tDllName,
                        HOOK_SIZE);

                WriteFile(this->hOutputFile,pszMsg,_tcslen(pszMsg)*sizeof(TCHAR),&dwWrittenBytes,NULL);

            }
            // signal it to user
            _stprintf(pszMsg,_T("function %s() may can't be hooked: It's size seems to be less than %u bytes --> you have to do a manual checking \r\n"),
                        this->tFunctionName,
                        HOOK_SIZE);
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
        }
    }

}

//-----------------------------------------------------------------------------
// Name: AddFirstByteCanExecuteAnywhereOptionsToFile
// Object: add "|FirstBytesCanExecuteAnywhere", "|FirstBytesCanExecuteAnywhere=" or 
//             "|FirstBytesCantExecuteAnywhere" option to file
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::AddFirstByteCanExecuteAnywhereOptionsToFile(CHookAvailabilityCheck::FUNCTION_CHECK_RESULT* pFunctionCheckResult)
{
    DWORD dwWrittenBytes;

    if (pFunctionCheckResult->CheckResult==CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE)
        return;

    switch (pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.FirstBytesCanBeExecutedAnyWhereResult)
    {
    // case CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY: // too insecure
    case CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES:
        WriteFile(this->hOutputFile,_T("|"),(DWORD)sizeof(TCHAR),&dwWrittenBytes,NULL);
        WriteFile(this->hOutputFile,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE,(DWORD)_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        if (pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace!=5)
        {
            TCHAR pszNb[10];
            _stprintf(pszNb,_T("=%u"),pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace);
            WriteFile(this->hOutputFile,pszNb,(DWORD)_tcslen(pszNb)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        }
        break;
    // case CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY_NEED_RELATIVE_ADDRESS_CHANGES: // too insecure
    case CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES_NEED_RELATIVE_ADDRESS_CHANGES:
        WriteFile(this->hOutputFile,_T("|"),(DWORD)sizeof(TCHAR),&dwWrittenBytes,NULL);
        WriteFile(this->hOutputFile,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE,(DWORD)_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        if (pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace!=5)
        {
            TCHAR pszNb[10];
            _stprintf(pszNb,_T("=%u"),pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace);
            WriteFile(this->hOutputFile,pszNb,(DWORD)_tcslen(pszNb)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        }
        break;
    case CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO:
        // by default put can't option to avoid to check it when file will be loaded (as it's the same algo)
        WriteFile(this->hOutputFile,_T("|"),(DWORD)_tcslen(_T("|"))*sizeof(TCHAR),&dwWrittenBytes,NULL);
        WriteFile(this->hOutputFile,OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE,(DWORD)_tcslen(OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)*sizeof(TCHAR),&dwWrittenBytes,NULL);
        break;
    }
}


//-----------------------------------------------------------------------------
// Name: RemoveFirstByteCanExecuteAnywhereOptions
// Object: remove old "|FirstBytesCanExecuteAnywhere", "|FirstBytesCanExecuteAnywhere=" or 
//                    "|FirstBytesCantExecuteAnywhere", 
//                    _T("|FirstBytesCanExecuteAnywhereWithRelativeAddressChange"),_T("|FirstBytesCanExecuteAnywhereWithRelativeAddressChange=")
// Parameters :
//     in out : TCHAR* pszDef : function definition
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
void CMonitoringFileBuilder::RemoveFirstByteCanExecuteAnywhereOptions(TCHAR* pszDef)
{
    TCHAR* pszBegin;
    TCHAR* pszEnd;
    BOOL bFound=FALSE;

    // remove old "|FirstBytesCanExecuteAnywhere" or "|FirstBytesCanExecuteAnywhere=" option
    pszBegin=_tcsstr(pszDef,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE);
    // if not found 
    if (!pszBegin)
        // search "|FirstBytesCantExecuteAnywhere" option
        pszBegin=_tcsstr(pszDef,OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE);

    //// if not found 
    if (!pszBegin)
        // search OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE
        pszBegin=_tcsstr(pszDef,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE);

    // if a FirstBytesCanXExecuteAnywhere option has been found
    if (pszBegin)
    {
        if (pszBegin==pszDef)
            return;

        // search begin of option ( search previous '|')
        pszBegin--;
        while (pszBegin>pszDef)
        {
            if (*pszBegin=='|')
            {
                bFound=TRUE;
                break;
            }
            pszBegin--;
        }
        if(!bFound)
            return;

        // search next option
        pszEnd=_tcschr(pszBegin+1,_T('|'));
        // if found
        if (pszEnd)
        {
            // remove FirstBytesCanExecuteAnywhere option
            _tcscpy(pszBegin,pszEnd);
        }
        else
            // end string
            *pszBegin=0;
    }
}

//-----------------------------------------------------------------------------
// Name: RemoveBytesAnalysis
// Object: Remove bytes analysis from a monitoring file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::RemoveBytesAnalysis(TCHAR* MonitoringFileToUpdate,TCHAR* OutPutFile)
{
    // create monitoring file
    if (!this->CreateMonitoringFile(OutPutFile))
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    CTextFile::ParseLines(MonitoringFileToUpdate,RemoveBytesAnalysisStaticCallBack,this);

    // update the percent graph (stupid as we can't predict the number of lines in file --> directly put it to 100
    //                           anyway text parsing must be very speed)
    if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
        this->PercentCompletedCallBack(100,this->PercentCompletedCallBackUserParam);

    CloseHandle(this->hOutputFile);
    return TRUE;
}
BOOL CMonitoringFileBuilder::RemoveBytesAnalysisStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(dwLineNumber);

    DWORD dwBytesWritten=0;
    CMonitoringFileBuilder* pMonitoringFileBuilder=(CMonitoringFileBuilder*)UserParam;
    pMonitoringFileBuilder->RemoveFirstByteCanExecuteAnywhereOptions(Line);
    WriteFile(pMonitoringFileBuilder->hOutputFile,Line,_tcslen(Line)*sizeof(TCHAR),&dwBytesWritten,NULL);
    WriteFile(pMonitoringFileBuilder->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwBytesWritten,NULL);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UpdateBytesAnalysis
// Object: only update bytes analysis of a monitoring file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilder::UpdateBytesAnalysis(TCHAR* MonitoringFileToUpdate,TCHAR* OutPutFile)
{
    // create monitoring file
    if (!this->CreateMonitoringFile(OutPutFile))
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    this->bGenerateFirstBytesAnalysisInformation=TRUE;
    this->ParsedLibraryList=new CLinkList(sizeof(CHookAvailabilityCheck::LIBRARY_EXPORT_LIST));

    // parse each line of the file
    CTextFile::ParseLines(MonitoringFileToUpdate,UpdateBytesAnalysisStaticCallBack,this);

    // free memory required by forwarded functions
    CLinkListItem* pItem=this->ParsedLibraryList->Head;
    while (pItem)
    {
        delete ((CHookAvailabilityCheck::LIBRARY_EXPORT_LIST*)pItem->ItemData)->pListCheckExportTableResult;
        delete ((CHookAvailabilityCheck::LIBRARY_EXPORT_LIST*)pItem->ItemData)->pPe;
        pItem=pItem->NextItem;
    }
    delete this->ParsedLibraryList;


    // update the percent graph (stupid as we can't predict the number of lines in file --> directly put it to 100
    //                           anyway text parsing must be very speed)
    if (!IsBadCodePtr((FARPROC)this->PercentCompletedCallBack))
        this->PercentCompletedCallBack(100,this->PercentCompletedCallBackUserParam);

    CloseHandle(this->hOutputFile);
    return TRUE;
}
BOOL CMonitoringFileBuilder::UpdateBytesAnalysisStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(dwLineNumber);

    DWORD FunctionNameSize;
    DWORD Index;
    TCHAR* pszDllNameEnd;
    TCHAR* pszFuncNameEnd;
    TCHAR* pszFuncNameBegin;
    TCHAR* pszFuncNameBegin2;
    TCHAR* pszDescription;
    TCHAR* pszNotDisabledLine;
    TCHAR DllName[MAX_PATH];
    TCHAR FunctionName[MAX_PATH];
    TCHAR DllFullPath[MAX_PATH];
    TCHAR pszMsg[4*MAX_PATH];
    CHookAvailabilityCheck::LIBRARY_EXPORT_LIST LibraryExportList;
    CHookAvailabilityCheck::LIBRARY_EXPORT_LIST* pLibraryExportList=NULL;
    CLinkListItem* pItem;
    CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT ExportedFuncCheckResult;
    BOOL bFound;

    DWORD dwBytesWritten=0;
    FUNCTION_DISABLE_STATE OldState=FUNCTION_DISABLE_STATE_ENABLE;
    CMonitoringFileBuilder* pMonitoringFileBuilder=(CMonitoringFileBuilder*)UserParam;

    // search EXE_INTERNAL@ or DLL_INTERNAL@
    if (_tcsstr(Line,EXE_INTERNAL_PREFIX)||_tcsstr(Line,DLL_INTERNAL_PREFIX)||_tcsstr(Line,DLL_ORDINAL_PREFIX) ||
        _tcsstr(Line,EXE_INTERNAL_POINTER_PREFIX)||
        _tcsstr(Line,DLL_INTERNAL_POINTER_PREFIX)||
        _tcsstr(Line,EXE_INTERNAL_RVA_PREFIX)||
        _tcsstr(Line,EXE_INTERNAL_RVA_POINTER_PREFIX)
        )
    {
        pMonitoringFileBuilder->ReportUserMessageInfo(_T("Can't update EXE_INTERNAL, DLL_INTERNAL or DLL_ORDINAL (Definition translated without change).\r\n")
                                                      _T("BE CAREFUL location address has probably changed if binary is a new version."),
                                                      USER_MESSAGE_WARNING);
        // write line without changes
        WriteFile(pMonitoringFileBuilder->hOutputFile,Line,_tcslen(Line)*sizeof(TCHAR),&dwBytesWritten,NULL);
        WriteFile(pMonitoringFileBuilder->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwBytesWritten,NULL);
        // continue parsing
        return TRUE;
    }

    // search |
    pszFuncNameEnd=0;
    pszDllNameEnd=_tcschr(Line,'|');
    if (pszDllNameEnd)
        pszFuncNameEnd=_tcschr(pszDllNameEnd,'(');



    // if | or ( have not been found --> not a function definition
    if ((!pszDllNameEnd)||(!pszFuncNameEnd))
    {
        // write line without changes
        WriteFile(pMonitoringFileBuilder->hOutputFile,Line,_tcslen(Line)*sizeof(TCHAR),&dwBytesWritten,NULL);
        WriteFile(pMonitoringFileBuilder->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwBytesWritten,NULL);
        // continue parsing
        return TRUE;
    }

    // get dll and function name
    // remove ';', '!' and space starting the line
    pszNotDisabledLine=Line;
    while((*pszNotDisabledLine==';')||(*pszNotDisabledLine=='!')||(*pszNotDisabledLine==' '))
    {
        if (*pszNotDisabledLine==';')
            OldState=FUNCTION_DISABLE_STATE_DISABLED;
        if (*pszNotDisabledLine=='!')
            OldState=FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED;
        pszNotDisabledLine++;
    }

    pszDllNameEnd=_tcschr(pszNotDisabledLine,'|');
    pszFuncNameEnd=_tcschr(pszDllNameEnd,'(');
    pszFuncNameEnd--; // point before '('

    // get dll name
    _tcsncpy(DllName,pszNotDisabledLine,pszDllNameEnd-pszNotDisabledLine);
    DllName[pszDllNameEnd-pszNotDisabledLine]=0;

    if (pMonitoringFileBuilder->DllStub.IsStubDll(CStdFileOperations::GetFileName(DllName)))
    {
        TCHAR RealName[MAX_PATH];
        if (pMonitoringFileBuilder->DllStub.GetRealModuleNameFromStubName(_T(""),CStdFileOperations::GetFileName(DllName),RealName))
        {
            _tcscpy(DllName,RealName);
        }
        else
        {
            // write line without changes
            WriteFile(pMonitoringFileBuilder->hOutputFile,Line,_tcslen(Line)*sizeof(TCHAR),&dwBytesWritten,NULL);
            WriteFile(pMonitoringFileBuilder->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwBytesWritten,NULL);
            // continue parsing
            return TRUE;
        }

    }


    // remove dll Name
    pszDescription=pszDllNameEnd+1;

    // get function name
    FunctionNameSize=(DWORD)(pszFuncNameEnd-pszDllNameEnd);
    _tcsncpy(FunctionName,pszDescription,FunctionNameSize);
    FunctionName[FunctionNameSize]=0;
    // remove space between end of function name and (
    for (Index=FunctionNameSize-1;FunctionNameSize>0;Index--)
    {
        if (FunctionName[Index]==' ')
            FunctionName[Index]=0;
        else
            break;
    }
    // remove return (search for the first '*' or ' ' before the function name) 
    pszFuncNameBegin=_tcsrchr(FunctionName,'*');
    pszFuncNameBegin2=_tcsrchr(FunctionName,' ');
    if (pszFuncNameBegin2>pszFuncNameBegin)
        pszFuncNameBegin=pszFuncNameBegin2;
    // if there's a return
    if (pszFuncNameBegin)
    {
        // point after the space or * (at the begin of the function name)
        pszFuncNameBegin++;
        // remove it
        _tcscpy(FunctionName,pszFuncNameBegin);
    }

    // set working dll filename
    pMonitoringFileBuilder->SetWorkingFile(DllName);
    _tcscpy(pMonitoringFileBuilder->tFunctionName,FunctionName);

    // reset vars to default
    ExportedFuncCheckResult.ExportTableCheckedOk=TRUE;

    memset(&ExportedFuncCheckResult.FunctionFirstByteCheckResult,0,sizeof(CHookAvailabilityCheck::FUNCTION_CHECK_RESULT));
    ExportedFuncCheckResult.FunctionFirstByteCheckResult.CheckResult=CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE;
    ExportedFuncCheckResult.FunctionFirstByteCheckResult.FunctionInExecutableSection=TRUE;
    ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=FALSE;
    ExportedFuncCheckResult.HasAnotherFunctionTheSameAddress=FALSE;
    *ExportedFuncCheckResult.FirstFunctionWithSameAddressName=0;
    ExportedFuncCheckResult.RVA=0;
    _tcscpy(ExportedFuncCheckResult.FunctionName,FunctionName);
    ExportedFuncCheckResult.ForwardedFunction=FALSE;
    *ExportedFuncCheckResult.ForwardedFunctionName=0;

    // check if library has already been parsed by doing a loop in pMonitoringFileBuilder->ParsedLibraryList
    bFound=FALSE;
    
    for (pItem=pMonitoringFileBuilder->ParsedLibraryList->Head;pItem;pItem=pItem->NextItem)
    {
        pLibraryExportList=(CHookAvailabilityCheck::LIBRARY_EXPORT_LIST*)pItem->ItemData;
        // if name of library found
        if (_tcscmp(DllName,pLibraryExportList->LibraryName)==0)
        {
            // pLibraryExportList points to the correct list --> nothing to do
            // item has been found
            bFound=TRUE;
            break;
        }
    }

    // if not found in list, we have to parse the export table of the dll
    if (!bFound)
    {
        CDllFinder::FindDll(pMonitoringFileBuilder->tWorkingDir,DllName,DllFullPath);
        // create an CheckExportTableResult list
        LibraryExportList.pListCheckExportTableResult=new CLinkList(sizeof(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT));
        LibraryExportList.pPe=new CPE(DllFullPath);
        LibraryExportList.pPe->Parse(TRUE,FALSE);

        // get export result
        if (CHookAvailabilityCheck::CheckExportTable(pMonitoringFileBuilder->tWorkingDir,
                                                    LibraryExportList.pPe,
                                                    LibraryExportList.pListCheckExportTableResult,
                                                    FALSE
                                                    )
            )
        {
            // item is ok
            bFound=TRUE;
            // fill dll name of the struct
            _tcscpy(LibraryExportList.LibraryName,DllName);
            // add LibraryExportList list into pMonitoringFileBuilder->ParsedLibraryList
            pMonitoringFileBuilder->ParsedLibraryList->AddItem(&LibraryExportList);
            // fill pLibraryExportList var
            pLibraryExportList=&LibraryExportList;
        }

        else
        {
            _stprintf(pszMsg,_T("Error parsing %s."),DllName);
            pMonitoringFileBuilder->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);

            // on error free allocated memory
            delete LibraryExportList.pListCheckExportTableResult;
            delete LibraryExportList.pPe;
        }
    }

    if(!bFound)
        return TRUE;

    // do first bytes checking
    if (!CHookAvailabilityCheck::FullCheck(pMonitoringFileBuilder->tWorkingDir,pLibraryExportList->pPe,FunctionName,0,FALSE,&ExportedFuncCheckResult.FunctionFirstByteCheckResult))
    {
        _stprintf(pszMsg,_T("Error checking function %s in %s.\r\n"),FunctionName,DllName);
        pMonitoringFileBuilder->ReportUserMessageInfo(pszMsg,USER_MESSAGE_WARNING);
        _tcscat(pszMsg,_T("Do you want to keep it in new monitoring file ?"));
        if (MessageBox(pMonitoringFileBuilder->hWndParent,pszMsg,_T("Question"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDYES)
        {
            // write line without changes
            WriteFile(pMonitoringFileBuilder->hOutputFile,Line,_tcslen(Line)*sizeof(TCHAR),&dwBytesWritten,NULL);
            WriteFile(pMonitoringFileBuilder->hOutputFile,_T("\r\n"),2*sizeof(TCHAR),&dwBytesWritten,NULL);
            return TRUE;
        }
    }

    // we are in a first bytes update only, user should have choose 
    // which function he want to keep from all having the same address
    ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=FALSE;

    // fill pszFuncDefinition members of CMonitoringFileBuilder
    _tcscpy(pMonitoringFileBuilder->pszFuncDefinition,pszNotDisabledLine);
    // write function description, remove old first bytes analysis and write the new ones
    pMonitoringFileBuilder->WriteFuncDefWithFirstBytesAnalysis(&ExportedFuncCheckResult,OldState);

    return TRUE;
}

BOOL CMonitoringFileBuilder::FindStackSizeByCall(TCHAR* pszLibName,TCHAR* pszDecoratedFuncName,DWORD* pStackSize)
{
    TCHAR strCommandLine[2*MAX_PATH];
    BOOL bRet;
    _stprintf(strCommandLine,_T(" \"%s\" %s"),pszLibName,pszDecoratedFuncName);
    bRet=this->FindStackSizeByCall(strCommandLine,pStackSize);
    return bRet;
}
BOOL CMonitoringFileBuilder::FindStackSizeByCall(TCHAR* pszLibName,DWORD FunctionOrdinal,DWORD* pStackSize)
{
    TCHAR strCommandLine[2*MAX_PATH];
    BOOL bRet;
    _stprintf(strCommandLine,_T(" \"%s\" 0x%x"),pszLibName,FunctionOrdinal);
    bRet=this->FindStackSizeByCall(strCommandLine,pStackSize);
    return bRet;
}


BOOL CMonitoringFileBuilder::FindStackSizeByCall(TCHAR* CommandLine,DWORD* pStackSize)
{
    BOOL bRet=FALSE;
    DWORD WaitRet;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    memset(&ProcessInformation,0,sizeof(ProcessInformation));
    memset(&StartupInfo,0,sizeof(StartupInfo));

    // default stack size
    *pStackSize=0;

    // make full app path
    TCHAR ApplicationName[MAX_PATH];
    _tcscpy(ApplicationName,this->CurrentDirectory);
    _tcscat(ApplicationName,FindStackSizeByCall_APPLICATION_NAME);
   
    // create process to do the call (so in case of crash or ExitProcess call inside called function,
    // monitoring file builder process won't be affected)
    if (!::CreateProcess(ApplicationName,CommandLine,NULL,NULL,FALSE,NULL,NULL,NULL,&StartupInfo,&ProcessInformation))
        return FALSE;

    for(;;)
    {
        // wait until end of process during 30sec
        WaitRet=::WaitForSingleObject(ProcessInformation.hProcess,30000);

        switch(WaitRet)
        {
        case WAIT_TIMEOUT:
            // query user to wait more
            if (::MessageBox(this->hWndParent,_T("Function doesn't reply yet.\r\nDo you want to wait more ?") ,_T("Question"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDNO)
            {
                ::TerminateProcess( ProcessInformation.hProcess, UINT (-1) );
                this->ReportUserMessageInfo(_T("Blocking call cancelled by user\r\n"),USER_MESSAGE_INFORMATION);
                goto CleanUp;
            }
        	break;
        case WAIT_OBJECT_0:
            // process has ended
            {
                DWORD ExitCode=0;
                if (!::GetExitCodeProcess(ProcessInformation.hProcess,&ExitCode))
                    goto CleanUp;
                
                switch (ExitCode & CFindStackSizeByCall::SUCCESS_RESULT_MASK)
                {
                case CFindStackSizeByCall::SUCCESS:
                    // get result (see FindStackSizeByCall.h)
                    *pStackSize=ExitCode & CFindStackSizeByCall::RESULT_MASK;
                    bRet=TRUE;
                    break;

                case CFindStackSizeByCall::FAILURE:
                    // get error code (see FindStackSizeByCall.h)
                    switch (ExitCode & CFindStackSizeByCall::RESULT_MASK)
                    {
                    case CFindStackSizeByCall::ERROR_CODE_ADDRESS_IS_NOT_IN_AN_EXECUTABLE_SECTION:
                        {
                            TCHAR Msg[2*MAX_PATH];
                            _stprintf(Msg,_T("%s is not in an executable address space, so it can't be called. Maybe it's an exported variable\r\n"),CommandLine);
                            this->ReportUserMessageInfo(Msg,USER_MESSAGE_WARNING);
                        }
                        break;
                    case CFindStackSizeByCall::ERROR_CODE_HARDWARE_EXCEPTION_INSIDE_CALL:
                        this->ReportUserMessageInfo(_T("Hardware Exception inside call\r\n"),USER_MESSAGE_INFORMATION);
                        break;
                    case CFindStackSizeByCall::ERROR_CODE_SOFTWARE_EXCEPTION_INSIDE_CALL:
                        this->ReportUserMessageInfo(_T("Software Exception inside call\r\n"),USER_MESSAGE_INFORMATION);
                        break;
                    default:
                        // known failure like memory allocation error, pe parsing error -> no report
                        break;
                    }
                    
                    // ends case CFindStackSizeByCall::FAILURE:
                    break;

                default:
                    // unknown failure
                    break;
                }
            }

            // ends WAIT_OBJECT_0 (process has ended)
            goto CleanUp;

        default:
            goto CleanUp;

        }// ends WaitForSingleObject result switch

    }// ends for(;;)
    
CleanUp:
    // Close process and thread handles. 
    CloseHandle( ProcessInformation.hProcess );
    CloseHandle( ProcessInformation.hThread );

    return bRet;
}