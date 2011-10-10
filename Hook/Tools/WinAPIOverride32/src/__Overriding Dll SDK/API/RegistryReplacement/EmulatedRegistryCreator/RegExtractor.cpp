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

#include "RegExtractor.h"

// RelatvePathFromBaseKey must be free with free(*pRelatvePathFromBaseKey)
BOOL CRegExtractorGUI::GetBaseKeyFromFullPath(TCHAR* const FullPath,OUT HKEY* phBaseKey,OUT TCHAR** pRelatvePathFromBaseKey)
{
    // extract base key name from FullPath
    TCHAR* pc = _tcsdup(FullPath);
    TCHAR* pSplitter = _tcschr(pc,'\\');
    if (pSplitter)
    {
        *pSplitter = NULL;
        pSplitter++;
        *pRelatvePathFromBaseKey = _tcsdup(pSplitter);
    }
    else
    {
        *pRelatvePathFromBaseKey = _tcsdup(_T(""));
    }
        
    // convert base key name to hKey
    *phBaseKey = EmulatedRegistry::StringToBaseKey(pc);
    
    // free allocated memory
    free(pc);
    
    return (*phBaseKey != NULL);
}

// warning : recursive call
BOOL CRegExtractorGUI::ExtractRealRegistryValues(BOOL bRegistryView32,EmulatedRegistry::CKeyReplace* pKey,HKEY hKeyToExtract)
{
    DWORD Index;
    BOOL bSuccess = TRUE;
    LONG Result;

    DWORD ClassSize;
    TCHAR* Class;    
    DWORD KeyNameSize;
    DWORD ValueNameSize;
    TCHAR* Name;
    DWORD BufferSize;
    BYTE* Buffer;
    DWORD NumberOfSubKeys;
    DWORD NumberOfValues;
    DWORD TmpSizeName;
    DWORD TmpSize;
    DWORD Type;
    
    ClassSize = 1024;
    Class = new TCHAR[ClassSize];
    
    // get key infos : number of value, subkeys, max subkey name saize, max value name size, max class size
    Result = ::RegQueryInfoKey(hKeyToExtract,Class,&ClassSize,0,&NumberOfSubKeys,&KeyNameSize,0,&NumberOfValues,&ValueNameSize,&BufferSize,0,&pKey->LastWriteTime);
    if (Result != ERROR_SUCCESS)
    {
        if (Result == ERROR_MORE_DATA)
        {
            delete [] Class;
            Class = new TCHAR[ClassSize];
            Result = ::RegQueryInfoKey(hKeyToExtract,Class,&ClassSize,0,&NumberOfSubKeys,&KeyNameSize,0,&NumberOfValues,&ValueNameSize,&BufferSize,0,&pKey->LastWriteTime);
            if (Result != ERROR_SUCCESS)
            {
                delete [] Class;
                return FALSE;
            }
        }
        else
        {
            delete [] Class;
            return FALSE;
        }
    }
    pKey->Class = Class;
    delete [] Class;
    
    // allocate buffers
    Name = new TCHAR[__max(KeyNameSize,ValueNameSize)+1];
    Buffer = new BYTE[BufferSize];
    
    // for each value
    for (Index = 0;Index<NumberOfValues;Index++)
    {
        // get value name type and content
        TmpSizeName= ValueNameSize+1;
        TmpSize = BufferSize;
        Result = ::RegEnumValue(hKeyToExtract,Index,Name,&TmpSizeName,0,&Type,Buffer,&TmpSize);
        if (Result != ERROR_SUCCESS)
        {
            bSuccess = FALSE;
            continue;
        }
        
        // add value infos to emulated key
        pKey->SetValue(Name,Type,Buffer,TmpSize); // Warning :TmpSize is real value and BufferSize is the max buffer size
    }
    
    EmulatedRegistry::CKeyReplace* pChildKey;
    HKEY hChildKey;
    // for each key
    for (Index = 0;Index<NumberOfSubKeys;Index++)
    {
        // get subkey
        Result = ::RegEnumKey(hKeyToExtract,Index,Name,KeyNameSize+1);
        if (Result != ERROR_SUCCESS) 
        {
            bSuccess = FALSE;
            continue;
        }
        
        // add subkey to current emulated key
        pChildKey = pKey->GetOrAddSubKey(Name);

        REGSAM RegSam = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ;
        this->GetRegistryView(bRegistryView32,&RegSam);

        // get subkey content
        if (::RegOpenKeyEx(hKeyToExtract,Name,0,RegSam,&hChildKey) == ERROR_SUCCESS)
        {
            // /!\ recursive call
            bSuccess = bSuccess && this->ExtractRealRegistryValues(bRegistryView32,pChildKey,hChildKey);
            
            // close open key
            ::RegCloseKey(hChildKey);
        }
    }

    delete[] Name;
    delete[] Buffer;
    
    return bSuccess;
}

