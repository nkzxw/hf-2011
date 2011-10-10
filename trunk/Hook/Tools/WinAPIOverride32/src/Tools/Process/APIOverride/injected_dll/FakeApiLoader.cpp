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
// Object: load fake api dll
//-----------------------------------------------------------------------------

#include "fakeapiloader.h"

#include "COM_Manager.h"
#include "NET_Manager.h"
extern CLinkList* pLinkListAPIInfos;
extern CLinkList* pLinkListpFakeDllInfos;
extern CCOM_Manager* pComManager;
extern CNET_Manager* pNetManager;

typedef struct tagFakingDllUserDefaultAction 
{
    BOOL QuestionAsked;
    BOOL TakeSameAction;
    BOOL UpdateFakeApi;
}FAKING_DLL_USER_DEFAULT_ACTION,*PFAKING_DLL_USER_DEFAULT_ACTION;
FAKING_DLL_USER_DEFAULT_ACTION gFakingDllUserDefaultAction={0};// init all to FALSE

BOOL __stdcall OverridingDllSendDataToPlugin(IN TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize);
BOOL __stdcall OverridingDllSendDataToPluginAndWaitReply(IN TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,OUT PBYTE* pDataFromPlugin,OUT SIZE_T* pDataFromPluginSize,DWORD WaitTimeInMs /* INFINITE for infinite wait */);
void __stdcall OverridingDllSendDataToPluginAndWaitReplyFreeReceivedData(IN PBYTE pDataFromPluginToBeFree);

BOOL __stdcall OverridingDllSendDataToPluginA(CHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    BOOL bRet;
    WCHAR PluginNameW[MAX_PATH];
    CAnsiUnicodeConvert::AnsiToUnicode(PluginName,PluginNameW,MAX_PATH);
    bRet=OverridingDllSendDataToPlugin(PluginNameW,DataToPlugin,DataToPluginSize);
    return bRet;
#else
    return OverridingDllSendDataToPlugin(PluginName,DataToPlugin,DataToPluginSize);
#endif
}
BOOL __stdcall OverridingDllSendDataToPluginW(WCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return OverridingDllSendDataToPlugin(PluginName,DataToPlugin,DataToPluginSize);
#else
    BOOL bRet;
    CHAR PluginNameA[MAX_PATH];
    CAnsiUnicodeConvert::UnicodeToAnsi(PluginName,PluginNameA,MAX_PATH);
    bRet=OverridingDllSendDataToPlugin(PluginNameA,DataToPlugin,DataToPluginSize);
    return bRet;
#endif
}

// if not null DataFromPlugin must be delete if not null
BOOL __stdcall OverridingDllSendDataToPluginAndWaitReplyA(
    CHAR* PluginName,
    IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,
    OUT PBYTE* pDataFromPlugin,OUT SIZE_T* pDataFromPluginSize,
    DWORD WaitTimeInMs /* INFINITE for infinite wait */)
{
#if (defined(UNICODE)||defined(_UNICODE))
    BOOL bRet;
    WCHAR PluginNameW[MAX_PATH];
    CAnsiUnicodeConvert::AnsiToUnicode(PluginName,PluginNameW,MAX_PATH);
    bRet=OverridingDllSendDataToPluginAndWaitReply(PluginNameW,DataToPlugin,DataToPluginSize,pDataFromPlugin,pDataFromPluginSize,WaitTimeInMs);
    return bRet;
#else
    return OverridingDllSendDataToPluginAndWaitReply(PluginName,DataToPlugin,DataToPluginSize,pDataFromPlugin,pDataFromPluginSize,WaitTimeInMs);
#endif
}
BOOL __stdcall OverridingDllSendDataToPluginAndWaitReplyW(
    WCHAR* PluginName,
    IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,
    OUT PBYTE* pDataFromPlugin,OUT SIZE_T* pDataFromPluginSize,
    DWORD WaitTimeInMs /* INFINITE for infinite wait */)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return OverridingDllSendDataToPluginAndWaitReply(PluginName,DataToPlugin,DataToPluginSize,pDataFromPlugin,pDataFromPluginSize,WaitTimeInMs);
#else
    BOOL bRet;
    CHAR PluginNameA[MAX_PATH];
    CAnsiUnicodeConvert::UnicodeToAnsi(PluginName,PluginNameA,MAX_PATH);
    bRet = OverridingDllSendDataToPluginAndWaitReply(PluginNameA,DataToPlugin,DataToPluginSize,pDataFromPlugin,pDataFromPluginSize,WaitTimeInMs);
    return bRet;
#endif
}

APIOVERRIDE_EXPORTED_FUNCTIONS ApiOverrideExportedFuncA =
{
    (tagOverridingDllSendDataToPlugin)OverridingDllSendDataToPluginA,
    (tagOverridingDllSendDataToPluginAndWaitReply)OverridingDllSendDataToPluginAndWaitReplyA,
    OverridingDllSendDataToPluginAndWaitReplyFreeReceivedData
};
APIOVERRIDE_EXPORTED_FUNCTIONS ApiOverrideExportedFuncW =
{
    (tagOverridingDllSendDataToPlugin)OverridingDllSendDataToPluginW,
    (tagOverridingDllSendDataToPluginAndWaitReply)OverridingDllSendDataToPluginAndWaitReplyW,
    OverridingDllSendDataToPluginAndWaitReplyFreeReceivedData
};

