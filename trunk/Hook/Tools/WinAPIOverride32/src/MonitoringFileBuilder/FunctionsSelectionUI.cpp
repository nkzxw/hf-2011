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
// Object: manages the functions selection UI
//-----------------------------------------------------------------------------

#include "FunctionsSelectionUI.h"

using namespace NET;

//using NET::CFunctionInfo;
//using NET::CParameterInfo;

CFunctionsSelectionUI::CFunctionsSelectionUI(CNetMonitoringFileGenerator* pNetMonitoringFileGenerator)
{
    this->CommonConstructor();
    this->bExport=TRUE;
    this->pNetMonitoringFileGenerator=pNetMonitoringFileGenerator;

    CLinkListItem* pItem;
    DWORD cnt;
    CFunctionInfo* pFunctionInfo;
    CLinkListItem* pItemParam;
    CParameterInfo* pParamInfo;
    TCHAR* psz;
    TCHAR szParam[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];

    this->pLibrariesSize=1;
    // we just get one export lib
    this->pLibraries=new LIBRARY[1];
    // create exported func array
    this->pLibraries[0].pFunctions=new IMPORT_EXPORT_FUNCTIONS[pNetMonitoringFileGenerator->pLinkListFunctions->GetItemsCount()];

    // for each exported function
    for (pItem=pNetMonitoringFileGenerator->pLinkListFunctions->Head,cnt=0;pItem;pItem=pItem->NextItem,cnt++)
    {
        pFunctionInfo=(CFunctionInfo*)pItem->ItemData;
        // no ordinal for .NET but Token
        this->pLibraries[0].pFunctions[cnt].bOrdinal=TRUE;

        // get ordinal value
        this->pLibraries[0].pFunctions[cnt].Ordinal=pFunctionInfo->FunctionToken;

        // copy func name

        // function name
        _tcscpy(this->pLibraries[0].pFunctions[cnt].Name,pFunctionInfo->szName);

        _tcscat(this->pLibraries[0].pFunctions[cnt].Name,_T("("));
        // for each parameter
        for (pItemParam=pFunctionInfo->pParameterInfoList->Head;pItemParam;pItemParam=pItemParam->NextItem)
        {
            pParamInfo=(CParameterInfo*)pItemParam->ItemData;
            // if not first param
            if (pItemParam!=pFunctionInfo->pParameterInfoList->Head)
                _tcscat(this->pLibraries[0].pFunctions[cnt].Name,_T(","));

            // get type
            psz=CSupportedParameters::GetParamName(pParamInfo->GetWinAPIOverrideType() & SIMPLE_TYPE_FLAG_MASK);

            // get name (in case of typed arg)
            pParamInfo->GetName(szParam,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);

            // if type and name are different
            if (_tcscmp(psz,szParam)!=0)
            {
                // show both
                _tcscat(this->pLibraries[0].pFunctions[cnt].Name,psz);
                _tcscat(this->pLibraries[0].pFunctions[cnt].Name,_T(" "));
                _tcscat(this->pLibraries[0].pFunctions[cnt].Name,szParam);
            }
            else // only show one
                _tcscat(this->pLibraries[0].pFunctions[cnt].Name,psz);
        }
        _tcscat(this->pLibraries[0].pFunctions[cnt].Name,_T(")"));



        *this->pLibraries[0].pFunctions[cnt].UndecoratedName=0;

        // defined it as selected by default
        this->pLibraries[0].pFunctions[cnt].bSelected=TRUE;
    }
    // get number of functions for which we get name
    this->pLibraries[0].NbFunctions=cnt;
}

