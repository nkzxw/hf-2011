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
// Object: manages the search dialog
//-----------------------------------------------------------------------------

#include "search.h"

CSearch::CSearch(void)
{
    this->HexData=NULL;
}

CSearch::~CSearch(void)
{
    if (this->HexData)
        delete this->HexData;
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CSearch::ModelessDialogThread(PVOID lParam)
{
    CSearch* pSearch=(CSearch*)lParam;
    DialogBoxParam (pSearch->hInstance,(LPCTSTR)IDD_DIALOG_SEARCH,NULL,(DLGPROC)CSearch::WndProc,(LPARAM)pSearch);
    delete pSearch;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Show(HINSTANCE hInstance,HWND hWndDialog,CListview* pListview,COptions* pOptions)
{
    // show dialog
    // don't use hWndDialog to allow to put parent window to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    CSearch* pSearch=new CSearch();
    pSearch->pOptions=pOptions;
    pSearch->pListview=pListview;
    pSearch->hInstance=hInstance;
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CSearch::ModelessDialogThread,pSearch,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Init()
{
    this->LoadOptions();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Close()
{
    this->SaveOptions();
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: search dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CSearch::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // init dialog
            CSearch* pSearch;
            pSearch=(CSearch*)lParam;
            pSearch->hWndDialog=hWnd;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pSearch);

            pSearch->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_SEARCH);
           
        }
        break;
    case WM_CLOSE:
        // close dialog
        ((CSearch*)GetWindowLongPtr(hWnd,GWLP_USERDATA))->Close();
        break;
    case WM_COMMAND:
        {
            CSearch* pSearch=((CSearch*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pSearch)
                break;

            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_SEARCH_FIND_FIRST:
                    pSearch->Find();
                    break;
                case IDC_BUTTON_SEARCH_FIND_NEXT:
                    pSearch->FindNext(pSearch->pListview->GetSelectedIndex()+1);
                    break;
                case IDC_BUTTON_SEARCH_FIND_PREVIOUS:
                    pSearch->FindPrevious(pSearch->pListview->GetSelectedIndex()-1);
                    break;
                case IDCANCEL:
                    pSearch->Close();
                    break;
            }
        }
    default:
        return FALSE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: LoadOptions
// Object: load user interface values from pOptions object
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::LoadOptions()
{
    SetDlgItemText(this->hWndDialog,IDC_EDIT_SEARCH_VALUE,this->pOptions->SearchContent);

    if (this->pOptions->SearchAscii)
        CheckDlgButton(this->hWndDialog,IDC_CHECK_SEARCH_ASCII,BST_CHECKED);

    if (this->pOptions->SearchMatchCase)
        CheckDlgButton(this->hWndDialog,IDC_CHECK_SEARCH_MATCH_CASE,BST_CHECKED);

    if (this->pOptions->SearchUnicode)
        CheckDlgButton(this->hWndDialog,IDC_CHECK_SEARCH_UNICODE,BST_CHECKED);

    if (this->pOptions->SearchHex)
        CheckDlgButton(this->hWndDialog,IDC_RADIO_SEARCH_HEX_DATA,BST_CHECKED);
    else
        CheckDlgButton(this->hWndDialog,IDC_RADIO_SEARCH_TEXT,BST_CHECKED);
}

//-----------------------------------------------------------------------------
// Name: SaveOptions
// Object: store user interface values into pOptions object
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::SaveOptions()
{
    GetDlgItemText(this->hWndDialog,IDC_EDIT_SEARCH_VALUE,this->pOptions->SearchContent,MAX_PATH);
    this->pOptions->SearchAscii=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_ASCII)==BST_CHECKED);
    this->pOptions->SearchMatchCase=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_MATCH_CASE)==BST_CHECKED);
    this->pOptions->SearchUnicode=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_UNICODE)==BST_CHECKED);
    this->pOptions->SearchHex=(IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_SEARCH_HEX_DATA)==BST_CHECKED);
}

