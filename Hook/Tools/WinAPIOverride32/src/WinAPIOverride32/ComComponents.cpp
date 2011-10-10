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

//-----------------------------------------------------------------------------
// Object: manages the COM components dialog
//-----------------------------------------------------------------------------

#include "comcomponents.h"

//////////////////////////
// registry keys
//////////////////////////
#define CComComponents_KEY_CLSID _T("CLSID\\")
#ifdef _WIN64
    #define CComComponents_KEY_INPROCSERVER _T("InprocServer64")
    #define CComComponents_KEY_LOCALSERVER _T("LocalServer64")

#else
    #define CComComponents_KEY_INPROCSERVER _T("InprocServer32")
    #define CComComponents_KEY_LOCALSERVER _T("LocalServer32")
#endif
#define CComComponents_KEY_PROG_ID _T("ProgID")
#define CComComponents_KEY_VERSIONINDEPENDENTPROGID _T("VersionIndependentProgID")
#define CComComponents_KEY_THREADINGMODEL _T("ThreadingModel")

//////////////////////////
// columns definitions
//////////////////////////
#define CComComponents_NB_COLUMNS 6
enum tagCComComponents_LISTVIEW_COLUMN_INDEX
{
    CComComponents_CLSID_COLUMN_INDEX=0,
    CComComponents_NAME_COLUMN_INDEX,
    CComComponents_PROGID_COLUMN_INDEX,
    CComComponents_VERSIONINDEPENDENTPROGID_COLUMN_INDEX,
    CComComponents_THREADING_MODEL_COLUMN_INDEX,
    CComComponents_MODULE_NAME_COLUMN_INDEX
    
};
CListview::COLUMN_INFO CComComponentsListViewColumnInfo[CComComponents_NB_COLUMNS]=
{
    {_T("CLSID"),240,LVCFMT_LEFT},
    {_T("Name"),200,LVCFMT_LEFT},
    {_T("Prog ID"),150,LVCFMT_LEFT},
    {_T("Prog ID (Version Independent)"),180,LVCFMT_LEFT},
    {_T("Threading Model"),100,LVCFMT_LEFT},
    {_T("Module Name"),300,LVCFMT_LEFT}
};

CComComponents::CComComponents(void)
{
    this->pListView=NULL;
    *this->szSearchedString=0;
    this->hParsingFinished=NULL;
    this->bStopParsing=FALSE;
    this->bClosing=FALSE;
}

