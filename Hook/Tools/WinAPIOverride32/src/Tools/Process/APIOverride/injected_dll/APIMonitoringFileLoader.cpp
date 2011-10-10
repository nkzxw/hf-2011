/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originally based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: load monitoring files
//-----------------------------------------------------------------------------

#include "APIMonitoringFileLoader.h"

#include "COM_Manager.h"
#include "NET_Manager.h"
#include "../../../linklist/linklist.h"
#include "../../../string/trimstring.h"
#include "../../../string/StringConverter.h"
#include "../../../string/codeparserhelper.h"
#include "../../../File/textfile.h"
#include "../../../File/StdFileOperations.h"
#include "DynamicLoadedFuncs.h"
#include "QueryMonitoringOwnership.h"
#include "reportmessage.h"
#include "../../../String/StrToHex.h"
#include "../UserDataType.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

TCHAR ApiMonitoringFileLoaderParseParametersUselessKeywordsArray[][64] = {
                                        _T("CONST"),
                                        _T("STRUCT"),
                                        _T("IN"),
                                        _T("OUT"),
                                        _T("INOUT"),
                                        _T("FAR"),
                                        _T("CLASS"),
                                        _T("ENUM"),
                                        _T("unsigned"),
                                        _T("__in"),
                                        _T("__inout"),
                                        _T("__out"),
                                        _T("__reserved"),
                                        _T("__in_opt"),
                                        _T("__in_default")
                                    };

extern CLinkList* pLinkListpMonitoringFileInfos;
extern CLinkList* pLinkListAPIInfos;
extern CCOM_Manager* pComManager;
extern CNET_Manager* pNetManager;
extern HANDLE hevtFreeProcess;
extern HANDLE ApiOverrideHeap;
extern TCHAR DefinesPath[MAX_PATH];
extern TCHAR UserTypesPath[MAX_PATH];
extern void UserTypeParsingReportError(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);

typedef struct tagMonitoringFileUserDefaultAction 
{
    BOOL QuestionAsked;
    BOOL TakeSameAction;
    BOOL TakeOwnerShip;
    BOOL RemoveConditionalBreakAndLogParameters;
}MONITORING_FILE_USER_DEFAULT_ACTION,*PMONITORING_FILE_USER_DEFAULT_ACTION;
MONITORING_FILE_USER_DEFAULT_ACTION gMonitoringFileUserDefaultAction={0};// init all to FALSE


//-----------------------------------------------------------------------------
// Name: FindType
// Object: search for description of an unknown type (a not fully supported type [see SupportedParameters.h/cpp])
//          WARNING : CASE SENSITIVE SEARCH IS DONE
// Parameters :
//     IN TCHAR* const UserTypesPath_loc : path containing user data type description
//     IN TCHAR* const ModuleName : name of the module (to be used as a subdirectory of UserTypesPath_loc)
//     IN TCHAR* const TypeName : name of searched type WARNING : case sensitive search is done
//     OUT USER_TYPE_INFOS** const ppUserTypeInfos : short file informations that will be stored in ApiInfo struct
//     OUT CUserDataType** ppUserDataType : pointeur to user data type successfully parsed
// Return : TRUE if type description has been found and successfully be parsed
//-----------------------------------------------------------------------------
BOOL FindType(IN TCHAR* const UserTypesPath_loc,
              IN TCHAR* const ModuleName,
              IN TCHAR* const TypeName,
              OUT USER_TYPE_INFOS** const ppUserTypeInfos,
              OUT CUserDataType** ppUserDataType)
{
    *ppUserDataType = NULL;
    *ppUserTypeInfos = NULL;
    if (TypeName==NULL)
        return FALSE;
    SIZE_T TypeNbPointedTime = 0;
    // module name is decorated in case of EXE_INTERNAL_XXX definitions 
    // ModuleName is like MyExe.exe|EXE_INTERNAL_XXX --> remove decoration |EXE_INTERNAL_XXX
    TCHAR LocalModuleName[2*MAX_PATH];
    TCHAR* psz= _tcschr(ModuleName,'|');
    if (psz)
    {
        // do string copy to avoid to modify original string
        _tcsncpy(LocalModuleName,ModuleName,2*MAX_PATH);
        LocalModuleName[2*MAX_PATH-1]=0;
        // look for '|' inside copy
        psz= _tcschr(LocalModuleName,'|');
        // remove '|' to keep only file name
        if (psz)
            *psz=0;
        psz = LocalModuleName;
    }
    else
        psz = ModuleName;
    // assume to keep only file name
    psz = CStdFileOperations::GetFileName(psz);

    CUserDataType::DEFINITION_FOUND DefinitionFound;
    CUserDataType* pUserDataType;
    // try to find user type description for wanted type
    // WARNING : TypeName is case sensitive !!!
    pUserDataType = CUserDataType::Parse(UserTypesPath_loc ,psz,TypeName,&TypeNbPointedTime,
                                         UserTypeParsingReportError,NULL,&DefinitionFound);

#ifdef _DEBUG
    {
        TCHAR Msg[512];
        TCHAR Way[256];
        switch (DefinitionFound)
        {
        case CUserDataType::DEFINITION_FOUND_BY_FILE_PARSING_IN_DEFAULT_DIRECTORY:
            _tcscpy(Way,_T("found by parsing default directory"));
            break;
        case CUserDataType::DEFINITION_FOUND_BY_FILE_PARSING_IN_SPECIALIZED_DIRECTORY:
            _tcscpy(Way,_T("found by parsing specialized directory"));
            break;
        case CUserDataType::DEFINITION_FOUND_IN_DEFAULT_CACHE:
            _tcscpy(Way,_T("found in default cache"));
            break;
        case CUserDataType::DEFINITION_FOUND_IN_FULLY_SUPPORTED_LIST:
            _tcscpy(Way,_T("found fully supported list"));
            break;
        case CUserDataType::DEFINITION_FOUND_IN_SPECIALIZED_CACHE:
            _tcscpy(Way,_T("found in specialized cache"));
            break;
        case CUserDataType::DEFINITION_FOUND_NOT_FOUND:
            _tcscpy(Way,_T("not found"));
            break;
        default:
            _tcscpy(Way,_T("unknown DEFINITION_FOUND enum value !!!"));
            break;
        }
        _sntprintf(Msg, 512, _T("Definition for type %s %s \r\n"),TypeName,Way);
        OutputDebugString(Msg);
    }
#endif

    if (!pUserDataType)
        return FALSE;

    // if type is a base type
    if (pUserDataType->IsBaseType())
    {
        // type is not an enum (enum must be parsed)
        if (!pUserDataType->IsEnum())
        {
            // if type is always PARAM_UNKNOWN, that means we found no matching definition for type
            if (pUserDataType->BaseType == PARAM_UNKNOWN)
                return FALSE;
        }
    }
    // fill output informations
    *ppUserDataType = pUserDataType;
    *ppUserTypeInfos = (USER_TYPE_INFOS*) HeapAlloc( ApiOverrideHeap,HEAP_ZERO_MEMORY,sizeof(USER_TYPE_INFOS));
    (*ppUserTypeInfos)->NbPointedTimes = TypeNbPointedTime;
    (*ppUserTypeInfos)->TypeSize = pUserDataType->GetSize();
    _tcsncpy((*ppUserTypeInfos)->szName,TypeName,MAX_PATH-1);
    // (*ppUserTypeInfos)->szName[MAX_PATH-1] = 0; // not needed due to HEAP_ZERO_MEMORY flag on HeapAlloc call

    // do not delete pUserDataType : CUserDataType manage it's one memory
    return (DefinitionFound != CUserDataType::DEFINITION_FOUND_NOT_FOUND);
}

