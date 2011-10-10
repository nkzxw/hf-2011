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
// Object: IID <--> Interface name and CLSID <--> class name convert routines
//-----------------------------------------------------------------------------
#include "guidstringconvert.h"
// CGUIDStringConvert_INTERFACES_REGISTRY_KEY from hkcr
#define CGUIDStringConvert_INTERFACES_REGISTRY_KEY _T("Interface\\")
// CGUIDStringConvert_CLASSES_REGISTRY_KEY from hkcr
#define CGUIDStringConvert_CLASSES_REGISTRY_KEY  _T("CLSID\\")

//-----------------------------------------------------------------------------
// Name: IsKnownCLSID
// Object: check if a clsid is known for current computer
// Parameters :
//     in  : TCHAR* pszClsid : class identifier
//     return : TRUE if clsid is known
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::IsKnownCLSID(IN TCHAR* pszClsid)
{
    if (!pszClsid)
        return FALSE;
    HKEY hKeyInterface;
    TCHAR lpSubKey[128];
    _tcscpy(lpSubKey,CGUIDStringConvert_CLASSES_REGISTRY_KEY);
    _tcscat(lpSubKey,pszClsid);
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,lpSubKey,0,KEY_READ,&hKeyInterface)!=ERROR_SUCCESS)
        return FALSE;
    RegCloseKey(hKeyInterface);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsKnownIID
// Object: check if an iid is known for current computer
// Parameters :
//     in  : TCHAR* pszIID : interface identifier
//     return : TRUE if iid is known
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::IsKnownIID(IN TCHAR* pszIID)
{
    if (!pszIID)
        return FALSE;
    HKEY hKeyInterface;
    TCHAR lpSubKey[128];
    _tcscpy(lpSubKey,CGUIDStringConvert_INTERFACES_REGISTRY_KEY);
    _tcscat(lpSubKey,pszIID);
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,lpSubKey,0,KEY_READ,&hKeyInterface)!=ERROR_SUCCESS)
        return FALSE;
    RegCloseKey(hKeyInterface);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: TcharFromCLSID
