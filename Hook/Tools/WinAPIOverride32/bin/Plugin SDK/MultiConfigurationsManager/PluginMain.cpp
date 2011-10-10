#include <vector>
#include "../_Common_Files/IWinApiOverridePlugin.h"
#include "resource.h"
#include "Tools/StdFileOperations.h"
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

    ////////////////////////////////////////////////
    // add your object methods and properties here
    ////////////////////////////////////////////////
    typedef struct tagConfFileListContent
    {
        UINT MenuId;
        TCHAR Name[MAX_PATH];
    }CONF_FILE_LIST_CONTENT;

    HWND hWndDialog;
    IWinApiOverride* pWinApiOverride;
    IWinApiOverrideMenu* pMenu;
    IWinApiOverrideMenu* pLoadMenu;
    std::vector<CONF_FILE_LIST_CONTENT> ConfFileList;
    UINT MenuItem0Id;
    SIZE_T NbFilesInConfFileList;
    TCHAR AssociatedDataFolder[MAX_PATH];
    TCHAR AssociatedConfigFileName[MAX_PATH];

    void AddConfigFileToMenu(TCHAR* ConfFileName);
};

CPlugin Plugin;
// the only required dll exported function
extern "C" __declspec(dllexport) IWinApiOverridePlugin* __stdcall GetPluginObject()
{
    return &Plugin;
}

