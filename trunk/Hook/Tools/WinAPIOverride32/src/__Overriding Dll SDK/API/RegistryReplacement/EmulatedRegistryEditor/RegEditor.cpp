/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#include "RegEditor.h"
#include "CommandLineParsing.h"

#ifndef _countof
    #define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

BOOL CRegEditorGUI::SaveRegistry(TCHAR* const FileName)
{
    TCHAR UsedFileName[MAX_PATH];
    if (!FileName)
    {
        if (!this->QuerySavingFileName(UsedFileName))
            return FALSE;
    }
    else
    {
        _tcsncpy(UsedFileName,FileName,MAX_PATH);
        UsedFileName[MAX_PATH-1]=0;
    }
    
    // query new file while saving failure occurs
    // until saving success
    while ( 
            ( !this->pRegistry->Save(UsedFileName) )
          )
    {
        if (!this->QuerySavingFileName(UsedFileName))
            return FALSE;
    }

    // update currently used file name with UsedFileName
    _tcscpy(this->CurrentRegistryFileName,UsedFileName);
    // we just finished saving --> reset bChangesSinceLastSaving flag
    this->bChangesSinceLastSaving = FALSE;
    
    return TRUE;
}
// FileName size must be at least MAX_PATH in TCHAR count
BOOL CRegEditorGUI::QuerySavingFileName(TCHAR* FileName)
{
    OPENFILENAME ofn;
    
    // save file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->GetControlHandle();
    ofn.hInstance=this->GetInstance();
    ofn.lpstrFilter=_T("xml\0*.xml\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("xml");
    *FileName=0;
    ofn.lpstrFile=FileName;
    ofn.nMaxFile=MAX_PATH;
    
    return ::GetSaveFileName(&ofn);
}
BOOL CRegEditorGUI::LoadRegistry(TCHAR* const FileName)
{
    // check saving before changing CurrentRegistryFileName
    this->CheckSaving();
    this->bChangesSinceLastSaving = FALSE;

    // clean screen before removing registry content to avoid onchange errors
    this->pListviewValues->Clear();
    this->pTreeViewKeys->Clear();

    // free current loaded registry if any
    if (this->pRegistry)
        delete this->pRegistry;


    _tcscpy(this->CurrentRegistryFileName,FileName);
    this->pRegistry = new EmulatedRegistry::CEmulatedRegistry();
    if (!this->pRegistry->Load(this->CurrentRegistryFileName))
    {
        TCHAR Msg[2*MAX_PATH];
        _sntprintf(Msg,2*MAX_PATH,_T("Error loading registry %s"),this->CurrentRegistryFileName);
        this->ReportError(Msg);
        return FALSE;
    }
    this->pListviewValues->RemoveAllColumns();
    if (this->pRegistry->pOptions->GetSpyMode())
    {
        this->pListviewValues->SetColumn(0,_T("Name"),200,LVCFMT_LEFT);
        this->pListviewValues->AddColumn(_T("Type"),100,LVCFMT_LEFT);
        this->pListviewValues->AddColumn(_T("Access"),50,LVCFMT_LEFT);
        this->pListviewValues->AddColumn(_T("Content"),300,LVCFMT_LEFT);
    }
    else
    {
        this->pListviewValues->SetColumn(0,_T("Name"),200,LVCFMT_LEFT);
        this->pListviewValues->AddColumn(_T("Type"),100,LVCFMT_LEFT);
        this->pListviewValues->AddColumn(_T("Content"),300,LVCFMT_LEFT);
    }
    // sort values by name
    this->pListviewValues->Sort(0,TRUE);

    this->EnableEditing(!this->pRegistry->pOptions->GetSpyMode());

    // this->pTreeViewKeys->Insert(NULL,_T("Emulated Registries"));

    // for each host
    std::vector<EmulatedRegistry::CKeyReplace*>::iterator iHostKey;
    for (iHostKey = this->pRegistry->pRootKey->SubKeys.begin();iHostKey!=this->pRegistry->pRootKey->SubKeys.end();iHostKey++)
    {
        EmulatedRegistry::CHostKey* pHostKey = (EmulatedRegistry::CHostKey*)*iHostKey;
        // this->ShowKeyContent(this->pTreeViewKeys->GetRoot(),pHostKey);
        this->ShowKeyContent(NULL,pHostKey);
    }

    return TRUE;
}

void CRegEditorGUI::UpdateKeyIcon(HTREEITEM hTreeItemKey,EmulatedRegistry::CKeyReplace* pKey)
{
    // set treeview icon according to key type (host, emulated, not emulated)
    int IconIndex;
    if (pKey->IsHostKey())
    {
        IconIndex = this->TreeViewIconIndexRegistry;
    }
    else if (pKey->Emulated)
    {
        IconIndex = this->TreeViewIconIndexEmulatedKey;
    }
    else
    {
        IconIndex = this->TreeViewIconIndexNotEmulatedKey;
    }

    this->pTreeViewKeys->SetItemIconIndex(hTreeItemKey,IconIndex);
}

void CRegEditorGUI::ShowKeyContent(HTREEITEM hTreeItem,EmulatedRegistry::CKeyReplace* pKey)
{
    HTREEITEM hTreeItemKey = this->pTreeViewKeys->Insert(hTreeItem,TVI_SORT,(TCHAR*)pKey->KeyName.c_str(),(LPARAM)pKey);
    
    this->UpdateKeyIcon(hTreeItemKey,pKey);


    // for each subkey
    std::vector<EmulatedRegistry::CKeyReplace*>::iterator iSubKeys;
    for (iSubKeys = pKey->SubKeys.begin();iSubKeys!=pKey->SubKeys.end();iSubKeys++)
    {
        EmulatedRegistry::CKeyReplace* pChildKey = *iSubKeys;
        this->ShowKeyContent(hTreeItemKey,pChildKey);
    }
}

#define MAP_ENTRY(x) { x, _T(#x)}

//#define REG_NONE                    ( 0 )   // No value type
//#define REG_SZ                      ( 1 )   // Unicode nul terminated string
//#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
//                                            // (with environment variable references)
//#define REG_BINARY                  ( 3 )   // Free form binary
//#define REG_DWORD                   ( 4 )   // 32-bit number
//#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
//#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
//#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
//#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
//#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
//#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
//#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )
//#define REG_QWORD                   ( 11 )  // 64-bit number
//#define REG_QWORD_LITTLE_ENDIAN     ( 11 )  // 64-bit number (same as REG_QWORD)
#define REG_SZ_ANSI 12 // not existing type only to detect bad REG_SZ
static const struct { DWORD Value ; TCHAR* Name;} c_KeyTypeMap[] =
{
    MAP_ENTRY(REG_NONE),
    MAP_ENTRY(REG_SZ),
    MAP_ENTRY(REG_EXPAND_SZ),
    MAP_ENTRY(REG_BINARY),
    MAP_ENTRY(REG_DWORD), // same as REG_DWORD_LITTLE_ENDIAN
    //MAP_ENTRY(REG_DWORD_LITTLE_ENDIAN),
    MAP_ENTRY(REG_DWORD_BIG_ENDIAN),
    MAP_ENTRY(REG_LINK),
    MAP_ENTRY(REG_MULTI_SZ),
    MAP_ENTRY(REG_RESOURCE_LIST),
    MAP_ENTRY(REG_FULL_RESOURCE_DESCRIPTOR),
    MAP_ENTRY(REG_RESOURCE_REQUIREMENTS_LIST),
    MAP_ENTRY(REG_QWORD), // same as REG_QWORD_LITTLE_ENDIAN
    //MAP_ENTRY(REG_QWORD_LITTLE_ENDIAN),
    MAP_ENTRY(REG_SZ_ANSI)
}; 

TCHAR* CRegEditorGUI::TypeToString(DWORD Type)
{
    for (SIZE_T Cnt = 0;Cnt<_countof(c_KeyTypeMap);Cnt++)
    {
        if (c_KeyTypeMap[Cnt].Value == Type)
        {
            return c_KeyTypeMap[Cnt].Name;
        }
    }
    // not found
    return _T("");
}

void CRegEditorGUI::UpdateKeyValue(int ListViewItemIndex,EmulatedRegistry::CKeyValue* pValue)
{
    BOOL bStringIcon=FALSE;
    int SubItemIndex = 0;

    // set Name
    this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,(TCHAR*)pValue->Name.c_str());
    SubItemIndex++;
    
    // set type
    this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,TypeToString(pValue->Type));
    SubItemIndex++;

    if (this->pRegistry->pOptions->GetSpyMode())
    {
        TCHAR szAccess[MAX_PATH];
        *szAccess=0;
        if (pValue->EmulatedRegistry_Flag & CKeyValue_EmulatedRegistry_Flag_ACCESS_READ)
            _tcscat(szAccess,_T("R"));
        if (pValue->EmulatedRegistry_Flag & CKeyValue_EmulatedRegistry_Flag_ACCESS_WRITE)
            _tcscat(szAccess,_T("W"));

        this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,szAccess);
        SubItemIndex++;
    }

    
    // set content
    switch (pValue->Type)
    {
    default:
        {
            TCHAR* str = CStrToHex::ByteArrayToStrHexArray(pValue->Buffer,pValue->BufferSize,_T(" "));
            if (str)
            {
                this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,str);
                SubItemIndex++;
                delete[] str;
            }
        }
        break;


    // case REG_DWORD: // REG_DWORD == REG_DWORD_LITTLE_ENDIAN
    case REG_DWORD_LITTLE_ENDIAN:
        {
            TCHAR str[64];
            _stprintf(str,_T("0x%.8X"),*((DWORD*)pValue->Buffer));
            this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,str);
            SubItemIndex++;
        }
        
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        {
            bStringIcon = TRUE;
            TCHAR* str=NULL;
            BOOL bAnsi=FALSE;
            if (pValue->GetStringValue(&str,&bAnsi))
            {
                this->pListviewValues->SetItemText(ListViewItemIndex,SubItemIndex,(TCHAR*)str);
                delete [] str;
            }
            SubItemIndex++;

        }
        break;
    }

    if (bStringIcon)
        this->pListviewValues->SetItemIconIndex(ListViewItemIndex,this->ListViewIconIndexRegistryString);
    else
        this->pListviewValues->SetItemIconIndex(ListViewItemIndex,this->ListViewIconIndexRegistryBinary);

}



