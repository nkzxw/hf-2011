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

// to avoid rebasing, use the linker/advanced/BaseAddress option 
#include "../SupportedParameters.h"
#include <windows.h>
#include "APIMonitoringFileLoader.h"
#include "APIOverrideKernel.h"
#include "FakeApiLoader.h"
#include "ProcessInternalCallRequest.h"
#include "ModuleFilters.h"
#include "COM_Manager.h"
#include "NET_Manager.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../InterProcessCommunication.h"
#include "../../../CleanCloseHandle/CleanCloseHandle.h"
#include "../../../APIError/APIError.h"
#include "../../singleinstance/csingleinstance.h"
#include "SetUserWindowStation.h"
#include "DumpUserInterface.h"
#include "reportmessage.h"
#include "../HookAvailabilityCheck/HookAvailabilityCheck.h"

// we don't want to link to the user32.dll, so don't use msg box inside MailSlotServer
// assume TOOLS_NO_MESSAGEBOX is defined in project options/c++/preprocessor
#include "../../MailSlot/MailSlotServer.h"

#include "DynamicLoadedFuncs.h"

extern "C" LPVOID __stdcall TlsGetValueApiSubstitution(DWORD dwTlsIndex);
extern "C" BOOL __stdcall TlsSetValueApiSubstitution(DWORD dwTlsIndex, LPVOID lpTlsValue);

DWORD dwSystemPageSize=4096;
DWORD dwCurrentProcessID=0;
CLinkList* pLinkListAPIInfos=NULL;
CLinkList* pLinkListpMonitoringFileInfos=NULL;
CLinkList* pLinkListpFakeDllInfos=NULL;
CLinkList* pLinkListAPIInfosToBeFree=NULL;
CLinkList* pLinkListLoadedDll=NULL;
CMailSlotClient* pMailSlotClt=NULL;
CMailSlotServer* pMailSlotSrv=NULL;
CSingleInstance* pSingleInstance=NULL;
CModulesFilters* pModulesFilters=NULL;
CSetUserWindowStation* pSetUserWindowStation=NULL;
CCOM_Manager* pComManager=NULL;
CNET_Manager* pNetManager=NULL;
HANDLE hevtFreeAPIInfo=NULL;
HANDLE hevtAllAPIUnhookedDllFreeAll=NULL;
HANDLE hevtWaitForUnlocker=NULL;
HANDLE hevtEndOfProcessInternalCall=NULL;
HANDLE hevtSimpleHookCriticalCallUnlockedEvent=NULL;

BOOL bMonitoring=TRUE;
BOOL bFaking=TRUE;
BOOL bFiltersApplyToMonitoring=TRUE;
BOOL bFiltersApplyToFaking=TRUE;

tagFirstBytesAutoAnalysis FirstBytesAutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_NONE;
BOOL bDebugMonitoringFile=FALSE;
BOOL bLogCallStack=FALSE;
DWORD CallStackEbpRetrievalSize=4*sizeof(PBYTE);
BOOL bBreakDialogDontBreakAPIOverrideThreads=FALSE;
// bBreakDialogDontBreakAPIOverrideThreads needs to let only 2 threads 
// running for communication between WinApiOverride and ApiOverride dll
// those threads are the watching events thread and the mailslot server thread
DWORD APIOverrideThreads[NB_APIOVERRIDE_WORKER_THREADS]={0};

BOOL bProcessFreeDone=FALSE;
BOOL FreeingThreadGracefullyClosed=FALSE;

//(Injector -> APIOverride.dll)
HANDLE hevtStartMonitoring=NULL;
HANDLE hevtStopMonitoring=NULL;
HANDLE hevtStartFaking=NULL;
HANDLE hevtStopFaking=NULL;
HANDLE hevtFreeProcess=NULL;
HANDLE hevtSingleThreadedMailSlotServerEndLoop=NULL;
DWORD NumberOfCurrentRemoteCalls=0;
    
// (APIOverride.dll -> Injector)
HANDLE hevtAPIOverrideDllProcessAttachCompleted=NULL;
HANDLE hevtAPIOverrideDllProcessDetachCompleted=NULL;
HANDLE hevtProcessFree=NULL;
HANDLE hevtMonitoringFileLoaded=NULL;
HANDLE hevtMonitoringFileUnloaded=NULL;
HANDLE hevtFakeAPIDLLLoaded=NULL;
HANDLE hevtFakeAPIDLLUnloaded=NULL;
HANDLE hevtClientMailslotOpen=NULL;
HANDLE hevtError=NULL;
HANDLE hevtUnload=NULL;

TCHAR ModulePath[MAX_PATH];
TCHAR DefinesPath[MAX_PATH];
TCHAR UserTypesPath[MAX_PATH];

BOOL bInsideTlsCallback=FALSE;
BOOL bSuccessfulLoading=FALSE;
HANDLE hThreadFreeingHooks=NULL;
BYTE FloatingNotSet[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff};
double dFloatingNotSet;
LARGE_INTEGER PerformanceFrequency;
LARGE_INTEGER ReferenceCounter;
LARGE_INTEGER ReferenceTime;
HANDLE hevtCreatedCOMObjectsForStaticHooksReleased=NULL;
TCHAR GpszMailslotServerName[2*MAX_PATH];
HANDLE ApiOverrideHeap=NULL;// current dll heap, avoid to interact with original process heap
HANDLE ApiOverrideLogHeap=NULL;// very short heap only for temporary parameters allocation before sending log
HANDLE ApiOverrideKernelHeap=NULL;// short heap for function call
HANDLE ApiOverrideLogApiTmpHeap=NULL;// quick heap used by LogApi.cpp

DWORD ApiOverrideTlsIndex=0;
DWORD ApiOverrideListCriticalPartTlsIndex=0;// FALSE : not in critical part, item can be added in pLinkListTlsData linked list
                                            // TRUE : in critical part, item can't be added in pLinkListTlsData linked list
DWORD ApiOverrideSimpleHookCriticalCallTlsIndex=0;
CLinkListSimple* pLinkListAllThreadTlsData=NULL;// link list containing pointers of all created thread tls data (CApiOverrideTlsData* list)
BOOL ApiOverrideDllDetaching=FALSE;
#ifndef HEAP_CREATE_ENABLE_EXECUTE
    #define HEAP_CREATE_ENABLE_EXECUTE 0x00040000
#endif 

void FreeAll(BOOL CleanWay);
void MailSlotSrvCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData);
DWORD WINAPI ThreadWatchingEventsProc(LPVOID lpParameter);
DWORD WINAPI ThreadDump(LPVOID lpParameter);
DWORD WINAPI ThreadProcessInternalCallRequest(LPVOID lpParameter);

