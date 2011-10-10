#include "StubResolver.h"

CStubResolverGUI::CStubResolverGUI()
{
    this->pDllStub = new CDllStub();
    this->pListView = NULL;
}
CStubResolverGUI::~CStubResolverGUI()
{
    delete this->pDllStub;
}

void CStubResolverGUI::OnInit()
{
    this->pListView = new CListview(this->GetDlgItem(IDC_LIST_STUB_INFOS));
    this->pListView->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListView->AddColumn(_T("Importing Dll"),200,LVCFMT_LEFT);
    this->pListView->AddColumn(_T("Really Used Dll"),150,LVCFMT_LEFT);

    this->EnableDragAndDrop(TRUE);
}
void CStubResolverGUI::OnClose()
{
    delete this->pListView;
    this->pListView = NULL;
}

void CStubResolverGUI::OnResolve()
{
    TCHAR StubDllName[MAX_PATH];
    TCHAR* Array[2];
    TCHAR* ShortStubDllName;

    this->pListView->Clear();

    this->GetDlgItemText(IDC_EDIT_DLL_NAME,StubDllName,MAX_PATH);
    

    ShortStubDllName = CStdFileOperations::GetFileName(StubDllName);
    CStdFileOperations::RemoveFileExt(ShortStubDllName);

    CDllStub::API_SET_MODULE_ENTRY_EX* pApiSetModuleEntryEx;
    pApiSetModuleEntryEx = this->pDllStub->GetStubDllInfos(ShortStubDllName);
    if (!pApiSetModuleEntryEx)
        return;

    CLinkListItem* pItem;
    CDllStub::HOST_MODULE_ENTRY_EX* pHostModuleEntryEx;

    // search through module entry list 
    for (pItem = pApiSetModuleEntryEx->pHostModuleEntry->Head; pItem; pItem=pItem->NextItem)
    {
        pHostModuleEntryEx = (CDllStub::HOST_MODULE_ENTRY_EX*) pItem->ItemData;
        
        if (*pHostModuleEntryEx->ImportingModuleName)
            Array[0] = pHostModuleEntryEx->ImportingModuleName;
        else
            Array[0] = _T("Default");
        Array[1] = pHostModuleEntryEx->HostModuleName;

        this->pListView->AddItemAndSubItems(2,Array);
    }
}

void CStubResolverGUI::OnCommand(WPARAM wParam,LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_BUTTON_BROWSE_DLL:
        this->OnBrowse(IDC_EDIT_DLL_NAME, _T("Dll\0*.dll\0All\0*.*\0"));
        break;
    case IDOK:
        this->OnResolve();
        break;
    case IDC_BUTTON_QUIT:
        this->Close();
        break;
    }
}
void CStubResolverGUI::OnNotify(WPARAM wParam,LPARAM lParam)
{
    if (this->pListView)
    {
        if (this->pListView->OnNotify(wParam,lParam))
            return;
    }
}

void CStubResolverGUI::OnDropFiles(WPARAM wParam,LPARAM lParam)
{
    HDROP hDrop= (HDROP)wParam;
    TCHAR pszFileName[MAX_PATH];
    ::DragQueryFile(hDrop, 0,pszFileName, MAX_PATH);
    this->SetDlgItemText(IDC_EDIT_DLL_NAME,pszFileName);
    ::DragFinish(hDrop);
}

void CStubResolverGUI::OnBrowse(int EditId,TCHAR* Filter)
{
    TCHAR FileName[MAX_PATH];
    *FileName=0;
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->GetControlHandle();
    ofn.hInstance=this->GetInstance();
    ofn.lpstrFilter= Filter;
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=FileName;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrTitle=_T("Select File");

    // get file name
    if (::GetOpenFileName(&ofn))
    {
        this->SetDlgItemText(EditId,FileName);
    }
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    CStubResolverGUI RegExtractorGUI;
    RegExtractorGUI.Show(hInstance,0,IDD_DIALOG_STUB_RESOLVER,IDI_ICON_APP);
}