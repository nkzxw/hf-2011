#include "PluginManager.h"


#include "IWinApiOverride.h"
#include "IWinApiOverrideOptions.h"
#include "IWinApiOverrideLogs.h"
#include "IWinApiOverrideMenu.h"
#include "WinApiOverride.h"
#include "../Tools/gui/Menu/PopUpMenu.h"
#include "../Tools/File/StdFileOperations.h"
#include "../Tools/File/FileSearch.h"

extern CPopUpMenu* pPluginsPopUpMenu;
extern CLinkListSimple* pApiOverrideList;
extern CLinkList* pLogList;

class CIWinApiOverrideRaiseEventsImplements:public IWinApiOverrideRaiseEvents
{
private:
    CPluginManager* pAssociatedPluginManager;
public:
    CIWinApiOverrideRaiseEventsImplements(CPluginManager* pAssociatedPluginManager)
    {
        this->pAssociatedPluginManager = pAssociatedPluginManager;
    }
    ~CIWinApiOverrideRaiseEventsImplements()
    {

    }
    virtual void STDMETHODCALLTYPE RaiseEventAfterProcessHooking(IApiOverride* pApiOverride)
    {
        // as plugin list can be locked, assume to avoid dead lock
        this->pAssociatedPluginManager->OnProcessHooked((CApiOverride*)pApiOverride,!this->pAssociatedPluginManager->pPluginsList->IsLocked());
    }
    virtual void STDMETHODCALLTYPE RaiseEventBeforeProcessUnhooking(IApiOverride* pApiOverride)
    {
        // as plugin list can be locked, assume to avoid dead lock
        this->pAssociatedPluginManager->OnProcessUnhooked((CApiOverride*)pApiOverride,!this->pAssociatedPluginManager->pPluginsList->IsLocked());
    }
};

class CIWinApiOverrideLogsImplement:public IWinApiOverrideLogs
{
    virtual void STDMETHODCALLTYPE Clear(BOOL bWaitEndOfClearing)
    {
        ::ClearLogs(bWaitEndOfClearing);
    }
    virtual BOOL STDMETHODCALLTYPE Load(TCHAR* LogName,BOOL bWaitEndOfLoading)
    {
        return ::LoadLogs(LogName,bWaitEndOfLoading);
    }
    virtual BOOL STDMETHODCALLTYPE Save(TCHAR* LogName,BOOL bWaitEndOfSaving)
    {
        return ::SaveLogs(LogName,bWaitEndOfSaving);
    }
    virtual BOOL STDMETHODCALLTYPE DisplayDetailsForLog(LOG_LIST_ENTRY* pLogEntry)
    {
        return ::UpdateDetailListView(pLogEntry);
    }
    virtual void STDMETHODCALLTYPE ForEach(pfForEachLogListEntryCallBack CallBack,PVOID UserParam)
    {
        CLinkListItem* pItem;
        if (::IsBadCodePtr((FARPROC)CallBack))
            return;

        pLogList->Lock();
        for (pItem = pLogList->Head ; pItem ; pItem = pItem->NextItem)
        {
            if (!CallBack( (LOG_LIST_ENTRY*)pItem->ItemData , UserParam ))
                break;
        }
        pLogList->Unlock();
    }
};
class CIWinApiOverrideMenuImpement:public IWinApiOverrideMenu
{
public:
    CPopUpMenu* pPopUpMenu;
    BOOL bPopupMenuCreated;
    CIWinApiOverrideMenuImpement(CIWinApiOverrideMenuImpement* pParentMenu)
    {
        this->pPopUpMenu = new CPopUpMenu(pParentMenu->pPopUpMenu);
        this->bPopupMenuCreated = TRUE;
    }
    CIWinApiOverrideMenuImpement(CPopUpMenu* pParentMenu)
    {
        this->pPopUpMenu = new CPopUpMenu(pParentMenu);
        this->bPopupMenuCreated = TRUE;
    }
    CIWinApiOverrideMenuImpement()
    {
        this->pPopUpMenu = NULL;
        this->bPopupMenuCreated = FALSE;
    }
    ~CIWinApiOverrideMenuImpement()
    {
        if (this->bPopupMenuCreated)
            delete this->pPopUpMenu;
    }
    virtual int STDMETHODCALLTYPE GetItemCount()
    {
        return this->pPopUpMenu->GetItemCount();
    }
    virtual UINT STDMETHODCALLTYPE AddMenuItem(TCHAR* Name,HICON hIcon)
    {
        return this->pPopUpMenu->Add(Name,hIcon);
    }
    virtual UINT STDMETHODCALLTYPE AddMenuItem(TCHAR* Name,HICON hIcon,UINT Index)
    {
        return this->pPopUpMenu->Add(Name,hIcon,Index);
    }
    virtual UINT STDMETHODCALLTYPE AddSubMenuItem(TCHAR* SubMenuName,IWinApiOverrideMenu* SubMenu,HICON hIcon)
    {
        if (!SubMenu)
            return (UINT)(-1);
        return this->pPopUpMenu->AddSubMenu(SubMenuName,((CIWinApiOverrideMenuImpement*)SubMenu)->pPopUpMenu,hIcon);
    }
    virtual UINT STDMETHODCALLTYPE AddSubMenuItem(TCHAR* SubMenuName,IWinApiOverrideMenu* SubMenu,HICON hIcon,UINT Index)
    {
        if (!SubMenu)
            return (UINT)(-1);
        return this->pPopUpMenu->AddSubMenu(SubMenuName,((CIWinApiOverrideMenuImpement*)SubMenu)->pPopUpMenu,hIcon,Index);
    }
    virtual UINT STDMETHODCALLTYPE AddSeparatorItem()
    {
        return this->pPopUpMenu->AddSeparator();
    }
    virtual UINT STDMETHODCALLTYPE AddSeparatorItem(UINT Index)
    {
        return this->pPopUpMenu->AddSeparator(Index);
    }