typedef struct tagPluginCommunicationMessageInfos
{
    PBYTE Data;
    SIZE_T DataSize;
    HANDLE hWaitingEvent;
}PLUGIN_COMMUNICATION_MESSAGE_INFOS;
BOOL PluginReplyToOverridingDllQuery(PVOID pData,SIZE_T DataSize);
BOOL OverridingDllSendDataToPlugin(TCHAR* PluginName,PLUGIN_COMMUNICATION_MESSAGE_INFOS* pMessageInfos,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize);
BOOL STDMETHODCALLTYPE OverridingDllSendDataToPlugin(TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize);
BOOL STDMETHODCALLTYPE OverridingDllSendDataToPluginAndWaitReply(TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,OUT PBYTE* DataFromPlugin,OUT SIZE_T* DataFromPluginSize,DWORD WaitTimeInMs /* INFINITE for infinite wait */);

BOOL CreateApioverrideDllMailslotServer(TCHAR* pszMailslotServerName)
{
    pMailSlotSrv=new CMailSlotServer(pszMailslotServerName,MailSlotSrvCallback,NULL);
    if (!pMailSlotSrv->Start(TRUE))
    {
        TCHAR pszMsg[2*MAX_PATH];
        SetEvent(hevtError);

        _sntprintf(pszMsg,2*MAX_PATH,_T("Error starting MailSlot server %s"),pszMailslotServerName);
        DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

        FreeAll(FALSE);
        CleanCloseHandle(&hevtAPIOverrideDllProcessDetachCompleted);
        return FALSE;
    }

    // add mailslot server thread Id to ApiOverride dll working thread
    pMailSlotSrv->GetServerThreadId(&APIOverrideThreads[1]);

    SetEvent(hevtClientMailslotOpen);
    return TRUE;
}

void SingleThreadedMailSlotServerCheck(TCHAR* pszMailslotServerName)
{
    bInsideTlsCallback=TRUE;

    DWORD dwResult;
    DWORD dwError;
    DWORD dwBytesRead=0;
    DWORD dwMsgSize=0;
    DWORD dwMessageCount;

    DWORD dwMaxMessageSize=512;
    PBYTE pMailSlotData=new BYTE[dwMaxMessageSize];
    OVERLAPPED Overlapped;
    memset(&Overlapped,0,sizeof(OVERLAPPED));
    HANDLE hOverlappedEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    Overlapped.hEvent=hOverlappedEvent;
    HANDLE ph[2]={hevtSingleThreadedMailSlotServerEndLoop,Overlapped.hEvent};

    // use mailslot server in current thread, as thread creation will start only after all
    // nt loader as finished is job and tls callbacks have been called
    HANDLE hMailslot=CreateMailslot(pszMailslotServerName,0,MAILSLOT_WAIT_FOREVER,0);

    // apioverride dll is fully loaded (only after mailslot creation)
    SetEvent(hevtAPIOverrideDllProcessAttachCompleted);
    SetEvent(hevtClientMailslotOpen);

    for (;;)
    {
        // see CMailSlotClient : in the same write operation we write data size on first DWORD
        // and next data
        if (!ReadFile(hMailslot, pMailSlotData, dwMsgSize, &dwBytesRead, &Overlapped))
        {
            dwError=GetLastError();
            if (dwError==ERROR_INSUFFICIENT_BUFFER)
            {
                if(GetMailslotInfo(hMailslot, 0, &dwMsgSize, &dwMessageCount, NULL))
                {
                    // adjust our local buffer size
                    if (dwMaxMessageSize<dwMsgSize)
                    {
                        delete[] pMailSlotData;
                        pMailSlotData=new BYTE[dwMsgSize];
                        dwMaxMessageSize=dwMsgSize;
                    }
                    // restart the read operation with correct buffer size
                    continue;
                }
                else
                {
                    goto CleanUp;
                }
            }

            if (dwError!=ERROR_IO_PENDING)
            {
                goto CleanUp;
            }
        }
        // wait for overlapped event or end loop event
        dwResult = WaitForMultipleObjects(2,ph,FALSE,INFINITE);
        switch (dwResult)
        {
        case WAIT_OBJECT_0:// end loop
            goto CleanUp;
        case WAIT_OBJECT_0+1:// read event
            if (dwMsgSize==0)
                break;

            MailSlotSrvCallback(pMailSlotData,dwBytesRead,NULL);
            // gives a new fake buffer size to allow to catch ERROR_INSUFFICIENT_BUFFER error
            dwMsgSize=0;
            break;
        case WAIT_FAILED:
            goto CleanUp;
        }
    }


CleanUp:
    ResetEvent(hevtClientMailslotOpen);
    CloseHandle(hMailslot); // close mailslot server
    CloseHandle(hOverlappedEvent);// close overlapped event
    hOverlappedEvent=NULL;
    delete[] pMailSlotData;
}

 void UserTypeParsingReportError(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam)
 {
     UNREFERENCED_PARAMETER(UserParam);
    CReportMessage::ReportMessage(REPORT_MESSAGE_ERROR,ErrorMessage);
 }