void CRegExtractorGUI::SetGUIFromHostKeyName(EmulatedRegistry::CHostKey* pHostKey)
{
    this->SetDlgItemText(IDC_EDIT_ADD_HOST,(TCHAR*)pHostKey->KeyName.c_str());
}

void CRegExtractorGUI::AddHost(TCHAR* HostName)
{
    if (this->pRegistry->pRootKey->GetSubKey(HostName))
    {
        this->ReportError(_T("Host Already Existing"));
        return;
    }
    EmulatedRegistry::CHostKey* pHostKey;
    pHostKey = this->pRegistry->pRootKey->AddHost(HostName);
    pHostKey->KeysFilteringType = EmulatedRegistry::CHostKey::KeysFilteringType_ONLY_SPECIFIED; // currently not configurable by user interface not to be implement (risk : confuse users + extraction of whole registry)

    CHostInfos* pHostKeyInfos = new CHostInfos();
    pHostKeyInfos->pHostKey = pHostKey;
    int ListViewIndex = this->pListviewHosts->AddItem((TCHAR*)pHostKey->KeyName.c_str(),(LPVOID)pHostKeyInfos);
    this->pListviewHosts->UnselectAll();
    this->pListviewHosts->SetSelectedIndex(ListViewIndex);
}

void CRegExtractorGUI::SetHostKeyInfosFromGUI(CHostInfos* pHostKeyInfos)
{

    // set gui according to generic settings
    pHostKeyInfos->bExtractRegistry = this->IsDlgButtonChecked(IDC_CHECK_EXTRACT_VALUES);
    
    ////////////////////////////
    // fill pHostKey attributes
    ////////////////////////////
    EmulatedRegistry::CHostKey* pHostKey = pHostKeyInfos->pHostKey;
    
    pHostKey->AllowConnectionToRemoteHost = this->IsDlgButtonChecked(IDC_CHECK_ALLOW_CONNECTION_TO_REMOTE_REGISTRY_FOR_SELECTED_HOST);
    pHostKey->DisableWriteOperationsOnNotEmulatedKeys = this->IsDlgButtonChecked(IDC_CHECK_DISABLE_WRITE_ACCESS_ON_NOT_EMULATED_KEYS);

    // free previous emulated key list
    pHostKey->RemoveAllSpecifiedKeys();
    
    // get key name to emulate
    TCHAR* KeyName;
    SIZE_T KeyNameSize = this->pListviewKeys->GetItemTextLenMax(FALSE);
    if (KeyNameSize)
    {
        KeyName = new TCHAR[KeyNameSize];
        std::tstring* ptstring;
        for (SSIZE_T Cnt = 0; Cnt<this->pListviewKeys->GetItemCount();Cnt++)
        {
            // get text
            this->pListviewKeys->GetItemText(Cnt,KeyName,KeyNameSize);
            ptstring = new std::tstring(KeyName);
            pHostKey->SpecifiedKeys.push_back(ptstring);
        }
        delete[] KeyName;
    }

    ////////////////////////////
    // end of fill pHostKey attributes
    ////////////////////////////
}
void CRegExtractorGUI::SetGUIFromHostKeyInfos(CHostInfos* pHostKeyInfos)
{
    // store changes of previous key
    if (this->pCurrentlyEditedHostKey)
    {
        if (pHostKeyInfos != this->pCurrentlyEditedHostKey)
            this->SetHostKeyInfosFromGUI(this->pCurrentlyEditedHostKey);
    }
    
    // store current key as the edited one
    this->pCurrentlyEditedHostKey = pHostKeyInfos;
    
    // set gui according to generic settings
    this->SetDlgButtonCheckState(IDC_CHECK_EXTRACT_VALUES,pHostKeyInfos->bExtractRegistry);
    
    ///////////////////////////
    // set gui according to CHostKey infos
    ///////////////////////////
    EmulatedRegistry::CHostKey* pHostKey = pHostKeyInfos->pHostKey;

    this->SetDlgButtonCheckState(IDC_CHECK_ALLOW_CONNECTION_TO_REMOTE_REGISTRY_FOR_SELECTED_HOST,pHostKey->AllowConnectionToRemoteHost);
    this->SetDlgButtonCheckState(IDC_CHECK_DISABLE_WRITE_ACCESS_ON_NOT_EMULATED_KEYS,pHostKey->DisableWriteOperationsOnNotEmulatedKeys);

    // clear list view
    this->pListviewKeys->Clear();

    // for each specifiedkey,    
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = pHostKey->SpecifiedKeys.begin();iSpecifiedKeys!=pHostKey->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;
        this->pListviewKeys->AddItem((TCHAR*)pString->c_str());
    }
    
    ///////////////////////////
    // end set gui according to CHostKey infos
    ///////////////////////////
}
BOOL CRegExtractorGUI::GetRegistryView(BOOL bRegistryView32,IN OUT REGSAM* pSamDesired)
{
    if (!this->b64BitOs)
        return TRUE;

#ifndef KEY_WOW64_32KEY
    #define KEY_WOW64_32KEY    0x0200
#endif
#ifndef KEY_WOW64_64KEY
    #define KEY_WOW64_64KEY    0x0100
#endif

    if (bRegistryView32)
        *pSamDesired|=KEY_WOW64_32KEY;
    else
        *pSamDesired|=KEY_WOW64_64KEY;

    return TRUE;
}
BOOL CRegExtractorGUI::Is64BitOs()
{
#ifndef PROCESSOR_ARCHITECTURE_AMD64
    #define PROCESSOR_ARCHITECTURE_AMD64    9 // x64 (AMD or Intel)
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
    #define PROCESSOR_ARCHITECTURE_IA64     6 // Intel Itanium Processor Family (IPF)
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
    #define PROCESSOR_ARCHITECTURE_INTEL    0 // x86
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
    #define PROCESSOR_ARCHITECTURE_UNKNOWN  0xffff 
#endif

    SYSTEM_INFO SystemInfo={0};
    ::GetNativeSystemInfo(&SystemInfo);
    return ( (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            || (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
            );
}

BOOL CRegExtractorGUI::ExtractRealRegistryValues(BOOL bRegistryView32,EmulatedRegistry::CHostKey* pHostKey,OUT BOOL* pErrorDetectedDuringExtraction)
{
    BOOL bSuccess = TRUE;
    HKEY hBaseKey = NULL;
    HKEY hRemoteBaseKey = NULL;
    HKEY hKey = NULL;
    TCHAR* RelatvePathFromBaseKey = NULL;
    *pErrorDetectedDuringExtraction = FALSE;
    
    ////////////////////////////////////////////////////
    // for pHostKey->KeysFilteringType == KeysFilteringType_ONLY_SPECIFIED; only
    ////////////////////////////////////////////////////
#ifdef _DEBUG
    if (pHostKey->KeysFilteringType != EmulatedRegistry::CHostKey::KeysFilteringType_ONLY_SPECIFIED)
        DebugBreak();
#endif    
    
    REGSAM RegSam = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ;
    this->GetRegistryView(bRegistryView32,&RegSam);

    // for each specified key   
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = pHostKey->SpecifiedKeys.begin();iSpecifiedKeys!=pHostKey->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;

        // get base key name and relative key from base key
        if (!this->GetBaseKeyFromFullPath((TCHAR*)pString->c_str(),&hBaseKey,&RelatvePathFromBaseKey))
        {
            free(RelatvePathFromBaseKey);
            return FALSE;
        }    
        
        // get base key
        if (pHostKey->IsLocalHost())
        {
            bSuccess = (::RegOpenKeyEx(hBaseKey,RelatvePathFromBaseKey,0,RegSam,&hKey) == ERROR_SUCCESS);
        }
        else // remote host
        {
            // remote registry must be enabled : see MSDN RegConnectRegistry for informations
            bSuccess = (::RegConnectRegistry((TCHAR*)pHostKey->KeyName.c_str(),hBaseKey,&hRemoteBaseKey) == ERROR_SUCCESS);
            if (bSuccess)
                bSuccess = (::RegOpenKeyEx(hRemoteBaseKey,RelatvePathFromBaseKey,0,RegSam,&hKey) == ERROR_SUCCESS);
        }
        
        // in case of success 
        if (bSuccess)
        {
            // create tree to subkey inside emulated registry
            TCHAR* pSplitter;
            TCHAR* LocalBuffer;
            TCHAR* KeyName;
            LocalBuffer = _tcsdup((TCHAR*)pString->c_str());
            KeyName = LocalBuffer;
            
            EmulatedRegistry::CKeyReplace* pUpperKey = (EmulatedRegistry::CKeyReplace*)pHostKey;
            EmulatedRegistry::CKeyReplace* pKey;

            pSplitter = KeyName; // provide a not empty value for splitter
            while(pSplitter)
            {
                pSplitter = _tcschr(KeyName,'\\');
                if (pSplitter)
                {
                    *pSplitter = 0;
                }
                if (!*KeyName)
                    break;
                pKey = pUpperKey->GetOrAddSubKey(KeyName);
                pUpperKey = pKey;
                KeyName = pSplitter+1;
            }
            free(LocalBuffer);
            
            // extract key values
            *pErrorDetectedDuringExtraction = *pErrorDetectedDuringExtraction || (!this->ExtractRealRegistryValues(bRegistryView32,pKey,hKey));
        }
        else
        {
            TCHAR Msg[2048];
            _sntprintf(Msg,2048, _T("Error opening key %s"),pString->c_str());
            this->ReportError(Msg);
            *pErrorDetectedDuringExtraction = TRUE;
        }
        
        // free memory
        free(RelatvePathFromBaseKey);
        if (hKey)
            ::RegCloseKey(hKey);
        if (hRemoteBaseKey)
            ::RegCloseKey(hRemoteBaseKey);
    
    }
    
    // return success status
    return bSuccess;
}

// FileName size must be at least MAX_PATH in TCHAR count
BOOL CRegExtractorGUI::QuerySavingFileName(TCHAR* FileName)
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

BOOL CRegExtractorGUI::SaveRegistry(EmulatedRegistry::CEmulatedRegistry* pEmulatedRegistry,TCHAR* const FileName)
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

    pEmulatedRegistry->pOptions->SetAllowConnectionToUnspecifiedRemoteHosts(this->IsDlgButtonChecked(IDC_CHECK_ALLOW_CONNECTION_TO_UNKNOWN_HOSTS));
    pEmulatedRegistry->pOptions->SetEmulateSubProcesses(this->IsDlgButtonChecked(IDC_CHECK_EMULATE_CHILD_PROCESSES));

    // query new file while saving failure occurs
    // until saving success
    while ( 
            ( !pEmulatedRegistry->Save(UsedFileName) )
          )
    {
        if (!this->QuerySavingFileName(UsedFileName))
            return FALSE;
    }

    return TRUE;
}

