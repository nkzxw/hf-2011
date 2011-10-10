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
// Object: user input parameter parsing
//-----------------------------------------------------------------------------
#pragma once

#include "../Tools/LinkList/LinkList.h"
#include "../Tools/String/TrimString.h"
#include "../Tools/String/StringConverter.h"
#include "../Tools/String/StrToHex.h"
#include "../Tools/String/AnsiUnicodeConvert.h"
#include "../Tools/process/apioverride/interprocesscommunication.h"


#define  CParseParametersForRemoteCall_DOUBLE             _T("Double=")
#define  CParseParametersForRemoteCall_FLOAT              _T("Float=")
#define  CParseParametersForRemoteCall_BUFFER             _T("Buffer=")
#define  CParseParametersForRemoteCall_REF_OUTVALUE       _T("OutValue")
#define  CParseParametersForRemoteCall_REF_OUTBUFFER      _T("OutBuffer")
#define  CParseParametersForRemoteCall_ANSI_PREFIX        _T("\"")

#define  CParseParametersForRemoteCall_UNICODE_PREFIX     _T("L\"")
#define  CParseParametersForRemoteCall_UNICODE_PREFIX_FIRSTCHAR 'L'


#define  CParseParametersForRemoteCall_VARIANT_BYREF      _T("_BYREF")
#define  CParseParametersForRemoteCall_VARIANT_PREFIX     _T("VT_")
#define  CParseParametersForRemoteCall_VARIANT_EMPTY      _T("EMPTY")
#define  CParseParametersForRemoteCall_VARIANT_NULL       _T("NULL")
#define  CParseParametersForRemoteCall_VARIANT_I1         _T("I1")
#define  CParseParametersForRemoteCall_VARIANT_I2         _T("I2")
#define  CParseParametersForRemoteCall_VARIANT_I4         _T("I4")
#define  CParseParametersForRemoteCall_VARIANT_I8         _T("I8")
#define  CParseParametersForRemoteCall_VARIANT_INT        _T("INT")
#define  CParseParametersForRemoteCall_VARIANT_UI1        _T("UI1")
#define  CParseParametersForRemoteCall_VARIANT_UI2        _T("UI2")
#define  CParseParametersForRemoteCall_VARIANT_UI4        _T("UI4")
#define  CParseParametersForRemoteCall_VARIANT_UI8        _T("UI8")
#define  CParseParametersForRemoteCall_VARIANT_UINT       _T("UINT")
#define  CParseParametersForRemoteCall_VARIANT_BOOL       _T("BOOL")
#define  CParseParametersForRemoteCall_VARIANT_ERROR      _T("ERROR")
#define  CParseParametersForRemoteCall_VARIANT_R4         _T("R4")
#define  CParseParametersForRemoteCall_VARIANT_R8         _T("R8")
#define  CParseParametersForRemoteCall_VARIANT_CY         _T("CY")
#define  CParseParametersForRemoteCall_VARIANT_DATE       _T("DATE ")
#define  CParseParametersForRemoteCall_VARIANT_DISPATCH   _T("DISPATCH")
#define  CParseParametersForRemoteCall_VARIANT_UNKNOWN    _T("UNKNOWN")
#define  CParseParametersForRemoteCall_VARIANT_BSTR       _T("BSTR")

class CParseParametersForRemoteCall
{
private:
    CLinkList* pLinkListParams;
    void EmptyParamList();
    BOOL IsVARIANT_BYREF(IN OUT TCHAR* psz);
public:
    CParseParametersForRemoteCall(void);
    ~CParseParametersForRemoteCall(void);
    BOOL Parse(IN TCHAR* pszParams,
                OUT STRUCT_FUNC_PARAM** ppParams,
                OUT DWORD* NbParams,
                BOOL* pbAreSomeParameterPassedAsRef,
                OUT TCHAR* ErrorMessage,
                size_t ErrorMessageMaxSize);
    TCHAR* RemoveBackslash(TCHAR* psz);
};
