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
// Object: allow to convert a string with format "str1;str2;str3" to an array of string
//         or a string with format "1;4;6-9;14-20;22;33" to an array of DWORD
//-----------------------------------------------------------------------------


#pragma once

#include "../LinkList/LinkListSimple.h"
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <stdio.h>
#define CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_STRING _T(";")
#define CMULTIPLEELEMENTSPARSING_MINOR_SPLITTER_STRING _T("-")
#define CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_CHAR ';'
#define CMULTIPLEELEMENTSPARSING_MINOR_SPLITTER_CHAR '-'


class CMultipleElementsParsing
{
private:
    static BOOL CMultipleElementsParsing::GetValue(TCHAR* psz,DWORD* pValue);
public:
    static DWORD* ParseDWORD(TCHAR* pszText,DWORD* pdwArraySize);
    static TCHAR** ParseString(TCHAR* pszText,DWORD* pdwArraySize);
    static void   ParseStringArrayFree(TCHAR** pArray,DWORD dwArraySize);
};