BOOL CRegExtractorGUI::ExtractAndSaveRegistry(BOOL bRegistryView32)
{
    
    BOOL bErrorDetectedDuringExtraction = FALSE;
    BOOL bErrorDetectedDuringExtractionOfHost = FALSE;
    BOOL bSaveRegistry = TRUE;
    BOOL bSaveCanceledOrError = FALSE;
 
// msg extracting data, please wait
 
    // for each host
    CHostInfos* pHostKeyInfos;
    for (SSIZE_T Cnt = 0; Cnt<this->pListviewHosts->GetItemCount();Cnt++)
    {
        this->pListviewHosts->GetItemUserData(Cnt,(LPVOID*)&pHostKeyInfos);
        if (!pHostKeyInfos)
            continue;

        if (pHostKeyInfos->bExtractRegistry)
            this->ExtractRealRegistryValues(bRegistryView32,pHostKeyInfos->pHostKey,&bErrorDetectedDuringExtractionOfHost);
        bErrorDetectedDuringExtraction = bErrorDetectedDuringExtraction || bErrorDetectedDuringExtractionOfHost;
    }
 
    if (bErrorDetectedDuringExtraction)
    {
        bSaveRegistry = (
                         ::MessageBox(this->GetControlHandle(),
                                     _T("Some errors occurs during windows registry data retrieval\r\n")
                                     _T("Do you want to save registry anyway ?"),
                                     _T("Warning"),
                                     MB_YESNO|MB_ICONQUESTION
                                     )
                         == IDYES 
                         );
    }

    if (bSaveRegistry)
        bSaveCanceledOrError = !this->SaveRegistry(this->pRegistry,NULL);


    // for each host, free all extracted keys
    std::vector<EmulatedRegistry::CKeyReplace*>::iterator iHostKey;
    std::vector<EmulatedRegistry::CKeyReplace*>::iterator iSubKeys;
    std::vector<EmulatedRegistry::CKeyValue*>::iterator iValue;
    for (iHostKey = this->pRegistry->pRootKey->SubKeys.begin();iHostKey!=this->pRegistry->pRootKey->SubKeys.end();iHostKey++)
    {
        EmulatedRegistry::CHostKey* pHostKey = (EmulatedRegistry::CHostKey*)*iHostKey;
        
        for (iSubKeys = pHostKey->SubKeys.begin();iSubKeys!=pHostKey->SubKeys.end();iSubKeys++)
        {
            EmulatedRegistry::CKeyReplace* pSubKey = *iSubKeys;
            delete pSubKey;
        }
        pHostKey->SubKeys.clear();

        // for each value, delete value
        
        for (iValue = pHostKey->Values.begin();iValue!=pHostKey->Values.end();iValue++)
        {
            EmulatedRegistry::CKeyValue* pValue = *iValue;
            delete pValue;
        }
        pHostKey->Values.clear();
    }

    if (bSaveRegistry)
    {
        if (bSaveCanceledOrError)
            return FALSE;
        this->ReportMessage(_T("Emulated registry creation successfully completed"),ReportMessageType_INFO);
    }
    return TRUE;
}