//-----------------------------------------------------------------------------
// Name: UpdateSearchFields
// Object: update class members from user interface
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CSearch::UpdateSearchFields()
{
    GetDlgItemText(this->hWndDialog,IDC_EDIT_SEARCH_VALUE,this->SearchContent,MAX_PATH);
    
    this->SearchAscii=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_ASCII)==BST_CHECKED);
    this->SearchMatchCase=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_MATCH_CASE)==BST_CHECKED);
    this->SearchUnicode=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_UNICODE)==BST_CHECKED);
    this->SearchHex=(IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_SEARCH_HEX_DATA)==BST_CHECKED);
    if (this->SearchHex)
    {
        if (this->HexData)
            delete this->HexData;
        this->HexData=CStrToHex::StrHexArrayToByteArray(this->SearchContent,&this->HexDataSize);
    }
    else
    {
        GetDlgItemTextA(this->hWndDialog,IDC_EDIT_SEARCH_VALUE,this->AsciiSearchContent,MAX_PATH);
        GetDlgItemTextW(this->hWndDialog,IDC_EDIT_SEARCH_VALUE,this->UnicodeSearchContent,MAX_PATH);
        AsciiSearchContentSize=strlen(this->AsciiSearchContent);
        UnicodeSearchContentSize=wcslen(this->UnicodeSearchContent)*sizeof(wchar_t);
        if (!this->SearchMatchCase)
        {
            strupr(this->AsciiSearchContent);
            wcsupr(this->UnicodeSearchContent);
        }
    }
    // if text search, and ascii and unicode are not checked
    if ((!this->SearchHex)
        && (this->SearchAscii||this->SearchUnicode)==FALSE
        )
    {
        // default to ascii text
        this->SearchAscii=TRUE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: NoItemFoundMessage
// Object: show a Not Found Item Message box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::NoItemFoundMessage()
{
    MessageBox(this->hWndDialog,_T("No Item Found"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
}
//-----------------------------------------------------------------------------
// Name: Find
// Object: find first item matching conditions
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Find()
{
    this->FindNext(0);
}
//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the next item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::FindNext(int StartItemIndex)
{
    // if we are at the end of the list
    if (StartItemIndex>=this->pListview->GetItemCount())
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
    LONG_PTR Styles=GetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for next matching item in listview
    for(int cnt=StartItemIndex;cnt<this->pListview->GetItemCount();cnt++)
    {
        // stop if item matches
        if (this->DoesItemMatch(cnt))
        {
            // select item
            this->pListview->SetSelectedIndex(cnt);

            // restore style
            SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles);

            // Set Focus to Main dialog
            SetActiveWindow(GetParent(this->hWndDialog));
            SetFocus(this->pListview->GetControlHandle());

            // assume item is visible
            this->pListview->ScrollTo(cnt);

            return;
        }
    }
    // restore style
    SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles);
    // if no item found
    this->NoItemFoundMessage();
}
//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the previous item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::FindPrevious(int StartItemIndex)
{
    // if we are at the begin of the list
    if (StartItemIndex<0)
    {
        // we can't find more item
        this->NoItemFoundMessage();
        return;
    }
    
    // StartItemIndex check
    if (StartItemIndex>this->pListview->GetItemCount()-1)
        StartItemIndex=this->pListview->GetItemCount()-1;

    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;


    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for previous matching item in listview
    for(int cnt=StartItemIndex;cnt>=0;cnt--)
    {
        // stop if item matches
        if (this->DoesItemMatch(cnt))
        {
            // select item
            this->pListview->SetSelectedIndex(cnt);

            // restore style
            SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles);
            // Set Focus to Main dialog
            SetActiveWindow(GetParent(this->hWndDialog));
            SetFocus(this->pListview->GetControlHandle());

            // assume item is visible
            this->pListview->ScrollTo(cnt);

            return;
        }
    }

    // restore style
    SetWindowLongPtr(this->pListview->GetControlHandle(),GWL_STYLE,Styles);

    // if no item found
    this->NoItemFoundMessage();
}