//-----------------------------------------------------------------------------
// Name: LoadMonitoringFile
// Object: Parse config file and attach hooks.
//
// lines are like
// KERNEL32.DLL|GetProcAddress(HANDLE, PSTR)                       for classical definition
// KERNEL32.DLL|GetProcAddress(HANDLE, PSTR)|InOut                 for definition with parameter spy direction
// EXE_INTERNAL@0x12345|SupposedFuncName(HANDLE, PSTR)|Out         for direct addressing (allow to spy exe internal func), address is the virtual one not the raw one
//
// for default direction see DEFAULT_PARAMETER_DIRECTION_SPYING in APIMonitoringFileLoader.h
//
// Works only with lines ending with \r\n
//
// lines beginning with ';' are considered as comments
//                     '!' are considered as temporary disabled
//
// Parameters :
//     in  : TCHAR* pszFileName : config file name
// Return : TRUE if file is partially loaded (even if some non fatal errors occurs),
//          FALSE if file not loaded at all
//-----------------------------------------------------------------------------
BOOL LoadMonitoringFile(TCHAR* pszFileName)
{
    CLinkListItem* pListItemMonitoringFile;
    MONITORING_FILE_INFOS* pMonitoringFileInfos;
    TCHAR pszMsg[2*MAX_PATH];
    MONITORING_FILE_LINE_PARSER_USER_PARAM FileParserUserParam;

    if (*pszFileName==0)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Empty file name"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // check if file is already loaded
    if (GetMonitoringFileInfos(pszFileName,&pMonitoringFileInfos))
    {
        // Disabled for 5.3 version since multiple plugins can ask for a same overriding dll loading
        // so avoid user interaction
        //_stprintf(pszMsg,_T("File %s already loaded"),pszFileName);
        //DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // check if config file exists
    if (!CStdFileOperations::DoesFileExists(pszFileName))
    {
        _stprintf(pszMsg,_T("File %s not found"),pszFileName);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    pListItemMonitoringFile=pLinkListpMonitoringFileInfos->AddItem();
    if (!pListItemMonitoringFile)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Not enough memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    pMonitoringFileInfos=(MONITORING_FILE_INFOS*)pListItemMonitoringFile->ItemData;
    _tcsncpy(pMonitoringFileInfos->szFileName,pszFileName,MAX_PATH);

    // parse lines of config file
    FileParserUserParam.pMonitoringFileInfos=pMonitoringFileInfos;
    CTextFile::ParseLines(pszFileName,hevtFreeProcess,MonitoringFileLineParser,(PVOID)&FileParserUserParam);

    // if a function has been successfully hooked, we have to signal it to WinApiOverride
    // with a view to unload functions associated with this monitoring file
    // so we return TRUE even if only one function has been hooked
    return TRUE;
}


BOOL MonitoringFileLineParser(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    TCHAR* pszAPIDefinition; // full api description (module name + api name + optional parameters)
    TCHAR* pszModuleName;    // module name
    TCHAR* pszAPIName;       // api name
    TCHAR* pszParameterList; // parameters description
    TCHAR* pszAPIOptionalParameters; // optional parameters
    
    API_INFO *pAPIInfo;
    CLinkListItem *pItemAPIInfo;
    PBYTE pbAPI=0;
    BOOL bAlreadyHooked=FALSE;
    TCHAR pszMsg[3*MAX_PATH];
    BOOL bExeDllInternalHook=FALSE;
    BOOL bFunctionPointer;
    BOOL bMonitoringFileAlreadyDefined;
    BOOL bRemoveConditionalBreakAndLogParameters;
    MONITORING_FILE_INFOS* pMonitoringFileInfos;
    BOOL bRet;
    MONITORING_FILE_LINE_PARSER_USER_PARAM* pFileParserUserParam;
    tagCALLING_CONVENTION CallingConvention;

    pFileParserUserParam=(MONITORING_FILE_LINE_PARSER_USER_PARAM*)UserParam;

    // get current FileID
    pMonitoringFileInfos=pFileParserUserParam->pMonitoringFileInfos;

    // trim string buffer
    pszAPIDefinition=CTrimString::TrimString(Line);

    // if empty or commented line
    if ((*pszAPIDefinition==0)||(*pszAPIDefinition==';')||(*pszAPIDefinition=='!'))
        // continue parsing
        return TRUE;

    // check for particular cases (COM or NET monitoring files)
    BOOL bCOMDefinition=IsCOMHookDefinition(pszAPIDefinition);
    BOOL bNETDefinition=IsNetHookDefinition(pszAPIDefinition);
    if (bCOMDefinition || bNETDefinition)
    {
        MONITORING_FILE_INFOS* pAlreadyHookingMonitoringFileInfo=NULL;
        BOOL bParsingError=FALSE;

        // in case of COM hook definition
        if (bCOMDefinition)
        {
            pComManager->AddHookComMonitoringDefinition(
                                                        pFileParserUserParam->pMonitoringFileInfos,
                                                        pszAPIDefinition,
                                                        pszFileName,
                                                        dwLineNumber,
                                                        &pAlreadyHookingMonitoringFileInfo,
                                                        &bParsingError);
        }

        // in case of .NET hook definition
        else if (bNETDefinition)
        {
            if (!pNetManager->AddHookNetMonitoringDefinition(
                                                        pFileParserUserParam->pMonitoringFileInfos,
                                                        pszAPIDefinition,
                                                        pszFileName,
                                                        dwLineNumber,
                                                        &pAlreadyHookingMonitoringFileInfo,
                                                        &bParsingError)
                )
                return FALSE; // avoid lot of error message if .NET is not enabled
        }

        // check if hook was already defined
        if (pAlreadyHookingMonitoringFileInfo)
        {
            _sntprintf(pszMsg,3*MAX_PATH,_T("Hook defined by %s \r\nin %s at line: %u\r\nis already owned by %s\r\n"),
                pszAPIDefinition,pszFileName,dwLineNumber,pAlreadyHookingMonitoringFileInfo->szFileName);
            CReportMessage::ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
            // avoid multiple message boxes
            // DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
        }

        // check for parsing error
        if (bParsingError)
        {
            // parsing error. Ask if we should continue parsing
            _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d\r\n%s"),pszFileName,dwLineNumber,pszAPIDefinition);
            CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
            _tcsncat(pszMsg,_T("\r\nContinue parsing ?"),3*MAX_PATH);
            if (DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
                // continue parsing
                return TRUE;
            else
                // stop parsing
                return FALSE;
        }

        // continue parsing
        return TRUE;
    }
    // end of NET or COM hook definition case


    // standard api/win32 functions (no COM or .NET)

    // pszAPIDefinition is like KERNEL32.DLL|GetProcAddress(HANDLE, PSTR)
    //                       or KERNEL32.DLL|FARPROC GetProcAddress(HANDLE, PSTR)|In
    //                       or EXE_INTERNAL@0x411B70|BOOL WINAPI Add(int x,int y)
    //                       or DLL_INTERNAL@0x411B70|BOOL Add(int x,int y)|In
    //                       or DLL_ORDINAL@0x12@MyDll|BOOL Add(int x,int y)|In

    // get module name
    pszModuleName = pszAPIDefinition;

    // get pointer to function description (without module name)
    pszAPIName = _tcschr(pszModuleName, FIELDS_SEPARATOR);
    
    // if api name not found
    if (!pszAPIName)
    {
        // parsing error. Ask if we should continue parsing
        _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d\r\n%s"),pszFileName,dwLineNumber,pszAPIDefinition);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
        _tcsncat(pszMsg,_T("\r\nContinue parsing ?"),3*MAX_PATH-_tcslen(pszMsg)-1);
        if (DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
            // continue parsing
            return TRUE;
        else
            // stop parsing
            return FALSE;
    }

    // we have assume that api name was found

    // ends pszModuleName string by replacing FIELDS_SEPARATOR by \0
    *pszAPIName = 0;

    // get char after FIELDS_SEPARATOR for pszAPIName
    pszAPIName++;


    if (!ParseFunctionDescription(pszAPIName,&pszAPIName,&pszParameterList,&pszAPIOptionalParameters,&CallingConvention))
    {
        // parsing error ask if we should continue
        _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d"),pszFileName,dwLineNumber);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
        _tcsncat(pszMsg,_T("\r\nContinue parsing ?"),3*MAX_PATH);
        if (DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
            // continue parsing
            return TRUE;
        else
            // stop parsing
            return FALSE;
    }
 
    // Get function address, and check it's validity before continue parsing
    pbAPI=GetWinAPIOverrideFunctionDescriptionAddress(pszModuleName,pszAPIName,&bExeDllInternalHook,&bFunctionPointer);
    if (IsBadCodePtr((FARPROC)pbAPI))
    {
        _sntprintf(pszMsg,
                    3*MAX_PATH,
                    _T("Bad code pointer 0x%p for function %s in dll %s Function not hooked"),
                    pbAPI,
                    pszAPIName,
                    pszModuleName);
        // avoid multiple message boxes
        // DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);

        return TRUE;
    }

    // get item in list or add an item to list
    pItemAPIInfo=GetAssociatedItemAPIInfo(pbAPI,&bAlreadyHooked);
    if (!pItemAPIInfo) // check memory allocation
        return FALSE; 

    // get pAPIInfo associated to retrieved item
    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

    // modules filters are not checked for EXE_INTERNAL or DLL_INTERNAL as modules are calling callback
    // (by the way if the internal function is an EnumWindowsProc for EnumWindows() api)
    pAPIInfo->DontCheckModulesFilters=bExeDllInternalHook;

    // init vars
    bMonitoringFileAlreadyDefined=FALSE;
    bRemoveConditionalBreakAndLogParameters=TRUE;

    // check if monitoring is already hooked by another file
    if (pAPIInfo->pMonitoringFileInfos!=0)
    {
        BOOL bTakeOwnerShip=TRUE;
        bMonitoringFileAlreadyDefined=TRUE;

        if (gMonitoringFileUserDefaultAction.TakeSameAction)
        {
            bTakeOwnerShip=gMonitoringFileUserDefaultAction.TakeOwnerShip;
            bRemoveConditionalBreakAndLogParameters=gMonitoringFileUserDefaultAction.RemoveConditionalBreakAndLogParameters;
        }
        else
        {    
            TCHAR BlockMsg[5*MAX_PATH];
            _stprintf(BlockMsg,
                      _T("File %s\r\n provides new definition for function %s in %s, but a definition is already provided"),
                     pszFileName,
                     pszAPIName,
                     pszModuleName
                     );

            // get already hooking file name
            TCHAR BlockFileName[MAX_PATH];
            if (GetMonitoringFileName(pAPIInfo->pMonitoringFileInfos,BlockFileName))
            {
                _tcscat(BlockMsg,_T(" in\r\n"));
                _tcscat(BlockMsg,BlockFileName);
            }

            // if 2 different func names point on a same entry point, get user the name of the oldest func
            if (_tcsicmp(pszAPIName,pAPIInfo->szAPIName)
                || _tcsicmp(pszModuleName,pAPIInfo->szModuleName)
                )
            {
                _tcscat(BlockMsg,_T(" by function"));
                _tcscat(BlockMsg,pAPIInfo->szAPIName);
                _tcscat(BlockMsg,_T(" in "));
                _tcscat(BlockMsg,pAPIInfo->szModuleName);
            }

            CQueryMonitoringOwnerShip QueryMonitoringOwnerShip(
                            BlockMsg,
                            &gMonitoringFileUserDefaultAction.TakeSameAction,
                            &gMonitoringFileUserDefaultAction.TakeOwnerShip,
                            &gMonitoringFileUserDefaultAction.RemoveConditionalBreakAndLogParameters,
                            &gMonitoringFileUserDefaultAction.QuestionAsked);

            QueryMonitoringOwnerShip.ShowDialog();

            // fill local values
            bTakeOwnerShip=gMonitoringFileUserDefaultAction.TakeOwnerShip;
            bRemoveConditionalBreakAndLogParameters=gMonitoringFileUserDefaultAction.RemoveConditionalBreakAndLogParameters;
        }
        // if new file doesn't take ownership, don't update hook --> nothing to do
        if (!bTakeOwnerShip)
            return TRUE;
    }

    // if a monitoring file was already defined for this func
    if (bMonitoringFileAlreadyDefined)
    {
        // if user don't want to keep old conditional break and log conditions
        if (bRemoveConditionalBreakAndLogParameters)
        {
            // empty Conditional lists
            FreeOptionalParametersMemory(pAPIInfo);

            // reset direction spying type
            pAPIInfo->ParamDirectionType=DEFAULT_PARAMETER_DIRECTION_SPYING;
        }
    }

    // if a monitoring file was not previously defined
    // and a fake API as not already be loaded for this API
    // (else pAPIInfo->szModuleName and pAPIInfo->szAPIName are already allocated)
    if (!bAlreadyHooked)
    {
        pAPIInfo->bFunctionPointer=bFunctionPointer;
        pAPIInfo->CallingConvention=CallingConvention;
        if (!InitializeApiInfo(pAPIInfo,pszModuleName,pszAPIName))
        {
            // hook is not installed don't remove it
            FreeApiInfoItem(pItemAPIInfo);

            // memory allocation error --> stop parsing
            return FALSE;
        }
    }

    // set default spying type (in case of no pszAPIOptionalParameters)
    pAPIInfo->ParamDirectionType=DEFAULT_PARAMETER_DIRECTION_SPYING;

    bRet=ParseParameters(pAPIInfo,pszParameterList,pszFileName,dwLineNumber);
    if (bRet)
    {
        if (pszAPIOptionalParameters)
            bRet=ParseOptions(pAPIInfo,pszAPIOptionalParameters,bAlreadyHooked,pszFileName,dwLineNumber);
    }

    // store reference to file (only after having parsed parameters and options)
    pAPIInfo->pMonitoringFileInfos=pMonitoringFileInfos;

    // if function is already hook, continue parsing
    // whatever the result of parameter and options parsing
    if (bAlreadyHooked)
        return TRUE;// continue parsing

    if(!bRet) // free memory allocated for this hook
    {
        // hook is not installed don't remove it
        FreeApiInfoItem(pItemAPIInfo);
        return TRUE;
    }

    // we have assume all is ok 
    // --> install hook

    // install hook now (only now when pAPIInfo is fully completed else you'll get unexpected results :D)
    if (!HookAPIFunction(pAPIInfo))
    {
        // hook is not installed don't remove it
        FreeApiInfoItem(pItemAPIInfo);

        // continue parsing
        return TRUE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseFunctionDescription
// Object: split a function description into function name, parameters and optional parameters
//          Warning : directly act on pszFunctionDescription
// Parameters :
//     in  : TCHAR* pszFunctionDescription : function description libnamelike|function(params)|option1|option2|...|optionN
//     out : OUT TCHAR** ppszFunctionName : function name (not allocated point inside pszFunctionDescription)
//           OUT TCHAR** ppszParameters : parameters (not allocated point inside pszFunctionDescription)
//           OUT TCHAR** ppszOptions : options (not allocated point inside pszFunctionDescription)
//           OUT tagCALLING_CONVENTION* pCallingConvention
// Return : TRUE on success, FALSE on error (if function mustn't be hooked)
//-----------------------------------------------------------------------------
BOOL __stdcall ParseFunctionDescription(TCHAR* pszFunctionDescription,OUT TCHAR** ppszFunctionName, OUT TCHAR** ppszParameters, OUT TCHAR** ppszOptions,OUT tagCALLING_CONVENTION* pCallingConvention)
{
    TCHAR* pc;
    TCHAR* pcCallingConvention;
    *ppszFunctionName=pszFunctionDescription;
    *ppszParameters=NULL;
    *ppszOptions=NULL;
    *pCallingConvention=CALLING_CONVENTION_STDCALL_OR_CDECL;// used definition as default

    // look for optional hook options
    *ppszOptions=_tcschr(pszFunctionDescription,FIELDS_SEPARATOR);
    // if direction is specified
    if (*ppszOptions)
    {
        // ends function name
        **ppszOptions=0;
        // get char after FIELDS_SEPARATOR
        (*ppszOptions)++;
    }

    // look for the begin of parameters
    *ppszParameters = _tcschr(*ppszFunctionName, '(');

    // specific case : "::operator()(" like for "void  __cdecl ATL::CTraceFileAndLineInfo::operator()(PVOID pObject,unsigned long dwCategory,unsigned int nLevel,const char* pszFmt);"
    pc = _tcsstr(*ppszFunctionName, _T("::operator()("));
    if (pc)// if found
    {
        // if "::operator()(" begins before first "("
        if (pc<*ppszParameters)
        {
            // end function name
            *pc = 0;
            // move ppszParameters after _T("::operator()"), make it point to '('
            *ppszParameters = pc + _tcslen(_T("::operator()"));
        }
    }

    // look for the end of parameters (the last ')' as some func description are like f(int, int (*callback)(long))
    pc=0;
    if (*ppszParameters)
        pc=_tcsrchr(*ppszParameters,')');

    // check existence of parameters
    if ((*ppszParameters==0)||(pc==0))
        return FALSE;

    // we have assume we get parameters description

    // end *ppszParameters string (parameters description)
    *pc=0;

    // ends function name string by replacing ( by \0
    **ppszParameters=0;
    // pszParameterList begins after (
    (*ppszParameters)++;

    // trim function name
    *ppszFunctionName=CTrimString::TrimString(*ppszFunctionName);

    pcCallingConvention=*ppszFunctionName;
    // search for calling convention 
    if (   (_tcsstr(pcCallingConvention,_T("__stdcall ")))
        || (_tcsstr(pcCallingConvention,_T("WINAPI ")))
        || (_tcsstr(pcCallingConvention,_T("STDAPI ")))
        || (_tcsstr(pcCallingConvention,_T("STDMETHODCALLTYPE ")))
        || (_tcsstr(pcCallingConvention,_T("STDAPICALLTYPE ")))
        || (_tcsstr(pcCallingConvention,_T("WINOLEAPI ")))
        || (_tcsstr(pcCallingConvention,_T("STDMETHODIMP ")))
        )
    {
        *pCallingConvention=CALLING_CONVENTION_STDCALL;
    }
    else if (   (_tcsstr(pcCallingConvention,_T("__cdecl ")))
        || (_tcsstr(pcCallingConvention,_T("STDMETHODVCALLTYPE ")))
        || (_tcsstr(pcCallingConvention,_T("STDAPIVCALLTYPE ")))
        || (_tcsstr(pcCallingConvention,_T("STDAPIV ")))
        || (_tcsstr(pcCallingConvention,_T("STDMETHODIMPV ")))
        )
    {
        *pCallingConvention=CALLING_CONVENTION_CDECL;
    }
    else if (_tcsstr(pcCallingConvention,_T("__fastcall ")))
    {
        *pCallingConvention=CALLING_CONVENTION_FASTCALL;
    }
    else if (_tcsstr(pcCallingConvention,_T("__thiscall ")))
    {
        *pCallingConvention=CALLING_CONVENTION_THISCALL;
    }

    // split function name from any keyword (return type in function name if any or WINAPI, __cdecl)
    // in case of template we can have 
    // "public: __thiscall std::allocator<class CGenericQuickWayToFolderObject *>::allocator<class CGenericQuickWayToFolderObject *>"
    // "public: class std::vector<class CFolderMenu::CFolderPopUpMenuInfos * _Off,class std::allocator<class CFolderMenu::CFolderPopUpMenuInfos *> >::iterator __thiscall std::vector<class CFolderMenu::CFolderPopUpMenuInfos *,class std::allocator<class CFolderMenu::CFolderPopUpMenuInfos *> >::iterator::operator+(int)const"
    // CKeyReplace** CKeyReplace __thiscall std::vector<CKeyReplace *,std::allocator<CKeyReplace *> >::iterator::operator*();

    // in case of template
    if (_tcschr(*ppszFunctionName,'<'))
    {
        int Depth=0;
        pc = NULL;
        int Index=(int)(_tcslen(*ppszFunctionName)-1);
        
        // parse function name from end to begining
        for (int Cnt=Index;Cnt>=0;Cnt--)
        {
            switch((*ppszFunctionName)[Cnt])
            {
            case '>':
                Depth++;
                break;
            case '<':
                Depth--;
                break;
            // case any available splitter
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                if (Depth==0)
                {
                    // point to first space not in template < >
                    pc=(*ppszFunctionName)+Cnt;
                    *pc=0;
                    // point to char after splitter
                    pc++;
                    // keep only data after last splitter
                    *ppszFunctionName=pc;

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
        // pc=_tcsrchr(*ppszFunctionName,' ');
        pc= CCodeParserHelper::FindPreviousSplitterChar(*ppszFunctionName,*ppszFunctionName+_tcslen(*ppszFunctionName));
        
        // keep only data after last splitter
        if (pc == *ppszFunctionName)// we don't know if function fails by stopping on first char or success
        {
            // if a non alpha num or underscore has been found
            if (!CCodeParserHelper::IsDecoratedFunctionNameChar(*pc))
                *ppszFunctionName=pc+1;
            else
                *ppszFunctionName=pc;
        }
        else
            *ppszFunctionName=pc+1;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CreateParameterConditionalBreakContentListIfDoesntExist
// Object: check if pConditionalBreakContent is allocated for existing parameter
//          and allocate it if it doesn't exist
// Parameters :
//     in  : PARAMETER_INFOS* pParameter : parameter
// Return : value affected to pParameter->pConditionalBreakContent (to check allocation error)
//-----------------------------------------------------------------------------
CLinkList* __stdcall CreateParameterConditionalBreakContentListIfDoesntExist(PARAMETER_INFOS* pParameter)
{
    if (pParameter->pConditionalBreakContent==NULL)
        pParameter->pConditionalBreakContent=new CLinkList(sizeof(MONITORING_PARAMETER_OPTIONS));
    return pParameter->pConditionalBreakContent;
}

//-----------------------------------------------------------------------------
// Name: CreateParameterConditionalLogContentListIfDoesntExist
// Object: check if pConditionalLogContent is allocated for existing parameter
//          and allocate it if it doesn't exist
// Parameters :
//     in  : PARAMETER_INFOS* pParameter : parameter
// Return : value affected to pParameter->pConditionalBreakContent (to check allocation error)
//-----------------------------------------------------------------------------
CLinkList* __stdcall CreateParameterConditionalLogContentListIfDoesntExist(PARAMETER_INFOS* pParameter)
{
    if (pParameter->pConditionalLogContent==NULL)
        pParameter->pConditionalLogContent=new CLinkList(sizeof(MONITORING_PARAMETER_OPTIONS));
    return pParameter->pConditionalLogContent;
}

//-----------------------------------------------------------------------------
// Name: ParseParameters
// Object: parse parameters and parameters options of a monitoring file
// Parameters :
//     in  : API_INFO *pAPIInfo : pointer to an allocated API_INFO structure
//           TCHAR* pszParameters : string containing all parameters
//           TCHAR* pszFileName : name of monitoring file (for error report)
//           DWORD dwLineNumber : line number of monitoring file (for error report)
// Return : TRUE on success, FALSE on error (if function mustn't be hooked)
//-----------------------------------------------------------------------------
BOOL __stdcall ParseParameters(API_INFO *pAPIInfo,TCHAR* pszParameters,TCHAR* pszFileName,DWORD dwLineNumber)
{
    TCHAR pszMsg[3*MAX_PATH];
    TCHAR* pszParameter;
    BYTE Cnt;
    TCHAR* pEndOfPointedParamType;
    BYTE cParamIndex;
    CLinkListItem *pItem;
    TCHAR* pc;
    TCHAR* pc2;
    TCHAR* pc3;
    MONITORING_PARAMETER_OPTIONS* pParamOption;
    TCHAR* pszOptionalParameters;
    TCHAR* pszNextOptionalParameters;
    TCHAR* pszTmp;
    TCHAR* pszNextParam;
    BOOL bFunctionParameter;
    TCHAR szParamFunction[32];
    SIZE_T Depth;
    SIZE_T NbPointedTimes;
    DWORD StaticArraySize=1;
    DWORD CurrentStaticArraySize=1;
    BOOL bStaticArray=FALSE;

    //////////////////////////////////////
    // parse all func parameters
    //////////////////////////////////////

    // check if no param func
    BOOL bNoParams=FALSE;
    pszParameters=CTrimString::TrimString(pszParameters);

    // if func contains no args but has the (void) description, remove void
    if (_tcsicmp(pszParameters,_T("void"))==0)
        bNoParams=TRUE;
    else
    {
        bNoParams=TRUE;
        pc=pszParameters+_tcslen(pszParameters)-1;
        // search char between ( [pszParameters] and ) [pc]
        while (pc>pszParameters)
        {
            if (!CCodeParserHelper::IsSpaceOrSplitter(*pc))
            {
                bNoParams=FALSE;
                break;
            }
            pc--;
        }
    }


    // initialize parameter counter
    cParamIndex = 0;

    // in case of __thiscall, the first PVOID pObject is not in parameter list --> add it
    // see debug info generated files
    if (pAPIInfo->CallingConvention==CALLING_CONVENTION_THISCALL)
    {
        _tcscpy(pAPIInfo->ParamList[0].pszParameterName,_T("pObject"));
        pAPIInfo->ParamList[0].dwType=PARAM_POINTER;
        pAPIInfo->ParamList[0].pConditionalBreakContent=0;
        pAPIInfo->ParamList[0].pConditionalLogContent=0;
        pAPIInfo->ParamList[0].dwSizeOfData=CSupportedParameters::GetParamStackSize(PARAM_POINTER);
        pAPIInfo->ParamList[0].dwSizeOfPointedData=CSupportedParameters::GetParamPointedSize(PARAM_POINTER);
        cParamIndex++;
    }

    // find first parameter
    pszNextParam=pszParameters-1;// adjust pszNextParam for loop


    // if there's some param
    if (!bNoParams)
    {
        BOOL bLastParam=FALSE;
        // fill given params type
        for (;!bLastParam;)
        {
            bFunctionParameter=FALSE;
            pszParameter=pszNextParam+1;


            // assume to parse correctly functions like 
            // "class std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::iterator std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::iterator __thiscall std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::erase(class std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::iterator _First,class std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::iterator _Last);"
            // "bool  __thiscall std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::const_iterator::operator!=(const class std::vector<_ITEMIDLIST *,std::allocator<_ITEMIDLIST *> >::const_iterator& _Right);""

            // "int __cdecl Function(int i,unsigned int (__cdecl*)(void *,char const *,unsigned int) wfunc,void * param)"
FindNextParam:
            pszNextParam=_tcspbrk(pszNextParam+1,_T(",()<"));
            if (!pszNextParam)
            {
                bLastParam=TRUE;
            }
            else
            {
                if (*pszNextParam=='<')
                {
                    Depth=1;
FindNextTemplateDelimiter:
                    pszNextParam=_tcspbrk(pszNextParam+1,_T("<>"));
                    if (!pszNextParam)
                    {
                        if (pszFileName)
                        {
                            _sntprintf(pszMsg,3*MAX_PATH,
                                _T("Parsing error for function %s in file %s at line %u "),
                                pAPIInfo->szAPIName,
                                pszFileName,
                                dwLineNumber);
                        }
                        else
                        {
                            _sntprintf(pszMsg,3*MAX_PATH,
                                _T("Parsing error for function %s"),
                                pAPIInfo->szAPIName);
                        }
                        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                        break;
                    }
                    else
                    {
                        if (*pszNextParam=='<')
                        {
                            Depth++;
                            goto FindNextTemplateDelimiter;
                        }
                        else // if (*pszNextParam=='>')
                        {
                            Depth--;
                            if (Depth<=0)
                            {
                                goto FindNextParam;
                            }
                            else
                            {
                                goto FindNextTemplateDelimiter;
                            }
                        }
                    }
                }
                else if (*pszNextParam=='(')
                {
                    // parameter is a function parameter
                    bFunctionParameter=TRUE;
                    // find end of function description
                    pszNextParam=_tcschr(pszNextParam+1,')');
                    if (pszNextParam)
                    {
                        goto FindNextParam;
                    }
                    else
                    {
                        if (pszFileName)
                        {
                            _sntprintf(pszMsg,3*MAX_PATH,
                                _T("Parsing error for function %s in file %s at line %u "),
                                pAPIInfo->szAPIName,
                                pszFileName,
                                dwLineNumber);
                        }
                        else
                        {
                            _sntprintf(pszMsg,3*MAX_PATH,
                                _T("Parsing error for function %s"),
                                pAPIInfo->szAPIName);
                        }
                        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                        break;
                    }
                }
                *pszNextParam=0;
            }


            if (bFunctionParameter)
            {
                // do not use pszParameter = API_MONITORING_FILE_LOADER_FUNCTION_PARAMETER;
                // as operation like _tcsupr are done on pszParameter
                _tcscpy(szParamFunction,API_MONITORING_FILE_LOADER_FUNCTION_PARAMETER);
                pszParameter = szParamFunction;
            }

            pszParameter = CTrimString::TrimString(pszParameter);

            // if var args, stop parameter parsing
            if (_tcsicmp(pszParameter,_T("..."))==0)
                break;

            //////////////////////////////////////////////
            // search for optionals parameter keywords
            //////////////////////////////////////////////

            // search for parameter option
            pszOptionalParameters=pszParameter;
FindFirstParameterOption:
            pszOptionalParameters=_tcschr(pszOptionalParameters,PARAMETER_OPTION_SEPARATOR);

            // patch while PARAMETER_OPTION_SEPARATOR == ':'
            if (pszOptionalParameters)
            {
                // "::" case
                if (*(pszOptionalParameters+1)==PARAMETER_OPTION_SEPARATOR)
                {
                    pszOptionalParameters=pszOptionalParameters+2;
                    goto FindFirstParameterOption;
                }
            }

            pszNextOptionalParameters=pszOptionalParameters;
            // if we found an optional parameter
            if (pszNextOptionalParameters)
            {
                // ends pszParameter description
                *pszNextOptionalParameters=0;
                // point after separator
                pszNextOptionalParameters++;
            }
            // if optional parameter is specified
            while (pszNextOptionalParameters)
            {
                pszOptionalParameters=pszNextOptionalParameters;

FindNextParameterOption:
                // find next optional parameter
                pszNextOptionalParameters=_tcschr(pszOptionalParameters,PARAMETER_OPTION_SEPARATOR);

                // patch while PARAMETER_OPTION_SEPARATOR == ':'
                if (pszNextOptionalParameters)
                {
                    // "::" case
                    if (*(pszNextOptionalParameters+1)==PARAMETER_OPTION_SEPARATOR)
                    {
                        pszOptionalParameters=pszOptionalParameters+2;
                        goto FindNextParameterOption;
                    }
                }


                if (pszNextOptionalParameters)
                {
                    *pszNextOptionalParameters=0;
                    pszNextOptionalParameters++;
                }

                // trim
                pszOptionalParameters=CTrimString::TrimString(pszOptionalParameters);

                // find type of option and store it into conditional list

                if (_tcsnicmp(pszOptionalParameters,OPTION_DATA_SIZE,_tcslen(OPTION_DATA_SIZE))==0)
                {
                    pszTmp=&pszOptionalParameters[_tcslen(OPTION_DATA_SIZE)];
                    CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->ParamList[cParamIndex].dwSizeOfData);

                    // empty dwSizeOfPointedData
                    pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData=0;
                }
                else if (_tcsnicmp(pszOptionalParameters,OPTION_POINTED_DATA_SIZE,_tcslen(OPTION_POINTED_DATA_SIZE))==0)
                {
                    pszTmp=&pszOptionalParameters[_tcslen(OPTION_POINTED_DATA_SIZE)];
                    // if pointed data size defined by another arg
                    if(_tcsnicmp(pszTmp,OPTION_POINTED_DATA_SIZE_DEFINED_BY_ANOTHER_ARG,_tcslen(OPTION_POINTED_DATA_SIZE_DEFINED_BY_ANOTHER_ARG))==0)
                    {
                        // string is like "ArgX" or "ArgX*Y"

                        // set flag
                        pAPIInfo->ParamList[cParamIndex].bSizeOfPointedDataDefinedByAnotherParameter=TRUE;
                        // mov after "arg"
                        pszTmp+=_tcslen(OPTION_POINTED_DATA_SIZE_DEFINED_BY_ANOTHER_ARG);

                        // get number of arg
                        CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData);

                        // look for ArgX*Y
                        TCHAR* ptc = _tcschr(pszTmp,'*');
                        if (ptc)
                        {
                            // point after *
                            ptc++;
                            CStringConverter::StringToDWORD(ptc,&pAPIInfo->ParamList[cParamIndex].SizeOfPointedDataDefinedByAnotherParameterFactor);
                        }
                        else
                        {
                            pAPIInfo->ParamList[cParamIndex].SizeOfPointedDataDefinedByAnotherParameterFactor = 1;
                        }
                        
                        // put the arg number from 1 based to 0 based
                        if (pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData)
                            pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData--;
                    }
                    else
                    {
                        // set flag
                        pAPIInfo->ParamList[cParamIndex].bSizeOfPointedDataDefinedByAnotherParameter=FALSE;
                        // get value
                        CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData);
                    }

                    // empty dwSizeOfData
                    pAPIInfo->ParamList[cParamIndex].dwSizeOfData=0;
                }
                else if (_tcsnicmp(pszOptionalParameters,OPTION_LOG_VALUE,_tcslen(OPTION_LOG_VALUE))==0)
                {
                    if (CreateParameterConditionalLogContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalLogContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pszTmp=&pszOptionalParameters[_tcslen(OPTION_LOG_VALUE)];
                        CStringConverter::StringToPBYTE(pszTmp,&pParamOption->Value);
                    }
                }
                else if(_tcsnicmp(pszOptionalParameters,OPTION_BREAK_VALUE,_tcslen(OPTION_BREAK_VALUE))==0)
                {
                    if (CreateParameterConditionalBreakContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalBreakContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pszTmp=&pszOptionalParameters[_tcslen(OPTION_BREAK_VALUE)];
                        CStringConverter::StringToPBYTE(pszTmp,&pParamOption->Value);
                    }
                }

                else if(_tcsnicmp(pszOptionalParameters,OPTION_LOG_BUFFER_VALUE,_tcslen(OPTION_LOG_BUFFER_VALUE))==0)
                {
                    if (CreateParameterConditionalLogContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalLogContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pParamOption->pbPointedValue=StrByteArrayToByteArray(
                            &pszOptionalParameters[_tcslen(OPTION_LOG_BUFFER_VALUE)],
                            &pParamOption->dwValueSize);
                        // if an error occurs, remove item from linked list
                        if (pParamOption->pbPointedValue==NULL)
                            pAPIInfo->ParamList[cParamIndex].pConditionalLogContent->RemoveItem(pItem);
                    }
                }

                else if(_tcsnicmp(pszOptionalParameters,OPTION_LOG_POINTED_VALUE,_tcslen(OPTION_LOG_POINTED_VALUE))==0)
                {
                    if (CreateParameterConditionalLogContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalLogContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pParamOption->pbPointedValue=StrByteArrayToByteArray(
                            &pszOptionalParameters[_tcslen(OPTION_LOG_POINTED_VALUE)],
                            &pParamOption->dwPointedValueSize);
                        // if an error occurs, remove item from linked list
                        if (pParamOption->pbPointedValue==NULL)
                            pAPIInfo->ParamList[cParamIndex].pConditionalLogContent->RemoveItem(pItem);
                    }
                }
                else if(_tcsnicmp(pszOptionalParameters,OPTION_BREAK_POINTED_VALUE,_tcslen(OPTION_BREAK_POINTED_VALUE))==0)
                {
                    if (CreateParameterConditionalBreakContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalBreakContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pParamOption->pbPointedValue=StrByteArrayToByteArray(
                            &pszOptionalParameters[_tcslen(OPTION_BREAK_POINTED_VALUE)],
                            &pParamOption->dwPointedValueSize);
                        // if an error occurs, remove item from linked list
                        if (pParamOption->pbPointedValue==NULL)
                            pAPIInfo->ParamList[cParamIndex].pConditionalBreakContent->RemoveItem(pItem);
                    }
                }

                else if(_tcsnicmp(pszOptionalParameters,OPTION_BREAK_BUFFER_VALUE,_tcslen(OPTION_BREAK_BUFFER_VALUE))==0)
                {
                    if (CreateParameterConditionalBreakContentListIfDoesntExist(&pAPIInfo->ParamList[cParamIndex]))
                    {
                        pItem=pAPIInfo->ParamList[cParamIndex].pConditionalBreakContent->AddItem();
                        pParamOption=((MONITORING_PARAMETER_OPTIONS*)pItem->ItemData);
                        pParamOption->pbPointedValue=StrByteArrayToByteArray(
                            &pszOptionalParameters[_tcslen(OPTION_BREAK_BUFFER_VALUE)],
                            &pParamOption->dwValueSize);
                        // if an error occurs, remove item from linked list
                        if (pParamOption->pbPointedValue==NULL)
                            pAPIInfo->ParamList[cParamIndex].pConditionalBreakContent->RemoveItem(pItem);
                    }
                }
                else if (_tcsnicmp(pszOptionalParameters,OPTION_DEFINE_VALUES,_tcslen(OPTION_DEFINE_VALUES))==0)
                {
                    // make full file name
                    TCHAR FullPath[MAX_PATH];
                    _tcscpy(FullPath,DefinesPath);
                    _tcsncat(FullPath,&pszOptionalParameters[_tcslen(OPTION_DEFINE_VALUES)],MAX_PATH);
                    // check if file exists before storing value
                    // allow to earn time if file doesn't exist
                    if (CStdFileOperations::DoesFileExists(FullPath))
                    {
                        pAPIInfo->ParamList[cParamIndex].pDefineInfos = (DEFINE_INFOS*)HeapAlloc(ApiOverrideHeap, 0, sizeof(DEFINE_INFOS));
                        _tcsncpy(pAPIInfo->ParamList[cParamIndex].pDefineInfos->szFileName,&pszOptionalParameters[_tcslen(OPTION_DEFINE_VALUES)],MAX_PATH);
                    }
                    else
                    {
                        _sntprintf(pszMsg,3*MAX_PATH,_T("File %s not found"),FullPath);
                        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
                    }
                }

                else
                {
                    if (pszFileName)
                    {
                        _sntprintf(pszMsg,3*MAX_PATH,
                            _T("Unknown option %s used for function %s in file %s at line %u "),
                            pszOptionalParameters,
                            pAPIInfo->szAPIName,
                            pszFileName,
                            dwLineNumber);
                    }
                    else
                    {
                        _sntprintf(pszMsg,3*MAX_PATH,
                            _T("Unknown option %s used for function %s"),
                            pszOptionalParameters,
                            pAPIInfo->szAPIName);
                    }
                    DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                }

            }

            // remove useless keywords (const, struct, IN, OUT, INOUT, FAR, class)
            // must be done before checking for " *" (like for "char FAR *")
            // see ApiMonitoringFileLoaderParseParametersUselessKeywordsArray to add string
            TCHAR* KeyWordBegin;
            TCHAR* KeyWordEnd;
            SIZE_T ItemSize;
            BOOL bKeyWordRemoved;
           
            KeyWordEnd = pszParameter;
            for(BOOL bStopLoop = FALSE;!bStopLoop;) 
            {
                KeyWordBegin = CCodeParserHelper::FindNextAlphaNumOrUnderscore(KeyWordEnd);
                if (*KeyWordBegin==0)
                    break;

                // go to next keyword splitter
                KeyWordEnd = CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(KeyWordBegin+1);
                if (*KeyWordEnd == 0)
                    bStopLoop = TRUE;
                KeyWordEnd--;

                ItemSize = KeyWordEnd-KeyWordBegin+1;

                // check if keyword belongs to the removed ones
                bKeyWordRemoved = FALSE;
                for (Cnt = 0; Cnt< _countof(ApiMonitoringFileLoaderParseParametersUselessKeywordsArray) ;Cnt++)
                {
                    if (ItemSize<_tcslen(ApiMonitoringFileLoaderParseParametersUselessKeywordsArray[Cnt]))
                        continue;
                    if (_tcsnicmp(KeyWordBegin,ApiMonitoringFileLoaderParseParametersUselessKeywordsArray[Cnt],ItemSize)==0)
                    {
                        memmove(KeyWordBegin,KeyWordEnd+1,(_tcslen(KeyWordEnd+1)+1)*sizeof(TCHAR));//+1 to move \0
                        KeyWordEnd = KeyWordBegin;
                        bKeyWordRemoved = TRUE;
                        break; // break for
                    }
                }
                // if keyword has not been removed
                if (!bKeyWordRemoved)
                    // point on first splitter char
                    KeyWordEnd++;
            }

            pszParameter = CTrimString::TrimString(pszParameter);

            // replace references '&' by '*'
            pc=pszParameter-1;
            for(;;)
            {
                pc=_tcschr(pc+1,'&');
                if (pc==NULL)
                    break;
                *pc='*';
            }

///////////////////////////////////////////////////////
// replace all [] by a single * (because Type[x][y][z] is like Type[x*y*z] in memory)
            // only if a size has not been defined
if (pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData==0)
{
    pc = pszParameter;
    for (;pc;pc = pc2+1)
    {
        pc = _tcschr(pc,'[');
        if (!pc)
            break;
        pc2 = _tcschr(pc+1,']');
        if (!pc2)
            break;
        if (CStringConverter::StringToDWORD(pc+1,&CurrentStaticArraySize))
        {
            StaticArraySize *= CurrentStaticArraySize;
        }
        else
            break;
        // replace '[x][y][z]' by '     '
        for (;pc<=pc2;pc++)
        {
            *pc=' ';
        }
        if (!bStaticArray)
        {
            bStaticArray = TRUE;
        }
    }
}
///////////////////////////////////////////////////////
            
            // looking for " *" for definitions like "char *psz" to keep "char*"
            pEndOfPointedParamType=NULL;
            
            // in case of a parameter like "class std::allocator<class CGenericQuickWayToFolderObject *> const & __formal"
            // we got here "std::allocator<class CGenericQuickWayToFolderObject *> * __formal"
            // find last '>' (parameter name is after last '>')
            pc=_tcsrchr(pszParameter,'>');
            if (pc)
            {
                pc++;
            }
            else
            {
                pc=pszParameter;
            }

            // find '*'
            pc2=_tcschr(pc,'*');
            if (pc2)
            {
                // find previous not space char
                pc = CCodeParserHelper::FindPreviousNotSplitterChar(pc,pc2);
                // point on first space if any
                pc++;
                // replace space by *
                *pc='*';
                // point after *
                pc++;

                // check for other * like char **
                for(;;)
                {
                    pc2 = CCodeParserHelper::FindNextNotSplitterChar(pc2+1);
                    if (*pc2 != '*')
                        break;

                    *pc='*';
                    pc++;
                }
                // end string
                *pc=0;

                // mark pEndOfPointedParamType
                pEndOfPointedParamType=pc2;
            }

            *pAPIInfo->ParamList[cParamIndex].pszParameterName=0;

            // if it's a pointed type pointer name is just at pEndOfPointedParamType (next name after * and spaces)
            if (pEndOfPointedParamType)
            {

                _tcsncpy(pAPIInfo->ParamList[cParamIndex].pszParameterName,pEndOfPointedParamType,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
                pAPIInfo->ParamList[cParamIndex].pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;
                // string is already ended
            }
            else
            {
                // check if there's still param name by searching next space
                pc=_tcspbrk(pszParameter,_T(" \t"));

                if (pc)// if it is the case
                {
                    _tcsncpy(pAPIInfo->ParamList[cParamIndex].pszParameterName,pc+1,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
                    pAPIInfo->ParamList[cParamIndex].pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;

                    *pc=0;// remove param name or other key word after param name
                }
            }
            // try to remove keyword if any after param name
            if (*pAPIInfo->ParamList[cParamIndex].pszParameterName)
            {
                // remove beginning and ending spaces if any
                pc=CTrimString::TrimString(pAPIInfo->ParamList[cParamIndex].pszParameterName);
                _tcsncpy(pAPIInfo->ParamList[cParamIndex].pszParameterName,pc,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
                pAPIInfo->ParamList[cParamIndex].pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;

                // search for space after first name
                pc=CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(pAPIInfo->ParamList[cParamIndex].pszParameterName);
                // remove data after first space
                *pc=0;
            }


            // in case of static array, add reference
            if (bStaticArray)
                _tcscat(pszParameter,_T("*"));

            // get parameter infos from parameter name
            SUPPORTED_PARAMETERS_STRUCT* pSupportedParameterInfos;
            pSupportedParameterInfos = CSupportedParameters::GetParamInfos(pszParameter);

            // particular case : if PBYTE buffer defined by "unsigned char*" or "char*", 
            // and a size is specified, force type to PBYTE to avoid a string representation
            if (pSupportedParameterInfos->ParamType == PARAM_PSTR)
            {
                // if size has been specified with :PointedDataSize keyword
                if (pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData != 0)
                {
                    pSupportedParameterInfos = CSupportedParameters::GetParamInfos(PARAM_PBYTE);
                }
            }

            // get parameter type
            pAPIInfo->ParamList[cParamIndex].dwType = pSupportedParameterInfos->ParamType;
            
            if (
                // if parameter type has not been found in internally supported types
                pAPIInfo->ParamList[cParamIndex].dwType == PARAM_UNKNOWN
                )
            {
                NbPointedTimes = 0;

                pc = _tcschr(pszParameter,'*');
                while(pc)
                {
                    pc = _tcschr(pc+1,'*');
                    NbPointedTimes++;
                    // if pointed more than once we can do nothing (not currently supported)
                    if (NbPointedTimes>1)
                    {
                        pAPIInfo->ParamList[cParamIndex].dwType = PARAM_POINTER;
                        pAPIInfo->ParamList[cParamIndex].dwSizeOfData=CSupportedParameters::GetParamStackSize(PARAM_POINTER);
                        pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData=CSupportedParameters::GetParamPointedSize(PARAM_POINTER);
                        break;
                    }
                }

                if (NbPointedTimes<2)
                {
                    // NbPointedTimes == 0 || 1
                    pc = pszParameter;
                    pc2 = 0;
                    if (NbPointedTimes>0)
                    {
                        // NbPointedTimes == 1

                        // do a local copy of type name to remove '*'
                        pc2 = _tcsdup(pc);
                        pc = pc2;

                        // remove '*' from type name
                        pc3 = _tcschr(pc2,'*');
                        if (pc3)
                            *pc3 = 0;
                        pc = CTrimString::TrimString(pc);
                    }

                    // pc contains parameter type name without * and & and other keywords
                    CUserDataType* pUserDataType;
                    // look for type description
                    BOOL bTypeFound = FindType(UserTypesPath,pAPIInfo->szModuleName,pc,&pAPIInfo->ParamList[cParamIndex].pUserTypeInfos,&pUserDataType);

                    if (pc2)
                        free(pc2);

                    // if type is describe
                    if (bTypeFound && pUserDataType)
                    {
                        // update pointed times in case of P,LP or LPC types
                        NbPointedTimes+=pAPIInfo->ParamList[cParamIndex].pUserTypeInfos->NbPointedTimes;
                        // if pointed more than once
                        if (NbPointedTimes>1)
                        {
                            // if pointed more than once we can do nothing (not currently supported)
                            pAPIInfo->ParamList[cParamIndex].dwType = PARAM_POINTER;
                            pAPIInfo->ParamList[cParamIndex].dwSizeOfData=CSupportedParameters::GetParamStackSize(PARAM_POINTER);
                            pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData=CSupportedParameters::GetParamPointedSize(PARAM_POINTER);
                        }
                        else
                        {
                            // NbPointedTimes == 0 || 1

                            // if type is an enum or a user define type
                            if (pUserDataType->IsEnum() || (!pUserDataType->IsBaseType()) ) // IsBaseType should not occur as we are in PARAM_UNKNOWN condition
                            {
                                // as in standard view type are not parsed for speed performances,
                                // adjust enum type so it will be displayed in integer form
                                if (pUserDataType->IsEnum())
                                {
                                    // adjust logged size according to enum size and NbPointedTimes
                                    if (NbPointedTimes == 0)
                                        pAPIInfo->ParamList[cParamIndex].dwType = PARAM_SIZE_T;
                                    else 
                                        pAPIInfo->ParamList[cParamIndex].dwType = PARAM_PSIZE_T;
                                }
                                // add the EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE to type
                                pAPIInfo->ParamList[cParamIndex].dwType|=EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE;
                            }
                            else if (pUserDataType->IsBaseType())
                            {
                                pAPIInfo->ParamList[cParamIndex].dwType = pUserDataType->BaseType;
                            }

                            // if log size informations were specified, keep them
                            if (
                                // if size is specified
                                (pAPIInfo->ParamList[cParamIndex].dwSizeOfData != 0)
                                // if pointed size specified
                                || (pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData != 0)
                                // if size defined by another arg
                                || (pAPIInfo->ParamList[cParamIndex].bSizeOfPointedDataDefinedByAnotherParameter)
                                )
                            {
                                // keep size infos
                            }
                            else // no log size specified in monitoring file
                            {
                                // if (NbPointedTimes<2) is assumed as we already the if (NbPointedTimes>1) check
                                {
                                    // adjust logging size informations according to NbPointedTimes and pUserTypeInfos->TypeSize
                                    if (NbPointedTimes == 0)
                                    {
                                        // get stack size
                                        pAPIInfo->ParamList[cParamIndex].dwSizeOfData = StackSize(pUserDataType->GetSize());
                                    }
                                    else // if (NbPointedTimes == 1)
                                    {
                                        pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData = pUserDataType->GetSize();
                                    }
                                }
                            }
                        }
                    }// end if type found
                    // else let type to PARAM_UNKNOWN (we are in the if (pAPIInfo->ParamList[cParamIndex].dwType == PARAM_UNKNOWN) condition)
                }
            }

            // if item is not a pointer
            if (pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData==0)
            {
                if (pSupportedParameterInfos->DataSize == 0)
                {
                    pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData = pSupportedParameterInfos->PointedDataSize;
                }
                else
                {
                    // if no size has been specified for parameter
                    if (pAPIInfo->ParamList[cParamIndex].dwSizeOfData==0)
                        // get the default one
                        pAPIInfo->ParamList[cParamIndex].dwSizeOfData=StackSize(pSupportedParameterInfos->DataSize);
                }
            }

            // if defines infos are present, add flag to type
            if (pAPIInfo->ParamList[cParamIndex].pDefineInfos)
                pAPIInfo->ParamList[cParamIndex].dwType|=EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE;

            if (bStaticArray)
            {
                // dwSizeOfPointedData has not been defined by user, so dwSizeOfPointedData contains the size of a single element
                pAPIInfo->ParamList[cParamIndex].dwSizeOfPointedData *= StaticArraySize;
            }

            // increase parameter count
            cParamIndex++;

            // check number of parameters
            if (cParamIndex == MAX_PARAM)
                // too much param : can't be hooked (MAX_PARAM can be changed in APIOverrideKernel.h)
                // hook only the first MAX_PARAM parameters
                break;
        }
    }

    // we have assume cParamIndex <= MAX_PARAM
    pAPIInfo->MonitoringParamCount = cParamIndex;

    // get stack size
    pAPIInfo->StackSize=0;
    for (Cnt = 0; Cnt < pAPIInfo->MonitoringParamCount; Cnt++)
    {
        /////////////////////////////////////
        // compute param required stack size
        /////////////////////////////////////
        if (pAPIInfo->ParamList[Cnt].dwSizeOfData)
            pAPIInfo->StackSize+=pAPIInfo->ParamList[Cnt].dwSizeOfData;
        else // pointer or less than REGISTER_BYTE_SIZE
            // add default register size in byte
            pAPIInfo->StackSize+=REGISTER_BYTE_SIZE;

        /////////////////////////////////////
        // check param number of PointedDataDefinedByAnotherParameter value
        /////////////////////////////////////
        if (pAPIInfo->ParamList[Cnt].bSizeOfPointedDataDefinedByAnotherParameter)
        {
            if (pAPIInfo->ParamList[Cnt].dwSizeOfPointedData>=pAPIInfo->MonitoringParamCount)
            {
                if (pszFileName)
                {
                    _sntprintf(pszMsg,
                        3*MAX_PATH,
                        _T("Bad argument number for %s at line %u for function %s\r\n in file %s"),
                        OPTION_POINTED_DATA_SIZE,
                        dwLineNumber,
                        pAPIInfo->szAPIName,
                        pszFileName);
                }
                else
                {
                    _sntprintf(pszMsg,
                        3*MAX_PATH,
                        _T("Bad argument number for %s for function %s"),
                        OPTION_POINTED_DATA_SIZE,
                        pAPIInfo->szAPIName
                        );
                }

                // reset flags
                pAPIInfo->ParamList[Cnt].bSizeOfPointedDataDefinedByAnotherParameter=FALSE;
                pAPIInfo->ParamList[Cnt].dwSizeOfPointedData=0;

                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            }
            else
            {
                // assume referencing type is not interpreted
                DWORD dwReferencingType = pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwType;
                if ( dwReferencingType == PARAM_PSTR)
                {
                    // can occurred due to unsigned char* translated to char*
                    // convert it to PBYTE
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwType = PARAM_PBYTE;
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwSizeOfData = 0;
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwSizeOfPointedData = 1;
                }
                else if ( dwReferencingType == PARAM_PWSTR)
                {
                    // convert to PSHORT
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwType = PARAM_PWORD;
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwSizeOfData = 0;
                    pAPIInfo->ParamList[pAPIInfo->ParamList[Cnt].dwSizeOfPointedData].dwSizeOfPointedData = 2;
                }
            }
        }
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ParseOptions
// Object: parse function options of a monitoring file
// Parameters :
//     in  : API_INFO *pAPIInfo : pointer to an allocated API_INFO structure
//           TCHAR* pszParameters : string containing all parameters
//           BOOL bAlreadyHooked : flag to know if function was already hooked
//                                 as some options can't be changed when the hook is already installed
//           TCHAR* pszFileName : name of monitoring file (for error report)
//           DWORD dwLineNumber : line number of monitoring file (for error report)
// Return : TRUE on success, FALSE on error (if function mustn't be hooked)
//-----------------------------------------------------------------------------
BOOL __stdcall ParseOptions(API_INFO *pAPIInfo,TCHAR* pszOptions,BOOL bAlreadyHooked,TCHAR* pszFileName,DWORD dwLineNumber)
{
    TCHAR* pszOptionalParameters;
    TCHAR* pszNextOptionalParameters;
    int iScanfRes;
    TCHAR* pszTmp;
    TCHAR pszMsg[3*MAX_PATH];

    // set all optional options to default
    pAPIInfo->ParamDirectionType=DEFAULT_PARAMETER_DIRECTION_SPYING;
    memset(&pAPIInfo->LogBreakWay,0,sizeof(API_LOG_BREAK_WAY));

    //////////////////////////////////////
    // specify optional API informations
    //////////////////////////////////////
    pszOptionalParameters=pszOptions;
    pszNextOptionalParameters=pszOptionalParameters;
    // if optional parameter is specified
    while (pszNextOptionalParameters)
    {
        pszOptionalParameters=pszNextOptionalParameters;

        // find next optional parameter
        pszNextOptionalParameters=_tcschr(pszOptionalParameters,FIELDS_SEPARATOR);
        if (pszNextOptionalParameters)
        {
            *pszNextOptionalParameters=0;
            pszNextOptionalParameters++;
        }

        // trim
        pszOptionalParameters=CTrimString::TrimString(pszOptionalParameters);
        // find type
        if (_tcsicmp(pszOptionalParameters,PARAM_DIR_IN_NAME)==0)
            pAPIInfo->ParamDirectionType=PARAM_DIR_IN;
        else if (_tcsicmp(pszOptionalParameters,PARAM_DIR_OUT_NAME)==0)
            pAPIInfo->ParamDirectionType=PARAM_DIR_OUT;
        else if (_tcsicmp(pszOptionalParameters,PARAM_DIR_INOUT_NAME)==0)
            pAPIInfo->ParamDirectionType=PARAM_DIR_INOUT;
        else if (_tcsicmp(pszOptionalParameters,PARAM_DIR_NONE_NAME)==0)
            pAPIInfo->ParamDirectionType=PARAM_DIR_NONE;
        else if (_tcsicmp(pszOptionalParameters,PARAM_DIR_INNORET_NAME)==0)
            pAPIInfo->ParamDirectionType=PARAM_DIR_IN_NO_RETURN;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_BEFORE_CALL)==0)
            pAPIInfo->LogBreakWay.BreakBeforeCall=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_AFTER_CALL)==0)
            pAPIInfo->LogBreakWay.BreakAfterCall=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_BEFORE_AND_AFTER_CALL)==0)
        {
            pAPIInfo->LogBreakWay.BreakBeforeCall=1;
            pAPIInfo->LogBreakWay.BreakAfterCall=1;
        }
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_AFTER_CALL_IF_NULL_RESULT)==0)
            pAPIInfo->LogBreakWay.BreakAfterCallIfNullResult=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_AFTER_CALL_IF_NOT_NULL_RESULT)==0)
            pAPIInfo->LogBreakWay.BreakAfterCallIfNotNullResult=1; 

        else if (_tcsicmp(pszOptionalParameters,OPTION_LOG_IF_NULL_RESULT)==0)
            pAPIInfo->LogBreakWay.LogIfNullResult=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_LOG_IF_NOT_NULL_RESULT)==0)
            pAPIInfo->LogBreakWay.LogIfNotNullResult=1; 

        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_LOG_INPUT_AFTER)==0)
            pAPIInfo->LogBreakWay.BreakLogInputAfter=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_LOG_OUTPUT_AFTER)==0)
            pAPIInfo->LogBreakWay.BreakLogOutputAfter=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_LOG_ON_FAILURE)==0)
            pAPIInfo->LogBreakWay.LogOnFailure=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_LOG_ON_SUCCESS)==0)
            pAPIInfo->LogBreakWay.LogOnSuccess=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_BREAK_ON_FAILURE)==0)
            pAPIInfo->LogBreakWay.BreakOnFailure=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_BREAK_BREAK_ON_SUCCESS)==0)
            pAPIInfo->LogBreakWay.BreakOnSuccess=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_NULL_RET)==0)
            pAPIInfo->LogBreakWay.FailureIfNullRet=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_NOT_NULL_RET)==0)
            pAPIInfo->LogBreakWay.FailureIfNotNullRet=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_NEGATIVE_RET_VALUE)==0)
            pAPIInfo->LogBreakWay.FailureIfNegativeRetValue=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_POSITIVE_RET_VALUE)==0)
            pAPIInfo->LogBreakWay.FailureIfPositiveRetValue=1;
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_RET_VALUE,_tcslen(OPTION_FAILURE_IF_RET_VALUE))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_RET_VALUE)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfRetValue=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_NOT_RET_VALUE,_tcslen(OPTION_FAILURE_IF_NOT_RET_VALUE))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_NOT_RET_VALUE)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfNotRetValue=1;
        }
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_NULL_FLOATING_RET)==0)
            pAPIInfo->LogBreakWay.FailureIfNullFloatingRet=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_NOT_NULL_FLOATING_RET)==0)
            pAPIInfo->LogBreakWay.FailureIfNotNullFloatingRet=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_FLOATING_NEGATIVE_RET_VALUE)==0)
            pAPIInfo->LogBreakWay.FailureIfFloatingNegativeRetValue=1;
        else if (_tcsicmp(pszOptionalParameters,OPTION_FAILURE_IF_FLOATING_POSITIVE_RET_VALUE)==0)
            pAPIInfo->LogBreakWay.FailureIfFloatingPositiveRetValue=1;
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_FLOATINGRET_VALUE,_tcslen(OPTION_FAILURE_IF_FLOATINGRET_VALUE))==0)
        {   
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_FLOATINGRET_VALUE)];
            iScanfRes=_stscanf(pszTmp,_T("%le"),&pAPIInfo->FloatingFailureValue);
            if (iScanfRes>0)
                pAPIInfo->LogBreakWay.FailureIfFloatingRetValue=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_NOT_FLOATING_RET_VALUE,_tcslen(OPTION_FAILURE_IF_NOT_FLOATING_RET_VALUE))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_NOT_FLOATING_RET_VALUE)];
            iScanfRes=_stscanf(pszTmp,_T("%le"),&pAPIInfo->FloatingFailureValue);
            if (iScanfRes>0)
                pAPIInfo->LogBreakWay.FailureIfNotFloatingRetValue=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_SIGNED_RET_LESS,_tcslen(OPTION_FAILURE_IF_SIGNED_RET_LESS))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_SIGNED_RET_LESS)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfSignedRetLess=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_SIGNED_RET_UPPER,_tcslen(OPTION_FAILURE_IF_SIGNED_RET_UPPER))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_SIGNED_RET_UPPER)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfSignedRetUpper=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_UNSIGNED_RET_LESS,_tcslen(OPTION_FAILURE_IF_UNSIGNED_RET_LESS))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_UNSIGNED_RET_LESS)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfUnsignedRetLess=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_UNSIGNED_RET_UPPER,_tcslen(OPTION_FAILURE_IF_UNSIGNED_RET_UPPER))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_UNSIGNED_RET_UPPER)];
            if (CStringConverter::StringToPBYTE(pszTmp,&pAPIInfo->FailureValue))
                pAPIInfo->LogBreakWay.FailureIfUnsignedRetUpper=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_FLOATING_RET_LESS,_tcslen(OPTION_FAILURE_IF_FLOATING_RET_LESS))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_FLOATING_RET_LESS)];
            iScanfRes=_stscanf(pszTmp,_T("%le"),&pAPIInfo->FloatingFailureValue);
            if (iScanfRes>0)
                pAPIInfo->LogBreakWay.FailureIfFloatingRetLess=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_FLOATING_RET_UPPER,_tcslen(OPTION_FAILURE_IF_FLOATING_RET_UPPER))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_FLOATING_RET_UPPER)];
            iScanfRes=_stscanf(pszTmp,_T("%le"),&pAPIInfo->FloatingFailureValue);
            if (iScanfRes>0)
                pAPIInfo->LogBreakWay.FailureIfFloatingRetUpper=1;
        }

        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_LAST_ERROR_VALUE_VALUE,_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_VALUE))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_VALUE)];
            if (CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->FailureLastErrorValue))
                pAPIInfo->LogBreakWay.FailureIfLastErrorValue=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_NOT_LAST_ERROR_VALUE_VALUE,_tcslen(OPTION_FAILURE_IF_NOT_LAST_ERROR_VALUE_VALUE))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_NOT_LAST_ERROR_VALUE_VALUE)];
            if (CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->FailureLastErrorValue))
                pAPIInfo->LogBreakWay.FailureIfNotLastErrorValue=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_LAST_ERROR_VALUE_LESS,_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_LESS))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_LESS)];
            if (CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->FailureLastErrorValue))
                pAPIInfo->LogBreakWay.FailureIfLastErrorValueLess=1;
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FAILURE_IF_LAST_ERROR_VALUE_UPPER,_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_UPPER))==0)
        {
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FAILURE_IF_LAST_ERROR_VALUE_UPPER)];
            if (CStringConverter::StringToDWORD(pszTmp,&pAPIInfo->FailureLastErrorValue))
                pAPIInfo->LogBreakWay.FailureIfLastErrorValueUpper=1;
        }   

        else if (_tcsicmp(pszOptionalParameters,OPTION_BLOCKING_CALL)==0)
            pAPIInfo->BlockingCall=TRUE;
        else if (_tcsnicmp(pszOptionalParameters,OPTION_DISPLAY_NAME,_tcslen(OPTION_DISPLAY_NAME))==0)
        {
            TCHAR* pc;

            // point to undecorated name
            pszOptionalParameters+=_tcslen(OPTION_DISPLAY_NAME);

            // allocate memory (free mem and return on error)
            pc = (TCHAR*)HeapAlloc(ApiOverrideHeap, 0, (_tcslen(pszOptionalParameters) + 1)*sizeof(TCHAR));
            if (pc == NULL)
            {
                DynamicMessageBoxInDefaultStation(NULL,_T("Memory allocation error"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                return FALSE;
            }
            // free previously allocated api name
            HeapFree(ApiOverrideHeap, 0, pAPIInfo->szAPIName);

            pAPIInfo->szAPIName=pc;

            // copy undecorated name
            _tcscpy(pAPIInfo->szAPIName, pszOptionalParameters);
        }
        else if (_tcsnicmp(pszOptionalParameters,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE,_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE))==0)
        {
            // don't use pAPIInfo->FirstBytesCanExecuteAnywhereSize directly
            DWORD dw;
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE)];
            if (CStringConverter::StringToDWORD(pszTmp,&dw))
            {
                if (!bAlreadyHooked)
                    pAPIInfo->FirstBytesCanExecuteAnywhereSize=dw;
            }
        }

        else if (_tcsicmp(pszOptionalParameters,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE)==0)
        {
            if (!bAlreadyHooked)
                pAPIInfo->FirstBytesCanExecuteAnywhereSize=OPCODE_REPLACEMENT_SIZE;
        }

        else if (_tcsnicmp(pszOptionalParameters,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE_SIZE,_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE_SIZE))==0)
        {
            // don't use pAPIInfo->FirstBytesCanExecuteAnywhereSize directly
            DWORD dw;
            pszTmp=&pszOptionalParameters[_tcslen(OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE_SIZE)];
            if (CStringConverter::StringToDWORD(pszTmp,&dw))
            {
                if (!bAlreadyHooked)
                {
                    pAPIInfo->FirstBytesCanExecuteAnywhereNeedRelativeAddressChange=TRUE;
                    pAPIInfo->FirstBytesCanExecuteAnywhereSize=dw;
                }
            }
        }

        else if (_tcsicmp(pszOptionalParameters,OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_WITH_RELATIVE_ADDRESS_CHANGE)==0)
        {
            pAPIInfo->FirstBytesCanExecuteAnywhereNeedRelativeAddressChange=TRUE;
            if (!bAlreadyHooked)
                pAPIInfo->FirstBytesCanExecuteAnywhereSize=OPCODE_REPLACEMENT_SIZE;
        }

        else if (_tcsicmp(pszOptionalParameters,OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)==0)
        {
            if (!bAlreadyHooked)
                pAPIInfo->FirstBytesCanExecuteAnywhereSize=(DWORD)-1;
        }
        else if (_tcsicmp(pszOptionalParameters,OPTION_DONT_CHECK_MODULES_FILTERS)==0)
        {
            pAPIInfo->DontCheckModulesFilters = TRUE;
        }        
         
        else
        {
            if (pszFileName)
            {
                _sntprintf(pszMsg,3*MAX_PATH,
                    _T("Unknown option %s used for function %s in file %s at line %u"),
                    pszOptionalParameters,
                    pAPIInfo->szAPIName,
                    pszFileName,
                    dwLineNumber);
            }
            else
            {
                _sntprintf(pszMsg,3*MAX_PATH,
                    _T("Unknown option %s used for function %s"),
                    pszOptionalParameters,
                    pAPIInfo->szAPIName
                    );
            }

            DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        }
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: UnloadMonitoringFile
// Object: try to unload all associated hook for this monitoring file
// Parameters :
//     in  : TCHAR* pszFileName : monitoring file name (full path)
// Return : TRUE if file is not hooked (even if error), FALSE if it is still loaded
//-----------------------------------------------------------------------------
BOOL UnloadMonitoringFile(TCHAR* pszFileName)
{
    MONITORING_FILE_INFOS* pMonitoringFileInfos;
    API_INFO *pAPIInfo;
    CLinkListItem* pItemAPIInfo;
    CLinkListItem* pNextItemAPIInfo;
    BOOL bRet;
    BOOL bUnHookSuccessFull=TRUE;
    TCHAR pszMsg[MAX_PATH];

    if (*pszFileName==0)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Empty file name"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // get file ID
    if (!GetMonitoringFileInfos(pszFileName,&pMonitoringFileInfos))
    {
        _sntprintf(pszMsg,MAX_PATH,_T("File %s was not loaded"),pszFileName);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return TRUE;
    }

    // look in pLinkListAPIInfos for hooks with matching pMonitoringFileInfos
    
    bRet=TRUE;
    pLinkListAPIInfos->Lock();
begin:
    for (pItemAPIInfo = pLinkListAPIInfos->Head;pItemAPIInfo;pItemAPIInfo = pNextItemAPIInfo)
    {
        pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
        pNextItemAPIInfo=pItemAPIInfo->NextItem;//UnhookAPIFunction release pItemAPIInfo data
        if (pAPIInfo->pMonitoringFileInfos == pMonitoringFileInfos)
        {

            if (pAPIInfo->HookType==HOOK_TYPE_NET)
            {
                if (pNetManager)
                    pNetManager->RemoveHookNetMonitoringDefinition(pMonitoringFileInfos);
            }

            pAPIInfo->pMonitoringFileInfos=0;

            // we have to unlock list to avoid dead lock
            // as pLinkListAPIInfos is used in UnHookIfPossible
            pLinkListAPIInfos->Unlock();

            // unhook item if possible (no other hooking way attached to pAPIInfo)
            bRet=UnHookIfPossible(pItemAPIInfo,TRUE);
            bUnHookSuccessFull=bUnHookSuccessFull && bRet;

            pLinkListAPIInfos->Lock();

            // if next pointer has been destroyed during the unlocking operation
            if (!pLinkListpMonitoringFileInfos->IsItemStillInList(pNextItemAPIInfo,TRUE))
                goto begin;
        }
    }
    pLinkListAPIInfos->Unlock();

    if (bUnHookSuccessFull)
    {
        // wait for all hook to be free
        WaitForAllHookFreeing();
        
        // remove pItemMonitoringFileInfo
        pLinkListpMonitoringFileInfos->RemoveItemFromItemData(pMonitoringFileInfos);

    }
    return bUnHookSuccessFull;
}