void CRegExtractorGUI::OnCreateConfigurationFile()
{
    // save current host infos
    // store changes of previous key
    if (this->pCurrentlyEditedHostKey)
        this->SetHostKeyInfosFromGUI(this->pCurrentlyEditedHostKey);

#ifdef _WIN64
    to implement (no difference between 32 and 64 bit code, only to get an error when porting to 64)
#endif
    // todo : to implement when winapioverride will be ported to 64bit
    BOOL bRegistryView32 = TRUE;

    // save registry
    this->ExtractAndSaveRegistry(bRegistryView32);
}

CRegExtractorGUI::CRegExtractorGUI()
{
    this->pListviewHosts = NULL;
    this->pListviewKeys = NULL;
    this->pCurrentlyEditedHostKey = NULL;
    this->pRegistry = new EmulatedRegistry::CEmulatedRegistry();
    this->b64BitOs = this ->Is64BitOs();
}

CRegExtractorGUI::~CRegExtractorGUI()
{
    if (pRegistry)
        delete pRegistry;
}

void CRegExtractorGUI::OnInit()
{
    this->pListviewHosts = new CListview(this->GetDlgItem(IDC_LIST_HOSTS));
    this->pListviewHosts->AddColumn(_T("Host Name"),200,LVCFMT_LEFT);
    this->pListviewHosts->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListviewHosts->SetSelectItemCallback(CRegExtractorGUI::OnSelectHostChange,this);

    this->pListviewKeys = new CListview(this->GetDlgItem(IDC_LIST_HOST_KEY));
    this->pListviewKeys->AddColumn(_T("Key Name"),600,LVCFMT_LEFT);
    this->pListviewKeys->SetStyle(TRUE,FALSE,FALSE,FALSE);

    this->AddHost(RegistryReplacement_LOCAL_MACHINE_REGISTRY);

}
void CRegExtractorGUI::OnClose()
{
    CHostInfos* pHostKeyInfos;
    for (SSIZE_T Cnt = 0; Cnt<this->pListviewHosts->GetItemCount();Cnt++)
    {
        this->pListviewHosts->GetItemUserData(Cnt,(LPVOID*)&pHostKeyInfos);
        if (pHostKeyInfos)
            delete pHostKeyInfos;
    }
    delete this->pListviewHosts;
    delete this->pListviewKeys;
}

