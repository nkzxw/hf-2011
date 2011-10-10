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

#include "Key.h"
#include "RootKey.h"
#include "HostKey.h"

namespace EmulatedRegistry
{

CKeyReplace::CKeyReplace()
{
    this->KeyName=_T("");
    this->Class = _T("");
    this->ParentKey = NULL;
    this->Emulated = TRUE;
    this->bRootKey = FALSE;
    this->hRealRegistryKeyHandleIfNotEmulated=NULL;
    this->OpenedHandleCount=0;
    this->AskedForDeletion=FALSE;
    memset(&this->LastWriteTime, 0, sizeof(FILETIME));
    ::InitializeCriticalSection(&this->CriticalSection);
};
CKeyReplace::~CKeyReplace()
{
    this->Lock();

    CRootKey* pRootKey = this->GetRootKey();
    
    // for each subkey, delete subkey   
    std::vector<CKeyReplace*>::iterator iSubKeys;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pSubKey = *iSubKeys;
        if (pRootKey)
            delete pSubKey;
    }
    this->SubKeys.clear();

    // for each value, delete value
    std::vector<CKeyValue*>::iterator iValue;
    for (iValue = this->Values.begin();iValue!=this->Values.end();iValue++)
    {
        CKeyValue* pValue = *iValue;
        delete pValue;
    }
    this->Values.clear();
    if (pRootKey)
        pRootKey->UnregisterKey(this);

    this->Unlock();

    ::DeleteCriticalSection(&this->CriticalSection);
}

void CKeyReplace::MarkKeyForDeletion(BOOL bMarkSubKeysForDeletion)
{
    this->AskedForDeletion=TRUE;
    if (bMarkSubKeysForDeletion)
    {
        this->Lock();
        // for each sub key
        std::vector<CKeyReplace*>::iterator iSubKeys;
        for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
        {
            CKeyReplace* pSubKey = *iSubKeys;
            // mark sub keys for deletion
            pSubKey->MarkKeyForDeletion(TRUE);
        }
        this->Unlock();    
    }
}
BOOL CKeyReplace::IsKeyMarkedForDeletion()
{
    return this->AskedForDeletion;
}
SIZE_T CKeyReplace::GetOpenedHandleCount()
{
    return this->OpenedHandleCount;
}
SIZE_T CKeyReplace::IncreaseOpenedHandleCount()
{
    this->OpenedHandleCount++;
    return this->OpenedHandleCount;
}
SIZE_T CKeyReplace::DecreaseOpenedHandleCount()
{
    if (this->OpenedHandleCount>0)
        this->OpenedHandleCount--;
    return this->OpenedHandleCount;
}

BOOL CKeyReplace::IsHostKey()
{
    if (!this->ParentKey)
        return FALSE;
    return (this->ParentKey->IsRootKey());
}

CHostKey* CKeyReplace::GetHostKey()
{
    CKeyReplace* pKeyReplace;
    pKeyReplace = this;
    if (!pKeyReplace->ParentKey)
        return NULL;
    while(!pKeyReplace->ParentKey->IsRootKey())
    {
        pKeyReplace = pKeyReplace->ParentKey;
        if (!pKeyReplace->ParentKey)
            return NULL;
    }
    return (CHostKey* )pKeyReplace;
}
CRootKey* CKeyReplace::GetRootKey()
{
    CKeyReplace* pKeyReplace;
    pKeyReplace = this;
    while(!pKeyReplace->IsRootKey())
    {
        pKeyReplace = pKeyReplace->ParentKey;
        if (!pKeyReplace)
            return NULL;
    }
    return (CRootKey* )pKeyReplace;
}
BOOL CKeyReplace::IsRootKey()
{
    return (this->bRootKey);
}
BOOL CKeyReplace::IsBaseKey()
{
    if (!this->ParentKey)
        return FALSE;
    return (this->ParentKey->IsHostKey());
}

CKeyReplace* CKeyReplace::GetBaseKey()
{
    if (this->IsRootKey())
        return NULL;
    if (this->IsHostKey())
        return NULL;
    CKeyReplace* pKey = this;
    while (!pKey->IsBaseKey())
    {
        pKey = pKey->ParentKey;
    }
    return pKey;
}