//-----------------------------------------------------------------------------
// Name: StrByteArrayToByteArray
// Object: convert a string byte array to a byte array
// Parameters :
//      in: TCHAR* pc : pointer string to convert like "AF12", "AD-FF-CD" "AB fe"
//      out : DWORD* pdwSize size in byte of return array
// Return : NULL on error, pointer to byte array else
//-----------------------------------------------------------------------------
PBYTE StrByteArrayToByteArray(TCHAR* pc,DWORD* pdwSize)
{
    // remove non alpha num char
    DWORD PosInArray;
    DWORD PosInRemovedArray=0;
    BYTE* Buffer;
    DWORD Size=(DWORD)_tcslen(pc);
    for (PosInArray=0;PosInArray<Size;PosInArray++)
    {
        if (((pc[PosInArray]>='a')&&(pc[PosInArray]<='f'))
            ||((pc[PosInArray]>='A')&&(pc[PosInArray]<='F'))
            ||((pc[PosInArray]>='0')&&(pc[PosInArray]<='9'))
            )
        {
            pc[PosInRemovedArray]=pc[PosInArray];
            PosInRemovedArray++;
        }
    }
    // ends string
    pc[PosInRemovedArray]=0;

    // compute size of data
    *pdwSize=(DWORD)_tcslen(pc)/2;
    Buffer=(BYTE*)HeapAlloc(ApiOverrideHeap,0,*pdwSize);
    if (!Buffer)
    {
        *pdwSize=0;
        return NULL;
    }
    // translate hex data to Byte
    for (PosInArray=0;PosInArray<*pdwSize;PosInArray++)
        Buffer[PosInArray]=CStrToHex::StrHexToByte(&pc[PosInArray*2]);

    return Buffer;
}