void CRegExtractorGUI::OnAddHost()
{
    SIZE_T MachineNameLen = this->GetDlgItemTextLength(IDC_EDIT_ADD_HOST);
    if (MachineNameLen==0)
    {
        this->ReportError(_T("Empty Machine Name"));
        return;
    }
    MachineNameLen++;//for \0
    TCHAR* MachineName=new TCHAR[MachineNameLen];
    this->GetDlgItemText(IDC_EDIT_ADD_HOST,MachineName,MachineNameLen);
    this->AddHost(MachineName);
    delete[] MachineName;
}
void CRegExtractorGUI::OnRenameHost()
{
    CHostInfos* pHostKeyInfos = this->GetSelectedHost();
    if (!pHostKeyInfos)
        return;

    EmulatedRegistry::CHostKey* pHostKey = pHostKeyInfos->pHostKey;
    if (!pHostKey)
        return;

    if (_tcsicmp(pHostKey->KeyName.c_str(),RegistryReplacement_LOCAL_MACHINE_REGISTRY)==0)
    {
        this->ReportError(_T("Can't Rename Local Host"));
        return;
    }

    SIZE_T MachineNameLen = this->GetDlgItemTextLength(IDC_EDIT_ADD_HOST);
    if (MachineNameLen==0)
    {
        this->ReportError(_T("Empty Machine Name"));
        return;
    }
    MachineNameLen++;//for \0
    TCHAR* MachineName=new TCHAR[MachineNameLen];
    this->GetDlgItemText(IDC_EDIT_ADD_HOST,MachineName,MachineNameLen);
    
    // if name already used
    if (this->pRegistry->pRootKey->GetSubKey(MachineName))
    {
        this->ReportError(_T("Host Name already used"));
        delete[] MachineName;
        return;
    }

    pHostKey->KeyName = MachineName;

    int ItemIndex = this->pListviewHosts->GetSelectedIndex();
    this->pListviewHosts->SetItemText(ItemIndex,MachineName);
    delete[] MachineName;
}

