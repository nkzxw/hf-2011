// include winsock2.h or #define _WINSOCKAPI_ before including windows.h
#include <winsock2.h>
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../../../Tools/Process/APIOverride/ApiOverride.h"

void STDMETHODCALLTYPE CallBackBeforeAppResume(DWORD dwProcessID,PVOID pUserParam);
void STDMETHODCALLTYPE CallbackMonitoringLog(LOG_ENTRY* pLog,PVOID pUserParam);
void STDMETHODCALLTYPE CallBackUnexpectedUnload(DWORD dwProcessID,PVOID pUserParam);

HWND hWndListView=NULL;
HANDLE hevtUnexpectedUnload=NULL;

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    STARTUPINFO StartupInfo={0};
    PROCESS_INFORMATION ProcessInfo={0};

    CApiOverride* pApiOverride;
    pApiOverride=new CApiOverride(hWndListView); // hWndListView : handle to the listview that will receive logs,
                                                 // put to NULL if you don't use a listview

    // you can optionally set a monitoring log call back
    pApiOverride->SetMonitoringCallback(CallbackMonitoringLog, // call back
                                        pApiOverride,          // call back parameter
                                        FALSE);                //we don't want to keep log in memory 
                                                               // after callback has been called

    // set an unload callback to now when the hooked process ends
    pApiOverride->SetUnexpectedUnloadCallBack(CallBackUnexpectedUnload, // call back
                                      pApiOverride              // call back parameter
                                      );

    // create event for unexpected unload
    hevtUnexpectedUnload=::CreateEvent(NULL,FALSE,FALSE,NULL);

    // start all
    if (!pApiOverride->Start(_T("c:\\windows\\notepad.exe"),// app path
                            NULL,                           // app command line
                            CallBackBeforeAppResume,        // call back before resuming application : trap to inject 
                                                            // monitoring files and faking dll before the application start.
                                                            // application will be resumed at the end of the callback
                            pApiOverride,                   // callback parameter (current ApiOverride object)
                            CApiOverride::StartWaySuspended,// start with suspended app
                            0,                              // useless parameter for StartWaySuspended start
                            &StartupInfo,                   // optional allow to get process infos
                            &ProcessInfo                    // optional allow to get process infos
                            ))
    {
        delete pApiOverride;
        ::CloseHandle(hevtUnexpectedUnload);
        if (ProcessInfo.hProcess)
            ::CloseHandle(ProcessInfo.hProcess);
        if (ProcessInfo.hThread)
            ::CloseHandle(ProcessInfo.hThread);
        return -1;
    }

    if (ProcessInfo.hThread)
        ::CloseHandle(ProcessInfo.hThread);

    SIZE_T NbObjectToWait = 1;
    HANDLE WaitingObjectHandles[2];
    WaitingObjectHandles[0] = hevtUnexpectedUnload;
    if (ProcessInfo.hProcess)
    {
        NbObjectToWait++;
        WaitingObjectHandles[1] = ProcessInfo.hProcess; // better if process crash without reporting hevtUnexpectedUnload event
    }

    // wait for the end of launched application
    ::WaitForMultipleObjects(NbObjectToWait,WaitingObjectHandles,FALSE,INFINITE);

    ::CloseHandle(hevtUnexpectedUnload);
    if (ProcessInfo.hProcess)
        ::CloseHandle(ProcessInfo.hProcess);

    delete pApiOverride;
    return 0;
}


void STDMETHODCALLTYPE CallBackBeforeAppResume(DWORD dwProcessID,PVOID pUserParam)
{

    CApiOverride* pApiOverride=(CApiOverride*)pUserParam;

    // change some options
    pApiOverride->SetAutoAnalysis(FIRST_BYTES_AUTO_ANALYSIS_SECURE); // hooking way    
    
    // set filters
    pApiOverride->SetFakingModuleFiltersState(TRUE); // enable filters for overriding
    pApiOverride->SetMonitoringModuleFiltersState(TRUE); // enable filters for monitoring
    pApiOverride->LogOnlyBaseModule(FALSE); // log all module not only the .exe
    pApiOverride->SetModuleFilteringWay(FILTERING_WAY_ONLY_SPECIFIED_MODULES); // log only specified modules
    pApiOverride->AddToFiltersModuleList(_T("c:\\winapioverride\\monitoring files\\HookedOnlyModuleList.txt")); // set filters by file containing multiple definitions
    pApiOverride->SetModuleLogState(_T("<TargetDir>\\*"),TRUE); // set module logging state directly
    pApiOverride->SetModuleLogState(_T("c:\\SomePath\\Somedll.dll"),TRUE); // set module logging state directly    
    
    // load a monitoring file
    pApiOverride->LoadMonitoringFile(_T("c:\\winapioverride\\monitoring files\\InputTextDataRetrival.txt"));

    // load an overriding dll (overriding or pre/post hook or COM object creation monitoring)
    pApiOverride->LoadFakeAPI(_T("c:\\winapioverride\\example\\messagebox and internal faking\\FakeMsgBox.dll"));

}

void STDMETHODCALLTYPE CallbackMonitoringLog(LOG_ENTRY* pLog,PVOID pUserParam)
{
    // get object on which event applies
    CApiOverride* pApiOverride=(CApiOverride*)pUserParam;

    // do whatever you want with the log (write some fields to file,....)
}

void STDMETHODCALLTYPE CallBackUnexpectedUnload(DWORD dwProcessID,PVOID pUserParam)
{
    // get object on which event applies
    CApiOverride* pApiOverride=(CApiOverride*)pUserParam;

    // do some of your freeing memory actions

    // signal end of process hooking
    if (hevtUnexpectedUnload)
        ::SetEvent(hevtUnexpectedUnload);
}