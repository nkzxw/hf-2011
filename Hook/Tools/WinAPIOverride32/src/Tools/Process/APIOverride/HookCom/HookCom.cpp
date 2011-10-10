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
// Object: manage com hook
//
// WARNING HookCom.dll Heap can differ from ApiOverride.dll 
//         --> standard memory allocation (malloc, new) done in HookCom.dll can't be deleted from ApiOverride.dll
//-----------------------------------------------------------------------------

#include "HookCom.h"

#define HOOK_COM_DLL_UNLOAD_WATCHING_POOLING_TIME 5000 // time in ms

HOOK_COM_OPTIONS HookComOptions={0};// Hooking COM options
HOOK_COM_INIT HookComInfos={0};// struct containing function pointer of ApiOverride dll

CLinkListSimple* pLinkListCOMObjectCreationCallBack=NULL;// link list of Com object creation callbacks
CLinkList* pLinkListMonitoringFileInfo=NULL; // link list of MONITORING_FILE_INFOS 
                                             // keep names of auto loaded monitoring files
                                             // shared throw all CHookedClass objects
CLinkListSimple* pLinkListObjectCreatedForStaticHooks=NULL;// link list of CHookedObjedct

CCOMCreationPostApiCallHooks* pCOMCreationPostApiCallHooks=NULL;// class that manages API of Com object creation
CCLSIDFilters* pCLSIDFilters=NULL;// class that manages CLSID filters
BOOL bOleInitialized=FALSE;
BOOL bHookComInitialized=FALSE;// TRUE when Initialize has successfully completed
BOOL bHookComLoaded=FALSE;// TRUE until dll is loaded has successfully completed
TCHAR pszMonitoringFilesPath[MAX_PATH];// path containing IID files definitions
TCHAR HookComDllPath[MAX_PATH];// path of hookcom dll
DWORD dwSystemPageSize=4096;
CLinkListSimple* pLinkListHookedClasses=NULL;// link list of CHookedClass*
CLinkListSimple* pLinkListOpenDialogs=NULL;// link list of handle of open dialog windows
HANDLE hDllUnloadWatchingThread=NULL;// handle of thread that monitors dll unloading to report not fully released com objects
HANDLE hevtStopDllUnloadWatching=NULL;// event to stop hDllUnloadWatchingThread
HANDLE hSemaphoreOpenDialogs=NULL;// semaphore that manages opened dialogs closing
BOOL bCOMAutoHookingEnabled=FALSE;// flag to know if com auto hooking is enabled or not
HINSTANCE DllhInstance=NULL;// current dll instance
BOOL WatchUnloadingDllThreadClosedCleanly=TRUE; // gives a way to know if thread has been closed cleanly (set to TRUE until thread is not started)
DWORD SetHookForObjectLockTlsIndex=0; // avoid functions called by SetHookForObject to be hooked (it can make crash)
                                      // You must do it because it's may not the same api which calls SetHookForObject
                                      // by the way if CoCreateInstance and CoGetMalloc are hooked
                                      // when an object is hooked, CoCreateInstance is called, which calls SetHookForObject,which calls SysAllocString which internally can call CoGetMalloc which is hooked;
                                      // so CoGetMalloc hooks calls SetHookForObject which is reentered for the same thread, thing that can crash process

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DllhInstance=hInstDLL;
        DisableThreadLibraryCalls(DllhInstance);
        SetHookForObjectLockTlsIndex = TlsAlloc(); // MSDN : The slots for the index are initialized to zero
        return Initialize();
    case DLL_PROCESS_DETACH:
        Destroy(TRUE);
        TlsFree(SetHookForObjectLockTlsIndex);
        return TRUE;
    }
    return TRUE;
}

void UserTypeParsingReportError(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    if (HookComInfos.ReportMessage)
        HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,ErrorMessage);
}