HINSTANCE DllhInstance;
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);
    
    TCHAR psz[MAX_PATH+50];
    TCHAR pszMsg[2*MAX_PATH];
    TCHAR pszPID[50];
    SYSTEM_INFO siSysInfo;
    HANDLE hThreadWatchingEvents=NULL;
    FILETIME ft;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DllhInstance=hInstDLL;

            // get process ID
            dwCurrentProcessID=GetCurrentProcessId();

            pLinkListAllThreadTlsData=new CLinkListSimple();

            // get CPU counter frequency
            QueryPerformanceFrequency(&PerformanceFrequency);
            // get performance counter before GetSystemTimeAsFileTime as it's more precise
            QueryPerformanceCounter(&ReferenceCounter);
            // get UTC time
            GetSystemTimeAsFileTime(&ft);
            // convert UTC time to local filetime
            FileTimeToLocalFileTime(&ft,&ft);
            ReferenceTime.HighPart=ft.dwHighDateTime;
            ReferenceTime.LowPart=ft.dwLowDateTime;

            // get page size
            GetSystemInfo(&siSysInfo); 
            dwSystemPageSize=siSysInfo.dwPageSize;

            // mark heap as executable for first bytes analysis use (for processors using "non executable heap" protection)
            ApiOverrideHeap=HeapCreate(HEAP_CREATE_ENABLE_EXECUTE | HEAP_GROWABLE,dwSystemPageSize,0);
            // create log heap
            ApiOverrideLogHeap=HeapCreate(HEAP_GROWABLE,dwSystemPageSize,0);
            // create kernel heap
            ApiOverrideKernelHeap=HeapCreate(HEAP_GROWABLE,dwSystemPageSize,0);
            // logapi heap
            ApiOverrideLogApiTmpHeap=HeapCreate(HEAP_GROWABLE,dwSystemPageSize,0);

            // allocate memory for changing window station
            pSetUserWindowStation=new CSetUserWindowStation();

            // store the invalid floating result in dFloatingNotSet
            memcpy(&dFloatingNotSet,FloatingNotSet,8);

            // signal dll call to be logged/faked only once for |FirstBytesCanExecuteAnyWhere option
            AddAPIOverrideInternalModule(hInstDLL);

            CStdFileOperations::GetModulePath(hInstDLL,ModulePath,MAX_PATH);
            _tcscpy(DefinesPath,ModulePath);
            _tcscat(DefinesPath,APIOVERRIDE_DEFINES_PATH);
            _tcscpy(UserTypesPath,ModulePath);
            _tcscat(UserTypesPath,APIOVERRIDE_USER_TYPES_PATH);

            // pid -> string
            _stprintf(pszPID,_T("0x%X"),dwCurrentProcessID);

            // open error event
            _tcscpy(psz,APIOVERRIDE_EVENT_ERROR);
            _tcscat(psz,pszPID);
            hevtError=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);

            // create event for extreme case
            hevtSimpleHookCriticalCallUnlockedEvent = CreateEvent(NULL,FALSE,TRUE,NULL);

            // allocate thread local storage memory
            ApiOverrideTlsIndex=TlsAlloc();// MSDN : The slots for the index are initialized to zero
            ApiOverrideListCriticalPartTlsIndex=TlsAlloc();
            ApiOverrideSimpleHookCriticalCallTlsIndex=TlsAlloc();
            if (    (ApiOverrideTlsIndex==TLS_OUT_OF_INDEXES) 
                 || (ApiOverrideListCriticalPartTlsIndex==TLS_OUT_OF_INDEXES)
               )
            {
                if (hevtError)
                    SetEvent(hevtError);
                _sntprintf(pszMsg,2*MAX_PATH,_T("Error Allocating Tls for Process Id %s"),pszPID);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                return FALSE;
            }

            // check that only one apioverride.dll is injected
            _tcscpy(psz,APIOVERRIDE_MUTEX);
            _tcscat(psz,pszPID);
            pSingleInstance=new CSingleInstance(psz);
            if (pSingleInstance->IsAnotherInstanceRunning())
            {
                if (hevtError)
                    SetEvent(hevtError);
                _sntprintf(pszMsg,2*MAX_PATH,_T("Error API override already loaded for Process Id %s"),pszPID);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                delete pSingleInstance;
                if (pSetUserWindowStation)
                    delete pSetUserWindowStation;
                return FALSE;
            }

            //////////////////////////
            // try to open events
            //////////////////////////

            //(Injector -> APIOverride.dll)
            _tcscpy(psz,APIOVERRIDE_EVENT_START_MONITORING);
            _tcscat(psz,pszPID);
            hevtStartMonitoring=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_STOP_MONITORING);
            _tcscat(psz,pszPID);
            hevtStopMonitoring=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_START_FAKING);
            _tcscat(psz,pszPID);
            hevtStartFaking=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_STOP_FAKING);
            _tcscat(psz,pszPID);
            hevtStopFaking=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_FREE_PROCESS);
            _tcscat(psz,pszPID);
            hevtFreeProcess=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);

            _tcscpy(psz,APIOVERRIDE_EVENT_SINGLETHREADEDMAILSLOTSERVER_END_WAITING_INSTRUCTIONS_LOOP);
            _tcscat(psz,pszPID);
            hevtSingleThreadedMailSlotServerEndLoop=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);// can be null if no tls hook

            // (APIOverride.dll -> Injector)
            _tcscpy(psz,APIOVERRIDE_EVENT_DLLPROCESS_ATTACH_COMPLETED);
            _tcscat(psz,pszPID);
            hevtAPIOverrideDllProcessAttachCompleted=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_DLL_DETACHED_COMPLETED);
            _tcscat(psz,pszPID);
            hevtAPIOverrideDllProcessDetachCompleted=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_PROCESS_FREE);
            _tcscat(psz,pszPID);
            hevtProcessFree=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_MONITORING_FILE_LOADED);
            _tcscat(psz,pszPID);
            hevtMonitoringFileLoaded=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_MONITORING_FILE_UNLOADED);
            _tcscat(psz,pszPID);
            hevtMonitoringFileUnloaded=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_FAKE_API_DLL_LOADED);
            _tcscat(psz,pszPID);
            hevtFakeAPIDLLLoaded=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_FAKE_API_DLL_UNLOADED);
            _tcscat(psz,pszPID);
            hevtFakeAPIDLLUnloaded=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
            _tcscpy(psz,APIOVERRIDE_EVENT_CLIENT_MAILSLOT_OPEN);
            _tcscat(psz,pszPID);
            hevtClientMailslotOpen=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);

            if (!(hevtStartMonitoring&&hevtStopMonitoring&&hevtStartFaking&&hevtStopFaking&&hevtFreeProcess
                &&hevtAPIOverrideDllProcessAttachCompleted&&hevtAPIOverrideDllProcessDetachCompleted&&hevtProcessFree
                &&hevtMonitoringFileLoaded&&hevtMonitoringFileUnloaded
                &&hevtFakeAPIDLLLoaded&&hevtFakeAPIDLLUnloaded
                &&hevtClientMailslotOpen
                &&hevtError))
            {
                if (hevtError)
                    SetEvent(hevtError);

                _sntprintf(pszMsg,2*MAX_PATH,_T("Error opening named events for Process Id %s"),pszPID);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                FreeAll(FALSE);
                CleanCloseHandle(&hevtAPIOverrideDllProcessDetachCompleted);
                return FALSE;
            }

            //////////////////////////
            // create mailslots 
            //////////////////////////

            // client for sending logs to spying process
            _tcscpy(psz,APIOVERRIDE_MAILSLOT_TO_INJECTOR);
            _tcscat(psz,pszPID);

            pMailSlotClt=new CMailSlotClient(psz);
            if (!pMailSlotClt->Open())
            {
                SetEvent(hevtError);
                _sntprintf(pszMsg,2*MAX_PATH,_T("Can't open MailSlot %s"),psz);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                
                FreeAll(FALSE);
                CleanCloseHandle(&hevtAPIOverrideDllProcessDetachCompleted);
                return FALSE;
            }

            // create internal events
            hevtUnload=CreateEvent(NULL,TRUE,FALSE,NULL);
            hevtFreeAPIInfo=CreateEvent(NULL,FALSE,FALSE,NULL);
            hevtAllAPIUnhookedDllFreeAll=CreateEvent(NULL,TRUE,FALSE,NULL);
            hevtWaitForUnlocker=CreateEvent(NULL,FALSE,FALSE,NULL);
            hevtEndOfProcessInternalCall=CreateEvent(NULL,FALSE,FALSE,NULL);

            // server to get slow commands from spying process
            _tcscpy(psz,APIOVERRIDE_MAILSLOT_FROM_INJECTOR);
            _tcscat(psz,pszPID);
            _tcscpy(GpszMailslotServerName,psz);

            // if not tls callback hook
            if (!hevtSingleThreadedMailSlotServerEndLoop)
            {
                // create mailslot server in a new thread
                if (!CreateApioverrideDllMailslotServer(GpszMailslotServerName))
                    return FALSE;
            }

            // set error report for CSupportedParameters
            CSupportedParameters::SetErrorReport(UserTypeParsingReportError,NULL);

            //////////////////////////
            // create link lists 
            //////////////////////////

            // create a link list for API informations
            pLinkListAPIInfos=new CLinkList(sizeof(API_INFO));
            // pLinkListAPIInfos->SetHeap(ApiOverrideHeap) is really required as API_INFO contains
            // opcode that can be executed, so the must be stored in a heap that as been created with 
            // HEAP_CREATE_ENABLE_EXECUTE. Other SetHeap for link list is to split a little more
            // original process heap and ApiOverride's data. (This is quite a mix as all new
            // are done in default process heap; but as previouly said, heap is really important only for
            // API_INFO data)
            pLinkListAPIInfos->SetHeap(ApiOverrideHeap);

            // create a link list for api monitoring file ID
            pLinkListpMonitoringFileInfos=new CLinkList(sizeof(MONITORING_FILE_INFOS));
            pLinkListpMonitoringFileInfos->SetHeap(ApiOverrideHeap);

            // create a link list for fake api dll file ID
            pLinkListpFakeDllInfos=new CLinkList(sizeof(FAKING_DLL_INFOS));
            pLinkListpFakeDllInfos->SetHeap(ApiOverrideHeap);

            // create a linked list for API info to be free
            pLinkListAPIInfosToBeFree=new CLinkList(sizeof(FREE_APIINFO));
            pLinkListAPIInfosToBeFree->SetHeap(ApiOverrideHeap);

            // create a linked list for loaded dll
            pLinkListLoadedDll=new CLinkList(sizeof(LOADED_DLL));
            pLinkListLoadedDll->SetHeap(ApiOverrideHeap);
            
            // create an CLogAPI object
            pModulesFilters=new CModulesFilters();

            // create com manager
            pComManager=new CCOM_Manager();
            // create event to signal end the full release of com created object for static hooks
            hevtCreatedCOMObjectsForStaticHooksReleased=CreateEvent(NULL,FALSE,FALSE,NULL);

            // create Net manager
            pNetManager=new CNET_Manager();

            // start Thread for looking Injector -> APIOverride.dll events and local unload event
            hThreadWatchingEvents=CreateThread(
                                    NULL,//LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                    0,// SIZE_T dwStackSize,
                                    ThreadWatchingEventsProc,//LPTHREAD_START_ROUTINE lpStartAddress,
                                    NULL,//LPVOID lpParameter,
                                    0,//DWORD dwCreationFlags,
                                    &APIOverrideThreads[0]//LPDWORD lpThreadId
                                    );
            hThreadFreeingHooks=CreateThread(
                                            NULL,//LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                            0,// SIZE_T dwStackSize,
                                            ThreadFreeingHooksProc,//LPTHREAD_START_ROUTINE lpStartAddress,
                                            (LPVOID)FALSE,//LPVOID lpParameter,
                                            0,//DWORD dwCreationFlags,
                                            NULL//LPDWORD lpThreadId
                                            );
            if ((!hThreadWatchingEvents)||(!hThreadFreeingHooks))
            {
                SetEvent(hevtError);

                _sntprintf(pszMsg,2*MAX_PATH,_T("Error creating watching thread for Process Id %s"),pszPID);
                DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

                
                FreeAll(FALSE);
                CleanCloseHandle(&hevtAPIOverrideDllProcessDetachCompleted);
                return FALSE;
            }
            CleanCloseHandle(&hThreadWatchingEvents);
            
            // hevtSingleThreadedMailSlotServerEndLoop event is created only for tls hooks
            if (hevtSingleThreadedMailSlotServerEndLoop)
            {
                bSuccessfulLoading=TRUE;
                SingleThreadedMailSlotServerCheck(GpszMailslotServerName);

                // create mailslot server in a new thread only now
                if (!CreateApioverrideDllMailslotServer(GpszMailslotServerName))
                    return FALSE;
            }
            else
            {
                // apioverride dll is fully loaded
                SetEvent(hevtAPIOverrideDllProcessAttachCompleted);

                bSuccessfulLoading=TRUE;
            }

            break;

        case DLL_PROCESS_DETACH: // not called if DLL_PROCESS_ATTACH return FALSE --> free memory in DLL_PROCESS_ATTACH before returning FALSE
            ApiOverrideDllDetaching=TRUE;
            if (!bSuccessfulLoading)
                break;

            if (hevtUnload)
                SetEvent(hevtUnload);
            FreeAll(FALSE);

            // set hevtAPIOverrideDllProcessDetachCompleted event before closing it
            SetEvent(hevtAPIOverrideDllProcessDetachCompleted);
            CleanCloseHandle(&hevtAPIOverrideDllProcessDetachCompleted);

            HeapDestroy(ApiOverrideHeap);
            HeapDestroy(ApiOverrideLogHeap);
            HeapDestroy(ApiOverrideKernelHeap);
            HeapDestroy(ApiOverrideLogApiTmpHeap);
            delete pLinkListAllThreadTlsData;
            break;
        case DLL_THREAD_ATTACH:
            // CreateTlsData(); // not required here avoid to create if not needed
            break;
            
        case DLL_THREAD_DETACH:
            DestroyTlsData();
            break;
    }

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: MailSlotSrvCallback
// Object: callback for commands with data
// Parameters :
//      in: PVOID pData : PSTRUCT_COMMAND
//          DWORD dwDataSize : pData size
//          PVOID pUserData : not used
// Return : 
//-----------------------------------------------------------------------------
void MailSlotSrvCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData)
{
    UNREFERENCED_PARAMETER(pUserData);
    if (dwDataSize<sizeof(PSTRUCT_COMMAND))
        return;
    PSTRUCT_COMMAND pCmd=(PSTRUCT_COMMAND)pData;

    if ((dwDataSize<sizeof(STRUCT_COMMAND))
        && (pCmd->dwCommand_ID!=CMD_PROCESS_INTERNAL_CALL_QUERY)
        && (pCmd->dwCommand_ID!=CMD_COM_HOOKING_OPTIONS)
        && (pCmd->dwCommand_ID!=CMD_NET_HOOKING_OPTIONS)
        && (pCmd->dwCommand_ID!=CMD_PLUGIN_REPLY_TO_OVERRIDING_DLL_QUERY)
		&& (pCmd->dwCommand_ID!=CMD_NET_HOOK_FROM_TOKEN)
		&& (pCmd->dwCommand_ID!=CMD_NET_UNHOOK_FROM_TOKEN)
        )
        return;
   
    switch(pCmd->dwCommand_ID)
    {
        case CMD_LOAD_MONITORING_FILE:
            if (LoadMonitoringFile(pCmd->pszStringParam))
                // signal file is loaded (no information returned about success or not)
                SetEvent(hevtMonitoringFileLoaded);
            else
                SetEvent(hevtError);
            break;
        case CMD_UNLOAD_MONITORING_FILE:
            if (UnloadMonitoringFile(pCmd->pszStringParam))
                // signal file is unloaded (no information returned about success or not)
                SetEvent(hevtMonitoringFileUnloaded);
            else
                SetEvent(hevtError);
            break;
        case CMD_LOAD_FAKE_API_DLL:
            if (LoadFakeApiDll(pCmd->pszStringParam))
                // signal dll is loaded (no information returned about success or not)
                SetEvent(hevtFakeAPIDLLLoaded);
            else
                SetEvent(hevtError);
            break;
        case CMD_UNLOAD_FAKE_API_DLL:
            if (UnloadFakeApiDll(pCmd->pszStringParam))
                // signal dll is unloaded (no information returned about success or not)
                SetEvent(hevtFakeAPIDLLUnloaded);
            else
                SetEvent(hevtError);
            break;

        case CMD_START_LOG_ONLY_BASE_MODULE:
            pModulesFilters->LogOnlyBaseModule(TRUE);
            break;
        case CMD_STOP_LOG_ONLY_BASE_MODULE:
            pModulesFilters->LogOnlyBaseModule(FALSE);
            break;
        case CMD_SET_LOGGED_MODULE_LIST_FILTERS_WAY:
            pModulesFilters->SetFilteringWay((tagFilteringWay)pCmd->Param[0]);
            break;
        case CMD_START_MODULE_LOGGING:
            pModulesFilters->SetModuleLogState(pCmd->pszStringParam,TRUE);
            break;
        case CMD_STOP_MODULE_LOGGING:
            pModulesFilters->SetModuleLogState(pCmd->pszStringParam,FALSE);
            break;
        case CMD_CLEAR_LOGGED_MODULE_LIST_FILTERS:
            pModulesFilters->ClearLoggedModulesListFilters();
            break;
        case CMD_ENABLE_MODULE_FILTERS_FOR_MONITORING:
            bFiltersApplyToMonitoring=TRUE;
            break;
        case CMD_DISABLE_MODULE_FILTERS_FOR_MONITORING:
            bFiltersApplyToMonitoring=FALSE;
            break;
        case CMD_ENABLE_MODULE_FILTERS_FOR_FAKING:
            bFiltersApplyToFaking=TRUE;
            break;
        case CMD_DISABLE_MODULE_FILTERS_FOR_FAKING:
            bFiltersApplyToFaking=FALSE;
            break;
        case CMD_NOT_LOGGED_MODULE_LIST_QUERY:
            if (!pModulesFilters->TransfertNotLoggedModuleList())
                SetEvent(hevtError);
            break;
        case CMD_DUMP:
            // show dump dialog into thread to avoid to lock mailslot dialog
            CloseHandle(CreateThread(NULL,0,ThreadDump,NULL,0,NULL));
            break;
        case CMD_BREAK_DONT_BREAK_APIOVERRIDE_THREADS:
            bBreakDialogDontBreakAPIOverrideThreads=pCmd->Param[0];
            break;
        case CMD_MONITORING_FILE_DEBUG_MODE:
            bDebugMonitoringFile=pCmd->Param[0];
            break;
        case CMD_CALLSTACK_RETRIEVAL:
            bLogCallStack=pCmd->Param[0];
            CallStackEbpRetrievalSize=pCmd->Param[1];
            break;
        case CMD_AUTOANALYSIS:
            FirstBytesAutoAnalysis=(tagFirstBytesAutoAnalysis)pCmd->Param[0];
            break;
        case CMD_COM_HOOKING_OPTIONS:
            if (dwDataSize<sizeof(HOOK_COM_OPTIONS)+sizeof(DWORD))
                return;
            if (pComManager)
                pComManager->SetOptions((HOOK_COM_OPTIONS*)(&((PBYTE)pData)[sizeof(DWORD)]));
            break;
        case CMD_COM_HOOKING_START_STOP:
            if (pCmd->Param[0])
            {
                if (pComManager)
                    pComManager->StartAutoHooking();
            }
            else
            {
                if (pComManager)
                    pComManager->StopAutoHooking();
            }
            break;
        case CMD_COM_INTERACTION:
            if (pComManager)
                pComManager->ShowCOMInteraction();
            break;
        case CMD_COM_RELEASE_CREATED_COM_OBJECTS_FOR_STATIC_HOOKS:// used internally only by apioverride dll
            if (pComManager)
                pComManager->ReleaseCreatedCOMObjectsForStaticHooks();
            SetEvent(hevtCreatedCOMObjectsForStaticHooksReleased);
            break;
        case CMD_COM_RELEASE_AUTO_HOOKED_OBJECTS:
            if (pComManager)
                pComManager->UnHookAllComObjects();
            break;
        case CMD_NET_INITIALIZE_HOOKNET_DLL:
            if (pNetManager)
                pNetManager->InitializeHookNetDll();
            break;
        case CMD_NET_SHUTDOWN_HOOKNET_DLL:
            if (pNetManager)
                pNetManager->ShutDownHookNetDll();
            break;
        case CMD_NET_HOOKING_START_STOP:
            if (pCmd->Param[0])
            {
                if (pNetManager)
                    pNetManager->StartAutoHooking();
            }
            else
            {
                if (pNetManager)
                    pNetManager->StopAutoHooking();
            }
            break;
        case CMD_NET_HOOKING_OPTIONS:
            if (dwDataSize<sizeof(HOOK_NET_OPTIONS)+sizeof(DWORD))
                return;
            if (pNetManager)
                pNetManager->SetOptions((HOOK_NET_OPTIONS*)(&((PBYTE)pData)[sizeof(DWORD)]));
            break;
        case CMD_NET_INTERACTION:
            if (pNetManager)
                pNetManager->ShowNetInteraction();
            break;
        case CMD_NET_RELEASE_HOOKED_METHODS:
            if (pNetManager)
                pNetManager->UnHookAllNetMethods();
            break;
        case CMD_PROCESS_INTERNAL_CALL_QUERY:// pData is not a PSTRUCT_COMMAND
            {
                // make a local copy of pData (it will be deleted in ThreadProcessInternalCallRequest)
                PBYTE p=new BYTE[dwDataSize-sizeof(DWORD)];
                HANDLE ThreadHandle;
                if (!p)
                    return;
                memcpy(p,&((PBYTE)pData)[sizeof(DWORD)],dwDataSize-sizeof(DWORD));
                ThreadHandle=CreateThread(NULL,0,ThreadProcessInternalCallRequest,p,0,NULL);
                // if thread creation success
                if (ThreadHandle)
                    // close thread handle
                    CloseHandle(ThreadHandle);
                else
                    // delete local buffer
                    delete[] p;
            }
            break;
        case CMD_CLEAR_USER_DATA_TYPE_CACHE:
            CSupportedParameters::ClearUserDataTypeCache();
            if (pComManager)
                pComManager->ClearUserDataTypeCache();
            if (pNetManager)
                pNetManager->ClearUserDataTypeCache();
            break;
        case CMD_PLUGIN_REPLY_TO_OVERRIDING_DLL_QUERY:
            PluginReplyToOverridingDllQuery(pData,dwDataSize);
            break;
		case CMD_NET_HOOK_FROM_TOKEN:
			{
				// check buffer size
				if (dwDataSize<sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM)+sizeof(DWORD))
					return;
				HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* Param;
				Param = (HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*) ((PBYTE)pCmd+sizeof(DWORD));

				if (dwDataSize<sizeof(DWORD) // command id
						    +sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM) // struct begin
							+ Param->TokensNumber*sizeof(ULONG32/*mdToken*/) // TokenArray
					)
					return;
				Param->TokenArray = (ULONG32*)(Param+sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM));
				// call function
				if (pNetManager)
					pNetManager->AddHookNetFromTokenForCompiledFuntions(Param);
				break;
			}
		case CMD_NET_UNHOOK_FROM_TOKEN:
			{
				// check buffer size
				if (dwDataSize<sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM)+sizeof(DWORD))
					return;
				HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* Param;
				Param = (HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*) ((PBYTE)pCmd+sizeof(DWORD));

				if (dwDataSize<sizeof(DWORD) // command id
						    +sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM) // struct begin
							+ Param->TokensNumber*sizeof(ULONG32/*mdToken*/) // TokenArray
					)
					return;
				Param->TokenArray = (ULONG32*)(Param+sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM));
				// call function
				if (pNetManager)
					pNetManager->RemoveHookNetFromTokenForCompiledFuntion(Param);
				break;
			}
    }
}

