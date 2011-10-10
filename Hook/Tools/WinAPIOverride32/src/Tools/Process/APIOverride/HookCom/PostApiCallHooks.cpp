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
// Object: manage COM object creation hooking
//         parse COM object creation file and hook all specified functions
//-----------------------------------------------------------------------------

#include "postapicallhooks.h"

extern HOOK_COM_INIT HookComInfos;
extern HINSTANCE DllhInstance;
extern DWORD dwSystemPageSize;

//-----------------------------------------------------------------------------
// Name: CPostApiCallHooks
// Object: constructor
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
CCOMCreationPostApiCallHooks::CCOMCreationPostApiCallHooks(void)
{
    this->bStarted=FALSE;
    this->pLinkListPostApiCallHooks=new CLinkList(sizeof(LINKLIST_POSTAPICALL_HOOKDATA));
}

//-----------------------------------------------------------------------------
// Name: CPostApiCallHooks
// Object: destructor
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
CCOMCreationPostApiCallHooks::~CCOMCreationPostApiCallHooks(void)
{
    this->Stop();
    if (this->pLinkListPostApiCallHooks)
    {
        delete this->pLinkListPostApiCallHooks;
        this->pLinkListPostApiCallHooks=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: IsStarted
// Object: get com creation api spying state
// Parameters :
// Return : TRUE if com creation api spying is started
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::IsStarted()
{
    return this->bStarted;
}


//-----------------------------------------------------------------------------
// Name: Start
// Object: parse config file and hook specified COM object creation functions
// Parameters :
//          in : TCHAR* pszConfigFileName : configuration file name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::Start(TCHAR* pszConfigFileName)
{
    // if COM object creation is already started
    if (this->bStarted)
        return TRUE;

    // COM object creation is started
    this->bStarted=TRUE;

    // parse file (see ParseConfigFileLineCallBack)
    if (!CTextFile::ParseLines(pszConfigFileName,
                            HookComInfos.hevtFreeProcess,
                            CCOMCreationPostApiCallHooks::ParseConfigFileLineCallBack,
                            this))
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Stop
// Object: stop hooking COM object creation functions 
// Parameters :
//          in : TCHAR* pszConfigFileName : configuration file name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::Stop()
{
    // if COM object creation is already stopped
    if (!this->bStarted)
        return TRUE;

    BOOL bRet=TRUE;
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    LINKLIST_POSTAPICALL_HOOKDATA* pPostApiCallHookData;

    // remove hook for each function
    // for each item of this->pLinkListPostApiCallHooks
    this->pLinkListPostApiCallHooks->Lock();
    for (pItem=this->pLinkListPostApiCallHooks->Head;pItem;pItem=pNextItem)
    {
        pNextItem=pItem->NextItem;
        pPostApiCallHookData=(LINKLIST_POSTAPICALL_HOOKDATA*)pItem->ItemData;
        // remove post api call hook
        if (HookComInfos.RemovePostApiCallCallBack(pPostApiCallHookData->pItemAPIInfo,(pfPostApiCallCallBack)pPostApiCallHookData->PostApiCallFunctionPointer,TRUE))
        {
            // in case of success, remove item from linked list
            this->pLinkListPostApiCallHooks->RemoveItem(pItem,TRUE);
        }
        else
            bRet=FALSE;
        
    }
    this->pLinkListPostApiCallHooks->Unlock();

    // COM object creation is stopped
    this->bStarted=FALSE;

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: ParseConfigFileLineCallBack
// Object: parse a line of configuration file 
// Parameters :
//          in : TCHAR* pszFileName : configuration file name
//               TCHAR* Line :  Line content
//               DWORD dwLineNumber : Line number
//               LPVOID UserParam : CPostApiCallHooks object
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::ParseConfigFileLineCallBack(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    TCHAR* pszAPIDefinition;
    TCHAR* pszModuleName;
    TCHAR* pszFunctionName;
    TCHAR* pszOption;
    BOOL ExeDllInternal;
    BOOL bFunctionPointer;
    PBYTE pFunction;
    DWORD StackSize=0;
    CCOMCreationPostApiCallHooks* pPostApiCallHooks;

    LINKLIST_POSTAPICALL_HOOKDATA HookData;
    pPostApiCallHooks=(CCOMCreationPostApiCallHooks*)UserParam;


    // DllName|FunctionName|Option1|Option2...|OptionN
    // WinApiOverrideDllNameLike|FunctionName|Option1|Option2...|OptionN

    // trim string buffer
    pszAPIDefinition=CTrimString::TrimString(Line);

    // if empty or commented line
    if ((*pszAPIDefinition==0)||(*pszAPIDefinition==';')||(*pszAPIDefinition=='!'))
        // continue parsing
        return TRUE;

    // get module name
    pszModuleName = pszAPIDefinition;

    // get pointer to function description (without module name)
    pszFunctionName = _tcschr(pszModuleName, FIELDS_SEPARATOR);
    
    // if api name not found
    if (!pszFunctionName)
    {
        TCHAR pszMsg[3*MAX_PATH];
        // parsing error. Ask if we should continue parsing
        _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d\r\n%s\r\nContinue parsing ?"),pszFileName,dwLineNumber,pszAPIDefinition);
        if (HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
            // continue parsing
            return TRUE;
        else
            // stop parsing
            return FALSE;
    }

    // we have assume that api name was found

    // ends pszModuleName string by replacing FIELDS_SEPARATOR by \0
    *pszFunctionName = 0;

    // get char after FIELDS_SEPARATOR for pszFunctionName
    pszFunctionName++;

    // get pointer to options
    pszOption = _tcschr(pszFunctionName, FIELDS_SEPARATOR);
    if (pszOption)
    {
        // end function name
        *pszOption=0;
        // point to first option value
        pszOption++;
    }


    // get function address
    pFunction=HookComInfos.GetWinAPIOverrideFunctionDescriptionAddress(pszModuleName,pszFunctionName,&ExeDllInternal,&bFunctionPointer);
    if (IsBadCodePtr((FARPROC)pFunction))
    {
        TCHAR pszMsg[3*MAX_PATH];
        _sntprintf(pszMsg,3*MAX_PATH,_T("Bad code pointer for function %s in dll\r\n%s\r\nFunction not hooked"),pszFunctionName,pszModuleName);
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return TRUE;
    }

    CLSID ClsidNull=CLSID_NULL;
    if (!pPostApiCallHooks->ParseComCreationParameters(pszOption,&ClsidNull,TRUE,&HookData,&StackSize))
    {
        TCHAR pszMsg[3*MAX_PATH];
        // parsing error. Ask if we should continue parsing
        _sntprintf(pszMsg,3*MAX_PATH,_T("Some options are missing or contain bad values in %s at line: %d\r\nContinue parsing ?"),pszFileName,dwLineNumber);
        if (HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
            // continue parsing
            return TRUE;
        else
            // stop parsing
            return FALSE;
    }

    // install hook
    pPostApiCallHooks->InstallPostHook(pszModuleName,pszFunctionName,pFunction,bFunctionPointer,StackSize,&HookData);

    // continue file parsing
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ParseComCreationParameters
// Object: parse com creation parameters
// Parameters :
//     in  : TCHAR* Parameters : com creation parameters
//           BOOL StackSizeMustBeSpecified : TRUE if stack size is requiered
//     out : LINKLIST_POSTAPICALL_HOOKDATA* pHookData : decoded informations WARNING struct content is reset
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::ParseComCreationParameters(TCHAR* Parameters,CLSID* pCurrentCLSID,BOOL StackSizeMustBeSpecified,OUT LINKLIST_POSTAPICALL_HOOKDATA* pHookData,OUT DWORD* pStackSize)
{
    BOOL SingleComSet;
    BOOL MultiComSet;
    BOOL MultiComCountSet;
    BOOL StackSizeSet;
    BOOL CLSIDSet;
    BOOL IIDSet;
    BOOL bComOptionSet;

    memset (pHookData,0,sizeof(LINKLIST_POSTAPICALL_HOOKDATA));
    *pStackSize=0;

    SingleComSet=FALSE;
    MultiComSet=FALSE;
    MultiComCountSet=FALSE;
    StackSizeSet=FALSE;
    CLSIDSet=FALSE;
    IIDSet=FALSE;
    TCHAR* pszOption=Parameters;
    TCHAR* pszNextOption=NULL;
    TCHAR* psz;

    // parse options
    while (pszOption)
    {
        pszNextOption=_tcschr(pszOption+1, FIELDS_SEPARATOR);

        if (pszNextOption)
        {
            // replace field separator by \0 and so ends pszOption string
            *pszNextOption=0;
            // point to next char
            pszNextOption++;
        }

        // remove spaces
        pszOption = CTrimString::TrimString(pszOption);

        // look for stack size
        if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_STACK_SIZE,_tcslen(COM_OBJECT_CREATION_API_STACK_SIZE))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_STACK_SIZE)];
            StackSizeSet=CStringConverter::StringToDWORD(psz,pStackSize);
        }
        // look for single object stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_POBJECT_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_POBJECT_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_POBJECT_STACK_INDEX)];
            SingleComSet=CStringConverter::StringToDWORD(psz,&pHookData->COMObjectStackIndex);
            pHookData->COMObjectStackIndexSet=SingleComSet;
        }
        // look for MULTI_QI* stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_PMULTI_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_PMULTI_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_PMULTI_STACK_INDEX)];
            MultiComSet=CStringConverter::StringToDWORD(psz,&pHookData->COMArrayStackIndex);
            pHookData->COMArrayStackIndexSet=MultiComSet;
            // MULTI_QI contain IID definitions
            IIDSet=MultiComSet;
        }
        // look for MULTI_QI* count stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_PMULTI_SIZE_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_PMULTI_SIZE_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_PMULTI_SIZE_STACK_INDEX)];
            MultiComCountSet=CStringConverter::StringToDWORD(psz,&pHookData->COMArrayCountStackIndex);
        }
        // look for ObjectIsReturnedValue
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_POBJECT_IS_RETURNED_VALUE,_tcslen(COM_OBJECT_CREATION_API_POBJECT_IS_RETURNED_VALUE))==0)
        {
            SingleComSet=TRUE;
            pHookData->COMObjectPtrInReturnedValue=TRUE;
        }
  
        // look for clsid stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_CLSID_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_CLSID_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_CLSID_STACK_INDEX)];
            CLSIDSet=CStringConverter::StringToDWORD(psz,&pHookData->CLSIDStackIndex);
            pHookData->CLSIDInfos=HOOK_DATA_VALUE_INFO_BY_STACK;
        }
        // clsid ref stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_REFCLSID_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_REFCLSID_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_REFCLSID_STACK_INDEX)];
            CLSIDSet=CStringConverter::StringToDWORD(psz,&pHookData->CLSIDStackIndex);
            pHookData->CLSIDInfos=HOOK_DATA_VALUE_INFO_BY_STACK_REF;
        }
        // clsid value
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_CLSID_VALUE,_tcslen(COM_OBJECT_CREATION_API_CLSID_VALUE))==0)
        {
            pHookData->CLSIDInfos=HOOK_DATA_VALUE_INFO_BY_VALUE;
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_CLSID_VALUE)];

            // if "CLSIDValue=CurrentObjectCLSID"
            if (_tcsnicmp(psz,COM_OBJECT_CREATION_API_CLSID_VALUE_CURRENT,_tcslen(COM_OBJECT_CREATION_API_CLSID_VALUE_CURRENT))==0)
            {
                // if provided clsid is not null
                if (!IsEqualCLSID(CLSID_NULL,*pCurrentCLSID))
                {
                    CLSIDSet=TRUE;
                    memcpy(&pHookData->CLSIDValue,pCurrentCLSID,sizeof(CLSID));
                }
            }
            else
            {
                CLSIDSet=CGUIDStringConvert::CLSIDFromTchar(psz,&pHookData->CLSIDValue);
            }
        }
        // iid stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_IID_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_IID_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_IID_STACK_INDEX)];
            IIDSet=CStringConverter::StringToDWORD(psz,&pHookData->IIDStackIndex);
            pHookData->IIDInfos=HOOK_DATA_VALUE_INFO_BY_STACK;
        }
        // iid ref stack index
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_REFIID_STACK_INDEX,_tcslen(COM_OBJECT_CREATION_API_REFIID_STACK_INDEX))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_REFIID_STACK_INDEX)];
            IIDSet=CStringConverter::StringToDWORD(psz,&pHookData->IIDStackIndex);
            pHookData->IIDInfos=HOOK_DATA_VALUE_INFO_BY_STACK_REF;
        }
        // iid value
        else if (_tcsnicmp(pszOption,COM_OBJECT_CREATION_API_IID_VALUE,_tcslen(COM_OBJECT_CREATION_API_IID_VALUE))==0)
        {
            psz=&pszOption[_tcslen(COM_OBJECT_CREATION_API_IID_VALUE)];
            IIDSet=CGUIDStringConvert::IIDFromTchar(psz,&pHookData->IIDValue);
            pHookData->IIDInfos=HOOK_DATA_VALUE_INFO_BY_VALUE;
        }

        pszOption=pszNextOption;
    }

    // check if options are fully defined for single or multiple objects creation
    bComOptionSet=(SingleComSet || (MultiComSet && MultiComCountSet))
                   && IIDSet && CLSIDSet; 

    if ( (!bComOptionSet) 
         || ( (!StackSizeSet)&& StackSizeMustBeSpecified ) 
       )
    {
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InstallPostHook
// Object: install post api call hook for com object creation hook functions
// Parameters :
//     in  : TCHAR* DllName : name of dll in which is located api function to hook
//           TCHAR* ApiName : name of api function to hook
//           PBYTE HookedFunctionAddress : address of function to hook
//           BOOL bFunctionPointer : if patch address is a pointer
//           DWORD StackSize : stack size of the function to hook
//           LINKLIST_POSTAPICALL_HOOKDATA* pHookData : information about com hook
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::InstallPostHook(TCHAR* DllName,TCHAR* ApiName,PBYTE HookedFunctionAddress,BOOL bFunctionPointer,DWORD StackSize,LINKLIST_POSTAPICALL_HOOKDATA* pHookData)
{
    BOOL bAlreadyHooked;
    API_INFO* pAPIInfo;
    CLinkListItem* pItem;

    if (!this->bStarted)
        return FALSE;

    // find associated item info
    pHookData->pItemAPIInfo=HookComInfos.GetAssociatedItemAPIInfo(HookedFunctionAddress,&bAlreadyHooked);
    if (!pHookData->pItemAPIInfo)
        return FALSE;

    if (pHookData->COMObjectPtrInReturnedValue)
        pHookData->PostApiCallFunctionPointer=(PBYTE)CCOMCreationPostApiCallHooks::COMObjectCreationUsingReturnedValuePostHook;
    // if single object creation hook
    else if (pHookData->COMObjectStackIndexSet)
        pHookData->PostApiCallFunctionPointer=(PBYTE)CCOMCreationPostApiCallHooks::COMObjectCreationPostHook;
    else if (pHookData->COMArrayStackIndexSet)
        pHookData->PostApiCallFunctionPointer=(PBYTE)CCOMCreationPostApiCallHooks::COMObjectsArrayCreationPostHook;
    else
        return FALSE;

    // get pointer to API_INFO struct
    pAPIInfo=(API_INFO*)pHookData->pItemAPIInfo->ItemData;

    // if function was not hooked
    if (!bAlreadyHooked)
    {
        pAPIInfo->bFunctionPointer=bFunctionPointer;
        // initialize it
        HookComInfos.InitializeApiInfo(pAPIInfo,DllName,ApiName);
        // fill the stack size
        pAPIInfo->StackSize=StackSize;
    }

    // store this pointer
    pHookData->pCCOMCreationPostApiCallHooks=this;
    
    // put pHookData into linked list to keep object data in memory
    pItem=this->pLinkListPostApiCallHooks->AddItem(pHookData);
    if (!pItem)
    {
        // on hook error free pItemAPIInfo
        if (!bAlreadyHooked)
            HookComInfos.FreeApiInfoItem(pHookData->pItemAPIInfo);

        return FALSE;
    }

    if (!HookComInfos.AddPostApiCallCallBack(pHookData->pItemAPIInfo,!bAlreadyHooked,DllhInstance,(pfPostApiCallCallBack)pHookData->PostApiCallFunctionPointer,pItem->ItemData))
    {
        // remove previously added item
        this->pLinkListPostApiCallHooks->RemoveItem(pItem);

        // free pItemAPIInfo
        if (!bAlreadyHooked)
            HookComInfos.FreeApiInfoItem(pHookData->pItemAPIInfo);

        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetCLSID
// Object: retrieve CLSID from stack and pHookData
// Parameters :
//     in  : PBYTE pEspArgs : pointer to esp parameters
//           LINKLIST_POSTAPICALL_HOOKDATA* pHookData : data associated to hook
//     out : CLSID* pClsid : CLSID value passed by address
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::GetCLSID(PBYTE pEspArgs,LINKLIST_POSTAPICALL_HOOKDATA* pHookData,OUT CLSID* pClsid)
{
    CLSID** mppCLSID;
    CLSID* mpCLSID;

    // default CLSID
    *pClsid=CLSID_NULL;

    switch(pHookData->CLSIDInfos)
    {
    case HOOK_DATA_VALUE_INFO_BY_VALUE:
        *pClsid=pHookData->CLSIDValue;
        break;
    case HOOK_DATA_VALUE_INFO_BY_STACK:
        mpCLSID=(CLSID*)(pEspArgs+pHookData->CLSIDStackIndex);
        // esp is always a valid read pointer so
        *pClsid=*mpCLSID;
        break;
    case HOOK_DATA_VALUE_INFO_BY_STACK_REF:
        mppCLSID=(CLSID**)(pEspArgs+pHookData->CLSIDStackIndex);
        // esp is always a valid read pointer so
        mpCLSID=*mppCLSID;
        // check mpCLSID pointer validity
        if (IsBadReadPtr(mpCLSID,sizeof(CLSID)))
            return FALSE;
        *pClsid=*mpCLSID;
        break;
    }

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: GetIID
// Object: retrieve IID from stack and pHookData
// Parameters :
//     in  : PBYTE pEspArgs : pointer to esp parameters
//           LINKLIST_POSTAPICALL_HOOKDATA* pHookData : data associated to hook
//     out : IID* pIid : IID value passed by address
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CCOMCreationPostApiCallHooks::GetIID(PBYTE pEspArgs,LINKLIST_POSTAPICALL_HOOKDATA* pHookData,OUT IID* pIid)
{
    IID** mppIID;
    IID* mpIID;

    // default IID
    *pIid=IID_IUnknown;

    switch(pHookData->IIDInfos)
    {
    case HOOK_DATA_VALUE_INFO_BY_VALUE:
        *pIid=pHookData->IIDValue;
        break;
    case HOOK_DATA_VALUE_INFO_BY_STACK:
        mpIID=(IID*)(pEspArgs+pHookData->IIDStackIndex);
        // esp is always a valid read pointer so
        *pIid=*mpIID;
        break;
    case HOOK_DATA_VALUE_INFO_BY_STACK_REF:
        mppIID=(IID**)(pEspArgs+pHookData->IIDStackIndex);
        // esp is always a valid read pointer so
        mpIID=*mppIID;
        // check mpIID pointer validity
        if (IsBadReadPtr(mpIID,sizeof(IID)))
            return FALSE;
        *pIid=*mpIID;
        break;
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: COMObjectsArrayCreationPostHook
// Object: generic post hook function used for multiple COM objects creation
// Parameters :
//     in  : PBYTE pEspArgs : pointer to esp parameters
//           REGISTERS* pAfterCallRegisters : pointer to registers
//           PVOID UserParam : user param (LINKLIST_POSTAPICALL_HOOKDATA*) associated to function
//     out : 
//     return : TRUE to continue post hook chain
//-----------------------------------------------------------------------------
BOOL __stdcall CCOMCreationPostApiCallHooks::COMObjectsArrayCreationPostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    // if call comes from this dll, avoid to auto-hook it as COM object are 
    // created in this dll only to install single hook
    if (pHookInfos->CallingModuleHandle==(HMODULE)DllhInstance)
        // do nothing and continue post call hook chain
        return TRUE;

    // if function has failed
    if (FAILED(pAfterCallRegisters->eax))
        // do nothing and continue post call hook chain
        return TRUE;

    // check user param
    if (IsBadReadPtr(UserParam,sizeof(LINKLIST_POSTAPICALL_HOOKDATA)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    LINKLIST_POSTAPICALL_HOOKDATA* pHookData=(LINKLIST_POSTAPICALL_HOOKDATA*)UserParam;
    CLSID Clsid;
    IID Iid;

    // get pointer to the number of created objects
    DWORD* pCount=(DWORD*)(pEspArgs+pHookData->COMArrayCountStackIndex);
    if (IsBadReadPtr(pCount,sizeof(DWORD)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    // if no created objects
    if (*pCount==0)
        // continue post call hook chain
        return TRUE;

    // get pointer on the array
    MULTI_QI** ppResults=(MULTI_QI**)(pEspArgs+pHookData->COMArrayStackIndex);

    // check ppResults pointer
    if (IsBadReadPtr(ppResults,sizeof(MULTI_QI*)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    MULTI_QI* pResults=*ppResults;

    // we should have at least one element in the array
    if (IsBadReadPtr(pResults,sizeof(MULTI_QI)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    // get clsid and iid
    if (!CCOMCreationPostApiCallHooks::GetCLSID(pEspArgs,pHookData,&Clsid))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    // for each created object
    for (DWORD cnt=0;cnt<*pCount;cnt++)
    {
        // if object creation has failed
        if (FAILED(pResults[cnt].hr))
            continue;

        if (IsBadReadPtr(pResults[cnt].pIID,sizeof(IID)))
            Iid=IID_IUnknown;
        else
            Iid=*(pResults[cnt].pIID);

        if (IsBadReadPtr(pResults[cnt].pItf,sizeof(IUnknown)))
            continue;

        // set hook for the created object
        SetHookForObject(pResults[cnt].pItf,&Clsid,&Iid);

        // call COM object creation callbacks
        CallCOMObjectCreationCallBacks(&Clsid,&Iid,(PVOID)pResults[cnt].pItf,pHookInfos);
    }

    // continue post call hook chain
    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: COMObjectCreationPostHook
// Object: generic post hook function used for single COM object creation
// Parameters :
//     in  : PBYTE pEspArgs : pointer to esp parameters
//           REGISTERS* pAfterCallRegisters : pointer to registers
//           PVOID UserParam : user param (LINKLIST_POSTAPICALL_HOOKDATA*) associated to function
//     out : 
//     return : TRUE to continue post hook chain
//-----------------------------------------------------------------------------
BOOL __stdcall CCOMCreationPostApiCallHooks::COMObjectCreationPostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    // if call comes from this dll, avoid to auto-hook it as COM object are 
    // created in this dll only to install single hook
    if (pHookInfos->CallingModuleHandle==(HMODULE)DllhInstance)
        // do nothing and continue post call hook chain
        return TRUE;

    // if function has failed
    if (FAILED(pAfterCallRegisters->eax))
        // do nothing and continue post call hook chain
        return TRUE;

    // check user param
    if (IsBadReadPtr(UserParam,sizeof(LINKLIST_POSTAPICALL_HOOKDATA)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    CLSID Clsid;
    IID Iid;
    LINKLIST_POSTAPICALL_HOOKDATA* pHookData=(LINKLIST_POSTAPICALL_HOOKDATA*)UserParam;

    if (IsBadReadPtr(pEspArgs+pHookData->COMObjectStackIndex,sizeof(PBYTE)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    IUnknown** ppInterface=*(IUnknown***)(pEspArgs+pHookData->COMObjectStackIndex);

    // check object pointer
    if (ppInterface==NULL)
    {
        // avoid to call IsBadReadPtr to earn some time

        // continue post call hook chain
        return TRUE;
    }
    if (IsBadReadPtr(ppInterface,sizeof(IUnknown*)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }
    if (IsBadReadPtr(*ppInterface,sizeof(IUnknown)))
        // continue post call hook chain
        return TRUE;

    // get clsid and iid
    if (!CCOMCreationPostApiCallHooks::GetCLSID(pEspArgs,pHookData,&Clsid))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }
    if (!CCOMCreationPostApiCallHooks::GetIID(pEspArgs,pHookData,&Iid))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }


    // set hook for the created object
    SetHookForObject(*ppInterface,&Clsid,&Iid);

    // call COM object creation callbacks
    CallCOMObjectCreationCallBacks(&Clsid,&Iid,(PVOID)*ppInterface,pHookInfos);

    // continue post call hook chain
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: COMObjectCreationUsingReturnedValuePostHook
// Object: generic post hook function used for single COM object creation returned by function
// Parameters :
//     in  : PBYTE pEspArgs : pointer to esp parameters
//           REGISTERS* pAfterCallRegisters : pointer to registers
//           PVOID UserParam : user param (LINKLIST_POSTAPICALL_HOOKDATA*) associated to function
//     out : 
//     return : TRUE to continue post hook chain
//-----------------------------------------------------------------------------
BOOL __stdcall CCOMCreationPostApiCallHooks::COMObjectCreationUsingReturnedValuePostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    // if call comes from this dll, avoid to auto-hook it as COM object are 
    // created in this dll only to install single hook
    if (pHookInfos->CallingModuleHandle==(HMODULE)DllhInstance)
        // do nothing and continue post call hook chain
        return TRUE;

    // if function has failed
    if (pAfterCallRegisters->eax==NULL)
        // do nothing and continue post call hook chain
        return TRUE;

    // check user param
    if (IsBadReadPtr(UserParam,sizeof(LINKLIST_POSTAPICALL_HOOKDATA)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    CLSID Clsid;
    IID Iid;
    LINKLIST_POSTAPICALL_HOOKDATA* pHookData=(LINKLIST_POSTAPICALL_HOOKDATA*)UserParam;

    // check object pointer
    if (IsBadReadPtr((PBYTE)pAfterCallRegisters->eax,sizeof(IUnknown)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }

    IUnknown* pInterface=(IUnknown*)(pAfterCallRegisters->eax);

    // get clsid and iid
    if (!CCOMCreationPostApiCallHooks::GetCLSID(pEspArgs,pHookData,&Clsid))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }
    if (!CCOMCreationPostApiCallHooks::GetIID(pEspArgs,pHookData,&Iid))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call hook chain
        return TRUE;
    }


    // set hook for the created object
    SetHookForObject(pInterface,&Clsid,&Iid);

    // call COM object creation callbacks
    CallCOMObjectCreationCallBacks(&Clsid,&Iid,(PVOID)pInterface,pHookInfos);

    // continue post call hook chain
    return TRUE;
}