//-----------------------------------------------------------------------------
// Name: Initialize
// Object: like constructor : allocate memory and initialize
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL Initialize()
{
    if (bHookComLoaded)
        return TRUE;

    // set error report for CSupportedParameters
    CSupportedParameters::SetErrorReport(UserTypeParsingReportError,NULL);

    // get page size
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo); 
    dwSystemPageSize=siSysInfo.dwPageSize;

    // create object to manage COM object creation spying
    pCOMCreationPostApiCallHooks=new CCOMCreationPostApiCallHooks();
    if (!pCOMCreationPostApiCallHooks)
        return FALSE;

    // create list of hooked com items
    pLinkListHookedClasses=new CLinkListSimple();
    if (!pLinkListHookedClasses)
        return FALSE;

    // create CLSID filter object
    pCLSIDFilters=new CCLSIDFilters();
    if(!pCLSIDFilters)
        return FALSE;

    // create list for COM object creation callback
    pLinkListCOMObjectCreationCallBack=new CLinkListSimple();
    if (!pLinkListCOMObjectCreationCallBack)
        return FALSE;

    // create list for COM created object for static hooks
    pLinkListObjectCreatedForStaticHooks=new CLinkListSimple();
    if (!pLinkListObjectCreatedForStaticHooks)
        return FALSE;

    // create link list of MONITORING_FILE_INFOS
    pLinkListMonitoringFileInfo=new CLinkList(sizeof(FAKING_HOOK_INFORMATIONS));
    if (!pLinkListMonitoringFileInfo)
        return FALSE;

    hevtStopDllUnloadWatching=CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!hevtStopDllUnloadWatching)
        return FALSE;

    // get dll path
    GetModuleFileName(DllhInstance,HookComDllPath,MAX_PATH);
    CStdFileOperations::GetFilePath(HookComDllPath,HookComDllPath,MAX_PATH);

    // get COM monitoring files path
    _tcscpy(pszMonitoringFilesPath,HookComDllPath);
    _tcsncat(pszMonitoringFilesPath,COM_MONITORING_FILES_RELATIVE_PATH,MAX_PATH-1-_tcslen(pszMonitoringFilesPath));

    bHookComLoaded=TRUE;

    pLinkListOpenDialogs=new CLinkListSimple();
    if (!pLinkListOpenDialogs)
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Destroy
// Object:  like destructor : free memory
// Parameters :
//     in  : BOOL CalledByFreeLibrary : TRUE if called by DllMain DLL_PROCESS_DETACH
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL Destroy(BOOL CalledByFreeLibrary)
{
    if (!bHookComLoaded)
        return TRUE;
    bHookComLoaded=FALSE;

    StopAutoHooking();
    StopDllUnloadWatching();
    if (CalledByFreeLibrary)
    {
        if ( hDllUnloadWatchingThread && (!WatchUnloadingDllThreadClosedCleanly) )
            WaitForSingleObject(hDllUnloadWatchingThread,1000);// gives a chance to DllUnloadWatchingThread to ends cleanly (if system not already stopped it)
    }
    else
    {
        if (hDllUnloadWatchingThread)
            WaitForSingleObject(hDllUnloadWatchingThread,10000);
    }

    if (pCOMCreationPostApiCallHooks)
    {
        StopHookingCreatedCOMObjects();
        delete pCOMCreationPostApiCallHooks;
        pCOMCreationPostApiCallHooks=NULL;
    }

    UnHookAllComObjects(CalledByFreeLibrary);

    if (pLinkListOpenDialogs)
    {
        CLinkListItem* pItem;
        if (pLinkListOpenDialogs->Head) // if at least a dialog is displayed
        {
            pLinkListOpenDialogs->LockWaitTime=5000;
            pLinkListOpenDialogs->Lock();
            // create semaphore
            hSemaphoreOpenDialogs=CreateSemaphore(NULL,0,pLinkListOpenDialogs->GetItemsCount(),NULL);
            // for each existing dialog
            for (pItem=pLinkListOpenDialogs->Head;pItem;pItem=pItem->NextItem)
            {
                // close dialog
                EndDialog((HWND)pItem->ItemData,0);
            }
            // wait for all dialog to be close during 5 sec
            DWORD dwRes;
            for (DWORD Cnt=0;Cnt<pLinkListOpenDialogs->GetItemsCount();Cnt++)
            {
                dwRes=WaitForSingleObject(hSemaphoreOpenDialogs,5000);
                if (dwRes==WAIT_OBJECT_0)
                    continue;
                else
                    break;
            }
            // sleep a little to allow thread having called ReleaseSemaphore to end
            Sleep(100);

            // close semaphore
            CloseHandle(hSemaphoreOpenDialogs);
            hSemaphoreOpenDialogs=NULL;

            pLinkListOpenDialogs->RemoveAllItems(TRUE);
            pLinkListOpenDialogs->Unlock();
        }
        delete pLinkListOpenDialogs;
    }

    // free list of hooked com items
    if (pLinkListHookedClasses)
    {
        delete pLinkListHookedClasses;
        pLinkListHookedClasses=NULL;
    }

    if (pCLSIDFilters)
    {
        delete pCLSIDFilters;
        pCLSIDFilters=NULL;
    }

    if(pLinkListCOMObjectCreationCallBack)
    {
        delete pLinkListCOMObjectCreationCallBack;
        pLinkListCOMObjectCreationCallBack=NULL;
    }

    if (pLinkListObjectCreatedForStaticHooks)
    {
        delete pLinkListObjectCreatedForStaticHooks;
        pLinkListObjectCreatedForStaticHooks=NULL;
    }

    if (pLinkListMonitoringFileInfo)
    {
        delete pLinkListMonitoringFileInfo;
        pLinkListMonitoringFileInfo=NULL;
    }

    if (hevtStopDllUnloadWatching)
    {
        CloseHandle(hevtStopDllUnloadWatching);
        hevtStopDllUnloadWatching=NULL;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: PrepareDllUnload
// Object: should be call before calling module call FreeLibrary
//         it's terminate threads and free memory more cleanly
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall PrepareDllUnload()
{
    return Destroy(FALSE);
}

//-----------------------------------------------------------------------------
// Name: ReleaseCreatedCOMObjectsForStaticHooks
// Object:  Release com object created for static hooks
//  MUST BE CALLED IN THE SAME THREAD because
//  1) some objects are single threaded
//  2) if thread has ended Release operations will crash
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ReleaseCreatedCOMObjectsForStaticHooks()
{
    if (!bHookComLoaded)
        return FALSE;

    CLinkListItem* pItem;

    // for each locally created objects 
    // (must be done before calling pHookedClass->unhook 
    // 'to avoid to remove items from associated pHookedClass->pLinkListHookedObjects)
    pLinkListObjectCreatedForStaticHooks->Lock(FALSE);
    for (pItem=pLinkListObjectCreatedForStaticHooks->Head;pItem;pItem=pItem->NextItem)
    {
        // release object
        CSecureIUnknown::Release(((IUnknown*)pItem->ItemData));
    }
    pLinkListObjectCreatedForStaticHooks->RemoveAllItems(TRUE);
    pLinkListObjectCreatedForStaticHooks->Unlock();

    // uninitialize OLE
    if (bOleInitialized)
        OleUninitialize();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UnHookAllComObjects
// Object:  unhook all currently hooked COM objects
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall UnHookAllComObjects()
{
    return UnHookAllComObjects(FALSE);
}

//-----------------------------------------------------------------------------
// Name: UnHookAllComObjects
// Object:  unhook all currently hooked COM objects
// Parameters :
//     in  : BOOL CalledFromFreeLibrary : TRUE if called from FreeLibrary (and some thread wheren't closed gracefully)
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnHookAllComObjects(BOOL CalledFromFreeLibrary)
{
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    CHookedClass* pHookedClass;

    // pLinkListHookedClasses is already destroyed
    if (!pLinkListHookedClasses)
        return TRUE;

    if (CalledFromFreeLibrary) // other hook com dll threads may won't be closed nicely
        pLinkListHookedClasses->LockWaitTime=5000;// avoid deadlock
    // add lock to pLinkListHookedClasses
    pLinkListHookedClasses->Lock(FALSE);

    // for each hooked class
    for (pItem=pLinkListHookedClasses->Head;pItem;pItem=pNextItem)
    {
        // store next hooked class before removal
        pNextItem=pItem->NextItem;

        // get pointer on hooked class object
        pHookedClass=(CHookedClass*)pItem->ItemData;

        // unhook class
        pHookedClass->Unhook(TRUE);

        // free memory
        delete pHookedClass;

        // remove class from list
        pLinkListHookedClasses->RemoveItem(pItem,TRUE);
    }
    
    pLinkListHookedClasses->Unlock();


    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CallCOMObjectCreationCallBacks
// Object:  call callbacks defined for COM object creation
// Parameters :
//     in  : CLSID* pClsid : pointer to object CLSID
//           IID* pIid : pointer to interface iid
//           PVOID pInterface : pointer to interface
//           PRE_POST_API_CALL_HOOK_INFOS* pHookInfos : hook informations
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CallCOMObjectCreationCallBacks(CLSID* pClsid,IID* pIid,PVOID pInterface,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos)
{
    if (!bHookComLoaded)
        return FALSE;
    if (!pLinkListCOMObjectCreationCallBack)
        return FALSE;
    for (CLinkListItem* pItem=pLinkListCOMObjectCreationCallBack->Head;pItem;pItem=pItem->NextItem)
    {
        // check callback code validity
        if (IsBadCodePtr((FARPROC)pItem->ItemData))
            continue;
        // if callback return FALSE, stop the chain
        if (!(((pfCOMObjectCreationCallBack)(pItem->ItemData))(pClsid,pIid,pInterface,pHookInfos)))
            break;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveCOMObjectCreationCallBack
// Object:  remove a callback for COM object creation
// Parameters :
//     in  : pfCOMObjectCreationCallBack pCOMObjectCreationCallBack : callback function
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall RemoveCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack)
{
    if (!bHookComLoaded)
        return FALSE;
    if (pCOMObjectCreationCallBack==NULL)
        return TRUE;
    pLinkListCOMObjectCreationCallBack->RemoveItemFromItemData((PVOID)pCOMObjectCreationCallBack);

    // if com auto hooking is not enable, there's no need to monitor com object creation
    if (!bCOMAutoHookingEnabled)
        StopHookingCreatedCOMObjects();

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: AddCOMObjectCreationCallBack
// Object:  add a callback for COM object creation
// Parameters :
//     in  : pfCOMObjectCreationCallBack pCOMObjectCreationCallBack : callback function
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall AddCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack)
{
    if (!bHookComLoaded)
        return FALSE;

    // if object is already destroyed
    if (!pCOMCreationPostApiCallHooks)
        return FALSE;

    if (IsBadCodePtr((FARPROC)pCOMObjectCreationCallBack))
        return FALSE;

    if (!pCOMCreationPostApiCallHooks->IsStarted())
    {
        if (!StartHookingCreatedCOMObjects())
            return FALSE;
    }

    return (pLinkListCOMObjectCreationCallBack->AddItem((PVOID)pCOMObjectCreationCallBack)!=NULL);
}

//-----------------------------------------------------------------------------
// Name: StartAutoHooking
// Object:  Start autohooking
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StartAutoHooking()
{
    if (!bHookComLoaded)
        return FALSE;

    // if object is already destroyed
    if (!pCOMCreationPostApiCallHooks)
        return FALSE;

    if (!pCOMCreationPostApiCallHooks->IsStarted())
    {
        if (!StartHookingCreatedCOMObjects())
            return FALSE;
    }
    bCOMAutoHookingEnabled=TRUE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StopAutoHooking
// Object:  stop auto hooking
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StopAutoHooking()
{
    bCOMAutoHookingEnabled=FALSE;

    // if com object creation is needed by COMObjectCreationCallBack, don't stop to 
    // spy object creation
    if (pLinkListCOMObjectCreationCallBack)
    {
        if(pLinkListCOMObjectCreationCallBack)
            return TRUE;
    }

    // else there's no need to monitor com object creation
    StopHookingCreatedCOMObjects();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StartHookingCreatedCOMObjects
// Object:  start to hook functions that create com object 
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StartHookingCreatedCOMObjects()
{
    if (!bHookComLoaded)
        return FALSE;

    // if object is already destroyed
    if (!pCOMCreationPostApiCallHooks)
        return FALSE;

    // assume object creation spying is stopped
    if (!StopHookingCreatedCOMObjects())
        return FALSE;

    if (!pCOMCreationPostApiCallHooks->Start(HookComOptions.pszConfigFileComObjectCreationHookedFunctions))
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StopHookingCreatedCOMObjects
// Object:  stop to hook functions that create com object 
//          --> no more new object will be hooked
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StopHookingCreatedCOMObjects()
{
    if (!bHookComLoaded)
        return FALSE;

    // if object is already destroyed
    if (!pCOMCreationPostApiCallHooks)
        return TRUE;

    // stop com object creation spying
    if (!pCOMCreationPostApiCallHooks->Stop())
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitializeHookCom
// Object: Initialize HookCom dll
//         provide Hook com dll functions handler to ApiOverride.dll functions
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall InitializeHookCom(HOOK_COM_INIT* pInitHookCom)
{
    if (!bHookComLoaded)
        return FALSE;

    // Initialize local HookComInfos struct
    memcpy(&HookComInfos,pInitHookCom,sizeof(HOOK_COM_INIT));

    bHookComInitialized=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: MakeFullPathIfNeeded
// Object: add HookCom dll path to filename if file name was not given with full path
//         translate xxx.txt into HookComDllPath\xxx.txt
// Parameters :
//     inout  : TCHAR* pszName : in : file name of a file
//                               out : full path of file
//                              MUST BE AT LEAST OF MAX_PATH SIZE
//     return : 
//-----------------------------------------------------------------------------
void MakeFullPathIfNeeded(IN OUT TCHAR* pszName)
{
    // if full path is provide, 
    if (_tcschr(pszName,'\\'))
        // do nothing
        return;

    // else add hook com dll path to filename

    TCHAR pszTmp[MAX_PATH];
    _tcsncpy(pszTmp,HookComDllPath,MAX_PATH);
    _tcsncat(pszTmp,pszName,MAX_PATH-_tcslen(pszTmp)-1);
    _tcsncpy(pszName,pszTmp,MAX_PATH);
}

//-----------------------------------------------------------------------------
// Name: SetHookComOptions
// Object: set COM hooks options
// Parameters :
//     in  : HOOK_COM_OPTIONS* pHookComOptions : COM monitoring options
//     out : 
//     return : TRUE on success
//              FALSE if some options are wrong and so new options are not applied
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetHookComOptions(HOOK_COM_OPTIONS* pHookComOptions)
{
    if (!bHookComLoaded)
        return FALSE;

    BOOL bRet=TRUE;

    MakeFullPathIfNeeded(pHookComOptions->pszConfigFileComObjectCreationHookedFunctions);

    if (!CStdFileOperations::DoesFileExists(pHookComOptions->pszConfigFileComObjectCreationHookedFunctions))
    {


        TCHAR pszMsg[2*MAX_PATH];
        _stprintf(pszMsg,_T("File %s doesn't exist\r\nCOM hooking won't work until you specify a valid \r\n")
                         _T("COM object creation hooked functions file (see COM options)"),
                         pHookComOptions->pszConfigFileComObjectCreationHookedFunctions);
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    pCLSIDFilters->ClearFilters();
    if (pHookComOptions->bUseClsidFilter)
    {
        if (pHookComOptions->CLSIDFilterType==COM_CLSID_FilterDontHookSpecified)
        {
            MakeFullPathIfNeeded(pHookComOptions->pszNotHookedFileName);
            bRet=pCLSIDFilters->ParseFiltersFile(pHookComOptions->pszNotHookedFileName,TRUE);
        }
        else
        {
            MakeFullPathIfNeeded(pHookComOptions->pszOnlyHookedFileName);
            bRet=pCLSIDFilters->ParseFiltersFile(pHookComOptions->pszOnlyHookedFileName,FALSE);
        }
    }

    // Initialize local HookComInfos struct
    memcpy(&HookComOptions,pHookComOptions,sizeof(HOOK_COM_OPTIONS));

    if (HookComOptions.ReportHookedCOMObject)
        StartDllUnloadWatching();
    else
        StopDllUnloadWatching();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: ReportParsingError
// Object: Report a parsing error in a file and query user if he want to continue parsing
// Parameters :
//     in  : TCHAR* pszFileName
//           DWORD dwLineNumber
//     out : 
//     return : TRUE if file parsing should continue
//-----------------------------------------------------------------------------
BOOL ReportParsingError(TCHAR* pszFileName,DWORD dwLineNumber)
{
    TCHAR pszMsg[3*MAX_PATH];
    {
        // parsing error ask if we should continue
        _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d\r\nContinue parsing ?"),pszFileName,dwLineNumber);
        if (HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)
            ==IDYES)
            return TRUE;
        else
            return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Name: ReportNotSupportedInterface
// Object: Report error for a not supported interface query (QueryInterface failure)
// Parameters :
//     in  : CLSID* pClsid : class id for which iid was queried
//           IID* pIid : queried Interface id
//     out : 
//     return : TRUE if file parsing should continue
//-----------------------------------------------------------------------------
void ReportNotSupportedInterface(CLSID* pClsid,IID* pIid)
{
    // if CLSID not supporting IDispatch must be reported
    if (HookComOptions.ReportCLSIDNotSupportingIDispatch)
    {
        // report warning
        TCHAR pszMsg[STRING_PROGID_MAX_SIZE+STRING_IID_MAX_SIZE+100];
        BOOL bReportMessage=FALSE;
        TCHAR _tIid[STRING_IID_MAX_SIZE];
        if (!CGUIDStringConvert::TcharFromIID(pIid,_tIid))
            return;

        if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
        {
            CGUIDStringConvert::GetInterfaceName(_tIid,_tIid,STRING_IID_MAX_SIZE);

            TCHAR _tProgId[STRING_PROGID_MAX_SIZE];

            bReportMessage=CGUIDStringConvert::GetClassName(pClsid,_tProgId,STRING_PROGID_MAX_SIZE);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: type %s doesn't support %s"),_tProgId,_tIid);
        }
        // if progId retrieval failure or !ReportUseProgIdInsteadOfCLSIIfPossible
        if (!bReportMessage)
        {
            TCHAR _tClsid[STRING_GUID_SIZE];
            bReportMessage=CGUIDStringConvert::TcharFromCLSID(pClsid,_tClsid);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: CLSID %s doesn't support %s"),_tClsid,_tIid);
        }
        if(bReportMessage)
            HookComInfos.ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
    }
}

//-----------------------------------------------------------------------------
// Name: ReportBadFunctionAddress
// Object: Report error for Methods not implemented or not hookable (bad code pointer)
// Parameters :
//     in  : CMethodInfo* pMethodInfo : information on method
//           PBYTE pAddress : Address
//     out : 
//     return : TRUE if file parsing should continue
//-----------------------------------------------------------------------------
void ReportBadFunctionAddress(CMethodInfo* pMethodInfo,PBYTE pAddress)
{
    TCHAR pszMsg[2*MAX_PATH];
    TCHAR pszFuncDesc[2048];
    pMethodInfo->ToString(TRUE,pszFuncDesc);
    _sntprintf(pszMsg,MAX_PATH,_T("Bad Address 0x%p for method %s. Method can't be hooked"),pAddress,pszFuncDesc);
    HookComInfos.ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
}

//-----------------------------------------------------------------------------
// Name: GetOrCreateHookedClass
// Object: get or create (if not exist) CHookedClass object associated to CLSID
// Parameters :
//     in  : CLSID* pCLSID : clsid which needs to be hooked
//     out : 
//     return : TRUE if object created with such CLSID must be hooked
//-----------------------------------------------------------------------------
CHookedClass* GetOrCreateHookedClass(CLSID* pCLSID)
{
    CHookedClass* pHookedClass;
    CLinkListItem* pHookedClassItem;

    // lock list
    pLinkListHookedClasses->Lock();

    // for each pLinkListHookedClasses item
    for (pHookedClassItem=pLinkListHookedClasses->Head;pHookedClassItem;pHookedClassItem=pHookedClassItem->NextItem)
    {
        pHookedClass=(CHookedClass*)pHookedClassItem->ItemData;
        // if pHookedClass defines the specified CLSID
        if (IsEqualCLSID(pHookedClass->Clsid,*pCLSID))
        {
            // unlock list
            pLinkListHookedClasses->Unlock();

            // return HookedClass object corresponding to the CLSID
            return pHookedClass;
        }
    }
    // if not found
    pHookedClass=new CHookedClass(pCLSID);
    if (!pHookedClass)
    {
        // unlock list
        pLinkListHookedClasses->Unlock();
        return NULL;
    }
    if (!pLinkListHookedClasses->AddItem(pHookedClass,TRUE))
    {
        delete pHookedClass;
        // unlock list
        pLinkListHookedClasses->Unlock();
        return NULL;
    }

    // unlock list
    pLinkListHookedClasses->Unlock();

    return pHookedClass;
}

//-----------------------------------------------------------------------------
// Name: SetHookForObject
// Object: put hook on methods of the given object
//         Called by api handlers
// Parameters :
//     in  : IUnknown* pObject : pointer to object to hook
//           CLSID* pCLSID
//           IID* pIID
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL SetHookForObject(IUnknown* pObject,CLSID* pCLSID,IID* pIID)
{
    if(!bCOMAutoHookingEnabled)
        return TRUE;

    if (IsBadReadPtr(pObject,sizeof(IUnknown)))
        return FALSE;

    CHookedClass* pHookedClass;

    // check init
    if (!bHookComInitialized)
        return FALSE;

    // check current function lock for current thread
    if (TlsGetValue(SetHookForObjectLockTlsIndex))
        return FALSE;
    // set current function lock for current thread
    TlsSetValue(SetHookForObjectLockTlsIndex,(LPVOID)TRUE);

    // check CLSID filtering
    if (!pCLSIDFilters->CheckFilters(pCLSID))
        return FALSE;

    // get CHookedClass associated to CLSID
    pHookedClass=GetOrCreateHookedClass(pCLSID);
    if (!pHookedClass)
        return FALSE;

    BOOL bRet = pHookedClass->AddAutoHookedObject(pObject,pIID);

    // reset current function lock for current thread
    TlsSetValue(SetHookForObjectLockTlsIndex,(LPVOID)FALSE);
    return bRet;
}


//-----------------------------------------------------------------------------
// Name: StaticLoadingReportCOMCreationError
// Object: report com creation error with a messagebox due to manual file loading
// Parameters :
//     in  : 
//           CLSID* pCLSID : pointer to CLSID
//           IID* pIID : pointer to IID
//           TCHAR* pszFileName : monitoring filename of faking dll file name
//           DWORD dwLineNumberOrFunctionIndex : line number for monitoring file,
//                                               function index for monitoring file
//           BOOL bMonitoring : TRUE if error occurs during a monitoring file loading
//                              FALSE if error occurs during a faking dll loading
//           tagFakingDllArray FakingType : used only when faking dll loading
//                                          specify which array is loading (faking, pre or post api call)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void StaticLoadingReportCOMCreationError(CLSID* pClsid,
                                         IID* pIid,
                                         TCHAR* pszFileName,
                                         DWORD dwLineNumberOrFunctionIndex,
                                         BOOL bMonitoring,
                                         tagFakingDllArray FakingType)
{
    TCHAR pszCLSID[STRING_PROGID_MAX_SIZE];
    TCHAR pszIID[STRING_IID_MAX_SIZE];
    TCHAR pszMsg[MAX_PATH+STRING_PROGID_MAX_SIZE+STRING_IID_MAX_SIZE];


    // get IID 
    if (!CGUIDStringConvert::TcharFromIID(pIid,pszIID))
        return;
    // if name instead of ID
    if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
    {
        if (!CGUIDStringConvert::TcharFromCLSID(pClsid,pszCLSID))
        {
            if (!CGUIDStringConvert::TcharFromCLSID(pClsid,pszCLSID))
                return;
        }
        CGUIDStringConvert::GetInterfaceName(pszIID,pszIID,STRING_IID_MAX_SIZE);
    }
    // in case of ID
    else
    {
        if (!CGUIDStringConvert::TcharFromCLSID(pClsid,pszCLSID))
            return;
    }
    
    // display error message
    if (pszFileName)
    {
        TCHAR pszTmpMsg[100];
        _sntprintf(pszMsg,MAX_PATH+STRING_PROGID_MAX_SIZE+STRING_IID_MAX_SIZE,
            _T("Error creating following COM object \r\n")
            _T("CLSID:%s\r\n")
            _T("IID:%s\r\n")
            _T("for file %s "),
            pszCLSID,
            pszIID,
            pszFileName
            );
        if (bMonitoring)
            _stprintf(pszTmpMsg,_T("at line %u"),dwLineNumberOrFunctionIndex);
        else
        {
            _stprintf(pszTmpMsg,_T("function %u"),dwLineNumberOrFunctionIndex);

            switch(FakingType)
            {
            case FAKING_DLL_ARRAY_FAKING:
                _tcscat(pszTmpMsg,_T(" of faking array"));
                break;
            case FAKING_DLL_ARRAY_PRE_HOOK:
                _tcscat(pszTmpMsg,_T(" of pre hook array"));
                break;
            case FAKING_DLL_ARRAY_POST_HOOK:
                _tcscat(pszTmpMsg,_T(" of post hook array"));
                break;
            }
        }
        _tcscat(pszMsg,pszTmpMsg);
    }
    else
    {
        _sntprintf(pszMsg,MAX_PATH+STRING_PROGID_MAX_SIZE+STRING_IID_MAX_SIZE,
            _T("Error creating following COM object \r\n")
            _T("CLSID:%s\r\n")
            _T("IID:%s"),
            pszCLSID,
            pszIID
            );
    }

    HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: AddHookComMonitoringDefinition
// Object: add a hook from a monitoring file (design only for static hook)
//         hook removal is done like a standard monitoring file inside ApiOverride.dll
//         __stdcall UnHookAllComObjects must be called by the same thread
// Parameters :
//     in  : MONITORING_FILE_INFOS* pMonitoringFileInfo : Monitoring file pointer stored in API_INFO struct associated to hook
//           TCHAR* pszFullDescription : full hook description (full line of monitoring file)
//           TCHAR* pszFileName : monitoring file name
//           DWORD dwLineNumber : monitoring line number currently parsed
//
//     out : MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo : contains monitoring file information owning the hook
//                                                                        if hook is already install
//                                                                        NULL if function wasn't already hooked
//           BOOL* pbParsingError : TRUE if a parsing error occurs
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) 
BOOL __stdcall AddHookComMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                              TCHAR* pszFullDescription,
                                              TCHAR* pszFileName,
                                              DWORD dwLineNumber,
                                              MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                              BOOL* pbParsingError)
{

    if (!bHookComLoaded)
        return FALSE;

    // pszFullDescription is like COM@CLSID@IID|function name|options
    TCHAR* pszFunctionDescription;
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    IUnknown* pObject;
    CLSID Clsid;
    HRESULT hResult;

    *ppAlreadyHookingMonitoringFileInfo=NULL;
    *pbParsingError=FALSE;

    // initialize com and ole for current thread (we can't know if it has already been done)
    if (!bOleInitialized)
    {
        OleInitialize(NULL);
        bOleInitialized=TRUE;
    }

    // find function name
    pszFunctionDescription=_tcschr(pszFullDescription,FIELDS_SEPARATOR);
    if (pszFunctionDescription)
    {
        // ends pszFullDescription after module infos
        *pszFunctionDescription=0;
        // point to function description
        pszFunctionDescription++;
    }

    // decode COM definition
    if (!DecodeHookComDefinition(pszFullDescription,&Clsid,&HookDefinitionInfos))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }

    // get or create CHookedClass object associated to the class
    CHookedClass* pHookedClass=GetOrCreateHookedClass(&Clsid);
    if(!pHookedClass)
        return FALSE;

    CHookedObject* pHookedObject;
    // if an object of this type has already been created by our dll, use it instead of creating a new one
    pHookedObject=pHookedClass->GetInternallyCreatedObject();

    // if we already have creating a such object in our class
    if (pHookedObject)
    {
        // get wanted interface
        if (IsEqualIID(HookDefinitionInfos.InterfaceID,IID_IUnknown))
        {
            pObject=pHookedObject->pIUnknown;
        }
        else
        {
            pObject=NULL;
            if(FAILED(CSecureIUnknown::QueryInterface(pHookedObject->pObject,HookDefinitionInfos.InterfaceID,(void**)&pObject)))
            {
                StaticLoadingReportCOMCreationError(&Clsid,&HookDefinitionInfos.InterfaceID,pszFileName,dwLineNumber,TRUE,(tagFakingDllArray)0);
                ReportNotSupportedInterface(&Clsid,&HookDefinitionInfos.InterfaceID);
                return FALSE;
            }
            if (IsBadReadPtr(pObject,sizeof(IUnknown)))
                return FALSE;

            // we only want pointer, so release object to decrement internal object count
            CSecureIUnknown::Release(pObject);
        }
    }
    else
    {
        // creating an object make the corresponding dll to be loaded,
        // and until we don't release this object, dll is not unloaded until the end of the application
        // so we can overwrite functions first bytes and install our hooks
        // --> object will be released only when UnHookAllComObjects wil be called
        hResult=CoCreateInstance(Clsid,
            NULL,
            CLSCTX_ALL,
            HookDefinitionInfos.InterfaceID,
            (void**)&pObject
            );
        if (FAILED(hResult))
        {
            StaticLoadingReportCOMCreationError(&Clsid,&HookDefinitionInfos.InterfaceID,pszFileName,dwLineNumber,TRUE,(tagFakingDllArray)0);
            return FALSE;
        }
        
        if (bCOMAutoHookingEnabled) // object may have been hooked
            pHookedObject=pHookedClass->GetObject(pObject,&HookDefinitionInfos.InterfaceID);
        
        if (!pHookedObject)
        {
            pHookedObject=pHookedClass->AddObject(pObject,&HookDefinitionInfos.InterfaceID);
            if(!pHookedObject)
            {
                CSecureIUnknown::Release(pObject);
                return FALSE;
            }
        }
        pHookedObject->bCreatedByStaticFileLoading=TRUE;
        pLinkListObjectCreatedForStaticHooks->AddItem(pObject);
    }
    


    // if we need a full iid spying, the matching IID file needs to be loaded
    if (HookDefinitionInfos.VTBLInfoType==VTBL_INFO_TYPE_OBJECT_FULL_IID)
    {
        return pHookedClass->SetMonitoringHookFromFile(pHookedObject,
                                                        pObject,
                                                        &HookDefinitionInfos.InterfaceID,
                                                        &HookDefinitionInfos.InterfaceID,
                                                        NULL,
                                                        FALSE,
                                                        pMonitoringFileInfo,
                                                        ppAlreadyHookingMonitoringFileInfo);
    }

    // add static monitoring informations
    if (!pHookedClass->AddMonitoringHookForObjectMethod(pHookedObject,
                                                        pObject,
                                                        NULL,
                                                        pMonitoringFileInfo,
                                                        FALSE,
                                                        &HookDefinitionInfos,
                                                        pszFunctionDescription,
                                                        pszFileName,
                                                        dwLineNumber,
                                                        ppAlreadyHookingMonitoringFileInfo,
                                                        pbParsingError)
        )
    {
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddHookComFakingDefinition
// Object: add a faking hook or pre/post api call hook for com objects
//         __stdcall UnHookAllComObjects must be called by the same thread
// Parameters :
//     in  : FAKING_DLL_INFOS* pFakingDllInfos : Faking file pointer stored in API_INFO struct associated to hook
//           STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos : full hook description
//           TCHAR* pszDllName : faking dll file name
//           DWORD dwFunctionIndex : 0 based index of faking function definition array of faking dll
//           tagFakingDllArray FakingType : faking way : faking, pre or post hook
//     out : 
//           FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos : NULL if method wasn't already hooked
//                                                               faking dll information of dll owning the hook
//           BOOL* pbParsingError : TRUE if a parsing error occurs
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) 
BOOL __stdcall AddHookComFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                          STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                          TCHAR* pszDllName,
                                          DWORD dwFunctionIndex,
                                          tagFakingDllArray FakingType,
                                          FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                          BOOL* pbParsingError)
{
    // pszFullDescription is like COM@IID|function name|options
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    IUnknown* pObject;
    CLSID Clsid;
    HRESULT hResult;

    if (!bHookComLoaded)
        return FALSE;

    *pbParsingError=FALSE;
    *ppAlreadyHookingFakingDllInfos=NULL;


    // decode COM hook definition CLSID + (auto, IDispatch, Object) [+ Index]
    if (!DecodeHookComDefinition(pFakeApiInfos->pszModuleName,&Clsid,&HookDefinitionInfos))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }

    // we can override another full iid so
    if (HookDefinitionInfos.VTBLInfoType==VTBL_INFO_TYPE_OBJECT_FULL_IID)
        return FALSE;


    // fill information necessary to the hook
    FAKING_HOOK_INFORMATIONS FakeHookInfos;
    memcpy(&FakeHookInfos.Clsid,&Clsid,sizeof(CLSID));
    FakeHookInfos.FakingType=(tagHookFakingType)FakingType;
    FakeHookInfos.FakingDllIndex=dwFunctionIndex;
    memcpy(&FakeHookInfos.HookDefinitionInfos,&HookDefinitionInfos,sizeof(HOOK_DEFINITION_INFOS));
    memcpy(&FakeHookInfos.FakeApiInfos,pFakeApiInfos,sizeof(STRUCT_FAKE_API_WITH_USERPARAM));
    FakeHookInfos.pFakingDllInfos=pFakingDllInfos;// store dll info pointer

    // initialize com and ole needed only for static COM Hook
    if (!bOleInitialized)
    {
        OleInitialize(NULL);
        bOleInitialized=TRUE;
    }

    // get or create CHookedClass object associated to the class
    CHookedClass* pHookedClass=GetOrCreateHookedClass(&Clsid);
    if(!pHookedClass)
        return FALSE;

    CHookedObject* pHookedObject;
    // if an object of this type has already been created by our dll, use it instead of creating a new one
    pHookedObject=pHookedClass->GetInternallyCreatedObject();

    // if we already have creating a such object in our class
    if (pHookedObject)
    {
        // get wanted interface
        if (IsEqualIID(HookDefinitionInfos.InterfaceID,IID_IUnknown))
        {
            pObject=pHookedObject->pIUnknown;
        }
        else
        {
            pObject=NULL;
            if(FAILED(CSecureIUnknown::QueryInterface(pHookedObject->pObject,HookDefinitionInfos.InterfaceID,(void**)&pObject)))
            {
                StaticLoadingReportCOMCreationError(&Clsid,&HookDefinitionInfos.InterfaceID,pszDllName,dwFunctionIndex,FALSE,FakingType);
                ReportNotSupportedInterface(&Clsid,&HookDefinitionInfos.InterfaceID);
                return FALSE;
            }
            if (IsBadReadPtr(pObject,sizeof(IUnknown)))
                return FALSE;

            // we only want pointer, so release object to decrement internal object count
            CSecureIUnknown::Release(pObject);
        }
    }
    else
    {
        // creating an object make the corresponding dll to be loaded,
        // and until we don't release this object, dll is not unloaded until the end of the application
        // so we can overwrite functions first bytes and install our hooks
        // --> object will be released only when UnHookAllComObjects wil be called
        hResult=CoCreateInstance(Clsid,
                                NULL,
                                CLSCTX_ALL,
                                HookDefinitionInfos.InterfaceID,
                                (void**)&pObject
                                );
        if (FAILED(hResult))
        {
            StaticLoadingReportCOMCreationError(&Clsid,&HookDefinitionInfos.InterfaceID,pszDllName,dwFunctionIndex,FALSE,FakingType);

            return FALSE;
        }

        if (bCOMAutoHookingEnabled) // object may have been hooked
            pHookedObject=pHookedClass->GetObject(pObject,&HookDefinitionInfos.InterfaceID);

        if(!pHookedObject)
        {
            pHookedObject=pHookedClass->AddObject(pObject,&HookDefinitionInfos.InterfaceID);
            if(!pHookedObject)
            {
                CSecureIUnknown::Release(pObject);
                return FALSE;
            }
        }
        pHookedObject->bCreatedByStaticFileLoading=TRUE;
        pLinkListObjectCreatedForStaticHooks->AddItem(pObject);
    }

    // add static hook
    if (!pHookedClass->AddFakingHookForObjectMethod(pFakingDllInfos,pHookedObject,pObject,&FakeHookInfos,ppAlreadyHookingFakingDllInfos))
    {
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitializeHook
// Object: get Item associated to API_INFO struct defining the hook
//         or if this struct is not initialized, do it, filling 
//         - hook address, 
//         - function name
//         - module name
// Parameters :
//     in  :TCHAR* pszModuleName : name of associated class name 
//          CMethodInfo* pMethodInfo : pointer to method object to hook
//     out : BOOL* pbAlreadyHooked : TRUE if method was already hooked
//           CLinkListItem** ppItemAPIInfo : pointer to an allocated pItemAPIInfo 
//                                           which owns API_INFO struct associated to the hook
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
BOOL InitializeHook(TCHAR* pszModuleName,
                    CMethodInfo* pMethodInfo,
                    CLinkListItem** ppItemAPIInfo,
                    BOOL* pbAlreadyHooked)
{
    if (!bHookComInitialized)
        return FALSE;

    API_INFO*      pApiInfo;
    PBYTE          pAddressToPatch;
    BOOL           bFunctionPointer;
    BOOL           bRet;
    TCHAR*         pszName;
    TCHAR          pszDefaultName[1];
    *pszDefaultName=0;

    // get address to hook
    if (!GetHookAddress(pMethodInfo,&pAddressToPatch,&bFunctionPointer))
        return FALSE;

    // get a pointer to a pApiInfo
    *ppItemAPIInfo=HookComInfos.GetAssociatedItemAPIInfo(pAddressToPatch,pbAlreadyHooked);
    if (!*ppItemAPIInfo)
        return FALSE;

    // if pAddressToPatch has already been patched
    if (*pbAlreadyHooked)
        return TRUE;

    pApiInfo=(API_INFO*)((*ppItemAPIInfo)->ItemData);

    // initialize pApiInfo Object according to ApiOverride dll encoding convention
#if (defined(UNICODE)||defined(_UNICODE))
    if (pMethodInfo->Name)
        pszName=pMethodInfo->Name;
    else
        pszName=pszDefaultName;
    bRet=HookComInfos.InitializeApiInfo(pApiInfo,pszModuleName,pszName);
#else
    if (pMethodInfo->Name)
        CAnsiUnicodeConvert::UnicodeToAnsi(pMethodInfo->Name,&pszName);
    else
        pszName=pszDefaultName;
    
    bRet=HookComInfos.InitializeApiInfo(pApiInfo,pszModuleName,pszName);
    if (pMethodInfo->Name)
        free(pszName);
#endif
    if (!bRet)
    {
        // api hasn't been hooked --> don't need to unhook it

        // free associated memory
        HookComInfos.FreeApiInfoItem(*ppItemAPIInfo);
        return FALSE;
    }
   
    // fill hooking type
    pApiInfo->bFunctionPointer=bFunctionPointer;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetHookAddress
// Object: get address to patch for installing hook from a CMethodInfo object
// Parameters :
//     in  : CMethodInfo* pMethodInfo : pointer to method object to hook
//     out : PBYTE* ppAddressToPatch : address of memory to modify to install hook
//           BOOL* pbFunctionPointer : TRUE if hook must be installed as a function pointer one
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
BOOL GetHookAddress(CMethodInfo* pMethodInfo,OUT PBYTE* ppAddressToPatch,OUT BOOL* pbFunctionPointer)
{
    BOOL bVTBL_Hook;

    // get the hooking way (vtbl or function first bytes)
    bVTBL_Hook=pMethodInfo->CanBeHookedByVTBL();

    // in case of vtbl hook, define hook as a function pointer hook
    *pbFunctionPointer=bVTBL_Hook;

    // get address to patch (vtbl address or function address)
    if (bVTBL_Hook)
        *ppAddressToPatch=pMethodInfo->VTBLAddress;
    else
        *ppAddressToPatch=pMethodInfo->Address;

    // check pointer validity
    if (IsBadCodePtr((FARPROC)*ppAddressToPatch))
    {
        ReportBadFunctionAddress(pMethodInfo,*ppAddressToPatch);
        return FALSE;
    }
    if (bVTBL_Hook)
    {
        if (IsBadCodePtr((FARPROC)(*((PBYTE*)pMethodInfo->VTBLAddress))))
        {
            ReportBadFunctionAddress(pMethodInfo,*((PBYTE*)pMethodInfo->VTBLAddress));
            return FALSE;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetModuleName
// Object: get module name corresponding to address
// Parameters :
//     in  : PBYTE FunctionAddress : function address (or an address inside the corresponding module)
//           DWORD MaxModuleNameSize : module name max size
//     out : TCHAR* pszModuleName : module name
//           HMODULE* pModuleHandle
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
BOOL GetModuleName(PBYTE FunctionAddress,TCHAR* pszModuleName,DWORD MaxModuleNameSize,HMODULE* pModuleHandle)
{
    *pszModuleName=0;

    if (!bHookComInitialized)
        return FALSE;

    if(MaxModuleNameSize<MAX_PATH)
        return FALSE;
    
    PBYTE RelativeAdress;
    BOOL  ShouldLog;


    return HookComInfos.GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
                        (
                            FunctionAddress,
                            pModuleHandle,
                            pszModuleName,
                            &RelativeAdress,
                            &ShouldLog,
                            TRUE,
                            TRUE
                        );

}


//-----------------------------------------------------------------------------
// Name: GetHookDefinitionInfo
// Object: decode following com options
//            "ExposedThrowIDispatch"
//            "IIDVTBLIndex={IID}:Index"
//            "BaseIID={IID}"
//          and fill an HOOK_DEFINITION_INFOS struct
// Parameters :
//     in  : TCHAR* pszDefinition : string containing definition
//           IID* pCurrentIID : pointer to current interface Id of interface being described 
//                              (NULL if no interface is specified)
//     out : HOOK_DEFINITION_INFOS* pHookDefinitionInfos : struct containing infos
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
BOOL GetHookDefinitionInfo(TCHAR* pszDefinition,IN IID* pCurrentIID, OUT HOOK_DEFINITION_INFOS* pHookDefinitionInfos)
{
    // hook definition is like
    // empty string (=ExposedThrowIDispatch)
    // "VTBLIndex=3"
    // "ExposedThrowIDispatch"
    // "IIDVTBLIndex={IID}:Index"   // static monitoring file only
    // "BaseIID={IID}"              // for linked IID files

    pszDefinition=CTrimString::TrimString(pszDefinition);
    // default interface id ref to IUnknown (we are sure it exists for all objects)
    pHookDefinitionInfos->InterfaceID=IID_IUnknown;

    // in case of no vtbl info specified
    if (*pszDefinition==0)
    {
        pHookDefinitionInfos->VTBLInfoType=VTBL_INFO_TYPE_EXPOSED_THROW_IDISPATCH;
        // store interface id ref
        pHookDefinitionInfos->InterfaceID=IID_IDispatch;
        return TRUE;
    }

    // look for inclusion of a base Interface IID file
    if (_tcsnicmp(pszDefinition,COM_DEFINITION_BASE_IID,_tcslen(COM_DEFINITION_BASE_IID))==0)
    {
        // store option type
        pHookDefinitionInfos->VTBLInfoType=VTBL_INFO_TYPE_OBJECT_FULL_IID;

        // point after option name
        pszDefinition+=_tcslen(COM_DEFINITION_BASE_IID);

        // get IID value
        if (!CGUIDStringConvert::IIDFromTchar(pszDefinition,&pHookDefinitionInfos->InterfaceID))
            return FALSE;

        return TRUE;
    }

    // look for current IID object vtbl index
    if (_tcsnicmp(pszDefinition,COM_DEFINITION_CURRENT_IID_VTBL_INDEX,_tcslen(COM_DEFINITION_CURRENT_IID_VTBL_INDEX))==0)
    {
        if (pCurrentIID==NULL)
            return FALSE;

        // store option type
        pHookDefinitionInfos->VTBLInfoType=VTBL_INFO_TYPE_OBJECT_IID;

        // store interface id ref
        pHookDefinitionInfos->InterfaceID=*pCurrentIID;

        // point after option name
        pszDefinition+=_tcslen(COM_DEFINITION_CURRENT_IID_VTBL_INDEX);

        // get option value
        if(!CStringConverter::StringToDWORD(pszDefinition,&pHookDefinitionInfos->VTBLIndex))
            return FALSE;
    }
    // look for exposed throw IDispatch
    else if (_tcsnicmp(pszDefinition,COM_DEFINITION_EXPOSED_THROW_IDISPATCH,_tcslen(COM_DEFINITION_EXPOSED_THROW_IDISPATCH))==0)
    {
        // store option type
        pHookDefinitionInfos->VTBLInfoType=VTBL_INFO_TYPE_EXPOSED_THROW_IDISPATCH;
    }
    // look for another IID object vtbl index
    else if (_tcsnicmp(pszDefinition,COM_DEFINITION_OBJECT_VTBL_INDEX,_tcslen(COM_DEFINITION_OBJECT_VTBL_INDEX))==0)
    {
        // string is like ObjectIIDVTBLIndex={IID}:Index
        TCHAR* pszIndexValue;

        // store option type
        pHookDefinitionInfos->VTBLInfoType=VTBL_INFO_TYPE_OBJECT_IID;

        // point after option name
        pszDefinition+=_tcslen(COM_DEFINITION_OBJECT_VTBL_INDEX);

        // find ':'
        pszIndexValue=_tcschr(pszDefinition,':');
        if (!pszIndexValue)
            return FALSE;

        *pszIndexValue=0;
        pszIndexValue++;

        // get IID value
        if (!CGUIDStringConvert::IIDFromTchar(pszDefinition,&pHookDefinitionInfos->InterfaceID))
            return FALSE;

        // get option value
        if(!CStringConverter::StringToDWORD(pszIndexValue,&pHookDefinitionInfos->VTBLIndex))
            return FALSE;
    }
    else
    {
        TCHAR pszMsg[MAX_PATH];
        _sntprintf(pszMsg,MAX_PATH,
            _T("Unknown option %s"),
            pszDefinition);
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DecodeHookComDefinition
// Object: decode a COM hook predefinition for static monitoring files and faking dll
//          and fill HOOK_DEFINITION_INFOS struct
// Parameters :
//     in  : TCHAR* pszDefinition : string containing definition
//     out : CLSID* pCLSID : class Id of object to hook
//           HOOK_DEFINITION_INFOS* pHookDefinitionInfos : struct containing infos
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
BOOL DecodeHookComDefinition(TCHAR* pszDefinition,OUT CLSID* pCLSID, OUT HOOK_DEFINITION_INFOS* pHookDefinitionInfos)
{
    // COMDef|Function description --> hook com definition is like
    // COM@CLSID|Function description    // empty string (=ExposedThrowIDispatch)
    // COM@CLSID@ExposedThrowIDispatch|Function description
    // COM@CLSID@ObjectIIDVTBLIndex={IID}:Index|Function description
    // COM@CLSID@BaseIID={IID} // for linked IID files --> no "|Function description"
    TCHAR* pszOptionnalParameter;

    // remove first spaces if any
    while(*pszDefinition==' ')
        pszDefinition++;

    // check if begin with COM@
    if(_tcsnicmp(pszDefinition,COM_DEFINITION_PREFIX,_tcslen(COM_DEFINITION_PREFIX))!=0)
        return FALSE;

    // point after COM@
    pszDefinition+=_tcslen(COM_DEFINITION_PREFIX);

    // looks for next @
    pszOptionnalParameter=_tcschr(pszDefinition,'@');
    if (pszOptionnalParameter)
    {
        *pszOptionnalParameter=0;
        pszOptionnalParameter++;
    }

    // convert string CLSID to CLSID
    if (!CGUIDStringConvert::CLSIDFromTchar(pszDefinition,pCLSID))
        return FALSE;

    // get parameter value
    if (pszOptionnalParameter)
    {
        if (!GetHookDefinitionInfo(pszOptionnalParameter,NULL,pHookDefinitionInfos))
            return FALSE;
    }

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: StartDllUnloadWatching
// Object: Start watching unloading dll to report not fully released COM objects
//
//   Notice :
//          as we have take the choice for not using windows debug function
//          like DebugProcess for hooking, 
//          we can't watch dll unload with UNLOAD_DLL_DEBUG_EVENT.
//          Spying NtUnmapViewOfSection is quite time consuming
//          and doesn't give information on loading count (that means we have to 
//          call "Undocumented" win api function to check if dll is loaded, 
//          and as we query it very shortly, these api tell us that dll is still loaded
//          even if it is really unloaded few ms after IsBadCodePtr and IsBadReadPtr give
//          the same result)
//
//          in conclusion : until I find nothing more easy and secure, I will do a polling thread
//                          (You are really encourage to provide me solution if you got a working one)
//          
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------

BOOL StartDllUnloadWatching()
{
    // if dll unload watching is already started
    if (hDllUnloadWatchingThread)
        return TRUE;

    // reset event to stop thread
    ResetEvent(hevtStopDllUnloadWatching);

    // create watching thread
    hDllUnloadWatchingThread=CreateThread(NULL,0,WatchUnloadingDll,NULL,0,NULL);

    return (hDllUnloadWatchingThread!=NULL);
}


//-----------------------------------------------------------------------------
// Name: StopDllUnloadWatching
// Object: stop watching dll unloading
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL StopDllUnloadWatching()
{
    // if thread not started
    if (!hDllUnloadWatchingThread)
        return FALSE;

    // set event to stop thread
    SetEvent(hevtStopDllUnloadWatching);

    // clear thread handle
    CloseHandle(hDllUnloadWatchingThread);
    hDllUnloadWatchingThread=NULL;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WatchUnloadingDll
// Object: watch for unloaded dll
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------
DWORD WINAPI WatchUnloadingDll(LPVOID lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    CLinkListItem* pItem;
    CHookedClass* pHookedClass;

    const DWORD MaxPoolingWait=60;
    DWORD InitialTicks;
    DWORD Ticks;
    DWORD dwRes;
    HANDLE ph[2];
    ph[0]=HookComInfos.hevtFreeProcess;
    ph[1]=hevtStopDllUnloadWatching;

    WatchUnloadingDllThreadClosedCleanly=FALSE;

    for (;;)
    {
        dwRes=WaitForMultipleObjects(2,ph,FALSE,HOOK_COM_DLL_UNLOAD_WATCHING_POOLING_TIME);
        if (dwRes!=WAIT_TIMEOUT)// if an event has happen, stop thread
            return 0;


        // else, for each hooked class, check that dll is still loaded

        // add lock to pLinkListHookedClasses
        pLinkListHookedClasses->Lock(FALSE);

        // for each hooked class
        for (pItem=pLinkListHookedClasses->Head;pItem;pItem=pItem->NextItem)
        {
            // get pointer on hooked class object
            pHookedClass=(CHookedClass*)pItem->ItemData;

            if (!pHookedClass->IsAssociatedDllLoaded())
            {
                // Cnt only to avoid infinite loop in case of crash of targeted application in Release function call
                InitialTicks=GetTickCount();
                Ticks=InitialTicks;
                while( (pHookedClass->NbReleaseProcessing) && (Ticks-InitialTicks<MaxPoolingWait) )
                {
                    Ticks=GetTickCount();
                    Sleep(1);
                }

                if ( (Ticks-InitialTicks) >= MaxPoolingWait )
                    pHookedClass->NbReleaseProcessing=0;

                // check again that dll is still unloaded (in case an object of same type was created again,
                // this avoid to remove new hooked object and report it has a not released one
                // this should not occurs thanks to pLinkListHookedClasses->Lock)
                if (pHookedClass->IsAssociatedDllLoaded())
                    continue;

                // free previously hooked object and report bad release state
                pHookedClass->FreeAllHookedObjectsAndReportNotReleased();
            }
        }

        pLinkListHookedClasses->Unlock();
    }

    WatchUnloadingDllThreadClosedCleanly=TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsMethodHookSharedWithAnotherInterface
// Object: assume that method hook belongs only to specified pMethodInfo
// Parameters :
//          in : CMethodInfo* pMethodInfo : method info for which we want hook removal
//               CHookedInterface* pLockedInterface : interface owning pMethodInfo, must have pHookedInterface->pMethodInfoList locked
//               CHookedClass* pLockedInterfacesHookedClass : class owning pMethodInfo, must have pHookedClass->pLinkListHookedInterfaces locked
// Return : TRUE if hook is shared with other pMethodInfo
//-----------------------------------------------------------------------------
BOOL IsMethodHookSharedWithAnotherInterface(CMethodInfo* pMethodInfo,CHookedInterface* pLockedInterface,CHookedClass* pLockedInterfacesHookedClass,BOOL bLinkListHookedClassesLocked)
{
    CLinkListItem* pItem;
    CHookedClass* pHookedClass;
    CLinkListItem* pItemInterface;
    CHookedInterface* pHookedInterface;
    CLinkListItem* pItemMethodInfo;
    CMethodInfo* pAssociatedMethodInfo;

    // add lock to pLinkListHookedClasses
    if (!bLinkListHookedClassesLocked)
        pLinkListHookedClasses->Lock(FALSE);

    // for each hooked class
    for (pItem=pLinkListHookedClasses->Head;pItem;pItem=pItem->NextItem)
    {
        // get pointer on hooked class object
        pHookedClass=(CHookedClass*)pItem->ItemData;

        // lock interface list of hooked class
        if (pHookedClass!=pLockedInterfacesHookedClass)
            pHookedClass->pLinkListHookedInterfaces->Lock();

        // for each hooked interface
        for (pItemInterface=pHookedClass->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pItemInterface->NextItem)
        {
            pHookedInterface=(CHookedInterface*)pItemInterface->ItemData;

            // lock pMethodInfoList
            if (pHookedInterface!=pLockedInterface)
                pHookedInterface->pMethodInfoList->Lock();

            // for each method info of this interface
            for (pItemMethodInfo=pHookedInterface->pMethodInfoList->Head;pItemMethodInfo;pItemMethodInfo=pItemMethodInfo->NextItem)
            {
                pAssociatedMethodInfo=(CMethodInfo*)pItemMethodInfo->ItemData;
                if (IsBadReadPtr(pAssociatedMethodInfo,sizeof(CMethodInfo)))
                    continue;

                // if pAssociatedMethodInfo is the current pMethodInfo we want to watch
                if (   (pHookedClass==pLockedInterfacesHookedClass)
                    && (pHookedInterface==pLockedInterface)
                    && (pAssociatedMethodInfo==pMethodInfo)
                    )
                    // go to next item
                    continue;

                if (pAssociatedMethodInfo->pItemAPIInfo==pMethodInfo->pItemAPIInfo)
                {
                    // unlock pMethodInfoList
                    if (pHookedInterface!=pLockedInterface)
                        pHookedInterface->pMethodInfoList->Unlock();

                    // unlock interface list of hooked class
                    if (pHookedClass!=pLockedInterfacesHookedClass)
                        pHookedClass->pLinkListHookedInterfaces->Unlock();

                    // remove lock to pLinkListHookedClasses
                    if (!bLinkListHookedClassesLocked)
                        pLinkListHookedClasses->Unlock();

                    return TRUE;
                }
            }

            // unlock pMethodInfoList
            if (pHookedInterface!=pLockedInterface)
                pHookedInterface->pMethodInfoList->Unlock();
        }

        // unlock interface list of hooked class
        if (pHookedClass!=pLockedInterfacesHookedClass)
            pHookedClass->pLinkListHookedInterfaces->Unlock();
    }

    // remove lock to pLinkListHookedClasses
    if (!bLinkListHookedClassesLocked)
        pLinkListHookedClasses->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CreateInterfaceMonitoringFile
// Object:  create an interface monitoring file
// Parameters :
//     in  : TCHAR* pszMonitoringFileName : filename
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CreateInterfaceMonitoringFile(TCHAR* pszMonitoringFileName)
{
    TCHAR pszMsg[2*MAX_PATH];
    HANDLE hFile;
    // create a new one
    if (!CTextFile::CreateTextFile(pszMonitoringFileName,&hFile))
    {
        _stprintf(pszMsg,_T("Error creating file \r\n%s"),pszMonitoringFileName);
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // write monitoring file header
    CTextFile::WriteText(hFile,HOOK_COM_INTERFACE_MONITORING_FILE_HEADER);

    // close file
    CloseHandle(hFile);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CreateInterfaceMonitoringFile
// Object:  create an interface monitoring file
// Parameters :
//     in  : IID* pIid : interface identifier
//     out : TCHAR* pszMonitoringFileName : name of monitoring file. Must be MAX_PATH length at least
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL GetMonitoringFileName(IID* pIid,OUT TCHAR* pszMonitoringFileName)
{
    TCHAR _tIID[STRING_GUID_SIZE];

    *pszMonitoringFileName=0;
    // get string iid corresponding to interface 
    if (!CGUIDStringConvert::TcharFromIID(pIid,_tIID))
        return FALSE;

    // get path of com monitoring files
    _tcscpy(pszMonitoringFileName,pszMonitoringFilesPath);
    _tcsncat(pszMonitoringFileName,_tIID,MAX_PATH-1-_tcslen(pszMonitoringFileName));
    _tcsncat(pszMonitoringFileName,_T(".txt"),MAX_PATH-1-_tcslen(pszMonitoringFileName));
    pszMonitoringFileName[MAX_PATH-1]=0;

    return TRUE;
}


// called each time a registered item is going to be deleted
void __stdcall ApiInfoItemDeletionCallback(CLinkListItem* pItemAPIInfo)
{
    if (!bHookComLoaded)
        return;
    CLinkListItem* pItem;
    CHookedClass* pHookedClass;
    CLinkListItem* pItemInterface;
    CHookedInterface* pHookedInterface;
    CLinkListItem* pItemMethodInfo;
    CMethodInfo* pMethodInfo;

    // add lock to pLinkListHookedClasses
    pLinkListHookedClasses->Lock(FALSE);

    // for each hooked class
    for (pItem=pLinkListHookedClasses->Head;pItem;pItem=pItem->NextItem)
    {
        // get pointer on hooked class object
        pHookedClass=(CHookedClass*)pItem->ItemData;

        // lock interface list of hooked class
        pHookedClass->pLinkListHookedInterfaces->Lock();

        // for each hooked interface
        for (pItemInterface=pHookedClass->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pItemInterface->NextItem)
        {
            pHookedInterface=(CHookedInterface*)pItemInterface->ItemData;

            // lock pMethodInfoList
            pHookedInterface->pMethodInfoList->Lock();

            // for each method info of this interface
            for (pItemMethodInfo=pHookedInterface->pMethodInfoList->Head;pItemMethodInfo;pItemMethodInfo=pItemMethodInfo->NextItem)
            {
                pMethodInfo=(CMethodInfo*)pItemMethodInfo->ItemData;
                if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
                    continue;
                
                // if pMethodInfo->pItemAPIInfo points to the item that is going to be deleted 
                if (pMethodInfo->pItemAPIInfo==pItemAPIInfo)
                    // empty pMethodInfo->pItemAPIInfo
                    pMethodInfo->pItemAPIInfo=NULL;
            }

            // unlock pMethodInfoList
            pHookedInterface->pMethodInfoList->Unlock();
        }

        // unlock interface list of hooked class
        pHookedClass->pLinkListHookedInterfaces->Unlock();
    }

    // remove lock to pLinkListHookedClasses
    pLinkListHookedClasses->Unlock();
}

//-----------------------------------------------------------------------------
// Name: ClearUserDataTypeCache
// Object: clear user data type cache
//         called on each file change in UserTypes directory
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ClearUserDataTypeCache()
{
    CSupportedParameters::ClearUserDataTypeCache();
    return TRUE;
}