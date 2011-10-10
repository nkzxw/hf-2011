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

#include "EmulatedRegistry.h"

namespace EmulatedRegistry
{

// dynamic loading of user32 for error message (loaded only on error)
HMODULE CEmulatedRegistry::User32Module=0;
CEmulatedRegistry::pMessageBox CEmulatedRegistry::pfpMessageBox=0;
CEmulatedRegistry::pMessageBox CEmulatedRegistry::GetMessageBox()
{
    if (CEmulatedRegistry::pfpMessageBox)
        return CEmulatedRegistry::pfpMessageBox;

    CEmulatedRegistry::User32Module = ::GetModuleHandle(_T("user32.dll"));
    if (!CEmulatedRegistry::User32Module)
    {
        CEmulatedRegistry::User32Module = ::LoadLibrary(_T("user32.dll"));
        if (!CEmulatedRegistry::User32Module)
            return NULL;
    }
#if (defined(UNICODE)||defined(_UNICODE))
    CEmulatedRegistry::pfpMessageBox = (pMessageBox)::GetProcAddress(CEmulatedRegistry::User32Module,"MessageBoxW");
#else
    CEmulatedRegistry::pfpMessageBox = (pMessageBox)::GetProcAddress(CEmulatedRegistry::User32Module,"MessageBoxA");
#endif
    return CEmulatedRegistry::pfpMessageBox;
}

CEmulatedRegistry::CEmulatedRegistry()
{
    // create options object
    this->pOptions = new COptions();

    // create list containing all keys to know there validity
    this->pLinkListKeys = new CLinkListSimple();
    this->hHeap = ::HeapCreate(0,0x100000,0);
    this->pLinkListKeys->SetHeap(this->hHeap);

    // create rootkey
    this->pRootKey = new CRootKey(this);
    this->pRootKey->KeyName = _T("Root");

    this->pKeyLocalHost = NULL;
}
CEmulatedRegistry::~CEmulatedRegistry()
{
    
    delete this->pRootKey;
    delete this->pOptions;

#if _DEBUG
    if (this->pLinkListKeys->GetItemsCount())
    {
        ::DebugBreak();
    }
#endif
    ::HeapDestroy(this->hHeap);
    this->pLinkListKeys->ReportHeapDestruction();
    delete this->pLinkListKeys;
}

BOOL CEmulatedRegistry::RegisterKey(CKeyReplace* pKey)
{
    return (this->pLinkListKeys->AddItem(pKey)!=NULL);
}
BOOL CEmulatedRegistry::UnregisterKey(CKeyReplace* pKey)
{
    return (this->pLinkListKeys->RemoveItemFromItemData(pKey)!=NULL);
}

BOOL CEmulatedRegistry::IsValidKey(CKeyReplace* pKey)
{
    BOOL bRet = FALSE;

    this->pLinkListKeys->Lock();
    CLinkListItem* pListItem;
    // parse list
    for (pListItem=this->pLinkListKeys->Head;pListItem;pListItem=pListItem->NextItem)
    {
        // if Cnt match Index
        if (pListItem->ItemData==pKey)
        {
            bRet=TRUE;
            break;
        }
    }

    this->pLinkListKeys->Unlock();
    return bRet;
}

BOOL CEmulatedRegistry::Load(TCHAR* FileName)
{
    TCHAR* pszContent;
    TCHAR* pszCurrentMarkup;
    TCHAR* pszNextMarkup;
    TCHAR* Buffer;
    DWORD BufferSize;
    BOOL bUnicodeSave=TRUE;
    BOOL bRetValue = TRUE;

    if (!DoesFileExists(FileName)) // avoid CTextFile error report if no file exists
        goto Error;

    if(!CTextFile::Read(FileName,&pszContent,&bUnicodeSave))
        goto Error;

    pszCurrentMarkup=pszContent;

    if (!CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("REGISTRY_REPLACEMENT"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        delete[] pszContent;

        pMessageBox pfMessageBox = this->GetMessageBox();
        if (pfMessageBox)
            pfMessageBox(NULL,_T("Invalid File"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

        goto Error;
    }

    pszCurrentMarkup=Buffer;

    // read options
    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("OPTIONS"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        bRetValue =  bRetValue && this->pOptions->Load(Buffer);
        // if spy mode, put rootkey in spy mode
        if (this->pOptions->GetSpyMode())
            this->pRootKey->SetSpyMode();
    }
    else
    {
        bRetValue = FALSE;
    }

    // read key content
    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("ROOT"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        bRetValue =  bRetValue && this->pRootKey->Load(Buffer,pOptions->GetUnicodeSave());
    }
    else
    {
        bRetValue = FALSE;
    }    

    // assume local host exists
    // get (or add but should not be the case) local host to root key
    this->pKeyLocalHost = (CHostKey*)pRootKey->GetSubKey(RegistryReplacement_LOCAL_MACHINE_REGISTRY);
    if (!this->pKeyLocalHost)
    {
        this->pKeyLocalHost = new CHostKey();
        this->pKeyLocalHost->KeyName=RegistryReplacement_LOCAL_MACHINE_REGISTRY;
        this->pKeyLocalHost->KeysFilteringType=CHostKey::KeysFilteringType_ALL_BUT_SPECIFIED;
        this->pRootKey->AddHost(this->pKeyLocalHost);
    }
    // local host is considered as connected
    this->pKeyLocalHost->IsHostConnected = TRUE;

    delete[] pszContent;
    return bRetValue;

Error:
    // assume pKeyLocalHost exists to avoid crash by app using this class without checking result of Load operation
    if (!this->pKeyLocalHost)
    {
        this->pKeyLocalHost = new CHostKey();
        this->pKeyLocalHost->KeyName=RegistryReplacement_LOCAL_MACHINE_REGISTRY;
        this->pKeyLocalHost->KeysFilteringType=CHostKey::KeysFilteringType_ALL_BUT_SPECIFIED;
        this->pRootKey->AddHost(this->pKeyLocalHost);
    }
    // local host is considered as connected
    this->pKeyLocalHost->IsHostConnected = TRUE;
    return FALSE;
}
BOOL CEmulatedRegistry::Save(TCHAR* FileName)
{
    return this->Save(FileName,FALSE);
}

BOOL CEmulatedRegistry::Save(TCHAR* FileName,BOOL bOnlyIfContentChanges)
{
    if (bOnlyIfContentChanges)
    {
        // if no change occurs
        if ( (!this->pOptions->HasChangedSinceLastSave()) && (!this->pRootKey->HasChangedSinceLastSave()) )
        {
            return TRUE;
        }
    }

    // save all value of pseudo registry
    ::DeleteFile(FileName); // file MUST BE deleted first to reflect deleted keys

    BOOL bRetValue = TRUE;
    HANDLE hFile;
    if (!CTextFile::CreateTextFile(FileName,&hFile))
        return FALSE;

    CTextFile::WriteText(hFile,_T("<REGISTRY_REPLACEMENT>"));

    CTextFile::WriteText(hFile,_T("<OPTIONS>"));
    bRetValue = bRetValue && this->pOptions->Save(hFile);
    CTextFile::WriteText(hFile,_T("</OPTIONS>"));

    bRetValue = bRetValue && this->pRootKey->Save(hFile);

    CTextFile::WriteText(hFile,_T("</REGISTRY_REPLACEMENT>"));
    ::CloseHandle(hFile);

    return bRetValue;
}

void CEmulatedRegistry::SetSpyMode()
{
    this->pOptions->SetSpyMode(TRUE);
    this->pRootKey->SetSpyMode();
}

}