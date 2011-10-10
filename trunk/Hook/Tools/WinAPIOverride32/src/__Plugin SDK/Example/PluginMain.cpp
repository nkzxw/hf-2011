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
    IWinApiOverrideMenu* pSubMenu1;
    UINT MenuItem0Id;
    UINT SubMenuItem0Id;
    UINT SubMenuItem1Id;
};

CPlugin Plugin;
// the only required dll exported function
extern "C" __declspec(dllexport) IWinApiOverridePlugin* __stdcall GetPluginObject()
{
    return &Plugin;
}

CPlugin::CPlugin()
{
    this->hWndDialog = NULL;
    this->pMenu = NULL;
    this->pSubMenu1 = NULL;
    this->pWinApiOverride = NULL;
    this->MenuItem0Id = -1;
    this->SubMenuItem0Id = -1;
    this->SubMenuItem1Id = -1;
}
CPlugin::~CPlugin()
{

}

void STDMETHODCALLTYPE CPlugin::Initialize(HWND hWndDialog,IWinApiOverride* pWinApiOverride)
{
    IWinApiOverrideMenu* pPluginMenu;
    this->hWndDialog = hWndDialog;
    this->pWinApiOverride = pWinApiOverride;
    pPluginMenu = this->pWinApiOverride->GetPluginMenu();
    this->pMenu = this->pWinApiOverride->CreateMenu(pPluginMenu);
    this->pSubMenu1 = this->pWinApiOverride->CreateMenu(this->pMenu);

    pPluginMenu->AddSubMenuItem(_T("Example"),this->pMenu,LoadIcon(IDI_ICON1));
    this->MenuItem0Id = this->pMenu->AddMenuItem(_T("MenuItem0"),LoadIcon(IDI_ICON2));
    this->pMenu->AddSeparatorItem();
    this->pMenu->AddSubMenuItem(_T("SubMenu"),this->pSubMenu1,LoadIcon(IDI_ICON3));
    this->SubMenuItem0Id = this->pSubMenu1->AddMenuItem(_T("SubMenuItem0"),LoadIcon(IDI_ICON4));
    this->SubMenuItem1Id = this->pSubMenu1->AddMenuItem(_T("SubMenuItem1"),NULL);
}
void STDMETHODCALLTYPE CPlugin::Destroy()
{
    if (this->pMenu)
    {
        this->pWinApiOverride->DestroyMenu(this->pMenu);
        this->pMenu = NULL;
    }
    if (this->pSubMenu1)
    {
        this->pWinApiOverride->DestroyMenu(this->pSubMenu1);
        this->pSubMenu1 = NULL;
    }
}
BOOL STDMETHODCALLTYPE CPlugin::OnPluginMenuClick(UINT MenuItemID)
{
    if (MenuItemID==this->MenuItem0Id)
    {
        MessageBox(this->hWndDialog,_T("MenuItem0 clicked"),_T("Ex Plugin"),MB_OK|MB_ICONINFORMATION);
    	return TRUE;
    }
    else if (MenuItemID==this->SubMenuItem0Id)
    {
        MessageBox(this->hWndDialog,_T("SubMenuItem0 clicked"),_T("Ex Plugin"),MB_OK|MB_ICONINFORMATION);
        return TRUE;
    }
    else if (MenuItemID==this->SubMenuItem1Id)
    {
        MessageBox(this->hWndDialog,_T("SubMenuItem1 clicked"),_T("Ex Plugin"),MB_OK|MB_ICONINFORMATION);
        return TRUE;
    }
    return FALSE;
}
BOOL STDMETHODCALLTYPE CPlugin::OnOverridingDllQuery(IApiOverride* pApiOverride,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize)
{
    // pApiOverride : item from which request comes
    // MessageId : NULL if no reply is required
    UNREFERENCED_PARAMETER(pApiOverride);
    UNREFERENCED_PARAMETER(MessageId);
    UNREFERENCED_PARAMETER(pMsg);
    UNREFERENCED_PARAMETER(MsgSize);
    // pApiOverride->SendReplyToOverridingDllQuery(MessageId,pMsg,MsgSize);
    return FALSE;
}
void STDMETHODCALLTYPE CPlugin::OnProcessHooked(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
    this->pWinApiOverride->ReportMessage(_T("My Plugin OnProcessHooked"),MSG_INFORMATION);
}
void STDMETHODCALLTYPE CPlugin::OnProcessUnhooked(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
    this->pWinApiOverride->ReportMessage(_T("My Plugin OnProcessUnhooked"),MSG_INFORMATION);
}
void STDMETHODCALLTYPE CPlugin::OnProcessUnload(IApiOverride* pApiOverride)
{
    UNREFERENCED_PARAMETER(pApiOverride);
    this->pWinApiOverride->ReportMessage(_T("My Plugin OnProcessUnload"),MSG_INFORMATION);
}