BOOL PluginReplyToOverridingDllQuery(PVOID pData,SIZE_T DataSize)
{
    // look at SendReplyToOverridingDllQuery in apioverride.cpp to get encoding
    PBYTE pbData = (PBYTE)pData;
    DWORD dw;
    SIZE_T UserDataSize;
    PBYTE UserData;
    PLUGIN_COMMUNICATION_MESSAGE_INFOS* pPluginCommunicationMessageInfos;

    // command id
    // message id
    // data size
    // data

    // trash command id
    pbData+=sizeof(DWORD);

    // message id
    if ((DataSize-(pbData - (PBYTE)pData))<sizeof(PVOID))
        return FALSE;
    memcpy(&pPluginCommunicationMessageInfos,pbData,sizeof(PVOID));
    pbData+=sizeof(PVOID);

    if (IsBadReadPtr(pPluginCommunicationMessageInfos,sizeof(PLUGIN_COMMUNICATION_MESSAGE_INFOS)))
        return FALSE;

    // data size
    if ((DataSize-(pbData - (PBYTE)pData))<sizeof(DWORD))
        return FALSE;
    memcpy(&dw,pbData,sizeof(DWORD));
    UserDataSize = dw;
    pbData+= sizeof(DWORD);

    // data
    if ((DataSize-(pbData - (PBYTE)pData))<UserDataSize)
        return FALSE;
    UserData = pbData;
    // pbData+=UserDataSize;

    pPluginCommunicationMessageInfos->DataSize = UserDataSize;
    if (pPluginCommunicationMessageInfos->DataSize==0)
        pPluginCommunicationMessageInfos->Data = NULL;
    else
    {
        pPluginCommunicationMessageInfos->Data = new BYTE[pPluginCommunicationMessageInfos->DataSize];        
        memcpy(pPluginCommunicationMessageInfos->Data,UserData,pPluginCommunicationMessageInfos->DataSize);
    }

    if(!SetEvent(pPluginCommunicationMessageInfos->hWaitingEvent))
    {
        // if set events fails, that mean event doesn't exists anymore and our allocated buffer will never be free
        // --> free buffer
        if (pPluginCommunicationMessageInfos->Data)
            delete pPluginCommunicationMessageInfos->Data;
        return FALSE;
    }
    return TRUE;
}

