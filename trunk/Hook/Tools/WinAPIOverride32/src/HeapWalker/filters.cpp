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
// Object: manages the processes filters dialog
//-----------------------------------------------------------------------------

#include "filters.h"

CFilters::CFilters()
{
    this->hWndDialog=NULL;
}
CFilters::~CFilters()
{

}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::Init()
{
    // add values to combo before loading options
    HWND hWndComboFlags=GetDlgItem(this->hWndDialog,IDC_COMBO_FILTERS_FLAGS);
    ComboBox_AddString(hWndComboFlags,HEAP_WALKER_MEMORY_FLAG_ALL);
    ComboBox_AddString(hWndComboFlags,HEAP_WALKER_MEMORY_FLAG_FIXED);
    ComboBox_AddString(hWndComboFlags,HEAP_WALKER_MEMORY_FLAG_MOVABLE);
    ComboBox_AddString(hWndComboFlags,HEAP_WALKER_MEMORY_FLAG_FREE);

    // load options
    this->LoadOptions();
}

//-----------------------------------------------------------------------------
// Name: LoadOptions
// Object: set user interface values according to pOptions object
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::LoadOptions()
{
    if (this->pOptions->FilterApplyToCurrentEntries)
        CheckDlgButton(this->hWndDialog,IDC_CHECK_FILTERS_APPLY_TO_CURRENT_RESULTS,BST_CHECKED);

    if (this->pOptions->FilterMinSize!=0)
        SetDlgItemInt(this->hWndDialog,IDC_EDIT_FILTERS_MIN_SIZE,this->pOptions->FilterMinSize,FALSE);
    if (this->pOptions->FilterMaxSize!=0)
        SetDlgItemInt(this->hWndDialog,IDC_EDIT_FILTERS_MAX_SIZE,this->pOptions->FilterMaxSize,FALSE);

    TCHAR psz[MAX_PATH];
    if (this->pOptions->FilterMinAddress!=0)
    {
        _stprintf(psz,_T("0x%x"),this->pOptions->FilterMinAddress);
        SetDlgItemText(this->hWndDialog,IDC_EDIT_FILTERS_MIN_ADDRESS,psz);
    }
    if (this->pOptions->FilterMaxAddress!=0)
    {
        _stprintf(psz,_T("0x%x"),this->pOptions->FilterMaxAddress);
        SetDlgItemText(this->hWndDialog,IDC_EDIT_FILTERS_MAX_ADDRESS,psz);
    }

    HWND hWndComboFlags=GetDlgItem(this->hWndDialog,IDC_COMBO_FILTERS_FLAGS);
    switch (this->pOptions->FilterMemoryFlags)
    {
        case COptions::FILTERMEMORYFLAGS_ALL:
            ComboBox_SelectString(hWndComboFlags,-1,HEAP_WALKER_MEMORY_FLAG_ALL);
            break;
        case COptions::FILTERMEMORYFLAGS_FIXED:
            ComboBox_SelectString(hWndComboFlags,-1,HEAP_WALKER_MEMORY_FLAG_FIXED);
            break;
        case COptions::FILTERMEMORYFLAGS_MOVABLE:
            ComboBox_SelectString(hWndComboFlags,-1,HEAP_WALKER_MEMORY_FLAG_MOVABLE);
            break;
        case COptions::FILTERMEMORYFLAGS_FREE:
            ComboBox_SelectString(hWndComboFlags,-1,HEAP_WALKER_MEMORY_FLAG_FREE);
            break;
    }

}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve DWORD value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : DWORD* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CFilters::GetValue(int ControlID,DWORD* pValue)
{
    TCHAR psz[MAX_PATH];
    int iScanfRes;
    *pValue=0;
    
    if (!SendMessage(GetDlgItem(this->hWndDialog,ControlID),(UINT) WM_GETTEXTLENGTH,0, 0))
        return TRUE;
    if (!GetDlgItemText(this->hWndDialog,ControlID,psz,MAX_PATH-1))
        return FALSE;
    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),pValue);
    else
        iScanfRes=_stscanf(psz,_T("%u"),pValue);

    if (iScanfRes!=1)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Bad Value : %s"),psz);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SaveOptions
// Object: save user interface options into pOptions object
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CFilters::SaveOptions()
{

    this->pOptions->FilterApplyToCurrentEntries=
        (IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_FILTERS_APPLY_TO_CURRENT_RESULTS)==BST_CHECKED);

    if (!this->GetValue(IDC_EDIT_FILTERS_MIN_SIZE,&this->pOptions->FilterMinSize))
        return FALSE;

    if (!this->GetValue(IDC_EDIT_FILTERS_MAX_SIZE,&this->pOptions->FilterMaxSize))
        return FALSE;

    if (!this->GetValue(IDC_EDIT_FILTERS_MIN_ADDRESS,&this->pOptions->FilterMinAddress))
        return FALSE;

    if (!this->GetValue(IDC_EDIT_FILTERS_MAX_ADDRESS,&this->pOptions->FilterMaxAddress))
        return FALSE;

    TCHAR psz[MAX_PATH];
    HWND hWndComboFlags=GetDlgItem(this->hWndDialog,IDC_COMBO_FILTERS_FLAGS);
    ComboBox_GetText(hWndComboFlags,psz,MAX_PATH);

    if (_tcscmp(psz,HEAP_WALKER_MEMORY_FLAG_ALL)==0)
    {
        this->pOptions->FilterMemoryFlags=COptions::FILTERMEMORYFLAGS_ALL;
    }
    else if (_tcscmp(psz,HEAP_WALKER_MEMORY_FLAG_FIXED)==0)
    {
        this->pOptions->FilterMemoryFlags=COptions::FILTERMEMORYFLAGS_FIXED;
    }
    else if (_tcscmp(psz,HEAP_WALKER_MEMORY_FLAG_MOVABLE)==0)
    {
        this->pOptions->FilterMemoryFlags=COptions::FILTERMEMORYFLAGS_MOVABLE;
    }
    else if (_tcscmp(psz,HEAP_WALKER_MEMORY_FLAG_FREE)==0)
    {
        this->pOptions->FilterMemoryFlags=COptions::FILTERMEMORYFLAGS_FREE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowFilterDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CFilters::Show(HINSTANCE hInstance,HWND hWndDialog,COptions* pOptions)
{
    INT_PTR Ret;
    CFilters* pFilters=new CFilters();
    pFilters->hInstance=hInstance;
    pFilters->pOptions=pOptions;
    // show dialog
    Ret= DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_FILTERS,hWndDialog,(DLGPROC)CFilters::WndProc,(LPARAM)pFilters);
    delete pFilters;
    return Ret;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close Filters dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::Close(tagDlgRes DlgRes)
{
    EndDialog(this->hWndDialog,(INT_PTR) DlgRes);
}

//-----------------------------------------------------------------------------
// Name: FiltersWndProc
// Object: dialog callback of the Filters dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CFilters::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CFilters* pFilters=(CFilters*)lParam;
            pFilters->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pFilters);
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_FILTER);

            pFilters->Init();

        }

        break;
    case WM_CLOSE:
        {
            CFilters* pFilters=((CFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pFilters)
                break;
            pFilters->Close(CFilters::DLG_RES_CANCEL);
        }
        break;
    case WM_COMMAND:
        {
            CFilters* pFilters=((CFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pFilters)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
                // save filters settings
                if (!pFilters->SaveOptions())
                    break;
                // exit
                pFilters->Close(CFilters::DLG_RES_OK);
                break;
            case IDCANCEL:
                pFilters->Close(CFilters::DLG_RES_CANCEL);
                break;
            }
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}