void CFunctionsSelectionUI::CommonConstructor()
{
    this->pPE=NULL;
    this->bExport=TRUE;
    this->pListViewLibraries=NULL;
    this->pListViewFunctions=NULL;
    this->pLibrariesSize=0;
    this->pLibraries=NULL;
    this->pLibrariesCurrentIndex=0;
    this->hWndDialog=0;
    this->pNetMonitoringFileGenerator=NULL;

    this->hWndComboSearch=NULL;
    this->evtComboTextChange=CreateEvent(NULL,FALSE,FALSE,NULL);
    this->evtClose=CreateEvent(NULL,TRUE,FALSE,NULL);
    this->bClosed=FALSE;

    // create thread
    this->FinderHelperThread=CreateThread(NULL,0,CFunctionsSelectionUI::FunctionFinderHelper,this,0,NULL);
}
//-----------------------------------------------------------------------------
// Name: CFunctionsSelectionUI
// Object: constructor
// Parameters :
//     in  : CPE* pPE : pointer to CPE object of selected file
//           BOOL bExport : TRUE if export table has been parsed, FALSE if import table has been parsed
//     out :
//     return : 
//-----------------------------------------------------------------------------
CFunctionsSelectionUI::CFunctionsSelectionUI(CPE* pPE,BOOL bExport)
{
    this->CommonConstructor();
    this->pPE=pPE;
    this->bExport=bExport;


    CLinkListItem* pItem;
    DWORD cnt;

#if(defined(UNICODE)||defined(_UNICODE))
    char* DecoratedName;
    char UndecoratedName[2*MAX_PATH];
    TCHAR* psz;
#endif


    if (this->bExport)
    {
        this->pLibrariesSize=1;
        // we just get one export lib
        this->pLibraries=new LIBRARY[1];
        // create exported func array
        this->pLibraries[0].pFunctions=new IMPORT_EXPORT_FUNCTIONS[this->pPE->pExportTable->GetItemsCount()];

        // for each exported function
        for (pItem=this->pPE->pExportTable->Head,cnt=0;pItem;pItem=pItem->NextItem,cnt++)
        {
            // we always get ordinal for export table
            this->pLibraries[0].pFunctions[cnt].bOrdinal=TRUE;

            // get ordinal value
            this->pLibraries[0].pFunctions[cnt].Ordinal=((CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData)->ExportedOrdinal;

            // copy func name
            _tcscpy(this->pLibraries[0].pFunctions[cnt].Name,((CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData)->FunctionName);

            // if ordinal import only
            if (*this->pLibraries[0].pFunctions[cnt].Name==0)
            {
                *this->pLibraries[0].pFunctions[cnt].Name=0;
                *this->pLibraries[0].pFunctions[cnt].UndecoratedName=0;
            }
            else
            {
#if(defined(UNICODE)||defined(_UNICODE))
                CAnsiUnicodeConvert::UnicodeToAnsi(this->pLibraries[0].pFunctions[cnt].Name,&DecoratedName);
                UnDecorateSymbolName(DecoratedName,UndecoratedName,2*MAX_PATH,UNDNAME_COMPLETE);
                free(DecoratedName);

                CAnsiUnicodeConvert::AnsiToUnicode(UndecoratedName,&psz);
                _tcscpy(this->pLibraries[0].pFunctions[cnt].UndecoratedName,psz);
                free(psz);
#else
                UnDecorateSymbolName(this->pLibraries[0].pFunctions[cnt].Name,
                                    this->pLibraries[0].pFunctions[cnt].UndecoratedName,
                                    2*MAX_PATH,
                                    UNDNAME_COMPLETE
                                    );
#endif

            }

            // defined it as selected by default
            this->pLibraries[0].pFunctions[cnt].bSelected=TRUE;
        }
        // get number of functions for which we get name
        this->pLibraries[0].NbFunctions=cnt;
    }
    else
    {
        CPE::IMPORT_LIBRARY_ITEM* pImportLibraryItem;
        CPE::IMPORT_FUNCTION_ITEM* pImportFunctionItem;
        CLinkListItem* pItemFunc;
        DWORD cnt2;
        // get number of imported libs
        this->pLibrariesSize=this->pPE->pImportTable->GetItemsCount();

        if (this->pLibrariesSize>0)
        {
            this->pLibraries=new LIBRARY[this->pLibrariesSize];
            
            // for each imported library
            for (pItem=this->pPE->pImportTable->Head,cnt=0;pItem;pItem=pItem->NextItem,cnt++)
            {
                // get library info
                pImportLibraryItem=(CPE::IMPORT_LIBRARY_ITEM*)pItem->ItemData;
                // get lib name
                _tcscpy(this->pLibraries[cnt].Name,pImportLibraryItem->LibraryName);

                // get nb imported functions for this lib
                this->pLibraries[cnt].NbFunctions=pImportLibraryItem->pFunctions->GetItemsCount();

                // create imported func array
                this->pLibraries[cnt].pFunctions=new IMPORT_EXPORT_FUNCTIONS[this->pLibraries[cnt].NbFunctions];

                // for each imported function of imported library
                for (pItemFunc=pImportLibraryItem->pFunctions->Head,cnt2=0;pItemFunc;pItemFunc=pItemFunc->NextItem,cnt2++)
                {
                    pImportFunctionItem=(CPE::IMPORT_FUNCTION_ITEM*)pItemFunc->ItemData;

                    // ordinal is significant only if function was ordinal import
                    this->pLibraries[cnt].pFunctions[cnt2].bOrdinal=pImportFunctionItem->bOrdinalOnly;

                    // get ordinal value
                    this->pLibraries[cnt].pFunctions[cnt2].Ordinal=pImportFunctionItem->Ordinal;

                    // if ordinal import only
                    if (*pImportFunctionItem->FunctionName==0)
                    {
                        *this->pLibraries[cnt].pFunctions[cnt2].Name=0;
                        *this->pLibraries[cnt].pFunctions[cnt2].UndecoratedName=0;
                    }
                    else
                    {
                        // copy func name to array 
                        _tcscpy(this->pLibraries[cnt].pFunctions[cnt2].Name,pImportFunctionItem->FunctionName);

#if(defined(UNICODE)||defined(_UNICODE))
                        CAnsiUnicodeConvert::UnicodeToAnsi(this->pLibraries[cnt].pFunctions[cnt2].Name,&DecoratedName);
                        UnDecorateSymbolName(DecoratedName,UndecoratedName,2*MAX_PATH,UNDNAME_COMPLETE);
                        free(DecoratedName);

                        CAnsiUnicodeConvert::AnsiToUnicode(UndecoratedName,&psz);
                        _tcscpy(this->pLibraries[cnt].pFunctions[cnt2].UndecoratedName,psz);
                        free(psz);
#else
                        UnDecorateSymbolName(this->pLibraries[cnt].pFunctions[cnt2].Name,
                            this->pLibraries[cnt].pFunctions[cnt2].UndecoratedName,
                            2*MAX_PATH,
                            UNDNAME_COMPLETE
                            );
#endif

                    }

                    // defined it as selected by default
                    this->pLibraries[cnt].pFunctions[cnt2].bSelected=TRUE;
                }
            }
        }
    }
}
CFunctionsSelectionUI::~CFunctionsSelectionUI()
{
    // free memory
    int cnt;
    for (cnt=0;cnt<this->pLibrariesSize;cnt++)
        delete[] this->pLibraries[cnt].pFunctions;

    delete[] this->pLibraries;

    // wait the end of FinderHelperThread, because object variable are accessed
    if (this->FinderHelperThread)
    {
        WaitForSingleObject(this->FinderHelperThread,FUNCTION_SELECTION_UI_FUNCTION_HELPER_TIME_TO_SHOW*2);
        CloseHandle(this->FinderHelperThread);
    }
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show function selection dialog
// Parameters :
//     in  : HINSTANCE Instance : instance of module containing resources
//           HWND hWndParent : parent window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CFunctionsSelectionUI::Show(HINSTANCE Instance,HWND hWndParent)
{
    // create object
    
    this->hInstance=Instance;
    // show dialog
    return DialogBoxParam(Instance, (LPCTSTR)IDD_DIALOG_FUNCTIONS_SELECTION,hWndParent, (DLGPROC)WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: Init function selection dialog
// Parameters :
//     in  : HWND hWnd : window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::Init(HWND hwnd)
{
    HWND hItem;
    RECT Rect;
    RECT Rect2;
    this->hWndDialog=hwnd;

    this->hWndComboSearch=GetDlgItem(this->hWndDialog,IDC_COMBO_WIZARD_FUNC_SEARCH);

    if (this->bExport)
    {
        // hide IDC_STATIC_IMPORTED_LIBRARIES
        ShowWindow(GetDlgItem(this->hWndDialog,IDC_STATIC_IMPORTED_LIBRARIES),SW_HIDE);
        
        // hide IDC_LIST_IMPORTED_LIBRARIES
        ShowWindow(GetDlgItem(this->hWndDialog,IDC_LIST_IMPORTED_LIBRARIES),SW_HIDE);

        // hide check / uncheck all imported lib
        ShowWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_CHECK_ALL_IMPORTED_LIBS),SW_HIDE);
        ShowWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_UNCHECK_ALL_IMPORTED_LIBS),SW_HIDE);

        // set IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS name
        SetDlgItemText(this->hWndDialog,IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS,_T("Exported Functions"));

        // resize IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS
        CDialogHelper::GetClientWindowRect(this->hWndDialog,GetDlgItem(this->hWndDialog,IDC_STATIC_IMPORTED_LIBRARIES),&Rect2);
        hItem=GetDlgItem(this->hWndDialog,IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS);
        CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
        SetWindowPos(hItem,
                     HWND_NOTOPMOST,
                     Rect2.left,
                     Rect.top,
                     Rect.right-Rect2.left,
                     Rect.bottom-Rect.top,
                     SWP_NOACTIVATE|SWP_NOZORDER);
        CDialogHelper::Redraw(hItem);

        // resize IDC_LIST_IMPORTED_EXPORTED_FUNCTIONS
        CDialogHelper::GetClientWindowRect(this->hWndDialog,GetDlgItem(this->hWndDialog,IDC_LIST_IMPORTED_LIBRARIES),&Rect2);
        hItem=GetDlgItem(this->hWndDialog,IDC_LIST_IMPORTED_EXPORTED_FUNCTIONS);
        CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
        SetWindowPos(hItem,
                     HWND_NOTOPMOST,
                     Rect2.left,
                     Rect.top,
                     Rect.right-Rect2.left,
                     Rect.bottom-Rect.top,
                     SWP_NOACTIVATE|SWP_NOZORDER);
        CDialogHelper::Redraw(hItem);

    }
    else
    {
        // set IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS name
        SetDlgItemText(this->hWndDialog,IDC_STATIC_IMPORTED_EXPORTED_FUNCTIONS,_T("Imported Functions"));

        this->pListViewLibraries=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_IMPORTED_LIBRARIES));
        this->pListViewLibraries->SetStyle(TRUE,FALSE,FALSE,TRUE);
        this->pListViewLibraries->AddColumn(_T("Import Libraries"),180,LVCFMT_LEFT);
        this->pListViewLibraries->SetSelectItemCallback(CFunctionsSelectionUI::SelectItemCallbackStatic,this);
        this->pListViewLibraries->SetUnselectItemCallback(CFunctionsSelectionUI::UnselectItemCallbackStatic,this);

        // show imported libraries
        for (int cnt=0;cnt<this->pLibrariesSize;cnt++)
        {
            this->pListViewLibraries->AddItem(this->pLibraries[cnt].Name);
        }
        this->pListViewLibraries->SelectAll();
    }

    int ColumnSize;
    if (this->bExport)
        ColumnSize=300;
    else
        ColumnSize=200;
    this->pListViewFunctions=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_IMPORTED_EXPORTED_FUNCTIONS));
    this->pListViewFunctions->SetStyle(TRUE,FALSE,FALSE,TRUE);
    if (this->pNetMonitoringFileGenerator)
    {
        this->pListViewFunctions->AddColumn(_T("Token"),100,LVCFMT_LEFT);
        this->pListViewFunctions->AddColumn(_T("Function Name"),2*ColumnSize,LVCFMT_LEFT);
        this->pListViewFunctions->AddColumn(_T(""),0,LVCFMT_LEFT);// only to keep same number as other cases
    }
    else
    {
        this->pListViewFunctions->AddColumn(_T("Ordinal"),100,LVCFMT_LEFT);
        this->pListViewFunctions->AddColumn(_T("Function Name"),ColumnSize,LVCFMT_LEFT);
        this->pListViewFunctions->AddColumn(_T("Undecorated Name"),ColumnSize,LVCFMT_LEFT);
    }
    if (this->pLibrariesSize)
        this->ShowFunctionsOfLibrary(0);
    else
        MessageBox(this->hWndDialog,_T("No imported libraries"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close function selection dialog
// Parameters :
//     in  : INT_PTR DlgRes : dialog result
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::Close(INT_PTR DlgRes)
{
    SetEvent(this->evtClose);
    this->bClosed=TRUE;

    if (this->pListViewLibraries)
    {
        delete this->pListViewLibraries;
        this->pListViewLibraries=NULL;
    }

    if (this->pListViewFunctions)
    {
        delete this->pListViewFunctions;
        this->pListViewFunctions=NULL;
    }

    // close dialog
    EndDialog(this->hWndDialog,DlgRes);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: function selection window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CFunctionsSelectionUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLong(hWnd,GWLP_USERDATA,(LONG)lParam);
        // set Dialog icon
        CDialogHelper::SetIcon(hWnd,IDI_ICON_MONITORINGFILEBUILDER);
        // call the init method
        ((CFunctionsSelectionUI*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // get CFunctionsSelectionUI object
            CFunctionsSelectionUI* pFunctionsSelectionUI=(CFunctionsSelectionUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pFunctionsSelectionUI)
                // call the close method
                pFunctionsSelectionUI->Close(IDCANCEL);
            break;
        }
        break;
    case WM_COMMAND:
        {
            // get CFunctionsSelectionUI object
            CFunctionsSelectionUI* pFunctionsSelectionUI=(CFunctionsSelectionUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pFunctionsSelectionUI)
            {
                switch (LOWORD(wParam))
                {
                case IDOK:
                    // call the close method
                    pFunctionsSelectionUI->ApplyChanges();
                    pFunctionsSelectionUI->Close(IDOK);
                    break;
                case IDCANCEL:
                    pFunctionsSelectionUI->Close(IDCANCEL);
                    break;
                case IDC_BUTTON_CHECK_ALL:
                    pFunctionsSelectionUI->CheckAll();
                    break;
                case IDC_BUTTON_UNCHECK_ALL:
                    pFunctionsSelectionUI->UncheckAll();
                    break;
                case IDC_BUTTON_CHECK_SELECTED:
                    pFunctionsSelectionUI->CheckSelected();
                    break;
                case IDC_BUTTON_UNCHECK_SELECTED:
                    pFunctionsSelectionUI->UncheckSelected();
                    break;

                case IDC_BUTTON_CHECK_ALL_IMPORTED_LIBS:
                    pFunctionsSelectionUI->CheckAllImportedLibs();
                    break;
                case IDC_BUTTON_UNCHECK_ALL_IMPORTED_LIBS:
                    pFunctionsSelectionUI->UncheckAllImportedLibs();
                    break;
                }

                if ((HWND)lParam==pFunctionsSelectionUI->hWndComboSearch)
                {
                    switch(HIWORD(wParam))
                    {
                    case CBN_EDITCHANGE:
                        SetEvent(pFunctionsSelectionUI->evtComboTextChange);
                        break;
                    case CBN_SELCHANGE:
                        pFunctionsSelectionUI->SelectSearchedFunctionInListView();
                        break;
                    }
                }

            }
            break;
        }
    case WM_NOTIFY:
        {
            // get CFunctionsSelectionUI object
            CFunctionsSelectionUI* pFunctionsSelectionUI=(CFunctionsSelectionUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pFunctionsSelectionUI)
            {
                if (pFunctionsSelectionUI->pListViewLibraries)
                {
                    if (pFunctionsSelectionUI->pListViewLibraries->OnNotify(wParam,lParam))
                        // if message has been proceed by pListview->OnNotify
                        break;
                }
                if (pFunctionsSelectionUI->pListViewFunctions)
                {
                    if (pFunctionsSelectionUI->pListViewFunctions->OnNotify(wParam,lParam))
                        // if message has been proceed by pListview->OnNotify
                        break;
                }
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SelectItemCallbackStatic
// Object: callback called on library name selection in library listview
// Parameters :
//     in  : int ItemIndex : selected item index in library listview
//           int SubItemIndex : selected subitem index in library listview
//           LPVOID UserParam : associated CFunctionsSelectionUI object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::SelectItemCallbackStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    // reenter object oriented model
    ((CFunctionsSelectionUI*)UserParam)->SelectItemCallback(ItemIndex,SubItemIndex);
}

void CFunctionsSelectionUI::SelectItemCallback(int ItemIndex,int SubItemIndex)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    TCHAR pszLibName[MAX_PATH];
    int IndexOfpLibraries=0;

    // find index of pLibraries corresponding to the clicked library
    this->pListViewLibraries->GetItemText(ItemIndex,pszLibName,MAX_PATH);

    // find it in this->pLibraries
    for (int cnt=0;cnt<this->pLibrariesSize;cnt++)
    {
        if (_tcscmp(this->pLibraries[cnt].Name,pszLibName)==0)
        {
            IndexOfpLibraries=cnt;
            break;
        }
    }

    this->ShowFunctionsOfLibrary(IndexOfpLibraries);
}

//-----------------------------------------------------------------------------
// Name: UnselectItemCallbackStatic
// Object: callback called when item is unselected in library listview
// Parameters :
//     in  : int ItemIndex : unselected item index in library listview
//           int SubItemIndex : unselected subitem index in library listview
//           LPVOID UserParam : associated CFunctionsSelectionUI object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::UnselectItemCallbackStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    // reenter object oriented model
    ((CFunctionsSelectionUI*)UserParam)->UnselectItemCallback(ItemIndex,SubItemIndex);
}
void CFunctionsSelectionUI::UnselectItemCallback(int ItemIndex,int SubItemIndex)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    UNREFERENCED_PARAMETER(SubItemIndex);
    // don't search IndexOfpLibraries : it is already stored in this->pLibrariesCurrentIndex

    // store item state
    for (int cnt=0;cnt<this->pListViewFunctions->GetItemCount();cnt++)
        this->pLibraries[this->pLibrariesCurrentIndex].pFunctions[cnt].bSelected=
                this->IsFuncSelected(this->pLibraries[this->pLibrariesCurrentIndex].pFunctions[cnt].Name,this->pLibraries[this->pLibrariesCurrentIndex].pFunctions[cnt].Ordinal);
}

//-----------------------------------------------------------------------------
// Name: ShowFunctionsOfLibrary
// Object: show functions of selected library
// Parameters :
//     in  : int IndexOfpLibraries : index of selected item in library listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::ShowFunctionsOfLibrary(int IndexOfpLibraries)
{
    DWORD cnt;
    TCHAR* ppcText[3];
    TCHAR pszOrdinal[50];
    // first column contains ordinal value
    ppcText[0]=pszOrdinal;

     // clear ListViewFunctions
    this->pListViewFunctions->Clear();

    // check if there's functions
    if (this->pLibraries[IndexOfpLibraries].NbFunctions==0)
        MessageBox(this->hWndDialog,_T("No functions or no functions names"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    // add items
    for (cnt=0;cnt<this->pLibraries[IndexOfpLibraries].NbFunctions;cnt++)
    {
        // check if we have ordinal value
        if (this->pLibraries[IndexOfpLibraries].pFunctions[cnt].bOrdinal)
        {
            if (this->pNetMonitoringFileGenerator)
                _stprintf(pszOrdinal,_T("0x%.8X"),this->pLibraries[IndexOfpLibraries].pFunctions[cnt].Ordinal);
            else
                // copy ordinal
                _stprintf(pszOrdinal,_T("0x%.4X"),this->pLibraries[IndexOfpLibraries].pFunctions[cnt].Ordinal);
        }
        else
            *pszOrdinal=0;

        // store pointer to function name
        ppcText[1]=this->pLibraries[IndexOfpLibraries].pFunctions[cnt].Name;

        // check if undacorated name is the same as standard name
        if (_tcscmp(this->pLibraries[IndexOfpLibraries].pFunctions[cnt].Name,
                    this->pLibraries[IndexOfpLibraries].pFunctions[cnt].UndecoratedName
                    )==0
            )
        {
            this->pListViewFunctions->AddItemAndSubItems(2,ppcText);
        }
        else
        {
            ppcText[2]=this->pLibraries[IndexOfpLibraries].pFunctions[cnt].UndecoratedName;
            this->pListViewFunctions->AddItemAndSubItems(3,ppcText);
        }

        this->pListViewFunctions->SetSelectedState(cnt,this->pLibraries[IndexOfpLibraries].pFunctions[cnt].bSelected);
    }
    // store IndexOfpLibraries
    this->pLibrariesCurrentIndex=IndexOfpLibraries;

    // restore last sorting order
    this->pListViewFunctions->ReSort();
}
void CFunctionsSelectionUI::CheckAllImportedLibs()
{
    this->pListViewLibraries->SelectAll();
}
void CFunctionsSelectionUI::UncheckAllImportedLibs()
{
    this->pListViewLibraries->UnselectAll();
}
void CFunctionsSelectionUI::CheckAll()
{
    this->pListViewFunctions->SelectAll();
}
void CFunctionsSelectionUI::UncheckAll()
{
    this->pListViewFunctions->UnselectAll();
}
void CFunctionsSelectionUI::CheckSelected()
{
    this->pListViewFunctions->CheckSelected();
}
void CFunctionsSelectionUI::UncheckSelected()
{
    this->pListViewFunctions->UncheckSelected();
}

//-----------------------------------------------------------------------------
// Name: ApplyChanges
// Object: apply changes done by user directly to PE (really bad programming :( )
// Parameters :
//     in  : int IndexOfpLibraries : index of selected item in library listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::ApplyChanges()
{
    // force last selected lib to update it's func states
    this->UnselectItemCallback(0,0);

    // directly apply changes to pe object (it's not the cleanest way but the fastest one :D )
    CLinkListItem* pItemLib;
    CLinkListItem* pNextItemLib;
    CLinkListItem* pItemFunc;
    CLinkListItem* pNextItemFunc;
    DWORD IndexFunc;
    DWORD IndexLib;
    CPE::IMPORT_LIBRARY_ITEM* pImportLibraryItem;
    BOOL bLibSelected;
    if (this->pNetMonitoringFileGenerator)
    {
        pItemFunc=this->pNetMonitoringFileGenerator->pLinkListFunctions->Head;
        IndexFunc=0;
        while (pItemFunc)
        {
            // store next item
            pNextItemFunc=pItemFunc->NextItem;

            // if item is unselected remove it
            if (!this->pLibraries[0].pFunctions[IndexFunc].bSelected)// we can do it because index is linked to list order (see allocation of this->pLibraries)
            {
                delete ((CFunctionInfo*)pItemFunc->ItemData);
                this->pNetMonitoringFileGenerator->pLinkListFunctions->RemoveItem(pItemFunc);
            }
            // restore next item
            pItemFunc=pNextItemFunc;
            IndexFunc++;
        }
    }
    else
    {
        if (this->bExport)
        {
            pItemFunc=this->pPE->pExportTable->Head;
            IndexFunc=0;
            while (pItemFunc)
            {
                // store next item
                pNextItemFunc=pItemFunc->NextItem;

                // if item is unselected remove it
                if (!this->pLibraries[0].pFunctions[IndexFunc].bSelected)// we can do it because index is linked to list order (see allocation of this->pLibraries)
                    this->pPE->pExportTable->RemoveItem(pItemFunc);

                // restore next item
                pItemFunc=pNextItemFunc;
                IndexFunc++;
            }
        }
        else
        {
            pItemLib=this->pPE->pImportTable->Head;
            IndexLib=0;
            while(pItemLib)
            {
                // store next item
                pNextItemLib=pItemLib->NextItem;

                pImportLibraryItem=(CPE::IMPORT_LIBRARY_ITEM*)pItemLib->ItemData;

                bLibSelected=this->IsLibSelected(pImportLibraryItem->LibraryName);
                if (bLibSelected)
                {
                    pItemFunc=pImportLibraryItem->pFunctions->Head;
                    IndexFunc=0;
                    while (pItemFunc)
                    {
                        // store next item
                        pNextItemFunc=pItemFunc->NextItem;

                        // if item is unselected remove it
                        if (!this->pLibraries[IndexLib].pFunctions[IndexFunc].bSelected)// we can do it because index is linked to list order (see allocation of this->pLibraries)
                            pImportLibraryItem->pFunctions->RemoveItem(pItemFunc);

                        // restore next item
                        pItemFunc=pNextItemFunc;
                        IndexFunc++;
                    }   
                }
                else
                {
                    // remove func associated with pImportLibraryItem
                    delete pImportLibraryItem->pFunctions;
                    // remove pItemLib item
                    this->pPE->pImportTable->RemoveItem(pItemLib);
                }

                // restore next item
                pItemLib=pNextItemLib;
                IndexLib++;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: IsLibSelected
// Object: check if LibName is selected in library listview
// Parameters :
//     in  : TCHAR* LibName
//     out :
//     return : TRUE if selected, FALSE else
//-----------------------------------------------------------------------------
BOOL CFunctionsSelectionUI::IsLibSelected(TCHAR* LibName)
{
    return IsListviewNameSelected(this->pListViewLibraries,LibName,0);
}

//-----------------------------------------------------------------------------
// Name: IsFuncSelected
// Object: check if function is selected in function listview
//         by checking function name or ordinal
// Parameters :
//     in  : TCHAR* FuncName
//           DWORD Ordinal
//     out :
//     return : TRUE if selected, FALSE else
//-----------------------------------------------------------------------------
BOOL CFunctionsSelectionUI::IsFuncSelected(TCHAR* FuncName,DWORD Ordinal)
{
    if (*FuncName==0)
    {
        TCHAR pszOrdinal[20];
        _stprintf(pszOrdinal,_T("0x%.4X"),Ordinal);
        return IsListviewNameSelected(this->pListViewFunctions,pszOrdinal,0);
    }
    return IsListviewNameSelected(this->pListViewFunctions,FuncName,1);
}

//-----------------------------------------------------------------------------
// Name: IsListviewNameSelected
// Object: check if Name is selected listview pListview
// Parameters :
//     in  : CListview* pListview : listview in which to search
//           TCHAR* Name : name to check selected state in pListview 
//           int SubItemIndex : SubItem index where Name is located
//     out :
//     return : TRUE if selected, FALSE else
//-----------------------------------------------------------------------------
BOOL CFunctionsSelectionUI::IsListviewNameSelected(CListview* pListview,TCHAR* Name,int SubItemIndex)
{
    TCHAR pszListviewName[MAX_PATH];
    // for each item of pListViewLibraries
    for (int cnt=0;cnt<pListview->GetItemCount();cnt++)
    {
        // get text
        pListview->GetItemText(cnt,SubItemIndex,pszListviewName,MAX_PATH);
        // if good lib
        if (_tcscmp(Name,pszListviewName)==0)
            // return lib state
            return pListview->IsItemSelected(cnt);
    }
    // by default return lib as selected one
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SearchFunction
// Object: Called when user combobox input changes
//          Search results throw listview, select first matching item 
//          gives user choice for other matching function names by filling combo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::SearchFunction()
{
    TCHAR FunctionName[2*MAX_PATH];

    TCHAR* SearchedFunction;
    LONG SearchedFunctionLen;

    LONG ItemCount;
    LONG Cnt;
    BOOL FirstMatchingItemSelected;
    BOOL bMatch;

    // hide combo content
    SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)FALSE,0);

    // remove all item of the combo list (don't use CB_RESETCONTENT because it will remove content of edit too)
    ItemCount=(LONG)SendMessage(this->hWndComboSearch,CB_GETCOUNT,0,0);
    if (ItemCount==CB_ERR)
        return;
    for (Cnt=ItemCount-1;Cnt>=0;Cnt--)
    {
        SendMessage(this->hWndComboSearch,CB_DELETESTRING,(WPARAM)Cnt,0);
    }

    // get searched function len 
    SearchedFunctionLen=(LONG)SendMessage(this->hWndComboSearch,WM_GETTEXTLENGTH,0,0);
    if (SearchedFunctionLen<=0)
        return;
    // allocate memory 
    SearchedFunction=new TCHAR[SearchedFunctionLen+3];// 1 for null + 2 for * before and * after name for a wild char search
    SearchedFunction[0]='*';

    // get searched function 
    if (!SendMessage(this->hWndComboSearch,WM_GETTEXT,(WPARAM)(SearchedFunctionLen+1),(LPARAM)(SearchedFunction+1)))
    {
        delete[] SearchedFunction;
        return;
    }
    _tcscat(SearchedFunction,_T("*"));

    // search in function listview 
    ItemCount=this->pListViewFunctions->GetItemCount();
    FirstMatchingItemSelected=FALSE;
    // for each listview item
    for (Cnt=0;Cnt<ItemCount;Cnt++)
    {
        // get description text
        this->pListViewFunctions->GetItemText(Cnt,1,FunctionName,2*MAX_PATH);

        bMatch=FALSE;
        // if function match
        if (CWildCharCompare::WildICmp(SearchedFunction,FunctionName))
            bMatch=TRUE;
        else // get the undecorated name
        {
            this->pListViewFunctions->GetItemText(Cnt,2,FunctionName,2*MAX_PATH);
            if (CWildCharCompare::WildICmp(SearchedFunction,FunctionName))
                bMatch=TRUE;
        }
        if (bMatch)
        {
            // add to combo
            SendMessage(this->hWndComboSearch,CB_ADDSTRING,0,(LPARAM)FunctionName);

            // if first matching item is not selected
            if (!FirstMatchingItemSelected)
            {
                // set single selection style
                LONG_PTR Styles=GetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE);
                SetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

                // select item
                this->pListViewFunctions->SetSelectedIndex(Cnt);
                ListView_EnsureVisible(this->pListViewFunctions->GetControlHandle(),Cnt,FALSE);

                // restore multiple selection
                Styles&=~LVS_SINGLESEL;
                SetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE,Styles);

                FirstMatchingItemSelected=TRUE;
            }
        }
    }

    // if one item at least is found
    if (FirstMatchingItemSelected)
    {
        SetFocus(this->hWndComboSearch);
        // display combo content
        SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)TRUE,0);
    }

    delete[] SearchedFunction;
}

//-----------------------------------------------------------------------------
// Name: SelectSearchedFunctionInListView
// Object: search and select the function selected with the combobox
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFunctionsSelectionUI::SelectSearchedFunctionInListView()
{
    TCHAR* SearchedFunction;
    LONG SearchedFunctionLen;

    TCHAR FunctionName[2*MAX_PATH];

    LONG ItemCount;
    LONG ItemIndex;
    LONG Cnt;
    BOOL bMatch;

    ItemIndex=(LONG)SendMessage(this->hWndComboSearch,CB_GETCURSEL,0,0);
    if (ItemIndex==CB_ERR)
        return;

    // get searched function len 
    SearchedFunctionLen=(LONG)SendMessage(this->hWndComboSearch,CB_GETLBTEXTLEN,ItemIndex,0);
    if (SearchedFunctionLen<=0)
        return;
    // allocate memory 
    SearchedFunction=new TCHAR[SearchedFunctionLen+1];// 1 for null

    // get searched function 
    if (!SendMessage(this->hWndComboSearch,CB_GETLBTEXT,ItemIndex,(LPARAM)SearchedFunction))
    {
        delete[] SearchedFunction;
        return;
    }

    // search in function listview 
    ItemCount=this->pListViewFunctions->GetItemCount();

    // for each listview item
    for (Cnt=0;Cnt<ItemCount;Cnt++)
    {
        // get description text
        this->pListViewFunctions->GetItemText(Cnt,1,FunctionName,2*MAX_PATH);

        // if function match
        bMatch=FALSE;
        // if function match
        if (_tcscmp(SearchedFunction,FunctionName)==0)
            bMatch=TRUE;
        else // get the undecorated name
        {
            this->pListViewFunctions->GetItemText(Cnt,2,FunctionName,2*MAX_PATH);
            if (_tcscmp(SearchedFunction,FunctionName)==0)
                bMatch=TRUE;
        }
        if (bMatch)
        {
            // set single selection style
            LONG_PTR Styles=GetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE);
            SetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

            // select item
            this->pListViewFunctions->SetSelectedIndex(Cnt);
            ListView_EnsureVisible(this->pListViewFunctions->GetControlHandle(),Cnt,FALSE);

            // restore multiple selection
            Styles&=~LVS_SINGLESEL;
            SetWindowLongPtr(this->pListViewFunctions->GetControlHandle(),GWL_STYLE,Styles);

            delete[] SearchedFunction;
            return;
        }
    }
    delete[] SearchedFunction;
}

//-----------------------------------------------------------------------------
// Name: FunctionFinderHelper
// Object: delay help for user
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CFunctionsSelectionUI::FunctionFinderHelper(LPVOID lpParameter)
{
    // reenter object oriented model
    ((CFunctionsSelectionUI*)lpParameter)->FunctionFinderHelper();
    return 0;
}

void CFunctionsSelectionUI::FunctionFinderHelper()
{

    HANDLE ph[2];
    DWORD dwRes;

    ph[0]=this->evtComboTextChange;
    ph[1]=this->evtClose;

    for(;;)
    {
        // wait for an update event or application exit
        dwRes=WaitForMultipleObjects(2,ph,FALSE,INFINITE);
        if (dwRes!=WAIT_OBJECT_0)// stop event or error
        {
            CloseHandle(this->evtClose);
            this->evtClose=NULL;

            CloseHandle(this->evtComboTextChange);
            this->evtComboTextChange=NULL;

            return;
        }
        // until update event are raised in less than FUNCTION_SELECTION_UI_FUNCTION_HELPER_TIME_TO_SHOW ms
        // avoid to combo for each key pressed do it only after user pause
        while (WaitForSingleObject(this->evtComboTextChange,FUNCTION_SELECTION_UI_FUNCTION_HELPER_TIME_TO_SHOW)!=WAIT_TIMEOUT)
        {
            // do nothing (avoid detail combo display for each key pressed)
        }

        if (!this->bClosed)
            // provide choice to user
            this->SearchFunction();

    }
}