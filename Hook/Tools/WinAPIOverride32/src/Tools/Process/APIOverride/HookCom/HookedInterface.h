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
// Object: manage informations on a hooked interface
//-----------------------------------------------------------------------------
#pragma once

#include "COM_include.h"
#include "InterfaceInfo.h"

class CHookedInterface:public CInterfaceInfo
{
public:
    IUnknown* pInterfaceAssociatedToIID;// interface pointer associated to iid
    BOOL  bIUnknownDerivedInterfaceHooked;// TRUE if Release and QueryInterface method are hooked
    BOOL  bHookedByAutoMonitoring;// TRUE if hooked by auto monitoring
    CLinkListItem* pReleaseHookInfo;// pointer to CLinkListItem containing API_INFO, so hook informations
    CLinkListItem* pQueryInterfaceHookInfo;// pointer to CLinkListItem containing API_INFO, so hook informations
    CHookedInterface(IUnknown* pInterfaceAssociatedToIID,IID* pIid);
    ~CHookedInterface(void);
};
