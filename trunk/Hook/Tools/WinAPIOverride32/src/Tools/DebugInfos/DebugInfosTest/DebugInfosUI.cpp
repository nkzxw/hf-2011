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
// Object: manages the DebugInfos dialog
//-----------------------------------------------------------------------------
#include "debuginfosui.h"
#include "../../LinkList/LinkListTemplate.cpp"

CDebugInfosUI::CDebugInfosUI(void)
{
    this->pTreeview=NULL;
    this->DialogResult=0;
    this->hMonitoringFile=INVALID_HANDLE_VALUE;
    this->hWndDialog=NULL;
    this->hInstance=NULL;
    this->hParentHandle=NULL;
    this->pToolbar=NULL;
    this->pDebugInfos=NULL;
    this->pPE=NULL;
    this->FileToMonitorType=FileToMonitorType_EXE;
    this->pToolbar=NULL;
    this->pMenuExport=NULL;
    this->MenuExportExpandedOnlyId=0;
    this->MenuExportSelectedOnlyId=0;
    this->IconObjIndex=0;
    this->IconFunctionIndex=0;
    this->IconParamIndex=0;
    this->IconLocalIndex=0;
    this->IconThunkIndex=0;

}

CDebugInfosUI::~CDebugInfosUI(void)
{
    this->FreeMemory();
}

void CDebugInfosUI::FreeMemory()
{
    // free previous debug info if any
    if (this->pDebugInfos)
    {
        delete this->pDebugInfos;
        this->pDebugInfos=NULL;
    }

    // free previous pe if any 
    if (this->pPE)
    {
        delete this->pPE;
        this->pPE=NULL;
    }
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
INT_PTR CDebugInfosUI::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    return CDebugInfosUI::Show(hInstance,hWndDialog,NULL);
}
//-----------------------------------------------------------------------------
// Name: Show
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//           TCHAR* pszFile : file to parse at startup
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CDebugInfosUI::Show(HINSTANCE hInstance,HWND hWndDialog,TCHAR* pszFile)
{
    INT_PTR Ret;
    CDebugInfosUI* pDebugInfosUI=new CDebugInfosUI();
    pDebugInfosUI->hInstance=hInstance;
    pDebugInfosUI->hParentHandle=hWndDialog;
    pDebugInfosUI->pszFileToParseAtStartup=pszFile;

    Ret=DialogBoxParam(pDebugInfosUI->hInstance,(LPCTSTR)IDD_DIALOG_DEBUG_INFOS,NULL,(DLGPROC)CDebugInfosUI::WndProc,(LPARAM)pDebugInfosUI);

    delete pDebugInfosUI;
    return Ret;
}

void CDebugInfosUI::VariantToString(VARIANT var,TCHAR* psz,DWORD pszMaxSize)
{
    switch(var.vt)
    {
    case VT_UI1:
    case VT_I1:
        _sntprintf(psz,pszMaxSize,_T("0x%x"), var.bVal);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        _sntprintf(psz,pszMaxSize,_T("0x%x"), var.iVal);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        _sntprintf(psz,pszMaxSize,_T("0x%x"), var.lVal);
        break;
    case VT_R4:
        _sntprintf(psz,pszMaxSize,_T("%f"), var.fltVal);
        break;
    case VT_R8:
        _sntprintf(psz,pszMaxSize,_T("%dn"), var.dblVal);
        break;
    case VT_BSTR:
#if (defined(UNICODE)||defined(_UNICODE))
            _tcsncpy(psz, var.bstrVal ,pszMaxSize);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(var.bstrVal,psz,pszMaxSize);
#endif
        break;
    default:
        _tcsncpy(psz,_T("?"),pszMaxSize);
    }
}
BOOL CDebugInfosUI::CollapseModulesParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->CollapseExpandParseCallBack(hItem,TRUE);
    // continue parsing
    return TRUE;
}
BOOL CDebugInfosUI::ExpandModulesParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->CollapseExpandParseCallBack(hItem,FALSE);
    // continue parsing
    return TRUE;
}
void CDebugInfosUI::CollapseExpandParseCallBack(HTREEITEM hItem,BOOL bCollapse)
{
    if (bCollapse)
    {
        this->pTreeview->Collapse(hItem,FALSE);
    }
    else
    {
        this->pTreeview->Expand(hItem,FALSE);
    }
}
void CDebugInfosUI::OnCollapseModules()
{
    this->SetWaitCursor(TRUE);
    this->pTreeview->ParseSpecifiedDepth(
                                        this->pTreeview->GetRoot(),
                                        0,
                                        CDebugInfosUI::CollapseModulesParseCallBackStatic,
                                        this
                                        );
    this->SetWaitCursor(FALSE);
}
void CDebugInfosUI::OnExpandModules()
{
    this->SetWaitCursor(TRUE);
    this->pTreeview->ParseSpecifiedDepth(
                                        this->pTreeview->GetRoot(),
                                        0,
                                        CDebugInfosUI::ExpandModulesParseCallBackStatic,
                                        this
                                        );
    this->SetWaitCursor(FALSE);
}

