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
#include "Key.h"

namespace EmulatedRegistry
{

class CHostKey: public CKeyReplace
{
public:
    typedef enum tagKeysFilteringType
    {
        KeysFilteringType_ONLY_SPECIFIED=0,
        KeysFilteringType_ALL_BUT_SPECIFIED=1
    }KEYS_FILTERING_TYPE;

    // saved fields
    KEYS_FILTERING_TYPE KeysFilteringType;
    std::vector<std::tstring*> SpecifiedKeys; // typically HKLM/Software/Compagny/Product  and HKCU/Software/Compagny/Product
    BOOL IsHostConnected; // meaning full only for direct child of root key (host abstract key)
    BOOL AllowConnectionToRemoteHost; // if FALSE only key imported during configuration file creation with EmulatedRegistryEditor.exe will be accessible (meaning full only for remote host not for the local one)
    BOOL DisableWriteOperationsOnNotEmulatedKeys;// TRUE to Disable write

    CHostKey();
    virtual ~CHostKey();// must be virtual
    BOOL IsLocalHost();
    BOOL IsEmulatedKey(TCHAR* FullKeyName);
    BOOL Load(TCHAR* KeyContent,BOOL bUnicodeSave);
    BOOL Save(HANDLE hFile);
    void RemoveAllSpecifiedKeys();
    BOOL RemoveSpecifiedKey(TCHAR* KeyName);
};

}