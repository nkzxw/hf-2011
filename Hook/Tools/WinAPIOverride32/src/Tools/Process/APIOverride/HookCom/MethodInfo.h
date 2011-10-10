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
// Object: manages COM method info from IDispatch parsing
//-----------------------------------------------------------------------------

#pragma once
/*

INVOKEKIND:
INVOKE_FUNC The member is called using a normal function invocation syntax. 
INVOKE_PROPERTYGET The function is invoked using a normal property-access syntax. 
INVOKE_PROPERTYPUT The function is invoked using a property value assignment syntax. 
Syntactically, a typical programming language might represent changing 
a property in the same way as assignment. For example: object.property : = value. 
INVOKE_PROPERTYPUTREF The function is invoked using a property reference assignment syntax.  

FUNCKIND:
FUNC_PUREVIRTUAL The function is accessed through the virtual function table (VTBL), and takes an implicit this pointer. 
FUNC_VIRTUAL The function is accessed the same as PUREVIRTUAL, except the function has an implementation. 
FUNC_NONVIRTUAL The function is accessed by static address and takes an implicit this pointer. 
FUNC_STATIC The function is accessed by static address and does not take an implicit this pointer. 
FUNC_DISPATCH The function can be accessed only through IDispatch. 

FUNCFLAGS:
FUNCFLAG_FRESTRICTED The function should not be accessible from macro languages. This flag is intended for system-level functions or functions that type browsers should not display. 
FUNCFLAG_FSOURCE The function returns an object that is a source of events. 
FUNCFLAG_FBINDABLE The function that supports data binding. 
FUNCFLAG_FREQUESTEDIT When set, any call to a method that sets the property results first in a call to IPropertyNotifySink::OnRequestEdit. The implementation of OnRequestEdit determines if the call is allowed to set the property. 
FUNCFLAG_FDISPLAYBIND The function that is displayed to the user as bindable. FUNC_FBINDABLE must also be set.  
FUNCFLAG_FDEFAULTBIND The function that best represents the object. Only one function in a type information can have this attribute. 
FUNCFLAG_FHIDDEN The function should not be displayed to the user, although it exists and is bindable. 
FUNCFLAG_USESGETLASTERROR The function supports GetLastError. If an error occurs during the function, the caller can call GetLastError to retrieve the error code. 
FUNCFLAG_FDEFAULTCOLLELEM Permits an optimization in which the compiler looks for a member named "xyz" on the type of "abc". If such a member is found and is flagged as an accessor function for an element of the default collection, then a call is generated to that member function. Permitted on members in dispinterfaces and interfaces; not permitted on modules. For more information, refer to defaultcollelem in Type Libraries and the Object Description Language.  
FUNCFLAG_FUIDEFAULT The type information member is the default member for display in the user interface. 
FUNCFLAG_FNONBROWSABLE The property appears in an object browser, but not in a properties browser. 
FUNCFLAG_FREPLACEABLE Tags the interface as having default behaviors. 
FUNCFLAG_FIMMEDIATEBIND Mapped as individual bindable properties. 
*/

#include "COM_include.h" // for for #define _WIN32_WINNT
#include "DefinesAndStructs.h"
#include "ParameterInfo.h"
#include "../../../LinkList/LinkList.h"
#include "../../../LinkList/LinkListSimple.h"
#include "../../../String/AnsiUnicodeConvert.h"

#define CMETHODINFO_PARAM_DEFAULT_NAME L"Parameter"

class CMethodInfo
{
private:
    void FreeMemory();
    void FreeParametersMemory();
    BOOL ResultOfIDispatchParsing;
public:
    CMethodInfo();
    ~CMethodInfo();

    HRESULT Parse(IUnknown* pObject,IDispatch* pDispatch,ITypeInfo* pTypeInfo,const FUNCDESC* pFuncDesc);
    BOOL  MustThisPointerBeAddedAsFirstParameter();
    BOOL CanBeHookedByVTBL();
    BOOL  HasAnOutParameter();
    DWORD GetStackSize();
    void SetName(TCHAR* pszName);

    SHORT           VTBLIndex;
    PBYTE           VTBLAddress;// address of function redirection in the VTBL
    PBYTE           Address;
    BSTR            Name;
    CParameterInfo  Return;
    
    CLinkListSimple* pParameterInfoList;// linked list of CParameterInfo*
    
    
    MEMBERID memid;
    FUNCKIND funckind;
    INVOKEKIND invkind;
    CALLCONV callconv;
    void ToString(IN BOOL AddAdvancedInformation,OUT TCHAR* pszFuncDesc);

    CLinkListItem* pItemAPIInfo;// for hooking purpose only : pointer to hook information struct
    BOOL AskedToBeNotLogged;
    BOOL SetListOfBaseInterfaces(CLinkList* pLinkListOfBaseInterfacesID);
    CLinkList* pLinkListOfBaseInterfacesID; // sorted list of base interface ID owning this method
                                            // something like IID_IAgent,IID_IDispatch,IID_IUnknown
};