void CRegEditorGUI::ShowKeyValues(EmulatedRegistry::CKeyReplace* pKey)
{
    // clean listview
    this->pListviewValues->Clear();

    std::tstring KeyFullName;
    std::tstring KeyNameWithHostName;
    if (pKey->GetKeyPath(KeyFullName))
    {
        KeyNameWithHostName = pKey->GetHostKey()->KeyName;
        KeyNameWithHostName+=_T("\\");
        KeyNameWithHostName+=KeyFullName;
        this->pStatusBar->SetText(0,(TCHAR*)KeyNameWithHostName.c_str());
    }
    

    // in case of host key
    if (pKey->IsHostKey())
    {
// this->IsHostKey = TRUE;
        // host key has no value
        // instead show options (list of emulated key, write only, ...)
// to implement 
// Warning : OnModifyValue, OnAddValue, OnRemoveSelectedValues must be implement too for this case

        // change columns names
        return;
    }
    else
    {
// to implement    
        // restore columns names
    }

    this->EnableKeyEdition(pKey->Emulated);
    // if (pKey->Emulated) // for spy mode result files, keys contain value but are marked as not emulated
    {
        int ItemIndex;
        // for each value, save value
        std::vector<EmulatedRegistry::CKeyValue*>::iterator iValues;
        for (iValues = pKey->Values.begin();iValues!=pKey->Values.end();iValues++)
        {
            EmulatedRegistry::CKeyValue* pValue = *iValues;
            // add name
            ItemIndex = this->pListviewValues->AddItem((TCHAR*)pValue->Name.c_str(),(LPVOID)pValue,FALSE);

            this->UpdateKeyValue(ItemIndex,pValue);
        }
    }

    this->pListviewValues->ReSort();
}
void CRegEditorGUI::EnableKeyEdition(BOOL bEnable)
{
    // avoid editing in case of spy mode
    if (this->pRegistry->pOptions->GetSpyMode())
    {
        if (bEnable)
            return;
        this->EnableDlgItem(IDC_BUTTON_ADD_KEY,bEnable);
        this->EnableDlgItem(IDC_BUTTON_MODIFY_SELECTED_KEY,bEnable);
    }
    else
    {
        // keys are editable in all cases
        this->EnableDlgItem(IDC_BUTTON_ADD_KEY,TRUE);
        this->EnableDlgItem(IDC_BUTTON_MODIFY_SELECTED_KEY,TRUE);
    }

    this->EnableDlgItem(IDC_BUTTON_ADD_VALUE,bEnable);
    this->EnableDlgItem(IDC_EDIT_VALUE_NAME,bEnable);
    this->EnableDlgItem(IDC_EDIT_VALUE_CONTENT,bEnable);
    this->EnableDlgItem(IDC_COMBO_VALUE_TYPE,bEnable);
    this->EnableDlgItem(IDC_BUTTON_MODIFY_SELECTED_VALUE,bEnable);
    this->EnableDlgItem(IDC_BUTTON_REMOVE_SELECTED_VALUES,bEnable);
}

