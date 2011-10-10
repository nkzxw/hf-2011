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


#include "RootKey.h"
#include "HostKey.h"
#include "Options.h"
#include "EmulatedRegistry.h"

namespace EmulatedRegistry
{

CRootKey::CRootKey(CEmulatedRegistry* pEmulatedRegistry)
{
    this->bSpyMode = FALSE;
    this->bRootKey = TRUE;
    this->pEmulatedRegistry = pEmulatedRegistry;
    this->bHasChangedSinceLastSave = FALSE;
}
CRootKey::~CRootKey()
{
    
}

BOOL CRootKey::IsValidKey(CKeyReplace* pKey)
{
    return this->pEmulatedRegistry->IsValidKey(pKey);
}

BOOL CRootKey::RegisterKey(CKeyReplace* pKey)
{
    return this->pEmulatedRegistry->RegisterKey(pKey);
}
BOOL CRootKey::UnregisterKey(CKeyReplace* pKey)
{
    return this->pEmulatedRegistry->UnregisterKey(pKey);
}

BOOL CRootKey::AddHost(CHostKey* pHostKey)
{
    this->SubKeys.push_back((CKeyReplace*)pHostKey);
    pHostKey->ParentKey = this;
    this->RegisterKey(pHostKey);
    this->NotifyKeyOrValueChange();
    return TRUE;
}

BOOL CRootKey::Load(TCHAR* RootContent,BOOL bUnicodeSave)
{
    BOOL bRetValue = TRUE;
    TCHAR* pszCurrentMarkup = RootContent;
    TCHAR* pszNextMarkup = NULL;
    TCHAR* Buffer;
    SIZE_T BufferSize;     

    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("HOSTS"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        TCHAR* pszHostCurrentMarkup;
        TCHAR* pszHostNextMarkup;
        TCHAR* pszHost;
        pszHostCurrentMarkup = Buffer;

        while(
            CXmlLite::ReadXMLMarkupContent(pszHostCurrentMarkup,_T("HOST"),&Buffer,&BufferSize,&pszHostNextMarkup)
            )
        {
            pszHost=new TCHAR[BufferSize+1];
            memcpy(pszHost,Buffer,BufferSize*sizeof(TCHAR));
            pszHost[BufferSize]=0;
            pszCurrentMarkup = pszHost;

            CHostKey* pHostKey = new CHostKey();
            pHostKey->ParentKey = this; // must be set before loading for correct state of emulated flag
            bRetValue = bRetValue && pHostKey->Load(pszCurrentMarkup,bUnicodeSave);
            pHostKey->Emulated = TRUE; // host are emulated

            this->AddHost(pHostKey);

            delete[] pszHost;
            pszHostCurrentMarkup=pszHostNextMarkup;      
        }
    }
    this->bHasChangedSinceLastSave=FALSE;
    return bRetValue;
}

CHostKey* CRootKey::AddHost(TCHAR* HostName)
{
    CHostKey* pHostKey = new CHostKey();
    pHostKey->ParentKey = this; // must be set before loading for correct state of emulated flag
    pHostKey->KeyName = HostName;
    pHostKey->Emulated = TRUE; // host are emulated
    this->AddHost(pHostKey);
    return pHostKey;
}

BOOL CRootKey::Save(HANDLE hFile)
{
    CTextFile::WriteText(hFile,_T("<ROOT>"));

    CTextFile::WriteText(hFile,_T("<HOSTS>"));
    // for each host
    std::vector<CKeyReplace*>::iterator iHostKey;
    for (iHostKey = this->SubKeys.begin();iHostKey!=this->SubKeys.end();iHostKey++)
    {
        CHostKey* pHostKey = (CHostKey*)*iHostKey;
        pHostKey->Save(hFile);
    }
    CTextFile::WriteText(hFile,_T("</HOSTS>"));

    CTextFile::WriteText(hFile,_T("</ROOT>"));

    this->bHasChangedSinceLastSave=FALSE;
    return TRUE;
}

void CRootKey::SetSpyMode()
{
    this->bSpyMode = TRUE;
}
BOOL CRootKey::GetSpyMode()
{
    return this->bSpyMode;
}
}