#define MULTI_CONFIG_MANAGER_DATA _T("MultiConfigManagerData\\")
#define MULTI_CONFIG_MANAGER_CONFIG_FILE _T("MultiConfigManagerData.ini")
#define MULTI_CONFIG_MANAGER_SECTION_NAME_LOAD      _T("Load")
#define MULTI_CONFIG_MANAGER_KEY_NAME_NB_FILES      _T("NbFiles")
#define MULTI_CONFIG_MANAGER_ITEM_PREFIX            _T("Item")

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
void CPlugin::AddConfigFileToMenu(TCHAR* ConfFileName)
{
    if (!ConfFileName)// bad pointer
        return;
    if (!*ConfFileName)// empty string
        return;
    if (!CStdFileOperations::DoesFileExists(ConfFileName))// check if file exists
        return;

    // for each load menu, assume file is not already inserted
    for (SIZE_T Cnt=0;Cnt<this->ConfFileList.size();Cnt++)
    {
        // if menu id match
        if (_tcsicmp(this->ConfFileList[Cnt].Name,ConfFileName)==0)
            return;
    }

    
    CONF_FILE_LIST_CONTENT Content;
    _tcscpy(Content.Name,ConfFileName);
    // add item to menu
    Content.MenuId = this->pLoadMenu->AddMenuItem(ConfFileName,0);
    // add item struct infos to vector
    this->ConfFileList.push_back(Content);
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
  
    _tcscat(this->AssociatedDataFolder,MULTI_CONFIG_MANAGER_DATA);

    // forge AssociatedConfigFileName
    _tcscpy(this->AssociatedConfigFileName,this->AssociatedDataFolder);
    _tcscat(this->AssociatedConfigFileName,MULTI_CONFIG_MANAGER_CONFIG_FILE);

    // load config data
    this->NbFilesInConfFileList = (BOOL) ::GetPrivateProfileInt(MULTI_CONFIG_MANAGER_SECTION_NAME_LOAD,
                                                                MULTI_CONFIG_MANAGER_KEY_NAME_NB_FILES,
                                                                FALSE,
                                                                this->AssociatedConfigFileName
                                                                );


    IWinApiOverrideMenu* pPluginMenu;
    this->hWndDialog = hWndDialog;
    this->pWinApiOverride = pWinApiOverride;
    // make menu
    pPluginMenu = this->pWinApiOverride->GetPluginMenu();
    this->pMenu = this->pWinApiOverride->CreateMenu(pPluginMenu);
    pPluginMenu->AddSubMenuItem(_T("MultiConfigManager"),this->pMenu,LoadIcon(IDI_ICON1));
    this->MenuItem0Id = this->pMenu->AddMenuItem(_T("Save Current Config"),LoadIcon(IDI_ICON3));
    this->pLoadMenu = this->pWinApiOverride->CreateMenu(this->pMenu);
    this->pMenu->AddSubMenuItem(_T("Load Config"),this->pLoadMenu,LoadIcon(IDI_ICON2));

    // for each recent file
    TCHAR KeyName[256];
    TCHAR FileName[MAX_PATH];
    for (SIZE_T Cnt=0;Cnt<this->NbFilesInConfFileList;Cnt++)
    {
        // add it to menu
        _stprintf(KeyName,MULTI_CONFIG_MANAGER_ITEM_PREFIX _T("%u"),Cnt);
        ::GetPrivateProfileString(MULTI_CONFIG_MANAGER_SECTION_NAME_LOAD,
                                    KeyName,
                                    _T(""),
                                    FileName,
                                    MAX_PATH,
                                    this->AssociatedConfigFileName
                                    );

        this->AddConfigFileToMenu(FileName);
    }
}
void STDMETHODCALLTYPE CPlugin::Destroy()
{
    this->NbFilesInConfFileList = this->pMenu->IsChecked(this->MenuItem0Id);
    
    // assume directory exists
    CStdFileOperations::CreateDirectory(this->AssociatedDataFolder);

    // save options
    TCHAR pszValue[128];
    this->NbFilesInConfFileList = this->ConfFileList.size();
    _itot((int)this->NbFilesInConfFileList,pszValue,10);
    ::WritePrivateProfileString(MULTI_CONFIG_MANAGER_SECTION_NAME_LOAD,
                                MULTI_CONFIG_MANAGER_KEY_NAME_NB_FILES,
                                pszValue,
                                this->AssociatedConfigFileName
                                );

    TCHAR KeyName[256];
    for (SIZE_T Cnt=0;Cnt<this->NbFilesInConfFileList;Cnt++)
    {
        _stprintf(KeyName,MULTI_CONFIG_MANAGER_ITEM_PREFIX _T("%u"),Cnt);
        ::WritePrivateProfileString(MULTI_CONFIG_MANAGER_SECTION_NAME_LOAD,
                                    KeyName,
                                    this->ConfFileList[Cnt].Name,
                                    this->AssociatedConfigFileName
                                    );
    }

    if (this->pLoadMenu)
    {
        this->pWinApiOverride->DestroyMenu(this->pLoadMenu);
        this->pLoadMenu = NULL;
    }

    if (this->pMenu)
    {
        this->pWinApiOverride->DestroyMenu(this->pMenu);
        this->pMenu = NULL;
    }

}
BOOL STDMETHODCALLTYPE CPlugin::OnPluginMenuClick(UINT MenuItemID)
{
    IWinApiOverrideOptions* pOptions = this->pWinApiOverride->GetOptions();

    if (MenuItemID==this->MenuItem0Id)
    {
        // assume directory exists
        CStdFileOperations::CreateDirectory(this->AssociatedDataFolder);

        // save
        TCHAR FileName[MAX_PATH]=_T("");

        // open dialog
        OPENFILENAME ofn;
        memset(&ofn,0,sizeof (OPENFILENAME));
        ofn.lStructSize=sizeof (OPENFILENAME);
        ofn.hwndOwner=this->hWndDialog;
        ofn.hInstance=NULL;
        ofn.lpstrFilter=_T("ini\0*.ini\0All Files\0*.*\0");
        ofn.nFilterIndex = 1;
        ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt=_T("ini");
        ofn.lpstrFile=FileName;
        ofn.nMaxFile=MAX_PATH;
        ofn.lpstrInitialDir = this->AssociatedDataFolder;

        if (!GetSaveFileName(&ofn))
            return TRUE;

        if (pOptions->Save(FileName))
        {
            this->AddConfigFileToMenu(FileName);
        }
    	return TRUE;
    }

    // for each load menu
    for (SIZE_T Cnt=0;Cnt<this->ConfFileList.size();Cnt++)
    {
        // if menu id match
        if (this->ConfFileList[Cnt].MenuId == MenuItemID)
        {
            // load specified file
            if (!pOptions->Load(this->ConfFileList[Cnt].Name))
            {
                // on load failure
                ::MessageBox(this->hWndDialog,_T("Error loading file"),_T("Multi Config Manager"),MB_OK | MB_ICONERROR);
                // suppose that file doesn't exists anymore or is not valid --> remove it, from menu and from list
                this->pLoadMenu->Remove(this->ConfFileList[Cnt].MenuId);
                this->ConfFileList.erase(this->ConfFileList.begin()+Cnt);
            }
            else
            {
                this->pWinApiOverride->UpdateGUIFromOptions();
            }
            return TRUE;
        }
    }
    return FALSE;
}