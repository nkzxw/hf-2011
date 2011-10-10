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
#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <shlobj.h>

#include "../Tools/Pe/PE.h"
#include "../Tools/Process/Memory/ProcessMemory.h"
#include "../Tools/Process/ModulesParser/ModulesParser.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../Tools/Dll/DllFindFuncNameFromRVA.h"
#include "../Tools/LinkList/SingleThreaded/LinkListTemplateSingleThreaded.h"
#include "../Tools/Dll/DllStub.h"

#define PE_SECTION_NAME_SIZE 8
#define SECTION_NAME_STRING_SIZE PE_SECTION_NAME_SIZE+1
class CModuleIntegrityCheckingChanges;
class CModuleIntegrityCheckingSectionChanges;

class CModuleIntegrityChecking
{
private:
    PBYTE DllBaseAddress;
    DWORD DllSizeInMemory;
    TCHAR DllPath[MAX_PATH];
    TCHAR DllName[MAX_PATH];
    BOOL  DllNameContainsFullPath;
    DWORD ProcessId;

    PBYTE DllLocalCopy;
    PBYTE OriginalFile;
    HANDLE hFileOriginalFile;

    CDllStub DllStub;

    CPE* FindOrCreatePe(TCHAR* FileName);
    BOOL UpdateOriginalIATFromRebasedDllAddresses(PBYTE pMemoryPEStartAddress ,CPE* pPE);
    static BOOL __fastcall IsRebasedAddress(ULONGLONG OriginalBaseAddress, ULONGLONG RebasedAddress, PBYTE MayOriginalPointer, PBYTE MayRebasedPointer, BOOL b64BitProcess, OUT PBYTE* pRebasedAddressBegin, OUT PBYTE* pRebasedAddressEnd);
    BOOL CheckIntegrity(BOOL CheckOnlyCodeAndExecutableSections,BOOL DontCheckWritableSections,BOOL ShowRebasing,DWORD MinMatchingSizeAfterReplacement);
    BOOL CloseFileModule();
    BOOL OpenFileModule();
    BOOL GetDllInformations();
    BOOL MakeLocalCopyOfDll();
    static BOOL ModuleFoundCallbackStatic(MODULEENTRY* pModuleEntry,PVOID UserParam);
    BOOL ModuleFoundCallback(MODULEENTRY* pModuleEntry);
    void ClearChangesLinkList();
    CLinkListTemplateSingleThreaded<MODULEENTRY> ProcessModuleList;
    CLinkListTemplateSingleThreaded<CPE> ParsedPeList;

public:
    enum tagChangesType
    {
        ChangesType_Undefined,
        ChangesType_None,
        ChangesType_Header,
        ChangesType_SectionNotInBinary,
        ChangesType_SectionNotInMemory,
        ChangesType_SectionBinaryContainsMoreData,
        ChangesType_SectionMemoryContainsMoreData,
        ChangesType_ByteChangesInSection
    };

    CModuleIntegrityChecking(void);
    ~CModuleIntegrityChecking(void);
    MODULEENTRY* GetModuleEntry(TCHAR* LibraryName);
    ULONGLONG GetRebasedAddressOfFunction(TCHAR* DllName, TCHAR* FunctionName);
    ULONGLONG GetRebasedAddressOfFunction(TCHAR* DllName,WORD FunctionHintOrOrdinal, BOOL Ordinal);
    ULONGLONG GetRebasedAddressOfFunction(MODULEENTRY* pDllEntry,CPE* pDllPE,WORD FunctionHintOrOrdinal, BOOL Ordinal);
    ULONGLONG GetRebasedAddressOfFunction(MODULEENTRY* pDllEntry, CPE* pDllPE, CPE::EXPORT_FUNCTION_ITEM* pExportedFunctionInfos);
    BOOL CheckIntegrity(DWORD ProcessId,TCHAR* DllName,BOOL CheckOnlyCodeAndExecutableSections,BOOL DontCheckWritableSections,BOOL ShowRebasing,DWORD MinMatchingSizeAfterReplacement);
    BOOL RestoreFullModuleIntegrity();
    BOOL RestoreSectionIntegrity(CModuleIntegrityCheckingSectionChanges* pSectionChange);
    BOOL RestoreIntegrity(CModuleIntegrityCheckingChanges* pChange);
    PBYTE GetModuleBaseAddress();
    TCHAR* GetModulePath();
    DWORD  GetProcessId();
    CLinkListSimple* pSectionsChangesLinkList;// link list of changes; link list of CModuleIntegrityCheckingSectionChanges* item
};