BOOL CKeyReplace::IsWriteAccessAllowed()
{
    // allow write access on emulated key
    if (this->Emulated)
        return TRUE;
    CHostKey* pHostKey=this->GetHostKey();
    if (!pHostKey)
        return FALSE;
    return !pHostKey->DisableWriteOperationsOnNotEmulatedKeys;
}
BOOL CKeyReplace::IsEmulatedKey() 
{
    // if root key
    if(!this->ParentKey)
        return TRUE; // root key is emulated (abstract key only for emulation)

    // if host key
    if(this->ParentKey->IsRootKey()) // host key (abstract key only for emulation)
        return TRUE;

    CHostKey* pHostKey = this->GetHostKey();
    if (!pHostKey)
        return TRUE;

    std::tstring KeyNameRelativeToHost=_T("");
    if (!this->GetKeyPath(KeyNameRelativeToHost))
        return TRUE;
    
    // check if (host,relative key name to host) key is emulated or not
    return pHostKey->IsEmulatedKey((TCHAR*)KeyNameRelativeToHost.c_str());
}

BOOL CKeyReplace::GetKeyPath(OUT std::tstring & RefKeyNameRelativeToHost)
{
    // Get Current Key full path
    CKeyReplace* pKey;
    
    // browse registry tree backward
    std::vector<std::tstring> KeyNames;
    for (pKey = this;!pKey->IsHostKey();pKey = pKey->ParentKey)
    {
        KeyNames.insert(KeyNames.begin(),pKey->KeyName);
    }

    // forge key name relative to host
    RefKeyNameRelativeToHost=_T("");
    SIZE_T KeyNameRelativeToHostDepth = KeyNames.size();
    for (SIZE_T Cnt = 0; Cnt<KeyNameRelativeToHostDepth; Cnt++)
    {
        if (Cnt>0)
            RefKeyNameRelativeToHost+=_T("\\");
        RefKeyNameRelativeToHost+=KeyNames[Cnt];
    }
    return TRUE;
}
BOOL CKeyReplace::GetKeyPathWithoutBaseKeyName(OUT std::tstring & RefKeyNameRelativeToHost)
{
    if (!this->GetKeyPath(RefKeyNameRelativeToHost))
        return FALSE;
        
    TCHAR* psz = _tcschr((TCHAR*)RefKeyNameRelativeToHost.c_str(),'\\');
    if (!psz)
    {
        RefKeyNameRelativeToHost=_T("");
        return TRUE;
    }
    
    // point after '\\'
    psz++;
    
    RefKeyNameRelativeToHost=psz;
    
    return TRUE;
}

void CKeyReplace::Lock()
{
    ::EnterCriticalSection(&this->CriticalSection);
}
void CKeyReplace::Unlock()
{
    ::LeaveCriticalSection(&this->CriticalSection);
}

BOOL CKeyReplace::HasChildKeysEmulated()
{
    BOOL bRetValue = FALSE;
    this->Lock();
    // for each sub key
    std::vector<CKeyReplace*>::iterator iSubKeys;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pSubKey = *iSubKeys;
        // if sub key is emulated
        if (pSubKey->Emulated)
        {
            bRetValue = TRUE;
            break;
        }
        else // sub key not emulated
        {
            // look inside subkey subkeys
            if (pSubKey->HasChildKeysEmulated())
            {
                bRetValue = TRUE;
                break;
            }
            // else
            // continue searching through subkeys
        }
    }
    this->Unlock();
    return bRetValue;
}
SIZE_T CKeyReplace::GetSubKeysCount()
{
    if (this->IsKeyMarkedForDeletion())
        return 0;
    return this->SubKeys.size();
}

SIZE_T CKeyReplace::GetValuesCount()
{
    if (this->IsKeyMarkedForDeletion())
        return 0;

    // in spy mode we must not take into account failure access key
    CRootKey* pRootKey = this->GetRootKey();
    if (pRootKey)
    {
        if (pRootKey->GetSpyMode())
        {
            SIZE_T KeySuccessCount = 0;
            this->Lock();
            std::vector<CKeyValue*>::iterator iValue;
            for (iValue = this->Values.begin();iValue!=this->Values.end();iValue++)
            {
                CKeyValue* pValue = *iValue;
                if (!pValue->IsAccessFailure())
                    KeySuccessCount++;
            }
            this->Unlock();
            return KeySuccessCount;
        }
    }

    // standard mode
    return this->Values.size();
}
SIZE_T CKeyReplace::GetMaxSubKeyLengh()
{
    if (this->IsKeyMarkedForDeletion())
        return 0;
    this->Lock();
    std::vector<CKeyReplace*>::iterator iSubKeys;
    SIZE_T MaxKeyLen = 0;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pSubKey = *iSubKeys;
        MaxKeyLen = __max(MaxKeyLen, pSubKey->KeyName.size()); // _tcslen(pSubKey->KeyName.c_str());
    }
    this->Unlock();
    return MaxKeyLen;
}

