/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

Reference
An In-Depth Look into the Win32 Portable Executable File Format, Part 1
An In-Depth Look into the Win32 Portable Executable File Format, Part 2 
from Matt Pietrek 

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
// Object: PE helper
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#pragma intrinsic (memcpy,memset,memcmp)
#include <stdio.h>
#include <delayimp.h>
#include "../APIError/APIError.h"
#include "../LinkList/LinkList.h"
#include "../String/AnsiUnicodeConvert.h"
#include "../Dll/DllFinder.h"

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)


// this class is just a begin of pe parsing 
#define CPE_MAX_IMAGE_DIRECTORY_ENTRY 15
class CPE
{
protected:
    BOOL bNtHeaderParsed;
    BOOL GetRVASectionLimits(ULONGLONG KnownRVABelongingToSection,ULONGLONG* pStartRVA,ULONGLONG* pEndRVA);
    BOOL ParseIMAGE_SECTION_HEADER();
    BOOL ParseIMAGE_NT_HEADERS();
    BOOL ParseExportTable();
    BOOL ParseImportTable();
    BOOL ParseTls();
    BOOL ParseDelayImportTable();
    void RemoveImportTableItems();
    BOOL GetOrdinalImportedFunctionName(WORD Ordinal,CPE* pPe,OUT TCHAR* FunctionName);
    unsigned char* pBeginOfFile;
    void ShowError(TCHAR* pcMsg);
    TCHAR pcFilename[MAX_PATH];
    void CommonConstructor();
    DWORD IMAGE_NT_HEADERS_Size;
    DWORD IMAGE_THUNK_DATA_Size;
    BOOL GetSectionIndexFromRaw(ULONGLONG RAW,int* pSectionIndex);
    BOOL GetSectionIndexFromRva(ULONGLONG RVA,int* pSectionIndex);
    void ShowLastApiError();
    static HMODULE LoadImageHelpLibrary();
    static SIZE_T RoundToUpperAlignment(SIZE_T CurrentSize,SIZE_T Alignment);

    typedef PIMAGE_NT_HEADERS (__stdcall *pfCheckSumMappedFile)(PVOID BaseAddress,DWORD FileLength,PDWORD HeaderSum,PDWORD CheckSum);
    typedef BOOL (__stdcall *pfImageRemoveCertificate)(HANDLE FileHandle,DWORD Index);
public:
    typedef struct tagImportItem
    {
        IMAGE_IMPORT_DESCRIPTOR Descriptor;
        TCHAR LibraryName[MAX_PATH];
        BOOL bDelayLoadDll;
        CLinkList* pFunctions; // list of IMPORT_FUNCTION_ITEM
    }IMPORT_LIBRARY_ITEM,*PIMPORT_LIBRARY_ITEM;

    typedef struct tagImportFunctionItem
    {
        TCHAR FunctionName[MAX_PATH];
        WORD Hint;
        WORD Ordinal;
        BOOL bOrdinalOnly;
        DWORD PointerToIATEntryRVA;
        ULONGLONG FunctionAddress;
    }IMPORT_FUNCTION_ITEM,*PIMPORT_FUNCTION_ITEM;

    typedef struct tagExportFunctionItem
    {
        TCHAR FunctionName[MAX_PATH];
        ULONGLONG FunctionAddressRVA;
        WORD Hint;
        WORD ExportedOrdinal;
        BOOL  Forwarded;                // TRUE if function is forwarded
        TCHAR ForwardedName[MAX_PATH];  // DllName.EntryPointName
    }EXPORT_FUNCTION_ITEM,*PEXPORT_FUNCTION_ITEM;

    typedef struct
    {
        TCHAR* FunctionName;
        WORD Hint;
        WORD Ordinal;
        BOOL bOrdinal; // if TRUE fill ordinal info (Hint and FunctionName are useless)
                       // if FALSE, fill hint and function name (ordinal is useless)
    }ADD_IMPORT_FUNCTION_DESCRIPTION;
    typedef struct
    {
        TCHAR* LibraryName;
        ADD_IMPORT_FUNCTION_DESCRIPTION* FunctionDescriptionArray;
        SIZE_T FunctionDescriptionArraySize;
    }ADD_IMPORT_LIBRARY_DESCRIPTION;

