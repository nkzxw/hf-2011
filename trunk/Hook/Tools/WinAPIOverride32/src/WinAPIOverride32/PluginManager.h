#pragma once

#define CPluginManager_PLUGIN_DIRECTORY _T("Plugins\\")

#include "../Tools/LinkList/LinkList.h"
#include "IWinApiOverridePlugin.h"
#include "../Tools/Process/APIOverride/ApiOverride.h"

class CIWinApiOverrideImpement;
class CIWinApiOverrideRaiseEventsImplements;

class CPluginManager
{
    friend class CIWinApiOverrideRaiseEventsImplements;
private:
    CIWinApiOverrideImpement* pWinApiOverrideImplement;
    typedef struct 
    {
        HMODULE hModule;
        TCHAR Name[MAX_PATH];
        IWinApiOverridePlugin* pWinApiOverridePlugin;
    }PLUGIN_INFOS;
    CLinkList* pPluginsList;
    static BOOL FileFoundCallBack(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam);
    void ReportError(TCHAR* Msg);
    HWND hWndDialog;
    void OnProcessHooked(CApiOverride* pApiOverride,BOOL bLockPluginList);
    void OnProcessUnhooked(CApiOverride* pApiOverride,BOOL bLockPluginList);
public:
    CPluginManager(void);
    ~CPluginManager(void);
    void SetDialog(HWND hWndDialog);
    void LoadAndInitialize();
    void UninitializeAndUnload();

    // notifications
    void OnMenuClick(UINT MenuId);
    void OnProcessHooked(CApiOverride* pApiOverride);
    void OnProcessUnhooked(CApiOverride* pApiOverride);
    void OnProcessUnload(CApiOverride* pApiOverride);
    void OnOverridingDllQuery(CApiOverride* pApiOverride,TCHAR* PluginName,HANDLE MessageId,PBYTE pMsg,SIZE_T MsgSize);
};