SIZE_T CKeyReplace::GetMaxSubKeyClassLengh()
{
    if (this->IsKeyMarkedForDeletion())
        return 0;
    this->Lock();
    std::vector<CKeyReplace*>::iterator iSubKeys;
    SIZE_T MaxClassLen = 0;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pSubKey = *iSubKeys;
        MaxClassLen = __max(MaxClassLen, pSubKey->Class.size()); // _tcslen(pSubKey->Class.c_str());
    }
    this->Unlock();
    return MaxClassLen;
}

SIZE_T CKeyReplace::GetMaxValueNameLengh()
{
    if (this->IsKeyMarkedForDeletion())
        return 0;
    this->Lock();
    std::vector<CKeyValue*>::iterator iValue;
    SIZE_T MaxValueNameLen = 0;
    for (iValue = this->Values.begin();iValue!=this->Values.end();iValue++)
    {
        CKeyValue* pValue = *iValue;
        MaxValueNameLen = __max(MaxValueNameLen, pValue->Name.size()); // _tcslen(pValue->Name.c_str());
    }
    this->Unlock();
    return MaxValueNameLen;
}

CKeyReplace* CKeyReplace::GetOrAddSubKeyWithFullPath(TCHAR* StrKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrKeyName)
        return this;

    if (*StrKeyName==0)
        return this;

    BOOL bKeyWasExisting;
    return GetOrAddSubKeyWithFullPath(StrKeyName,&bKeyWasExisting);
}

CKeyReplace* CKeyReplace::GetSubKeyWithFullPath(TCHAR* StrFullPathKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrFullPathKeyName)
        return this;

    if (*StrFullPathKeyName==0)
        return this;

    TCHAR* Next = _tcschr(StrFullPathKeyName,'\\');
    if (Next)
    {
        CKeyReplace* pReturnedKey = NULL;
        TCHAR* psz = _tcsdup(StrFullPathKeyName);
        Next = _tcschr(psz,'\\');
        if (Next)// should always be the case
        {
            *Next = 0; // end first key name
            Next++;
            CKeyReplace* pKey = this->GetSubKey(psz);
            // if object exists
            if (pKey)
            {
                // let geted object manage the remaining of the path
                pReturnedKey = pKey->GetSubKeyWithFullPath(Next);
            }
            else
                pReturnedKey = NULL;
        }
        free(psz);
        return pReturnedKey;
    }
    else
    {
        return this->GetSubKey(StrFullPathKeyName);
    }
}

CKeyReplace* CKeyReplace::GetOrAddSubKeyWithFullPath(TCHAR* StrFullPathKeyName,BOOL* pKeyWasExisting)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrFullPathKeyName)
        return this;

    if (*StrFullPathKeyName==0)
        return this;

    TCHAR* Next = _tcschr(StrFullPathKeyName,'\\');
    if (Next)
    {
        CKeyReplace* pReturnedKey = NULL;
        TCHAR* psz = _tcsdup(StrFullPathKeyName);
        Next = _tcschr(psz,'\\');
        if (Next)// should always be the case
        {
            *Next = 0; // end first key name
            Next++;
            CKeyReplace* pKey = this->GetOrAddSubKey(psz,pKeyWasExisting);
            // let created object manage the remaining of the path
            pReturnedKey = pKey->GetOrAddSubKeyWithFullPath(Next,pKeyWasExisting);
        }
        free(psz);
        return pReturnedKey;
    }
    else
    {
        return this->GetOrAddSubKey(StrFullPathKeyName,pKeyWasExisting);
    }
}

CKeyReplace* CKeyReplace::GetSubKey(TCHAR* StrKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;
        
    if (StrKeyName==NULL)
        return this;

    if (*StrKeyName==0)
        return this;

    this->Lock();
    std::vector<CKeyReplace*>::iterator iSubKeys;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pKey = *iSubKeys;
        // /!\ windows registry is case insensitive
        if ( ( _tcsicmp(pKey->KeyName.c_str(), StrKeyName)==0 ) // if name match
             && (!pKey->IsKeyMarkedForDeletion()) // if subkey is not marked for deletion
           )
        {
            // key exists, return pointer on it
            this->Unlock();
            return pKey;
        }
    }
    // not found
    this->Unlock();
    return NULL;
}

CKeyReplace* CKeyReplace::GetOrAddSubKey(TCHAR* StrKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrKeyName)
        return this;

    if (*StrKeyName==0)
        return this;

    BOOL bKeyWasExisting;
    return GetOrAddSubKey(StrKeyName,&bKeyWasExisting);
}