    CPE(TCHAR* filename);
    CPE();
    ~CPE(void);
    BOOL Parse();
    BOOL Parse(BOOL ParseExportTable,BOOL ParseImportTable);
    BOOL Parse(BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry);
    BOOL ParseFromMemory(PBYTE BaseAddress,BOOL ParseExportTable,BOOL ParseImportTable);
    BOOL ParseFromMemory(PBYTE BaseAddress,BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry);
    BOOL Parse(TCHAR* filename,BOOL ParseExportTable,BOOL ParseImportTable);
    BOOL Parse(TCHAR* filename,BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry);
    BOOL SaveIMAGE_DOS_HEADER();
    BOOL SaveIMAGE_NT_HEADERS();
    BOOL SavePIMAGE_SECTION_HEADER();
    BOOL VaToRva(ULONGLONG VaAddress,ULONGLONG* pRvaAddress);
    BOOL RvaToVa(ULONGLONG RvaAddress,ULONGLONG* pVaAddress);
    BOOL RawToRva(ULONGLONG RawAddress,ULONGLONG* pRvaAddress);
    BOOL RvaToRaw(ULONGLONG RvaAddress,ULONGLONG* pRawAddress);
    BOOL GetOrdinalImportedFunctionName(WORD Ordinal,TCHAR* DllName,OUT TCHAR* FunctionName);
    BOOL Is64Bits();
    BOOL IsNET();
    BOOL IsDynamicallyBased();
    BOOL IsExecutable(ULONGLONG RvaAddress);
    ULONGLONG GetUnrebasedVirtualAddress(ULONGLONG RebasedRelativeAddress);
    void GetFileName(TCHAR* pszFilename);
    IMAGE_SECTION_HEADER* GetSectionHeader(CHAR* SectionName);

    BOOL GetChecksum(DWORD* pCurrentHeaderCheckSum,DWORD* pRealCheckSum);
    BOOL CorrectChecksum();
    static BOOL RemoveCertificate(TCHAR* FileName);
    static BOOL AddImports(TCHAR* FileName,TCHAR* AddedSectionName,ADD_IMPORT_LIBRARY_DESCRIPTION* ImportLibraryArray,SIZE_T ImportLibraryArraySize,BOOL BeforeExisting);

    BOOL bDisplayErrorMessages;
    BOOL bShowDllFindingErrorForManualSearch;
    IMAGE_DOS_HEADER DosHeader;
    // NTHeader.OptionalHeader.DataDirectory index :
    //IMAGE_DIRECTORY_ENTRY_EXPORT          0 
    //IMAGE_DIRECTORY_ENTRY_IMPORT          1 
    //IMAGE_DIRECTORY_ENTRY_RESOURCE        2 
    //IMAGE_DIRECTORY_ENTRY_EXCEPTION       3 
    //IMAGE_DIRECTORY_ENTRY_SECURITY        4 
    //IMAGE_DIRECTORY_ENTRY_BASERELOC       5 
    //IMAGE_DIRECTORY_ENTRY_DEBUG           6 
    //IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7 
    //IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7 
    //IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8 
    //IMAGE_DIRECTORY_ENTRY_TLS             9 
    //IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10 
    //IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11 
    //IMAGE_DIRECTORY_ENTRY_IAT            12 
    //IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13 
    //IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14 
    IMAGE_NT_HEADERS64 NTHeader; // provide a single storage for 32 and 64 binaries 
                                 // for same access in calling code whatever target is 32 or 64 bit 
    // all IMAGE_NT_HEADERS32 fields can be accessed from IMAGE_NT_HEADERS64, but BaseOfData which has disappeared
    DWORD IMAGE_NT_HEADERS32_BaseOfData; // valid only for 32 bits

    // size of pSectionHeaders is in NTHeader.FileHeader.NumberOfSections
    IMAGE_SECTION_HEADER* pSectionHeaders;

    IMAGE_TLS_DIRECTORY64 TlsDirectory;

    CLinkList* pImportTable; // list of IMPORT_LIBRARY_ITEM
    CLinkList* pExportTable; // list of EXPORT_FUNCTION_ITEM
    CLinkList* pTlsCallbacksTable;// list of tls callbacks (ULONGLONG)
};
