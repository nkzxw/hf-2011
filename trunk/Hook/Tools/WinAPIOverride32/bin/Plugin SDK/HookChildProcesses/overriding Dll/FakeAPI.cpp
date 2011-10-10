#include <windows.h>
#include "../../../__Overriding Dll SDK/_Common_Files/GenericFakeAPI.h"
extern APIOVERRIDE_EXPORTED_FUNCTIONS* pApiOverrideExportedFunctions;
// You just need to edit this file to add new fake api 
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

///////////////////////////////////////////////////////////////////////////////
// fake API prototype MUST HAVE THE SAME PARAMETERS 
// for the same calling convention see MSDN : 
// "Using a Microsoft modifier such as __cdecl on a data declaration is an outdated practice"
///////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI
mCreateProcessA(
               IN LPCSTR lpApplicationName,
               IN LPSTR lpCommandLine,
               IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
               IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
               IN BOOL bInheritHandles,
               IN DWORD dwCreationFlags,
               IN LPVOID lpEnvironment,
               IN LPCSTR lpCurrentDirectory,
               IN LPSTARTUPINFOA lpStartupInfo,
               OUT LPPROCESS_INFORMATION lpProcessInformation
               );

BOOL
WINAPI
mCreateProcessW(
               IN LPCWSTR lpApplicationName,
               IN LPWSTR lpCommandLine,
               IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
               IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
               IN BOOL bInheritHandles,
               IN DWORD dwCreationFlags,
               IN LPVOID lpEnvironment,
               IN LPCWSTR lpCurrentDirectory,
               IN LPSTARTUPINFOW lpStartupInfo,
               OUT LPPROCESS_INFORMATION lpProcessInformation
               );