CKeyReplace* CKeyReplace::GetOrAddSubKey(TCHAR* StrKeyName,BOOL* pKeyWasExisting)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrKeyName)
        return this;

    if (*StrKeyName==0)
        return this;

    CKeyReplace* pKey = this->GetSubKey(StrKeyName);
    *pKeyWasExisting = (pKey != NULL);
    // if key exists, return key
    if (pKey)
        return pKey;
    // else create key
    return this->AddSubKey(StrKeyName);
}

CKeyReplace* CKeyReplace::GetSubKeyFromIndex(SIZE_T Index)
{
    // in beautiful word 
    // return this->SubKeys[Index];
    // in real life : we have to bypass key marked for deletion
    if (Index>=this->SubKeys.size())
        return NULL;


    CKeyReplace* pReturnedKey = NULL;
    CKeyReplace* pKey = NULL;
    SIZE_T Cnt;
    this->Lock();

    // for each sub key
    std::vector<CKeyReplace*>::iterator iSubKeys;
    for (Cnt = 0,iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        pKey = *iSubKeys;
        if (pKey->IsKeyMarkedForDeletion())
            continue;
        Cnt++;
        if (Cnt==Index)
        {
            pReturnedKey = pKey;
            break;
        }
    }
    this->Unlock();

    return pReturnedKey;
}

CKeyReplace* CKeyReplace::AddSubKey(TCHAR* StrKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrKeyName)
        return NULL;

    if (*StrKeyName==0)
        return NULL;

    CKeyReplace* pNewKey = new CKeyReplace();
    pNewKey->KeyName = StrKeyName;
    pNewKey->ParentKey = this;
    ::GetSystemTimeAsFileTime(&pNewKey->LastWriteTime);
    // checking is necessary as we save not emulated keys names in case there subkeys are emulated
    pNewKey->Emulated = pNewKey->IsEmulatedKey();
    this->SubKeys.push_back(pNewKey);
    CRootKey* pRootKey = this->GetRootKey();
    if (pRootKey)
    {
        pRootKey->RegisterKey(pNewKey);
        pRootKey->NotifyKeyOrValueChange();
    }
    return pNewKey;
}
BOOL CKeyReplace::RemoveSubKey(TCHAR* StrKeyName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (!StrKeyName)
        return FALSE;

    if (*StrKeyName==0)
        return FALSE;

    this->Lock();

    // for each sub key
    std::vector<CKeyReplace*>::iterator iSubKeys;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        CKeyReplace* pKey = *iSubKeys;
        // /!\ windows registry is case insensitive
        if ( _tcsicmp(pKey->KeyName.c_str(), StrKeyName)==0 )
        {
            CRootKey* pRootKey = this->GetRootKey();
            if (pRootKey)
            {
                // pRootKey->UnregisterKey(pKey); let destructor call the unregister function
                pRootKey->NotifyKeyOrValueChange();
            }

            this->SubKeys.erase(iSubKeys);
            delete pKey;
            this->Unlock();
            return TRUE;
        }
    }
    this->Unlock();
    return FALSE;
}

BOOL CKeyReplace::RemoveCurrentKey()
{
    if (this->ParentKey)
        return this->ParentKey->RemoveSubKey((TCHAR*)this->KeyName.c_str());
    return FALSE;
}

CKeyValue* CKeyReplace::GetValueFromIndex(SIZE_T Index)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    if (Index>=this->Values.size())
        return NULL;

    // in spy mode we must not take into account failure access key
    CRootKey* pRootKey = this->GetRootKey();
    if (pRootKey)
    {
        if (pRootKey->GetSpyMode())
        {
            SIZE_T KeySuccessCount = 0;
            this->Lock();
            std::vector<CKeyValue*>::iterator iValue;
            CKeyValue* pValue = NULL;
            CKeyValue* pRetValue = NULL;
            for (iValue = this->Values.begin();iValue!=this->Values.end();iValue++)
            {
                pValue = *iValue;
                if (!pValue->IsAccessFailure())
                {
                    KeySuccessCount++;
                    if (KeySuccessCount == Index)
                    {
                        pRetValue = pValue;
                        break;
                    }
                }
            }
            this->Unlock();
            return pRetValue;
        }
    }

    // standard mode
    return this->Values[Index];
}