    virtual void STDMETHODCALLTYPE SetCheckedState(UINT MenuID,BOOL bChecked)
    {
        return this->pPopUpMenu->SetCheckedState(MenuID,bChecked);
    }
    virtual BOOL STDMETHODCALLTYPE IsChecked(UINT MenuID)
    {
        return this->pPopUpMenu->IsChecked(MenuID);
    }
    virtual void STDMETHODCALLTYPE SetEnabledState(UINT MenuID,BOOL bEnabled)
    {
        return this->pPopUpMenu->SetEnabledState(MenuID,bEnabled);
    }
    virtual BOOL STDMETHODCALLTYPE IsEnabled(UINT MenuID)
    {
        return this->pPopUpMenu->IsEnabled(MenuID);
    }
    virtual BOOL STDMETHODCALLTYPE SetText(UINT MenuID,TCHAR* pszText)
    {
        return this->pPopUpMenu->SetText(MenuID,pszText);
    }
    virtual BOOL STDMETHODCALLTYPE SetIcon(UINT MenuID,HICON hIcon)
    {
        return this->pPopUpMenu->SetIcon(MenuID,hIcon);
    }
    virtual void STDMETHODCALLTYPE Remove(UINT MenuID)
    {
        return this->pPopUpMenu->Remove(MenuID);
    }
    void SetPopUpMenu(CPopUpMenu* pPopUpMenu)
    {
        this->pPopUpMenu = pPopUpMenu;
    }
};

class CIWinApiOverrideImpement:public IWinApiOverride
{
protected:
    CIWinApiOverrideMenuImpement* pWinApiOverridePluginsPopUpMenu;
    CIWinApiOverrideLogsImplement* pLogs;
    CIWinApiOverrideRaiseEventsImplements* pRaiseEventsInterface;
    // currently IUnknown not supported
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject)
    {
        UNREFERENCED_PARAMETER(riid);
        if (!ppvObject)
            return S_FALSE;
        *ppvObject=NULL;
        return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return 1;
    }
    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
        return 1;
    }

    HWND hWndDialog;