BOOL OverridingDllSendDataToPlugin(TCHAR* PluginName,PLUGIN_COMMUNICATION_MESSAGE_INFOS* pMessageInfos,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize)
{
    PBYTE Data;
    SIZE_T DataSize;
    DWORD dw;
    SIZE_T Index;
    SIZE_T PluginNameSize;

    if (DataToPluginSize == 0)
        return FALSE;

    if (IsBadReadPtr(DataToPlugin,DataToPluginSize))
        return FALSE;

    PluginNameSize = (_tcslen(PluginName)+1)*sizeof(TCHAR);
    DataSize= sizeof(DWORD) + sizeof(SIZE_T) + PluginNameSize + sizeof(PVOID) + sizeof(SIZE_T) + DataToPluginSize ;

    Data = new BYTE[DataSize];
    Index = 0;

    // command
    dw = CMD_OVERRIDING_DLL_QUERY_TO_PLUGIN;
    memcpy(&Data[Index],&dw,sizeof(DWORD));
    Index+=sizeof(DWORD);

    // plugin name size
    memcpy(&Data[Index],&PluginNameSize,sizeof(SIZE_T));
    Index+=sizeof(SIZE_T);

    // plugin name
    memcpy(&Data[Index],PluginName,PluginNameSize);
    Index+=PluginNameSize;

    // message id
    memcpy(&Data[Index],&pMessageInfos,sizeof(PVOID));
    Index+=sizeof(PVOID);

    // MsgSize
    memcpy(&Data[Index],&DataToPluginSize,sizeof(SIZE_T));
    Index+=sizeof(SIZE_T);

    // Msg
    memcpy(&Data[Index],DataToPlugin,DataToPluginSize);
    // Index+=DataToPluginSize;


    return pMailSlotClt->Write(Data,DataSize);
}


