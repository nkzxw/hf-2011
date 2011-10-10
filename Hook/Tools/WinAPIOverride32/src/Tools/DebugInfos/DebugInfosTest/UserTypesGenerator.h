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
// Object: generates user types (struct, union, enum) files from pdb
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../DiaSDK/include/dia2.h"
#include "../DiaSDK/include/cvconst.h"
#include "../../String/AnsiUnicodeConvert.h"
#include "../../LinkList/LinkList.h"
#include "../../File/StdFileOperations.h"

class CTypesGeneratedManager
{
    typedef struct _TypesGeneratedInfos
    {
        WCHAR TypeName[MAX_PATH];
    }TYPES_GENERATED_INFOS;

    CLinkList* pList;
public:
    CTypesGeneratedManager()
    {
        this->pList=new CLinkList(sizeof(TYPES_GENERATED_INFOS));
    }
    ~CTypesGeneratedManager()
    {
        delete this->pList;
    }
    void Clear()
    {
        this->pList->RemoveAllItems();
    }
    BOOL IsGenerated(WCHAR* TypeName)
    {
        CLinkListItem* pItem;
        TYPES_GENERATED_INFOS* pInfos;
        BOOL bIsGenerated = FALSE;

        this->pList->Lock();
        for (pItem = this->pList->Head;pItem;pItem=pItem->NextItem)
        {
            pInfos = (TYPES_GENERATED_INFOS*)(pItem->ItemData);
            if(wcscmp(pInfos->TypeName,TypeName)==0)
            {
                bIsGenerated = TRUE;
                break;
            }
        }
        this->pList->Unlock();
        return bIsGenerated;
    }
    void Add(WCHAR* TypeName)
    {
        TYPES_GENERATED_INFOS Infos;
        wcsncpy(Infos.TypeName,TypeName,MAX_PATH);
        Infos.TypeName[MAX_PATH-1]=0;
        this->pList->AddItem(&Infos);
    }
};


class CUserTypesGenerator
{
protected:
    IDiaSymbol* pRootSymbol;
    DWORD RootSymbolSymTag;
    DWORD RootSymbolSymUdtKind;
    TCHAR* UserTypePath;
    CTypesGeneratedManager* pTypesGenerated;

    WCHAR CurrentTypeArrayDimensions[MAX_PATH];
    IDiaSymbol* PreviousSymbol;

    void PrintBound(FILE* hFile,IDiaSymbol* pSymbol);
    void PrintName(FILE* hFile , IDiaSymbol* pSymbol);
    void PrintUDT(FILE* hFile ,IDiaSymbol* pSymbol);
    void EndPreviousBitFieldIfRequired(FILE* hFile ,IDiaSymbol* pSymbol);
    void PrintBitField(FILE* hFile ,IDiaSymbol* pSymbol);
    void PrintSymbolType(FILE* hFile, IDiaSymbol* pSymbol);
    void PrintType(FILE* hFile ,IDiaSymbol* pSymbol);
    void PrintTypeInDetail(FILE* hFile,IDiaSymbol* pSymbol);
    void PrintDepth(FILE* hFile,SIZE_T Depth);
    static void ReplaceForbiddenFileChar(TCHAR* Str);
public:
    CUserTypesGenerator(IDiaSymbol* pSymbol,TCHAR* UserTypePath,CTypesGeneratedManager* pTypesGenerated);
    ~CUserTypesGenerator(void);
    BOOL Generate();
};
