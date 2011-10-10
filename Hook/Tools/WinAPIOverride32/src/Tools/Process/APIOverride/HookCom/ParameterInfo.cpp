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
// Object: manages COM parameter info from IDispatch parsing
//-----------------------------------------------------------------------------
#include "parameterinfo.h"

CParameterInfo::CParameterInfo()
{
    this->ParamFlag=NULL;
    this->Name=NULL;
    this->Type=VT_I4;
    this->PointedType=VT_EMPTY;
    this->NbPointedType=0;
    this->WinAPIOverrideType=PARAM_UNKNOWN;
}
CParameterInfo::~CParameterInfo(void)
{
    if (this->Name)
    {
        free(this->Name);
        this->Name=NULL;
    }
}
//-----------------------------------------------------------------------------
// Name: GetParamFlag
// Object: get parameter Flag
// Parameters :
//     in  : 
//     out : 
//     return : parameter flag
//-----------------------------------------------------------------------------
USHORT CParameterInfo::GetParamFlag()
{
    return this->ParamFlag;
}

//-----------------------------------------------------------------------------
// Name: IsHRESULT
// Object: check if type is an HRESULT
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if type is an HRESULT
//-----------------------------------------------------------------------------
BOOL CParameterInfo::IsHRESULT()
{
    if (this->NbPointedType)
        return FALSE;

    return (this->Type==VT_HRESULT);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse current parameter
// Parameters :
//     in  : ITypeInfo* pTypeInfo
//           BSTR Name
//           ELEMDESC* pElemDesc
//     out : 
//     return : success result
//-----------------------------------------------------------------------------
HRESULT CParameterInfo::Parse(ITypeInfo* pTypeInfo,BSTR Name,ELEMDESC* pElemDesc)
{
    this->ParamFlag=pElemDesc->paramdesc.wParamFlags;
    if (Name)
        this->Name=wcsdup(Name);

    this->FillInfos(pTypeInfo,&pElemDesc->tdesc);
   
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: translate return type to a parameter
// Parameters :
//     in  : ITypeInfo* pTypeInfo
//           ELEMDESC* pElemDesc
//     out : 
//     return : success result
//-----------------------------------------------------------------------------
HRESULT CParameterInfo::MakeParameterFromReturn(ITypeInfo* pTypeInfo,ELEMDESC* pElemDesc)
{
    HRESULT hResult;
    // parse parameter object
    hResult=this->Parse(pTypeInfo,L"RetValue",pElemDesc);
    if (FAILED(hResult))
        return hResult;

    this->PointedType=this->Type;
    this->Type=VT_EMPTY;
    this->NbPointedType=1;
    this->ParamFlag=PARAMFLAG_FOUT|PARAMFLAG_FRETVAL;

    this->FindWinAPIOverrideType();

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FillInfos
// Object: fill class information (used by parse)
// Parameters :
//     in  : ITypeInfo* pTypeInfo
//           TYPEDESC* pTypeDesc
//     out : 
//     return : success result
//-----------------------------------------------------------------------------
void CParameterInfo::FillInfos(ITypeInfo* pTypeInfo,TYPEDESC* pTypeDesc)
{
    this->NbPointedType=0;
    this->Type=pTypeDesc->vt;

    switch(this->Type)
    {
    case VT_PTR:
        // if type is a pointer
        // there's only one pointed element
        this->NbPointedType=1;
        // get pointed type
        this->PointedType=pTypeDesc->lptdesc->vt;

        if (this->PointedType==VT_USERDEFINED)
            this->GetUserDefinedType(pTypeDesc->lptdesc->hreftype,pTypeInfo,&this->PointedType);
        break;
    // if type is an array
    case VT_CARRAY:
        // get pointed type
        this->PointedType=pTypeDesc->lpadesc->tdescElem.vt;
        // get number of elements in array
        this->NbPointedType=0;
        for (DWORD cnt=0;cnt<pTypeDesc->lpadesc->cDims;cnt++)
        {
            this->NbPointedType+=pTypeDesc->lpadesc->rgbounds[cnt].cElements;
        }
        if (this->PointedType==VT_USERDEFINED)
            this->GetUserDefinedType(pTypeDesc->lptdesc->hreftype,pTypeInfo,&this->PointedType);
        break;
    case VT_USERDEFINED:
        this->GetUserDefinedType(pTypeDesc->hreftype,pTypeInfo,&this->Type);
    }
    this->FindWinAPIOverrideType();
}

//-----------------------------------------------------------------------------
// Name: GetUserDefinedType
// Object: try to get standard type from user defined type
// Parameters :
//     in  : HREFTYPE hrefType
//           ITypeInfo* pTypeInfo
//           VARTYPE* pType
//     out : 
//     return : success result
//-----------------------------------------------------------------------------
HRESULT CParameterInfo::GetUserDefinedType(HREFTYPE hrefType, ITypeInfo* pTypeInfo,VARTYPE* pType)
{
    if (IsBadReadPtr(pTypeInfo,sizeof(ITypeInfo)))
        return E_FAIL;

    ITypeInfo* pUserDefinedTypeInfo;
    HRESULT hr = pTypeInfo->GetRefTypeInfo(hrefType, &pUserDefinedTypeInfo);
    if (SUCCEEDED(hr) && pUserDefinedTypeInfo)
    {
        TYPEATTR* pTypeAttr;
        hr = pUserDefinedTypeInfo->GetTypeAttr( &pTypeAttr );
        if( FAILED( hr ) )
            return hr;

        hr=S_OK;

        switch(pTypeAttr->typekind)
        {
        case TKIND_ENUM:
            *pType = VT_I4;
            break;
        case TKIND_RECORD:
            *pType = VT_RECORD;
            break;
        case TKIND_MODULE:
            break;
        case TKIND_INTERFACE:
            *pType = VT_UNKNOWN;
            break;
        case TKIND_DISPATCH:
            *pType = VT_DISPATCH;
            break;
        case TKIND_COCLASS:
            *pType = VT_DISPATCH;
            break;
        case TKIND_ALIAS:
            switch (pTypeAttr->tdescAlias.vt)
            {
            default:
                break;
            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            case VT_I1:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_I8:
            case VT_UI8:
            case VT_INT:
            case VT_UINT:
                *pType = pTypeAttr->tdescAlias.vt;
                // we should have pTypeAttr->tdescAlias.hreftype == 0
                break;
            case VT_USERDEFINED:
                hr=this->GetUserDefinedType(pTypeAttr->tdescAlias.hreftype, pUserDefinedTypeInfo,pType);
            }
            break;
        case TKIND_UNION:
            break;
        case TKIND_MAX:
            break;
        default:
            break;
        }

        CSecureIUnknown::Release(pUserDefinedTypeInfo);
        return hr;
    }
    return E_FAIL;
}

//-----------------------------------------------------------------------------
// Name: IsReturnedValue
// Object: check if parameter is an output parameter
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if parameter is an output parameter
//-----------------------------------------------------------------------------
BOOL CParameterInfo::IsOutParameter()
{
    if ((this->ParamFlag&PARAMFLAG_FOUT)
        ||(this->ParamFlag==PARAMFLAG_NONE))
        return TRUE;

    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: IsReturnedValue
// Object: check if parameter is a return value
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if value is a return value
//-----------------------------------------------------------------------------
BOOL CParameterInfo::IsReturnedValue()
{
    if (this->ParamFlag&PARAMFLAG_FRETVAL)
        return TRUE;

    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: GetStackSize
// Object: get parameter required stack size
// Parameters :
//     in  : 
//     out : 
//     return : required stack size
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetStackSize()
{
    return this->StackSize;
}
//-----------------------------------------------------------------------------
// Name: GetPointedSize
// Object: get parameter pointed size
// Parameters :
//     in  : 
//     out : 
//     return : pointed size
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetPointedSize()
{
    return this->PointedSize;
}
//-----------------------------------------------------------------------------
// Name: GetWinAPIOverrideType
// Object: get WinAPIOverride type
// Parameters :
//     in  : 
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
DWORD CParameterInfo::GetWinAPIOverrideType()
{
    return this->WinAPIOverrideType;
}

//-----------------------------------------------------------------------------
// Name: GetWinAPIOverrideType
// Object: translate COM type to a WinAPIOverride one defined in CSupportedParameters
// Parameters :
//     in  : 
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
void CParameterInfo::FindWinAPIOverrideType()
{
    // as we are retrieving data from a type we just interest to [T] var (see VARENUM definition)

    // if standard values
    if (this->NbPointedType==0)
    {
        switch(this->Type)
        {
            case VT_CY:             // Currency.
                this->WinAPIOverrideType=PARAM_INT64;
                break;
            case VT_DATE:           // Date.
                this->WinAPIOverrideType=PARAM_DOUBLE;
                break;            
            case VT_ERROR:          // Scodes.
                this->WinAPIOverrideType=PARAM_LONG;
                break;
            case VT_BOOL:           // Boolean; True=-1, False=0.
                this->WinAPIOverrideType=PARAM_SHORT;
                break;
            case VT_DECIMAL:        // 16 byte fixed point.
                this->WinAPIOverrideType=PARAM_DECIMAL;
                break;
            case VT_RECORD:           // User defined type
                this->WinAPIOverrideType=PARAM_PVOID;
                break;
            case VT_R4:              // 4-byte real. 
                this->WinAPIOverrideType=PARAM_FLOAT;
                break;
            case VT_R8:              // 8-byte real.
                this->WinAPIOverrideType=PARAM_DOUBLE;
                break;
            case VT_I1:              // Char.
                this->WinAPIOverrideType=PARAM_CHAR;
                break;
            case VT_I2:              // 2-byte signed int.
                this->WinAPIOverrideType=PARAM_SHORT;
                break;
            case VT_I4:              // 4-byte-signed int.
            case VT_INT:             // Signed machine int.
                this->WinAPIOverrideType=PARAM_INT;
                break;
            case VT_UI1:              // Unsigned char.
                this->WinAPIOverrideType=PARAM_UCHAR;
                break;
            case VT_UI2:              // 2 byte unsigned int.
                this->WinAPIOverrideType=PARAM_WORD;
                break;
            case VT_UI4:              // 4 byte unsigned int. 
            case VT_UINT:            // Unsigned machine int.
                this->WinAPIOverrideType=PARAM_UINT;
                break;
            case VT_I8:
            case VT_UI8:
                this->WinAPIOverrideType=PARAM_INT64;
                break;
            case VT_LPSTR:           // Null-terminated string.
                this->WinAPIOverrideType=PARAM_PSTR;
                break;
            case VT_LPWSTR:           // Wide null-terminated string.
            case VT_BSTR:              // Automation string.
                this->WinAPIOverrideType=PARAM_BSTR;
                break;
            case VT_VARIANT:           // VARIANT FAR*.
                this->WinAPIOverrideType=PARAM_VARIANT;
                break;
            case VT_HRESULT:                                   
                this->WinAPIOverrideType=PARAM_LONG;
                break;
            case VT_PTR:              // Pointer type.
                this->WinAPIOverrideType=PARAM_PVOID;
                break;
            case VT_VOID:            // C-style void.
            case VT_UNKNOWN:         // IUnknown FAR*.
            case VT_DISPATCH:        // IDispatch.Far*
                this->WinAPIOverrideType=PARAM_POINTER;
                break;
            case VT_SAFEARRAY:        // Use VT_ARRAY in VARIANT.
                this->WinAPIOverrideType=PARAM_SAFEARRAY;
                break;
            case VT_CARRAY:           // C-style array.
                this->WinAPIOverrideType=PARAM_PBYTE;
                break;
            case VT_USERDEFINED:     // User-defined type. // should not appear
            default:
                this->WinAPIOverrideType=PARAM_UNKNOWN;
                break;
        }
        this->StackSize=CSupportedParameters::GetParamStackSize(this->WinAPIOverrideType);
        this->PointedSize=CSupportedParameters::GetParamPointedSize(this->WinAPIOverrideType);
    }
    // if pointed values
    else
    {
        switch(this->PointedType)
        {
        case VT_CY:             // Currency.
            this->WinAPIOverrideType=PARAM_PINT64;
            break;
        case VT_DATE:           // Date.
            this->WinAPIOverrideType=PARAM_PDOUBLE;
            break;            
        case VT_ERROR:          // Scodes.
            this->WinAPIOverrideType=PARAM_PLONG;
            break;
        case VT_BOOL:           // Boolean; True=-1, False=0.
            this->WinAPIOverrideType=PARAM_PSHORT;
            break;
        case VT_DECIMAL:        // 16 byte fixed point.
            this->WinAPIOverrideType=PARAM_PDECIMAL;
            break;
        case VT_RECORD:           // User defined type
            this->WinAPIOverrideType=PARAM_PPOINTER;
            break;
        case VT_R4:              // 4-byte real. 
            this->WinAPIOverrideType=PARAM_PFLOAT;
            break;
        case VT_R8:              // 8-byte real.
            this->WinAPIOverrideType=PARAM_PDOUBLE;
            break;
        case VT_I1:              // Char.
            this->WinAPIOverrideType=PARAM_PBYTE;
            break;
        case VT_I2:              // 2-byte signed int.
            this->WinAPIOverrideType=PARAM_PSHORT;
            break;
        case VT_I4:              // 4-byte-signed int.
        case VT_INT:             // Signed machine int.
            this->WinAPIOverrideType=PARAM_PINT;
            break;
        case VT_UI1:              // Unsigned char.
            this->WinAPIOverrideType=PARAM_PUCHAR;
            break;
        case VT_UI2:              // 2 byte unsigned int.
            this->WinAPIOverrideType=PARAM_PSHORT;
            break;
        case VT_UI4:              // 4 byte unsigned int. 
        case VT_UINT:            // Unsigned machine int.
            this->WinAPIOverrideType=PARAM_PUINT;
            break;
        case VT_I8:
        case VT_UI8:
            this->WinAPIOverrideType=PARAM_PINT64;
            break;
        case VT_LPSTR:           // Null-terminated string.
            this->WinAPIOverrideType=PARAM_PPOINTER;
            break;
        case VT_LPWSTR:           // Wide null-terminated string.
        case VT_BSTR:             // Automation string.
            this->WinAPIOverrideType=PARAM_PPOINTER;
            break;
        case VT_VARIANT:           // VARIANT FAR*.
            this->WinAPIOverrideType=PARAM_PVARIANT;
            break;
        case VT_HRESULT:                                   
            this->WinAPIOverrideType=PARAM_PLONG;
            break;
        case VT_PTR:              // Pointer type.
            this->WinAPIOverrideType=PARAM_PPOINTER;
            break;
        case VT_VOID:              // C-style void.
        case VT_UNKNOWN:           // IUnknown FAR*.
        case VT_DISPATCH:          // IDispatch.Far*
            this->WinAPIOverrideType=PARAM_PPOINTER;
            break;
        case VT_SAFEARRAY:        // Use VT_ARRAY in VARIANT.
            this->WinAPIOverrideType=PARAM_PSAFEARRAY;
            break;
        case VT_CARRAY:           // C-style array.
            this->WinAPIOverrideType=PARAM_PPVOID;
            break;
        case VT_USERDEFINED:     // User-defined type. // should not appear
        default:
            this->WinAPIOverrideType=PARAM_POINTER;
            break;
        }
        this->StackSize=CSupportedParameters::GetParamStackSize(this->WinAPIOverrideType);
        this->PointedSize=this->NbPointedType*
            CSupportedParameters::GetParamPointedSize(this->WinAPIOverrideType);
    }
}