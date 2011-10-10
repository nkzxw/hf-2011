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
// Object: convert string to single type : byte, DWORD, INT, ...
//-----------------------------------------------------------------------------

#include "stringconverter.h"

//-----------------------------------------------------------------------------
// Name: StringToDWORD
// Object: convert string to DWORD
// Parameters :
//     in  : TCHAR* strValue
//     out : DWORD* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToDWORD(TCHAR* strValue,DWORD* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%x"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToPBYTE
// Object: convert string to PBYTE
// Parameters :
//     in  : TCHAR* strValue
//     out : PBYTE* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToPBYTE(TCHAR* strValue,PBYTE* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%p"),pValue);
    else
        iScanfRes=_stscanf(strValue,UNSIGNED_VALUE_DISPLAY,pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToSIZE_T
// Object: convert string to SIZE_T
// Parameters :
//     in  : TCHAR* strValue
//     out : SIZE_T* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSIZE_T(TCHAR* strValue,SIZE_T* pValue)
{
    return CStringConverter::StringToPBYTE(strValue,(PBYTE*)pValue);
}

//-----------------------------------------------------------------------------
// Name: StringToSignedInt
// Object: convert string to INT
// Parameters :
//     in  : TCHAR* strValue
//     out : int* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedInt(TCHAR* strValue,INT* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I32X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I32d"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToSignedInt64
// Object: convert string to INT64
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedInt64(TCHAR* strValue,INT64* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I64X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I64d"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToUnsignedInt
// Object: convert string to UINT
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToUnsignedInt(TCHAR* strValue,UINT* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I32X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I32u"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToUnsignedInt64
// Object: convert string to UINT64
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToUnsignedInt64(TCHAR* strValue,UINT64* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I64X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I64u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToDouble
// Object: convert string to double
// Parameters :
//     in  : TCHAR* strValue
//     out : double* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToDouble(TCHAR* strValue,double* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    return (_stscanf(strValue,_T("%g"),pValue)==1);
}

//-----------------------------------------------------------------------------
// Name: StringToFloat
// Object: convert string to float
// Parameters :
//     in  : TCHAR* strValue
//     out : float* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToFloat(TCHAR* strValue,float* pValue)
{
    *pValue=0.0;
    double dValue=0.0;
    if (!CStringConverter::StringToDouble(strValue,&dValue))
        return FALSE;
    
    *pValue=(float)dValue;
    return TRUE;
}