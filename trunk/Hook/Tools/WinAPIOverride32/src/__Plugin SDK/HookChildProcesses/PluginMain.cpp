#include "../_Common_Files/IWinApiOverridePlugin.h"
#include "resource.h"
HINSTANCE DllhInstance;
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DllhInstance=hInstDLL;
        ::DisableThreadLibraryCalls(DllhInstance);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
HICON LoadIcon(UINT IdIcon)
{
    return (HICON)::LoadImage(DllhInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
}

class CPlugin:public IWinApiOverridePlugin
{
public:
    CPlugin();
    ~CPlugin();
    virtual void STDMETHODCALLTYPE Initialize(HWND hWndDialog,IWinApiOverride* pWinApiOverride);
    virtual void STDMETHODCALLTYPE Destroy();
    virtual BOOL STDMETHODCALLTYPE OnPluginMenuClick(UINT MenuItemID);// should return TRUE if MenuItemID belongs to your plugin menu
    virtual BOOL STDMETHODCALLTYPE OnOverridingDllQuery(IApiOverride* pApiOverride,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize);
    virtual void STDMETHODCALLTYPE OnProcessHooked(IApiOverride* pApiOverride);
    virtual void STDMETHODCALLTYPE OnProcessUnhooked(IApiOverride* pApiOverride);
    virtual void STDMETHODCALLTYPE OnProcessUnload(IApiOverride* pApiOverride);

    ////////////////////////////////////////////////
    // add your object methods and properties here
    ////////////////////////////////////////////////
    HWND hWndDialog;
    IWinApiOverride* pWinApiOverride;
    IWinApiOverrideMenu* pMenu;
    UINT MenuItem0Id;
    BOOL bIsEnabled;
    TCHAR AssociatedDataFolder[MAX_PATH];
    TCHAR AssociatedDllToInject[MAX_PATH];
    TCHAR AssociatedConfigFileName[MAX_PATH];
    static BOOL STDMETHODCALLTYPE OnEnableStateChangeStatic(IApiOverride* pApiOverride,PVOID UserParam);
};

CPlugin Plugin;
// the only required dll exported function
extern "C" __declspec(dllexport) IWinApiOverridePlugin* __stdcall GetPluginObject()
{
    return &Plugin;
}

#define HOOK_CHILD_PROCESSES_DATA _T("HookChildProcessesData\\")
#define HOOK_CHILD_PROCESSES_INJECTED_DLL _T("HookChildProcessesInjectedPart.dll")
#define HOOK_CHILD_PROCESSES_CONFIG_FILE _T("HookChildProcessesData.ini")
#define HOOK_CHILD_PROCESSES_SECTION_NAME_GENERAL   _T("General")
#define HOOK_CHILD_PROCESSES_KEY_NAME_ENABLE        _T("Enabled")

CPlugin::CPlugin()
{
    this->hWndDialog = NULL;
    this->pMenu = NULL;
    this->pWinApiOverride = NULL;
    this->MenuItem0Id = -1;
   
}
CPlugin::~CPlugin()
{

}

void STDMETHODCALLTYPE CPlugin::Initialize(HWND hWndDialog,IWinApiOverride* pWinApiOverride)
{
    // forge this->AssociatedDataFolder, AssociatedDllToInject and AssociatedConfigFileName
    TCHAR* psz;
    *this->AssociatedDataFolder = 0;
    ::GetModuleFileName(DllhInstance,this->AssociatedDataFolder,MAX_PATH);// can be done in constructor, as it may will be called before DllMain
    psz = _tcsrchr(this->AssociatedDataFolder,'\\');
    if (psz)
        // remove data after '\\'
        *(psz+1) = 0;

    _tcscat(this->AssociatedDataFolder,HOOK_CHILD_PROCESSES_DATA);

    // forge AssociatedDllToInject
    _tcscpy(this->AssociatedDllToInject,this->AssociatedDataFolder);
    _tcscat(this->AssociatedDllToInject,HOOK_CHILD_PROCESSES_INJECTED_DLL);
    
    // forge AssociatedConfigFileName
    _tcscpy(this->AssociatedConfigFileName,this->AssociatedDataFolder);
    _tcscat(this->AssociatedConfigFileName,HOOK_CHILD_PROCESSES_CONFIG_FILE);

    // load config data
    this->bIsEnabled = (BOOL) ::GetPrivateProfileInt(   HOOK_CHILD_PROCESSES_SECTION_NAME_GENERAL,
                                                        HOOK_CHILD_PROCESSES_KEY_NAME_ENABLE,
                                                        FALSE,
                                                        this->AssociatedConfigFileName
                                                        );


    IWinApiOverrideMenu* pPluginMenu;
    this->hWndDialog = hWndDialog;
    this->pWinApiOverride = pWinApiOverride;
    // make menu
    pPluginMenu = this->pWinApiOverride->GetPluginMenu();
    this->pMenu = this->pWinApiOverride->CreateMenu(pPluginMenu);
    pPluginMenu->AddSubMenuItem(_T("HookChildProcesses"),this->pMenu,LoadIcon(IDI_ICON1));
    this->MenuItem0Id = this->pMenu->AddMenuItem(_T("Enable"),0);

    // set enabled state
    this->pMenu->SetCheckedState(this->MenuItem0Id,this->bIsEnabled );
}
void STDMETHODCALLTYPE CPlugin::Destroy()
{
    this->bIsEnabled = this->pMenu->IsChecked(this->MenuItem0Id);

    // save options
    TCHAR pszValue[128];
    _itot(this->bIsEnabled,pszValue,10);
    ::WritePrivateProfileString(HOOK_CHILD_PROCESSES_SECTION_NAME_GENERAL,
                                HOOK_CHILD_PROCESSES_KEY_NAME_ENABLE,
                                pszValue,
                                this->AssociatedConfigFileName
                                );

    if (this->pMenu)
    {
        this->pWinApiOverride->DestroyMenu(this->pMenu);
        this->pMenu = NULL;
    }
}
BOOL STDMETHODCALLTYPE CPlugin::OnPluginMenuClick(UINT MenuItemID)
{
    if (MenuItemID==this->MenuItem0Id)
    {
        // invert checked state
        this->bIsEnabled = !this->pMenu->IsChecked(this->MenuItem0Id);
        this->pMenu->SetCheckedState(this->MenuItem0Id,this->bIsEnabled);
        
        // 1.1 changes : apply changes on current hooked processes
        this->pWinApiOverride->ForEachApiOverrideItem(CPlugin::OnEnableStateChangeStatic,this);

    	return TRUE;
    }
    return FALSE;
}

// 1.1 changes : apply changes on current hooked processes
BOOL STDMETHODCALLTYPE CPlugin::OnEnableStateChangeStatic(IApiOverride* pApiOverride,PVOID UserParam)
{
    CPlugin* pPlugin = (CPlugin*)UserParam;
    if (pPlugin->bIsEnabled)
        pApiOverride->LoadFakeAPI(pPlugin->AssociatedDllToInject);
    else
        pApiOverride->UnloadFakeAPI(pPlugin->AssociatedDllToInject);

    return TRUE;
}

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

BOOL STDMETHODCALLTYPE CPlugin::OnOverridingDllQuery(IApiOverride* pApiOverride,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize)
{
    HOOK_CHILD_PROCESSES_REPLY_MSG Reply ={0};
    HOOK_CHILD_PROCESSES_QUERY_MSG* pQuery =(HOOK_CHILD_PROCESSES_QUERY_MSG*)pMsg;
    if (MsgSize<sizeof(HOOK_CHILD_PROCESSES_QUERY_MSG))
    {
        pApiOverride->SendReplyToOverridingDllQuery(MessageId,(PBYTE)&Reply,sizeof(Reply));
        return FALSE;
    }

    // create an apioverride object for child process
    IApiOverride* pApiOverrideChildProcess = this->pWinApiOverride->CreateApiOverride();
    // get current options
    IWinApiOverrideOptions* pOptions = this->pWinApiOverride->GetOptions();

    if ( (pQuery->ProcessId /* !=0 */ ) && (pQuery->ThreadId /* !=0 */) )
    {
        // set option before start
        this->pWinApiOverride->SetBeforeStartOptions(pApiOverrideChildProcess,pOptions);
        // start hooking (let thread suspended to avoid to use callback for before resume actions)
        Reply.bSuccess = pApiOverrideChildProcess->StartAtProcessCreation(pQuery->ProcessId,pQuery->ThreadId,TRUE,NULL,NULL);

        // report hooking success
        this->pWinApiOverride->ReportHookedProcess(pApiOverrideChildProcess,pQuery->ProcessId,Reply.bSuccess);
    }
    else
    {
        Reply.bSuccess = FALSE;
        this->pWinApiOverride->ReportMessage(_T("CreateProcess returns bad PROCESS_INFORMATION Process or thread Id. Process can't be hooked"),MSG_ERROR);
    }


    if (Reply.bSuccess)
    {
        // set options after start
        this->pWinApiOverride->SetAfterStartOptions(pApiOverrideChildProcess,pOptions);
        // load parent monitoring files and faking dlls
        this->pWinApiOverride->LoadCurrentMonitoringFiles(pApiOverrideChildProcess);
        this->pWinApiOverride->LoadCurrentOverridingFiles(pApiOverrideChildProcess);

        // send reply to injected dll (unlock the blocking create process state)
        pApiOverride->SendReplyToOverridingDllQuery(MessageId,(PBYTE)&Reply,sizeof(Reply));

        // send event to other plugins
        this->pWinApiOverride->GetRaiseEventsInterface()->RaiseEventAfterProcessHooking(pApiOverrideChildProcess);

        if (!pQuery->bLetSuspended)
        {
            // get thread handle
            HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS,FALSE,pQuery->ThreadId);
            ::ResumeThread(hThread);
            CloseHandle(hThread);
        }
    }
    else
    {
        // send reply to injected dll (unlock the blocking create process state)
        pApiOverride->SendReplyToOverridingDllQuery(MessageId,(PBYTE)&Reply,sizeof(Reply));

        // destroy created object
        this->pWinApiOverride->DestroyApiOverride(pApiOverrideChildProcess);
    }

    return TRUE;
}
void STDMETHODCALLTYPE CPlugin::OnProcessHooked(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
    if (!this->bIsEnabled)
        return;

    pApiOverride->LoadFakeAPI(this->AssociatedDllToInject);
}
void STDMETHODCALLTYPE CPlugin::OnProcessUnhooked(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
}
void STDMETHODCALLTYPE CPlugin::OnProcessUnload(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
}