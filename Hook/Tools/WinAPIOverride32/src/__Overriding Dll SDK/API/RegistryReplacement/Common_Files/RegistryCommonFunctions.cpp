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


#include "RegistryCommonFunctions.h"

namespace EmulatedRegistry
{

BOOL DoesFileExists(TCHAR* const FullPath)
{
    BOOL bFileExists=FALSE;

    HANDLE hFile=::CreateFile(FullPath,
        FILE_READ_ATTRIBUTES, 
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING, 
        0, 
        NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        ::CloseHandle(hFile);
    }
    return bFileExists;
}

BOOL IsBaseKey(HKEY const hKey)
{
    switch ((INT)hKey)
    {
    case HKEY_CLASSES_ROOT                 :
    case HKEY_CURRENT_USER                 :
    case HKEY_LOCAL_MACHINE                :
    case HKEY_USERS                        :
    case HKEY_PERFORMANCE_DATA             :
    case HKEY_PERFORMANCE_TEXT             :
    case HKEY_PERFORMANCE_NLSTEXT          :
    case HKEY_CURRENT_CONFIG               :
    case HKEY_DYN_DATA                     :
    case HKEY_CURRENT_USER_LOCAL_SETTINGS  :
        return TRUE;
        break;
    }
    return FALSE;
}

TCHAR* BaseKeyToString(HKEY const hKey)
{
    switch ((INT)hKey)
    {
    case HKEY_CLASSES_ROOT                 :
        return _T("HKEY_CLASSES_ROOT");
        break;
    case HKEY_CURRENT_USER                 :
        return _T("HKEY_CURRENT_USER");
        break;
    case HKEY_LOCAL_MACHINE                :
        return _T("HKEY_LOCAL_MACHINE");
        break;
    case HKEY_USERS                        :
        return _T("HKEY_USERS");
        break;
    case HKEY_PERFORMANCE_DATA             :
        return _T("HKEY_PERFORMANCE_DATA");
        break;
    case HKEY_PERFORMANCE_TEXT             :
        return _T("HKEY_PERFORMANCE_TEXT");
        break;
    case HKEY_PERFORMANCE_NLSTEXT          :
        return _T("HKEY_PERFORMANCE_NLSTEXT");
        break;
    case HKEY_CURRENT_CONFIG               :
        return _T("HKEY_CURRENT_CONFIG");
        break;
    case HKEY_DYN_DATA                     :
        return _T("HKEY_DYN_DATA");
        break;
    case HKEY_CURRENT_USER_LOCAL_SETTINGS  :
        return _T("HKEY_CURRENT_USER_LOCAL_SETTINGS");
        break;
    }
    return _T("");
}
HKEY StringToBaseKey(TCHAR* const BaseKeyName)
{
    if (_tcsicmp(BaseKeyName,_T("HKEY_CLASSES_ROOT")) == 0)
        return HKEY_CLASSES_ROOT;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_CURRENT_USER")) == 0)
        return HKEY_CURRENT_USER;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_LOCAL_MACHINE")) == 0)    
        return HKEY_LOCAL_MACHINE;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_USERS")) == 0)
        return HKEY_USERS;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_PERFORMANCE_DATA")) == 0)
        return HKEY_PERFORMANCE_DATA;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_PERFORMANCE_TEXT")) == 0)
        return HKEY_PERFORMANCE_TEXT;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_PERFORMANCE_NLSTEXT")) == 0)    
        return HKEY_PERFORMANCE_NLSTEXT;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_CURRENT_CONFIG")) == 0)
        return HKEY_CURRENT_CONFIG;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_DYN_DATA")) == 0)
        return HKEY_DYN_DATA;
    else
    if (_tcsicmp(BaseKeyName,_T("HKEY_CURRENT_USER_LOCAL_SETTINGS")) == 0)    
        return HKEY_CURRENT_USER_LOCAL_SETTINGS;
    else
        return NULL;
}
}