// Object: get string representation of class identifier
// Parameters :
//     in  : CLSID* pClsid : class identifier
//     out : TCHAR* pszCLSID : string representation of class identifier
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::TcharFromCLSID(IN CLSID* pClsid,OUT TCHAR* pszCLSID)
{
    WCHAR* pwcClsid;

    *pszCLSID=0;
    if (SUCCEEDED(StringFromCLSID(*pClsid,&pwcClsid)))
    {

#if (defined(UNICODE)||defined(_UNICODE))
        _tcscpy(pszCLSID,pwcClsid);
#else
        CHAR* pc;
        CAnsiUnicodeConvert::UnicodeToAnsi(pwcClsid,&pc);
        _tcscpy(pszCLSID,pc);
        free(pc);
#endif
        // free memory allocated by StringFromCLSID 
        CoTaskMemFree(pwcClsid);

        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CLSIDFromTchar
// Object: get class identifier from string representation
// Parameters :
//     in  : TCHAR* pszCLSID : string representation of class identifier
//     out : CLSID* pClsid : class identifier
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::CLSIDFromTchar(IN TCHAR* pszClsid,OUT CLSID* pClsid)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SUCCEEDED(CLSIDFromString(pszClsid,pClsid));
#else
    WCHAR* pwc;
    CAnsiUnicodeConvert::AnsiToUnicode(pszClsid,&pwc);
    BOOL bRes=SUCCEEDED(CLSIDFromString(pwc,pClsid));
    free(pwc);
    return bRes;
#endif
}

//-----------------------------------------------------------------------------
// Name: GetClassProgId
// Object: get string representation of class prog Id from class identifier
// Parameters :
//     in  : CLSID* pClsid : class identifier
//           LONG pszProgIdNameMaxSize : prog id max name representation
//     out : TCHAR* pszProgId : prog id
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::GetClassProgId(IN CLSID* pClsid,OUT TCHAR* pszProgId,IN LONG pszProgIdNameMaxSize)
{
    WCHAR* pwcProgId;

    *pszProgId=0;
    if (SUCCEEDED(ProgIDFromCLSID(*pClsid,&pwcProgId)))
    {
#if (defined(UNICODE)||defined(_UNICODE))
        _tcsncpy(pszProgId,pwcProgId,pszProgIdNameMaxSize);
        pszProgId[pszProgIdNameMaxSize-1]=0;
#else
        CHAR* pc;
        CAnsiUnicodeConvert::UnicodeToAnsi(pwcProgId,&pc);
        _tcsncpy(pszProgId,pc,pszProgIdNameMaxSize);
        free(pc);
#endif
        // free memory allocated by ProgIDFromCLSID 
        CoTaskMemFree(pwcProgId);

        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CLSIDFromProgId
// Object: get class identifier from prog id
// Parameters :
//     in  : TCHAR* pszProgId : prog id
//     out : CLSID* pClsid : class identifier
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::CLSIDFromProgId(IN TCHAR* pszProgId,OUT CLSID* pClsid)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SUCCEEDED(CLSIDFromProgID(pszProgId,pClsid));
#else
    WCHAR* pwc;
    CAnsiUnicodeConvert::AnsiToUnicode(pszProgId,&pwc);
    BOOL bRes=SUCCEEDED(CLSIDFromString(pwc,pClsid));
    free(pwc);
    return bRes;
#endif
}

//-----------------------------------------------------------------------------
// Name: GetClassName
// Object: get name associated to class identifier
// Parameters :
//     in  : CLSID* pClsid : class identifier
//           LONG pszCLSIDNameMaxSize : pszCLSIDName max size
//     out : TCHAR* pszCLSIDName : class name
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::GetClassName(IN CLSID* pClsid,OUT TCHAR* pszCLSIDName,IN LONG pszCLSIDNameMaxSize)
{
    WCHAR* pwcProgId;

    *pszCLSIDName=0;
    if (SUCCEEDED(OleRegGetUserType(*pClsid,USERCLASSTYPE_FULL,&pwcProgId)))
    {
#if (defined(UNICODE)||defined(_UNICODE))
        _tcsncpy(pszCLSIDName,pwcProgId,pszCLSIDNameMaxSize);
        pszCLSIDName[pszCLSIDNameMaxSize-1]=0;
#else
        CHAR* pc;
        CAnsiUnicodeConvert::UnicodeToAnsi(pwcProgId,&pc);
        _tcsncpy(pszCLSIDName,pc,pszCLSIDNameMaxSize);
        free(pc);
#endif
        // free memory allocated by ProgIDFromCLSID 
        CoTaskMemFree(pwcProgId);

        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: TcharFromIID
// Object: get string representation of interface identifier
// Parameters :
//     in  : CLSID* pszIID : interface identifier
//     out : TCHAR* pszCLSID : string representation of interface identifier
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::TcharFromIID(IN IID* pIid,OUT TCHAR* pszIID)
{
    WCHAR* pwcIid;

    *pszIID=0;

    if (SUCCEEDED(StringFromIID(*pIid,&pwcIid)))
    {
#if (defined(UNICODE)||defined(_UNICODE))
        _tcscpy(pszIID,pwcIid);
#else
        CHAR* pc;
        CAnsiUnicodeConvert::UnicodeToAnsi(pwcIid,&pc);
        _tcscpy(pszIID,pc);
        free(pc);
#endif
        // free memory allocated by StringFromIID 
        CoTaskMemFree(pwcIid);

        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: IIDFromTchar
// Object: get interface identifier from string representation
// Parameters :
//     in  : TCHAR* pszCLSID : string representation of interface identifier
//     out : CLSID* pszIID : interface identifier
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::IIDFromTchar(IN TCHAR* pszIID,OUT IID* pIid)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SUCCEEDED(IIDFromString(pszIID,pIid));
#else
    WCHAR* pwc;
    CAnsiUnicodeConvert::AnsiToUnicode(pszIID,&pwc);
    BOOL bRes=SUCCEEDED(IIDFromString(pwc,pIid));
    free(pwc);
    return bRes;
#endif
}

//-----------------------------------------------------------------------------
// Name: GetInterfaceName
// Object: get string representation of interface identifier
// Parameters :
//     in  : TCHAR* pszIID : interface identifier
//           LONG pszIIDNameMaxSize : pszIIDName max size
//     out : TCHAR* pszIIDName :  interface name
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::GetInterfaceName(IN TCHAR* pszIID,OUT TCHAR* pszIIDName,IN LONG pszIIDNameMaxSize)
{
    HKEY hKeyInterface;
    TCHAR lpSubKey[128];
    // assume psziid is ended to avoid buffer overflow
    if (_tcslen(pszIID)>38)
        pszIID[38]=0;

    _tcscpy(lpSubKey,CGUIDStringConvert_INTERFACES_REGISTRY_KEY);
    _tcscat(lpSubKey,pszIID);
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,lpSubKey,0,KEY_READ,&hKeyInterface)!=ERROR_SUCCESS)
        return FALSE;
    if(RegQueryValue(hKeyInterface,NULL,pszIIDName,&pszIIDNameMaxSize)!=ERROR_SUCCESS)
        return FALSE;
    RegCloseKey(hKeyInterface);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IIDFromInterfaceName
// Object: get interface identifier from an interface name
//              WARNING AN INTERFACE NAME CAN HAVE MULTIPLE IID IN REGISTRY !!!
//              THIS FUNCTION ONLY RETURN THE FIRST IID FOUND
// Parameters :
//     in  : TCHAR* pszIIDName : interface name
//     out : TCHAR* pszIID : first interface identifier matching interface name
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGUIDStringConvert::IIDFromInterfaceName(IN TCHAR* pszIIDName,OUT TCHAR* pszIID)
{
    DWORD cnt;
    TCHAR pszKey[128];
    TCHAR SubKeyName[CGUIDSTRINGCONVERT_STRING_GUID_SIZE];
    DWORD SubKeyNameSize;
    TCHAR IIDName[MAX_PATH];
    LONG IIDNameSize;
    DWORD NbSubKeys;
    HKEY hKeyInterfaces;
    LONG Ret;

    _tcscpy(pszKey,CGUIDStringConvert_INTERFACES_REGISTRY_KEY);
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,pszKey,0,KEY_READ,&hKeyInterfaces)!=ERROR_SUCCESS)
        return FALSE;

    // Get the class name and the value count. 
    RegQueryInfoKey(hKeyInterfaces,// key handle 
                    NULL,      // buffer for class name 
                    NULL,      // size of class string 
                    NULL,      // reserved 
                    &NbSubKeys,// number of subkeys 
                    NULL,      // longest subkey size 
                    NULL,      // longest class string 
                    NULL,      // number of values for this key 
                    NULL,      // longest value name 
                    NULL,      // longest value data 
                    NULL,      // security descriptor 
                    NULL);     // last write time 


    // Enumerate the child keys
    for (cnt = 0, Ret = ERROR_SUCCESS; 
        (Ret == ERROR_SUCCESS) && (cnt<NbSubKeys);
        cnt++) 
    { 
        SubKeyNameSize=CGUIDSTRINGCONVERT_STRING_GUID_SIZE;
        Ret = RegEnumKeyEx(hKeyInterfaces, 
                            cnt, 
                            SubKeyName, 
                            &SubKeyNameSize, 
                            NULL, 
                            NULL, 
                            NULL, 
                            NULL); 
        if (Ret != ERROR_SUCCESS) 
            break;

        IIDNameSize=MAX_PATH;
        if (RegQueryValue(hKeyInterfaces,SubKeyName,IIDName,&IIDNameSize)!=ERROR_SUCCESS)
            continue;

        // if Interface name has been found
        if (_tcscmp(pszIIDName,IIDName)==0)
        {
            _tcscpy(pszIID,SubKeyName);
            RegCloseKey(hKeyInterfaces);
            return TRUE;
        }
    } 
    RegCloseKey(hKeyInterfaces);
    return FALSE;
}