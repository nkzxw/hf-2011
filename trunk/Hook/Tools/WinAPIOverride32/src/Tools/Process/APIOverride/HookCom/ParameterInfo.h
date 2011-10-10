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
#pragma once

#include "../SupportedParameters.h"

#include "COM_include.h"

/*
// VARENUM usage key,
// 
// [V] - May appear in a VARIANT.
// [T] - May appear in a TYPEDESC.
// [P] - may appear in an OLE property set.
// [S] - May appear in a Safe Array.
// 
// 
VT_EMPTY            [V]   [P]         // Not specified.
VT_NULL             [V]               // SQL-style Null.
VT_I2               [V][T][P][S]      // 2-byte signed int.
VT_I4               [V][T][P][S]      // 4-byte-signed int.
VT_R4               [V][T][P][S]      // 4-byte real. 
VT_R8               [V][T][P][S]      // 8-byte real.
VT_CY               [V][T][P][S]      // Currency.
VT_DATE             [V][T][P][S]      // Date.
VT_BSTR             [V][T][P][S]      // Automation string.
VT_DISPATCH         [V][T]   [S]      // IDispatch.Far*
VT_ERROR            [V][T]   [S]      // Scodes.
VT_BOOL             [V][T][P][S]      // Boolean; True=-1, False=0.
VT_VARIANT          [V][T][P][S]      // VARIANT FAR*.
VT_DECIMAL          [V][T]   [S]      // 16 byte fixed point.
VT_RECORD           [V]   [P][S]      // User defined type
VT_UNKNOWN          [V][T]   [S]      // IUnknown FAR*.
VT_I1               [V][T]   [S]      // Char.
VT_UI1              [V][T]   [S]      // Unsigned char.
VT_UI2              [V][T]   [S]      // 2 byte unsigned int.
VT_UI4              [V][T]   [S]      // 4 byte unsigned int. 
VT_INT              [V][T]   [S]      // Signed machine int.
VT_UINT             [V][T]   [S]      // Unsigned machine int.
VT_VOID                [T]            // C-style void.
VT_HRESULT             [T]                                    
VT_PTR                 [T]            // Pointer type.
VT_SAFEARRAY           [T]            // Use VT_ARRAY in VARIANT.
VT_CARRAY              [T]            // C-style array.
VT_USERDEFINED         [T]            // User-defined type.
VT_LPSTR               [T][P]         // Null-terminated string.
VT_LPWSTR              [T][P]         // Wide null-terminated string.
VT_FILETIME               [P]         //FILETIME
VT_BLOB                   [P]         //Length prefixed bytes
VT_STREAM                 [P]         //Name of the stream follows
VT_STORAGE                [P]         //Name of the storage follows
VT_STREAMED_OBJECT        [P]         //Stream contains an object
VT_STORED_OBJECT          [P]         //Storage contains an object
VT_BLOB_OBJECT            [P]         //Blob contains an object
VT_CF                     [P]         //Clipboard format
VT_CLSID                  [P]         //A Class ID
VT_VECTOR                 [P]         //simple counted array
VT_ARRAY            [V]               // SAFEARRAY*.
VT_BYREF            [V]
VT_RESERVED


PARAMFLAG_NONE Whether the parameter passes or receives information is unspecified. IDispatch interfaces can use this flag. 
PARAMFLAG_FIN Parameter passes information from the caller to the callee. 
PARAMFLAG_FOUT Parameter returns information from the callee to the caller. 
PARAMFLAG_FLCID Parameter is the LCID of a client application.  
PARAMFLAG_FRETVAL Parameter is the return value of the member.  
PARAMFLAG_FOPT Parameter is optional. The pPARAMDescEx field contains a pointer to a VARIANT describing the default value for this parameter, if the PARAMFLAG_FOPT and PARAMFLAG_FHASDEFAULT bit of wParamFlags is set. 
PARAMFLAG_FHASDEFAULT Parameter has default behaviors defined. The pPARAMDescEx field contains a pointer to a VARIANT that describes the default value for this parameter, if the PARAMFLAG_FOPT and PARAMFLAG_FHASDEFAULT bit of wParamFlags is set.  

*/
class CParameterInfo
{
private:
    void FillInfos(ITypeInfo* pTypeInfo,TYPEDESC* pTypeDesc);
    HRESULT GetUserDefinedType(HREFTYPE hrefType, ITypeInfo* pTypeInfo,VARTYPE* pType);
    void FindWinAPIOverrideType();
    VARTYPE Type;
    VARTYPE PointedType;
    DWORD   NbPointedType;
    USHORT  ParamFlag;
    DWORD WinAPIOverrideType;
    DWORD StackSize;
    DWORD PointedSize;
public:
    CParameterInfo();
    ~CParameterInfo(void);

    HRESULT Parse(ITypeInfo* pTypeInfo,BSTR Name,ELEMDESC* pElemDesc);
    HRESULT MakeParameterFromReturn(ITypeInfo* pTypeInfo,ELEMDESC* pElemDesc);
    BOOL IsOutParameter();
    BOOL IsReturnedValue();

    DWORD GetWinAPIOverrideType();
    DWORD GetStackSize();
    DWORD GetPointedSize();
    USHORT GetParamFlag();
    BOOL  IsHRESULT();

    BSTR    Name;
};