// set a value (add value to key if value doesn't exists)
BOOL CKeyReplace::SetValue(TCHAR* lpValueName,SIZE_T Type,PBYTE Buffer,SIZE_T BufferSize)
{
    return this->SetValue(lpValueName,Type,Buffer,BufferSize,FALSE,AccessType_WRITE);
}
// for spying mode, allow to set key and specify an access error
BOOL CKeyReplace::SetValue(TCHAR* lpValueName,SIZE_T Type,PBYTE Buffer,SIZE_T BufferSize,BOOL bError,tagAccessType AccessType)
{
    if (this->IsKeyMarkedForDeletion())
        return FALSE;

    TCHAR* pszValueName;
    if (lpValueName)
        pszValueName = lpValueName;
    else
        pszValueName = _T("");


    CKeyValue* pValue = this->GetOrAddValue(pszValueName);
    if (!pValue)
        return FALSE;

    BOOL UpdateData = TRUE;

    if (AccessType == AccessType_READ)
    {
        pValue->EmulatedRegistry_Flag|=CKeyValue_EmulatedRegistry_Flag_ACCESS_READ;
    }
    else if (AccessType == AccessType_WRITE)
    {
        pValue->EmulatedRegistry_Flag|=CKeyValue_EmulatedRegistry_Flag_ACCESS_WRITE;
    }

    if (bError)
    {
        // never update data in case of error (can be an error more data, in that case buffer can be null and buffer size is not 0)
        UpdateData = FALSE;

        // add error flag
        pValue->EmulatedRegistry_Flag|=CKeyValue_EmulatedRegistry_Flag_ACCESS_ERROR;
    }
    else
    {
        if (Buffer == NULL)
            UpdateData = FALSE;
        else if (::IsBadReadPtr(Buffer,BufferSize))
            UpdateData = FALSE;

        // reset CKeyValue_EmulatedRegistry_Flag_ACCESS_ERROR (error can be due to error more data)
        pValue->EmulatedRegistry_Flag&=~CKeyValue_EmulatedRegistry_Flag_ACCESS_ERROR;

        // RegQueryValue return ERROR_SUCCESS if (Buffer == 0) and (BufferSize!=0)
        // to allow to get required buffer size without generating error
        // so in this case we have no data stored
        // spying mode only
        if (UpdateData)
        {
            // set success flag
            pValue->EmulatedRegistry_Flag|=CKeyValue_EmulatedRegistry_Flag_ACCESS_SUCCESS_AT_LEAST_ONCE;
        }
    }

    // update type anyway
    pValue->Type = Type;

    if (UpdateData)
    {
        pValue->BufferSize = BufferSize;

        // if all buffer was already allocated
        if (pValue->Buffer)
        {
            // free previously allocated memory
            delete[] pValue->Buffer;
            pValue->Buffer = NULL;
        }
        if (pValue->BufferSize>0)
        {
            pValue->Buffer = new BYTE[pValue->BufferSize];
            if (pValue->Buffer == NULL)
            {
                pValue->BufferSize = 0;
                return FALSE;
            }
            memcpy(pValue->Buffer,Buffer,pValue->BufferSize);
        }

        if (AccessType == AccessType_WRITE)
            // update current key last write time
            ::GetSystemTimeAsFileTime(&this->LastWriteTime);
    }

    return TRUE;
}

CKeyValue* CKeyReplace::GetOrAddValue(TCHAR* lpValueName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    TCHAR* pszValueName;
    if (lpValueName)
        pszValueName = lpValueName;
    else
        pszValueName = _T("");

    CKeyValue* pValue = this->GetValue(pszValueName,FALSE);
    if (!pValue)
    {
        pValue = new CKeyValue();
        pValue->Name = pszValueName;
        this->Lock();
        this->Values.push_back(pValue);
        this->Unlock();
        CRootKey* pRootKey = this->GetRootKey();
        if (pRootKey)
        {
            pRootKey->NotifyKeyOrValueChange();
        }
        // update current key last write time
        ::GetSystemTimeAsFileTime(&this->LastWriteTime);
    }
    return pValue;
}
CKeyValue* CKeyReplace::GetValue(TCHAR* lpValueName)
{
    return this->GetValue(lpValueName,TRUE);
}
CKeyValue* CKeyReplace::GetValue(TCHAR* lpValueName,BOOL bReturnNullIfFailureAccess)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    TCHAR* pszValueName;
    if (lpValueName)
        pszValueName = lpValueName;
    else
        pszValueName = _T("");

    this->Lock();
    // for each value
    std::vector<CKeyValue*>::iterator iValues;
    CKeyValue* pValue;
    // CRootKey* pRootKey;// if spying mode // work for both mode : spying and not spying (see below)
    for (iValues = this->Values.begin();iValues!=this->Values.end();iValues++)
    {
        pValue = *iValues;
        // /!\ windows registry is case insensitive
        if ( _tcsicmp(pValue->Name.c_str(), pszValueName)==0 )
        {
            if (bReturnNullIfFailureAccess)
            {
                //// if spying mode // work for both mode : spying and not spying
                //pRootKey = this->GetRootKey();
                //if (pRootKey)
                //{
                //    if (pRootKey->GetSpyMode())
                //    {
                        // if no access success
                        if (pValue->IsAccessFailure())
                            pValue = NULL;
                //    }
                //}
            }

            this->Unlock();
            return pValue;
        }
    }
    // item not found
    this->Unlock();

    return NULL;
}

