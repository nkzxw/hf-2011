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

#pragma once

#include "RegistryCommonFunctions.h"

namespace EmulatedRegistry
{

class CHostKey;
class CRootKey;

#define CKeyValue_EmulatedRegistry_Flag_ACCESS_ERROR 0x1
#define CKeyValue_EmulatedRegistry_Flag_ACCESS_SUCCESS_AT_LEAST_ONCE 0x2
#define CKeyValue_EmulatedRegistry_Flag_ACCESS_READ  0x4
#define CKeyValue_EmulatedRegistry_Flag_ACCESS_WRITE 0x8

class CKeyValue
{
public:
    // save fields
    SIZE_T Type;
    std::tstring Name;
    SIZE_T BufferSize;
    PBYTE Buffer;

    // saved only in spy mode
    SIZE_T EmulatedRegistry_Flag;

    CKeyValue ()
    {
        this->Name = _T("");
        this->Type = REG_SZ; // better to conform MSDN RegQueryValue / RegSetValue
        this->Buffer = NULL;
        this->BufferSize = 0;
        this->EmulatedRegistry_Flag = 0;
    }
    ~CKeyValue ()
    {
        if (this->Buffer)
            delete[] this->Buffer;
    }
    BOOL IsAccessFailure()
    {
        // use CKeyValue_EmulatedRegistry_Flag_ACCESS_SUCCESS_AT_LEAST_ONCE as error can be due to error more data
        return ( !(this->EmulatedRegistry_Flag & CKeyValue_EmulatedRegistry_Flag_ACCESS_SUCCESS_AT_LEAST_ONCE) );
    }
    BOOL Copy(CKeyValue* pDestValue)
    {
        pDestValue->Type = this->Type;
        pDestValue->Name = this->Name;
        pDestValue->BufferSize = this->BufferSize;
        if (pDestValue->Buffer)
            delete[] pDestValue->Buffer;
        pDestValue->Buffer = new BYTE[pDestValue->BufferSize];
        memcpy(pDestValue->Buffer,this->Buffer,pDestValue->BufferSize);
        return TRUE;
    }
    BOOL GetStringValue(OUT TCHAR** pStr,OUT BOOL* pbAnsiString)
    {
        // string values should be encoded in unicode with an ending \0,
        // but it's not always the case we can found not ended unicode, ansi and not ended ansi
        *pbAnsiString = FALSE;

        TCHAR* str;
        if (this->BufferSize==0)
        {
            str = new TCHAR[1];
            *str = 0;
            *pStr = str;
            return TRUE;
        }

        // copy the buffer with an extra WCHAR set to 0 (assume string is ended for unicode and ansi string)
        SIZE_T SecureBufferSize = this->BufferSize+sizeof(WCHAR);
        PBYTE SecureBuffer = new BYTE[SecureBufferSize];

        // copy value buffer
        memcpy(SecureBuffer,this->Buffer,this->BufferSize);
        // force 0 ending (not always the case
        memset(&SecureBuffer[this->BufferSize],0,sizeof(WCHAR));

        // try to detect ansi string
        if ( (this->BufferSize<2) // --> as ==0 check was done that means string size in byte is 1 --> ansi
            || (this->BufferSize%2 != 0) // number of byte is not multiple of sizeof(WCHAR) --> ansi
            )
        {
            *pbAnsiString = TRUE;
        }
        else
        {
            // if 2 last bytes are null --> unicode string
            if ( (SecureBuffer[this->BufferSize-1] ==0) && (SecureBuffer[this->BufferSize-2] ==0) )
            {
                // nothing to do
            }
            else // we have to guess encoding
            {
                // unicode string usually get at least one wchar with a byte at 0
                SIZE_T AnsiSize = strlen((CHAR*)SecureBuffer);
                if ( (AnsiSize == this->BufferSize) //  not 0 terminated ansi
                    || ( AnsiSize == this->BufferSize-1) // 0 terminated ansi
                    )
                {
                    *pbAnsiString = TRUE;
                }
            }
        }


        SIZE_T StringLen;

        if (*pbAnsiString)
            StringLen = this->BufferSize/sizeof(CHAR)+1; // add an extra char in case no \0 inside saved string
        else
            StringLen = this->BufferSize/sizeof(WCHAR)+1; // add an extra char in case no \0 inside saved string

        str = new TCHAR[StringLen]; 
        if (!str)
        {
            delete[] SecureBuffer;
            return FALSE;
        }

        if (*pbAnsiString)
            CAnsiUnicodeConvert::AnsiToTchar((CHAR*)SecureBuffer,str,StringLen);
        else
            CAnsiUnicodeConvert::UnicodeToTchar((WCHAR*)SecureBuffer,str,StringLen);

        *pStr = str;
        delete[] SecureBuffer;
        return TRUE;
    }
};

class CKeyReplace
{
protected:

    // unsaved fields
    CRITICAL_SECTION CriticalSection;
    BOOL bRootKey;
    SIZE_T OpenedHandleCount;
    BOOL AskedForDeletion;

    CKeyValue* GetValue(TCHAR* lpValueName,BOOL bReturnNullIfFailureAccess);
public:
    // saved fields
    std::tstring KeyName;
    std::vector<CKeyReplace*> SubKeys;
    std::vector<CKeyValue*> Values;
    std::tstring Class;
    FILETIME LastWriteTime;

    // unsaved fields
    CKeyReplace* ParentKey;
    BOOL Emulated;
    HKEY hRealRegistryKeyHandleIfNotEmulated;    

    enum tagAccessType
    {
        AccessType_READ,
        AccessType_WRITE
    };

    CKeyReplace();
    virtual ~CKeyReplace();// virtual is required don't remove it
    BOOL IsHostKey();
    CHostKey* GetHostKey();
    CRootKey* GetRootKey();
    BOOL IsRootKey();
    BOOL IsBaseKey();
    BOOL IsWriteAccessAllowed();
    BOOL IsEmulatedKey();
    CKeyReplace* GetBaseKey();
    BOOL GetKeyPath(OUT std::tstring & RefKeyNameRelativeToHost);
    BOOL GetKeyPathWithoutBaseKeyName(OUT std::tstring & RefKeyNameRelativeToHost);
    void Lock();
    void Unlock();
    
    BOOL HasChildKeysEmulated();
    SIZE_T GetSubKeysCount();
    SIZE_T GetValuesCount();
    SIZE_T GetMaxSubKeyLengh();
    SIZE_T GetMaxSubKeyClassLengh();
    SIZE_T GetMaxValueNameLengh();
    CKeyReplace* GetOrAddSubKeyWithFullPath(TCHAR* StrKeyName);
    CKeyReplace* GetSubKeyWithFullPath(TCHAR* StrFullPathKeyName);
    CKeyReplace* GetOrAddSubKeyWithFullPath(TCHAR* StrFullPathKeyName,BOOL* pKeyWasExisting);
    CKeyReplace* GetSubKey(TCHAR* StrKeyName);
    CKeyReplace* GetOrAddSubKey(TCHAR* StrKeyName);
    CKeyReplace* GetOrAddSubKey(TCHAR* StrKeyName,BOOL* pKeyWasExisting);
    CKeyReplace* GetSubKeyFromIndex(SIZE_T Index);
    CKeyReplace* AddSubKey(TCHAR* StrKeyName);
    BOOL RemoveSubKey(TCHAR* StrKeyName);
    BOOL RemoveCurrentKey();
    CKeyValue* GetValueFromIndex(SIZE_T Index);
    BOOL SetValue(TCHAR* lpValueName,SIZE_T Type,PBYTE Buffer,SIZE_T BufferSize);
    BOOL SetValue(TCHAR* lpValueName,SIZE_T Type,PBYTE Buffer,SIZE_T BufferSize,BOOL bError,tagAccessType AccessType);
    CKeyValue* GetOrAddValue(TCHAR* lpValueName);
    CKeyValue* GetValue(TCHAR* lpValueName);
    BOOL RemoveValue(TCHAR* lpValueName);
    void MarkKeyForDeletion(BOOL bMarkSubKeysForDeletion);
    BOOL IsKeyMarkedForDeletion();
    SIZE_T GetOpenedHandleCount();
    SIZE_T IncreaseOpenedHandleCount();
    SIZE_T DecreaseOpenedHandleCount();
    BOOL CopyKeyContent(CKeyReplace* pDestKey);
    
    BOOL Load(TCHAR* KeyContent,BOOL bUnicode);
    BOOL Save(HANDLE hFile);
};
}