//-----------------------------------------------------------------------------
// Name: LoadFakeApiDll
// Object: Load a fake API dll and attach hooks. See Fake API dll example
//         Standard exported func of dll gives a array of address hooking
//         array of type STRUCT_FAKE_API or STRUCT_FAKE_API_WITH_USERPARAM
// Parameters :
//     in  : TCHAR* szFakeAPIDLLFullPath : dll file name (full path)
// Return : TRUE if library is loaded (even if not all functions are loaded),
//          FALSE if library is not loaded at all
//-----------------------------------------------------------------------------
BOOL LoadFakeApiDll(TCHAR* szFakeAPIDLLFullPath)
{
    tagGetFakeAPIArrayFuncPointer pGetFakeAPIArrayFuncPointer=NULL;
    tagGetFakeAPIArrayFuncPointer pGetPreAPIArrayFuncPointer=NULL;
    tagGetFakeAPIArrayFuncPointer pGetPostAPIArrayFuncPointer=NULL;
    tagGetFakeAPIEncoding pGetFakeAPIEncoding;
    tagGetAPIOverrideBuildVersion pGetAPIOverrideBuildVersion;
    int iFakeAPIEncoding=FakeAPIEncodingANSI;
    HMODULE hModule;
    TCHAR pszMsg[3*MAX_PATH];
    BOOL bNoItems;
    CLinkListItem *pItemFakeApiFileInfo;
    int ApiOverrideBuildVersionFramework=1;
    FAKING_DLL_INFOS* pFakingDllInfos;
    DWORD cnt;
    tagFakingDllArray FakingDllArray=FAKING_DLL_ARRAY_FAKING;
    pfCOMObjectCreationCallBack pCOMObjectCreationCallBack=NULL;

    // check dll name
    if (*szFakeAPIDLLFullPath==0)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Empty file name"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // check if lib is already loaded
    if (GetModuleHandle(szFakeAPIDLLFullPath))
    {
        // Disabled for 5.3 version since multiple plugins can ask for a same overriding dll loading
        // so avoid user interaction
        // _sntprintf(pszMsg,3*MAX_PATH,_T("File %s already loaded"),szFakeAPIDLLFullPath);
        // DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // check if dll exists
    if (!CStdFileOperations::DoesFileExists(szFakeAPIDLLFullPath))
    {
        _sntprintf(pszMsg,3*MAX_PATH,_T("File %s not found"),szFakeAPIDLLFullPath);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // load library and get proc address to retrieve PSTRUCT_FAKE_API array
    pGetFakeAPIArrayFuncPointer=(tagGetFakeAPIArrayFuncPointer)GetFuncAddr(szFakeAPIDLLFullPath,GET_FAKE_API_ARRAY_FUNCTION_NAME,&hModule);
    // get proc address of to retrieve dll char encoding
    pGetFakeAPIEncoding=(tagGetFakeAPIEncoding)GetFuncAddr(szFakeAPIDLLFullPath,GET_FAKE_API_ENCODING_FUNCTION_NAME,&hModule);
    // GetAPIOverrideBuildVersion
    pGetAPIOverrideBuildVersion=(tagGetAPIOverrideBuildVersion)GetFuncAddr(szFakeAPIDLLFullPath,GET_APIOVERRIDE_BUILD_VERSION_FUNCTION_NAME,&hModule);

    // check library loading status
    if (hModule==NULL)
    {
        _sntprintf(pszMsg,3*MAX_PATH,_T("Load Library failed for dll %s"),szFakeAPIDLLFullPath);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    if (!pGetAPIOverrideBuildVersion)
    {
        // version 3 lost of backward compatibility
        DynamicMessageBoxInDefaultStation(NULL,
                          _T("This overriding dll is incompatible with new versions of WinAPIOverride.\r\n")
                          _T("You have to rebuild it with new conventions"),
                          _T("Error"),
                          MB_OK|MB_ICONERROR|MB_TOPMOST);
        FreeLibrary(hModule);
        return FALSE;
    }
    // allow to now with which framework fake dll was build
    ApiOverrideBuildVersionFramework=pGetAPIOverrideBuildVersion();

    // func GET_FAKE_API_ARRAY_FUNCTION_NAME not exported
    if (pGetFakeAPIArrayFuncPointer==NULL)
    {
        if (ApiOverrideBuildVersionFramework<4)
        {
            _sntprintf(pszMsg,3*MAX_PATH,_T("Exported function %s not found in dll\r\n %s"),GET_FAKE_API_ARRAY_FUNCTION_NAME,szFakeAPIDLLFullPath);
            DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            FreeLibrary(hModule);
            return FALSE;
        }
    }


    // if we can get encoding params (else iFakeAPIEncoding is defaulted to ANSI for backward compatibility)
    if (pGetFakeAPIEncoding)
        iFakeAPIEncoding=pGetFakeAPIEncoding();

    // if loaded faking dll version was build for version >=4 of WinAPIOverride framework
    if (ApiOverrideBuildVersionFramework>=4)
    {
        // get proc address to retrieve PSTRUCT_FAKE_API_WITH_USERPARAM array for pre api call
        pGetPreAPIArrayFuncPointer=(tagGetFakeAPIArrayFuncPointer)GetFuncAddr(szFakeAPIDLLFullPath,GET_PRE_API_CALL_ARRAY_FUNCTION_NAME,&hModule);
        // get proc address to retrieve PSTRUCT_FAKE_API_WITH_USERPARAM array for post api call
        pGetPostAPIArrayFuncPointer=(tagGetFakeAPIArrayFuncPointer)GetFuncAddr(szFakeAPIDLLFullPath,GET_POST_API_CALL_ARRAY_FUNCTION_NAME,&hModule);

        pCOMObjectCreationCallBack=(pfCOMObjectCreationCallBack)GetFuncAddr(szFakeAPIDLLFullPath,COM_OBJECT_CREATION_CALLBACK_EXPORTED_NAME,&hModule);
    }

    if (ApiOverrideBuildVersionFramework>=5)
    {
        tagInitializeFakeDllFuncPointer pInitializeFakeDllFuncPointer=(tagInitializeFakeDllFuncPointer)GetFuncAddr(szFakeAPIDLLFullPath,INITIALIZE_FAKE_DLL_FUNCTION_NAME,&hModule);
        if (pInitializeFakeDllFuncPointer)
        {
            switch (iFakeAPIEncoding)
            {
            case FakeAPIEncodingUNICODE:
                pInitializeFakeDllFuncPointer(&ApiOverrideExportedFuncW);
                break;
            case FakeAPIEncodingANSI:
                pInitializeFakeDllFuncPointer(&ApiOverrideExportedFuncA);
                break;
            }
        }
    }
    

    PVOID ArrayFakingArray[3];
    memset(ArrayFakingArray,0,3*sizeof(PVOID));

    bNoItems=TRUE;
    // get pointer to faking, pre and post array(s)
    if (pGetFakeAPIArrayFuncPointer)
        ArrayFakingArray[0]=(PVOID)pGetFakeAPIArrayFuncPointer();

    if (pGetPreAPIArrayFuncPointer)
        ArrayFakingArray[1]=(PVOID)pGetPreAPIArrayFuncPointer();

    if (pGetPostAPIArrayFuncPointer)
        ArrayFakingArray[2]=(PVOID)pGetPostAPIArrayFuncPointer();


    if (pCOMObjectCreationCallBack)
        bNoItems=FALSE;
    else
    {
        // check that at least an exported array is defined
        for (cnt=0;cnt<3;cnt++)
        {
            if (ArrayFakingArray[cnt]!=0)
            {
                bNoItems=FALSE;
                break;
            }
        }
    }
    if (bNoItems)
    {
        _sntprintf(pszMsg,3*MAX_PATH,_T("No functions defined in dll %s"),szFakeAPIDLLFullPath);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        FreeLibrary(hModule);
        return FALSE;
    }

    // add an item to the link list pLinkListpFakeDllInfos
    pItemFakeApiFileInfo=pLinkListpFakeDllInfos->AddItem();
    if (pItemFakeApiFileInfo==NULL)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Not enough memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        FreeLibrary(hModule);
        return FALSE;
    }

    // fill item info
    pFakingDllInfos=(FAKING_DLL_INFOS*)pItemFakeApiFileInfo->ItemData;
    pFakingDllInfos->hModule=hModule;
    pFakingDllInfos->pCOMObjectCreationCallBack=pCOMObjectCreationCallBack;
    pFakingDllInfos->ApiOverrideBuildVersionFramework=ApiOverrideBuildVersionFramework;

    // signal dll call to be logged/faked only once for |FirstBytesCanExecuteAnyWhere option
    AddAPIOverrideInternalModule(hModule);

    // for each faking type (faking , pre or post hook)
    // load array
    for (cnt=0;cnt<3;cnt++)
    {
        switch (cnt)
        {
        case 0:
            FakingDllArray=FAKING_DLL_ARRAY_FAKING;
            break;
        case 1:
            FakingDllArray=FAKING_DLL_ARRAY_PRE_HOOK;
            break;
        case 2:
            FakingDllArray=FAKING_DLL_ARRAY_POST_HOOK;
            break;
        }

        // load the array according to it's type
        if (ArrayFakingArray[cnt])
            LoadFakeArray(pFakingDllInfos,szFakeAPIDLLFullPath,ArrayFakingArray[cnt],iFakeAPIEncoding,FakingDllArray,ApiOverrideBuildVersionFramework);
    }

    // add com creation callback
    if (pFakingDllInfos->pCOMObjectCreationCallBack)
    {
        if (pComManager)
            pComManager->AddCOMObjectCreationCallBack(pFakingDllInfos->pCOMObjectCreationCallBack);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LoadFakeArray
// Object: Load a fake API array and attach hooks.
//          generic function for faking, pre or post array
// Parameters :
//     in  : FAKING_DLL_INFOS* pFakingDllInfos : information on faking dll owning the array
//           TCHAR* szFakeAPIDLLFullPath : dll path
//           PVOID FakeArray : faking, pre or post array
//           int iFakeAPIEncoding : encoding type of loaded dll : ansi or unicode
//           tagFakingDllArray FakingDllArray : type of faking array (faking, pre or post)
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL LoadFakeArray(FAKING_DLL_INFOS* pFakingDllInfos,TCHAR* szFakeAPIDLLFullPath,PVOID FakeArray,int iFakeAPIEncoding,tagFakingDllArray FakingDllArray,DWORD ApiOverrideBuildVersionFramework)
{
    int                      Index;
    BOOL                     bNoMoreItems;
    PSTRUCT_FAKE_API_ANSI_WITH_USERPARAM    pFakeAPIAnsi=NULL;
    PSTRUCT_FAKE_API_UNICODE_WITH_USERPARAM pFakeAPIUnicode=NULL;
    STRUCT_FAKE_API_WITH_USERPARAM          FakeApiInfos;
    PBYTE                                   NextArrayItemPointer;
    TCHAR                    pszMsg[3*MAX_PATH];
    DWORD StructSize;
    BOOL bHasUserParam=FALSE;

    if (!FakeArray)
        return FALSE;

    // get size of struct array item
    switch (FakingDllArray)
    {
    case FAKING_DLL_ARRAY_FAKING:
    default:
        if (iFakeAPIEncoding==FakeAPIEncodingANSI)
            StructSize=sizeof(STRUCT_FAKE_API_ANSI);
        else
            StructSize=sizeof(STRUCT_FAKE_API_UNICODE);

        bHasUserParam=FALSE;

        break;
    case FAKING_DLL_ARRAY_PRE_HOOK:
    case FAKING_DLL_ARRAY_POST_HOOK:

        if (iFakeAPIEncoding==FakeAPIEncodingANSI)
            StructSize=sizeof(STRUCT_FAKE_API_ANSI_WITH_USERPARAM);
        else
            StructSize=sizeof(STRUCT_FAKE_API_UNICODE_WITH_USERPARAM);

        bHasUserParam=TRUE;
        break;
    }

    Index=-1;
    bNoMoreItems=FALSE;
    NextArrayItemPointer=(PBYTE)FakeArray;

    // parse all array: remember last  element of array as it FakeAPI field put to null
    while(!bNoMoreItems)
    {
        Index++;

        // get pointer to next PSTRUCT_FAKE_API or PSTRUCT_FAKE_API_WITH_USERPARAM struct
        pFakeAPIAnsi=(PSTRUCT_FAKE_API_ANSI_WITH_USERPARAM)NextArrayItemPointer;
        pFakeAPIUnicode=(PSTRUCT_FAKE_API_UNICODE_WITH_USERPARAM)NextArrayItemPointer;

        // according to loaded dll encoding convention
        if (iFakeAPIEncoding==FakeAPIEncodingANSI)
        {
#if (defined(UNICODE)||defined(_UNICODE))
            // convert ansi struct into unicode struct
            if (!ConvertFakeAPIAnsiToUnicode(pFakeAPIAnsi,(PSTRUCT_FAKE_API_UNICODE_WITH_USERPARAM)&FakeApiInfos,bHasUserParam))
                continue;
#else
            // if we are in ansi mode, just copy struct
            memcpy(&FakeApiInfos,pFakeAPIAnsi,StructSize);
#endif
        }
        else// loadded dll is in unicode encoding
        {
#if (defined(UNICODE)||defined(_UNICODE))
            // if we are in unicode mode just copy data
            memcpy(&FakeApiInfos,pFakeAPIUnicode,StructSize);
#else
            // convert unicode struct into ansi struct
            if (!ConvertFakeAPIUnicodeToAnsi(pFakeAPIUnicode,(PSTRUCT_FAKE_API_ANSI_WITH_USERPARAM)&FakeApiInfos,bHasUserParam))
                continue;
#endif
        }

        // if no more items
        if (FakeApiInfos.FakeAPI==NULL)
            break;

        // check for particular cases (COM or NET monitoring files)
        BOOL bCOMDefinition=IsCOMHookDefinition(FakeApiInfos.pszModuleName);
        BOOL bNETDefinition=IsNetHookDefinition(FakeApiInfos.pszModuleName);

        if (bCOMDefinition || bNETDefinition)
        {
            FAKING_DLL_INFOS* pAlreadyHookedFakingDllInfos=NULL;
            BOOL bParsingError=FALSE;

            // if struct definition is for COM faking
            if (bCOMDefinition)
            {
                if (pComManager)
                {
                    // use the COM Manager class to translate the work to HookCOM.dll
                    pComManager->AddHookComFakingDefinition(pFakingDllInfos,
                                                            &FakeApiInfos,
                                                            szFakeAPIDLLFullPath,
                                                            Index,
                                                            FakingDllArray,
                                                            &pAlreadyHookedFakingDllInfos,
                                                            &bParsingError);
                }
            }

            // in case of .NET hook definition
            else if (bNETDefinition)
            {
                if (pNetManager)
                {
                    if (!pNetManager->AddHookNetFakingDefinition(pFakingDllInfos,
                                                            &FakeApiInfos,
                                                            szFakeAPIDLLFullPath,
                                                            Index,
                                                            FakingDllArray,
                                                            &pAlreadyHookedFakingDllInfos,
                                                            &bParsingError)
                        )
                        return FALSE;// avoid lot of error message if .NET is not enabled
                }
            }

            // in case of parsing error
            if (bParsingError)
            {
                // report the error message
                size_t msglen;
                *pszMsg=0;
                _sntprintf(pszMsg,
                            3*MAX_PATH,
                            _T("Parsing error in %s\r\nat function index %u"),
                            szFakeAPIDLLFullPath,
                            Index
                            );
                msglen=_tcslen(pszMsg);
                if (msglen+30<3*MAX_PATH) // 30 : max size of added message, change it if you change messages
                {
                    switch(FakingDllArray)
                    {
                    case FAKING_DLL_ARRAY_FAKING:
                        _tcscat(pszMsg,_T(" of faking array"));
                        break;
                    case FAKING_DLL_ARRAY_PRE_HOOK:
                        _tcscat(pszMsg,_T(" of pre hook array"));
                        break;
                    case FAKING_DLL_ARRAY_POST_HOOK:
                        _tcscat(pszMsg,_T(" of post hook array"));
                        break;
                    }
                }
                CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
            }
            // if a faking hook was already installed for this COM/NET definition
            if (pAlreadyHookedFakingDllInfos)
            {
                // For COM faking we can't change hook as old dll will still be registered in the COM module
                // and if we update, we will have two faking dll registered to patch the same address
                TCHAR szNewFakingDllName[MAX_PATH];
                TCHAR BlockMsg[5*MAX_PATH];
                GetFakeApiDllName(pFakingDllInfos,szNewFakingDllName);

                _stprintf(BlockMsg,
                    _T("File %s\r\n provides new faking function for %s in %s, but a faked function is already provided"),
                    szNewFakingDllName,
                    FakeApiInfos.pszAPIName,
                    FakeApiInfos.pszModuleName
                    );

                // get file name
                TCHAR szOldFileName[MAX_PATH];
                if (GetFakeApiDllName(pAlreadyHookedFakingDllInfos,szOldFileName))
                {
                    _tcscat(BlockMsg,_T(" in "));
                    _tcscat(BlockMsg,szOldFileName);
                }

                _tcscat(pszMsg,_T("\r\nFaking dll can't be changed for COM/NET hooking,\r\nso you must unload other faking dll before loading this new one"));
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_ICONERROR|MB_YESNO|MB_TOPMOST);
            }
        }

        else // not COM or NET definition
        {
            // do the job for specified struct
            LoadFakeAPIDefinition(pFakingDllInfos,&FakeApiInfos,FakingDllArray,ApiOverrideBuildVersionFramework);
        }

        // point to next item of faking/pre/post array
        NextArrayItemPointer+=StructSize;

    }// end while
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ConvertFakeAPIAnsiToUnicode
// Object: Convert a faking/pre/post hook definition struct from ansi to unicode
// Parameters :
//     in  : STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi : ansi struct _WITH_USERPARAM or not (see bHasUserParam)
//           BOOL bHasUserParam : allow to make function compatible for struct having UserParam or not
//     out : STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode : unicode struct _WITH_USERPARAM or not (see bHasUserParam)
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL ConvertFakeAPIAnsiToUnicode(STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi,OUT STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode,BOOL bHasUserParam)
{
    pFakeApiUnicode->FakeAPI=pFakeApiAnsi->FakeAPI;
    pFakeApiUnicode->StackSize=pFakeApiAnsi->StackSize;
    pFakeApiUnicode->FirstBytesCanExecuteAnywhereSize=pFakeApiAnsi->FirstBytesCanExecuteAnywhereSize;

    if (bHasUserParam)
        pFakeApiUnicode->UserParam=pFakeApiAnsi->UserParam;

    // we have to translate pszModuleName and pszAPIName from ansi to unicode
    if (!CAnsiUnicodeConvert::AnsiToUnicode(pFakeApiAnsi->pszModuleName,pFakeApiUnicode->pszModuleName,MAX_PATH))
        return FALSE;

    if (!CAnsiUnicodeConvert::AnsiToUnicode(pFakeApiAnsi->pszAPIName,pFakeApiUnicode->pszAPIName,MAX_PATH))
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ConvertFakeAPIUnicodeToAnsi
// Object: Convert a faking/pre/post hook definition struct from unicode to ansi
// Parameters :
//     in  : STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode : unicode struct _WITH_USERPARAM or not (see bHasUserParam)
//           BOOL bHasUserParam : allow to make function compatible for struct having UserParam or not
//     out : STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi : ansi struct _WITH_USERPARAM or not (see bHasUserParam)
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL ConvertFakeAPIUnicodeToAnsi(STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode,OUT STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi,BOOL bHasUserParam)
{
    pFakeApiAnsi->FakeAPI=pFakeApiUnicode->FakeAPI;
    pFakeApiAnsi->StackSize=pFakeApiUnicode->StackSize;
    pFakeApiAnsi->FirstBytesCanExecuteAnywhereSize=pFakeApiUnicode->FirstBytesCanExecuteAnywhereSize;

    if (bHasUserParam)
        pFakeApiAnsi->UserParam=pFakeApiUnicode->UserParam;

    // translate from unicode to ansi pszModuleName and pszAPIName
    if (!CAnsiUnicodeConvert::UnicodeToAnsi(pFakeApiUnicode->pszModuleName,pFakeApiAnsi->pszModuleName,MAX_PATH))
        return FALSE;

    if (!CAnsiUnicodeConvert::UnicodeToAnsi(pFakeApiUnicode->pszAPIName,pFakeApiAnsi->pszAPIName,MAX_PATH))
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LoadFakeAPIDefinition
// Object: install hook for a fake api definition for array (FakingDllArray) 
//          according to api info provide inside (pFakeApiInfos) for faking dll (pFakingDllInfos)
// Parameters :
//     in  : FAKING_DLL_INFOS* pFakingDllInfos : informations on faking dll being parsed
//           STRUCT_FAKE_API_WITH_USERPARAM*  pFakeApiInfos : hook description
//           tagFakingDllArray FakingDllArray : value specifying array being loaded : faking, pre or post array
//                                              according to this value pFakeApiInfos->UserParam is meaning full or not
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL LoadFakeAPIDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                           STRUCT_FAKE_API_WITH_USERPARAM*  pFakeApiInfos,
                           tagFakingDllArray FakingDllArray,
                           DWORD ApiOverrideBuildVersionFramework)
{
    PBYTE pbAPI=0;
    BOOL bExeDllInternalHook=FALSE;
    TCHAR pszMsg[3*MAX_PATH];
    API_INFO *pAPIInfo;
    CLinkListItem *pItemAPIInfo;
    BOOL bAlreadyHooked=FALSE;
    BOOL bFunctionPointer;
    BOOL bUpdateFakeAPI;

    // get hook address
    pbAPI=GetWinAPIOverrideFunctionDescriptionAddress(pFakeApiInfos->pszModuleName,pFakeApiInfos->pszAPIName,&bExeDllInternalHook,&bFunctionPointer);

    // check api code pointer value
    if (IsBadCodePtr((FARPROC)pbAPI))
    {
        _sntprintf(pszMsg,
                   3*MAX_PATH,
                   _T("Bad code pointer 0x%p for function %s in dll %s Function not hooked"),
                   pbAPI,
                   pFakeApiInfos->pszAPIName,
                   pFakeApiInfos->pszModuleName);

        // avoid multiple message boxes
        // DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);

        return FALSE;
    }

    // get Item in the pLinkListAPIInfos list
    pItemAPIInfo=GetAssociatedItemAPIInfo(pbAPI,&bAlreadyHooked);
    if (!pItemAPIInfo)
        return FALSE;

    // now we do the job for the faking case
    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

    if (!bAlreadyHooked)
    {
        if (!InitializeApiInfo(pAPIInfo,pFakeApiInfos->pszModuleName,pFakeApiInfos->pszAPIName))
            return TRUE;// dll has begin to be loaded --> must be unloaded

        // fill FakeAPIAddress only if faking
        if (FakingDllArray==FAKING_DLL_ARRAY_FAKING)
        {
            // FILL FAKEAPIADDRESS AND PFAKEDLLINFOS ONLY FOR FAKING
            pAPIInfo->FakeAPIAddress=pFakeApiInfos->FakeAPI;
            pAPIInfo->pFakeDllInfos=pFakingDllInfos;
        }

        // specify log way
        pAPIInfo->DontCheckModulesFilters=bExeDllInternalHook;

        // get required stack size
        pAPIInfo->StackSize=pFakeApiInfos->StackSize;

        if (ApiOverrideBuildVersionFramework>=6)
        {
            // for backward compatibility, 
            // pFakeApiInfos->FirstBytesCanExecuteAnywhereSize becomes an option flags and FirstBytesCanExecuteAnywhereSize is reduce to an unsigned 8 bits value (sufficient for our goal currently max size is 64 bits)

            // get FirstBytesCanExecuteAnywhereSize 
            pAPIInfo->FirstBytesCanExecuteAnywhereSize = (pFakeApiInfos->AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE_MASK);
            if ((pFakeApiInfos->AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)
            {
                pAPIInfo->FirstBytesCanExecuteAnywhereSize = (DWORD)(-1);
            }
            
            pAPIInfo->DontCheckModulesFiltersForFaking = ((pFakeApiInfos->AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS);
        }
        else
        {
            // get FirstBytesCanExecuteAnywhereSize 
            pAPIInfo->FirstBytesCanExecuteAnywhereSize=pFakeApiInfos->AdditionalOptions;        
        }
        
        // specify no parameter direction spying
        pAPIInfo->ParamDirectionType=PARAM_DIR_NONE;


        // store bFunctionPointer flag
        pAPIInfo->bFunctionPointer=bFunctionPointer;

    }

    // check specific cases of pre and post hook informations
    switch(FakingDllArray)
    {
    case FAKING_DLL_ARRAY_PRE_HOOK:
        return AddPreApiCallCallBack(pItemAPIInfo,!bAlreadyHooked,pFakingDllInfos->hModule,(pfPreApiCallCallBack)pFakeApiInfos->FakeAPI,pFakeApiInfos->UserParam);
    case FAKING_DLL_ARRAY_POST_HOOK:
        return AddPostApiCallCallBack(pItemAPIInfo,!bAlreadyHooked,pFakingDllInfos->hModule,(pfPostApiCallCallBack)pFakeApiInfos->FakeAPI,pFakeApiInfos->UserParam);
    }

    // if api was already hooked
    if (bAlreadyHooked)
    {
        bUpdateFakeAPI=FALSE;

        // if no fake api was installed (only monitoring hook or pre/post api callback defined)
        if (pAPIInfo->FakeAPIAddress==NULL)
            bUpdateFakeAPI=TRUE;
        else
        {
            // a fake api is already installed for this func

            if (gFakingDllUserDefaultAction.TakeSameAction)
            {
                bUpdateFakeAPI=gFakingDllUserDefaultAction.UpdateFakeApi;
            }
            else
            {
                TCHAR szNewFakingDllName[MAX_PATH];
                TCHAR BlockMsg[5*MAX_PATH];
                GetFakeApiDllName(pFakingDllInfos,szNewFakingDllName);

                _stprintf(BlockMsg,
                        _T("File %s\r\n provides new overriding function for %s in %s, but an overriding function is already provided"),
                        szNewFakingDllName,
                        pFakeApiInfos->pszAPIName,
                        pFakeApiInfos->pszModuleName
                        );

                // get file name
                TCHAR szOldFileName[MAX_PATH];
                if (GetFakeApiDllName(pAPIInfo->pFakeDllInfos,szOldFileName))
                {
                    _tcscat(BlockMsg,_T(" in "));
                    _tcscat(BlockMsg,szOldFileName);
                }

                CQueryFakingOwnerShip QueryFakingOwnerShip(
                    BlockMsg,
                    &gFakingDllUserDefaultAction.TakeSameAction,
                    &gFakingDllUserDefaultAction.UpdateFakeApi,
                    &gFakingDllUserDefaultAction.QuestionAsked);

                QueryFakingOwnerShip.ShowDialog();

                // fill local values
                bUpdateFakeAPI=gFakingDllUserDefaultAction.UpdateFakeApi;
            }
        }

        if (bUpdateFakeAPI)
        {
            // update pAPIInfo fields
            pAPIInfo->FakeAPIAddress=pFakeApiInfos->FakeAPI;
            pAPIInfo->pFakeDllInfos=pFakingDllInfos;

            // specify log way
            pAPIInfo->DontCheckModulesFilters=bExeDllInternalHook;

            // get required stack size
            pAPIInfo->StackSize=pFakeApiInfos->StackSize;

        }
    }
    else // api was not hooked
    {

        //////////////////////////////////////
        // install hook now (only now when pAPIInfo is fully completed else you'll get unexpected results :D)
        //////////////////////////////////////
        if (!HookAPIFunction(pAPIInfo))//==FALSE)
        {
            // hook is not installed don't remove it
            FreeApiInfoItem(pItemAPIInfo);
        }
    }// end if(!bAlreadyHooked)

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UnloadFakeApiDll
// Object: try to unload all associated hook for this dll
// Parameters :
//     in  : PSTR pszFileName : dll file name (full path)
// Return : TRUE if library is free (even if error), FALSE if dll is still loaded
//-----------------------------------------------------------------------------
BOOL UnloadFakeApiDll(TCHAR* szFakeAPIDLLFullPath)
{
    API_INFO *pAPIInfo;
    CLinkListItem* pItemAPIInfo;
    CLinkListItem* pNextItemAPIInfo;
    CLinkListItem* pItemPrePostApiCallChain;
    CLinkListItem* pNextItemPrePostApiCallChain;
    PRE_POST_API_CALL_CHAIN_DATA* pPrePostApiCallChainData;
    TCHAR pszMsg[2*MAX_PATH];
    BOOL bUnHookSuccessFull=TRUE;
    BOOL bRet=FALSE;
    FAKING_DLL_INFOS* pFakingDllInfos;
    BOOL bUnhookIfPossible;

    if (*szFakeAPIDLLFullPath==0)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Empty file name"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // get file ID
    if (GetFakeApiDllInfos(szFakeAPIDLLFullPath,&pFakingDllInfos)==FALSE)
    {
        HMODULE hModule;
        // check if lib is loaded
        hModule=GetModuleHandle(szFakeAPIDLLFullPath);
        if (hModule)
        {
            // remove filters associated to module
            RemoveAPIOverrideInternalModule(hModule);
            // unload library
            FreeLibrary(hModule);
        }
        else
        {
            _sntprintf(pszMsg,2*MAX_PATH,_T("Dll %s was not loaded"),szFakeAPIDLLFullPath);
            DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        }
        return TRUE;
    }

    // unregister dll for com watching
    if (pComManager)
        pComManager->RemoveCOMObjectCreationCallBack(pFakingDllInfos->pCOMObjectCreationCallBack);


    // look in pLinkListAPIInfos for hooks being associated with this dll
    
    pLinkListAPIInfos->Lock();
begin:
    for (pItemAPIInfo = pLinkListAPIInfos->Head;pItemAPIInfo ;pItemAPIInfo = pNextItemAPIInfo)
    {
        bUnhookIfPossible=FALSE;

        // get pointer to hook info
        pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
        pNextItemAPIInfo=pItemAPIInfo->NextItem;//UnhookAPIFunction release pItemAPIInfo data

        // check faking part
        if (pAPIInfo->pFakeDllInfos == pFakingDllInfos)
        {
            // unregister dll for Net watching
            if (pAPIInfo->HookType==HOOK_TYPE_NET)
            {
                if (pNetManager)
                    pNetManager->RemoveHookNetFakingDefinition(pFakingDllInfos);
            }

            // Fake address won't be called anymore 
            pAPIInfo->FakeAPIAddress=NULL;
            // dissociate hook with faking dll
            pAPIInfo->pFakeDllInfos=NULL;

            bUnhookIfPossible=TRUE;
        }

        // check pre hook part
        if (pAPIInfo->PreApiCallChain)
        {
            // parse pre hook chain
            pAPIInfo->PreApiCallChain->Lock();
            for (pItemPrePostApiCallChain=pAPIInfo->PreApiCallChain->Head;pItemPrePostApiCallChain;pItemPrePostApiCallChain=pNextItemPrePostApiCallChain)
            {
                pNextItemPrePostApiCallChain=pItemPrePostApiCallChain->NextItem;
                pPrePostApiCallChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItemPrePostApiCallChain->ItemData;
                if (pPrePostApiCallChainData->OwnerModule==pFakingDllInfos->hModule)
                {
                    // remove item from pre hook list
                    pAPIInfo->PreApiCallChain->RemoveItem(pItemPrePostApiCallChain,TRUE);

                    bUnhookIfPossible=TRUE;

                }
            }
            pAPIInfo->PreApiCallChain->Unlock();
        }

        // check post hook part
        if (pAPIInfo->PostApiCallChain)
        {
            // parse pre hook chain
            pAPIInfo->PostApiCallChain->Lock();
            for (pItemPrePostApiCallChain=pAPIInfo->PostApiCallChain->Head;pItemPrePostApiCallChain;pItemPrePostApiCallChain=pNextItemPrePostApiCallChain)
            {
                pNextItemPrePostApiCallChain=pItemPrePostApiCallChain->NextItem;
                pPrePostApiCallChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItemPrePostApiCallChain->ItemData;
                if (pPrePostApiCallChainData->OwnerModule==pFakingDllInfos->hModule)
                {
                    // remove item from pre hook list
                    pAPIInfo->PostApiCallChain->RemoveItem(pItemPrePostApiCallChain,TRUE);

                    bUnhookIfPossible=TRUE;
                }
            }
            pAPIInfo->PostApiCallChain->Unlock();
        }


        if (bUnhookIfPossible)
        {
            // UnHookIfPossible needs lock to be released
            pLinkListAPIInfos->Unlock();

            // unhook item if possible (no other hooking way attached to pAPIInfo)
            bRet=UnHookIfPossible(pItemAPIInfo,TRUE);
            bUnHookSuccessFull=bUnHookSuccessFull && bRet;

            // lock again
            pLinkListAPIInfos->Lock();

            // assume pNextItemAPIInfo has not been destroyed during lock release
            if (!pLinkListpFakeDllInfos->IsItemStillInList(pNextItemAPIInfo,TRUE))
                goto begin;
        }
    }
    pLinkListAPIInfos->Unlock();

    // Free library
    if (bUnHookSuccessFull) // else we are sure to crash host application as 
    {                       // blocking function is returning into the code of an unloaded dll
        WaitForAllHookFreeing();

        // unload library
        FreeLibrary(pFakingDllInfos->hModule);
        // remove associated filters
        RemoveAPIOverrideInternalModule(pFakingDllInfos->hModule);

        // remove faking information from pLinkListpFakeDllInfos
        pLinkListpFakeDllInfos->RemoveItemFromItemData(pFakingDllInfos);

        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: UnloadAllFakeApiDll
// Object: unload all faking api
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnloadAllFakeApiDlls()
{
    // remove faking hooks
    if (!pLinkListpFakeDllInfos)
        return TRUE;

    CLinkListItem* pItemAPIInfo;
    CLinkListItem* pNextItemAPIInfo;
    BOOL bSuccess=TRUE;
    BOOL bRet;
    FAKING_DLL_INFOS* pFakingDllInfos;
    TCHAR pszFakingDllFileName[MAX_PATH];
    
    // free pLinkListpFakeDllInfos
    pLinkListpFakeDllInfos->Lock();
begin:    
    for (pItemAPIInfo =pLinkListpFakeDllInfos->Head;pItemAPIInfo;pItemAPIInfo = pNextItemAPIInfo)
    {
        pNextItemAPIInfo = pItemAPIInfo->NextItem;// because pItemAPIInfo can be free by UnloadFakeApiDll

        pFakingDllInfos=(FAKING_DLL_INFOS*)pItemAPIInfo->ItemData;
       
        if (GetFakeApiDllName(pFakingDllInfos,pszFakingDllFileName))
        {
            pLinkListpFakeDllInfos->Unlock();

            // list must be unlocked because lock is needed by UnloadFakeApiDll
            bRet=UnloadFakeApiDll(pszFakingDllFileName);
            bSuccess=bSuccess&&bRet;

            pLinkListpFakeDllInfos->Lock();
            if (!pLinkListpFakeDllInfos->IsItemStillInList(pNextItemAPIInfo,TRUE))
                goto begin;
        }
    }
    pLinkListpFakeDllInfos->Unlock();
    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: GetFakeApiDllName
// Object: get Fake API dll name from api dll Id
// Parameters :
//     in : FAKING_DLL_INFOS* pFakingDllInfos : fake api dll infos
//     out  : TCHAR* pszFileName : dll file name (full path) (must be already allocated, not allocated inside func)
//                                 Buffer size should be >= MAX_PATH
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL GetFakeApiDllName(FAKING_DLL_INFOS* pFakingDllInfos,TCHAR* pszFileName)
{
    *pszFileName=0;
    if (!pFakingDllInfos)
        return FALSE;
    return GetModuleFileName(pFakingDllInfos->hModule,pszFileName,MAX_PATH);
}

//-----------------------------------------------------------------------------
// Name: GetFakeApiDllInfos
// Object: get Fake API dll Id from api dll name
// Parameters :
//     in :  TCHAR* pszFileName : dll file name 
//     out  : FAKING_DLL_INFOS** ppFakingDllInfos : pointer to FAKING_DLL_INFOS passed by address
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL GetFakeApiDllInfos(TCHAR* pszFileName,FAKING_DLL_INFOS** ppFakingDllInfos)
{
    HMODULE hModule=GetModuleHandle(pszFileName);
    if (hModule==NULL)
    {
        *ppFakingDllInfos=NULL;
        return FALSE;
    }

    CLinkListItem* pItemFakingDllInfos;

    pLinkListpFakeDllInfos->Lock();
    // search throw pLinkListpFakeDllInfos for an item having same module handle
    for (pItemFakingDllInfos =pLinkListpFakeDllInfos->Head;pItemFakingDllInfos;pItemFakingDllInfos = pItemFakingDllInfos->NextItem)
    {
        *ppFakingDllInfos=(FAKING_DLL_INFOS*)pItemFakingDllInfos->ItemData;

        // if module handle is the same
        if((*ppFakingDllInfos)->hModule==hModule)
        {
            pLinkListpFakeDllInfos->Unlock();
            return TRUE;
        }

        
    }
    pLinkListpFakeDllInfos->Unlock();

    *ppFakingDllInfos=NULL;
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: AddPreApiCallCallBack
// Object: add a callback called before api call
// Parameters :
//     in  : 
// Return : TRUE in case of success, FALSE else
//-----------------------------------------------------------------------------
BOOL __stdcall AddPreApiCallCallBack(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPreApiCallCallBack FunctionPointer,PVOID UserParam)
{
    // get associated API_INFO data
    API_INFO* pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

    PRE_POST_API_CALL_CHAIN_DATA Data;

    // check function pointer to avoid to do it on each hook
    if (IsBadCodePtr((FARPROC)FunctionPointer))
        return FALSE;

    // if pre api call chain was not created
    if (!pAPIInfo->PreApiCallChain)
    {
        // create it
        pAPIInfo->PreApiCallChain=new CLinkList(sizeof(PRE_POST_API_CALL_CHAIN_DATA));
        // in case of memory allocation error
        if (!pAPIInfo->PreApiCallChain)
            return FALSE;
    }

    // store pre hook call back informations
    Data.CallBack=(PBYTE)FunctionPointer;
    Data.UserParam=UserParam;
    Data.OwnerModule=OwnerModule;

    // add item to PreApiCallChain list
    if (!pAPIInfo->PreApiCallChain->AddItem(&Data))
    {
        // if empty list, free it
        if(pAPIInfo->PreApiCallChain->Head==NULL)
        {
            CLinkList* pTmp;
            // delete Call Chain
            // assign to null before destroying content (better for multi threading)
            pTmp=pAPIInfo->PreApiCallChain;
            pAPIInfo->PreApiCallChain=NULL;
            delete pTmp;
        }
        return FALSE;
    }

    // hook api if necessary
    if (bNeedToBeHooked)
    {
        if (!HookAPIFunction(pAPIInfo))
        {
            // FreeApiInfoItem is called by RemovePreApiCallCallBack
            RemovePreApiCallCallBack(pItemAPIInfo,FunctionPointer,FALSE);
            return FALSE;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemovePreApiCallCallBack
// Object: remove a previously added callback before api call
// Parameters :
//     in  : 
// Return : TRUE in case of success, FALSE else
//-----------------------------------------------------------------------------
BOOL __stdcall RemovePreApiCallCallBack(CLinkListItem* pItemAPIInfo,pfPreApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes)
{

    if(!pLinkListAPIInfos->IsItemStillInList(pItemAPIInfo))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return FALSE;
    }
    if (IsBadReadPtr(pItemAPIInfo,sizeof(CLinkListItem)))
        return FALSE;

    API_INFO* pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    if (IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        return FALSE;

    if (!pAPIInfo->PreApiCallChain)
        return FALSE;

    // search call back inside linked list
    CLinkListItem* pItem;
    PRE_POST_API_CALL_CHAIN_DATA* pChainData;
    pAPIInfo->PreApiCallChain->Lock(TRUE);
    
    for (pItem=pAPIInfo->PreApiCallChain->Head;pItem;pItem=pItem->NextItem)
    {
        pChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItem->ItemData;
        // item associated to callback is found
        if (pChainData->CallBack==(PBYTE)FunctionPointer)
        {
            // remove it
            pAPIInfo->PreApiCallChain->RemoveItem(pItem,TRUE);
            break;
        }
    }
    pAPIInfo->PreApiCallChain->Unlock();
    // if chain has no more item
    if (pAPIInfo->PreApiCallChain->Head==NULL)
    {
        CLinkList* pTmp;
        // delete Call Chain
        // assign to null before destroying content (better for multi threading)
        pTmp=pAPIInfo->PreApiCallChain;
        pAPIInfo->PreApiCallChain=NULL;
        delete pTmp;

        // unhook item api info if no more used
        return UnHookIfPossible(pItemAPIInfo,bRestoreOriginalBytes);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddPostApiCallCallBack
// Object: add a callback called after api call
// Parameters :
//     in  : 
// Return : TRUE in case of success, FALSE else
//-----------------------------------------------------------------------------
BOOL __stdcall AddPostApiCallCallBack(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPostApiCallCallBack FunctionPointer,PVOID UserParam)
{
    PRE_POST_API_CALL_CHAIN_DATA Data;

    // get associated API_INFO data
    API_INFO* pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

    // check function pointer to avoid to do it on each hook
    if (IsBadCodePtr((FARPROC)FunctionPointer))
        return FALSE;

    // if pre api call chain was not created
    if (!pAPIInfo->PostApiCallChain)
    {
        // create it
        pAPIInfo->PostApiCallChain=new CLinkList(sizeof(PRE_POST_API_CALL_CHAIN_DATA));
        // in case of memory allocation error
        if (!pAPIInfo->PostApiCallChain)
            return FALSE;

    }

    // store pre hook call back informations
    Data.CallBack=(PBYTE)FunctionPointer;
    Data.UserParam=UserParam;
    Data.OwnerModule=OwnerModule;

    // add item to PostApiCallChain list
    if (!pAPIInfo->PostApiCallChain->AddItem(&Data))
    {
        // if empty list, free it
        if(pAPIInfo->PostApiCallChain->Head==NULL)
        {
            CLinkList* pTmp;
            // delete Call Chain
            // assign to null before destroying content (better for multi threading)
            pTmp=pAPIInfo->PostApiCallChain;
            pAPIInfo->PostApiCallChain=NULL;
            delete pTmp;
        }
        return FALSE;
    }

    // hook api if necessary
    if (bNeedToBeHooked)
    {
        if (!HookAPIFunction(pAPIInfo))
        {
            // FreeApiInfoItem is called by RemovePostApiCallCallBack
            RemovePostApiCallCallBack(pItemAPIInfo,FunctionPointer,FALSE);
            return FALSE;
        }
    }

    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: RemovePostApiCallCallBack
// Object: remove a previously added callback after api call
// Parameters :
//     in  : 
// Return : TRUE in case of success, FALSE else
//-----------------------------------------------------------------------------
BOOL __stdcall RemovePostApiCallCallBack(CLinkListItem* pItemAPIInfo,pfPostApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes)
{
    if(!pLinkListAPIInfos->IsItemStillInList(pItemAPIInfo))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return FALSE;
    }
    if (IsBadReadPtr(pItemAPIInfo,sizeof(CLinkListItem)))
        return FALSE;

    API_INFO* pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    if (IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        return FALSE;

    if (!pAPIInfo->PostApiCallChain)
        return FALSE;

    // search call back inside linked list
    CLinkListItem* pItem;
    PRE_POST_API_CALL_CHAIN_DATA* pChainData;
    pAPIInfo->PostApiCallChain->Lock(TRUE);
    
    for (pItem=pAPIInfo->PostApiCallChain->Head;pItem;pItem=pItem->NextItem)
    {
        pChainData=(PRE_POST_API_CALL_CHAIN_DATA*)pItem->ItemData;

        // item associated to callback is found
        if (pChainData->CallBack==(PBYTE)FunctionPointer)
        {
            // remove it
            pAPIInfo->PostApiCallChain->RemoveItem(pItem,TRUE);
            break;
        }
    }
    pAPIInfo->PostApiCallChain->Unlock();
    // if chain has no more item
    if (pAPIInfo->PostApiCallChain->Head==NULL)
    {
        CLinkList* pTmp;
        // delete Call Chain
        // assign to null before destroying content (better for multi threading)
        pTmp=pAPIInfo->PostApiCallChain;
        pAPIInfo->PostApiCallChain=NULL;
        delete pTmp;

        // unhook item api info if no more used
        return UnHookIfPossible(pItemAPIInfo,bRestoreOriginalBytes);
    }
    return TRUE;
}