CComComponents::~CComComponents(void)
{
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : CComComponents* current object
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComComponents::ModelessDialogThread(PVOID lParam)
{
    CComComponents* pComComponents=(CComComponents*)lParam;
    // don't use hParentHandle to allow to put main window to an upper Z-order
    // show dialog
    DialogBoxParam(pComComponents->hInstance,(LPCTSTR)IDD_DIALOG_COM_COMPONENTS,NULL,(DLGPROC)CComComponents::WndProc,(LPARAM)pComComponents);
    delete pComComponents;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    CComComponents* pComComponents=new CComComponents();
    pComComponents->hInstance=hInstance;
    pComComponents->hParentHandle=hWndDialog;

    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CComComponents::ModelessDialogThread,pComComponents,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::Init()
{
    // create a listview object associated to listview
    this->pListView=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_COM_COMPONENTS));
    this->pListView->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListView->InitListViewColumns(CComComponents_NB_COLUMNS,CComComponentsListViewColumnInfo);

    // check find options
    CheckDlgButton(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_CLSID,BST_CHECKED);
    CheckDlgButton(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_CLASS_NAME,BST_CHECKED);
    CheckDlgButton(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_PROG_ID,BST_CHECKED);

    // render layout
    this->OnSize();

    // fill COM Components listview
    this->hParsingFinished=CreateEvent(NULL,FALSE,FALSE,NULL);
    CloseHandle(CreateThread(NULL,0,FillCOMList,this,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::Close()
{
    if (this->bClosing)
        return;

    this->bClosing=TRUE;

    // as we need to do a blocking call, avoid to block message pump (can deadlock the window)
    CloseHandle(CreateThread(NULL,0,WaitingClose,this,0,NULL));

}
DWORD WINAPI CComComponents::WaitingClose(LPVOID lpParam)
{
    CComComponents* pComComponents=(CComComponents*)lpParam;

    // query to stop parsing
    pComComponents->bStopParsing=TRUE;
    // wait for the end of parsing
    WaitForSingleObject(pComComponents->hParsingFinished,INFINITE);
    CloseHandle(pComComponents->hParsingFinished);

    delete pComComponents->pListView;
    pComComponents->pListView=NULL;
    EndDialog(pComComponents->hWndDialog,0);

    return 0;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CComComponents::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CComComponents* pComComponents=(CComComponents*)lParam;
            pComComponents->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pComComponents);

            pComComponents->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_COM);
        }
        break;
    case WM_CLOSE:
        {
            CComComponents* pComComponents=((CComComponents*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pComComponents)
                break;
            pComComponents->Close();
        }
        break;
    case WM_SIZING:
        {
            CComComponents* pComComponents=((CComComponents*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pComComponents)
                break;
            pComComponents->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CComComponents* pComComponents=((CComComponents*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pComComponents)
                break;
            pComComponents->OnSize();
        }
        break;
    case WM_COMMAND:
        {
            CComComponents* pComComponents=((CComComponents*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pComComponents)
                break;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_COM_COMPONENTS_FIND:
                pComComponents->Find();
                break;
            case IDC_BUTTON_COM_COMPONENTS_FIND_NEXT:
                pComComponents->FindNext(pComComponents->pListView->GetSelectedIndex()+1);
                break;
            case IDC_BUTTON_COM_COMPONENTS_FIND_PREVIOUS:
                pComComponents->FindPrevious(pComComponents->pListView->GetSelectedIndex()-1);
                break;
            }
        }
        break;
        case WM_NOTIFY:
            {
                // send notifications to listview
                CComComponents* pComComponents=((CComComponents*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
                if (!pComComponents)
                    break;
                if (pComComponents->pListView)
                {
                    if (pComComponents->pListView->OnNotify(wParam,lParam))
                        // if message has been proceed by pListview->OnNotify
                        break;
                }
            }
            break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CComComponents_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CComComponents_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CComComponents_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CComComponents_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::OnSize()
{
    RECT RectSearch;
    RECT RectListView;
    RECT RectWindow;
    HWND hItem;
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndDialog,&RectWindow);

    ////////////////
    // Search Group 
    ////////////////
    hItem=GetDlgItem(this->hWndDialog,IDC_STATIC_GROUP_COM_COMPONENTS_FIND);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectSearch);
    POINT SearchGroupPos;
    SearchGroupPos.x=RectSearch.left;
    SearchGroupPos.y=RectWindow.bottom -RectSearch.left -(RectSearch.bottom-RectSearch.top);// bottom - space - Search group height
    CDialogHelper::MoveGroupTo(this->hWndDialog,hItem,&SearchGroupPos);

    ////////////////
    // listview 
    ////////////////

    // get Search Group Rect
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectSearch);

    hItem=GetDlgItem(this->hWndDialog,IDC_LIST_COM_COMPONENTS);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectListView);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,0,
        2*RectWindow.left+(RectWindow.right-RectWindow.left)-2*(RectListView.left),
        (RectSearch.top-RectListView.top),
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: NoItemFoundMessage
// Object: show a Not Found Item Message box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::NoItemFoundMessage()
{
    MessageBox(this->hWndDialog,_T("No Item Found"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: UpdateSearchFields
// Object: update search param depending user interface
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CComComponents::UpdateSearchFields()
{
    // get check values
    this->SearchForCLSID=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_CLSID)==BST_CHECKED);
    this->SearchForName=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_CLASS_NAME)==BST_CHECKED);
    this->SearchForProgID=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_COM_COMPONENTS_PROG_ID)==BST_CHECKED);

    // add joker to the begin of searched string
    this->szSearchedString[0]=_T('*');

    // get search string
    GetDlgItemText(this->hWndDialog,IDC_EDIT_COM_COMPONENTS_FIND,&this->szSearchedString[1],MAX_PATH-3);
    this->szSearchedString[MAX_PATH-3]=0;

    // if empty search
    if (this->szSearchedString[1]==0)
    {
        MessageBox(this->hWndDialog,_T("Empty search"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // add joker to the end of searched string
    _tcscat(this->szSearchedString,_T("*"));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DoesItemMatch
// Object: check if item with index ItemIndex in main dialog list view match the filters
//          and if item match, select it
// Parameters :
//     in  : int ItemIndex : item index of dialog main pListview
//     out :
//     return : TRUE if item match, FALSE else
//-----------------------------------------------------------------------------
BOOL CComComponents::DoesItemMatch(int ItemIndex)
{
    TCHAR szText[MAX_PATH];
    if (this->SearchForCLSID)
    {
        this->pListView->GetItemText(ItemIndex,CComComponents_CLSID_COLUMN_INDEX,szText,MAX_PATH);
        if (CWildCharCompare::WildICmp(this->szSearchedString,szText))
            return TRUE;
    }
    if (this->SearchForName)
    {
        this->pListView->GetItemText(ItemIndex,CComComponents_NAME_COLUMN_INDEX,szText,MAX_PATH);
        if (CWildCharCompare::WildICmp(this->szSearchedString,szText))
            return TRUE;
    }
    if (this->SearchForProgID)
    {
        this->pListView->GetItemText(ItemIndex,CComComponents_PROGID_COLUMN_INDEX,szText,MAX_PATH);
        if (CWildCharCompare::WildICmp(this->szSearchedString,szText))
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: find first item matching conditions
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::Find()
{
    this->FindNext(0);
}

//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the previous item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::FindPrevious(int StartItemIndex)
{
    // if we are at the begin of the list
    if (StartItemIndex<0)
    {
        // we can't find more item
        this->NoItemFoundMessage();
        return;
    }

    // StartItemIndex check
    if (StartItemIndex>this->pListView->GetItemCount()-1)
        StartItemIndex=this->pListView->GetItemCount()-1;

    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;


    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(this->pListView->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->pListView->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for previous matching item in listview
    for(int cnt=StartItemIndex;cnt>=0;cnt--)
    {
        // stop if item matchs
        if (this->DoesItemMatch(cnt))
        {
            // select item
            this->pListView->SetSelectedIndex(cnt);

            // restore style
            SetWindowLongPtr(this->pListView->GetControlHandle(),GWL_STYLE,Styles);

            // assume item is visible
            this->pListView->ScrollTo(cnt);

            return;
        }
    }

    // restore style
    SetWindowLongPtr(this->pListView->GetControlHandle(),GWL_STYLE,Styles);

    // if no item found
    this->NoItemFoundMessage();
}

//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the next item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComComponents::FindNext(int StartItemIndex)
{
    // if we are at the end of the list
    if (StartItemIndex>=this->pListView->GetItemCount())
    {
        // we can't find more item
        this->NoItemFoundMessage();
        return;
    }

    // StartItemIndex check
    if (StartItemIndex<0)
        StartItemIndex=0;

    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;

    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(pListView->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(pListView->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for next matching item in listview
    for(int cnt=StartItemIndex;cnt<pListView->GetItemCount();cnt++)
    {
        // stop if item matchs
        if (this->DoesItemMatch(cnt))
        {
            // select item
            this->pListView->SetSelectedIndex(cnt);

            // restore style
            SetWindowLongPtr(pListView->GetControlHandle(),GWL_STYLE,Styles);

            // assume item is visible
            this->pListView->ScrollTo(cnt);

            return;
        }
    }
    // restore style
    SetWindowLongPtr(this->pListView->GetControlHandle(),GWL_STYLE,Styles);
    // if no item found
    this->NoItemFoundMessage();
}

//-----------------------------------------------------------------------------
// Name: FillCOMList
// Object: Fill COM Components listview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComComponents::FillCOMList(LPVOID lpParam)
{
    CComComponents* pComComponents=(CComComponents*)lpParam;

    DWORD cnt;
    TCHAR KeyName[MAX_PATH];
    DWORD KeyNameSize;
    TCHAR sz[MAX_PATH];
    DWORD szSize;
    DWORD NbSubKeys;
    
    HKEY hKeyCLSIDs;
    HKEY hKeyCurrentCLSID;
    HKEY hSubKey;
    LONG Ret;
    int ItemIndex;
    DWORD Type=0;

    _tcscpy(KeyName,CComComponents_KEY_CLSID);
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,KeyName,0,KEY_READ,&hKeyCLSIDs)!=ERROR_SUCCESS)
    {
        // error opening registry or not found
        MessageBox(pComComponents->hWndDialog,
                    _T("Error opening registry"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
    }

    
    // Get the class name and the value count. 
    RegQueryInfoKey(hKeyCLSIDs,// key handle 
                    NULL,      // buffer for class name 
                    NULL,      // size of class string 
                    NULL,      // reserved 
                    &NbSubKeys,// number of subkeys 
                    NULL,      // longest subkey size 
                    NULL,      // longest class string 
                    NULL,      // number of values for this key 
                    NULL,      // longest value name 
                    NULL,      // longest value data 
                    NULL,      // security descriptor 
                    NULL);     // last write time 


    // Enumerate the child keys
    for (cnt = 0, Ret = ERROR_SUCCESS; 
        (Ret == ERROR_SUCCESS) && (cnt<NbSubKeys) && (pComComponents->bStopParsing==FALSE);
        cnt++) 
    { 
        KeyNameSize=MAX_PATH;
        Ret = RegEnumKeyEx(hKeyCLSIDs, 
                            cnt, 
                            KeyName, 
                            &KeyNameSize, 
                            NULL, 
                            NULL, 
                            NULL, 
                            NULL); 
        if (Ret != ERROR_SUCCESS) 
            break;

        // CComComponents_CLSID_COLUMN_INDEX==0
        ItemIndex=pComComponents->pListView->AddItem(KeyName);
        if (ItemIndex<0)
            continue;

        // open CLSID key
        if(RegOpenKeyEx(hKeyCLSIDs,KeyName,0,KEY_READ,&hKeyCurrentCLSID)!=ERROR_SUCCESS)
            continue;

        // get default value
        szSize=MAX_PATH*sizeof(TCHAR);
        if (RegQueryValueEx(hKeyCurrentCLSID,NULL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
        {
            pComComponents->pListView->SetItemText(ItemIndex,CComComponents_NAME_COLUMN_INDEX,sz);
        }

        // open INPROCSERVER key
        if(RegOpenKeyEx(hKeyCurrentCLSID,CComComponents_KEY_INPROCSERVER,0,KEY_READ,&hSubKey)==ERROR_SUCCESS)
        {
            // get default value
            szSize=MAX_PATH*sizeof(TCHAR);
            if (RegQueryValueEx(hSubKey,NULL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
            {
                pComComponents->pListView->SetItemText(ItemIndex,CComComponents_MODULE_NAME_COLUMN_INDEX,sz);
            }
            // get THREADINGMODEL value
            szSize=MAX_PATH*sizeof(TCHAR);
            if (RegQueryValueEx(hSubKey,CComComponents_KEY_THREADINGMODEL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
            {
                pComComponents->pListView->SetItemText(ItemIndex,CComComponents_THREADING_MODEL_COLUMN_INDEX,sz);
            }
            // close INPROCSERVER key
            RegCloseKey(hSubKey);
        }
        else
        {
            // open LOCALSERVER key
            if(RegOpenKeyEx(hKeyCurrentCLSID,CComComponents_KEY_LOCALSERVER,0,KEY_READ,&hSubKey)==ERROR_SUCCESS)
            {
                // get default value
                szSize=MAX_PATH*sizeof(TCHAR);
                if (RegQueryValueEx(hSubKey,NULL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
                {
                    pComComponents->pListView->SetItemText(ItemIndex,CComComponents_MODULE_NAME_COLUMN_INDEX,sz);
                }
                // close LOCALSERVER key
                RegCloseKey(hSubKey);
            }
        }


        // open PROG_ID key
        if(RegOpenKeyEx(hKeyCurrentCLSID,CComComponents_KEY_PROG_ID,0,KEY_READ,&hSubKey)==ERROR_SUCCESS)
        {
            // get default value
            szSize=MAX_PATH*sizeof(TCHAR);
            if (RegQueryValueEx(hSubKey,NULL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
            {
                pComComponents->pListView->SetItemText(ItemIndex,CComComponents_PROGID_COLUMN_INDEX,sz);
            }
            // close PROG_ID key
            RegCloseKey(hSubKey);
        }

        // open VERSIONINDEPENDENTPROGID key
        if(RegOpenKeyEx(hKeyCurrentCLSID,CComComponents_KEY_VERSIONINDEPENDENTPROGID,0,KEY_READ,&hSubKey)==ERROR_SUCCESS)
        {
            // get default value
            szSize=MAX_PATH*sizeof(TCHAR);
            if (RegQueryValueEx(hSubKey,NULL,0,&Type,(PBYTE)sz,&szSize)==ERROR_SUCCESS)
            {
                pComComponents->pListView->SetItemText(ItemIndex,CComComponents_VERSIONINDEPENDENTPROGID_COLUMN_INDEX,sz);
            }
            // close VERSIONINDEPENDENTPROGID key
            RegCloseKey(hSubKey);
        }

        // close CLSID key
        RegCloseKey(hKeyCurrentCLSID);
    } 
    // close CLSIDs key
    RegCloseKey(hKeyCLSIDs);

    SetEvent(pComComponents->hParsingFinished);
    return 0;
}