///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning)
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T("Kernel32.dll"),_T("CreateProcessA"),(FARPROC)mCreateProcessA,StackSizeOf(LPCSTR)+StackSizeOf(LPSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(BOOL)+StackSizeOf(DWORD)+StackSizeOf(LPVOID) +StackSizeOf(LPCSTR)+StackSizeOf(LPSTARTUPINFOA)+StackSizeOf(LPPROCESS_INFORMATION),0},
    {_T("Kernel32.dll"),_T("CreateProcessW"),(FARPROC)mCreateProcessW,StackSizeOf(LPCWSTR)+StackSizeOf(LPWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(BOOL)+StackSizeOf(DWORD)+StackSizeOf(LPVOID) +StackSizeOf(LPCWSTR)+StackSizeOf(LPSTARTUPINFOW)+StackSizeOf(LPPROCESS_INFORMATION) ,0},
    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////// NEW API DEFINITION //////////////////////////////
/////////////////////// You don't need to export these functions //////////////
///////////////////////////////////////////////////////////////////////////////
typedef struct _HookChildProcessesQueryMsg
{
    DWORD ProcessId;
    DWORD ThreadId;
    BOOL bLetSuspended;
}HOOK_CHILD_PROCESSES_QUERY_MSG;
typedef struct _HookChildProcessesReplyMsg
{
    BOOL bSuccess;
}HOOK_CHILD_PROCESSES_REPLY_MSG;

BOOL
WINAPI
mCreateProcessA(
                IN LPCSTR lpApplicationName,
                IN LPSTR lpCommandLine,
                IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
                IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                IN BOOL bInheritHandles,
                IN DWORD dwCreationFlags,
                IN LPVOID lpEnvironment,
                IN LPCSTR lpCurrentDirectory,
                IN LPSTARTUPINFOA lpStartupInfo,
                OUT LPPROCESS_INFORMATION lpProcessInformation
                )
{
    BOOL bSuccess;
    PBYTE ReturnedData = NULL;
    SIZE_T ReturnedDataSize = 0;
    BOOL bLetSuspended = ((dwCreationFlags & CREATE_SUSPENDED)!=0);
    if (!::CreateProcessA(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation
                        )
        )
        // do nothing on process creation failure
        return FALSE;

    HOOK_CHILD_PROCESSES_QUERY_MSG Msg={0};
    Msg.ProcessId = lpProcessInformation->dwProcessId;
    Msg.ThreadId = lpProcessInformation->dwThreadId;
    Msg.bLetSuspended = bLetSuspended;
    bSuccess = pApiOverrideExportedFunctions->pOverridingDllSendDataToPluginAndWaitReply(
                                                                                        _T("HookChildProcesses.dll"),
                                                                                        (PBYTE)&Msg,
                                                                                        sizeof(Msg),
                                                                                        &ReturnedData,
                                                                                        &ReturnedDataSize,
                                                                                        INFINITE
                                                                                        );
    if (!bSuccess)
    {
        if (!bLetSuspended)
            ::ResumeThread(lpProcessInformation->hThread);
        return TRUE;// return the original CreateProcessValue
    }
    if (!ReturnedData)
        return TRUE;// return the original CreateProcessValue
    if (ReturnedDataSize<sizeof(HOOK_CHILD_PROCESSES_REPLY_MSG))
    {
        delete ReturnedData;
        return TRUE;// return the original CreateProcessValue
    }
    
    HOOK_CHILD_PROCESSES_REPLY_MSG* pReply;
    pReply = (HOOK_CHILD_PROCESSES_REPLY_MSG*)ReturnedData;
    bSuccess = pReply->bSuccess;
    pApiOverrideExportedFunctions->pOverridingDllSendDataToPluginAndWaitReplyFreeReceivedData(ReturnedData);
    return bSuccess;
}

BOOL
WINAPI
mCreateProcessW(
                IN LPCWSTR lpApplicationName,
                IN LPWSTR lpCommandLine,
                IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
                IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                IN BOOL bInheritHandles,
                IN DWORD dwCreationFlags,
                IN LPVOID lpEnvironment,
                IN LPCWSTR lpCurrentDirectory,
                IN LPSTARTUPINFOW lpStartupInfo,
                OUT LPPROCESS_INFORMATION lpProcessInformation
                )
{
    BOOL bSuccess;
    PBYTE ReturnedData = NULL;
    SIZE_T ReturnedDataSize = 0;
    BOOL bLetSuspended = ((dwCreationFlags & CREATE_SUSPENDED)!=0);
    if (!::CreateProcessW(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation
                        )
        )
        // do nothing on precess creation failure
        return FALSE;

    HOOK_CHILD_PROCESSES_QUERY_MSG Msg={0};
    Msg.ProcessId = lpProcessInformation->dwProcessId;
    Msg.ThreadId = lpProcessInformation->dwThreadId;
    Msg.bLetSuspended = bLetSuspended;
    bSuccess = pApiOverrideExportedFunctions->pOverridingDllSendDataToPluginAndWaitReply(
                                                                                        _T("HookChildProcesses.dll"),
                                                                                        (PBYTE)&Msg,
                                                                                        sizeof(Msg),
                                                                                        &ReturnedData,
                                                                                        &ReturnedDataSize,
                                                                                        INFINITE
                                                                                        );
    if (!bSuccess)
    {
        if (!bLetSuspended)
            ::ResumeThread(lpProcessInformation->hThread);
        return TRUE;// return the original CreateProcessValue
    }
    if (!ReturnedData)
        return TRUE;// return the original CreateProcessValue
    if (ReturnedDataSize<sizeof(HOOK_CHILD_PROCESSES_REPLY_MSG))
    {
        delete ReturnedData;
        return TRUE;// return the original CreateProcessValue
    }
    
    HOOK_CHILD_PROCESSES_REPLY_MSG* pReply;
    pReply = (HOOK_CHILD_PROCESSES_REPLY_MSG*)ReturnedData;
    bSuccess = pReply->bSuccess;
    pApiOverrideExportedFunctions->pOverridingDllSendDataToPluginAndWaitReplyFreeReceivedData(ReturnedData);
    return bSuccess;
}

