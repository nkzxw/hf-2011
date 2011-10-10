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

#include "HostKey.h"

namespace EmulatedRegistry
{

CHostKey::CHostKey()
{
    this->IsHostConnected = FALSE;
    this->AllowConnectionToRemoteHost = FALSE;    
    this->KeysFilteringType = KeysFilteringType_ALL_BUT_SPECIFIED;
    this->DisableWriteOperationsOnNotEmulatedKeys = FALSE;
}

void CHostKey::RemoveAllSpecifiedKeys()
{
    // for each specifiedkey,    
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = this->SpecifiedKeys.begin();iSpecifiedKeys!=this->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;
        delete pString;
    }
    this->SpecifiedKeys.clear();
}

BOOL CHostKey::RemoveSpecifiedKey(TCHAR* KeyName)
{
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = this->SpecifiedKeys.begin();iSpecifiedKeys!=this->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;

        if (_tcsicmp(KeyName,pString->c_str())==0)
        {
            delete pString;
            this->SpecifiedKeys.erase(iSpecifiedKeys);
            return TRUE;
        }
    }
    return FALSE;
}

CHostKey::~CHostKey()
{
    this->RemoveAllSpecifiedKeys();
}    

BOOL CHostKey::IsLocalHost()
{
    return (_tcsicmp(this->KeyName.c_str(),RegistryReplacement_LOCAL_MACHINE_REGISTRY) == 0);
}    

BOOL CHostKey::IsEmulatedKey(TCHAR* FullKeyName)
{
    BOOL bFoundInList = FALSE;

    // for each specifiedkey
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = this->SpecifiedKeys.begin();iSpecifiedKeys!=this->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;
        if (_tcsnicmp(FullKeyName,pString->c_str(),pString->length()) == 0)
        {
            bFoundInList = TRUE;
            break;
        }
    }

    switch(this->KeysFilteringType)
    {
    default:
    case CHostKey::KeysFilteringType_ONLY_SPECIFIED:
        return bFoundInList;
    case CHostKey::KeysFilteringType_ALL_BUT_SPECIFIED:
        return !bFoundInList;
    }        
} 

BOOL CHostKey::Load(TCHAR* KeyContent,BOOL bUnicodeSave) 
{
    BOOL bRetValue = TRUE;
    TCHAR* pszCurrentMarkup;
    TCHAR* pszNextMarkup;
    TCHAR* Buffer;
    SIZE_T BufferSize;

    pszCurrentMarkup = KeyContent;

    SIZE_T Tmp;
    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("KEYS_FILTERING_TYPE"),&Tmp,&pszNextMarkup);
    this->KeysFilteringType = (CHostKey::KEYS_FILTERING_TYPE)Tmp;

    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("ALLOW_CONNECTION"),&Tmp,&pszNextMarkup);
    this->AllowConnectionToRemoteHost = (BOOL)Tmp;      

    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("DISABLE_WRITE_OPERATIONS_ON_NOT_EMULATED_KEYS"),&Tmp,&pszNextMarkup);
    this->DisableWriteOperationsOnNotEmulatedKeys = (BOOL)Tmp;        

    // load key filtering options
    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("SPECIFIED_KEYS"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        TCHAR* pszSpecifiedKeyCurrentMarkup = Buffer;
        TCHAR* pszSpecifiedKeyNextMarkup;  
        TCHAR* pszSpecifiedKey;
        std::tstring* ptstring;
        while (CXmlLite::ReadXMLValue(pszSpecifiedKeyCurrentMarkup,_T("KEY_NAME"),&pszSpecifiedKey,bUnicodeSave,&pszSpecifiedKeyNextMarkup))
        {
            ptstring = new std::tstring(pszSpecifiedKey);
            this->SpecifiedKeys.push_back(ptstring);
            delete[] pszSpecifiedKey;
            pszSpecifiedKeyCurrentMarkup=pszSpecifiedKeyNextMarkup;        
        }
    }

    // load host values
    bRetValue = bRetValue && this->CKeyReplace::Load(pszNextMarkup,bUnicodeSave);

    return bRetValue;
}    

BOOL CHostKey::Save(HANDLE hFile)
{
    CTextFile::WriteText(hFile,_T("<HOST>"));

    CXmlLite::WriteXMLValue(hFile,_T("KEYS_FILTERING_TYPE"),this->KeysFilteringType);
    CXmlLite::WriteXMLValue(hFile,_T("ALLOW_CONNECTION"),this->AllowConnectionToRemoteHost);
    CXmlLite::WriteXMLValue(hFile,_T("DISABLE_WRITE_OPERATIONS_ON_NOT_EMULATED_KEYS"),this->DisableWriteOperationsOnNotEmulatedKeys);

    CTextFile::WriteText(hFile,_T("<SPECIFIED_KEYS>"));
    // for each specifiedkey,    
    std::vector<std::tstring*>::iterator iSpecifiedKeys;
    for (iSpecifiedKeys = this->SpecifiedKeys.begin();iSpecifiedKeys!=this->SpecifiedKeys.end();iSpecifiedKeys++)
    {
        std::tstring* pString = *iSpecifiedKeys;
        CXmlLite::WriteXMLValue(hFile,_T("KEY_NAME"),(TCHAR*)pString->c_str());
    }
    CTextFile::WriteText(hFile,_T("</SPECIFIED_KEYS>"));

    this->CKeyReplace::Save(hFile);

    CTextFile::WriteText(hFile,_T("</HOST>"));

    return TRUE;
}
}