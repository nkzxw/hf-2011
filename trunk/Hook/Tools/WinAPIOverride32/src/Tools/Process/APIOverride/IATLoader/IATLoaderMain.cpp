#define IAT_LOADER_EMPTY_FOR_DEBUG 0

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "..\..\MailSlot\MailSlotClient.h"
#include "..\..\..\File\StdFileOperations.h"
#include "..\..\..\Gui\DynamicMessageBox\DynamicMessageBox.h"
#include <Windows.h>
#define APIOVERRIDE_DLL_NAME _T("APIOverride.dll")
#define CURRENT_DLL_NAME _T("IAT Loader")

void DisplayErrorAndExitProcess(TCHAR* Message)
{
    if (DynamicMessageBox(NULL,Message,_T("Error"),MB_OK | MB_ICONERROR | MB_SERVICE_NOTIFICATION)==-1)
    {
        ::OutputDebugString(Message);
    }
    ::ExitProcess(0);
}

BOOL WINAPI DllMain(
                    HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID lpvReserved
                    )
{
    UNREFERENCED_PARAMETER(lpvReserved);

#if (IAT_LOADER_EMPTY_FOR_DEBUG == 1)
    // debug mode : allow to get normal process execution only with a modified PE header
    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(fdwReason);
#else
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // do whatever we want : currently process is not started, 
        // nt loader is doing is job, so no thread can be launched

        // disable thread library call
        ::DisableThreadLibraryCalls(hinstDLL);

        {
            TCHAR MailSlotName[MAX_PATH];
            TCHAR EventName[MAX_PATH];
            TCHAR ApplicationName[MAX_PATH];
            HANDLE hEvent;
            CStdFileOperations::GetAppName(ApplicationName,MAX_PATH);
            _tcscpy(MailSlotName,_T("\\\\.\\mailslot\\WAO_"));
            _tcscat(MailSlotName,CStdFileOperations::GetFileName(ApplicationName));

            DWORD ProcessId = ::GetCurrentProcessId();
            
            CMailSlotClient MailSlot(MailSlotName);

            if (!MailSlot.Open())
            {
                DisplayErrorAndExitProcess(CURRENT_DLL_NAME _T(" : Error opening shared mailslot"));
                return FALSE;
            }
            if (!MailSlot.Write(&ProcessId,sizeof(ProcessId)))
            {
                DisplayErrorAndExitProcess(CURRENT_DLL_NAME _T(" : Error writting in shared mailslot"));
                return FALSE;
            }
            MailSlot.Close();

            // let time to winapioverride.exe to open all required mailslots and events
            _tcscpy(EventName,_T("EVENT_"));
            _tcscat(EventName,CStdFileOperations::GetFileName(ApplicationName));
            hEvent = ::OpenEvent(EVENT_ALL_ACCESS,FALSE,EventName);
            if (!hEvent)
            {
                DisplayErrorAndExitProcess(CURRENT_DLL_NAME _T(" : Error opening shared event"));
                return FALSE;
            }
            if (::WaitForSingleObject(hEvent,10000)!=WAIT_OBJECT_0)
            {
                ::CloseHandle(hEvent);
                DisplayErrorAndExitProcess(CURRENT_DLL_NAME _T(" : unlocking event not received"));
                return FALSE;
            }
            ::CloseHandle(hEvent);

            // here we just need to load the apioverride dll (we have added winapioverride path to current environement var
            if (!::LoadLibrary(APIOVERRIDE_DLL_NAME))
            {
                DisplayErrorAndExitProcess(CURRENT_DLL_NAME _T(" : error loading ") APIOVERRIDE_DLL_NAME);
                return FALSE;
            }
        }
        break;
    //case DLL_THREAD_ATTACH:
    //    break;
    //case DLL_THREAD_DETACH:
    //    break;
    //case DLL_PROCESS_DETACH:
    //    break;
    }

#endif
    return TRUE;

}

// to be loaded by nt loader with import section, dll MUST export at least one function
// so we just export an empty function which will be never called
extern "C" __declspec(dllexport) 
BOOL __stdcall Initialize()
{
    // empty function, only for being loaded by ntloader
    return TRUE;
}