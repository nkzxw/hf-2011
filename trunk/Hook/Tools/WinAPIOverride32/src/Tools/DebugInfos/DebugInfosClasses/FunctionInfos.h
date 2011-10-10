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
// Object: manages debug informations for functions
//-----------------------------------------------------------------------------
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include "typeinfos.h"
#include "functionblockinfos.h"
#include "LabelInfos.h"
#include "../../LinkList/LinkListSimple.h"

class CFunctionInfos
{
private:
    BOOL Parse(IDiaSymbol *pSymbol,CFunctionBlockInfos* pBlockInfo);
public:
    CLinkListSimple* pLinkListParams;// function parameters
    CLinkListSimple* pLinkListBlocks;// {} in c++ contains local data
    CLinkListSimple* pLinkListLabels;// function local jumps

    TCHAR* Name;
    TCHAR* UndecoratedName;
    ULONGLONG RelativeVirtualAddress;
    DWORD SectionIndex;
    DWORD Offset;
    ULONGLONG Length;
    DWORD CallingConvention;
    CTypeInfos* pReturnInfos;
    DWORD Token;

    BOOL GetCallingConvention(TCHAR* StrCallingConvention,SIZE_T StrCallingConventionMaxSize);
    BOOL Parse(IDiaSymbol *pSymbol);
    CFunctionInfos(void);
    ~CFunctionInfos(void);
};