//-----------------------------------------------------------------------------
// Name: DisplayStaticLocation
// Object: display a static location of a debug item into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::DisplayStaticLocation(HTREEITEM hParentItem,DWORD SectionIndex,DWORD Offset,ULONGLONG RelativeVirtualAddress)
{
    TCHAR sz[MAX_PATH];
    HTREEITEM hTreeItem;
    _stprintf(sz,_T("Section Index : %u"),SectionIndex);
    hTreeItem=this->pTreeview->Insert(hParentItem,sz);
    this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

    if (this->pPE && SectionIndex)
    {
        if (SectionIndex<=this->pPE->NTHeader.FileHeader.NumberOfSections)
        {
            _stprintf(sz,_T("Raw Address : 0x%.8X (0x%.8X + 0x%.8X)"),
                Offset+this->pPE->pSectionHeaders[SectionIndex-1].PointerToRawData,
                this->pPE->pSectionHeaders[SectionIndex-1].PointerToRawData, // SectionIndex is 1 based, and pPE->pSectionHeaders is 0 based
                Offset);
        }
        else
        {
            _stprintf(sz,_T("Offset : 0x%.8X"),Offset);
        }

    }
    else
    {
        _stprintf(sz,_T("Offset : 0x%.8X"),Offset);
    }
    hTreeItem=this->pTreeview->Insert(hParentItem,sz);
    this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

    if (this->pPE)
    {
        if (this->pPE->Is64Bits())
        {
            _stprintf(sz,_T("Relative Virtual Address : 0x%I64X"),RelativeVirtualAddress);
            hTreeItem=this->pTreeview->Insert(hParentItem,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            _stprintf(sz,_T("Start Address (VA) : 0x%I64X"),RelativeVirtualAddress+this->pPE->NTHeader.OptionalHeader.ImageBase);
            hTreeItem=this->pTreeview->Insert(hParentItem,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        }
        else
        {
            _stprintf(sz,_T("Relative Virtual Address : 0x%I32X"),(DWORD)RelativeVirtualAddress);
            hTreeItem=this->pTreeview->Insert(hParentItem,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            _stprintf(sz,_T("Start Address (VA) : 0x%I32X"),(DWORD)(RelativeVirtualAddress+this->pPE->NTHeader.OptionalHeader.ImageBase));
            hTreeItem=this->pTreeview->Insert(hParentItem,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        }
    }
    else
    {
        _stprintf(sz,_T("Relative Virtual Address : 0x%I64X"),RelativeVirtualAddress);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
    }
}

//-----------------------------------------------------------------------------
// Name: DisplayLocation
// Object: display location of a debug item into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CDebugInfosUI::DisplayLocation(HTREEITEM hParentItem,CSymbolLocation* pSymbolLocation)
{
    TCHAR sz[MAX_PATH];
    TCHAR szRegisterName[MAX_PATH];
    HTREEITEM hTreeItem;
    if (!pSymbolLocation)
        return FALSE;
    // insert type string
    hTreeItem=this->pTreeview->Insert(hParentItem,pSymbolLocation->GetTypeString());
    this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

    switch(pSymbolLocation->SymbolLocationType)
    {
    case LocIsStatic: 
        this->DisplayStaticLocation(hParentItem,pSymbolLocation->SectionIndex,pSymbolLocation->Offset,pSymbolLocation->RelativeVirtualAddress);
        break;
    case LocIsTLS:
    case LocInMetaData:
    case LocIsIlRel:
        _stprintf(sz,_T("Section Index : %u"),pSymbolLocation->SectionIndex);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        _stprintf(sz,_T("Offset : 0x%.8X"),pSymbolLocation->Offset);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsRegRel:
        pSymbolLocation->GetRegisterName(szRegisterName,MAX_PATH);
        if (pSymbolLocation->RelativeOffset>0)
            _stprintf(sz,_T("%s + 0x%X"),szRegisterName,pSymbolLocation->RelativeOffset);
        else
            _stprintf(sz,_T("%s - 0x%X"),szRegisterName,-pSymbolLocation->RelativeOffset);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsThisRel:
        if (pSymbolLocation->RelativeOffset>0)
            _stprintf(sz,_T("this + 0x%X"),pSymbolLocation->RelativeOffset);
        else
            _stprintf(sz,_T("this - 0x%X"),-pSymbolLocation->RelativeOffset);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsBitField:
        if (pSymbolLocation->RelativeOffset>0)
            _stprintf(sz,_T("this + 0x%X"),pSymbolLocation->RelativeOffset);
        else
            _stprintf(sz,_T("this - 0x%X"),-pSymbolLocation->RelativeOffset);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        _stprintf(sz,_T("Bit Field Position : 0x%.8X"),pSymbolLocation->BitFieldPos);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        _stprintf(sz,_T("Bit Field Length : %u"),pSymbolLocation->BitFieldLen);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsEnregistered:
        pSymbolLocation->GetRegisterName(szRegisterName,MAX_PATH);
        hTreeItem=this->pTreeview->Insert(hParentItem,szRegisterName);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsNull:
        break;
    case LocIsSlot:
        _stprintf(sz,_T("Slot : 0x%.8X"),pSymbolLocation->Slot);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    case LocIsConstant:
        this->VariantToString(pSymbolLocation->ConstantValue,sz,MAX_PATH);
        hTreeItem=this->pTreeview->Insert(hParentItem,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
        break;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InsertBlocks
// Object: insert blocks info into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::InsertBlocks(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListBlocks)
{
    // if no blocks
    if (!pLinkListBlocks->Head)
        return;

    HTREEITEM hTreeItemBlocks;
    HTREEITEM hTreeItemBlock;
    HTREEITEM hTreeItemLocals;
    HTREEITEM hTreeItemVar;
    HTREEITEM hTreeItem;
    CLinkListItem* pItemBlock;
    CFunctionBlockInfos* pBlockInfos;
    CLinkListItem* pItemVar;
    CTypeInfos* pTypeInfos;
    TCHAR sz[2*MAX_PATH];
    DWORD Cnt;
    BOOL bDisplayStaticLocation;

    hTreeItemBlocks=this->pTreeview->Insert(hTreeItemParent,_T("Blocks"));
    this->pTreeview->SetCheckedState(hTreeItemBlocks,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

    // for each block
    for (pItemBlock=pLinkListBlocks->Head,Cnt=1;pItemBlock;pItemBlock=pItemBlock->NextItem,Cnt++)
    {
        pBlockInfos=(CFunctionBlockInfos*)pItemBlock->ItemData;

        _stprintf(sz,_T("Block %u"),Cnt);
        hTreeItemBlock=this->pTreeview->Insert(hTreeItemBlocks,sz);
        this->pTreeview->SetCheckedState(hTreeItemBlock,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

        bDisplayStaticLocation=TRUE;
        if (this->pPE)
        {
            if (this->pPE->IsNET())
                bDisplayStaticLocation=FALSE;
        }
        if (pBlockInfos->Offset!=0)
            bDisplayStaticLocation=TRUE;

        if (bDisplayStaticLocation)
            this->DisplayStaticLocation(hTreeItemBlock,pBlockInfos->SectionIndex,pBlockInfos->Offset,pBlockInfos->RelativeVirtualAddress);

        _stprintf(sz,_T("Length : %u"),pBlockInfos->Length);
        hTreeItem=this->pTreeview->Insert(hTreeItemBlock,sz);
        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

        // add locals
        if (pBlockInfos->pLinkListVars->Head)
        {
            _stprintf(sz,_T("Locals"));
            hTreeItemLocals=this->pTreeview->Insert(hTreeItemBlock,sz);
            this->pTreeview->SetCheckedState(hTreeItemLocals,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            // for each local
            for (pItemVar=pBlockInfos->pLinkListVars->Head;pItemVar;pItemVar=pItemVar->NextItem)
            {
                pTypeInfos=(CTypeInfos*)pItemVar->ItemData;
                pTypeInfos->GetPrettyName(sz,2*MAX_PATH);

                // add parameter
                hTreeItemVar=this->pTreeview->Insert(hTreeItemLocals,sz);
                this->pTreeview->SetCheckedState(hTreeItemVar,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                this->pTreeview->SetBoldState(hTreeItemVar,TRUE);
                this->pTreeview->SetItemIconIndex(hTreeItemVar,this->IconLocalIndex);
                switch (pTypeInfos->DataKind)
                {
                case SymTagUsingNamespace:
                    break;
                default:

                    if (!pTypeInfos->pSymbolLocation)
                        break;
                    // add param size
                    if (pTypeInfos->PointerLevel>0)
                        _stprintf(sz,_T("Pointed Type Size : %u"),pTypeInfos->Size);
                    else
                        _stprintf(sz,_T("Type Size : %u"),pTypeInfos->Size);
                    hTreeItem=this->pTreeview->Insert(hTreeItemVar,sz);
                    this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

                    // add param address
                    this->DisplayLocation(hTreeItemVar,pTypeInfos->pSymbolLocation);
                    break;
                }
            }
        }

        // add subblocks
        this->InsertBlocks(hTreeItemBlock,pBlockInfos->pLinkListBlocks);
    }
}

//-----------------------------------------------------------------------------
// Name: InsertLabels
// Object: insert labels info into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::InsertLabels(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListLabels)
{
    HTREEITEM hTreeItemLabels;
    HTREEITEM hTreeItemLabel;
    CLinkListItem* pItemLabel;
    CLabelInfos* pLabelInfos;

    if (pLinkListLabels->Head)
    {
        hTreeItemLabels=this->pTreeview->Insert(hTreeItemParent,_T("Labels"));
        this->pTreeview->SetCheckedState(hTreeItemLabels,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

        // for each label
        for (pItemLabel=pLinkListLabels->Head;pItemLabel;pItemLabel=pItemLabel->NextItem)
        {
            pLabelInfos=(CLabelInfos*)pItemLabel->ItemData;

            hTreeItemLabel=this->pTreeview->Insert(hTreeItemLabels,pLabelInfos->Name);
            this->pTreeview->SetCheckedState(hTreeItemLabel,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
            this->pTreeview->SetBoldState(hTreeItemLabel,TRUE);

            this->DisplayStaticLocation(hTreeItemLabel,pLabelInfos->SectionIndex,pLabelInfos->Offset,pLabelInfos->RelativeVirtualAddress);

        }
    }
}
//-----------------------------------------------------------------------------
// Name: InsertThunks
// Object: insert thunks info into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::InsertThunks(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListThunks)
{
    TCHAR sz[2*MAX_PATH];
    HTREEITEM hTreeItemThunks;
    HTREEITEM hTreeItemThunk;
    HTREEITEM hTreeItem;
    CLinkListItem* pItemThunk;
    CThunkInfos* pThunkInfos;

    if (pLinkListThunks->Head)
    {
        hTreeItemThunks=this->pTreeview->Insert(hTreeItemParent,_T("Thunks"));
        this->pTreeview->SetCheckedState(hTreeItemThunks,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

        // for each thunk
        for (pItemThunk=pLinkListThunks->Head;pItemThunk;pItemThunk=pItemThunk->NextItem)
        {
            pThunkInfos=(CThunkInfos*)pItemThunk->ItemData;

            hTreeItemThunk=this->pTreeview->Insert(hTreeItemThunks,pThunkInfos->Name);
            this->pTreeview->SetCheckedState(hTreeItemThunk,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
            this->pTreeview->SetBoldState(hTreeItemThunk,TRUE);
            this->pTreeview->SetItemIconIndex(hTreeItemThunk,this->IconThunkIndex);

            hTreeItem=this->pTreeview->Insert(hTreeItemThunk,pThunkInfos->GetOrdinalString());
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            _stprintf(sz,_T("Section Index : %u"),pThunkInfos->SectionIndex);
            hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            if (this->pPE && pThunkInfos->SectionIndex)
            {
                if (pThunkInfos->SectionIndex<=this->pPE->NTHeader.FileHeader.NumberOfSections)
                {
                    _stprintf(sz,_T("Raw Address : 0x%.8X (0x%.8X + 0x%.8X)"),
                        pThunkInfos->Offset+this->pPE->pSectionHeaders[pThunkInfos->SectionIndex-1].PointerToRawData,
                        this->pPE->pSectionHeaders[pThunkInfos->SectionIndex-1].PointerToRawData, // pSymbolLocation->SectionIndex is 1 based, and pPE->pSectionHeaders is 0 based
                        pThunkInfos->Offset);
                }
                else
                {
                    _stprintf(sz,_T("Offset : 0x%.8X"),pThunkInfos->Offset);
                }

            }
            else
            {
                _stprintf(sz,_T("Offset : 0x%.8X"),pThunkInfos->Offset);
            }
            hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            _stprintf(sz,_T("Length : %u"),pThunkInfos->Length);
            hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
            this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

            if (pThunkInfos->TargetOffset)
            {
                _stprintf(sz,_T("Target Section Index : %u"),pThunkInfos->TargetSectionIndex);
                hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
                this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                _stprintf(sz,_T("Target Offset : 0x%.8X"),pThunkInfos->TargetOffset);
                hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
                this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                _stprintf(sz,_T("Target Relative Virtual Address : 0x%p"),pThunkInfos->TargetRelativeVirtualAddress);
                hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
                this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

                if (this->pPE)
                {
                    _stprintf(sz,_T("Start Address (VA) : 0x%p"),pThunkInfos->TargetRelativeVirtualAddress+this->pPE->NTHeader.OptionalHeader.ImageBase);
                    hTreeItem=this->pTreeview->Insert(hTreeItemThunk,sz);
                    this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------
// Name: InsertDebugInfos
// Object: insert debug info into treeview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::InsertDebugInfos()
{
    CLinkListItem* pItemModule;
    CModuleInfos* pModuleInfos;
    CLinkListItem* pItemFunction;
    CFunctionInfos* pFunctionInfos;
    CLinkListItem* pItemParam;
    CTypeInfos* pTypeInfos;

    HTREEITEM hTreeItem;
    HTREEITEM hTreeItemModule;
    HTREEITEM hTreeItemFunction;
    HTREEITEM hTreeItemParameters;
    HTREEITEM hTreeItemParam;
    TCHAR sz[2*MAX_PATH];

    // clear previous content if any
    this->pTreeview->Clear();

    // for each module
    for (pItemModule=this->pDebugInfos->pLinkListModules->Head;pItemModule;pItemModule=pItemModule->NextItem)
    {
        pModuleInfos=(CModuleInfos*)pItemModule->ItemData;
        if (pModuleInfos->Name)
        {
            hTreeItemModule=this->pTreeview->Insert(NULL,pModuleInfos->Name);
        }
        else
            hTreeItemModule=this->pTreeview->Insert(NULL,_T(""));

        this->pTreeview->SetItemIconIndex(hTreeItemModule,this->IconObjIndex);

        // for each function
        for (pItemFunction=pModuleInfos->pLinkListFunctions->Head;pItemFunction;pItemFunction=pItemFunction->NextItem)
        {
            pFunctionInfos=(CFunctionInfos*)pItemFunction->ItemData;

            if (pFunctionInfos->UndecoratedName)
            {
                hTreeItemFunction=this->pTreeview->Insert(hTreeItemModule,pFunctionInfos->UndecoratedName);
            }
            else if (pFunctionInfos->Name)
            {
                hTreeItemFunction=this->pTreeview->Insert(hTreeItemModule,pFunctionInfos->Name);
            }
            else
                hTreeItemFunction=this->pTreeview->Insert(hTreeItemModule,_T(""));

            // set function node user data
            this->pTreeview->SetItemUserData(hTreeItemFunction,pFunctionInfos);
            // add bold state
            this->pTreeview->SetBoldState(hTreeItemFunction,TRUE);
            // add icon
            this->pTreeview->SetItemIconIndex(hTreeItemFunction,this->IconFunctionIndex);

            if (pFunctionInfos->Token)
            {
                _stprintf(sz,_T("Token : 0x%.8X"),pFunctionInfos->Token);
                hTreeItem=this->pTreeview->Insert(hTreeItemFunction,sz);
                this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
            }
            else
            {
                this->DisplayStaticLocation(hTreeItemFunction,pFunctionInfos->SectionIndex,pFunctionInfos->Offset,pFunctionInfos->RelativeVirtualAddress);

                _stprintf(sz,_T("Length : %u"),pFunctionInfos->Length);
                hTreeItem=this->pTreeview->Insert(hTreeItemFunction,sz);
                this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
            }

            // if function has parameters
            if (pFunctionInfos->pLinkListParams->Head)
            {
                hTreeItemParameters=this->pTreeview->Insert(hTreeItemFunction,_T("Parameters"));
                this->pTreeview->SetCheckedState(hTreeItemParameters,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);

                // for each parameter
                for (pItemParam=pFunctionInfos->pLinkListParams->Head;pItemParam;pItemParam=pItemParam->NextItem)
                {
                    pTypeInfos=(CTypeInfos*)pItemParam->ItemData;

                    pTypeInfos->GetPrettyName(sz,2*MAX_PATH);

                    // add parameter
                    hTreeItemParam=this->pTreeview->Insert(hTreeItemParameters,sz);
                    this->pTreeview->SetCheckedState(hTreeItemParam,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                    this->pTreeview->SetBoldState(hTreeItemParam,TRUE);
                    this->pTreeview->SetItemIconIndex(hTreeItemParam,this->IconParamIndex);

                    // add param size
                    if (pTypeInfos->Size>0)
                    {
                        if (pTypeInfos->PointerLevel>0)
                            _stprintf(sz,_T("Pointed Type Size : %u"),pTypeInfos->Size);
                        else
                            _stprintf(sz,_T("Type Size : %u"),pTypeInfos->Size);
                        hTreeItem=this->pTreeview->Insert(hTreeItemParam,sz);
                        this->pTreeview->SetCheckedState(hTreeItem,CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE);
                    }

                    this->DisplayLocation(hTreeItemParam,pTypeInfos->pSymbolLocation);
                }
            }

            // insert labels
            this->InsertLabels(hTreeItemFunction,pFunctionInfos->pLinkListLabels);

            // for block list
            this->InsertBlocks(hTreeItemFunction,pFunctionInfos->pLinkListBlocks);
        }

        // insert module labels
        this->InsertLabels(hTreeItemModule,pModuleInfos->pLinkListLabels);

        // insert module thunks
        this->InsertThunks(hTreeItemModule,pModuleInfos->pLinkListThunks);

    }
}

BOOL CDebugInfosUI::OpenFile()
{
    TCHAR FileName[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("exe, dll, ocx, pdb\0*.exe;*.dll;*.ocx;*.pdb\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt=_T("exe");
    ofn.lpstrFile=FileName;
    ofn.nMaxFile=MAX_PATH;

    if (!GetOpenFileName(&ofn))
        return FALSE;

    return this->OpenFile(FileName);
}

BOOL CDebugInfosUI::OpenFile(TCHAR* FileName)
{
    TCHAR sz[2*MAX_PATH];

    // update dialog caption
    _stprintf(sz,_T("%s : %s"),APPLICATION_NAME,CStdFileOperations::GetFileName(FileName));
    SetWindowText(this->hWndDialog,sz);

    // check if file exists
    if (!CStdFileOperations::DoesFileExists(FileName))
    {
        _stprintf(sz,_T("Error file %s doesn't exists"),FileName);
        this->ReportError(sz);
        return FALSE;
    }
    // free previous allocated memory
    this->FreeMemory();

    // create an new debug info showing error message
    this->pDebugInfos=new CDebugInfos(FileName,TRUE);

    // get file extension
    TCHAR* FileExt=CStdFileOperations::GetFileExt(FileName);
    // if pdb --> no pseudo header (and so no section address)
    if (_tcsicmp(FileExt,_T("pdb"))==0)
    {
        this->pPE=NULL;
    }
    else
    {
        // parse pseudo header (allow to no section address)
        this->pPE=new CPE();
        if (!this->pPE->Parse(FileName,FALSE,FALSE))
        {
            this->ReportError(_T("Error parsing PE"));
            delete this->pPE;
            this->pPE=NULL;
            return FALSE;
        }
    }

    this->SetWaitCursor(TRUE);

    // parse debug infos
    this->pDebugInfos->Parse();

    // insert debug infos into treeview
    this->InsertDebugInfos();

    this->SetWaitCursor(FALSE);

    return TRUE;
}
BOOL CDebugInfosUI::CloseFile()
{
    if (this->pDebugInfos)
    {
        delete this->pDebugInfos;
        this->pDebugInfos=NULL;
    }
    if (this->pPE)
    {
        delete this->pPE;
        this->pPE=NULL;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ToolBarDropDownMenuCallBackStatic
// Object: called on toolbar drop down menu click
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::ToolBarDropDownMenuCallBackStatic(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam)
{
    // reenter object model
    ((CDebugInfosUI*)UserParam)->ToolBarDropDownMenuCallBack(PopUpMenu,MenuId);
}
void CDebugInfosUI::ToolBarDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId)
{
    if (PopUpMenu==this->pMenuExport)
    {
        BOOL bChecked;
        if (MenuId==this->MenuExportExpandedOnlyId)
        {
            bChecked=this->pMenuExport->IsChecked(this->MenuExportExpandedOnlyId);
            bChecked=!bChecked;// new state
            this->pMenuExport->SetCheckedState(this->MenuExportExpandedOnlyId,bChecked);
        }
        else if (MenuId==this->MenuExportSelectedOnlyId)
        {
            bChecked=this->pMenuExport->IsChecked(this->MenuExportSelectedOnlyId);
            bChecked=!bChecked;// new state
            this->pMenuExport->SetCheckedState(this->MenuExportSelectedOnlyId,bChecked);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::Init()
{
    CAPIError::SetParentWindowHandle(this->hWndDialog);
    this->pToolbar=new CToolbar(this->hInstance,this->hWndDialog,TRUE,TRUE,24,24);
    this->pToolbar->AddButton(IDC_BUTTON_OPEN,_T("Open"),IDI_ICON_OPEN,_T("Open"));
    this->pToolbar->SetDropDownMenuCallBack(CDebugInfosUI::ToolBarDropDownMenuCallBackStatic,this);
    this->pMenuExport=new CPopUpMenu();
    this->MenuExportExpandedOnlyId=this->pMenuExport->Add(_T("Expanded Only"));
    this->pMenuExport->SetCheckedState(this->MenuExportExpandedOnlyId,TRUE);
    this->MenuExportSelectedOnlyId=this->pMenuExport->Add(_T("Selected Only"));
    this->pMenuExport->SetCheckedState(this->MenuExportSelectedOnlyId,TRUE);
    this->pToolbar->AddDropDownButton(IDC_BUTTON_EXPORT,_T("Export"),IDI_ICON_EXPORT,_T("Export"),this->pMenuExport,FALSE);
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_EXPAND_MODULES,IDI_ICON_EXPAND,_T("Expand Modules"));
    this->pToolbar->AddButton(IDC_BUTTON_COLLAPSE_MODULES,IDI_ICON_COLLAPSE,_T("Collapse Modules"));

    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_CHECK_PROJECT_SPECIFIC,IDI_ICON_CHECK_PROJECT_SPECIFIC,_T("Check Project Specific (Experimental)"));
    this->pToolbar->AddButton(IDC_BUTTON_CHECK_ALL,IDI_ICON_CHECK_ALL,_T("Check All (No recommanded: can produce large amount of logs)"));
    this->pToolbar->AddButton(IDC_BUTTON_UNCHECK_ALL,IDI_ICON_UNCHECK_ALL,_T("Unchek All"));

    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_SHOW_ASM,IDI_ICON_SHOW_ASM,_T("Show Selected Function ASM"));
    this->pToolbar->AddButton(IDC_BUTTON_GENERATE,_T("Generate"),IDI_ICON_GENERATE,_T("Generate Monitoring File For Checked Items"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_ABOUT,IDI_ICON_ABOUT,_T("About"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_DONATION,IDI_ICON_DONATION,_T("Make Donation"));

    // create a listview object associated to listview
    this->pTreeview=new CTreeview(GetDlgItem(this->hWndDialog,IDC_TREE_DEBUG_INFOS),TRUE);
    this->pTreeview->SetCheckedStateChangedCallback(CDebugInfosUI::OnTreeViewCheckedStateChangedStatic,this);
    // add an empty icon at index 0 for item having no icon (icon index = 0)
    this->IconObjIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_EMPTY);
    // add icons
    this->IconObjIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_OBJ);
    this->IconFunctionIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_FUNCTION);
    this->IconParamIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_PARAM);
    this->IconLocalIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_LOCAL);
    this->IconThunkIndex=this->pTreeview->AddIcon(this->hInstance,IDI_ICON_THUNK);

    // select search by name by default
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_BY_NAME,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // accept drag and drop operation
    DragAcceptFiles(this->hWndDialog, TRUE);

    // render layout
    this->OnSize();

    // parse input file if needed
    if (this->pszFileToParseAtStartup)
    {
        // if filename is not empty
        if (*this->pszFileToParseAtStartup)
            this->OpenFile(this->pszFileToParseAtStartup);
    }
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::Close()
{
    // doing call in same thread than DebugInfo constructor is better for CoInitialize / CoUninitialize
    this->FreeMemory();

    DragAcceptFiles(this->hWndDialog, FALSE);
    delete this->pTreeview;
    this->pTreeview=NULL;
    delete this->pToolbar;
    this->pToolbar=NULL;
    delete this->pMenuExport;
    this->pMenuExport=NULL;
    EndDialog(this->hWndDialog,this->DialogResult);

    return;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CDebugInfosUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CDebugInfosUI* pDebugInfosUI=(CDebugInfosUI*)lParam;
            pDebugInfosUI->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDebugInfosUI);

            pDebugInfosUI->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_APP);
        }
        break;
    case WM_CLOSE:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;
            pDebugInfosUI->Close();
        }
        break;
    case WM_SIZING:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;
            pDebugInfosUI->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;
            pDebugInfosUI->OnSize();
        }
        break;
    case WM_COMMAND:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_FIND:
                pDebugInfosUI->Find();
                break;
            case IDC_BUTTON_FIND_NEXT:
                pDebugInfosUI->FindNext();
                break;
            case IDC_BUTTON_FIND_PREVIOUS:
                pDebugInfosUI->FindPrevious();
                break;
            case IDC_BUTTON_OPEN:
                pDebugInfosUI->OpenFile();
                break;
            case IDC_BUTTON_EXPORT:
                pDebugInfosUI->pTreeview->Save(
                                                pDebugInfosUI->pMenuExport->IsChecked(pDebugInfosUI->MenuExportExpandedOnlyId),
                                                pDebugInfosUI->pMenuExport->IsChecked(pDebugInfosUI->MenuExportSelectedOnlyId),
                                                _T("html")
                                                );
                break;
            case IDC_BUTTON_GENERATE:
                pDebugInfosUI->OnGenerate();
                break;
            case IDC_BUTTON_SHOW_ASM:
                CloseHandle(CreateThread(0,0,CDebugInfosUI::OnShowFunctionDisasm,pDebugInfosUI,0,0));
                break;
            case IDCANCEL:
                pDebugInfosUI->OnCancel();
                break;
            case IDC_BUTTON_EXPAND_MODULES:
                pDebugInfosUI->OnExpandModules();
                break;
            case IDC_BUTTON_COLLAPSE_MODULES:
                pDebugInfosUI->OnCollapseModules();
                break;
            case IDC_BUTTON_CHECK_PROJECT_SPECIFIC:
                pDebugInfosUI->OnCheckProjectSpecific();
                break;
            case IDC_BUTTON_CHECK_ALL:
                pDebugInfosUI->OnCheckAll();
                break;
            case IDC_BUTTON_UNCHECK_ALL:
                pDebugInfosUI->OnUncheckAll();
                break;
            case IDC_RADIO_BY_NAME:
            case IDC_RADIO_BY_VIRTUAL_ADDRESS:
            case IDC_RADIO_BY_RELATIVE_VIRTUAL_ADDRESS:
                {
                    BOOL bEnable=(IsDlgButtonChecked(pDebugInfosUI->hWndDialog,IDC_RADIO_BY_NAME)==BST_CHECKED);
                    EnableWindow(GetDlgItem(pDebugInfosUI->hWndDialog,IDC_BUTTON_FIND_NEXT),bEnable);
                    EnableWindow(GetDlgItem(pDebugInfosUI->hWndDialog,IDC_BUTTON_FIND_PREVIOUS),bEnable);
                }
                break;
            case IDC_BUTTON_DONATION:
                pDebugInfosUI->OnDonation();
                break;
            case IDC_BUTTON_ABOUT:
                pDebugInfosUI->OnAbout();
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;

            if (pDebugInfosUI->pToolbar)
            {
                if (pDebugInfosUI->pToolbar->OnNotify(wParam, lParam))
                    break;
            }

            if (pDebugInfosUI->pTreeview)
            {
                if (pDebugInfosUI->pTreeview->OnNotify(wParam, lParam))
                    break;
            }
        }
        break;
    case WM_DROPFILES:
        {
            CDebugInfosUI* pDebugInfosUI=((CDebugInfosUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pDebugInfosUI)
                break;
            pDebugInfosUI->OnDropFile(hWnd, (HDROP)wParam);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnDropFile
// Object: called when user ends a drag and drop operation
// Parameters :
//     in  : HWND hWnd : window handle
//           HDROP hDrop : drop informations
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnDropFile(HWND hWnd, HDROP hDrop)
{
    TCHAR pszFileName[MAX_PATH];
    UINT NbFiles;
    POINT pt;
    HWND SubWindowHandle;

    // retrieve dialog subitem window handle
    DragQueryPoint(hDrop, &pt);
    ClientToScreen(hWnd,&pt);
    SubWindowHandle=WindowFromPoint(pt);

    // get number of files count in the drag and drop
    NbFiles=DragQueryFile(hDrop, 0xFFFFFFFF,NULL, MAX_PATH);
    // get first file name (in case we just need one name)
    DragQueryFile(hDrop, 0,pszFileName, MAX_PATH);

    // no need of SubWindowHandle switch case
    // no needs of multiple files (take only the first)
    // --> we can finish drag from now

    DragFinish(hDrop);

    this->OpenFile(pszFileName);
}

//-----------------------------------------------------------------------------
// Name: OnShowFunctionDisasm / ShowFunctionDisasm
// Object: Display selected function asm
// Parameters : 
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
#ifdef _WIN64
    #ifdef _DEBUG
        #pragma comment(lib, "../../Disasm/distorm64/distorm_debug64.lib")
    #else
        #pragma comment(lib, "../../Disasm/distorm64/distorm64.lib")
    #endif
#else
    #ifdef _DEBUG
        #pragma comment(lib, "../../Disasm/distorm64/distorm_debug.lib")
    #else
        #pragma comment(lib, "../../Disasm/distorm64/distorm.lib")
    #endif
#endif

DWORD WINAPI CDebugInfosUI::OnShowFunctionDisasm(LPVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->ShowFunctionDisasm();
    return 0;
}
void CDebugInfosUI::ShowFunctionDisasm()
{
    HTREEITEM hItem;
    if (!this->pPE)
    {
        this->ReportError(_T("Disassembly not supported for selected file"));
        return;
    }

// to be implemented .NET disassembler
    if (this->pPE->IsNET())
    {
        this->ReportError(_T("Disassembly not supported for .NET file"));
        return;
    }

    // find selected function
    hItem=this->pTreeview->GetSelectedItem();
    if (!hItem)
    {
        this->ReportError(_T("No Item Selected"));
        return;
    }

    CFunctionInfos* pFunctionInfos=NULL;
    do 
    {
        // get function info from item user data
        this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
        // if no item user data, get parent in case of function property is selected instead of the function item itself
        hItem=this->pTreeview->GetParent(hItem);
        if (!hItem)
        {
            this->ReportError(_T("No Function Selected"));
            return;
        }
    } while(pFunctionInfos==NULL);
    

    // open file associated to function
    TCHAR szFile[MAX_PATH];
    this->pPE->GetFileName(szFile);
    HANDLE hFile=CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!hFile)
    {
        TCHAR sz[2*MAX_PATH];
        _stprintf(sz,_T("Error opening file %s"),szFile);
        this->ReportError(sz);
        return;
    }

    // get function name
    TCHAR* pszFunctionName=pFunctionInfos->UndecoratedName ? pFunctionInfos->UndecoratedName : pFunctionInfos->Name;
    // create html window
    CHtmlViewerWindow HtmlWindow(640,480);
    if (!HtmlWindow.Show(this->hInstance,
                    this->hWndDialog,
                    pszFunctionName,
                    this->hInstance,IDI_ICON_APP)
        )
    {
        CloseHandle(hFile);
        return;
    }
 
    // get function code according to debug information
    PBYTE LocalBuffer=new BYTE[pFunctionInfos->Length];
    SetFilePointer(hFile,
        pFunctionInfos->Offset+this->pPE->pSectionHeaders[pFunctionInfos->SectionIndex-1].PointerToRawData,
        0,
        FILE_BEGIN);
    DWORD ReadBytes;
    ReadFile(hFile,LocalBuffer,pFunctionInfos->Length,&ReadBytes,NULL);

    // allow selection
    HtmlWindow.EnableSelection(TRUE);

    DIASM_CALLBACK_PARAM DisasmCallBackParams;
    DisasmCallBackParams.pHtmlViewerWindow=&HtmlWindow;
    DisasmCallBackParams.pFunctionSourcesInfos=NULL;
    DisasmCallBackParams.ImageBase=this->pPE->NTHeader.OptionalHeader.ImageBase;
    if (this->pPE->Is64Bits())
        DisasmCallBackParams.DecodeType=CDisasm::Decode64Bits;
    else
        DisasmCallBackParams.DecodeType=CDisasm::Decode32Bits;

    

    CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos=new CLinkListTemplate<CDebugInfosSourcesInfos>();
    if (this->pDebugInfos->FindLinesByRVA(pFunctionInfos->RelativeVirtualAddress,pLinkListSourcesInfos))
    {
        DisasmCallBackParams.pFunctionSourcesInfos=pLinkListSourcesInfos;
    }
   

    TCHAR* psz=new TCHAR[_tcslen(pszFunctionName)+4*MAX_PATH];

    // set html template content
    if (DisasmCallBackParams.pFunctionSourcesInfos)
    {
        _stprintf(psz,
            _T("<table border=\"0\" align=\"center\" cellpadding=\"5\" cellspacing=\"0\"><tr>")
            _T("<td><table border=\"0\" cellpadding=\"5\" cellspacing=\"0\" id=\"id_table\">")
            _T("<tr><td colspan=\"3\" class=\"SrcCode\">%s</td></tr>")
            _T("</table></td></tr>")
            _T("<!--DONT_SAVE_BEGIN-->")
            _T("<tr><td align=\"right\">")
            _T("<style type=\"text/css\"><!--")
            _T("a:link {text-decoration: none;color: #5D92F1;font-weight:bold;}\r\n")
            _T("a:visited {text-decoration: none;color: #5D92F1;font-weight:bold;}\r\n")
            _T("a:hover {text-decoration: underline;color: #5D92F1;font-weight:bold;}\r\n")
            _T("--></style>\r\n")
            _T("<br><a href=\"javascript:\" id=\"id_save\">Save</a></span>")
            _T("</td></tr>")
            _T("<!--DONT_SAVE_END-->")
            _T("</table>")
            _T("<style type=\"text/css\"><!--")
            _T(".Address {	background-color: #F0F0F0;}")
            _T(".SrcFile {	font-size: small;	color: #009933;}")
            _T(".SrcCode {	color: #0033CC;}")
            _T("--></style>"),
            pszFunctionName
            );
    }
    else
    {
        _stprintf(psz,
            _T("<table border=\"0\" align=\"center\" cellpadding=\"5\" cellspacing=\"0\">")
            _T("<tr><td colspan=\"3\">%s</td></tr>")
            _T("<tr><td id=\"id_asm_address\" bgcolor=\"#F0F0F0\"></td>")
            _T("<td id=\"id_asm_hex\"></td>")
            _T("<td id=\"id_asm_op\"></td>")
            _T("</tr>")
            _T("<!--DONT_SAVE_BEGIN-->")
            _T("<tr><td align=\"right\" colspan=\"3\">")
            _T("<style type=\"text/css\"><!--")
            _T("a:link {text-decoration: none;color: #5D92F1;font-weight:bold;}\r\n")
            _T("a:visited {text-decoration: none;color: #5D92F1;font-weight:bold;}\r\n")
            _T("a:hover {text-decoration: underline;color: #5D92F1;font-weight:bold;}\r\n")
            _T("--></style>\r\n")
            _T("<br><a href=\"javascript:\" id=\"id_save\">Save</a></span>")
            _T("</td></tr>")
            _T("<!--DONT_SAVE_END-->")
            _T("</table>"),
            pszFunctionName
            );
    }
    HtmlWindow.LoadEmptyPageAndSetBodyContent(psz);
    delete[] psz;


    // disasm function
    if (!CDisasm::Disasm(pFunctionInfos->RelativeVirtualAddress+this->pPE->NTHeader.OptionalHeader.ImageBase,
                        LocalBuffer,
                        pFunctionInfos->Length,
                        DisasmCallBackParams.DecodeType,
                        CDebugInfosUI::DisasmInstructionCallBack,
                        &DisasmCallBackParams)
        )
    {
        this->ReportError(_T("Error disassembling function"));
    }
    
    delete[] LocalBuffer;
    delete pLinkListSourcesInfos;

    // add html events support
    HtmlWindow.SetElementsEventsCallBack(CDebugInfosUI::AsmHtmlElementEventsCallBack,&HtmlWindow);
    // wait until user close the window (avoid local var destruction)
    HtmlWindow.WaitWindowClose();
    CloseHandle(hFile);
}
//-----------------------------------------------------------------------------
// Name: DisasmInstructionCallBack
// Object: called on each asm decoded instruction
// Parameters : CDisasm callback parameters
//     in  : 
//     out :
//     return : TRUE to continue asm instruction parsing
//-----------------------------------------------------------------------------
BOOL CDebugInfosUI::DisasmInstructionCallBack(ULONGLONG InstructionAddress,DWORD InstructionsSize,TCHAR* InstructionHex,TCHAR* Mnemonic,TCHAR* Operands,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(InstructionsSize);

    DIASM_CALLBACK_PARAM* pParam=(DIASM_CALLBACK_PARAM*)UserParam;
    CHtmlViewerWindow* pHtmlWindow=pParam->pHtmlViewerWindow;

    if (pParam->pFunctionSourcesInfos)
    {
        DWORD dwRes;
        THREADED_DIASM_CALLBACK_PARAM ThreadedDiasmCallBackParam;
        ThreadedDiasmCallBackParam.pDiasmCallBackParam=pParam;
        ThreadedDiasmCallBackParam.InstructionAddress=InstructionAddress;
        ThreadedDiasmCallBackParam.InstructionHex=InstructionHex;
        ThreadedDiasmCallBackParam.Mnemonic=Mnemonic;
        ThreadedDiasmCallBackParam.Operands=Operands;

        pHtmlWindow->ExecuteFunctionInHTMLViewerThread(ThreadedDisasmInstructionCallBack,&ThreadedDiasmCallBackParam,&dwRes);
    }
    else
    {
        _tcsupr(InstructionHex);
        TCHAR Content[MAX_PATH];

        if (pParam->DecodeType==CDisasm::Decode64Bits)
            _sntprintf(Content,MAX_PATH,_T("0x%I64X<br>"),InstructionAddress);
        else
            _sntprintf(Content,MAX_PATH,_T("0x%I32X<br>"),(DWORD)InstructionAddress);

        pHtmlWindow->AddHtmlContentToElement(_T("id_asm_address"),Content);
        _sntprintf(Content,MAX_PATH,_T("%s<br>"),InstructionHex);
        pHtmlWindow->AddHtmlContentToElement(_T("id_asm_hex"),Content);
        _sntprintf(Content,
                    MAX_PATH,_T("%s%s%s<br>"),
                    Mnemonic,
                    *Operands != 0 ? _T("&nbsp;") : _T(""),
                    Operands
                    );
        pHtmlWindow->AddHtmlContentToElement(_T("id_asm_op"),Content);
    }

    // continue parsing
    return TRUE;
}
DWORD CDebugInfosUI::ThreadedDisasmInstructionCallBack(PVOID UserParam)
{
    THREADED_DIASM_CALLBACK_PARAM* pThreadedDiasmCallBackParam=(THREADED_DIASM_CALLBACK_PARAM*)UserParam;
    CHtmlViewerWindow* pHtmlViewerWindow=pThreadedDiasmCallBackParam->pDiasmCallBackParam->pHtmlViewerWindow;

    BSTR bStr;
    WCHAR* pStr;
    WCHAR* pStrTmp;
    WCHAR Str[MAX_PATH];
#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
    WCHAR StrTmp[4*MAX_PATH];
#endif
    TCHAR* p_tStr;

    // we can use AddHtmlContentToElement for table object
    // so we have to take the table element object
    IHTMLElement* pElement=NULL;
    IHTMLTable* pTable=NULL;
    IHTMLTableRow* pRow=NULL;
    IHTMLTableCell* pCell=NULL;
    IHTMLElement* pCellElement=NULL;
    HRESULT hr;
    BSTR bsInsertAdjacentHTMLBeforeEnd=SysAllocString(L"beforeEnd");

    
    pElement=pHtmlViewerWindow->GetHTMLElement(_T("id_table"));
    if (!pElement)
        goto CleanUp;
    hr=pElement->QueryInterface(IID_IHTMLTable,(void**)&pTable);
    if (FAILED(hr) || (pTable==NULL))
        goto CleanUp;

    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pItem;
    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pNextItem;
    BOOL SrcCodeAdded=FALSE;
    for (pItem=pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->GetHead();
        pItem;
        pItem=pNextItem
        )
    {
        pNextItem=pItem->NextItem;

        // if source line match instruction address
        if ( (ULONGLONG)pThreadedDiasmCallBackParam->InstructionAddress
             >=(ULONGLONG)pThreadedDiasmCallBackParam->pDiasmCallBackParam->ImageBase+pItem->ItemData->RelativeVirtualAddress
            )
        {
            // on read error
            if (pItem->ItemData->LineContent==0)
            {
                // remove source code info from list (no more use and reduce list size)
                pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->RemoveItem(pItem);

                continue;
            }
            // if empty src line
            p_tStr=CTrimString::TrimString(pItem->ItemData->LineContent,TRUE);
            if (*p_tStr==0)
            {
                // remove source code info from list (no more use and reduce list size)
                pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->RemoveItem(pItem);

                free(p_tStr);
                continue;
            }

            // if line contains only closing bracket
            if (*p_tStr=='}' && p_tStr[1]==0) // string is not empty length is at least 1 --> we can access p_tStr[1]
            {
                // if not last item
                if (pItem!=pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->GetTail())
                {
                    // data is often function closing bracket --> remove it
                    if (pItem->NextItem->ItemData->LineNumber<pItem->ItemData->LineNumber)
                    {
                        // remove source code info from list (no more use and reduce list size)
                        pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->RemoveItem(pItem);

                        free(p_tStr);
                        continue;
                    }
                }
            }
            free(p_tStr);

            //////////////////
            // new table row file containing source code
            //////////////////
            if (!SrcCodeAdded) // add src file name and line only once if multiple lines
            {
                // insert row
                hr=pTable->insertRow(-1,(IDispatch**)&pRow);
                if (FAILED(hr) || (pRow==NULL))
                    goto CleanUp;

                // insert cell
                hr=pRow->insertCell(-1,(IDispatch**)&pCell);
                if (FAILED(hr) || (pCell==NULL))
                    goto CleanUp;

                // a single cell for 3 cells in other lines --> set colspan to 3
                pCell->put_colSpan(3);

                // get html element interface
                hr=pCell->QueryInterface(IID_IHTMLElement,(void**)&pCellElement);
                if (FAILED(hr) || (pCellElement==NULL))
                    goto CleanUp;

                // set content and class name
                bStr=SysAllocString(L"SrcFile");
                pCellElement->put_className(bStr);
                SysFreeString(bStr);

#if (defined(UNICODE)||defined(_UNICODE))
                pStr=pItem->ItemData->FileName;
#else
                CAnsiUnicodeConvert::AnsiToUnicode(pItem->ItemData->FileName,StrTmp,4*MAX_PATH);
                pStr=StrTmp;
#endif
                // add "FullPath (l:line)"
                swprintf(Str,L"%s&nbsp;(line %u)",pStr,pItem->ItemData->LineNumber);
                bStr=SysAllocString(Str);
                pCellElement->put_innerHTML(bStr);
                SysFreeString(bStr);

                // free allocated memory
                pCellElement->Release();
                pCellElement=NULL;
                pCell->Release();
                pCell=NULL;
                pRow->Release();
                pRow=NULL;

            }
            //////////////////
            // new table row for source code content
            //////////////////

            // insert row
            hr=pTable->insertRow(-1,(IDispatch**)&pRow);
            if (FAILED(hr) || (pRow==NULL))
                goto CleanUp;

            // insert cell
            hr=pRow->insertCell(-1,(IDispatch**)&pCell);
            if (FAILED(hr) || (pCell==NULL))
                goto CleanUp;

            // a single cell for 3 cells in other lines --> set colspan to 3
            pCell->put_colSpan(3);

            // get html element interface
            hr=pCell->QueryInterface(IID_IHTMLElement,(void**)&pCellElement);
            if (FAILED(hr) || (pCellElement==NULL))
                goto CleanUp;

            // set content and class name
            bStr=SysAllocString(L"SrcCode");
            pCellElement->put_className(bStr);
            SysFreeString(bStr);

            // add line content
#if (defined(UNICODE)||defined(_UNICODE))
            pStr=pItem->ItemData->LineContent;
#else
            CAnsiUnicodeConvert::AnsiToUnicode(pItem->ItemData->LineContent,StrTmp,4*MAX_PATH);
            pStr=StrTmp;
#endif

            // replace & with "&amp;" MUST BE THE FIRST else all our previous & will be replaced
            pStrTmp=new WCHAR[wcslen(pStr)*6+1];
            CStringReplace::ReplaceW(pStr,pStrTmp,L"&",L"&amp;");

            pStr=pStrTmp;
            pStrTmp=new WCHAR[wcslen(pStr)*6+1];
            CStringReplace::ReplaceW(pStr,pStrTmp,L"<",L"&lt;");
            delete[] pStr;

            pStr=pStrTmp;
            pStrTmp=new WCHAR[wcslen(pStr)*6+1];
            CStringReplace::ReplaceW(pStr,pStrTmp,L">",L"&gt;");
            delete[] pStr;

            pStr=pStrTmp;
            pStrTmp=new WCHAR[wcslen(pStr)*6+1];
            CStringReplace::ReplaceW(pStr,pStrTmp,L"\"",L"&quot;");
            delete[] pStr;

            // replace spaces with "&nbsp;"
            pStr=pStrTmp;
            pStrTmp=new WCHAR[wcslen(pStr)*6+1];
            CStringReplace::ReplaceW(pStr,pStrTmp,L" ",L"&nbsp;");
            delete[] pStr;

            bStr=SysAllocString(pStrTmp);
            pCellElement->put_innerHTML(bStr);
            SysFreeString(bStr);
            delete[] pStrTmp;

            // free allocated memory
            pCellElement->Release();
            pCellElement=NULL;
            pCell->Release();
            pCell=NULL;
            pRow->Release();
            pRow=NULL;

            // remove source code info from list (no more use and reduce list size)
            pThreadedDiasmCallBackParam->pDiasmCallBackParam->pFunctionSourcesInfos->RemoveItem(pItem);

            SrcCodeAdded=TRUE;
        }
        else // address are by increasing order
            break;
    }

    //////////////////
    // new table row for asm content
    //////////////////

    // insert row
    hr=pTable->insertRow(-1,(IDispatch**)&pRow);
    if (FAILED(hr) || (pRow==NULL))
        goto CleanUp;

    for (int ColumnIndex=0;ColumnIndex<3;ColumnIndex++)
    {
        hr=pRow->insertCell(-1,(IDispatch**)&pCell);
        if (FAILED(hr) || (pCell==NULL))
            goto CleanUp;

        hr=pCell->QueryInterface(IID_IHTMLElement,(void**)&pCellElement);
        if (FAILED(hr) || (pCellElement==NULL))
            goto CleanUp;

        switch (ColumnIndex)
        {
        case 0: // address
            bStr=SysAllocString(L"Address");
            pCellElement->put_className(bStr);
            SysFreeString(bStr);

            swprintf(Str,L"0x%p ",pThreadedDiasmCallBackParam->InstructionAddress);
            bStr=SysAllocString(Str);
            pCellElement->put_innerHTML(bStr);
            SysFreeString(bStr);
            break;

        case 1: // bytes
#if (defined(UNICODE)||defined(_UNICODE))
            pStr=pThreadedDiasmCallBackParam->InstructionHex;
#else
            CAnsiUnicodeConvert::AnsiToUnicode(pThreadedDiasmCallBackParam->InstructionHex,StrTmp,4*MAX_PATH);
            pStr=StrTmp;
#endif
            bStr=SysAllocString(pStr);
            pCellElement->put_innerHTML(bStr);
            SysFreeString(bStr);
            break;

        case 2: // mnemonics & operands
#if (defined(UNICODE)||defined(_UNICODE))
            pStr=pThreadedDiasmCallBackParam->Mnemonic;
#else
            CAnsiUnicodeConvert::AnsiToUnicode(pThreadedDiasmCallBackParam->Mnemonic,StrTmp,4*MAX_PATH);
            pStr=StrTmp;
#endif

            wcscpy(Str,pStr);
            if (*pThreadedDiasmCallBackParam->Operands)
                wcscat(Str,L"&nbsp;");

#if (defined(UNICODE)||defined(_UNICODE))
            pStr=pThreadedDiasmCallBackParam->Operands;
#else
            CAnsiUnicodeConvert::AnsiToUnicode(pThreadedDiasmCallBackParam->Operands,StrTmp,4*MAX_PATH);
            pStr=StrTmp;
#endif
            wcscat(Str,pStr);

            bStr=SysAllocString(Str);
            pCellElement->put_innerHTML(bStr);
            SysFreeString(bStr);
            break;
        }

        pCellElement->Release();
        pCellElement=NULL;
        pCell->Release();
        pCell=NULL;
    }
    pRow->Release();
    pRow=NULL;

CleanUp:
    SysFreeString(bsInsertAdjacentHTMLBeforeEnd);
    if (pCellElement)
        pCellElement->Release();
    if (pCell)
        pCell->Release();
    if (pRow)
        pRow->Release();
    if (pTable)
        pTable->Release();
    if (pElement)
        pElement->Release();

    return 0;
}

//-----------------------------------------------------------------------------
// Name: AsmHtmlElementEventsCallBack
// Object: called on each event on html window
// Parameters : CHtmlViewerWindow callback parameters
//     in  : 
//     out :
//     return : TRUE to continue asm instruction parsing
//-----------------------------------------------------------------------------
void __stdcall CDebugInfosUI::AsmHtmlElementEventsCallBack(DISPID dispidMember,WCHAR* ElementId,LPVOID UserParam)
{
    switch(dispidMember)
    {
    case DISPID_HTMLELEMENTEVENTS2_ONCLICK:
        {
            TCHAR* pszElementId;
            CHtmlViewerWindow* pHtmlViewerWindow=(CHtmlViewerWindow*)UserParam;

#if (defined(UNICODE)||defined(_UNICODE))
            pszElementId=ElementId;
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(ElementId,&pszElementId);
#endif

            if (_tcsncmp(pszElementId,_T("id_save"),_tcslen(_T("id_save")))==0)
                pHtmlViewerWindow->Save();

#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
            free(pszElementId);
#endif
        }
        break;
    default:
        break;
    }
}

void CDebugInfosUI::OnGenerate()
{
    if (this->GenerateMonitoringFile())
        this->DialogResult=1;
}
void CDebugInfosUI::OnCancel()
{
    this->DialogResult=0;
    this->Close();
}

//-----------------------------------------------------------------------------
// Name: OnTreeViewCheckedStateChangedStatic / OnTreeViewCheckedStateChanged
// Object: occurs when a treeview item has been checked / unchecked
// Parameters :
//     in  : HTREEITEM hTreeItemCheckedChanged : treeview item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnTreeViewCheckedStateChangedStatic(HTREEITEM hTreeItemCheckedChanged,PVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->OnTreeViewCheckedStateChanged(hTreeItemCheckedChanged);
}
void CDebugInfosUI::OnTreeViewCheckedStateChanged(HTREEITEM hTreeItemCheckedChanged)
{
    // if item is not a module one
    if (this->pTreeview->GetItemDepth(hTreeItemCheckedChanged)!=0)
        // return
        return;

    HTREEITEM hItem;
    CTreeview::tagCheckState OldCheckedState;
    CTreeview::tagCheckState NewCheckedState;
    CTreeview::tagCheckState BeforeClickCheckedState;

    // get new state (warning notification comes before new state is applied)
    this->pTreeview->GetCheckedState(hTreeItemCheckedChanged,&BeforeClickCheckedState);
    switch(BeforeClickCheckedState)
    {
    case CTreeview::CHECK_STATE_CHECKED:
        NewCheckedState=CTreeview::CHECK_STATE_UNCHECKED;
        break;
    case CTreeview::CHECK_STATE_UNCHECKED:
        NewCheckedState=CTreeview::CHECK_STATE_CHECKED;
            break;
    default:
        NewCheckedState=CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE;
        break;
    }
    

    // for each child
    hItem=TreeView_GetChild(this->pTreeview->GetControlHandle(),hTreeItemCheckedChanged);
    while (hItem)
    {
        // get item state
        this->pTreeview->GetCheckedState(hItem,&OldCheckedState);

        // assume item has a check box and new check state is different than the old one
        if ((OldCheckedState!=CTreeview::CHECK_STATE_NO_CHECK_BOX_IMAGE)
             && ( NewCheckedState != OldCheckedState))
             // set new state
             this->pTreeview->SetCheckedState(hItem,NewCheckedState);

        // get next item
        hItem=TreeView_GetNextSibling(this->pTreeview->GetControlHandle(),hItem);
    }

    return;
}

BOOL CDebugInfosUI::CheckCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->CheckCallBack(hItem);
    return TRUE;
}
void CDebugInfosUI::CheckCallBack(HTREEITEM hModuleItem)
{
    CTreeview::tagCheckState CheckedState;

    CheckedState = CTreeview::CHECK_STATE_UNCHECKED;

    switch(this->QueryCheckState)
    {
    case QueryCheckState_CHECK_ALL:
        CheckedState = CTreeview::CHECK_STATE_CHECKED;
        break;
    case QueryCheckState_CHECK_PROJECT_SPECIFIC:
        {
            CTreeview::tagCheckState OldCheckedState;
            CTreeview::tagCheckState NewCheckedState;
            TCHAR ModuleName[MAX_PATH];
            this->pTreeview->GetItemText(hModuleItem,ModuleName,MAX_PATH);

            // VS2003 module name checking : when build on vs2003, local project file can be identify because they start with ".\"
            if (_tcsncmp( ModuleName, _T(".\\"),2) == 0) // _tecslen(_T(".\\")) ) ==0 )
            {
                NewCheckedState = CTreeview::CHECK_STATE_CHECKED;
            }
            else
            {
                // if object file exists, consider object to belong to local project (works until developers download m$ obj and put them in the same path of m$ developers)
                if (CStdFileOperations::DoesFileExists(ModuleName))
                {
                    NewCheckedState = CTreeview::CHECK_STATE_CHECKED;
                }
                else
                {
                    NewCheckedState = CTreeview::CHECK_STATE_UNCHECKED;
                }
            }

            // for each child
            HTREEITEM hItem=TreeView_GetChild(this->pTreeview->GetControlHandle(),hModuleItem);
            while (hItem)
            {
                // get item state
                this->pTreeview->GetCheckedState(hItem,&OldCheckedState);

                // assume new check state is different than the old one
                if (NewCheckedState != OldCheckedState)
                    // set new state
                    this->pTreeview->SetCheckedState(hItem,NewCheckedState);

                // get next item
                hItem=TreeView_GetNextSibling(this->pTreeview->GetControlHandle(),hItem);
            }
            CheckedState = NewCheckedState;
        }

        break;
    case QueryCheckState_CHECK_UNCHECK_ALL:
        CheckedState = CTreeview::CHECK_STATE_UNCHECKED;
        break;
    }

    this->pTreeview->SetCheckedState(hModuleItem,CheckedState);
}
void CDebugInfosUI::OnCheckProjectSpecific()
{
    // uncheck module level 
    this->QueryCheckState = CDebugInfosUI::QueryCheckState_CHECK_UNCHECK_ALL;
    this->pTreeview->Parse(this->pTreeview->GetRoot(),0,CDebugInfosUI::CheckCallBackStatic,this,FALSE);

    // potentially check function level
    this->QueryCheckState = CDebugInfosUI::QueryCheckState_CHECK_PROJECT_SPECIFIC;
    this->pTreeview->Parse(this->pTreeview->GetRoot(),0,CDebugInfosUI::CheckCallBackStatic,this,FALSE);
}
void CDebugInfosUI::OnCheckAll()
{
    // check module and function level
    this->QueryCheckState = CDebugInfosUI::QueryCheckState_CHECK_ALL;
    this->pTreeview->Parse(this->pTreeview->GetRoot(),1,CDebugInfosUI::CheckCallBackStatic,this,FALSE);
}
void CDebugInfosUI::OnUncheckAll()
{
    // uncheck module and function level
    this->QueryCheckState = CDebugInfosUI::QueryCheckState_CHECK_UNCHECK_ALL;
    this->pTreeview->Parse(this->pTreeview->GetRoot(),1,CDebugInfosUI::CheckCallBackStatic,this,FALSE);
}

//-----------------------------------------------------------------------------
// Name: ReportError
// Object: function to report errors
// Parameters :
//     in  : TCHAR* Msg : error message
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::ReportError(TCHAR* Msg)
{
    // sometimes MB_TOPMOST make MessageBox fail if no dialog is associated to thread
    if (!MessageBox(this->hWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST))
        MessageBox(this->hWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR);
}

//-----------------------------------------------------------------------------
// Name: GenerateMonitoringParseCallBackStatic / GenerateMonitoringParseCallBack
// Object: callback called for each treeview item when generating monitoring file for WinAPIOverride
// Parameters :
//     in  : HTREEITEM hItem : treeview item
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CDebugInfosUI::GenerateMonitoringParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    ((CDebugInfosUI*)UserParam)->GenerateMonitoringParseCallBack(hItem);
    // continue parsing
    return TRUE;
}
void CDebugInfosUI::GenerateMonitoringParseCallBack(HTREEITEM hItem)
{
    CLinkListItem* pItemParam;
    CTypeInfos* pParamInfos;
    CFunctionInfos* pFunctionInfos;
    TCHAR sz[2*MAX_PATH];
    ULONGLONG FunctionAddress;

    // if item is not checked
    if (!this->pTreeview->IsChecked(hItem))
        return;

    // get item user data
    this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
    // check item user data (MUST BE FIELD ONLY FOR FUNCTIONS)
    if (!pFunctionInfos)
        return;
    if (IsBadReadPtr(pFunctionInfos,sizeof(CFunctionInfos)))
        return;

    if (pFunctionInfos->Token)
    {
        // report error only once
        if (!this->NetWarningReported)
        {
            this->ReportError(_T("Please use .Net Monitoring file generator for .Net assembly"));
            this->NetWarningReported=TRUE;
        }
        return;
    }

    // check function section index
    if (this->pPE->NTHeader.FileHeader.NumberOfSections<pFunctionInfos->SectionIndex)
    {
        this->ReportError(_T("Bad function section Index"));
        return;
    }


    // generate type infos for function
    this->GenerateUserTypesInfosForFunction(pFunctionInfos);

    // if function length is not sufficient to install hook
    // write a report and make next function hook definition begin with ";" to disable it
    if (pFunctionInfos->Length<OPCODE_REPLACEMENT_SIZE)
    {
        CTextFile::WriteText(this->hMonitoringFile,
                            _T(";Function can't be hook : size is too small\r\n")
                            _T(";"));
    }

    //////////////////////////////////////////
    // 1) write hook type
    //////////////////////////////////////////
    switch (this->FileToMonitorType)
    {
    case FileToMonitorType_DLL:
        CTextFile::WriteText(this->hMonitoringFile,DLL_INTERNAL);
        // dll internal use relative base address
        FunctionAddress=pFunctionInfos->RelativeVirtualAddress;
        break;
    default:
    case FileToMonitorType_EXE:
        CTextFile::WriteText(this->hMonitoringFile,EXE_INTERNAL_RVA);
        // exe internal relative virtual address (since vista, exe can be rebased at loading)
        FunctionAddress=pFunctionInfos->RelativeVirtualAddress;
        break;
    }

    //////////////////////////////////////////
    // 2) write hook address
    //////////////////////////////////////////
    if (this->pPE->Is64Bits())
    {
        _stprintf(sz,_T("0x%I64X"),FunctionAddress);
    }
    else
    {
        DWORD FunctionAddress32=(DWORD)FunctionAddress;
        _stprintf(sz,_T("0x%I32X"),FunctionAddress32);
    }
    CTextFile::WriteText(this->hMonitoringFile,sz);

    // in case of dll, we have to add dll name (as we use relative address, we need dll base address given from dll name)
    if (this->FileToMonitorType == FileToMonitorType_DLL)
    {
        CTextFile::WriteText(this->hMonitoringFile,_T("@"));
        this->pPE->GetFileName(sz);
        CTextFile::WriteText(this->hMonitoringFile,sz);
    }
    
    CTextFile::WriteText(this->hMonitoringFile,SPLITTER);

    //////////////////////////////////////////
    // 3) write hook function infos
    //////////////////////////////////////////

    // for object method with explicit calling convention (__stdcall or other),
    // "this" is declared as a local var, instead as a parameter,
    // so to correct it : 
    // check that first var in first block is not "this" with ebp+xx address (remember ebp+xx is a param and ebp-xx a function local var)
    BOOL BadThisLocal=FALSE;
    if (pFunctionInfos->pLinkListBlocks->Head)
    {
        CFunctionBlockInfos* pBlockInfos=(CFunctionBlockInfos*)pFunctionInfos->pLinkListBlocks->Head->ItemData;
        if (pBlockInfos->pLinkListVars->Head)
        {
            TCHAR szRegisterName[MAX_PATH];
            CTypeInfos* FirstLocal=(CTypeInfos*)pBlockInfos->pLinkListVars->Head->ItemData;
            FirstLocal->pSymbolLocation->GetRegisterName(szRegisterName,MAX_PATH);
            if (
                (_tcsicmp(FirstLocal->Name,_T("this"))==0) // check "this" var name
                && (_tcsicmp(szRegisterName,_T("ebp"))==0) // check "ebp" register
                && (FirstLocal->pSymbolLocation->RelativeOffset>0) // check ebp positive offset (ebp+xx)
                )
            {
                BadThisLocal=TRUE;
            }
        }
    }

    BOOL DontUseUndecoratedName=TRUE;
    // Notice : don't use undecorated name to give parameters name and size
    if (pFunctionInfos->UndecoratedName && (!BadThisLocal))
    {
        TCHAR* pszBegin;
        TCHAR* pszEnd;
        TCHAR* pszSubBlockBegin;
        TCHAR* pszSubBlockEnd;
        BOOL LastParam=FALSE;
        BOOL ParamListBegin=TRUE;
        BOOL SubBlockParam=FALSE;
        pszBegin=pFunctionInfos->UndecoratedName;
        pszSubBlockBegin=_tcschr(pszBegin,'(');
        if (pszSubBlockBegin)
        {
            TCHAR* psz = _tcsdup(pszSubBlockBegin +1);
            pszEnd = _tcschr(psz,')');
            if (!pszEnd)
            {
                // bad formed name --> use param list
                pszSubBlockBegin=NULL;
            }
            else
            {
                *pszEnd = 0;
                CTrimString::TrimString(psz,FALSE);
                if (_tcsicmp(psz, _T("void") ) ==0 )
                {
                    // we may get no information on parameters --> use param list
                    pszSubBlockBegin=NULL;
                }
            }
            free(psz);
        }
        if (!pszSubBlockBegin)
        {
            // undecorated name has no info on param --> use param list
        }
        else
        {
            // write function name with first '('
            CTextFile::WriteText(this->hMonitoringFile,pszBegin,pszSubBlockBegin-pszBegin +1);

            // point after begin of parameter list
            pszBegin = pszSubBlockBegin+1;

            DontUseUndecoratedName=FALSE;
            for(pItemParam=pFunctionInfos->pLinkListParams->Head;;)
            {
                // look for function parameters (like "int __cdecl put(int n,unsigned int (__cdecl*)(void * wfunc,char const * param,unsigned int),void *)"
                pszSubBlockBegin=_tcschr(pszBegin,'(');
                // find param splitter
                pszEnd=_tcschr(pszBegin,',');

                if (pszSubBlockBegin)
                {
                    // first sub block is for parameters
                    if (ParamListBegin)
                        ParamListBegin=FALSE;
                    else // parameter is a function
                    {
                        if (pszSubBlockBegin<pszEnd)
                        {
                            pszSubBlockEnd=_tcschr(pszSubBlockBegin,')');
                            if (!pszSubBlockEnd)// should not occur
                            {
                                CTextFile::WriteText(this->hMonitoringFile,pszBegin);
                                break;
                            }

                            // if previous was a param (do checking as calling convention can be a block)
                            if (!SubBlockParam)
                            {
                                // if not first param
                                if (pItemParam!=pFunctionInfos->pLinkListParams->Head)
                                    // add ','
                                    CTextFile::WriteText(this->hMonitoringFile,_T(","));
                            }


                            SubBlockParam=TRUE;
                            CTextFile::WriteText(this->hMonitoringFile,pszBegin,pszSubBlockEnd-pszBegin);
                            pszBegin=pszSubBlockEnd;
                            continue;
                        }
                    }
                }

                if (!pszEnd)
                {
                    pszEnd=_tcschr(pszBegin,')');
                    if (pszEnd)
                    {
                        LastParam=TRUE;
                    }
                    else
                    {
                        // no more param --> write the end of undecorated name
                        CTextFile::WriteText(this->hMonitoringFile,pszBegin);
                        break;
                    }
                }

                if (SubBlockParam)
                {
                    SubBlockParam=FALSE;
                }
                else
                {
                    // if not first param
                    if (pItemParam!=pFunctionInfos->pLinkListParams->Head)
                        // add ','
                        CTextFile::WriteText(this->hMonitoringFile,_T(","));
                }

                // if we still get param info
                if (pItemParam)
                {
                    // write data until param
                    CTextFile::WriteText(this->hMonitoringFile,pszBegin,pszEnd-pszBegin);
                    pParamInfos=(CTypeInfos*)pItemParam->ItemData;
                    // add param name
                    if (pParamInfos->Name)
                    {
                        // add space
                        CTextFile::WriteText(this->hMonitoringFile,_T(" "));
                        // add name
                        CTextFile::WriteText(this->hMonitoringFile,pParamInfos->Name);
                    }

                    // add param size if needed
                    if ((pParamInfos->bUserDefineType)
                        &&(pParamInfos->Size!=sizeof(PBYTE))
                        &&(pParamInfos->UserDefineTypeKind!=UdtClass)// don't generate data size for class (too big buffer repeated for each call of a class method)
                        )
                    {
                        _stprintf(sz,_T("%u"),pParamInfos->Size);
                        switch (pParamInfos->PointerLevel)
                        {
                        case 0:
                            CTextFile::WriteText(this->hMonitoringFile,_T(":DataSize="));
                            CTextFile::WriteText(this->hMonitoringFile,sz);
                            break;
                        case 1:
                            CTextFile::WriteText(this->hMonitoringFile,_T(":PointedDataSize="));
                            CTextFile::WriteText(this->hMonitoringFile,sz);
                            break;
                        }
                    }
                }
                else
                {
                    CTextFile::WriteText(this->hMonitoringFile,pszBegin);
                    break;
                }

                if (pItemParam)
                    pItemParam=pItemParam->NextItem;

                if (LastParam)
                {
                    CTextFile::WriteText(this->hMonitoringFile,pszEnd);
                    break;
                }

                pszBegin=pszEnd+1;
            }
        }
    }
    if (DontUseUndecoratedName)
    {
        //////////////////////////////////////////
        //      return type
        //////////////////////////////////////////
        // get type name
        pFunctionInfos->pReturnInfos->GetPrettyName(sz,2*MAX_PATH);
        CTextFile::WriteText(this->hMonitoringFile,sz);
        // add space
        CTextFile::WriteText(this->hMonitoringFile,_T(" "));
        
        //////////////////////////////////////////
        //      calling convention
        //////////////////////////////////////////
        pFunctionInfos->GetCallingConvention(sz,2*MAX_PATH);
        CTextFile::WriteText(this->hMonitoringFile,sz);
        // add space
        CTextFile::WriteText(this->hMonitoringFile,_T(" "));

        //////////////////////////////////////////
        //      function name
        //////////////////////////////////////////
        CTextFile::WriteText(this->hMonitoringFile,pFunctionInfos->Name);
        // add '('
        CTextFile::WriteText(this->hMonitoringFile,_T("("));

        //////////////////////////////////////////
        //      parameters
        //////////////////////////////////////////
        if (BadThisLocal)
            CTextFile::WriteText(this->hMonitoringFile,_T("PVOID pObject"));

        for (pItemParam=pFunctionInfos->pLinkListParams->Head;pItemParam;pItemParam=pItemParam->NextItem)
        {
            // if not first param
            if ( (pItemParam!=pFunctionInfos->pLinkListParams->Head)
                 || (BadThisLocal)
               )
                // add ','
                CTextFile::WriteText(this->hMonitoringFile,_T(","));

            pParamInfos=(CTypeInfos*)pItemParam->ItemData;

            pParamInfos->GetPrettyName(sz,2*MAX_PATH);
            CTextFile::WriteText(this->hMonitoringFile,sz);

            if ((pParamInfos->bUserDefineType)
                &&(pParamInfos->Size!=sizeof(PBYTE))
                )
            {
                _stprintf(sz,_T("%u"),pParamInfos->Size);
                switch (pParamInfos->PointerLevel)
                {
                case 0:
                    CTextFile::WriteText(this->hMonitoringFile,_T(":DataSize="));
                    CTextFile::WriteText(this->hMonitoringFile,sz);
                    break;
                case 1:
                    CTextFile::WriteText(this->hMonitoringFile,_T(":PointedDataSize="));
                    CTextFile::WriteText(this->hMonitoringFile,sz);
                    break;
                }
            }
        }

        // add ');'
        CTextFile::WriteText(this->hMonitoringFile,_T(");"));
    }
    //////////////////////////////////////////
    //      options
    //////////////////////////////////////////

    // if return is HRESULT add |FailureIfNegativeRet option
    if (pFunctionInfos->pReturnInfos)
    {
        if ((pFunctionInfos->pReturnInfos->BaseType==btHresult)
            && (pFunctionInfos->pReturnInfos->PointerLevel==0)
            )
        {
            CTextFile::WriteText(this->hMonitoringFile,SPLITTER);
            CTextFile::WriteText(this->hMonitoringFile,_T("FailureIfNegativeRet"));
        }
    }
    
    //////////////////////////////////////////
    // 4) add end of line for next definition
    //////////////////////////////////////////
    CTextFile::WriteText(this->hMonitoringFile,_T("\r\n"));
}

//-----------------------------------------------------------------------------
// Name: GenerateMonitoringFile
// Object: generate a monitoring file for WinAPIOverride
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CDebugInfosUI::GenerateMonitoringFile()
{
    BOOL bRet;
    TCHAR szFile[MAX_PATH];
    TCHAR sz[2*MAX_PATH];
    TCHAR* pszFileName;
    TCHAR* psz;
    if (!this->pDebugInfos)
    {
        this->ReportError(_T("Please open a file first"));
        return FALSE;
    }
    _tcscpy(szFile,this->pDebugInfos->GetFileName());
    pszFileName=CStdFileOperations::GetFileName(szFile);
    _tcscpy(sz,pszFileName);
    pszFileName=sz;

    if (!this->pPE)
    {
        this->ReportError(_T("Error parsing PE, can't generate monitoring file"));
        return FALSE;
    }
    if (this->pPE->IsNET())
    {
        this->ReportError(_T(".NET binary : use Monitoring File Builder software instead to generate monitoring file !"));
        return FALSE;        
    }

    // empty list of types generated
    this->TypesGenerated.Clear();

    // forge user types directory and monitoring file name
    CStdFileOperations::GetAppPath(szFile,MAX_PATH);
    _tcscpy(this->UserTypePath,szFile);
    _tcscat(this->UserTypePath,USER_TYPES_FILES_PATH);
    _tcscat(UserTypePath,pszFileName);
    _tcscat(UserTypePath,_T("\\"));

    psz=CStdFileOperations::GetFileExt(pszFileName);
    if (_tcsicmp(psz,_T("exe"))==0)
        this->FileToMonitorType=FileToMonitorType_EXE;
    else // dll, ocx
        this->FileToMonitorType=FileToMonitorType_DLL;

    CStdFileOperations::ChangeFileExt(pszFileName,_T("txt"));
    _tcscat(szFile,MONITORING_FILES_PATH);
    _tcscat(szFile,pszFileName);

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("Text File (*.txt)\0*.txt\0All (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("txt");
    ofn.lpstrFile=szFile;
    ofn.nMaxFile=MAX_PATH;

    if (!GetSaveFileName(&ofn))
        return FALSE;

    CStdFileOperations::CreateDirectoryForFile(szFile);
    if (!CTextFile::CreateTextFile(szFile,&this->hMonitoringFile))
    {
        _stprintf(sz,_T("Error creating file %s"),szFile);
        this->ReportError(sz);
        return FALSE;
    }

    this->NetWarningReported=FALSE;
    bRet=this->pTreeview->Parse(
                            this->pTreeview->GetRoot(),
                            2,
                            CDebugInfosUI::GenerateMonitoringParseCallBackStatic,
                            this
                            );

    CloseHandle(this->hMonitoringFile);

    // if monitoring file generation is successful
    if (bRet)
    {
        // query if user want to view/modify generated file
        if (MessageBox(this->hWndDialog,
            _T("Monitoring file successfully generated\r\nDo you want to open it now to check it or make some adjustments ?"),
            _T("Information"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)
            ==IDYES)
        {
            // launch default .txt editor
            if (((int)ShellExecute(NULL,_T("open"),szFile,NULL,NULL,SW_SHOWNORMAL))<32)
            {
                MessageBox(this->hWndDialog,_T("Error launching default editor application"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            }
        }
    }

    return bRet;
}

void CDebugInfosUI::GenerateUserTypesInfosForFunction(CFunctionInfos* pFunctionInfos)
{
    CLinkListItem* pItemParam;
    CTypeInfos* pTypeInfos;

    IDiaEnumSymbols* pEnumSymbols;
    IDiaSymbol* pSymbol;
    WCHAR* SymbolName;
    ULONG CountElements = 0;
    HRESULT hResult;
    BOOL bFound;
    LONG Count;

    // for each parameter
    for (pItemParam=pFunctionInfos->pLinkListParams->Head;pItemParam;pItemParam=pItemParam->NextItem)
    {
        pTypeInfos=(CTypeInfos*)pItemParam->ItemData;

        // we are only interested by enum / struct / union
        if (    (!pTypeInfos->bEnum )
             && (!pTypeInfos->bUserDefineType)
           )
           continue;

        // if TypeName is empty
        if (!*pTypeInfos->TypeName)
            continue;

        // at this point we should have only enum union or struct
#if (defined(UNICODE)||defined(_UNICODE))
        SymbolName = pTypeInfos->TypeName;
#else
        CAnsiUnicodeConvert::AnsiToUnicode(pTypeInfos->TypeName,&SymbolName);
#endif

        // search for Type Name in global symbols table
        bFound = TRUE;
        hResult = this->pDebugInfos->pDiaGlobalSymbol->findChildren(SymTagUDT,SymbolName,nsCaseSensitive,&pEnumSymbols);

#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
        free(SymbolName);
#endif

        if ( (hResult != S_OK) || (pEnumSymbols ==NULL) )
        {
            bFound = FALSE;
        }
        if (bFound)
        {
            hResult = pEnumSymbols->get_Count(&Count);
            if (hResult == S_OK)
            {
                bFound = (Count!=0);
                if (Count!=1)
                {
                    TCHAR Msg[MAX_PATH];
                    _stprintf(Msg,_T("Multiple definitions found for type %s : no generation done for these types"),pTypeInfos->TypeName);
                    this->ReportError(Msg);
                }
            }
        }
        if (!bFound)
        {
#ifdef _DEBUG
            {
                TCHAR Msg[MAX_PATH];
                _sntprintf(Msg,MAX_PATH,_T("Type name %s not found in global symbols"),pTypeInfos->TypeName);
                OutputDebugString(Msg);
            }
#endif
            pEnumSymbols->Release();
            continue;
        }
        // there should be only one loop
        // for(;;) 
        {
            pSymbol = NULL;
            hResult = pEnumSymbols->Next(1, &pSymbol, &CountElements);
            if ( (hResult == S_OK) || (pSymbol !=NULL) || (CountElements == 1) )
            {
                // generate user type file
                this->GenerateUserDataType(pSymbol);

                pSymbol->Release();
            }
        }
        pEnumSymbols->Release();
    }
}

void CDebugInfosUI::GenerateUserDataType(IDiaSymbol* pSymbol)
{
    CUserTypesGenerator UserTypeGenerator(pSymbol,this->UserTypePath,&TypesGenerated);
    UserTypeGenerator.Generate();
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CDebugInfosUI_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CDebugInfosUI_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CDebugInfosUI_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CDebugInfosUI_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnSize()
{

    RECT RectWindow;
    RECT RectGroup;
    RECT Rect;
    HWND hWnd;
    POINT Point;
    DWORD SpaceBetweenControls;

    // get window Rect
    GetClientRect(this->hWndDialog,&RectWindow);

    ///////////////
    // search Group
    ///////////////
    hWnd=GetDlgItem(this->hWndDialog,IDC_GROUP_FIND);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hWnd,&RectGroup);
    SpaceBetweenControls=RectGroup.left;
    Point.x=RectGroup.left;
    Point.y=RectWindow.bottom-(RectGroup.bottom-RectGroup.top)-SpaceBetweenControls;
    CDialogHelper::MoveGroupTo(this->hWndDialog,hWnd,&Point);
    // update RectGroup
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hWnd,&RectGroup);

    ///////////////
    // treeview 
    ///////////////
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->pTreeview->GetControlHandle(),&Rect);
    SetWindowPos(this->pTreeview->GetControlHandle(),HWND_NOTOPMOST,
        0,0,
        (RectWindow.right-RectWindow.left)-2*SpaceBetweenControls,
        (RectGroup.top-RectWindow.top)-Rect.top-SpaceBetweenControls,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndDialog);

    if (this->pToolbar)    
        this->pToolbar->Autosize();
}

//-----------------------------------------------------------------------------
// Name: NoItemFoundMessage
// Object: show a Not Found Item Message box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::NoItemFoundMessage()
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
void CDebugInfosUI::Find()
{
    if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_BY_NAME)==BST_CHECKED)
    {
        // search by name
        HTREEITEM hItem;
        TCHAR* psz;
        DWORD Size;
        HWND hWndTextFind=GetDlgItem(this->hWndDialog,IDC_EDIT_FIND);
        Size=GetWindowTextLength(hWndTextFind)+3;// 1 for \0, 1 for prefix '*', 1 for suffix '*'
        psz=(TCHAR*)_alloca(Size);
        *psz=0;
        _tcscpy(psz,_T("*"));
        GetWindowText(hWndTextFind,&psz[1],Size-2);
        _tcscat(psz,_T("*"));

        this->SetWaitCursor(TRUE);
        if (!this->pTreeview->FindAtSpecifiedDepth(psz,1,&hItem))
        {
            this->NoItemFoundMessage();
            this->SetWaitCursor(FALSE);
            return;
        }
        CFunctionInfos* pFunctionInfos;
        // assume item is a function by querying user data
        this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
        if (pFunctionInfos)// if a function
            this->pTreeview->SetSelectedItem(hItem);
        else
            this->FindNext();
        
        this->SetWaitCursor(FALSE);
    }
    else
    {
        /////////////////////
        // get function RVA
        /////////////////////
        TCHAR sz[MAX_PATH];
        PBYTE FunctionRVA; // relative from base address
        if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_BY_VIRTUAL_ADDRESS)==BST_CHECKED)
        {
            PBYTE FunctionVA;
            GetDlgItemText(this->hWndDialog,IDC_EDIT_FIND_BY_VIRTUAL_ADDRESS,sz,MAX_PATH);
            sz[MAX_PATH-1]=0;
            if (!CStringConverter::StringToPBYTE(sz,&FunctionVA))
            {
                this->ReportError(_T("Bad address format"));
                return;
            }
            if (!this->pPE)
            {
                this->ReportError(_T("Error parsing PE"));
                return;
            }
            FunctionRVA=FunctionVA-this->pPE->NTHeader.OptionalHeader.ImageBase;
        }
        else
        {
            GetDlgItemText(this->hWndDialog,IDC_EDIT_FIND_BY_RELATIVE_VIRTUAL_ADDRESS,sz,MAX_PATH);
            sz[MAX_PATH-1]=0;
            if (!CStringConverter::StringToPBYTE(sz,&FunctionRVA))
            {
                this->ReportError(_T("Bad address format"));
                return;
            }
        }

        if (!this->pDebugInfos->FindFunctionByRVA((ULONGLONG) FunctionRVA,
                                                  &this->pFunctionInfosFindInTreeByAddress))
        {
            this->ReportError(_T("No function found"));
            return;
        }
        /////////////////////
        // find in treeview
        /////////////////////
        this->SetWaitCursor(TRUE);
        this->pTreeview->ParseSpecifiedDepth(this->pTreeview->GetRoot(),1,CDebugInfosUI::FindInTreeByAddressParseCallBackStatic,this);
        this->SetWaitCursor(FALSE);

        delete this->pFunctionInfosFindInTreeByAddress;
    }
}

BOOL CDebugInfosUI::FindInTreeByAddressParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    return ((CDebugInfosUI*)UserParam)->FindInTreeByAddressParseCallBack(hItem);
}
BOOL CDebugInfosUI::FindInTreeByAddressParseCallBack(HTREEITEM hItem)
{
    CFunctionInfos* pFunctionInfos;
    // get item user data
    this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
    // check item user data (MUST BE FIELD ONLY FOR FUNCTIONS)
    if (!pFunctionInfos)
        // continue parsing
        return TRUE;
    if (IsBadReadPtr(pFunctionInfos,sizeof(CFunctionInfos)))
        // continue parsing
        return TRUE;

    // if item has been found
    if (pFunctionInfos->RelativeVirtualAddress==this->pFunctionInfosFindInTreeByAddress->RelativeVirtualAddress)
    {
        // select it
        this->pTreeview->SetSelectedItem(hItem);
        // stop parsing
        return FALSE;
    }
    // else
    // continue parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the previous item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::FindPrevious()
{
    CFunctionInfos* pFunctionInfos;
    HTREEITEM hItem;
    TCHAR* psz;
    DWORD Size;
    HWND hWndTextFind=GetDlgItem(this->hWndDialog,IDC_EDIT_FIND);
    Size=GetWindowTextLength(hWndTextFind)+3;// 1 for \0, 1 for prefix '*', 1 for suffix '*'
    psz=(TCHAR*)_alloca(Size);
    *psz=0;
    _tcscpy(psz,_T("*"));
    GetWindowText(hWndTextFind,&psz[1],Size-2);
    _tcscat(psz,_T("*"));

    this->SetWaitCursor(TRUE);
    hItem=this->pTreeview->GetSelectedItem();

FindAgain:
    if (!this->pTreeview->FindPreviousAtSpecifiedDepth(psz,hItem,1,&hItem))
    {
        this->NoItemFoundMessage();
        this->SetWaitCursor(FALSE);
        return;
    }

    // assume item is a function by querying user data
    this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
    if (!pFunctionInfos)// if not a function
        goto FindAgain;

    this->SetWaitCursor(FALSE);
    this->pTreeview->SetSelectedItem(hItem);
}

//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the next item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::FindNext()
{
    HTREEITEM hItem;
    TCHAR* psz;
    DWORD Size;
    CFunctionInfos* pFunctionInfos;
    HWND hWndTextFind=GetDlgItem(this->hWndDialog,IDC_EDIT_FIND);
    Size=GetWindowTextLength(hWndTextFind)+3;// 1 for \0, 1 for prefix '*', 1 for suffix '*'
    psz=(TCHAR*)_alloca(Size);
    *psz=0;
    _tcscpy(psz,_T("*"));
    GetWindowText(hWndTextFind,&psz[1],Size-2);
    _tcscat(psz,_T("*"));

    this->SetWaitCursor(TRUE);
    hItem=this->pTreeview->GetSelectedItem();

FindAgain:
    if (!this->pTreeview->FindNextAtSpecifiedDepth(psz,hItem,1,&hItem))
    {
        this->NoItemFoundMessage();
        this->SetWaitCursor(FALSE);
        return;
    }

    // assume item is a function by querying user data
    this->pTreeview->GetItemUserData(hItem,(LPVOID*)&pFunctionInfos);
    if (!pFunctionInfos)// if not a function
        goto FindAgain;

    this->SetWaitCursor(FALSE);
    this->pTreeview->SetSelectedItem(hItem);
}

//-----------------------------------------------------------------------------
// Name: SetWaitCursor
// Object: Set cursor to wait cursor or to normal cursor depending bSet
// Parameters :
//     in  : BOOL bSet : TRUE to set wait cursor, FALSE to restore normal cursor
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::SetWaitCursor(BOOL bSet)
{
    if (bSet)
    {
        SetCursor(::LoadCursor(NULL,IDC_WAIT));
    }
    // force cursor update
    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(pt.x,pt.y);
}


//-----------------------------------------------------------------------------
// Name: OnDonation
// Object: query to make donation
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnDonation()
{
    BOOL bEuroCurrency=FALSE;
    HKEY hKey;
    wchar_t pszCurrency[2];
    DWORD Size=2*sizeof(wchar_t);
    memset(pszCurrency,0,Size);
    TCHAR pszMsg[3*MAX_PATH];

    // check that HKEY_CURRENT_USER\Control Panel\International\sCurrency contains the euro symbol
    // retrieve it in unicode to be quite sure of the money symbol
    if (RegOpenKeyEx(HKEY_CURRENT_USER,_T("Control Panel\\International"),0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS)
    {
        // use unicode version only to make string compare
        if (RegQueryValueExW(hKey,L"sCurrency",NULL,NULL,(LPBYTE)pszCurrency,&Size)==ERROR_SUCCESS)
        {
            if (wcscmp(pszCurrency,L"€")==0)
                bEuroCurrency=TRUE;
        }
        // close open key
        RegCloseKey(hKey);
    }
    // yes, you can do it if u don't like freeware and open sources soft
    // but if you make it, don't blame me for not releasing sources anymore
    _tcscpy(pszMsg,_T("https://www.paypal.com/cgi-bin/webscr?cmd=_xclick&business=jacquelin.potier@free.fr")
        _T("&item_name=Donation%20for%20DebugInfosViewer&return=http://jacquelin.potier.free.fr/winapioverride32/"));
    // in case of euro monetary symbol
    if (bEuroCurrency)
        // add it to link
        _tcscat(pszMsg,_T("&currency_code=EUR"));

    // open donation web page
    if (((int)ShellExecute(NULL,_T("open"),pszMsg,NULL,NULL,SW_SHOWNORMAL))<33)
        // display error msg in case of failure
        MessageBox(this->hWndDialog,_T("Error Opening default browser. You can make a donation going to ")
        _T("http://jacquelin.potier.free.fr"),
        _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}
//-----------------------------------------------------------------------------
// Name: OnAbout
// Object: show about dlg box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfosUI::OnAbout()
{
    ShowAboutDialog(this->hInstance,this->hWndDialog);
}