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

#pragma once

#include "OnlineMsdnSearch.h"

#include <windows.h>
#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp")

#pragma warning (push)
#pragma warning(disable : 4018 )
#pragma warning(disable : 4389 )
#include <atlrx.h>// for regular expression
#pragma warning (pop)

#include "defines.h"
#include "../Tools/string/ansiunicodeconvert.h"
#include "../Tools/string/TrimString.h"
#include "../Tools/string/WildCharCompare.h"
#include "../Tools/File/TextFile.h"
#include "../Tools/pe/pe.h"
#include "../Tools/Version/Version.h"
#include "../Tools/Process/APIOverride/Injected_dll/defines.h"
#include "../Tools/Process/APIOverride/hookavailabilitycheck/hookavailabilitycheck.h"
#include "../Tools/Process/APIOverride/HookCom/_COMMonitoringFileGenerator/COMMonitoringFileGenerator.h"
#include "../Tools/Exception/HardwareException.h"
#include "../Tools/BinResources/BinResource.h"
#include "../Tools/Dll/DllStub.h"
#include "FunctionsSelectionUI.h"
#include "FindStackSizeByCall/FindStackSizeByCall.h"


#define MONITORING_FILES_PATH        _T("monitoring files\\") // same as winapioverride
#define COM_MONITORING_FILES_PATH    (MONITORING_FILES_PATH _T("COM\\")) // same as winapioverride
#define PROXY_FILE_NAME              _T("proxy.txt")

#define EXE_RESOURCE_SECTION_NAME    _T("EXE")
#define FindStackSizeByCall_APPLICATION_NAME _T("FindStackSizeByCall.exe")

// tag to specify internal address of software instead of a libname/funcname
#define EXE_INTERNAL_PREFIX _T("EXE_INTERNAL@0x")
#define EXE_INTERNAL_POINTER_PREFIX _T("EXE_INTERNAL_POINTER@0x")
#define EXE_INTERNAL_RVA_PREFIX _T("EXE_INTERNAL_RVA@0x")
#define EXE_INTERNAL_RVA_POINTER_PREFIX _T("EXE_INTERNAL_RVA_POINTER@0x")
// tag to specify internal address of a dll instead of an exported funcname
// allow to hook non exported function without knowing loaded bas address
#define DLL_INTERNAL_PREFIX _T("DLL_INTERNAL@0x")
#define DLL_INTERNAL_POINTER_PREFIX _T("DLL_INTERNAL_POINTER@0x")

// tag to specify ordinal exported address of a dll instead of an exported funcname
// allow to hook non exported function without knowing loaded bas address
#define DLL_ORDINAL_PREFIX _T("DLL_ORDINAL@0x")

class CMonitoringFileBuilder
{
public:
    typedef void (*tagPercentCompletedCallBack)(BYTE Percent,LPVOID UserParam);

private:
    enum FUNCTION_DISABLE_STATE
    {
        FUNCTION_DISABLE_STATE_ENABLE,
        FUNCTION_DISABLE_STATE_TEMPORARY_DISABLED,
        FUNCTION_DISABLE_STATE_DISABLED
    };

    HINSTANCE hInstance;
    HWND hWndParent;

    TCHAR CurrentDirectory[MAX_PATH];// monitoring file builder path
    TCHAR* pszFileToMonitor;
    TCHAR tWorkingDir[MAX_PATH]; // path of exe or dll for which we are generating a monitoring file 
    TCHAR tDllName[MAX_PATH];
    TCHAR tFunctionName[MAX_PATH];
    DWORD FunctionOrdinal;
    HANDLE hOutputFile;
    TCHAR tOutputFileName[MAX_PATH];
    CPE* pCurrentExportPE;

    HANDLE hCancelEvent;
    HANDLE hMonitoringFileParsingCancelEvent;
    BOOL bCancelCurrentOperation;
    BOOL bFuncDefinitionFoundInMonitoringFiles;

    BOOL Update;
    TCHAR* pszOldMonitoringFile;
    BOOL bMicrosoftDll;
    BOOL bImportTable;

    TCHAR* pszSearchedFunc;
    TCHAR* pszSearchedLib;
    TCHAR* pszSearchedLibFileName;
    TCHAR pszFuncDefinition[MAX_CONFIG_FILE_LINE_SIZE];
    COnlineMSDNSearch* pOnlineMSDNSearch;
    CDllStub DllStub;

    CLinkList* ParsedLibraryList;