void CRegEditorGUI::EnableEditing(BOOL bEnable)
{
    this->EnableKeyEdition(bEnable);

    this->EnableDlgItem(IDC_EDIT_KEY_NAME,bEnable);
    this->EnableDlgItem(IDC_EDIT_VALUE_NAME,bEnable);
    this->EnableDlgItem(IDC_EDIT_VALUE_CONTENT,bEnable);
    this->EnableDlgItem(IDC_COMBO_VALUE_TYPE,bEnable);
    this->EnableDlgItem(IDC_BUTTON_ADD_KEY,bEnable);
    this->EnableDlgItem(IDC_BUTTON_REMOVE_SELECTED_KEY,bEnable);
    this->EnableDlgItem(IDC_BUTTON_MODIFY_SELECTED_KEY,bEnable);
}
CRegEditorGUI::CRegEditorGUI()
{
    this->pRegistry = NULL;
    this->pTreeViewKeys = NULL;
    this->pListviewValues = NULL;
    this->pStatusBar = NULL;
    this->pToolbar=NULL;
    this->hWndComboValueType = 0;
    this->bChangesSinceLastSaving = FALSE;
    *this->CurrentRegistryFileName = 0;
    this->TreeViewIconIndexRegistry = 0;
    this->TreeViewIconIndexEmulatedKey = 0;
    this->TreeViewIconIndexNotEmulatedKey = 0;
    this->ListViewIconIndexRegistryBinary = 0;
    this->ListViewIconIndexRegistryString = 0;
    this->EnableDragAndDrop(TRUE);
}

