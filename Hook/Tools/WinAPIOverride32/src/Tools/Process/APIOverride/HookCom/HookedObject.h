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
// Object: manage informations on a single COM object
//-----------------------------------------------------------------------------
#pragma once

#include "COM_include.h"
#include "../../../LinkList/LinkList.h"

class CHookedObject
{
protected:
    DWORD NbLocks;
    HANDLE hevtUnlocked;
public:
    typedef struct tagHookedInterfacePointer
    {
        IID Iid;
        IUnknown* pInterface;
    }OBJECT_INFO_INTERFACE_POINTER,*POBJECT_INFO_INTERFACE_POINTER;

    BOOL bCreatedByStaticFileLoading;
    CLinkList* pLinkListInterfacePointers; // link list of OBJECT_INFO_INTERFACE_POINTER
    IUnknown* GetInterfacePointer(IID* pIid);
    IUnknown* GetInterfacePointer(IID* pIid,BOOL UserManagesListLock);
    CLinkListItem* GetLinkListInterfacePointersItem(IID* pIid,BOOL UserManagesListLock);

    CHookedObject(IUnknown* pObject,IID* pIid);
    ~CHookedObject(void);

    IUnknown* pObject; // value of the pointer returned at object creation
    IUnknown* pIUnknown;// pointer to IUnknown interface of the created object
    IDispatch* GetpIDispatch(IUnknown* pObject);
    IUnknown* GetpIUnknown(IUnknown* pObject);
    BOOL SuccessfullyCreated();

    BOOL AddInterfacePointer(IUnknown* pObject,IID* pIid);
    BOOL DoesPointerBelongToObject(IUnknown* pObject);

    DWORD OwningThread;
    BOOL IsGoingToBeDestroyed;
    BOOL Lock(BOOL* pbIsGoingToBeDestroyed);
    BOOL Unlock();
};
