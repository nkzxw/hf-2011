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

class CEmulatedRegistry;

class CRootKey:public CKeyReplace
{
private:
    BOOL bSpyMode;
    BOOL bHasChangedSinceLastSave;
public:
    CRootKey(CEmulatedRegistry* pEmulatedRegistry);
    virtual ~CRootKey();
    BOOL AddHost(CHostKey* pHost);
    CHostKey* AddHost(TCHAR* HostName);
    BOOL Load(TCHAR* RootContent,BOOL bUnicodeSave);
    BOOL Save(HANDLE hFile);
    BOOL IsValidKey(CKeyReplace* pKey);
    BOOL RegisterKey(CKeyReplace* pKey);
    BOOL UnregisterKey(CKeyReplace* pKey);
    CEmulatedRegistry* pEmulatedRegistry;
    void SetSpyMode();
    BOOL GetSpyMode();

    FORCEINLINE void NotifyKeyOrValueChange() {this->bHasChangedSinceLastSave=TRUE; }
    FORCEINLINE BOOL HasChangedSinceLastSave() const {return this->bHasChangedSinceLastSave; }
};

}