BOOL __stdcall OverridingDllSendDataToPlugin(TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize)
{
    return OverridingDllSendDataToPlugin(PluginName,NULL,DataToPlugin,DataToPluginSize);
}


// if not null DataFromPlugin must be free with "delete" call
BOOL __stdcall OverridingDllSendDataToPluginAndWaitReply(
    TCHAR* PluginName,
    IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,
    OUT PBYTE* pDataFromPlugin,OUT SIZE_T* pDataFromPluginSize,
    DWORD WaitTimeInMs /* INFINITE for infinite wait */)
{
    if (
           IsBadWritePtr(pDataFromPluginSize,sizeof(SIZE_T))
        || IsBadWritePtr(pDataFromPlugin,sizeof(PBYTE))
        )
        return FALSE;

    BOOL bSuccess;
    PLUGIN_COMMUNICATION_MESSAGE_INFOS MessageInfos;
    MessageInfos.hWaitingEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    bSuccess = FALSE;
    if (OverridingDllSendDataToPlugin(PluginName,&MessageInfos,DataToPlugin,DataToPluginSize))
    {
        bSuccess = (WaitForSingleObject(MessageInfos.hWaitingEvent,WaitTimeInMs)==WAIT_OBJECT_0);
        if (bSuccess)
        {
            *pDataFromPluginSize = MessageInfos.DataSize;
            *pDataFromPlugin = MessageInfos.Data;
        }
    }

    CloseHandle(MessageInfos.hWaitingEvent);

    return bSuccess;
}
// assume that data will be free in the same heap
void __stdcall OverridingDllSendDataToPluginAndWaitReplyFreeReceivedData(PBYTE pDataFromPluginToBeFree)
{
    delete pDataFromPluginToBeFree;
}
//-----------------------------------------------------------------------------
// Name: ThreadDump
// Object: non blocking dump dialog
// Parameters :
//      in: LPVOID lpParameter : not used
// Return : 
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadDump(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    CDumpUserInterface DumpUI;
    DumpUI.ShowDialog(NULL);
    return 0;
}
//-----------------------------------------------------------------------------
// Name: ThreadProcessInternalCallRequest
// Object: non blocking process internal call
// Parameters :
//      in: LPVOID lpParameter : pData buffer
// Return : 
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadProcessInternalCallRequest(LPVOID lpParameter)
{
    NumberOfCurrentRemoteCalls++;

    PBYTE pData=(PBYTE)lpParameter;

    // call func
    CProcessInternalCallRequest::ProcessInternalCallRequestFromMailSlot(pData,pMailSlotClt);
    // delete local allocated data
    delete[] pData;

    NumberOfCurrentRemoteCalls--;

    SetEvent(hevtEndOfProcessInternalCall);

    return 0;
}