public:
    CIWinApiOverrideImpement(CPluginManager* pAssociatedPluginManager)
    {
        this->pLogs = new CIWinApiOverrideLogsImplement();
        this->pWinApiOverridePluginsPopUpMenu = new CIWinApiOverrideMenuImpement();
        this->pWinApiOverridePluginsPopUpMenu->SetPopUpMenu(pPluginsPopUpMenu);
        this->hWndDialog = NULL;
        this->pRaiseEventsInterface = new CIWinApiOverrideRaiseEventsImplements(pAssociatedPluginManager);
    }
    ~CIWinApiOverrideImpement()
    {
        delete this->pWinApiOverridePluginsPopUpMenu;
        delete this->pLogs;
        delete this->pRaiseEventsInterface;
    }

    virtual void STDMETHODCALLTYPE ReportMessage(TCHAR* pszMsg, tagMsgTypes MsgType)
    {
        ::ReportMessage(pszMsg,MsgType);
    }
    virtual void ReportHookedProcess(IApiOverride* pApiOverride,DWORD ProcessId,BOOL bSuccess)
    {
        ::ReportHookedProcess((CApiOverride*)pApiOverride,ProcessId,bSuccess);
    }
    virtual void ReportUnhookedProcess(IApiOverride* pApiOverride,BOOL bSuccess)
    {
        ::ReportUnhookedProcess((CApiOverride*)pApiOverride,bSuccess);
    }

    virtual IWinApiOverrideLogs* STDMETHODCALLTYPE GetLogsInterface()
    {
        return this->pLogs;
    }

    // options
    virtual IWinApiOverrideOptions* STDMETHODCALLTYPE CreateOptions()
    {
        return new COptions();
    }
    virtual void STDMETHODCALLTYPE DestroyOptions(IWinApiOverrideOptions* pOptions)
    {
        delete ((COptions*)pOptions);
    }
    virtual IWinApiOverrideOptions* STDMETHODCALLTYPE GetOptions()// options for all hooked processes Options
    {
        return ::GetOptions();
    }
    virtual BOOL STDMETHODCALLTYPE UpdateGUIFromOptions()// update gui from options
    {
        return ::SetGUIFromOptions();
    }

    // Enumerates all ApiOverride Items
    virtual void STDMETHODCALLTYPE ForEachApiOverrideItem(pfForEachApiOverrideItemCallBack CallBack,PVOID UserParam)
    {
        if (::IsBadCodePtr((FARPROC)CallBack))
            return;
        CLinkListItem* pItem;
        pApiOverrideList->Lock();
        for (pItem = pApiOverrideList->Head ; pItem ; pItem = pItem->NextItem)
        {
            if (!CallBack((IApiOverride*)pItem->ItemData,UserParam))
                break;
        }
        pApiOverrideList->Unlock();
    }

    virtual IApiOverride* STDMETHODCALLTYPE CreateApiOverride()
    {
        return new CApiOverride(this->hWndDialog);
    }
    virtual void STDMETHODCALLTYPE DestroyApiOverride(IApiOverride* pApiOverride)
    {
        // remove from list and destroy
        ::DestroyApiOverride((CApiOverride*)pApiOverride);
    }

    // MUST BE CALLED FOR A WINAPIOVERRIDE INTEGRATION
    // call order : 
    //      1) SetBeforeStartOptions
    //      2) pApiOverride->Start
    //      3) SetAfterStartOptions [MUST BE CALLED IN CallBackBeforeAppResume for START_WAY_LAUNCH_APPLICATION way]
    virtual BOOL STDMETHODCALLTYPE SetBeforeStartOptions(IApiOverride* pApiOverride,IWinApiOverrideOptions* pOptions)
    {
        return ::SetBeforeStartOptions((CApiOverride*)pApiOverride,(COptions*)pOptions);
    }
    virtual BOOL STDMETHODCALLTYPE SetAfterStartOptions(IApiOverride* pApiOverride,IWinApiOverrideOptions* pOptions)
    {
        return ::SetAfterStartOptions((CApiOverride*)pApiOverride,(COptions*)pOptions);
    }

    // MUST BE CALLED FOR A WINAPIOVERRIDE INTEGRATION instead of pApiOverride->Stop
    virtual BOOL STDMETHODCALLTYPE StopApiOverride(IApiOverride* pApiOverride)
    {
        return ::StopApiOverride((CApiOverride*)pApiOverride);
    }

    // the following functions applies for all apioverride objects and update GUI
    virtual BOOL STDMETHODCALLTYPE StartStop(BOOL Start)
    {
        return ::StartStop(Start);
    }
    virtual BOOL STDMETHODCALLTYPE LoadMonitoringFile(TCHAR* pszFile)
    {
        return ::LoadMonitoringFile(pszFile);
    }
    virtual BOOL STDMETHODCALLTYPE UnloadMonitoringFile(TCHAR* pszFile)
    {
        return ::UnloadMonitoringFileAndRemoveFromGui(pszFile);
    }
    virtual BOOL STDMETHODCALLTYPE LoadOverridingFile(TCHAR* pszFile)
    {
        return ::LoadOverridingFile(pszFile);
    }
    virtual BOOL STDMETHODCALLTYPE StartStopMonitoring(BOOL Start)
    {
        return ::StartStopMonitoring(Start);
    }
    virtual BOOL STDMETHODCALLTYPE UnloadOverridingFile(TCHAR* pszFile)
    {
        return ::UnloadOverridingFileAndRemoveFromGui(pszFile);
    }
    virtual BOOL STDMETHODCALLTYPE StartStopFaking(BOOL Start)
    {
        return ::StartStopFaking(Start);
    }
    virtual BOOL STDMETHODCALLTYPE LogOnlyBaseModule(BOOL OnlyBaseModule)
    {
        return ::LogOnlyBaseModule(OnlyBaseModule);
    }
    virtual BOOL STDMETHODCALLTYPE StartStopCOMHooking(BOOL Start)
    {
        return ::StartStopCOMHooking(Start);
    }
    virtual BOOL STDMETHODCALLTYPE StartStopNETHooking(BOOL Start)
    {
        return ::StartStopNETHooking(Start);
    }

    virtual BOOL STDMETHODCALLTYPE LoadCurrentMonitoringFiles(IApiOverride* pApiOverride)
    {
        return ::LoadCurrentMonitoringFiles((CApiOverride*)pApiOverride);
    }
    virtual BOOL STDMETHODCALLTYPE LoadCurrentOverridingFiles(IApiOverride* pApiOverride)
    {
        return ::LoadCurrentOverridingFiles((CApiOverride*)pApiOverride);
    }

    // allow plug in to have menu and sub menu entries in the plug in menu
    virtual IWinApiOverrideMenu* STDMETHODCALLTYPE GetPluginMenu()
    {
        return (IWinApiOverrideMenu*)this->pWinApiOverridePluginsPopUpMenu;
    }
    virtual IWinApiOverrideMenu* STDMETHODCALLTYPE CreateMenu(IWinApiOverrideMenu* pParentMenu)// NULL for Plug in Menu
    {
        if (pParentMenu)
        {
            return new CIWinApiOverrideMenuImpement((CIWinApiOverrideMenuImpement*)pParentMenu);
        }
        else
        {
            return new CIWinApiOverrideMenuImpement(pPluginsPopUpMenu);
        }
    }
    virtual void STDMETHODCALLTYPE DestroyMenu(IWinApiOverrideMenu* pMenu)
    {
        delete (CIWinApiOverrideMenuImpement*) pMenu;
    }

    virtual void STDMETHODCALLTYPE Close()// query close operation. Returns before Winapioverride Stops (use your plugin "Destroy" method to get a more close event)
    {
        ::Exit(0);
    }

    // for auto updaters or check for update at startup
    virtual BOOL STDMETHODCALLTYPE CheckForUpdate(BOOL* pbNewVersionIsAvailable,WCHAR** ppDownloadLink)
    {
        return ::CheckForUpdate(pbNewVersionIsAvailable,ppDownloadLink);
    }

    virtual IWinApiOverrideRaiseEvents* STDMETHODCALLTYPE GetRaiseEventsInterface()
    {
        return this->pRaiseEventsInterface;
    }

    void SetDialog(HWND hWndDialog)
    {
        this->hWndDialog = hWndDialog;
    }
};

