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

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "TrimString.h"

#ifdef  _WIN64
    #define HEX_VALUE_DISPLAY _T("%I64X")
    #define HEX_VALUE_DISPLAY_FIXED_SIZE _T("%.16I64X")
    #define UNSIGNED_VALUE_DISPLAY _T("%I64u")
    #define SIGNED_VALUE_DISPLAY _T("%I64d")
#else
    #define HEX_VALUE_DISPLAY _T("%I32X")
    #define HEX_VALUE_DISPLAY_FIXED_SIZE _T("%.8I32X")
    #define UNSIGNED_VALUE_DISPLAY _T("%I32u")
    #define SIGNED_VALUE_DISPLAY _T("%I32d")
#endif

class CStringConverter
{
public:
    static BOOL StringToDWORD(TCHAR* strValue,DWORD* pValue);
    static BOOL StringToPBYTE(TCHAR* strValue,PBYTE* pValue);
    static BOOL StringToSIZE_T(TCHAR* strValue,SIZE_T* pValue);
    static BOOL StringToSignedInt(TCHAR* strValue,INT* pValue);
    static BOOL StringToSignedInt64(TCHAR* strValue,INT64* pValue);
    static BOOL StringToUnsignedInt(TCHAR* strValue,UINT* pValue);
    static BOOL StringToUnsignedInt64(TCHAR* strValue,UINT64* pValue);
    static BOOL StringToDouble(TCHAR* strValue,double* pValue);
    static BOOL StringToFloat(TCHAR* strValue,float* pValue);
    //-----------------------------------------------------------------------------
    // Name: ValueToBinaryString
    // Object: print string binary representation into strBinValue
    // Parameters :
    //     in  : T Value
    //     out : TCHAR* strBinValue : must have a length at least of (sizeof(T)*8+1)
    //     return : TRUE on success
    //-----------------------------------------------------------------------------
    template <class T> static BOOL ValueToBinaryString(T const Value,OUT TCHAR* strBinValue)
    {
        T LocalValue;
        TCHAR* pc;
        LocalValue = Value;
        strBinValue[sizeof(T)*8] = 0;
        for (pc = &strBinValue[sizeof(T)*8-1] ; pc>=strBinValue; pc--)
        {
            (LocalValue&1) ? *pc = '1' : *pc = '0';
            LocalValue = LocalValue>>1;
        }

        return TRUE;
    }
};