BOOL CKeyReplace::RemoveValue(TCHAR* lpValueName)
{
    if (this->IsKeyMarkedForDeletion())
        return NULL;

    TCHAR* pszValueName;
    if (lpValueName)
        pszValueName = lpValueName;
    else
        pszValueName = _T("");

    this->Lock();
    // for each value
    std::vector<CKeyValue*>::iterator iValues;
    for (iValues = this->Values.begin();iValues!=this->Values.end();iValues++)
    {
        CKeyValue* pValue = *iValues;
        // /!\ windows registry is case insensitive
        if ( _tcsicmp(pValue->Name.c_str(), pszValueName)==0 )
        {
            CRootKey* pRootKey = this->GetRootKey();
            if (pRootKey)
            {
                pRootKey->NotifyKeyOrValueChange();
            }
            this->Values.erase(iValues);
            delete pValue;
            this->Unlock();
            return TRUE;
        }
    }
    this->Unlock();
    return FALSE;
}

BOOL CKeyReplace::CopyKeyContent(CKeyReplace* pDestKey)
{
    ////////////////////////
    // copy key infos
    ////////////////////////
    pDestKey->KeyName = this->KeyName;
    pDestKey->Class = this->Class;
    ::GetSystemTimeAsFileTime(&pDestKey->LastWriteTime);
    
    this->Lock();
    
    ////////////////////////
    // copy subkeys
    ////////////////////////
    
    // for each subkey
    std::vector<CKeyReplace*>::iterator iSubKeys;
    CKeyReplace* pSubKey;
    CKeyReplace* pDestSubKey;
    for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
    {
        pSubKey = *iSubKeys;
        pDestSubKey = pDestKey->GetOrAddSubKey((TCHAR*)pSubKey->KeyName.c_str());
        pSubKey->CopyKeyContent(pDestSubKey);
    }
    
    ////////////////////////
    // copy values
    ////////////////////////
    
    // for each value
    std::vector<CKeyValue*>::iterator iValues;
    CKeyValue* pValue;
    CKeyValue* pDestValue;
    for (iValues = this->Values.begin();iValues!=this->Values.end();iValues++)
    {
        pValue = *iValues;
        pDestValue = pDestKey->GetOrAddValue((TCHAR*)pValue->Name.c_str());
        pValue->Copy(pDestValue);
    }
    this->Unlock();
    
    return TRUE;
}

BOOL CKeyReplace::Load(TCHAR* KeyContent,BOOL bUnicode)
{
    TCHAR* pszCurrentMarkup = KeyContent;
    TCHAR* pszNextMarkup;
    BOOL bRetValue = TRUE;
    TCHAR* Buffer;

    BOOL bSpyMode = FALSE;
    CRootKey* pRootKey = this->GetRootKey();
    if (pRootKey)
    {
        bSpyMode = pRootKey->GetSpyMode();
    }

    ////////////////////////////
    // load current key infos
    ////////////////////////////

    TCHAR* psz;
    if (CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("NAME"),&psz,bUnicode,&pszNextMarkup))
    {
        this->KeyName = psz;
        delete[] psz;
    }

    if (CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("CLASS"),&psz,bUnicode,&pszNextMarkup))
    {
        this->Class = psz;
        delete[] psz;
    }

    PBYTE BinBuffer;
    SIZE_T BufferSize;
    if (CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("LAST_WRITE_TIME"),&BinBuffer,&BufferSize,&pszNextMarkup))
    {
        if (BufferSize>=sizeof(FILETIME))
            memcpy(&this->LastWriteTime,BinBuffer,sizeof(FILETIME));
        delete[] BinBuffer;
    }
    
    // as parent key is set, we can fill the emulated information field
    this->Emulated = this->IsEmulatedKey();

    ////////////////////////////
    // load subkeys MUST BE DONE BEFORE "load values" for fast parsing
    ////////////////////////////

#ifdef _DEBUG
    TCHAR DebugStr[2048];
    _stprintf(DebugStr,_T("Key %s Begin:\r\n"),this->KeyName.c_str());
    OutputDebugString(DebugStr);