CPluginManager::CPluginManager(void)
{
    this->pWinApiOverrideImplement = NULL;
    this->pPluginsList=new CLinkList(sizeof(PLUGIN_INFOS));
    this->hWndDialog = NULL;
}

CPluginManager::~CPluginManager(void)
{
    this->UninitializeAndUnload();
    delete this->pPluginsList;
}
void CPluginManager::ReportError(TCHAR* Msg)
{
    ::MessageBox(this->hWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR);
}
void CPluginManager::SetDialog(HWND hWndDialog)
{
    this->hWndDialog = hWndDialog;
}
void CPluginManager::LoadAndInitialize()
{
    if (this->pWinApiOverrideImplement)
        delete this->pWinApiOverrideImplement;
    this->pWinApiOverrideImplement = new CIWinApiOverrideImpement(this); // must be created in LoadAndInitialize as it uses global var for its constructor
    TCHAR PluginPath[MAX_PATH];
    CStdFileOperations::GetAppPath(PluginPath,MAX_PATH);
    _tcscat(PluginPath,CPluginManager_PLUGIN_DIRECTORY);
    _tcscat(PluginPath,_T("*.dll"));

    CFileSearch::Search(PluginPath,FALSE,CPluginManager::FileFoundCallBack,(PVOID)this);
}

