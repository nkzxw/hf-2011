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
// Object: manages informations of pdb files
//-----------------------------------------------------------------------------
#pragma once
#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "callback.h"
#include "moduleinfos.h"

#include "../../LinkList/LinkListTemplate.h"
#include "../../LinkList/LinkList.h"
#include "../../COM/registercomcomponent.h"
#include "../../File/StdFileOperations.h"
#include "../../File/TextFile.h"
#include "../../String/ansiunicodeconvert.h"

#define REMOVE_LINKER_COMPILAND 1 // 0 if you want to keep linker infos
#define LINKER_COMPILAND_NAME L"* Linker *"
/////////////////////////////////////////////////////////////////////////////////////
// /!\ update associated CLSID_DiaSource in DebugInfo.cpp on DIA_DLL_NAME value change
/////////////////////////////////////////////////////////////////////////////////////
#define DIA_DLL_NAME _T("msdia80.dll") 
//#define DIA_DLL_NAME _T("msdia100.dll")

class CDebugInfosSourcesInfos
{
public:
    DWORD LineNumber;
    DWORD Offset;
    DWORD SectionIndex;
    ULONGLONG RelativeVirtualAddress;
    TCHAR FileName[MAX_PATH];
    TCHAR* LineContent;

    CDebugInfosSourcesInfos()
    {
        this->LineNumber=0;
        this->Offset=0;
        this->SectionIndex=0;
        *this->FileName=0;
        this->LineContent=NULL;
        this->RelativeVirtualAddress=0;
    }
    ~CDebugInfosSourcesInfos()
    {
        if (this->LineContent)
            free(this->LineContent);
    }
};

class CDebugInfos
{
private:
    TCHAR DiaDllPath[MAX_PATH];
    IDiaDataSource* pDiaDataSource;
    IDiaSession* pDiaSession;
    BOOL bComInitialized;
    BOOL bDisplayErrorMessages;
    DWORD MachineType;
    TCHAR FileName[MAX_PATH];
    void CommonConstructor(TCHAR* FileName);
    void FreeMemory();
    BOOL LoadDataFromPdb();
    void ReportError(TCHAR* Msg,HRESULT hr);
    BOOL ParseModules();
    typedef struct tagSourceFileNeededLines
    {
        TCHAR* FileName;
        CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos;
    }SOURCE_FILE_NEEDED_LINES,*PSOURCE_FILE_NEEDED_LINES;
    BOOL GetFunctionLines( IDiaSymbol* pSymbol, CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos);
    static BOOL CallBackSourceParseLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
public:
    IDiaSymbol* pDiaGlobalSymbol;
    CLinkListSimple* pLinkListModules;
    CDebugInfos(TCHAR* FileName);
    CDebugInfos(TCHAR* FileName,BOOL DisplayErrorMessages);
    ~CDebugInfos(void);
    BOOL HasDebugInfos();
    BOOL Parse();
    BOOL FindFunctionByRVA(ULONGLONG RelativeVirtualAddress,CFunctionInfos** ppFunctionInfo);
    BOOL FindLinesByRVA(ULONGLONG RelativeVirtualAddress,CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos);
    const TCHAR* GetFileName();
};