CHostInfos* CRegExtractorGUI::GetSelectedHost()
{
    CHostInfos* pHostKeyInfos;
    int ItemIndex=this->pListviewHosts->GetSelectedIndex();
    if (ItemIndex<0)
        return NULL;
    this->pListviewHosts->GetItemUserData(ItemIndex,(LPVOID*)&pHostKeyInfos);
    return pHostKeyInfos;
}

void CRegExtractorGUI::OnRemoveSelectedHosts()
{
    if (!this->pListviewHosts->GetSelectedCount())
    {
        this->ReportError(_T("Select a host first"));
        return;
    }

    CHostInfos* pHostKeyInfos;
    EmulatedRegistry::CHostKey* pHostKey;
    for(SSIZE_T Cnt = this->pListviewHosts->GetItemCount(); Cnt>=0; Cnt--)
    {
        if(!this->pListviewHosts->IsItemSelected(Cnt))
            continue;
        this->pListviewHosts->GetItemUserData(Cnt,(LPVOID*)&pHostKeyInfos);
        if (!pHostKeyInfos)
            continue;

        pHostKey = pHostKeyInfos->pHostKey;
        if (pHostKey)
        {
            if (_tcsicmp(pHostKey->KeyName.c_str(), RegistryReplacement_LOCAL_MACHINE_REGISTRY) == 0)
            {
                this->ReportError(_T("Can't remove local registry"));
                return;
            }
        }
        // remove from listview first to avoid memory error accessing item object
        this->pListviewHosts->RemoveItem(Cnt);

        if (this->pCurrentlyEditedHostKey == pHostKeyInfos)
            this->pCurrentlyEditedHostKey = NULL;

        // remove host
        pHostKey->RemoveCurrentKey();
        
        delete pHostKeyInfos;
    }
    
}

void CRegExtractorGUI::OnSelectHostChange(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    ((CRegExtractorGUI*)UserParam)->OnSelectHostChange(ItemIndex,SubItemIndex);
}
void CRegExtractorGUI::OnSelectHostChange(int ItemIndex,int SubItemIndex)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    // load new host infos
    EmulatedRegistry::CHostKey* pHostKey;
    CHostInfos* pHostKeyInfos;
    this->pListviewHosts->GetItemUserData(ItemIndex,(LPVOID*)&pHostKeyInfos);
    if(!pHostKeyInfos)
        return;
    pHostKey = pHostKeyInfos->pHostKey;
    if (!pHostKey)
        return;
    this->SetGUIFromHostKeyInfos(pHostKeyInfos);
    this->SetGUIFromHostKeyName(pHostKey);
}