// returns TRUE to continue parsing
BOOL CPluginManager::FileFoundCallBack(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(Directory);
    TCHAR FilePath[MAX_PATH];
    PLUGIN_INFOS Plugin;
    CPluginManager* pPluginManager = (CPluginManager*)UserParam;
    _tcscpy(FilePath,Directory);
    _tcscat(FilePath,_T("\\"));
    _tcscat(FilePath,pWin32FindData->cFileName);
    // load module
    Plugin.hModule = ::LoadLibrary(FilePath);
    if (!Plugin.hModule)
        return TRUE; // continue parsing
    _tcsncpy(Plugin.Name,pWin32FindData->cFileName,MAX_PATH);
    Plugin.Name[MAX_PATH-1]=0;
    // get required proc address
    pfGetPluginObject GetPluginObject = (pfGetPluginObject)::GetProcAddress(Plugin.hModule,IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAME);
    if (!GetPluginObject)
    {
        TCHAR Msg[2*MAX_PATH];
        _stprintf(Msg,_T("Function %s not exported by %s"),IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAMET,pWin32FindData->cFileName);
        pPluginManager->ReportError(Msg);
        return TRUE; // continue parsing
    }
    // get the IWinApiOverridePlugin interface of the plugin
    Plugin.pWinApiOverridePlugin = GetPluginObject();
    if (::IsBadReadPtr(Plugin.pWinApiOverridePlugin,sizeof(IWinApiOverridePlugin)))
    {
        TCHAR Msg[2*MAX_PATH];
        _stprintf(Msg,_T("Invalid object returned by GetPluginObject in %s"),pWin32FindData->cFileName);
        pPluginManager->ReportError(Msg);
        return TRUE; // continue parsing
    }

    // check unicode / ascii compatibility
    IWinApiOverridePlugin::ENCODING_TYPE PluginEncodingType = Plugin.pWinApiOverridePlugin->GetPluginEncoding();
    BOOL bBadEncoding;
    TCHAR* pNeededEncodingType;
    TCHAR* pInvertEncodingType;
#if (defined(UNICODE)||defined(_UNICODE))
    bBadEncoding = (PluginEncodingType != IWinApiOverridePlugin::ENCODING_TYPE_UNICODE);
    pNeededEncodingType = _T("UNICODE");
    pInvertEncodingType = _T("ASCII");
#else
    bBadEncoding = (PluginEncodingType != IWinApiOverridePlugin::ENCODING_TYPE_ASCII);
    pNeededEncodingType = _T("ASCII");
    pInvertEncodingType = _T("UNICODE");
#endif
    if (bBadEncoding)
    {
        TCHAR Msg[3*MAX_PATH];
        _stprintf(  Msg,
                    _T("Plugin %s bad encoding, you must rebuild it using %s charcter set, or use the %s version of WinApiOverride"),
                    pWin32FindData->cFileName,
                    pNeededEncodingType,
                    pInvertEncodingType
                    );
        ::ReportMessage(Msg,MSG_ERROR);
        return TRUE;// continue parsing
    }

    // check plugin framework version 
    if (Plugin.pWinApiOverridePlugin->GetPluginFrameworkVersion() != ((IWinApiOverridePlugin_MAJOR_VERSION<<16) | (IWinApiOverridePlugin_MINOR_VERSION)) )
    {
        TCHAR Msg[3*MAX_PATH];
        _stprintf(  Msg,
                    _T("Plugin %s is incompatible with current WinApiOverride version, you must rebuild it using new provided plugin SDK"),
                    pWin32FindData->cFileName
                    );
        ::ReportMessage(Msg,MSG_ERROR);
        return TRUE;// continue parsing
    }

    // call IWinApiOverridePlugin::Initialize of returned object
    Plugin.pWinApiOverridePlugin->Initialize(pPluginManager->hWndDialog,pPluginManager->pWinApiOverrideImplement);
    pPluginManager->pPluginsList->AddItem(&Plugin);
    return TRUE;// continue parsing
}