    LPVOID UserMessageInformationCallBackUserParam;
    tagUserMessageInformationCallBack UserMessageInformationCallBack;
    tagPercentCompletedCallBack PercentCompletedCallBack;
    LPVOID PercentCompletedCallBackUserParam;

    static void StaticReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType,LPVOID UserParam);
    void ReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType);

    void SetWorkingFile(TCHAR* pszFile);
    BOOL IsMicrosoftLibrary(TCHAR* FileName);
    BOOL FindFuncDescriptionInMonitoringFileFolder(TCHAR* pszLibName,TCHAR* pszDecoratedFuncName);
    BOOL FindFuncDescriptionInMonitoringFile(TCHAR* pszMonitoringFileName,TCHAR* pszLibName,TCHAR* pszDecoratedFuncName);
    void MonitoringFileLineCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber);
    static BOOL MonitoringFileLineStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    BOOL FindStackSizeByCall(TCHAR* pszLibName,TCHAR* pszDecoratedFuncName,DWORD* pStackSize);
    BOOL FindStackSizeByCall(TCHAR* pszLibName,DWORD FunctionOrdinal,DWORD* pStackSize);
    BOOL FindStackSizeByCall(TCHAR* CommandLine,DWORD* pStackSize);
    BOOL GenerateFunctionMonitoring(TCHAR* ImportingModuleName,CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT*);
    
    BOOL FunctionAnalysisAndGeneration(TCHAR* FunctionName,
                                        CLinkList* pLinkListExportResult,
                                        BOOL bDoFirstBytesAnalysis,
                                        BOOL bGenerateFunctionMonitoring,
                                        BOOL bWriteReportToFile,
                                        OUT CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pCheckExportResult);

    void WriteHeader();
    void ReportCheckResult(CLinkListItem* pExportTableResultItem,BOOL CheckOnly);
    void RemoveFirstByteCanExecuteAnywhereOptions(TCHAR* pszDef);
    void AddFirstByteCanExecuteAnywhereOptionsToFile(CHookAvailabilityCheck::FUNCTION_CHECK_RESULT* pFunctionCheckResult);
    void WriteFuncDefWithFirstBytesAnalysis(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult);
    void WriteFuncDefWithFirstBytesAnalysis(CHookAvailabilityCheck::CHECK_EXPORT_TABLE_RESULT* pFunctionCheckResult,
                                            FUNCTION_DISABLE_STATE OldFunctionDisableState);
    BOOL CreateMonitoringFile(IN TCHAR* FilePath);

    static BOOL RemoveBytesAnalysisStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    static BOOL UpdateBytesAnalysisStaticCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
   
public:
    CMonitoringFileBuilder(HINSTANCE hInstance,HWND hWndParent);
    ~CMonitoringFileBuilder(void);

    BOOL bGenerateFirstBytesAnalysisInformation;
    BOOL OnlineMSDN;
    BOOL TryToCall;
    BOOL bUseProxy;
    BOOL bCheckHookAvaibility;
    TCHAR ProxyFileName[MAX_PATH];
    

    BOOL GenerateExportMonitoring(TCHAR* pszFileToMonitor,
                                  TCHAR* pszMonitoringFile,
                                  BOOL FullGeneration,
                                  BOOL bCheckOnly,
                                  BOOL Update,
                                  TCHAR* pszOldMonitoringFile
                                  );
    BOOL GenerateImportMonitoring(TCHAR* pszFileToMonitor,
                                  TCHAR* pszMonitoringFile,
                                  BOOL FullGeneration,
                                  BOOL bCheckOnly,
                                  BOOL Update,
                                  TCHAR* pszOldMonitoringFile
                                  );

    BOOL GenerateNETMonitoring(TCHAR* pszFileToMonitor,TCHAR* pszMonitoringFile,BOOL FullGeneration);
    BOOL GenerateCOMTypeLibraryMonitoring(TCHAR* TypeLibraryPath);
    BOOL GenerateCOMAllRegisteredTypeLibrariesMonitoring();

    BOOL UpdateBytesAnalysis(TCHAR* MonitoringFileToUpdate,TCHAR* OutPutFile);
    BOOL RemoveBytesAnalysis(TCHAR* MonitoringFileToUpdate,TCHAR* OutPutFile);

    void CancelCurrentOperation();
    
    void SetUserMessageInformationCallBack(tagUserMessageInformationCallBack CallBack,LPVOID UserParam);
    void SetPercentCompletedCallback(tagPercentCompletedCallBack CallBack,LPVOID UserParam);
};