//-----------------------------------------------------------------------------
// Name: FindBufferInBuffer
// Object: search SearchedBuffer inside Buffer
// Parameters :
//     in  : PBYTE SearchedBuffer : searched buffer
//           DWORD SearchedBufferSize : searched buffer size
//           PBYTE Buffer : buffer into search data
//           DWORD BufferSize : buffer size
//     out :
//     return : TRUE if has been found, FALSE else
//-----------------------------------------------------------------------------
BOOL CSearch::FindBufferInBuffer(PBYTE SearchedBuffer, DWORD SearchedBufferSize, PBYTE Buffer, DWORD BufferSize)
{
    if (SearchedBufferSize>BufferSize)
        return FALSE;

    DWORD Cnt;
    DWORD Cnt2;

    // search SearchedBuffer inside Buffer
    for (Cnt=0;Cnt+SearchedBufferSize<BufferSize;Cnt++)
    {
        // for each byte of SearchedBuffer
        for (Cnt2=0;Cnt2<SearchedBufferSize;Cnt2++)
        {
            // if byte differs
            if (Buffer[Cnt+Cnt2]!=SearchedBuffer[Cnt2])
                break;
        }
        // if all SearchedBuffer bytes match
        if (Cnt2==SearchedBufferSize)
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: DoesItemMatch
// Object: check if item with index ItemIndex in main dialog list view match the filters
//          and if item match, select it
// Parameters :
//     in  : int ItemIndex : item index of dialog main this->pListview
//     out :
//     return : TRUE if item match, FALSE else
//-----------------------------------------------------------------------------
BOOL CSearch::DoesItemMatch(int ItemIndex)
{
    HEAP_CONTENT* pHeapContent;
    // get Heap content

    if(!this->pListview->GetItemUserData(ItemIndex,(LPVOID*)(&pHeapContent)))
        return FALSE;
    if (pHeapContent==0)
        return FALSE;
    if (IsBadReadPtr(pHeapContent,sizeof(HEAP_CONTENT)))
        return FALSE;
    if ((pHeapContent->HeapEntry.dwBlockSize==0)||(pHeapContent->pData==NULL))
        return FALSE;
    if (IsBadReadPtr(pHeapContent->pData,pHeapContent->HeapEntry.dwBlockSize))
        return FALSE;
   
    // if hex search
    if (this->SearchHex)
    {
        // search HexData into pHeapContent->pData
        return this->FindBufferInBuffer(this->HexData,
                                        this->HexDataSize,
                                        pHeapContent->pData,
                                        pHeapContent->HeapEntry.dwBlockSize
                                        );
    }
    else
    // if text search
    {
        if (this->SearchAscii)
        {
            char* pc;
            if (this->SearchMatchCase)
            {
                pc=(char*)pHeapContent->pData;
            }
            // if insensitive search
            else
            {
                // convert buffer to upper case
                pc=new char[pHeapContent->HeapEntry.dwBlockSize+1];
                if (!pc)
                    return FALSE;
                pc[pHeapContent->HeapEntry.dwBlockSize]=0;
                memcpy(pc,pHeapContent->pData,pHeapContent->HeapEntry.dwBlockSize);
                CharUpperBuffA(pc,pHeapContent->HeapEntry.dwBlockSize);
            }

            // search buffer
            if (this->FindBufferInBuffer((PBYTE)this->AsciiSearchContent,
                                        this->AsciiSearchContentSize,
                                        (PBYTE)pc,
                                        pHeapContent->HeapEntry.dwBlockSize
                                        )
                )
            {
                if (!this->SearchMatchCase)
                    delete pc;
                return TRUE;
            }
            if (!this->SearchMatchCase)
               delete pc;
        }


        if (this->SearchUnicode)
        {
            wchar_t* pc;
            
            if (this->SearchMatchCase)
            {
                pc=(wchar_t*)pHeapContent->pData;
            }
            // if insensitive search
            else
            {
                // convert buffer to upper case
                pc=new wchar_t[pHeapContent->HeapEntry.dwBlockSize/sizeof(wchar_t)+1];
                if (!pc)
                    return FALSE;
                pc[pHeapContent->HeapEntry.dwBlockSize/sizeof(wchar_t)]=0;
                memcpy(pc,pHeapContent->pData,pHeapContent->HeapEntry.dwBlockSize/sizeof(wchar_t));
                CharUpperBuffW(pc,pHeapContent->HeapEntry.dwBlockSize/sizeof(wchar_t));
            }

            // search buffer
            if (this->FindBufferInBuffer((PBYTE)this->UnicodeSearchContent,
                                        this->UnicodeSearchContentSize,
                                        (PBYTE)pc,
                                        pHeapContent->HeapEntry.dwBlockSize
                                        )
               )
            {
                if (!this->SearchMatchCase)
                    delete pc;
                return TRUE;
            }
            if (!this->SearchMatchCase)
                delete pc;
        }

        return FALSE;
    }
}