void CPluginManager::UninitializeAndUnload()
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        pPlugin->pWinApiOverridePlugin->Destroy();
        ::FreeLibrary(pPlugin->hModule);
    }
    this->pPluginsList->RemoveAllItems(TRUE);
    this->pPluginsList->Unlock();

    delete this->pWinApiOverrideImplement;
    this->pWinApiOverrideImplement = NULL;
}

// notifications
void CPluginManager::OnMenuClick(UINT MenuId)
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        if (pPlugin->pWinApiOverridePlugin->OnPluginMenuClick(MenuId))
            break;
    }
    this->pPluginsList->Unlock();
}
void CPluginManager::OnProcessHooked(CApiOverride* pApiOverride)
{
    return this->OnProcessHooked(pApiOverride,TRUE);
}
void CPluginManager::OnProcessHooked(CApiOverride* pApiOverride,BOOL bLockPluginList)
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    if (bLockPluginList)
        this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        pPlugin->pWinApiOverridePlugin->OnProcessHooked(pApiOverride);
    }
    if (bLockPluginList)
        this->pPluginsList->Unlock();
}
void CPluginManager::OnProcessUnhooked(CApiOverride* pApiOverride)
{
    return this->OnProcessUnhooked(pApiOverride,TRUE);
}
void CPluginManager::OnProcessUnhooked(CApiOverride* pApiOverride,BOOL bLockPluginList)
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    if (bLockPluginList)
        this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        pPlugin->pWinApiOverridePlugin->OnProcessUnhooked(pApiOverride);
    }
    if (bLockPluginList)
        this->pPluginsList->Unlock();
}

void CPluginManager::OnProcessUnload(CApiOverride* pApiOverride)
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        pPlugin->pWinApiOverridePlugin->OnProcessUnload(pApiOverride);
    }
    this->pPluginsList->Unlock();
}

void CPluginManager::OnOverridingDllQuery(CApiOverride* pApiOverride,TCHAR* PluginName,HANDLE MessageId,PBYTE pMsg,SIZE_T MsgSize)
{
    CLinkListItem* pItem;
    PLUGIN_INFOS* pPlugin;
    this->pPluginsList->Lock();
    for (pItem = this->pPluginsList->Head;pItem;pItem=pItem->NextItem)
    {
        pPlugin =(PLUGIN_INFOS*) pItem->ItemData;
        if (_tcsicmp(PluginName,pPlugin->Name)==0)
        {
            pPlugin->pWinApiOverridePlugin->OnOverridingDllQuery(pApiOverride,MessageId,pMsg,MsgSize);
            break;
        }
    }
    this->pPluginsList->Unlock();
}