void CRegExtractorGUI::OnAddKey()
{
    SIZE_T KeyNameLen = this->GetDlgItemTextLength(IDC_EDIT_FULL_KEY_NAME_TO_ADD);
    if (KeyNameLen==0)
    {
        this->ReportError(_T("Empty Key Name"));
        return;
    }
    KeyNameLen++;//for \0
    TCHAR* KeyName=new TCHAR[KeyNameLen];
    this->GetDlgItemText(IDC_EDIT_FULL_KEY_NAME_TO_ADD,KeyName,KeyNameLen);

    // check if key is alreay existing
    BOOL bAlreadyExiting = FALSE;
    TCHAR* ExistingKeyName;
    SIZE_T ExistingKeyNameSize = this->pListviewKeys->GetItemTextLenMax(FALSE);
    if (ExistingKeyNameSize)
    {
        ExistingKeyName = new TCHAR[ExistingKeyNameSize];
        for (SSIZE_T Cnt = 0; Cnt<this->pListviewKeys->GetItemCount();Cnt++)
        {
            // get text
            this->pListviewKeys->GetItemText(Cnt,ExistingKeyName,ExistingKeyNameSize);
            if (_tcsicmp(ExistingKeyName,KeyName) == 0)
            {
                bAlreadyExiting = TRUE;
                break;
            }
        }
        delete[] ExistingKeyName;
    }

    if (bAlreadyExiting)
    {
        this->ReportError(_T("Key Already Existing"));
    }
    else
    {
        this->pListviewKeys->AddItem(KeyName);
    }
    delete[] KeyName;
}
void CRegExtractorGUI::OnRemoveSelectedKeys()
{
    CHostInfos* pHostKeyInfos = this->GetSelectedHost();
    if (!pHostKeyInfos)
        return;
    
    EmulatedRegistry::CHostKey* pHostKey = pHostKeyInfos->pHostKey;
    if (!pHostKey)
        return;

    DWORD MaxSelectedKeyNameSize = this->pListviewKeys->GetItemTextLenMax(TRUE);
    if (MaxSelectedKeyNameSize)
    {
        TCHAR* KeyName = new TCHAR[MaxSelectedKeyNameSize];

        // warning do it in reverse order to avoid indexes changes
        for(SSIZE_T Cnt = this->pListviewKeys->GetItemCount(); Cnt>=0; Cnt--)
        {
            // if item is selected
            if (this->pListviewKeys->IsItemSelected(Cnt))
            {
                // get name
                this->pListviewKeys->GetItemText(Cnt,KeyName,MaxSelectedKeyNameSize);

                // remove from listview first
                this->pListviewKeys->RemoveItem(Cnt);

                // remove from list
                pHostKey->RemoveSpecifiedKey(KeyName);
            }
        }

        delete[] KeyName;
    }
}

void CRegExtractorGUI::OnCommand(WPARAM wParam,LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_BUTTON_ADD_HOST:
        this->OnAddHost();
        break;
    case IDC_BUTTON_REMOVE_SELECTED_HOSTS:
        this->OnRemoveSelectedHosts();
        break;
    case IDC_BUTTON_ADD_KEY:
        this->OnAddKey();
        break;
    case IDC_BUTTON_RENAME_HOST:
        this->OnRenameHost();
        break;
    case IDC_BUTTON_REMOVE_SELECTED_KEYS:
        this->OnRemoveSelectedKeys();
        break;
    case IDOK:
        this->OnCreateConfigurationFile();
        break;
    case IDCANCEL:
        this->Close();
        break;
    }
}
void CRegExtractorGUI::OnNotify(WPARAM wParam,LPARAM lParam)
{
    if (this->pListviewHosts)
    {
        if (this->pListviewHosts->OnNotify(wParam,lParam))
            return;
    }
    if (this->pListviewKeys )
    {
        if (this->pListviewKeys ->OnNotify(wParam,lParam))
            return;
    }
}

int WINAPI WinMain(HINSTANCE hInstance,
            HINSTANCE hPrevInstance,
            LPSTR lpCmdLine,
            int nCmdShow
            )
{
    CRegExtractorGUI RegExtractorGUI;
    RegExtractorGUI.Show(hInstance,0,IDD_DIALOG_REGISTRY_CREATOR,IDI_ICON_APP);
 }