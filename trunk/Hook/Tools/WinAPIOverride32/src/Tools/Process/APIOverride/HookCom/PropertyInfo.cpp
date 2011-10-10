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
// Object: manages COM property info from IDispatch parsing
//-----------------------------------------------------------------------------

#include "propertyinfo.h"

/*
VARFLAGS
VARFLAG_FREADONLY Assignment to the variable should not be allowed. 
VARFLAG_FSOURCE The variable returns an object that is a source of events. 
VARFLAG_FBINDABLE The variable supports data binding. 
VARFLAG_FREQUESTEDIT When set, any attempt to directly change the property results in a call to IPropertyNotifySink::OnRequestEdit. The implementation of OnRequestEdit determines if the change is accepted. 
VARFLAG_FDISPLAYBIND The variable is displayed to the user as bindable. VARFLAG_FBINDABLE must also be set.  
VARFLAG_FDEFAULTBIND The variable is the single property that best represents the object. Only one variable in type information can have this attribute.  
VARFLAG_FHIDDEN The variable should not be displayed to the user in a browser, although it exists and is bindable. 
VARFLAG_FRESTRICTED The variable should not be accessible from macro languages. This flag is intended for system-level variables or variables that you do not want type browsers to display. 
VARFLAG_FDEFAULTCOLLELEM Permits an optimization in which the compiler looks for a member named "xyz" on the type of abc. If such a member is found and is flagged as an accessor function for an element of the default collection, then a call is generated to that member function. Permitted on members in dispinterfaces and interfaces; not permitted on modules. 
VARFLAG_FUIDEFAULT The variable is the default display in the user interface. 
VARFLAG_FNONBROWSABLE The variable appears in an object browser, but not in a properties browser. 
VARFLAG_FREPLACEABLE Tags the interface as having default behaviors. 
VARFLAG_FIMMEDIATEBIND  The variable is mapped as individual bindable properties. 

VARKIND:
VAR_PERINSTANCE The variable is a field or member of the type. It exists at a fixed offset within each instance of the type. 
VAR_STATIC There is only one instance of the variable. 
VAR_CONST The VARDESC describes a symbolic constant. There is no memory associated with it. 
VAR_DISPATCH The variable can only be accessed through IDispatch::Invoke. 
*/

CPropertyInfo::CPropertyInfo(void)
{
    memset(&this->VarDesc,0,sizeof(VARDESC));
    this->pParameterInfo=new CParameterInfo();
    this->Name=SysAllocString(CPROPERTYINFO_DEFAULT_PROPERTY_NAME);
}

CPropertyInfo::~CPropertyInfo(void)
{
    if (this->Name)
    {
        SysFreeString(this->Name);
        this->Name=NULL;
    }
    delete this->pParameterInfo;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse property
// Parameters :
//     in  : 
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
HRESULT CPropertyInfo::Parse(ITypeInfo* pTypeInfo,const VARDESC* pVarDesc)
{
    // copy VARDESC struct locally
    memcpy(&this->VarDesc,pVarDesc,sizeof(VARDESC));

    // free var name
    if (this->Name)
    {
        SysFreeString(this->Name);
        this->Name=NULL;
    }

    // retrieve var name
    UINT NbNames;
    HRESULT hResult;
    hResult=pTypeInfo->GetNames(pVarDesc->memid,&this->Name,1,&NbNames);
    if(FAILED(hResult))
        this->Name=SysAllocString(CPROPERTYINFO_DEFAULT_PROPERTY_NAME);

    hResult=this->pParameterInfo->Parse(pTypeInfo,this->Name,(ELEMDESC*)&pVarDesc->elemdescVar);

#ifdef _DEBUG
    if (this->Name)
    {
        OutputDebugString(_T("\r\nProperty:"));
        OutputDebugStringW(this->Name);
    }
#endif

    return hResult;
}

//-----------------------------------------------------------------------------
// Name: GetWinAPIOverrideType
// Object: translate COM type to a WinAPIOverride one defined in CSupportedParameters
// Parameters :
//     in  : 
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
DWORD CPropertyInfo::GetWinAPIOverrideType()
{
    return this->pParameterInfo->GetWinAPIOverrideType();
}
//-----------------------------------------------------------------------------
// Name: GetStackSize
// Object: get property required stack size
// Parameters :
//     in  : 
//     out : 
//     return : required stack size
//-----------------------------------------------------------------------------
DWORD CPropertyInfo::GetStackSize()
{
    return this->pParameterInfo->GetStackSize();
}
//-----------------------------------------------------------------------------
// Name: GetPointedSize
// Object: get property pointed size
// Parameters :
//     in  : 
//     out : 
//     return : pointed size
//-----------------------------------------------------------------------------
DWORD CPropertyInfo::GetPointedSize()
{
    return this->pParameterInfo->GetPointedSize();
}