//-----------------------------------------------------------------------------
// Name: UnloadAllMonitoringFiles
// Object: unload all monitoring files
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnloadAllMonitoringFiles()
{
    if (!pLinkListpMonitoringFileInfos)
        return TRUE;

    CLinkListItem* pItemAPIInfo;
    CLinkListItem* pNextItemAPIInfo;
    BOOL bSuccess=TRUE;
    BOOL bRet;
    MONITORING_FILE_INFOS* pMonitoringFileInfo;
    TCHAR pszFileName[MAX_PATH];

    // free pLinkListpMonitoringFileInfos
    pLinkListpMonitoringFileInfos->Lock();
begin:
    for (pItemAPIInfo =pLinkListpMonitoringFileInfos->Head;pItemAPIInfo;pItemAPIInfo = pNextItemAPIInfo)
    {
        pMonitoringFileInfo=(MONITORING_FILE_INFOS*)pItemAPIInfo->ItemData;

        pNextItemAPIInfo = pItemAPIInfo->NextItem; // because pItemAPIInfo can be free by UnloadMonitoringFile

        if (GetMonitoringFileName(pMonitoringFileInfo,pszFileName))
        {
            pLinkListpMonitoringFileInfos->Unlock();

            // list must be unlocked because lock is needed by UnloadMonitoringFile
            bRet=UnloadMonitoringFile(pszFileName);
            bSuccess=bSuccess&&bRet;

            pLinkListpMonitoringFileInfos->Lock();
            if (!pLinkListpMonitoringFileInfos->IsItemStillInList(pNextItemAPIInfo,TRUE))
                goto begin;
        }
        
    }
    pLinkListpMonitoringFileInfos->Unlock();

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: GetMonitoringFileName
// Object: get monitoring file name from it's Id
// Parameters :
//     in : MONITORING_FILE_INFOS* pMonitoringFileInfo : pointer to monitoring file infos
//     out  : TCHAR* pszFileName : dll file name (full path) (must be already allocated, not allocated inside func)
//                                 Buffer size should be >= MAX_PATH
// Return : TRUE on success, FALSE on failure or if monitoring file is a file handled by HookCom.dll
//-----------------------------------------------------------------------------
BOOL GetMonitoringFileName(MONITORING_FILE_INFOS* pMonitoringFileInfo,TCHAR* pszFileName)
{
    *pszFileName=0;

    if(!pLinkListpMonitoringFileInfos)
        return FALSE;

    _tcscpy(pszFileName,pMonitoringFileInfo->szFileName);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetMonitoringFileInfos
// Object: get monitoring file id from it's name
// Parameters :
//     in : TCHAR* pszFileName : dll file name
//     out  :  MONITORING_FILE_INFOS** ppMonitoringFileInfo : pointer to monitoring file info passed by address
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL GetMonitoringFileInfos(TCHAR* pszFileName,MONITORING_FILE_INFOS** ppMonitoringFileInfo)
{
    if(pLinkListpMonitoringFileInfos==NULL)
    {
        *ppMonitoringFileInfo=NULL;
        return FALSE;
    }
    
    CLinkListItem* pLinkedListItem;
    // look throw pLinkedListItem
    pLinkListpMonitoringFileInfos->Lock();
    for (pLinkedListItem = pLinkListpMonitoringFileInfos->Head;pLinkedListItem;pLinkedListItem = pLinkedListItem->NextItem)
    {
        *ppMonitoringFileInfo=(MONITORING_FILE_INFOS*)pLinkedListItem->ItemData;

        if (_tcsicmp(pszFileName,(*ppMonitoringFileInfo)->szFileName)==0)
        {
            pLinkListpMonitoringFileInfos->Unlock();
            return TRUE;
        }
        
    }
    pLinkListpMonitoringFileInfos->Unlock();

    *ppMonitoringFileInfo=NULL;
    return FALSE;
}