#endif

    TCHAR* SubKeysContent;
    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("SUBKEYS"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        SubKeysContent=new TCHAR[BufferSize+1];
        memcpy(SubKeysContent,Buffer,BufferSize*sizeof(TCHAR));
        SubKeysContent[BufferSize]=0;

        TCHAR* pszSubKeyCurrentMarkup = SubKeysContent;
        TCHAR* pszSubKeyNextMarkup;  
        TCHAR* pszSubKey;   

        while (CXmlLite::ReadXMLMarkupContent(pszSubKeyCurrentMarkup,_T("KEY"),&Buffer,&BufferSize,&pszSubKeyNextMarkup))
        {
            pszSubKey=new TCHAR[BufferSize+1];
            memcpy(pszSubKey,Buffer,BufferSize*sizeof(TCHAR));
            pszSubKey[BufferSize]=0;

            pszSubKeyCurrentMarkup=pszSubKey;

            // load subkey content (and subkey-subkeys) by a recursive call
            CKeyReplace* pSubKey = new CKeyReplace();
            pSubKey->ParentKey = this; // must be set before loading for correct state of emulated flag
            if (pSubKey->Load(pszSubKey,bUnicode))
            {
                this->SubKeys.push_back(pSubKey);
                if (pRootKey)
                    pRootKey->RegisterKey(pSubKey);
            }
            else
            {
                delete pSubKey;
                bRetValue = FALSE;
            }


            delete[] pszSubKey;
            pszSubKeyCurrentMarkup=pszSubKeyNextMarkup;        
        }

        delete[] SubKeysContent;
    }

    // important : point after <subkeys> --> can't have mix for Values and a subkey values
    pszCurrentMarkup = pszNextMarkup;

    ////////////////////////////
    // load values
    ////////////////////////////
    TCHAR* ValuesContent;
    if (CXmlLite::ReadXMLMarkupContent(pszCurrentMarkup,_T("VALUES"),&Buffer,&BufferSize,&pszNextMarkup))
    {
        ValuesContent=new TCHAR[BufferSize+1];
        memcpy(ValuesContent,Buffer,BufferSize*sizeof(TCHAR));
        ValuesContent[BufferSize]=0;

        TCHAR* pszValueCurrentMarkup = ValuesContent;
        TCHAR* pszValueNextMarkup;
        TCHAR* pszTmp;
        TCHAR* pszValue;
        SIZE_T Tmp;
        TCHAR* Buffer2;

#ifdef _DEBUG
        _stprintf(DebugStr,_T("Key %s Values:\r\n"),this->KeyName.c_str());
        OutputDebugString(DebugStr);
#endif
        // while we find another VALUE tag
        while (CXmlLite::ReadXMLMarkupContent(pszValueCurrentMarkup,_T("VALUE"),&Buffer2,&BufferSize,&pszValueNextMarkup))
        {
            CKeyValue* pValue = new CKeyValue();
            pszValue=new TCHAR[BufferSize+1];
            memcpy(pszValue,Buffer2,BufferSize*sizeof(TCHAR));
            pszValue[BufferSize]=0;

            pszValueCurrentMarkup=pszValue;

            if (CXmlLite::ReadXMLValue(pszValue,_T("NAME"),&psz,bUnicode,&pszTmp))
            {
                pValue->Name = psz;
                delete[] psz;
            }

            if (CXmlLite::ReadXMLValue(pszValue,_T("TYPE"),&Tmp,&pszTmp))
            {
                pValue->Type = Tmp;
            }

            if (CXmlLite::ReadXMLValue(pszValue,_T("CONTENT"),&BinBuffer,&BufferSize,&pszTmp))
            {
                pValue->Buffer = BinBuffer;
                pValue->BufferSize = BufferSize;
            }            

            // set success flag (avoid IsAccessFailure check  inside GetValue call to return TRUE)
            pValue->EmulatedRegistry_Flag = CKeyValue_EmulatedRegistry_Flag_ACCESS_SUCCESS_AT_LEAST_ONCE;
            if (bSpyMode)
            {
                if (CXmlLite::ReadXMLValue(pszValue,_T("FLAGS"),&Tmp,&pszTmp))
                {
                    pValue->EmulatedRegistry_Flag = Tmp;
                }
            }

#ifdef _DEBUG
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
            switch(pValue->Type)
            {
            case REG_NONE:
            default:
                _stprintf(DebugStr,_T("Value %s %u\r\n"),pValue->Name.c_str(),pValue->Type);
                break;

            // case REG_DWORD: // REG_DWORD == REG_DWORD_LITTLE_ENDIAN
            case REG_DWORD_LITTLE_ENDIAN:
            case REG_DWORD_BIG_ENDIAN:
                if (pValue->Buffer) // can be null for access error in spy mode
                    _stprintf(DebugStr,_T("Value %s %u:0x%.8X\r\n"),pValue->Name.c_str(),pValue->Type,*((DWORD*)pValue->Buffer));
                else
                    _stprintf(DebugStr,_T("Value %s %u:0x (no value)\r\n"),pValue->Name.c_str(),pValue->Type);
                break;

            case REG_SZ:
            case REG_EXPAND_SZ:
                {
                    TCHAR* str;
                    BOOL bAnsi;
                    if (pValue->GetStringValue(&str,&bAnsi))
                    {
                        _sntprintf(DebugStr,2000,_T("Value %s %u:%s\r\n"),pValue->Name.c_str(),pValue->Type,str);
                        delete[] str;
                    }
                }
                break;
            }
            OutputDebugString(DebugStr);
#endif

            this->Values.push_back(pValue);

            delete[] pszValue;
            pszValueCurrentMarkup=pszValueNextMarkup;
        }
        delete[] ValuesContent;
    }