CRegEditorGUI::~CRegEditorGUI()
{
    if (this->pRegistry)
        delete this->pRegistry;
}

void CRegEditorGUI::OnInit()
{
    this->pToolbar=new CToolbar(this->GetInstance(),this->GetControlHandle(),TRUE,TRUE,24,24);
    this->pToolbar->AddButton(IDC_BUTTON_LOAD,_T("Open"),IDI_ICON_OPEN,_T("Open an emulated registry file"));
    this->pToolbar->AddButton(IDC_BUTTON_SAVE,_T("Save"),IDI_ICON_SAVE,_T("Save emulated registry file"));
    this->pToolbar->AddButton(IDC_BUTTON_SAVE_AS,_T("Save As"),IDI_ICON_SAVE_AS,_T("Save emulated registry file into another file"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton( IDC_BUTTON_COPY_KEY_PATH,_T("Copy Key Path"),IDI_ICON_COPY,_T("Copy current key path to clipboard"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton( IDC_BUTTON_EXPAND,_T("Expand"),IDI_ICON_EXPAND,_T("Expand Tree View"));
    this->pToolbar->AddButton( IDC_BUTTON_COLLAPSE,_T("Collapse"),IDI_ICON_COLLAPSE,_T("Collapse Tree View"));

    this->pStatusBar=new CStatusbar(this->GetInstance(),this->GetControlHandle());

    this->pTreeViewKeys = new CTreeview(this->GetDlgItem(IDC_TREE_KEYS));
    this->pTreeViewKeys->SetSelectedItemChangedCallback(CRegEditorGUI::OnSelectedKeyChange,this);
    this->TreeViewIconIndexRegistry = this->pTreeViewKeys->AddIcon(this->GetInstance(),IDI_ICON_APP);
    this->TreeViewIconIndexEmulatedKey = this->pTreeViewKeys->AddIcon(this->GetInstance(),IDI_ICON_KEY_EMULATED);
    this->TreeViewIconIndexNotEmulatedKey = this->pTreeViewKeys->AddIcon(this->GetInstance(),IDI_ICON_KEY_NOT_EMULATED);

    this->pListviewValues = new CListview(this->GetDlgItem(IDC_LIST_VALUES));
    this->pListviewValues->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListviewValues->SetSelectItemCallback(CRegEditorGUI::OnSelectedValueChange,this);
    this->pListviewValues->AddColumn(_T("Name"),200,LVCFMT_LEFT);
    this->pListviewValues->AddColumn(_T("Type"),100,LVCFMT_LEFT);
    this->pListviewValues->AddColumn(_T("Content"),300,LVCFMT_LEFT);

    this->ListViewIconIndexRegistryBinary = this->pListviewValues->AddIcon(CListview::ImageListSmall,this->GetInstance(),IDI_ICON_REGISTRY_BINARY);
    this->ListViewIconIndexRegistryString = this->pListviewValues->AddIcon(CListview::ImageListSmall,this->GetInstance(),IDI_ICON_REGISTRY_STRING);
    
    this->hWndComboValueType= this->GetDlgItem(IDC_COMBO_VALUE_TYPE);
    // add type
    // Assume index correspond to type value !!! + !CBS_SORT 
    for (SIZE_T Cnt = 0;Cnt<_countof(c_KeyTypeMap);Cnt++)
    {
#if _DEBUG
        if (Cnt!=c_KeyTypeMap[Cnt].Value)
        {
            // Assume index correspond to type value !!! + !CBS_SORT 
            DebugBreak();
        }
#endif
        ComboBox_AddString(this->hWndComboValueType,c_KeyTypeMap[Cnt].Name);
    }


    CEmulatedRegistryEditorCommandLine CommandLineOptions;
    if (CommandLineOptions.ParseCommandLine())
    {
        if (*CommandLineOptions.FileName !=0 )
            this->LoadRegistry(CommandLineOptions.FileName);
    }
}
void CRegEditorGUI::OnClose()
{
    this->CheckSaving();

    delete this->pTreeViewKeys;
    delete this->pListviewValues;
    delete this->pStatusBar;
    delete this->pToolbar;
}

void CRegEditorGUI::OnSelectedValueChange(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    ((CRegEditorGUI*)UserParam)->OnSelectedValueChange(ItemIndex,SubItemIndex);
}
void CRegEditorGUI::OnSelectedValueChange(int ItemIndex,int SubItemIndex)
{
    UNREFERENCED_PARAMETER(SubItemIndex);

    EmulatedRegistry::CKeyValue* pValue;
    this->pListviewValues->GetItemUserData(ItemIndex,(LPVOID*)&pValue);
    if(!pValue)
        return;
    this->SetUIFromValueContent(pValue);
}

void CRegEditorGUI::OnSelectedKeyChange(LPNMTREEVIEW pChangeInfos,PVOID UserParam)
{
    ((CRegEditorGUI*)UserParam)->OnSelectedKeyChange(pChangeInfos);
}
void CRegEditorGUI::OnSelectedKeyChange(LPNMTREEVIEW pChangeInfos)
{
    EmulatedRegistry::CKeyReplace* pKey;
    this->pTreeViewKeys->GetItemUserData(pChangeInfos->itemNew.hItem,(LPVOID*)&pKey);
    if (!pKey)
        return;

    this->ShowKeyValues(pKey);
    this->SetDlgItemText(IDC_EDIT_KEY_NAME,(TCHAR*)pKey->KeyName.c_str());
}

EmulatedRegistry::CKeyReplace* CRegEditorGUI::GetSelectedKey()
{
    HTREEITEM hSelectedItem = this->pTreeViewKeys->GetSelectedItem();
    if (!hSelectedItem)
        return NULL;
    EmulatedRegistry::CKeyReplace* pKey;
    this->pTreeViewKeys->GetItemUserData(hSelectedItem,(LPVOID*)&pKey);
    return pKey;
}

void CRegEditorGUI::OnModifyKey()
{
    EmulatedRegistry::CKeyReplace* pKey = this->GetSelectedKey();
    if (!pKey)
    {
        this->ReportError(_T("Select a registry or key first"));
        return;
    }
    TCHAR NewKeyName[MAX_PATH];
    this->GetDlgItemText(IDC_EDIT_KEY_NAME,NewKeyName,MAX_PATH);
    
    // change key name in registry
    pKey->KeyName = NewKeyName;
    // as key name change, key emulated status may have been changed
    BOOL bOldemulatedState = pKey->Emulated;
    pKey->Emulated = pKey->IsEmulatedKey();

    // change key name in treeview
    HTREEITEM hTreeItem = this->pTreeViewKeys->GetSelectedItem();
    this->pTreeViewKeys->SetItemText(hTreeItem,NewKeyName);
    
    // update emulated key status icon if emulated status has change
    if (bOldemulatedState != pKey->Emulated)
        this->UpdateKeyIcon(hTreeItem,pKey);

    // we need to update the state of all subkey emulated state 
    // in case name changing change emulated state of subkeys
    this->pTreeViewKeys->Parse(hTreeItem,CRegEditorGUI::TreeSubKeyParsingToUpdateEmulatedState,this);

    this->bChangesSinceLastSaving = TRUE;
}

BOOL CRegEditorGUI::TreeSubKeyParsingToUpdateEmulatedState(HTREEITEM hItem,LPVOID UserParam)
{
    return ((CRegEditorGUI*)UserParam)->TreeSubKeyParsingToUpdateEmulatedState(hItem);
}
BOOL CRegEditorGUI::TreeSubKeyParsingToUpdateEmulatedState(HTREEITEM hItem)
{
    EmulatedRegistry::CKeyReplace* pKey;
    this->pTreeViewKeys->GetItemUserData(hItem,(LPVOID*)&pKey);
    if (!pKey)
        return TRUE; // return TRUE to continue parsing

    // as upper key name change, key emulated status may have been changed
    BOOL bOldemulatedState = pKey->Emulated;
    pKey->Emulated = pKey->IsEmulatedKey();

    // update emulated key status icon if emulated status has change
    if (bOldemulatedState != pKey->Emulated)
        this->UpdateKeyIcon(hItem,pKey);

    return TRUE; // return TRUE to continue parsing
}

void CRegEditorGUI::OnAddKey()
{
    EmulatedRegistry::CKeyReplace* pParentKey = this->GetSelectedKey();
    if (!pParentKey)
    {
        this->ReportError(_T("Select a registry or key first"));
        return;
    }
    
    TCHAR NewKeyName[MAX_PATH];
    this->GetDlgItemText(IDC_EDIT_KEY_NAME,NewKeyName,MAX_PATH);
    
    // add key to registry
    EmulatedRegistry::CKeyReplace* pNewKey = pParentKey->AddSubKey(NewKeyName);
    
    // add key to treeview
    HTREEITEM hTreeItem = this->pTreeViewKeys->Insert(this->pTreeViewKeys->GetSelectedItem(),TVI_SORT,NewKeyName,(LPARAM)pNewKey);
    this->UpdateKeyIcon(hTreeItem,pNewKey);

    this->bChangesSinceLastSaving = TRUE;
}
void CRegEditorGUI::OnRemoveSelectedKey()
{
    EmulatedRegistry::CKeyReplace* pKey = this->GetSelectedKey();
    if (!pKey)
    {
        this->ReportError(_T("Select a registry or key first"));
        return;
    }
    
    // remove key from treeview (do it first to avoid action on deleted memory)
    this->pTreeViewKeys->Remove(this->pTreeViewKeys->GetSelectedItem());
    
    // remove key from registry
    pKey->RemoveCurrentKey();
    this->bChangesSinceLastSaving = TRUE;
}

EmulatedRegistry::CKeyValue* CRegEditorGUI::GetSelectedValue()
{
    int iSelectedIndex = this->pListviewValues->GetSelectedIndex();
    if (iSelectedIndex<0)
        return NULL;
    EmulatedRegistry::CKeyValue* pValue;
    this->pListviewValues->GetItemUserData(iSelectedIndex,(LPVOID*)&pValue);
    return pValue;
}

void CRegEditorGUI::SetUIFromValueContent(EmulatedRegistry::CKeyValue* pValue)
{
    // set Name
    this->SetDlgItemText(IDC_EDIT_VALUE_NAME,(TCHAR*)pValue->Name.c_str());
    
    // set type
    ComboBox_SetCurSel(this->hWndComboValueType,pValue->Type);

    // set content
    switch (pValue->Type)
    {
    default:
        {
            TCHAR* str = CStrToHex::ByteArrayToStrHexArray(pValue->Buffer,pValue->BufferSize,_T(" "));
            if (str)
            {
                this->SetDlgItemText(IDC_EDIT_VALUE_CONTENT,str);
                delete[] str;
            }
        }
        break;


    // case REG_DWORD: // REG_DWORD == REG_DWORD_LITTLE_ENDIAN
    case REG_DWORD_LITTLE_ENDIAN:
        {
            TCHAR str[64];
            _stprintf(str,_T("0x%.8X"),*((DWORD*)pValue->Buffer));
            this->SetDlgItemText(IDC_EDIT_VALUE_CONTENT,str);
        }
        
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        {
            TCHAR* str=NULL;
            BOOL bAnsi=FALSE;
            if (pValue->GetStringValue(&str,&bAnsi))
            {
                this->SetDlgItemText(IDC_EDIT_VALUE_CONTENT,str);
                delete [] str;
            }
            else
                this->SetDlgItemText(IDC_EDIT_VALUE_CONTENT,_T(""));
            if (bAnsi)
            {
                ComboBox_SetCurSel(this->hWndComboValueType,REG_SZ_ANSI);
            }
        }
        break;
    }
}

void CRegEditorGUI::SetValueContentFromUI(EmulatedRegistry::CKeyValue* pValue)
{
    // get name
    TCHAR NewName[MAX_PATH];
    this->GetDlgItemText(IDC_EDIT_VALUE_NAME,NewName,MAX_PATH);  
    pValue->Name= NewName;
    
    // get type
    pValue->Type=ComboBox_GetCurSel(this->hWndComboValueType);
    
    // CStrToHex::

    int TextSize = this->GetDlgItemTextLength(IDC_EDIT_VALUE_CONTENT)+1;
    TCHAR* StrValueContent = new TCHAR[TextSize];
    if (!StrValueContent)
        return;
    this->GetDlgItemText(IDC_EDIT_VALUE_CONTENT,StrValueContent,TextSize);
    
    switch (pValue->Type)
    {
    default:
        {
            BYTE* Buffer;
            SIZE_T BufferSize;
            
            Buffer = CStrToHex::StrHexArrayToByteArray(StrValueContent,&BufferSize);
            if (Buffer)
            {
                delete[] pValue->Buffer;
                pValue->Buffer = Buffer;
                pValue->BufferSize = BufferSize;
            }
        }
        break;


    // case REG_DWORD: // REG_DWORD == REG_DWORD_LITTLE_ENDIAN
    case REG_DWORD_LITTLE_ENDIAN:
        {
            if (_stscanf(StrValueContent,_T("0x%X"),(DWORD*)pValue->Buffer)==0)
                _stscanf(StrValueContent,_T("%u"),(DWORD*)pValue->Buffer);
        }
        
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        {
            WCHAR* str=new WCHAR[TextSize];
            if (str)
            {
                CAnsiUnicodeConvert::TcharToUnicode(StrValueContent,str,TextSize);
                delete[] pValue->Buffer;
                pValue->Buffer = (PBYTE)str;
                pValue->BufferSize = TextSize*sizeof(WCHAR);
            }
        }
        break;
    case REG_SZ_ANSI:
        {
            pValue->Type = REG_SZ;
            CHAR* str=new CHAR[TextSize];
            if (str)
            {
                CAnsiUnicodeConvert::TcharToAnsi(StrValueContent,str,TextSize);
                delete[] pValue->Buffer;
                pValue->Buffer = (PBYTE)str;
                pValue->BufferSize = TextSize*sizeof(CHAR);
            }
        }
        break;
    } 

    delete[] StrValueContent;
}

void CRegEditorGUI::OnModifyValue()
{
    EmulatedRegistry::CKeyValue* pValue = this->GetSelectedValue();
    if (!pValue)
    {
        this->ReportError(_T("Select a value first"));
        return;
    }
    // update pValue
    this->SetValueContentFromUI(pValue);
    
    // update listview
    int iSelectedIndex = this->pListviewValues->GetSelectedIndex();
    this->UpdateKeyValue(iSelectedIndex,pValue);

    this->bChangesSinceLastSaving = TRUE;
}
void CRegEditorGUI::OnAddValue()
{
    EmulatedRegistry::CKeyReplace* pKey = this->GetSelectedKey();
    if (!pKey)
    {
        this->ReportError(_T("Select a registry or key first"));
        return;
    }
    // get name
    TCHAR NewName[MAX_PATH];
    this->GetDlgItemText(IDC_EDIT_VALUE_NAME,NewName,MAX_PATH);  
    EmulatedRegistry::CKeyValue* pValue = pKey->GetOrAddValue(NewName);
    
    // update pValue
    this->SetValueContentFromUI(pValue);
    
    // add to listview
    int ItemIndex = this->pListviewValues->AddItem(NewName,(LPVOID)pValue,TRUE);
    this->UpdateKeyValue(ItemIndex,pValue);

    this->bChangesSinceLastSaving = TRUE;
}
void CRegEditorGUI::OnRemoveSelectedValues()
{
    EmulatedRegistry::CKeyValue* pValue;
    EmulatedRegistry::CKeyReplace* pKey = this->GetSelectedKey();
    if (!pKey)
        return;
   
    // warning do it in reverse order to avoid indexes changes
    for(SSIZE_T Cnt = this->pListviewValues->GetItemCount(); Cnt>=0; Cnt--)
    {
        // if item is selected
        if (this->pListviewValues->IsItemSelected(Cnt))
        {
            // get associated data
            this->pListviewValues->GetItemUserData(Cnt,(LPVOID*)&pValue);
            
            // remove from listview first
            this->pListviewValues->RemoveItem(Cnt);
            
            // free memory next
            if (pValue)
                pKey->RemoveValue((TCHAR*)pValue->Name.c_str());
        }
    }

    this->bChangesSinceLastSaving = TRUE;
}

void CRegEditorGUI::OnLoad()
{
    TCHAR FileName[MAX_PATH];
    *FileName=0;
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->GetControlHandle();
    ofn.hInstance=this->GetInstance();
    ofn.lpstrFilter=_T("xml\0*.xml\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=FileName;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrTitle=_T("Select Registry File");

    // get file name
    if (::GetOpenFileName(&ofn))
    {
        this->LoadRegistry(FileName);
    }
}

void CRegEditorGUI::OnSave()
{
    if (this->SaveRegistry(this->CurrentRegistryFileName))
    {
        this->ReportMessage(_T("Save Sucessfully Completed"),ReportMessageType_INFO);
    }
}

void CRegEditorGUI::OnSaveAs()
{
    if (this->SaveRegistry(NULL))
    {
        this->ReportMessage(_T("Save Sucessfully Completed"),ReportMessageType_INFO);
    }
}

void CRegEditorGUI::OnCopyKeyPath()
{
    EmulatedRegistry::CKeyReplace* pKey = this->GetSelectedKey();
    if (!pKey)
    {
        this->ReportError(_T("Select a registry or key first"));
        return;
    }

    std::tstring str;
    if (pKey->GetKeyPath(str))
    {
        CClipboard::CopyToClipboard(this->GetControlHandle(),(TCHAR*)str.c_str());
    }
}

void CRegEditorGUI::CheckSaving()
{
    // if changes occurred since last saving
    if (this->bChangesSinceLastSaving)
    {
        // query saving
        if (::MessageBox(this->GetControlHandle(),_T("You registry hasn't been saved since last change\r\nDo you want to save it ?"),_T("Question"),MB_ICONQUESTION|MB_YESNO) == IDYES)
        {
            this->SaveRegistry(this->CurrentRegistryFileName);
        }
    }
}

void CRegEditorGUI::OnDropFiles(WPARAM wParam,LPARAM lParam)
{
    HDROP hDrop= (HDROP)wParam;
    TCHAR pszFileName[MAX_PATH];
    ::DragQueryFile(hDrop, 0,pszFileName, MAX_PATH);
    this->LoadRegistry(pszFileName);
    ::DragFinish(hDrop);
}

void CRegEditorGUI::OnCommand(WPARAM wParam,LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_BUTTON_ADD_VALUE:
        this->OnAddValue();
        break;
    case IDC_BUTTON_MODIFY_SELECTED_VALUE:
        this->OnModifyValue();
        break;
    case IDC_BUTTON_REMOVE_SELECTED_VALUES:
        this->OnRemoveSelectedValues();
        break;

    case IDC_BUTTON_ADD_KEY:
        this->OnAddKey();
        break;
    case IDC_BUTTON_MODIFY_SELECTED_KEY:
        this->OnModifyKey();
        break;
    case IDC_BUTTON_REMOVE_SELECTED_KEY:
        this->OnRemoveSelectedKey();
        break;

    case IDC_BUTTON_LOAD:
        this->OnLoad();
        break;
    case IDC_BUTTON_SAVE:
        this->OnSave();
        break;
    case IDC_BUTTON_SAVE_AS:
        this->OnSaveAs();
        break;

    case IDC_BUTTON_COPY_KEY_PATH:
        this->OnCopyKeyPath();
        break;

    case IDC_BUTTON_EXPAND:
        if (this->pTreeViewKeys)
            this->pTreeViewKeys->Expand();
        break;
    case IDC_BUTTON_COLLAPSE:
        if (this->pTreeViewKeys)
            this->pTreeViewKeys->Collapse();
        break;
          
            
        
    }
}
void CRegEditorGUI::OnNotify(WPARAM wParam,LPARAM lParam)
{
    if (this->pTreeViewKeys)
    {
        if (this->pTreeViewKeys->OnNotify(wParam,lParam))
            return;
    }
    if (this->pListviewValues )
    {
        // if spy mode
        if (this->pRegistry)
        {
            if (this->pRegistry->pOptions->GetSpyMode())
            {
                // Value contains success/failure infroamtions --> put failure access in red --> we need custom draw

                NMHDR* pnmh=((NMHDR*)lParam);

                // listview messages
                if (pnmh->hwndFrom==this->pListviewValues->GetControlHandle())
                {
                    if (pnmh->code == NM_CUSTOMDRAW) 
                    {
                        //must use SetWindowLong to return value from dialog proc
                        SetWindowLongPtr(this->GetControlHandle(), DWLP_MSGRESULT, (LONG_PTR)this->ProcessCustomDraw(lParam));
                        return;
                    }
                }
            }
        }
        // else use default notify proc
        if (this->pListviewValues->OnNotify(wParam,lParam))
            return;
    }
    if (this->pToolbar)
    {
        if (this->pToolbar->OnNotify(wParam,lParam))
            return;
    }
}

LRESULT CRegEditorGUI::ProcessCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    EmulatedRegistry::CKeyValue* pValue;

    switch(lplvcd->nmcd.dwDrawStage) 
    {
    case CDDS_PREPAINT : //Before the paint cycle begins
        //request notifications for individual listview items
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: //Before an item is drawn
        // lplvcd->nmcd.dwItemSpec // item index
        // to request notification for subitems
        // return CDRF_NOTIFYSUBITEMDRAW;

        //customize item appearance
        if (lplvcd->nmcd.dwItemSpec%2==1)
            // set a background color
            lplvcd->clrTextBk = RGB(247,247,247);

        pValue = NULL;
        this->pListviewValues->GetItemUserData((int)lplvcd->nmcd.dwItemSpec,(LPVOID*)(&pValue));
        if (pValue)
        {
            if(pValue->IsAccessFailure())
                lplvcd->clrTextBk = RGB(255,174,174);
        }

        //To set a custom font:
        //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);
        // return CDRF_NEWFONT;

        return CDRF_DODEFAULT;

        // for subitem customization
        // case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // lplvcd->nmcd.dwItemSpec // item number
        // lplvcd->iSubItem // subitem number
        // // customize subitem appearance for column 0
        // lplvcd->clrText   = RGB(255,255,255);
        // lplvcd->clrTextBk = RGB(255,0,0);
        // //To set a custom font:
        // //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);

        // return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

int WINAPI WinMain(HINSTANCE hInstance,
            HINSTANCE hPrevInstance,
            LPSTR lpCmdLine,
            int nCmdShow
            )
{
    CRegEditorGUI RegEditorGUI;
    RegEditorGUI.Show(hInstance,0,IDD_DIALOG_REGISTRY_EDITOR,IDI_ICON_APP);
 }