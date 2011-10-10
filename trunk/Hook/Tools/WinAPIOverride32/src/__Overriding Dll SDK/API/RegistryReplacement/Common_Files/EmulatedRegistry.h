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

#include "RootKey.h"
#include "Options.h"
#include "HostKey.h"
#include "RegistryCommonFunctions.h"
#include "LinkListSimple.h"

// Registry emulation view :
// Rootkey is an abstract key containing the full registry
// HostKeys are classical windows registry view.(HKLM, HKCR,....). There can be the local machine registry (pKeyLocalHost) or remote registries 
//RootKey -----   HostKey0 ----- Key0 -----Key0.0 ----- Key0.0.0
//       |                |                |-----Key0.1 ----- Key0.1.0
//       |                |                |-----Key0.2 ----- Key0.2.0
//       |                |                            |----- Key0.2.1
//       |                |----- Key1 -----Key1.0 ----- Key1.0.0
//       |                            |-----Key1.1 ----- Key1.1.0
//       |                            |           |----- Key1.1.1
//       |                            |-----Key1.2 ----- Key1.2.0
//       | -----   HostKey1 ----- Key0 ...
//       | -----   HostKey2 ----- Key0 ...
namespace EmulatedRegistry
{

class CEmulatedRegistry
{
protected:
    static HMODULE User32Module;
    typedef int (__stdcall *pMessageBox)(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType);
    static pMessageBox pfpMessageBox;
    
    pMessageBox GetMessageBox();
public:
    CEmulatedRegistry();
    ~CEmulatedRegistry();

    COptions* pOptions;
    CRootKey* pRootKey;
    CHostKey* pKeyLocalHost;

    HANDLE hHeap;
    CLinkListSimple* pLinkListKeys; // link list of CKeyReplace*
    BOOL IsValidKey(CKeyReplace* pKey);
    BOOL RegisterKey(CKeyReplace* pKey);
    BOOL UnregisterKey(CKeyReplace* pKey);

    BOOL Load(TCHAR* FileName);
    BOOL Save(TCHAR* FileName);
    BOOL Save(TCHAR* FileName,BOOL bOnlyIfContentChanges);

    void SetSpyMode();
};
}