class CModuleIntegrityCheckingChanges
{
public:
    CModuleIntegrityChecking::tagChangesType ChangeType;
    PBYTE  FileOffset;
    PBYTE  MemoryOffset;
    PBYTE  FileBuffer;
    PBYTE  MemoryBuffer;
    SIZE_T BufferLen;
    TCHAR  AssociatedFunctionName[MAX_PATH];
    PBYTE  RelativeAddressFromFunctionStart;

    CModuleIntegrityCheckingChanges(void)
    {
        this->ChangeType=CModuleIntegrityChecking::ChangesType_Undefined;
        this->FileOffset=0;
        this->MemoryOffset=0;
        this->FileBuffer=0;
        this->MemoryBuffer=0;
        this->BufferLen=0; 
        this->RelativeAddressFromFunctionStart=0;
        *this->AssociatedFunctionName=0;
    };
    ~CModuleIntegrityCheckingChanges(void){};

friend class CModuleIntegrityCheckingSectionChanges;
friend class CModuleIntegrityChecking; // + private RestoreIntegrity : avoid user to use directly RestoreIntegrity function
                                       // force the use of CModuleIntegrityChecking::RestoreIntegrity
private:
    BOOL RestoreIntegrity(DWORD ProcessID,PBYTE DllBaseAddress)
    {
        switch (this->ChangeType)
        {
        case CModuleIntegrityChecking::ChangesType_None: 
            // no changes -> nothing to do
            return TRUE;

        case CModuleIntegrityChecking::ChangesType_ByteChangesInSection:
            {
            // some bytes have been changed : restore them
            CProcessMemory pm(ProcessID,FALSE,TRUE);
            SIZE_T NbWrittenBytes;
            return pm.Write(DllBaseAddress+(SIZE_T)this->MemoryOffset,this->FileBuffer,this->BufferLen,&NbWrittenBytes);
            }

        case CModuleIntegrityChecking::ChangesType_Header: // avoid to change header because it contains necessary information, can contain more or less section
        case CModuleIntegrityChecking::ChangesType_SectionNotInBinary: // don't remove memory new section else process may will crash
        case CModuleIntegrityChecking::ChangesType_SectionNotInMemory: // exe don't need section to run --> no need to add the section in memory
        case CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData:// exe don't need data to run --> no need to add the data in memory
        case CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData:// don't remove memory else process may will crash
        case CModuleIntegrityChecking::ChangesType_Undefined: // can do nothing
        default:
            MessageBox(NULL,_T("Only opcode replacement can be restored"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return FALSE;
        }
    }

};

class CModuleIntegrityCheckingSectionChanges
{
public:
    TCHAR SectionName[SECTION_NAME_STRING_SIZE];
    PBYTE SectionStartAddress;
    SIZE_T SectionSize;
    CLinkListSimple* pChangesLinkList;


    CModuleIntegrityCheckingSectionChanges(void)
    {
        this->SectionStartAddress=0;
        this->SectionSize=0;
        *this->SectionName=0;
        this->pChangesLinkList=new CLinkListSimple();
    }
    ~CModuleIntegrityCheckingSectionChanges(void)
    {
        delete this->pChangesLinkList;
    }

    BOOL RestoreIntegrity(DWORD ProcessId,PBYTE DllBaseAddress)
    {
        BOOL bRet=TRUE;
        CLinkListItem* pItem;
        CModuleIntegrityCheckingChanges* pChange;

        // restore all changes of the section
        this->pChangesLinkList->Lock();
        for (pItem=this->pChangesLinkList->Head;pItem;pItem=pItem->NextItem)
        {
            pChange=(CModuleIntegrityCheckingChanges*)pItem->ItemData;
            bRet&=pChange->RestoreIntegrity(ProcessId,DllBaseAddress);
        }
        this->pChangesLinkList->Unlock();
        return bRet;
    }
};