#ifdef _DEBUG
    _stprintf(DebugStr,_T("Key %s End.\r\n"),this->KeyName.c_str());
    OutputDebugString(DebugStr);
#endif

    return bRetValue;
}

BOOL CKeyReplace::Save(HANDLE hFile)
{
    if (this->IsKeyMarkedForDeletion())
        return FALSE;

    BOOL bSpyMode = FALSE;
    CRootKey* pRootKey = this->GetRootKey();
    if (pRootKey)
    {
        bSpyMode = pRootKey->GetSpyMode();
    }

    BOOL bHasChildKeysEmulated = this->HasChildKeysEmulated();
    if ( (!this->Emulated) && (!bHasChildKeysEmulated) )
    {
        if (!bSpyMode)
        {
            // key is not emulated and none of it's subkey is emulated --> don't need to store empty information on disk (avoid big files with no infos)
            return TRUE;
        }
    }
    
    CTextFile::WriteText(hFile,_T("<KEY>"));

    // save key infos
    CXmlLite::WriteXMLValue(hFile,_T("NAME"),(TCHAR*)this->KeyName.c_str());
    CXmlLite::WriteXMLValue(hFile,_T("CLASS"),(TCHAR*)this->Class.c_str());
    CXmlLite::WriteXMLValue(hFile,_T("LAST_WRITE_TIME"),(PBYTE)&this->LastWriteTime,sizeof(FILETIME));

    // trick for fast parsing SUBKEYS MUST BE BEFORE VALUES, so looking for values after subkeys avoid to fail in a subkey values
    CTextFile::WriteText(hFile,_T("<SUBKEYS>"));

    // if no child key is emulated, don't save infos (avoid big files with no infos)
    if (bHasChildKeysEmulated || bSpyMode)
    {
        // for each subkey, subkey->Save(hFile);  
        std::vector<CKeyReplace*>::iterator iSubKeys;
        for (iSubKeys = this->SubKeys.begin();iSubKeys!=this->SubKeys.end();iSubKeys++)
        {
            CKeyReplace* pKey = *iSubKeys;
            pKey->Save(hFile); 
        }
    }
    CTextFile::WriteText(hFile,_T("</SUBKEYS>"));


    // keep VALUES markup even if value not saved
    CTextFile::WriteText(hFile,_T("<VALUES>"));

    // save values only for emulated keys (keep VALUES markup)
    if (this->Emulated || bSpyMode)
    {        
        // for each value, save value
        std::vector<CKeyValue*>::iterator iValues;
        for (iValues = this->Values.begin();iValues!=this->Values.end();iValues++)
        {
            CTextFile::WriteText(hFile,_T("<VALUE>"));
            CKeyValue* pValue = *iValues;
            CXmlLite::WriteXMLValue(hFile,_T("NAME"),(TCHAR*)pValue->Name.c_str());
            CXmlLite::WriteXMLValue(hFile,_T("TYPE"),pValue->Type);
            CXmlLite::WriteXMLValue(hFile,_T("CONTENT"),pValue->Buffer,pValue->BufferSize);

            if (bSpyMode)
                CXmlLite::WriteXMLValue(hFile,_T("FLAGS"),pValue->EmulatedRegistry_Flag);

            CTextFile::WriteText(hFile,_T("</VALUE>"));
        }
    }
    CTextFile::WriteText(hFile,_T("</VALUES>"));


    CTextFile::WriteText(hFile,_T("</KEY>"));

    return TRUE;
}

}