//-----------------------------------------------------------------------------
// Name: ThreadWatchingEventsProc
// Object: thread for watching events (commands with no data)
// Parameters :
//      in: LPVOID lpParameter : not used
// Return : 0 on success -1 on error
//-----------------------------------------------------------------------------
DWORD WINAPI ThreadWatchingEventsProc(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);

    ///////////////////////////////////////
    // bInsideTlsCallback flag reset
    ///////////////////////////////////////

    // as soon as a thread is created, that means all tls callbacks have been called
    // so reset bInsideTlsCallback flag (notice this reset could be put in any thread startup)
    bInsideTlsCallback=FALSE;

    ///////////////////////////////////////
    // end of bInsideTlsCallback flag reset
    ///////////////////////////////////////


    DWORD dwRes;
    DWORD dwLastError;
    TCHAR pcError[MAX_PATH];

    HANDLE pHandles[6]={
                        hevtFreeProcess,
                        hevtStartMonitoring,
                        hevtStopMonitoring,
                        hevtStartFaking,
                        hevtStopFaking,
                        hevtUnload
                       };

    for(;;)
    {
        dwRes=WaitForMultipleObjects(6,pHandles,FALSE,INFINITE);
        switch(dwRes)
        {
        case WAIT_OBJECT_0://hevtFreeProcess
            FreeAll(TRUE);
            return 0;
        case WAIT_OBJECT_0+1://hevtStartMonitoring
            bMonitoring=TRUE;
            break;
        case WAIT_OBJECT_0+2://hevtStopMonitoring
            bMonitoring=FALSE;
            break;
        case WAIT_OBJECT_0+3://hevtStartFaking
            bFaking=TRUE;
            break;
        case WAIT_OBJECT_0+4://hevtStopFaking
            bFaking=FALSE;
            break;
        case WAIT_OBJECT_0+5://hevtUnload (quite useless as application close all dll thread at exit)
            return 0xFFFFFFFF;
        case WAIT_FAILED:
            dwLastError=GetLastError();
            dwRes=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL,
                                    dwLastError,
                                    GetUserDefaultLangID(),//GetSystemDefaultLangID()
                                    pcError,
                                    MAX_PATH-1,
                                    NULL);
    
            //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
            //  excluding the terminating null character.
            //If the function fails, the return value is zero
            if(dwRes!=0)
                DynamicMessageBoxInDefaultStation(NULL,pcError,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

            return WAIT_FAILED;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: FreeAll
// Object: free all allocated resources
// Parameters :
//      in: BOOL CleanWay : TRUE if no error and we got time (not dll unload due to appliction closing)
// Return : 
//-----------------------------------------------------------------------------
void FreeAll(BOOL CleanWay)
{
    if (bProcessFreeDone)
        return;

    bProcessFreeDone=TRUE;
    bMonitoring=FALSE;
    bFaking=FALSE;

    if (pComManager)
    {
        pComManager->StopAutoHooking();
        if (CleanWay)
        {
            // send a stupid message to our one mailslot to free COM created objects 
            // for static COM hooks, in the same thread they were allocated
            // Notice : if you monitor winapioverride for debug purpose, as bMonitoring has been 
            // put to FALSE, you wont see the log Release() with 0 result, but object will be reported as deleted
            CMailSlotClient* pTmpMailSlot=new CMailSlotClient(GpszMailslotServerName);
            STRUCT_COMMAND Cmd;
            Cmd.dwCommand_ID=CMD_COM_RELEASE_CREATED_COM_OBJECTS_FOR_STATIC_HOOKS;
            pTmpMailSlot->Open();
            pTmpMailSlot->Write(&Cmd,sizeof(STRUCT_COMMAND));
            pTmpMailSlot->Close();
            delete pTmpMailSlot;
            // wait 5 sec max for server to complete operation
            WaitForSingleObject(hevtCreatedCOMObjectsForStaticHooksReleased,5000);
        }
        CloseHandle(hevtCreatedCOMObjectsForStaticHooksReleased);
    }


    // close server mailslot (it's allow to unlock it's internally WaitForMultipleObjects)
    if (pMailSlotSrv)
    {
        pMailSlotSrv->Stop();
        delete pMailSlotSrv;
        pMailSlotSrv=NULL;
    }
    ResetEvent(hevtClientMailslotOpen);

    // speed up our thread
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);

    //////////////////////////
    // unhook all
    //////////////////////////
    UnhookAllAPIFunctions();

    // free ComManager object must be done before freeing pLinkListAPIInfos
    // only after UnhookAllAPIFunctions call
    if (pComManager)
    {
        delete pComManager;
        pComManager=NULL;
    }

    if (pNetManager)
    {
        delete pNetManager;
        pNetManager=NULL;
    }

    // ask freeing thread to stop
    SetEvent(hevtAllAPIUnhookedDllFreeAll);

    // wait for running threads to stop
    // we can't wait for hThreadWatchingEvents as it can be the caller of this func --> deadlock
    if (hThreadFreeingHooks)
    {
        WaitForSingleObject(hThreadFreeingHooks,INFINITE);
        CleanCloseHandle(&hThreadFreeingHooks);
    }

    // check if freeing thread as been closed gracefully (not the case with application unload)
    if (!FreeingThreadGracefullyClosed)
    {
        // unlock list if thread was kill during a lock
        pLinkListAllThreadTlsData->Unlock();
        pLinkListAPIInfosToBeFree->Unlock();
        // force freeing of hooks
        ThreadFreeingHooksProc((LPVOID)TRUE);
    }


    // wait until all process internal calls are ended
    while(NumberOfCurrentRemoteCalls>0)
    {
        if (WaitForSingleObject(hevtEndOfProcessInternalCall,UNHOOK_MAX_WAIT_TIME)!=WAIT_OBJECT_0)
        {
            if(DynamicMessageBoxInDefaultStation(NULL,
                                                 _T("Some process internal calls are not finished.\r\n")
                                                 _T("If you don't wait their end now, your process will crash when internal call will end.\r\n")
                                                 _T("Do you want to wait more ?\r\n"),
                                                 _T("Warning"),
                                                 MB_YESNO|MB_ICONWARNING|MB_TOPMOST
                                                 )==IDNO)
                break;
        }
    }
    CleanCloseHandle(&hevtEndOfProcessInternalCall);


    //////////////////////////
    // clear link lists must be after UnhookAllAPIFunctions
    //////////////////////////
    if (pLinkListAPIInfos)
    {
        delete pLinkListAPIInfos;
        pLinkListAPIInfos=0;
    }

    if (pLinkListpMonitoringFileInfos)
    {
        delete pLinkListpMonitoringFileInfos;
        pLinkListpMonitoringFileInfos=0;
    }

    if (pLinkListpFakeDllInfos)
    {
        delete pLinkListpFakeDllInfos;
        pLinkListpFakeDllInfos=0;
    }

    if (pLinkListAPIInfosToBeFree)
    {
        delete pLinkListAPIInfosToBeFree;
        pLinkListAPIInfosToBeFree=0;
    }

    if (pLinkListLoadedDll)
    {
        delete pLinkListLoadedDll;
        pLinkListLoadedDll = 0;
    }

    // Free MailSlot client
    if (pMailSlotClt)
    {
        delete pMailSlotClt;            
        pMailSlotClt=0;
    }

    // free CLogAPI object
    if (pModulesFilters)
    {
        delete pModulesFilters;
        pModulesFilters=0;
    }

    //////////////////////////
    // close events handles
    //////////////////////////

    //(Injector -> APIOverride.dll)
    CleanCloseHandle(&hevtStartMonitoring);
    CleanCloseHandle(&hevtStopMonitoring);
    CleanCloseHandle(&hevtStartFaking);
    CleanCloseHandle(&hevtStopFaking);
    CleanCloseHandle(&hevtFreeProcess);

    // (APIOverride.dll -> Injector)
    CleanCloseHandle(&hevtAPIOverrideDllProcessAttachCompleted);
    
    CleanCloseHandle(&hevtMonitoringFileLoaded);
    CleanCloseHandle(&hevtMonitoringFileUnloaded);
    CleanCloseHandle(&hevtFakeAPIDLLLoaded);
    CleanCloseHandle(&hevtFakeAPIDLLUnloaded);
    CleanCloseHandle(&hevtError);
    CleanCloseHandle(&hevtWaitForUnlocker);
    CleanCloseHandle(&hevtClientMailslotOpen);

    if (pSetUserWindowStation)
        delete pSetUserWindowStation;

    if (pSingleInstance)
        delete pSingleInstance; 

    CleanCloseHandle(&hevtUnload);

    TlsFree(ApiOverrideTlsIndex);
    TlsFree(ApiOverrideListCriticalPartTlsIndex);
    TlsFree(ApiOverrideSimpleHookCriticalCallTlsIndex);

    CleanCloseHandle(&hevtSimpleHookCriticalCallUnlockedEvent);

    // must be the last action: means that dll can be unloaded
    // as soon as remote app receive this event, dll is unloaded --> no more action can be done
    SetEvent(hevtProcessFree);
    CleanCloseHandle(&hevtProcessFree);

    CHookAvailabilityCheck::FreeStaticMembers();
}

//-----------------------------------------------------------------------------
// Name: GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
// Object: only to export pModulesFilters object method to HookCom and HookNet dlls
// Parameters : pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress parameters
// Return : return of pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
//-----------------------------------------------------------------------------
BOOL __stdcall GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE pOriginAddress,
                                                                        OUT HMODULE* pCallingModuleHandle,
                                                                        OUT TCHAR* pszModuleName,
                                                                        OUT PBYTE* pRelativeAddress,
                                                                        OUT BOOL* pbShouldLog,
                                                                        BOOL TakeAnotherSnapshotIfNotFound,
                                                                        BOOL ContinueEvenIfShouldNotBeLogged)
{
    return pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
                            (
                                pOriginAddress,
                                pCallingModuleHandle,
                                pszModuleName,
                                pRelativeAddress,
                                pbShouldLog,
                                TakeAnotherSnapshotIfNotFound,
                                ContinueEvenIfShouldNotBeLogged
                            );
}





/////////////////////////////////////////////////
// for thread creation testing
DWORD WINAPI DummyThread(LPVOID lParam)
{
    HANDLE hEvent = (HANDLE)lParam;
    ::SetEvent(hEvent);
    ::Sleep(1000);//let wait event to occur
    ::CloseHandle(hEvent);
    return 0;
}

// check if thread can be created and launched
// until ntloader hasn't finished it's job thread are not started,
// so to avoid deadlock sometimes, this function by returning TRUE assume that thread can be created and launched
BOOL bCanCreateThread = FALSE;
BOOL CanCreateThread()
{
    if (bInsideTlsCallback)
        return FALSE;
    // if creation has succeeded once, ntloader has finished it's job and all other thread creation will be successful
    // so once bCanCreateThread is TRUE, thread can be created
    if (bCanCreateThread)
        return TRUE;

    // try to create dummy thread
    HANDLE hEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL);
    ::CloseHandle(::CreateThread(NULL,0,DummyThread,hEvent,0,0));
    // let 1sec to thread to be created
    if (::WaitForSingleObject(hEvent,1000) == WAIT_TIMEOUT)
        return FALSE;

    // thread creation is successful
    bCanCreateThread = TRUE;  
    return TRUE;
}