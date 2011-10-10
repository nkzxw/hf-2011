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

#include "pe.h"
#define GetFilePointer(x) (::SetFilePointer(x,0,0,FILE_CURRENT))

void CPE::CommonConstructor()
{
    this->IMAGE_NT_HEADERS32_BaseOfData = 0;
    this->bDisplayErrorMessages=TRUE;
    this->bNtHeaderParsed=FALSE;
    this->bShowDllFindingErrorForManualSearch = FALSE;
    *this->pcFilename=0;
    memset(&this->DosHeader,0,sizeof(IMAGE_DOS_HEADER));
    this->pSectionHeaders=NULL;

    memset(&this->TlsDirectory,0,sizeof(IMAGE_TLS_DIRECTORY64));

    this->pImportTable=new CLinkList(sizeof(CPE::IMPORT_LIBRARY_ITEM));
    this->pExportTable=new CLinkList(sizeof(CPE::EXPORT_FUNCTION_ITEM));
    this->pTlsCallbacksTable=new CLinkList(sizeof(ULONGLONG));
    this->IMAGE_NT_HEADERS_Size=sizeof(IMAGE_NT_HEADERS);
    this->IMAGE_THUNK_DATA_Size=sizeof(IMAGE_THUNK_DATA);
}

CPE::CPE()
{
    this->CommonConstructor();
}

CPE::CPE(TCHAR* filename)
{
    this->CommonConstructor();
    if (filename)
    {
        _tcsncpy(this->pcFilename,filename,MAX_PATH);
        this->pcFilename[MAX_PATH-1]=0;
    }
    else
        *this->pcFilename = 0;
}

CPE::~CPE(void)
{
    this->RemoveImportTableItems();
    delete this->pImportTable;
    delete this->pExportTable;
    delete this->pTlsCallbacksTable;

    if (this->pSectionHeaders)
        delete this->pSectionHeaders;
}
void CPE::RemoveImportTableItems()
{
    CLinkListItem* pLinkListItem;
    this->pImportTable->Lock();
    for (pLinkListItem=this->pImportTable->Head;pLinkListItem;pLinkListItem=pLinkListItem->NextItem)
    {
        delete ((CPE::PIMPORT_LIBRARY_ITEM)pLinkListItem->ItemData)->pFunctions;
    }
    this->pImportTable->RemoveAllItems(TRUE);
    this->pImportTable->Unlock();
}
void CPE::ShowError(TCHAR* pcMsg)
{
#if (!defined(TOOLS_NO_MESSAGEBOX))
    if (this->bDisplayErrorMessages)
        MessageBox(NULL,pcMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#else
    UNREFERENCED_PARAMETER(pcMsg);
#endif
}

void CPE::ShowLastApiError()
{
#if (!defined(TOOLS_NO_MESSAGEBOX))
    if (this->bDisplayErrorMessages)
        CAPIError::ShowLastError();
#endif
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
//          optionally parse import or export table
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseFromMemory(PBYTE BaseAddress,BOOL ParseExportTable,BOOL ParseImportTable)
{
    return this->ParseFromMemory((PBYTE)BaseAddress,ParseExportTable,ParseImportTable,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
//          optionally parse import, export table and tls entry
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseFromMemory(PBYTE BaseAddress,BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry)
{
    BOOL bRes=TRUE;

    // empty import and export table list
    this->RemoveImportTableItems();
    this->pExportTable->RemoveAllItems();
    this->pTlsCallbacksTable->RemoveAllItems();

    this->pBeginOfFile=(PBYTE)BaseAddress;

    // check for exe
    if (((PIMAGE_DOS_HEADER)this->pBeginOfFile)->e_magic ==IMAGE_DOS_SIGNATURE)
    {
        // store DOS header
        memcpy(&this->DosHeader,this->pBeginOfFile,sizeof(IMAGE_DOS_HEADER));
        // parse NTHeader (and the remaining of the file)
        bRes=this->ParseIMAGE_NT_HEADERS();
        if (bRes)
        {
            bRes=this->ParseIMAGE_SECTION_HEADER();
            if (!bRes)
                this->ShowError(_T("Error parsing IMAGE_SECTION_HEADER"));
        }

        if (bRes&&ParseExportTable)
        {
            bRes=this->ParseExportTable();
            if (!bRes)
                this->ShowError(_T("Error parsing export table"));
        }
        if (bRes&&ParseImportTable)
        {
            bRes=this->ParseImportTable();
            if (!bRes)
                this->ShowError(_T("Error parsing import table"));
        }
        if (bRes&&ParseTlsEntry)
        {
            bRes=this->ParseTls();
            if (!bRes)
                this->ShowError(_T("Error parsing tls entry"));
        }
    }
    else
    {
        this->ShowError(_T("Unrecognized file format."));
        bRes=FALSE;
    }

    return bRes;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::Parse(TCHAR* filename,BOOL ParseExportTable,BOOL ParseImportTable)
{
    return this->Parse(filename,ParseExportTable,ParseImportTable,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::Parse(TCHAR* filename,BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry)
{
    if (!filename)
        return FALSE;

    _tcsncpy(this->pcFilename,filename,MAX_PATH);
    this->pcFilename[MAX_PATH-1]=0;
    return this->Parse(ParseExportTable,ParseImportTable,ParseTlsEntry);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
//          don't parse import nor export table
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::Parse()
{
    return this->Parse(FALSE,FALSE,FALSE);
}


//-----------------------------------------------------------------------------
// Name: Parse
// Object: loads DosHeader, NTHeader and pSectionHeaders infos from file
//          optionally parse import or export table
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::Parse(BOOL ParseExportTable,BOOL ParseImportTable)
{
    return this->Parse(ParseExportTable,ParseImportTable,FALSE);
}
BOOL CPE::Parse(BOOL ParseExportTable,BOOL ParseImportTable,BOOL ParseTlsEntry)
{
    BOOL bRes;
    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID BaseAddress;

    if (*this->pcFilename==0)
        return FALSE;

    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        this->ShowLastApiError();
        return FALSE;
    }
    // create file mapping
    hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    if (!hFileMapping)
    {
        this->ShowLastApiError();
        CloseHandle(hFile);
        return FALSE;
    }
    // map view of file
    BaseAddress=MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);

    if (BaseAddress==NULL)
    {
        this->ShowLastApiError();
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    bRes=this->ParseFromMemory((PBYTE)BaseAddress,ParseExportTable,ParseImportTable,ParseTlsEntry);

    // unmap view of file
    UnmapViewOfFile(BaseAddress);
    // close file mapping
    CloseHandle(hFileMapping);
    // close file
    CloseHandle(hFile);

    return bRes;
}

//-----------------------------------------------------------------------------
// Name: ParseIMAGE_NT_HEADERS
// Object: loads NTHeader infos
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseIMAGE_NT_HEADERS()
{
	PIMAGE_NT_HEADERS32 pNTHeader32;
	pNTHeader32 = (PIMAGE_NT_HEADERS32)(this->pBeginOfFile+this->DosHeader.e_lfanew);
	// First, verify that the e_lfanew field gave us a reasonable
	// pointer, then verify the PE signature.
	if (IsBadReadPtr(pNTHeader32, sizeof(IMAGE_NT_HEADERS32)) ||
	     pNTHeader32->Signature != IMAGE_NT_SIGNATURE )
	{
        this->ShowError(_T("Error reading IMAGE_NT_HEADERS"));
		return FALSE;
	}
    if (pNTHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
    this->IMAGE_NT_HEADERS_Size=sizeof(IMAGE_NT_HEADERS64);
    this->IMAGE_THUNK_DATA_Size=sizeof(IMAGE_THUNK_DATA64);

    PIMAGE_NT_HEADERS64 pNTHeader64;
    pNTHeader64=(PIMAGE_NT_HEADERS64)pNTHeader32;
        if (IsBadReadPtr(pNTHeader64, sizeof(IMAGE_NT_HEADERS64)))
        {
            this->ShowError(_T("Error reading IMAGE_NT_HEADERS"));
            return FALSE;
        }
    // store NTHeader
    memcpy(&this->NTHeader,pNTHeader64,sizeof(IMAGE_NT_HEADERS64));

        
    }
    else
    {
        this->IMAGE_NT_HEADERS_Size=sizeof(IMAGE_NT_HEADERS32);
        this->IMAGE_THUNK_DATA_Size=sizeof(IMAGE_THUNK_DATA32);

        // convert IMAGE_NT_HEADERS32 to IMAGE_NT_HEADERS64
        this->NTHeader.Signature=pNTHeader32->Signature;
        memcpy(&this->NTHeader.FileHeader,&pNTHeader32->FileHeader,sizeof(IMAGE_FILE_HEADER));
        this->NTHeader.OptionalHeader.Magic=                          pNTHeader32->OptionalHeader.Magic;
        this->NTHeader.OptionalHeader.MajorLinkerVersion=             pNTHeader32->OptionalHeader.MajorLinkerVersion;
        this->NTHeader.OptionalHeader.MinorLinkerVersion=             pNTHeader32->OptionalHeader.MinorLinkerVersion;
        this->NTHeader.OptionalHeader.SizeOfCode=                     pNTHeader32->OptionalHeader.SizeOfCode;
        this->NTHeader.OptionalHeader.SizeOfInitializedData=          pNTHeader32->OptionalHeader.SizeOfInitializedData;
        this->NTHeader.OptionalHeader.SizeOfUninitializedData=        pNTHeader32->OptionalHeader.SizeOfUninitializedData;
        this->NTHeader.OptionalHeader.AddressOfEntryPoint=            pNTHeader32->OptionalHeader.AddressOfEntryPoint;
        this->NTHeader.OptionalHeader.BaseOfCode=                     pNTHeader32->OptionalHeader.BaseOfCode;
        this->IMAGE_NT_HEADERS32_BaseOfData=                          pNTHeader32->OptionalHeader.BaseOfData;
        this->NTHeader.OptionalHeader.ImageBase=                      pNTHeader32->OptionalHeader.ImageBase;
        this->NTHeader.OptionalHeader.SectionAlignment=               pNTHeader32->OptionalHeader.SectionAlignment;
        this->NTHeader.OptionalHeader.FileAlignment=                  pNTHeader32->OptionalHeader.FileAlignment;
        this->NTHeader.OptionalHeader.MajorOperatingSystemVersion=    pNTHeader32->OptionalHeader.MajorOperatingSystemVersion;
        this->NTHeader.OptionalHeader.MinorOperatingSystemVersion=    pNTHeader32->OptionalHeader.MinorOperatingSystemVersion;
        this->NTHeader.OptionalHeader.MajorImageVersion=              pNTHeader32->OptionalHeader.MajorImageVersion;
        this->NTHeader.OptionalHeader.MinorImageVersion=              pNTHeader32->OptionalHeader.MinorImageVersion;
        this->NTHeader.OptionalHeader.MajorSubsystemVersion=          pNTHeader32->OptionalHeader.MajorSubsystemVersion;
        this->NTHeader.OptionalHeader.MinorSubsystemVersion=          pNTHeader32->OptionalHeader.MinorSubsystemVersion;
        this->NTHeader.OptionalHeader.Win32VersionValue=              pNTHeader32->OptionalHeader.Win32VersionValue;
        this->NTHeader.OptionalHeader.SizeOfImage=                    pNTHeader32->OptionalHeader.SizeOfImage;
        this->NTHeader.OptionalHeader.SizeOfHeaders=                  pNTHeader32->OptionalHeader.SizeOfHeaders;
        this->NTHeader.OptionalHeader.CheckSum=                       pNTHeader32->OptionalHeader.CheckSum;
        this->NTHeader.OptionalHeader.Subsystem=                      pNTHeader32->OptionalHeader.Subsystem;
        this->NTHeader.OptionalHeader.DllCharacteristics=             pNTHeader32->OptionalHeader.DllCharacteristics;
        this->NTHeader.OptionalHeader.SizeOfStackReserve=             pNTHeader32->OptionalHeader.SizeOfStackReserve;
        this->NTHeader.OptionalHeader.SizeOfStackCommit=              pNTHeader32->OptionalHeader.SizeOfStackCommit;
        this->NTHeader.OptionalHeader.SizeOfHeapReserve=              pNTHeader32->OptionalHeader.SizeOfHeapReserve;
        this->NTHeader.OptionalHeader.SizeOfHeapCommit=               pNTHeader32->OptionalHeader.SizeOfHeapCommit;
        this->NTHeader.OptionalHeader.LoaderFlags=                    pNTHeader32->OptionalHeader.LoaderFlags;
        this->NTHeader.OptionalHeader.NumberOfRvaAndSizes=            pNTHeader32->OptionalHeader.NumberOfRvaAndSizes;
        memcpy(this->NTHeader.OptionalHeader.DataDirectory,pNTHeader32->OptionalHeader.DataDirectory,sizeof(IMAGE_DATA_DIRECTORY)*IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
    }


    this->bNtHeaderParsed=TRUE;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: ParseIMAGE_SECTION_HEADER
// Object: loads pSectionHeaders infos
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseIMAGE_SECTION_HEADER()
{
    PIMAGE_SECTION_HEADER pSecHeader;
    // get sections informations
    pSecHeader=(PIMAGE_SECTION_HEADER)(this->pBeginOfFile+this->DosHeader.e_lfanew+this->IMAGE_NT_HEADERS_Size);
    if (IsBadReadPtr(pSecHeader, sizeof(IMAGE_SECTION_HEADER)*this->NTHeader.FileHeader.NumberOfSections))
	{
        this->ShowError(_T("Error in Sections Header"));
		return FALSE;
	}
    // free previous section headers if any
    if (this->pSectionHeaders!=NULL)
        delete this->pSectionHeaders;
    // allocate memory and copy data
    this->pSectionHeaders=new IMAGE_SECTION_HEADER[this->NTHeader.FileHeader.NumberOfSections];
    // get all sections infos
    memcpy(this->pSectionHeaders,pSecHeader,sizeof(IMAGE_SECTION_HEADER)*this->NTHeader.FileHeader.NumberOfSections);

    return TRUE;
}

BOOL CPE::ParseTls()
{
    ULONGLONG RawAddress;
    ULONGLONG RelativeVirtualAddress;

    // empty export table list
    this->pTlsCallbacksTable->RemoveAllItems();
    memset(&this->TlsDirectory,0,sizeof(IMAGE_TLS_DIRECTORY64));

    // if no tls data
    if (this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress==NULL)
        return TRUE;

    // get address of IMAGE_DIRECTORY_ENTRY_TLS 
    if (!this->RvaToRaw(this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress,&RawAddress))
        return FALSE;

    if (this->Is64Bits())
        memcpy(&this->TlsDirectory,this->pBeginOfFile+RawAddress,sizeof(IMAGE_TLS_DIRECTORY64));
    else
    {
        IMAGE_TLS_DIRECTORY32 TlsDir;
        memcpy(&TlsDir,this->pBeginOfFile+RawAddress,sizeof(IMAGE_TLS_DIRECTORY32));
        this->TlsDirectory.AddressOfCallBacks=TlsDir.AddressOfCallBacks;
        this->TlsDirectory.AddressOfIndex=TlsDir.AddressOfIndex;
        this->TlsDirectory.Characteristics=TlsDir.Characteristics;
        this->TlsDirectory.EndAddressOfRawData=TlsDir.EndAddressOfRawData;
        this->TlsDirectory.SizeOfZeroFill=TlsDir.SizeOfZeroFill;
        this->TlsDirectory.StartAddressOfRawData=TlsDir.StartAddressOfRawData;
    }
    if (this->TlsDirectory.AddressOfCallBacks==0)
        return TRUE;

    // get address of array of callbacks 
    if (!this->VaToRva(this->TlsDirectory.AddressOfCallBacks,&RelativeVirtualAddress))
        return FALSE;
    if (!this->RvaToRaw(RelativeVirtualAddress,&RawAddress))
        return FALSE;

    DWORD ImagePtrSize;
    if (this->Is64Bits())
        ImagePtrSize=sizeof(ULONGLONG);
    else
        ImagePtrSize=sizeof(ULONG);

    ULONGLONG PtrIMAGE_TLS_CALLBACK64;
    ULONG32   PtrIMAGE_TLS_CALLBACK32;
    ULONGLONG StartAddress=(ULONGLONG)this->pBeginOfFile+RawAddress;

    for (DWORD Cnt=0;;Cnt++)
    {
        // get address of each callback
        if (this->Is64Bits())
        {
            memcpy(&PtrIMAGE_TLS_CALLBACK64,(PBYTE)(StartAddress+Cnt*ImagePtrSize),ImagePtrSize);
        }
        else
        {
            memcpy(&PtrIMAGE_TLS_CALLBACK32,(PBYTE)(StartAddress+Cnt*ImagePtrSize),ImagePtrSize);
            PtrIMAGE_TLS_CALLBACK64=PtrIMAGE_TLS_CALLBACK32;
        }

        // if end of tls callback array is reached
        if (PtrIMAGE_TLS_CALLBACK64==0)
            break;

        this->pTlsCallbacksTable->AddItem(&PtrIMAGE_TLS_CALLBACK64,FALSE);
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ParseExportTable
// Object: parse the export table. Content of the export table is stored in 
//              CPE::pExportTable (list of EXPORT_FUNCTION_ITEM)
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseExportTable()
{
    // empty export table list
    this->pExportTable->RemoveAllItems();

    // if no export
    if (this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress==NULL)
        return TRUE;

    ULONGLONG RawAddress;
    IMAGE_EXPORT_DIRECTORY* pExportDirectory;
    CPE::EXPORT_FUNCTION_ITEM ExportFunctionItem={0};
    char** pFunctionName=0;
    PDWORD pFunctionAddress;
    PWORD pFunctionOrdinal=0;
    BOOL bFound;
    DWORD cnt2;
    DWORD ForwardedNameAddress;
    DWORD IMAGE_DIRECTORY_ENTRY_EXPORT_StartAddressRVA;
    DWORD IMAGE_DIRECTORY_ENTRY_EXPORT_EndAddressRVA;
    WORD Ordinal;
    char* pstrFunctionName;
    BOOL bNoNames=FALSE;// for ordinal export dll only
    BOOL bNoOrdinals;
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR* psz;
#endif

    // get limits of IMAGE_DIRECTORY_ENTRY_EXPORT DataDirectory to check for forwarded funcs
    IMAGE_DIRECTORY_ENTRY_EXPORT_StartAddressRVA=this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    IMAGE_DIRECTORY_ENTRY_EXPORT_EndAddressRVA=IMAGE_DIRECTORY_ENTRY_EXPORT_StartAddressRVA+
                                            this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (!this->RvaToRaw(IMAGE_DIRECTORY_ENTRY_EXPORT_StartAddressRVA,&RawAddress))
        return FALSE;

    pExportDirectory=(IMAGE_EXPORT_DIRECTORY*)(this->pBeginOfFile+RawAddress);

    // convert RVA pointers to RAW pointer, and next make them point to our file mapping
    if (!this->RvaToRaw(pExportDirectory->AddressOfFunctions,&RawAddress))
        return FALSE;
    pFunctionAddress=(DWORD*)(this->pBeginOfFile+RawAddress);

    if (pExportDirectory->AddressOfNameOrdinals)
    {
        bNoOrdinals=FALSE;
        if (!this->RvaToRaw(pExportDirectory->AddressOfNameOrdinals,&RawAddress))
            return FALSE;
        pFunctionOrdinal=(WORD*)(this->pBeginOfFile+RawAddress);
    }
    else
        bNoOrdinals=TRUE;

    if ((pExportDirectory->AddressOfNames==0)||(pExportDirectory->NumberOfNames==0))
        bNoNames=TRUE;
    else
    {
        if (!this->RvaToRaw(pExportDirectory->AddressOfNames,&RawAddress))
            return FALSE;
        pFunctionName=(char**)(this->pBeginOfFile+RawAddress);
    }

    // for each exported func
    for (DWORD cnt=0; cnt < pExportDirectory->NumberOfFunctions; cnt++, pFunctionAddress++)
    {

        if (*pFunctionAddress==0)// Skip over gaps in exported function ordinals
            continue;            // (the entry point is 0 for these functions)


        bFound=FALSE;

        // get func address
        ExportFunctionItem.FunctionAddressRVA=*pFunctionAddress;

        *ExportFunctionItem.FunctionName=0;
        if (!bNoOrdinals)
        {
            // Get function Hint
            for ( cnt2=0; cnt2 < pExportDirectory->NumberOfFunctions; cnt2++ )
            {
                // if func number is found in it
                if ( pFunctionOrdinal[cnt2] != cnt )
                    continue;

                ExportFunctionItem.Hint=(WORD)cnt2;
                bFound=TRUE;
                break;
            }
        }

        if(!bFound)// can appear on some dll. Function have only ordinal and entry point
        {
            ExportFunctionItem.ExportedOrdinal = (WORD)(cnt+pExportDirectory->Base);
            *ExportFunctionItem.ForwardedName=0;
            ExportFunctionItem.Forwarded = FALSE;
            ExportFunctionItem.Hint = 0xFFFF;
            // add item to list
            this->pExportTable->AddItem(&ExportFunctionItem);

            continue;
        }

        // get func name
        if (bFound && (!bNoNames) && (ExportFunctionItem.Hint<pExportDirectory->NumberOfNames))
        {
            if (!this->RvaToRaw((ULONGLONG)(pFunctionName[ExportFunctionItem.Hint]),&RawAddress))
                return FALSE;

            pstrFunctionName=(char*)(this->pBeginOfFile+RawAddress);

#if (defined(UNICODE)||defined(_UNICODE))
            CAnsiUnicodeConvert::AnsiToUnicode(pstrFunctionName,&psz);
            _tcsncpy(ExportFunctionItem.FunctionName,psz,MAX_PATH-1);
            free(psz);
#else
            _tcsncpy(ExportFunctionItem.FunctionName,pstrFunctionName,MAX_PATH-1);
#endif
            ExportFunctionItem.FunctionName[MAX_PATH-1]=0;
        }

        if (!bNoOrdinals)
        {
            // get func ordinal
            Ordinal=pFunctionOrdinal[ExportFunctionItem.Hint];// Ordinal = cnt;
            ExportFunctionItem.ExportedOrdinal=(WORD)(Ordinal+pExportDirectory->Base);
        }

        *ExportFunctionItem.ForwardedName=0;
        // check if func is forwarded
        // forwarded funcs RVA are inside the IMAGE_DIRECTORY_ENTRY_EXPORT DataDirectory
        ExportFunctionItem.Forwarded=((IMAGE_DIRECTORY_ENTRY_EXPORT_StartAddressRVA<=ExportFunctionItem.FunctionAddressRVA)
                    &&(ExportFunctionItem.FunctionAddressRVA<IMAGE_DIRECTORY_ENTRY_EXPORT_EndAddressRVA));

        // if func is forwarded
        if (ExportFunctionItem.Forwarded)
        {
            // get forwarded address
            ForwardedNameAddress=ExportFunctionItem.FunctionAddressRVA;
            if (!this->RvaToRaw(ForwardedNameAddress,&RawAddress))
                return FALSE;

            // copy forwarded func
            ForwardedNameAddress=(DWORD)(this->pBeginOfFile+RawAddress);
#if (defined(UNICODE)||defined(_UNICODE))
            CAnsiUnicodeConvert::AnsiToUnicode((char*)ForwardedNameAddress,&psz);
            _tcsncpy(ExportFunctionItem.ForwardedName,psz,MAX_PATH-1);
            free(psz);
#else
            _tcsncpy(ExportFunctionItem.ForwardedName,(char*)ForwardedNameAddress,MAX_PATH-1);
#endif
            ExportFunctionItem.ForwardedName[MAX_PATH-1]=0;
        }

        // add item to list
        this->pExportTable->AddItem(&ExportFunctionItem);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseDelayImportTable
// Object: parse the delay import table. Content of the delay import table is added to 
//              CPE::pImportTable (list of IMPORT_LIBRARY_ITEM)
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseDelayImportTable()
{
    PCImgDelayDescr pDelayDesc;
    ULONGLONG RawAddress;
    ULONGLONG RVA;
    BOOL bUsingRVA;
    PSTR pszDLLName;
    PBYTE pThunk;
    IMAGE_THUNK_DATA64 Thunk64;
    PIMAGE_THUNK_DATA32 pThunk32=NULL;
    PIMAGE_IMPORT_BY_NAME pImportByName;
    ULONGLONG dllNameRVA;
    TCHAR* psz;
    CPE::IMPORT_LIBRARY_ITEM ImportLibItem;
    CPE::IMPORT_FUNCTION_ITEM ImportFuncItem;
    CPE* pPE;
    BOOL bPEParseSuccess;

    if (this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress==0)
        return TRUE;

    if (!this->RvaToRaw(this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress,&RawAddress))
        return FALSE;

    // This code is more complicated than it needs to be, thanks to Microsoft.  When the
    // ImgDelayDescr was originally created for Win32, portability to Win64 wasn't
    // considered.  As such, MS used pointers, rather than RVAs in the data structures.
    // Finally, MS issued a new DELAYIMP.H, which issued a flag indicating whether the
    // field values are RVAs or VAs.  Unfortunately, Microsoft has been rather slow to
    // get this header file out into general distribution.  Currently, you can get it as
    // part of the Win64 headers, or as part of VC7.  In the meanwhile, we'll use some
    // preprocessor trickery so that we can use the new field names, while still compiling
    // with the original DELAYIMP.H.

#if _DELAY_IMP_VER < 2
#define rvaDLLName		szName
#define rvaHmod			phmod
#define rvaIAT			pIAT
#define rvaINT			pINT
#define rvaBoundIAT		pBoundIAT
#define rvaUnloadIAT	pUnloadIAT
#endif

    ImportLibItem.bDelayLoadDll = TRUE;
    pDelayDesc = (PCImgDelayDescr)(this->pBeginOfFile+RawAddress);
    while ( pDelayDesc->rvaDLLName )
    {
        // from more recent DELAYIMP.H:
        // enum DLAttr {                   // Delay Load Attributes
        //    dlattrRva = 0x1,                // RVAs are used instead of pointers
        //    };
        bUsingRVA = pDelayDesc->grAttrs & 1;

        // get RVA
        if (bUsingRVA)
            // if header use RVA
            dllNameRVA=(ULONGLONG)pDelayDesc->rvaDLLName;
        else
        {
            // else header use VA : convert it to RVA
            if (!this->VaToRva((ULONGLONG)pDelayDesc->rvaDLLName,&dllNameRVA))
                return FALSE;
        }

        // get raw address
        if (!this->RvaToRaw(dllNameRVA, &RawAddress))
            return FALSE;
        pszDLLName = (PSTR)(this->pBeginOfFile+RawAddress);


#if (defined(UNICODE)||defined(_UNICODE))
        CAnsiUnicodeConvert::AnsiToUnicode(pszDLLName,&psz);
        _tcsncpy(ImportLibItem.LibraryName,psz,MAX_PATH-1);
        free(psz);
#else
        _tcsncpy(ImportLibItem.LibraryName,pszDLLName,MAX_PATH-1);
#endif
        ImportLibItem.LibraryName[MAX_PATH-1]=0;

        // create a list of IMPORT_FUNCTION_ITEM
        ImportLibItem.pFunctions=new CLinkList(sizeof(CPE::IMPORT_FUNCTION_ITEM));

        // add the IMPORT_LIBRARY_ITEM into this->pImportTable
        if(!this->pImportTable->AddItem(&ImportLibItem))
        {
            delete ImportLibItem.pFunctions;
            return FALSE;
        }

        // get the Import Names Table.
        if (bUsingRVA)
            RVA=(DWORD)pDelayDesc->rvaINT;
        else
        {
            // convert VA to RVA
            if (!this->VaToRva(pDelayDesc->rvaINT,&RVA))
                return FALSE;
        }
        // get raw address
        if (!this->RvaToRaw(RVA, &RawAddress))
            return FALSE;

        pThunk=(PBYTE)(this->pBeginOfFile+RawAddress);
        if (IsBadReadPtr(pThunk,this->IMAGE_THUNK_DATA_Size))
            return FALSE;
        if (this->Is64Bits())
        {
            memcpy(&Thunk64,pThunk,sizeof(IMAGE_THUNK_DATA64));

        }
        else
        {
            pThunk32=(PIMAGE_THUNK_DATA32)pThunk;
            // union of ULONGLONG vs union of DWORD
            // only equals 1 term is enough
            Thunk64.u1.Ordinal=pThunk32->u1.Ordinal;
        }

        bPEParseSuccess=FALSE;
        pPE=NULL; // reset pPE
        // if (pPE==NULL)
        {

            TCHAR pszDirectory[MAX_PATH+1];
            TCHAR pszFile[MAX_PATH];
            TCHAR pszPath[MAX_PATH];

            // get imported dll filename
            _tcscpy(pszFile,ImportLibItem.LibraryName);
            // get directory of current file
            _tcscpy(pszDirectory,this->pcFilename);
            psz=_tcsrchr(pszDirectory,'\\');
            if (psz)
            {
                // ends directory
                psz++;
                *psz=0;
            }
            // if imported dll found
            if (CDllFinder::FindDll(this->pcFilename,pszDirectory,pszFile,pszPath,this->bShowDllFindingErrorForManualSearch))
            {
                _tcscpy(pszFile,pszPath);

                // parse export table of the imported dll to get name corresponding to ordinals
                pPE=new CPE(pszFile);
                bPEParseSuccess=pPE->Parse(TRUE,FALSE,FALSE);
            }
#ifdef _DEBUG
            else
            {
                TCHAR Msg[2*MAX_PATH];
                _sntprintf(Msg, 2*MAX_PATH, _T("Dll %s imported by %s not found\r\n"),pszFile,this->pcFilename);
                ::OutputDebugString(Msg);
            }
#endif
        }

        while (Thunk64.u1.AddressOfData!=0) // Until there's imported func
        {
            ImportFuncItem.bOrdinalOnly=FALSE;

            // get ordinal value
            if ( this->Is64Bits() )
            {
                if ( IMAGE_SNAP_BY_ORDINAL64(Thunk64.u1.Ordinal) )
                {
                    ImportFuncItem.Ordinal= (WORD)IMAGE_ORDINAL64(Thunk64.u1.Ordinal);
                    ImportFuncItem.bOrdinalOnly=TRUE;
                    ImportFuncItem.Hint=0xFFFF;
                }
            }
            else
            {
                if ( IMAGE_SNAP_BY_ORDINAL32(pThunk32->u1.Ordinal) )
                {
                    ImportFuncItem.Ordinal = (WORD)IMAGE_ORDINAL32(pThunk32->u1.Ordinal);
                    ImportFuncItem.bOrdinalOnly=TRUE;
                    ImportFuncItem.Hint=0xFFFF;
                }
            }

            if (ImportFuncItem.bOrdinalOnly)
            {
                *ImportFuncItem.FunctionName=0;

                // if pe parsing success
                if (bPEParseSuccess)
                    // try to retrieve name from export table of dll
                    this->GetOrdinalImportedFunctionName(ImportFuncItem.Ordinal,pPE,ImportFuncItem.FunctionName);

            }
            else
            {
                ImportFuncItem.Ordinal=0xFFFF;
                if (bUsingRVA)
                    RVA= Thunk64.u1.AddressOfData;
                else
                {   
                    // convert VA to RVA
                    if (!this->VaToRva(Thunk64.u1.AddressOfData,&RVA))
                    {
                        if (pPE)
                            delete pPE;
                        return FALSE;
                    }
                }
                // get raw address
                if (!this->RvaToRaw(RVA, &RawAddress))
                {
                    if (pPE)
                        delete pPE;
                    return FALSE;
                }

                pImportByName=(PIMAGE_IMPORT_BY_NAME)(this->pBeginOfFile+RawAddress);

                ImportFuncItem.Hint=pImportByName->Hint;
                // check if name is filled
                if (*pImportByName->Name!=0)
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    CAnsiUnicodeConvert::AnsiToUnicode((char*)pImportByName->Name,&psz);
                    _tcsncpy(ImportFuncItem.FunctionName,psz,MAX_PATH-1);
                    free(psz);
#else
                    _tcsncpy(ImportFuncItem.FunctionName,(char*)pImportByName->Name,MAX_PATH-1);
#endif
                    ImportFuncItem.FunctionName[MAX_PATH-1]=0;
                }
                else
                {
                    if (pPE)
                        delete pPE;
                    return FALSE;
                }
            }

            // add function to ImportLibItem.pFunctions
            if (!ImportLibItem.pFunctions->AddItem(&ImportFuncItem))
            {
                if (pPE)
                    delete pPE;
                return FALSE;
            }

            pThunk+=this->IMAGE_THUNK_DATA_Size;            // Advance to next thunk

            if (this->Is64Bits())
            {
                memcpy(&Thunk64,pThunk,sizeof(IMAGE_THUNK_DATA64));
            }
            else
            {
                pThunk32=(PIMAGE_THUNK_DATA32)pThunk;
                // union of ULONGLONG vs union of DWORD
                // only equals 1 term is enough
                Thunk64.u1.Ordinal=pThunk32->u1.Ordinal;
            }
        }

        // free pPE if it has been allocated
        if (pPE)
            delete pPE;

        pDelayDesc++;	// Pointer math.  Advance to next delay import desc.
    }

#if _DELAY_IMP_VER < 2 // Remove the alias names from the namespace
#undef szName
#undef phmod
#undef pIAT
#undef pINT
#undef pBoundIAT
#undef pUnloadIAT
#endif
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseImportTable
// Object: parse the import table. Content of the import table is stored in 
//              CPE::pImportTable (list of IMPORT_LIBRARY_ITEM)
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::ParseImportTable()
{
    // empty import table list
    this->RemoveImportTableItems();

    if (this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress==0)
        // parse delay import table
        return this->ParseDelayImportTable();

    ULONGLONG RawAddress;
    IMAGE_IMPORT_DESCRIPTOR* pImportDirectory;
    IMAGE_IMPORT_BY_NAME* pImportByName;
    IMAGE_THUNK_DATA64 Thunk64;
    PIMAGE_THUNK_DATA32 pThunk32=NULL;
    PBYTE pThunk=0;
    CPE::IMPORT_LIBRARY_ITEM ImportLibItem;
    CPE::IMPORT_FUNCTION_ITEM ImportFuncItem;
    ULONGLONG SectionMinRVA;
    ULONGLONG SectionMaxRVA;
    BOOL BadThunkDataPointer;
    CPE* pPE;
    BOOL bPEParseSuccess;
    TCHAR* psz;
    char* pstrLibName;
    
    if (!this->RvaToRaw(this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,&RawAddress))
        return FALSE;
    
    pImportDirectory=(IMAGE_IMPORT_DESCRIPTOR*)(this->pBeginOfFile+RawAddress);


    if (!this->GetRVASectionLimits(this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                             &SectionMinRVA,
                             &SectionMaxRVA))
                             return FALSE;

    ImportLibItem.bDelayLoadDll = FALSE;
    // for each imported dll
    for (;;)
    {
        // See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR (no more items)
        if ( (pImportDirectory->TimeDateStamp==0 ) && (pImportDirectory->Name==0) )
            break;

        // get lib name
        if (!this->RvaToRaw(pImportDirectory->Name,&RawAddress))
            return FALSE;

        pstrLibName=(char*)this->pBeginOfFile+RawAddress;
#if (defined(UNICODE)||defined(_UNICODE))
        CAnsiUnicodeConvert::AnsiToUnicode(pstrLibName,&psz);
        _tcsncpy(ImportLibItem.LibraryName,psz,MAX_PATH-1);
        free(psz);
#else
        _tcsncpy(ImportLibItem.LibraryName,pstrLibName,MAX_PATH-1);
#endif
        ImportLibItem.LibraryName[MAX_PATH-1]=0;

        // create a list of IMPORT_FUNCTION_ITEM
        ImportLibItem.pFunctions=new CLinkList(sizeof(CPE::IMPORT_FUNCTION_ITEM));

        // store informations to ImportLibItem.Descriptor
        ImportLibItem.Descriptor = *pImportDirectory;

        // add the IMPORT_LIBRARY_ITEM into this->pImportTable
        if(!this->pImportTable->AddItem(&ImportLibItem))
        {
            delete ImportLibItem.pFunctions;
            return FALSE;
        }

        BadThunkDataPointer=FALSE;
        // pImportDirectory->FirstThunk = RVA to IAT (if bound this IAT has actual addresses)
        // find ThunkData to get function names and ordinals
        if (!this->RvaToRaw(pImportDirectory->FirstThunk,&RawAddress))
            BadThunkDataPointer=TRUE;
        else
        {
            // get pThunk
            pThunk=(PBYTE)(this->pBeginOfFile+RawAddress);
            if (IsBadReadPtr(pThunk,this->IMAGE_THUNK_DATA_Size))
                BadThunkDataPointer=TRUE;
            else
            {
		        if ( (*(PDWORD)pThunk <= SectionMinRVA) || (*(PDWORD)pThunk >= SectionMaxRVA) )
                    BadThunkDataPointer=TRUE;
            }
        }
        // If the pointer that thunk points to is outside of the 
        // current section, it looks like this file is "pre-fixed up" with regards
		// to the thunk table.  In this situation, we'll need to fall back
		// to the hint-name (aka, the "Characteristics") table.
		if (BadThunkDataPointer )
		{
			if ( pImportDirectory->Characteristics == 0 )
				return FALSE;

            if (!this->RvaToRaw(pImportDirectory->Characteristics,&RawAddress))
                return FALSE;
            // update pThunk (The contents of Import Address Table RVA [Thunk Table] are identical to the contents 
            // of the import lookup table [Characteristics] until the image is bound)
            // --> use Characteristics instead
            pThunk=(PBYTE)(this->pBeginOfFile+RawAddress);
        }

        if (this->Is64Bits())
        {
            memcpy(&Thunk64,pThunk,sizeof(IMAGE_THUNK_DATA64));
        }
        else
        {
            pThunk32=(PIMAGE_THUNK_DATA32)pThunk;
            // union of ULONGLONG vs union of DWORD
            // only equals 1 term is enough
            Thunk64.u1.Ordinal=pThunk32->u1.Ordinal;
        }

        ImportFuncItem.PointerToIATEntryRVA = pImportDirectory->FirstThunk;
        // adjust ImportFuncItem.PointerToIATEntryRVA because of pre increment
        if (this->Is64Bits())
            ImportFuncItem.PointerToIATEntryRVA-=sizeof(ULONGLONG);
        else // 32 bits
            ImportFuncItem.PointerToIATEntryRVA-=sizeof(DWORD);

        bPEParseSuccess=FALSE;
        pPE=NULL;
        // if (pPE==NULL)
        {

            TCHAR pszDirectory[MAX_PATH+1];
            TCHAR pszFile[MAX_PATH];
            TCHAR pszPath[MAX_PATH];

            // get imported dll filename
            _tcscpy(pszFile,ImportLibItem.LibraryName);
            // get directory of current file
            _tcscpy(pszDirectory,this->pcFilename);
            psz=_tcsrchr(pszDirectory,'\\');
            if (psz)
            {
                // ends directory
                psz++;
                *psz=0;
            }
            // if imported dll found
            if (CDllFinder::FindDll(this->pcFilename,pszDirectory,pszFile,pszPath,this->bShowDllFindingErrorForManualSearch))
            {
                _tcscpy(pszFile,pszPath);

                // parse export table of the imported dll to get name corresponding to ordinals
                pPE=new CPE(pszFile);
                bPEParseSuccess=pPE->Parse(TRUE,FALSE,FALSE);
            }
#ifdef _DEBUG
            else
            {
                TCHAR Msg[2*MAX_PATH];
                _sntprintf(Msg, 2*MAX_PATH, _T("Dll %s imported by %s not found\r\n"),pszFile,this->pcFilename);
                ::OutputDebugString(Msg);
            }
#endif
        }

        // Import lookup table parsing and import address table parsing
        // for each imported func
        while( Thunk64.u1.AddressOfData != 0)// Until there's imported func
        {
            ///////////////////////////////////////
            // fill iat informations from Import Address Table
            ///////////////////////////////////////
            if (this->Is64Bits())
            {
                ImportFuncItem.PointerToIATEntryRVA+=sizeof(ULONGLONG);
                if (!this->RvaToRaw(ImportFuncItem.PointerToIATEntryRVA,&RawAddress))
                    return FALSE;
                ImportFuncItem.FunctionAddress =*(ULONGLONG*)(this->pBeginOfFile+RawAddress);
            }
            else
            {
                // 32 bits
                ImportFuncItem.PointerToIATEntryRVA+=sizeof(DWORD);
                if (!this->RvaToRaw(ImportFuncItem.PointerToIATEntryRVA,&RawAddress))
                    return FALSE;
                ImportFuncItem.FunctionAddress =*(DWORD*)(this->pBeginOfFile+RawAddress);
            }


            ///////////////////////////////////////
            // get infos from Import lookup table
            ///////////////////////////////////////
            ImportFuncItem.bOrdinalOnly=FALSE;

            // get ordinal value
            if ( this->Is64Bits() )
            {
                if ( IMAGE_SNAP_BY_ORDINAL64(Thunk64.u1.Ordinal) )
                {
                    ImportFuncItem.Ordinal= (WORD)IMAGE_ORDINAL64(Thunk64.u1.Ordinal);
                    ImportFuncItem.bOrdinalOnly=TRUE;
                    ImportFuncItem.Hint=0xFFFF;
                }
            }
            else
            {
                if ( IMAGE_SNAP_BY_ORDINAL32(pThunk32->u1.Ordinal) )
                {
                    ImportFuncItem.Ordinal = (WORD)IMAGE_ORDINAL32(pThunk32->u1.Ordinal);
                    ImportFuncItem.bOrdinalOnly=TRUE;
                    ImportFuncItem.Hint=0xFFFF;
                }
            }

            if (ImportFuncItem.bOrdinalOnly)
            {
                *ImportFuncItem.FunctionName=0;

                // if pe parsing success
                if (bPEParseSuccess)
                    // try to retrieve name from export table of dll
                    this->GetOrdinalImportedFunctionName(ImportFuncItem.Ordinal,pPE,ImportFuncItem.FunctionName);
            }
            else
			{
                ImportFuncItem.Ordinal=0xFFFF;
                if (!this->RvaToRaw(Thunk64.u1.AddressOfData,&RawAddress))
                {
                    if (pPE)
                        delete pPE;
                    return FALSE;
                }
                pImportByName=(IMAGE_IMPORT_BY_NAME*)(this->pBeginOfFile+RawAddress);
                ImportFuncItem.Hint=pImportByName->Hint;
                // check if name is filled
                if (*pImportByName->Name!=0)
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    CAnsiUnicodeConvert::AnsiToUnicode((char*)pImportByName->Name,&psz);
                    _tcsncpy(ImportFuncItem.FunctionName,psz,MAX_PATH-1);
                    free(psz);
#else
                    _tcsncpy(ImportFuncItem.FunctionName,(char*)pImportByName->Name,MAX_PATH-1);
#endif
                    ImportFuncItem.FunctionName[MAX_PATH-1]=0;
                }
                else
                {
                    if (pPE)
                        delete pPE;
                    return FALSE;
                }
            }


            // add function to ImportLibItem.pFunctions
            if (!ImportLibItem.pFunctions->AddItem(&ImportFuncItem))
            {
                if (pPE)
                    delete pPE;
                return FALSE;
            }

            // go to next IMAGE_THUNK_DATA
            pThunk+=this->IMAGE_THUNK_DATA_Size;

            if (this->Is64Bits())
            {
                memcpy(&Thunk64,pThunk,sizeof(IMAGE_THUNK_DATA64));
            }
            else
            {
                pThunk32=(PIMAGE_THUNK_DATA32)pThunk;
                // union of ULONGLONG vs union of DWORD
                // only equals 1 term is enough
                Thunk64.u1.Ordinal=pThunk32->u1.Ordinal;
            }
        }
        // free pPE if it has been allocated
        if (pPE)
            delete pPE;

        // go to next IMAGE_IMPORT_DESCRIPTOR
        pImportDirectory++;
    }

    // parse delay import table
    return this->ParseDelayImportTable();
}

//-----------------------------------------------------------------------------
// Name: GetOrdinalImportedFunctionName
// Object: Get function name from ordinal 
// Parameters :
//     in  : WORD Ordinal : imported ordinal value
//           TCHAR* DllName : Name of imported dll 
//     out : TCHAR* FunctionName : name of the function. Should be MAX_PATH size at least
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::GetOrdinalImportedFunctionName(WORD Ordinal,TCHAR* DllName,OUT TCHAR* FunctionName)
{
    CPE pe(DllName);
    if (!pe.Parse(TRUE,FALSE,FALSE))
        return FALSE;

    return this->GetOrdinalImportedFunctionName(Ordinal,&pe,FunctionName);
}

//-----------------------------------------------------------------------------
// Name: GetOrdinalImportedFunctionName
// Object: Get function name from ordinal 
// Parameters :
//     in  : WORD Ordinal
//           CPE* pPe : pointer to the Pe of file containing Export function name and ordinal
//     out : TCHAR* FunctionName : name of the function. Should be MAX_PATH size at least
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::GetOrdinalImportedFunctionName(WORD Ordinal,CPE* pPE,OUT TCHAR* FunctionName)
{
    *FunctionName=0;
    PEXPORT_FUNCTION_ITEM pExportFunction;
    CLinkListItem* pItemLibExport;

    // find function in exported array
    
    pPE->pExportTable->Lock();
    for (pItemLibExport=pPE->pExportTable->Head;pItemLibExport;pItemLibExport=pItemLibExport->NextItem)
    {
        pExportFunction=(PEXPORT_FUNCTION_ITEM)pItemLibExport->ItemData;
        // check if function is found
        if (Ordinal==pExportFunction->ExportedOrdinal)
        {
            if (*pExportFunction->FunctionName==0)
            {
                pPE->pExportTable->Unlock();
                return FALSE;
            }
            _tcscpy(FunctionName,pExportFunction->FunctionName);
            pPE->pExportTable->Unlock();
            return TRUE;
        }
        
    }
    pPE->pExportTable->Unlock();
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetRVASectionLimits
// Object: Get Section limit of KnownRVABelongingToSection RVA address 
// Parameters :
//     in  : ULONGLONG KnownRVABelongingToSection : an RVA belonging to a section
//     out : ULONGLONG* pStart : begin of the section 
//           ULONGLONG* pEnd : end of the section
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::GetRVASectionLimits(ULONGLONG KnownRVABelongingToSection,ULONGLONG* pStart,ULONGLONG* pEnd)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    if (IsBadWritePtr(pStart,sizeof(DWORD))||IsBadWritePtr(pEnd,sizeof(DWORD)))
        return FALSE;
    int SectionIndex;
    if (!this->GetSectionIndexFromRva(KnownRVABelongingToSection,&SectionIndex))
        return FALSE;

    *pStart=this->pSectionHeaders[SectionIndex].VirtualAddress;
    *pEnd=this->pSectionHeaders[SectionIndex].VirtualAddress+this->pSectionHeaders[SectionIndex].SizeOfRawData;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsExecutable
// Object: check if a RVA address is in an executable section
// Parameters :
//     in  : ULONGLONG RvaAddress : an RVA belonging to a section
//     out : 
//     return : TRUE if a RVA address is in an executable section
//-----------------------------------------------------------------------------
BOOL CPE::IsExecutable(ULONGLONG RvaAddress)
{
    int SectionIndex;
    if (!this->GetSectionIndexFromRva(RvaAddress,&SectionIndex))
        return FALSE;

    return ( (this->pSectionHeaders[SectionIndex].Characteristics & IMAGE_SCN_CNT_CODE)
             || (this->pSectionHeaders[SectionIndex].Characteristics & IMAGE_SCN_MEM_EXECUTE));
}

//-----------------------------------------------------------------------------
// Name: GetSectionIndexFromRaw
// Object: get section index from raw address
// Parameters :
//     in  : ULONGLONG RAW : raw address
//     out : 
//     return : TRUE if a matching section index has been found
//-----------------------------------------------------------------------------
BOOL CPE::GetSectionIndexFromRaw(ULONGLONG RAW,int* pSectionIndex)
{
    ULONGLONG RVA;
    if (!this->RawToRva(RAW,&RVA))
        return FALSE;
    return this->GetSectionIndexFromRva(RVA,pSectionIndex);
}

//-----------------------------------------------------------------------------
// Name: GetSectionIndexFromRva
// Object: get section index from raw address
// Parameters :
//     in  : ULONGLONG RVA : rva address
//     out : 
//     return : TRUE if a matching section index has been found
//-----------------------------------------------------------------------------
BOOL CPE::GetSectionIndexFromRva(ULONGLONG RVA,int* pSectionIndex)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    for (int cnt=0;cnt<this->NTHeader.FileHeader.NumberOfSections;cnt++)
    {
        if ((this->pSectionHeaders[cnt].VirtualAddress<=RVA)
            && (this->pSectionHeaders[cnt].VirtualAddress+this->pSectionHeaders[cnt].Misc.VirtualSize>RVA))
        {
            *pSectionIndex=cnt;
            return TRUE;
        }
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: SaveIMAGE_DOS_HEADER
// Object: save DosHeader to file
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::SaveIMAGE_DOS_HEADER()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    HANDLE hFile;
    BOOL bRet=TRUE;
    DWORD dwNbBytesWritten;
    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        this->ShowLastApiError();
        return FALSE;
    }
    // write IMAGE_DOS_HEADER
    if (!WriteFile(hFile,&this->DosHeader,sizeof(IMAGE_DOS_HEADER),&dwNbBytesWritten,NULL))
    {
        this->ShowLastApiError();
        bRet=FALSE;
    }
    // close file
    CloseHandle(hFile);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SaveIMAGE_NT_HEADERS
// Object: save NTHeader to file
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::SaveIMAGE_NT_HEADERS()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    HANDLE hFile;
    BOOL bRet=TRUE;
    DWORD dwNbBytesWritten;
    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        this->ShowLastApiError();
        return FALSE;
    }
    // move to IMAGE_NT_HEADERS start
    SetFilePointer(hFile,this->DosHeader.e_lfanew,0,FILE_CURRENT);
    // write IMAGE_NT_HEADERS
    if (this->Is64Bits())
    {
    if (!WriteFile(hFile,&this->NTHeader,sizeof(IMAGE_NT_HEADERS64),&dwNbBytesWritten,NULL))
    {
        this->ShowLastApiError();
        ::CloseHandle(hFile);
        bRet=FALSE;
    }
    }
    else
    {
        // convert IMAGE_NT_HEADERS64 to IMAGE_NT_HEADERS32
        IMAGE_NT_HEADERS32 NTHeader32;

        NTHeader32.Signature=this->NTHeader.Signature;
        memcpy(&NTHeader32.FileHeader,&this->NTHeader.FileHeader,sizeof(IMAGE_FILE_HEADER));
        NTHeader32.OptionalHeader.Magic=this->NTHeader.OptionalHeader.Magic;                          
        NTHeader32.OptionalHeader.MajorLinkerVersion=this->NTHeader.OptionalHeader.MajorLinkerVersion;             
        NTHeader32.OptionalHeader.MinorLinkerVersion=this->NTHeader.OptionalHeader.MinorLinkerVersion;             
        NTHeader32.OptionalHeader.SizeOfCode=this->NTHeader.OptionalHeader.SizeOfCode;                     
        NTHeader32.OptionalHeader.SizeOfInitializedData=this->NTHeader.OptionalHeader.SizeOfInitializedData;          
        NTHeader32.OptionalHeader.SizeOfUninitializedData=this->NTHeader.OptionalHeader.SizeOfUninitializedData;        
        NTHeader32.OptionalHeader.AddressOfEntryPoint=this->NTHeader.OptionalHeader.AddressOfEntryPoint;            
        NTHeader32.OptionalHeader.BaseOfCode=this->NTHeader.OptionalHeader.BaseOfCode;
        NTHeader32.OptionalHeader.BaseOfData=this->IMAGE_NT_HEADERS32_BaseOfData;
        NTHeader32.OptionalHeader.ImageBase=(DWORD)this->NTHeader.OptionalHeader.ImageBase;                      
        NTHeader32.OptionalHeader.SectionAlignment=this->NTHeader.OptionalHeader.SectionAlignment;               
        NTHeader32.OptionalHeader.FileAlignment=this->NTHeader.OptionalHeader.FileAlignment;                  
        NTHeader32.OptionalHeader.MajorOperatingSystemVersion=this->NTHeader.OptionalHeader.MajorOperatingSystemVersion;    
        NTHeader32.OptionalHeader.MinorOperatingSystemVersion=this->NTHeader.OptionalHeader.MinorOperatingSystemVersion;    
        NTHeader32.OptionalHeader.MajorImageVersion=this->NTHeader.OptionalHeader.MajorImageVersion;              
        NTHeader32.OptionalHeader.MinorImageVersion=this->NTHeader.OptionalHeader.MinorImageVersion;              
        NTHeader32.OptionalHeader.MajorSubsystemVersion=this->NTHeader.OptionalHeader.MajorSubsystemVersion;          
        NTHeader32.OptionalHeader.MinorSubsystemVersion=this->NTHeader.OptionalHeader.MinorSubsystemVersion;          
        NTHeader32.OptionalHeader.Win32VersionValue=this->NTHeader.OptionalHeader.Win32VersionValue;              
        NTHeader32.OptionalHeader.SizeOfImage=this->NTHeader.OptionalHeader.SizeOfImage;                    
        NTHeader32.OptionalHeader.SizeOfHeaders=this->NTHeader.OptionalHeader.SizeOfHeaders;                  
        NTHeader32.OptionalHeader.CheckSum=this->NTHeader.OptionalHeader.CheckSum;                       
        NTHeader32.OptionalHeader.Subsystem=this->NTHeader.OptionalHeader.Subsystem;                      
        NTHeader32.OptionalHeader.DllCharacteristics=this->NTHeader.OptionalHeader.DllCharacteristics;             
        NTHeader32.OptionalHeader.SizeOfStackReserve=(DWORD)this->NTHeader.OptionalHeader.SizeOfStackReserve;             
        NTHeader32.OptionalHeader.SizeOfStackCommit=(DWORD)this->NTHeader.OptionalHeader.SizeOfStackCommit;              
        NTHeader32.OptionalHeader.SizeOfHeapReserve=(DWORD)this->NTHeader.OptionalHeader.SizeOfHeapReserve;              
        NTHeader32.OptionalHeader.SizeOfHeapCommit=(DWORD)this->NTHeader.OptionalHeader.SizeOfHeapCommit;               
        NTHeader32.OptionalHeader.LoaderFlags=this->NTHeader.OptionalHeader.LoaderFlags;                    
        NTHeader32.OptionalHeader.NumberOfRvaAndSizes=this->NTHeader.OptionalHeader.NumberOfRvaAndSizes;            
        memcpy(NTHeader32.OptionalHeader.DataDirectory,this->NTHeader.OptionalHeader.DataDirectory,sizeof(IMAGE_DATA_DIRECTORY)*IMAGE_NUMBEROF_DIRECTORY_ENTRIES);

        if (!WriteFile(hFile,&NTHeader32,sizeof(IMAGE_NT_HEADERS32),&dwNbBytesWritten,NULL))
        {
            this->ShowLastApiError();
            ::CloseHandle(hFile);
            bRet=FALSE;
        }
    }

    
    // close file
    CloseHandle(hFile);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SavePIMAGE_SECTION_HEADER
// Object: save PIMAGE_SECTION_HEADER to file
//         WARNING currently this func don't allow you to change the number of 
//         sections. It's just allow you to modify fields of theses sections.
//         If number of sections is changed you'll probably get an unworking EXE
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::SavePIMAGE_SECTION_HEADER()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    HANDLE hFile;
    BOOL bRet=TRUE;
    DWORD dwNbBytesWritten;
    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        this->ShowLastApiError();
        return FALSE;
    }
    // move to IMAGE_NT_HEADERS start
    SetFilePointer(hFile,this->DosHeader.e_lfanew+this->IMAGE_NT_HEADERS_Size,0,FILE_CURRENT);

    // write IMAGE_SECTION_HEADER array
    if (!WriteFile(hFile,this->pSectionHeaders,sizeof(IMAGE_SECTION_HEADER)*this->NTHeader.FileHeader.NumberOfSections,&dwNbBytesWritten,NULL))
    {
        this->ShowLastApiError();
        bRet=FALSE;
    }
    
    // close file
    CloseHandle(hFile);
    return bRet;
}
//-----------------------------------------------------------------------------
// Name: VaToRva
// Object: Translate virtual address to relative virtual one
// Parameters :
//     in  : ULONGLONG VaAddress
//     out : ULONGLONG* pRvaAddress
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::VaToRva(ULONGLONG VaAddress,ULONGLONG* pRvaAddress)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    if (IsBadWritePtr(pRvaAddress,sizeof(ULONGLONG)))
        return FALSE;
    *pRvaAddress=VaAddress-this->NTHeader.OptionalHeader.ImageBase;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RvaToVa
// Object: Translate relative virtual address to virtual one
// Parameters :
//     in  : ULONGLONG RvaAddress
//     out : ULONGLONG* pVaAddress
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::RvaToVa(ULONGLONG RvaAddress,ULONGLONG* pVaAddress)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    if (IsBadWritePtr(pVaAddress,sizeof(ULONGLONG)))
        return FALSE;
    *pVaAddress=RvaAddress+this->NTHeader.OptionalHeader.ImageBase;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RawToRva
// Object: Translate raw address to relative virtual one
// Parameters :
//     in  : ULONGLONG RawAddress
//     out : ULONGLONG* pRvaAddress
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::RawToRva(ULONGLONG RawAddress,ULONGLONG* pRvaAddress)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    if (IsBadWritePtr(pRvaAddress,sizeof(ULONGLONG)))
        return FALSE;

    for (int cnt=0;cnt<this->NTHeader.FileHeader.NumberOfSections;cnt++)
    {
        if ((this->pSectionHeaders[cnt].PointerToRawData<=RawAddress)
            && (this->pSectionHeaders[cnt].PointerToRawData+this->pSectionHeaders[cnt].SizeOfRawData>RawAddress))
        {
            *pRvaAddress=RawAddress-this->pSectionHeaders[cnt].PointerToRawData+this->pSectionHeaders[cnt].VirtualAddress;
            return TRUE;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: RawToRva
// Object: Translate relative virtual address to raw one
// Parameters :
//     in  : ULONGLONG RvaAddress
//     out : ULONGLONG* pRawAddress
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::RvaToRaw(ULONGLONG RvaAddress,ULONGLONG* pRawAddress)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }

    if (IsBadWritePtr(pRawAddress,sizeof(ULONGLONG)))
        return FALSE;

    int SectionIndex;
    if (!this->GetSectionIndexFromRva(RvaAddress,&SectionIndex))
        return FALSE;

    *pRawAddress=RvaAddress-this->pSectionHeaders[SectionIndex].VirtualAddress+this->pSectionHeaders[SectionIndex].PointerToRawData;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetUnrebasedVirtualAddress
// Object: Translate relative virtual address to raw one
// Parameters :
//     in  : ULONGLONG RebasedRelativeAddress : address in the running process from ImageBase of the module in the running process
//                                          (RebasedRelativeAddress-RebasedImageBase)
//     out : 
//     return : non rebased address on success, -1 on error
//-----------------------------------------------------------------------------
ULONGLONG CPE::GetUnrebasedVirtualAddress(ULONGLONG RebasedRelativeAddress)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return (ULONGLONG)-1;
    }
    return RebasedRelativeAddress+this->NTHeader.OptionalHeader.ImageBase;
}

//-----------------------------------------------------------------------------
// Name: Is64Bits
// Object: TRUE if 64 bits binary
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if 64 bits
//-----------------------------------------------------------------------------
BOOL CPE::Is64Bits()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }
    return ( this->NTHeader.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );
}
//-----------------------------------------------------------------------------
// Name: IsNET
// Object: TRUE if .NET assembly
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if .NET assembly
//-----------------------------------------------------------------------------
BOOL CPE::IsNET()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }
    return ( this->NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress!=0);
}

//-----------------------------------------------------------------------------
// Name: IsDynamicallyBased
// Object: TRUE if Dynamically Based (if IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE flag is present in 
//          DLL Characteristics of optional header)
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if .NET assembly
//-----------------------------------------------------------------------------
#define IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE 0x0040
BOOL CPE::IsDynamicallyBased()
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return FALSE;
    }
    return ( (this->NTHeader.OptionalHeader.DllCharacteristics & IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE) !=0 );
}

//-----------------------------------------------------------------------------
// Name: GetFileName
// Object: get file name of current parsed file
// Parameters :
//     in  : 
//     out : TCHAR* pcFilename : Name of the file being parsed (size should be MAX_PATH in TCHAR)
//     return : 
//-----------------------------------------------------------------------------
void CPE::GetFileName(TCHAR* pszFilename)
{
    _tcscpy(pszFilename, this->pcFilename);
}

//-----------------------------------------------------------------------------
// Name: GetSectionHeader
// Object: get section infos from section name
// Parameters :
//     in  : 
//     out : CHAR* SectionName : name of section we are looking at
//     return : 
//-----------------------------------------------------------------------------
IMAGE_SECTION_HEADER* CPE::GetSectionHeader(CHAR* SectionName)
{
    if (!this->bNtHeaderParsed)
    {
        if (!this->Parse())
            return NULL;
    }
    // search name through all section names
    for (SIZE_T Cnt = 0 ; Cnt<this->NTHeader.FileHeader.NumberOfSections; Cnt++)
    {
        if (stricmp((CHAR*)this->pSectionHeaders[Cnt].Name, SectionName) == 0)
            // if found return section infos
            return &this->pSectionHeaders[Cnt];
    }
    // not found return null
    return NULL;
}

BOOL CPE::RemoveCertificate(TCHAR* FileName)
{
    HMODULE hModule;
    hModule = CPE::LoadImageHelpLibrary();
    if (!hModule)
       return FALSE;

    pfImageRemoveCertificate pImageRemoveCertificate =(pfImageRemoveCertificate) ::GetProcAddress(hModule,"ImageRemoveCertificate");
    if (!pImageRemoveCertificate)
        return FALSE;

    HANDLE FileHandle; 
    FileHandle = ::CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (FileHandle==INVALID_HANDLE_VALUE)
        return FALSE;
    while(pImageRemoveCertificate(FileHandle,0)==TRUE){};

    ::CloseHandle(FileHandle);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetChecksum
// Object: return the current file checksum stored in NTHeader.OptionalHeader.CheckSum
//         and the real file checksum
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::GetChecksum(DWORD* pCurrentHeaderCheckSum,DWORD* pRealCheckSum)
{
    HMODULE hModule;
    hModule = this->LoadImageHelpLibrary();
    if (!hModule)
        return FALSE;

    pfCheckSumMappedFile pCheckSumMappedFile =(pfCheckSumMappedFile) ::GetProcAddress(hModule,"CheckSumMappedFile");
    if (!pCheckSumMappedFile)
        return FALSE;    

    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID BaseAddress;

    if (IsBadWritePtr(pCurrentHeaderCheckSum,sizeof(DWORD))
        || IsBadWritePtr(pRealCheckSum,sizeof(DWORD)))
        return FALSE;

    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        this->ShowLastApiError();
        return FALSE;
    }
    // create file mapping
    hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    if (!hFileMapping)
    {
        this->ShowLastApiError();
        CloseHandle(hFile);
        return FALSE;
    }
    // map view of file
    BaseAddress=MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);

    if (BaseAddress==NULL)
    {
        this->ShowLastApiError();
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // get checksum infos
    if (!pCheckSumMappedFile(
                        BaseAddress,
                        GetFileSize(hFile,NULL), // FileLength
                        pCurrentHeaderCheckSum,  // HeaderSum
                        pRealCheckSum            // CheckSum
                        )
       )
    {
        this->ShowLastApiError();
        UnmapViewOfFile(BaseAddress);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // unmap view of file
    UnmapViewOfFile(BaseAddress);
    // close file mapping
    CloseHandle(hFileMapping);
    // close file
    CloseHandle(hFile);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: CorrectChecksum
// Object: compute and correct the checksum in NTHeader.OptionalHeader.CheckSum
//         Warning this func make a parsing before writting checksum
//         --> DosHeader, NTHeader and pSectionHeaders information will match 
//             file content and modifications to these member will be lost
//             exept if you save them before calling this func
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPE::CorrectChecksum()
{
    DWORD dwCurrentCheckSum;
    DWORD dwRealCheckSum;
    if (!this->Parse())
        return FALSE;
    if (!this->GetChecksum(&dwCurrentCheckSum,&dwRealCheckSum))
        return FALSE;
    this->NTHeader.OptionalHeader.CheckSum=dwRealCheckSum;

    return this->SaveIMAGE_NT_HEADERS();
}

//-----------------------------------------------------------------------------
// Name: AddImports
// Object: add new imports to a file
// Parameters :
//          TCHAR* FileName : target file name
//          TCHAR* AddedSectionName : as a new section is required to add import, name of the new section
//          ADD_IMPORT_LIBRARY_DESCRIPTION* ImportLibraryArray : array of dll/function to be added to import
//          SIZE_T ImportLibraryArraySize : size of array
//          BOOL BeforeExisting : TRUE to add import before the existing ones (added dll will be loaded before the existing ones)
//                                FALSE to add import after the existing ones (added dll will be loaded after the existing ones)
//     return : FALSE on error
// 
// Example of use
//
//#define _countof(array) (sizeof(array)/sizeof(array[0]))
//CPE::ADD_IMPORT_FUNCTION_DESCRIPTION Dll1Functions[] = { {_T("Func1"),1,0,FALSE},{0,0,0x1234,TRUE} };
//CPE::ADD_IMPORT_FUNCTION_DESCRIPTION Dll2Functions[] = { {_T("Func2"),5,0,FALSE},{_T("Func3"),6,0,FALSE} };
//CPE::ADD_IMPORT_LIBRARY_DESCRIPTION Imports [] =
//{
//    { _T("MyLibrary1.dll"),  Dll1Functions,  _countof(Dll1Functions) },
//    { _T("MyLibrary2.dll"),  Dll2Functions,  _countof(Dll2Functions) },
//};
//CPE::AddImports(_T("MyBinary.exe"),_T(".MySec"), Imports,_countof(Imports),TRUE);
//
//-----------------------------------------------------------------------------
BOOL CPE::AddImports(TCHAR* FileName,TCHAR* AddedSectionName,ADD_IMPORT_LIBRARY_DESCRIPTION* ImportLibraryArray,SIZE_T ImportLibraryArraySize,BOOL BeforeExisting)
{
    BOOL bSuccess = TRUE;
    // parse header
    CPE Pe(FileName);
    if (!Pe.Parse(FALSE,FALSE,FALSE))
        return FALSE;

    // check there's enough available space in first section (containing Dos, nt, optinal nt headers and sections descriptions) to add a new section
    if (Pe.DosHeader.e_lfanew+Pe.IMAGE_NT_HEADERS_Size+(Pe.NTHeader.FileHeader.NumberOfSections+1)*sizeof(IMAGE_SECTION_HEADER)
        >Pe.NTHeader.OptionalHeader.SizeOfHeaders)
    {
        // not enough place in current section
        // TODO : to be improved by increasing current section size and adding offset on all sections
        // can be dangerous because all IDT,IAT,... content may will be affect
        return FALSE;
    }

    // add content to file
    HANDLE FileHandle; 
    FileHandle = ::CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (FileHandle==INVALID_HANDLE_VALUE)
        return FALSE;

    IMAGE_SECTION_HEADER AddedSectionHeader={0};
    IMAGE_SECTION_HEADER* pSectionHeader;
    ULONGLONG RawAddress;
    IMAGE_IMPORT_DESCRIPTOR ImportDescriptor={0};
    IMAGE_IMPORT_DESCRIPTOR* pImportDescriptor=0;
    // memory layout : Import Descriptor Table, ImportLookUpTable, ImportAddressTable, DllNameAndFunctionHintNameTable
    ULONGLONG CurrentDescriptorFilePointer;// where to write IMAGE_IMPORT_DESCRIPTOR
    ULONGLONG CurrentImportLookUpTable;// where to write import lookup table
    ULONGLONG CurrentImportAddressTable;// where to write import lookup table
    ULONGLONG CurrentDllNameAndFunctionHintNameTable;// dll names are mixed with function hint/name struct
    DWORD NbBytes;
    SIZE_T CntDll;
    SIZE_T CntFunction;
    SIZE_T SectionSize;
    SIZE_T NumberOfItemsInLookUpTable;
    SIZE_T SizeOfItemsInLookUpTable;
    CLinkList ListImportEntry(sizeof(IMAGE_IMPORT_DESCRIPTOR));
    Pe.RvaToRaw(Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,&RawAddress);
    ::SetFilePointer(FileHandle,(DWORD)RawAddress,0,FILE_BEGIN);

    // for each imported dll
    for (;;)
    {
        ::ReadFile(FileHandle,&ImportDescriptor,sizeof(IMAGE_IMPORT_DESCRIPTOR),&NbBytes,NULL);
        if (NbBytes != sizeof(IMAGE_IMPORT_DESCRIPTOR))
        {
            ::CloseHandle(FileHandle);
            return FALSE;
        }
        // See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR (no more items)
        if ( (ImportDescriptor.TimeDateStamp==0 ) && (ImportDescriptor.Name==0) )
            break;

        ListImportEntry.AddItem(&ImportDescriptor);
    }

    ULONGLONG MinRawSection = (ULONGLONG)-1;
    ULONGLONG MinVirtualSection = (ULONGLONG)-1;
    ULONGLONG MaxRawSectionEnd = 0;
    ULONGLONG MaxVirtualSectionEnd = 0;
    for (SIZE_T CntSection = 0; CntSection<Pe.NTHeader.FileHeader.NumberOfSections ;CntSection++)
    {
        // find added section virtual and raw address
        pSectionHeader = &Pe.pSectionHeaders[CntSection];
        if (pSectionHeader->VirtualAddress>0)
            MinVirtualSection = __min(MinVirtualSection,pSectionHeader->VirtualAddress);
        if (pSectionHeader->PointerToRawData>0)
            MinRawSection = __min(MinRawSection,pSectionHeader->PointerToRawData);
        MaxVirtualSectionEnd = __max(MaxVirtualSectionEnd,pSectionHeader->VirtualAddress + CPE::RoundToUpperAlignment(pSectionHeader->Misc.VirtualSize,Pe.NTHeader.OptionalHeader.SectionAlignment));
        MaxRawSectionEnd = __max(MaxRawSectionEnd,pSectionHeader->PointerToRawData + CPE::RoundToUpperAlignment(pSectionHeader->SizeOfRawData,Pe.NTHeader.OptionalHeader.FileAlignment));
    }
    AddedSectionHeader.VirtualAddress = MaxVirtualSectionEnd;
    AddedSectionHeader.PointerToRawData = MaxRawSectionEnd;

    #define AddImports_GetVA(x)(x-AddedSectionHeader.PointerToRawData+AddedSectionHeader.VirtualAddress)

    // the following should be the same as ::SetFilePointer(FileHandle,0,0,FILE_END);
    // but it's not always the case if file is not rounded to file alignment
    ::SetFilePointer(FileHandle,AddedSectionHeader.PointerToRawData,0,FILE_BEGIN);

    // old import descriptor size
    SectionSize= ListImportEntry.GetItemsCount()*sizeof(IMAGE_IMPORT_DESCRIPTOR);

    CLinkListItem* pItem;

    // if new import should be added after the old one
    if (!BeforeExisting)
    {
        // write already existing directories
        for(pItem=ListImportEntry.Head;pItem;pItem=pItem->NextItem)
        {
            pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)pItem->ItemData;
            ::WriteFile(FileHandle,pImportDescriptor,sizeof(IMAGE_IMPORT_DESCRIPTOR),&NbBytes,NULL);
        }
    }

    CurrentDescriptorFilePointer = GetFilePointer(FileHandle);
    CurrentImportLookUpTable = CurrentDescriptorFilePointer+(ImportLibraryArraySize+ListImportEntry.GetItemsCount()+1)*sizeof(IMAGE_IMPORT_DESCRIPTOR);// +1 for ending null struct
    // get number of space needed by ImportLookUpTable and ImportAddressTable
    NumberOfItemsInLookUpTable = 0;
    //for each dll
    for (CntDll=0;CntDll<ImportLibraryArraySize;CntDll++)
    {
        // for each function
        for (CntFunction=0;CntFunction<ImportLibraryArray[CntDll].FunctionDescriptionArraySize;CntFunction++)
        {
            NumberOfItemsInLookUpTable++;
        }
        // a zero item is needed to end import tables at the end of each dll import
        NumberOfItemsInLookUpTable++;
    }

    ULONGLONG OrdinalFlag;
    if (Pe.Is64Bits())
    {
        SizeOfItemsInLookUpTable = sizeof(ULONGLONG);
        OrdinalFlag = IMAGE_ORDINAL_FLAG64;
    }
    else
    {
        SizeOfItemsInLookUpTable = sizeof(DWORD);
        OrdinalFlag = IMAGE_ORDINAL_FLAG32;
    }

    CurrentImportAddressTable = CurrentImportLookUpTable + NumberOfItemsInLookUpTable*SizeOfItemsInLookUpTable;
    CurrentDllNameAndFunctionHintNameTable = CurrentImportAddressTable + NumberOfItemsInLookUpTable*SizeOfItemsInLookUpTable;

    // write new directories
    SIZE_T DllNameSize;
    SIZE_T FunctionNameSize;
    ULONGLONG Ordinal;
    CHAR* pFunctionName;
    ImportDescriptor.ForwarderChain = (DWORD)-1;
    ADD_IMPORT_FUNCTION_DESCRIPTION* pFunctionInfos;
    ULONGLONG VA;
    memset(&ImportDescriptor,0,sizeof(ImportDescriptor));
    //for each dll
    for (CntDll=0;CntDll<ImportLibraryArraySize;CntDll++)
    {
        // memory layout : Import Descriptor Table, ImportLookUpTable, ImportAddressTable, DllNameAndFunctionHintNameTable
        ImportDescriptor.Characteristics = AddImports_GetVA(CurrentImportLookUpTable);
        ImportDescriptor.FirstThunk = AddImports_GetVA(CurrentImportAddressTable);
        ImportDescriptor.Name = AddImports_GetVA(CurrentDllNameAndFunctionHintNameTable);

        ::SetFilePointer(FileHandle,CurrentDescriptorFilePointer,0,FILE_BEGIN);
        ::WriteFile(FileHandle,&ImportDescriptor,sizeof(IMAGE_IMPORT_DESCRIPTOR),&NbBytes,NULL);
        SectionSize+=sizeof(IMAGE_IMPORT_DESCRIPTOR);
        CurrentDescriptorFilePointer+=sizeof(IMAGE_IMPORT_DESCRIPTOR);

        ::SetFilePointer(FileHandle,CurrentDllNameAndFunctionHintNameTable,0,FILE_BEGIN);
        DllNameSize = _tcslen(ImportLibraryArray[CntDll].LibraryName)+1;// keep \0
#if (defined(UNICODE)||defined(_UNICODE))
        {
            CHAR TmpDllName[MAX_PATH];
            CAnsiUnicodeConvert::UnicodeToAnsi(ImportLibraryArray[CntDll].LibraryName,TmpDllName,MAX_PATH);
            ::WriteFile(FileHandle,TmpDllName,DllNameSize,&NbBytes,NULL);
        }
#else
        ::WriteFile(FileHandle,ImportLibraryArray[CntDll].LibraryName,DllNameSize,&NbBytes,NULL);
#endif
        CurrentDllNameAndFunctionHintNameTable+=DllNameSize;
        SectionSize+=DllNameSize;

        // for each function
        for (CntFunction=0;CntFunction<ImportLibraryArray[CntDll].FunctionDescriptionArraySize;CntFunction++)
        {
            // get pointer to current item
            pFunctionInfos =&ImportLibraryArray[CntDll].FunctionDescriptionArray[CntFunction]; 
            
            // if import is an ordinal import
            if (pFunctionInfos->bOrdinal)
            {
                // lookup and import table contains ordinal flag and ordinal value
                Ordinal = OrdinalFlag | pFunctionInfos->Ordinal;

                // fill lookup table
                ::SetFilePointer(FileHandle,CurrentImportLookUpTable,0,FILE_BEGIN);
                ::WriteFile(FileHandle,&Ordinal,SizeOfItemsInLookUpTable,&NbBytes,NULL);
                CurrentImportLookUpTable+=SizeOfItemsInLookUpTable;
                SectionSize+=SizeOfItemsInLookUpTable;

                // fill import table
                ::SetFilePointer(FileHandle,CurrentImportAddressTable,0,FILE_BEGIN);
                ::WriteFile(FileHandle,&Ordinal,SizeOfItemsInLookUpTable,&NbBytes,NULL);
                CurrentImportAddressTable+=SizeOfItemsInLookUpTable;
                SectionSize+=SizeOfItemsInLookUpTable;
            }
            else
            {
                // lookup and import table contain pointer to a {hint,filename}

                // fill lookup table
                ::SetFilePointer(FileHandle,CurrentImportLookUpTable,0,FILE_BEGIN);
                VA = AddImports_GetVA(CurrentDllNameAndFunctionHintNameTable);
                ::WriteFile(FileHandle,&VA,SizeOfItemsInLookUpTable,&NbBytes,NULL);
                CurrentImportLookUpTable+=SizeOfItemsInLookUpTable;
                SectionSize+=SizeOfItemsInLookUpTable;

                // fill import table
                ::SetFilePointer(FileHandle,CurrentImportAddressTable,0,FILE_BEGIN);
                VA = AddImports_GetVA(CurrentDllNameAndFunctionHintNameTable);
                ::WriteFile(FileHandle,&VA,SizeOfItemsInLookUpTable,&NbBytes,NULL);
                CurrentImportAddressTable+=SizeOfItemsInLookUpTable;
                SectionSize+=SizeOfItemsInLookUpTable;

                // fill FunctionHintName
                ::SetFilePointer(FileHandle,CurrentDllNameAndFunctionHintNameTable,0,FILE_BEGIN);

                // write hint
                ::WriteFile(FileHandle,&pFunctionInfos->Hint,sizeof(WORD),&NbBytes,NULL);
                CurrentDllNameAndFunctionHintNameTable+=sizeof(WORD);
                SectionSize+=sizeof(WORD);

                // write function name
                FunctionNameSize = _tcslen(pFunctionInfos->FunctionName)+1;
                // Warning : string must be rounded on 2 bytes
                // assume to be aligned on word (required by pe format)
                if (FunctionNameSize%2 !=0 )
                    FunctionNameSize++;
                pFunctionName = new CHAR[FunctionNameSize];
                pFunctionName[FunctionNameSize-1]=0;
#if (defined(UNICODE)||defined(_UNICODE))
                CAnsiUnicodeConvert::UnicodeToAnsi(pFunctionInfos->FunctionName,pFunctionName,FunctionNameSize);
#else
                strcpy(pFunctionName,pFunctionInfos->FunctionName);
#endif
                ::WriteFile(FileHandle,pFunctionName,FunctionNameSize,&NbBytes,NULL);
                CurrentDllNameAndFunctionHintNameTable+=FunctionNameSize;
                SectionSize+=FunctionNameSize;
                delete[] pFunctionName;
            }
        }

        ////////////////////
        // fill look up and import table with an empty field to finish dll import functions array
        ////////////////////

        // fill lookup table
        Ordinal = 0;
        ::SetFilePointer(FileHandle,CurrentImportLookUpTable,0,FILE_BEGIN);
        ::WriteFile(FileHandle,&Ordinal,SizeOfItemsInLookUpTable,&NbBytes,NULL);
        CurrentImportLookUpTable+=SizeOfItemsInLookUpTable;
        SectionSize+=SizeOfItemsInLookUpTable;

        // fill import table
        ::SetFilePointer(FileHandle,CurrentImportAddressTable,0,FILE_BEGIN);
        ::WriteFile(FileHandle,&Ordinal,SizeOfItemsInLookUpTable,&NbBytes,NULL);
        CurrentImportAddressTable+=SizeOfItemsInLookUpTable;
        SectionSize+=SizeOfItemsInLookUpTable;
    }

    // if old import should be add after the new one
    if (BeforeExisting)
    {
        ::SetFilePointer(FileHandle,CurrentDescriptorFilePointer,0,FILE_BEGIN);
        // write already existing directories
        for(pItem=ListImportEntry.Head;pItem;pItem=pItem->NextItem)
        {
            pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)pItem->ItemData;
            ::WriteFile(FileHandle,pImportDescriptor,sizeof(IMAGE_IMPORT_DESCRIPTOR),&NbBytes,NULL);
            CurrentDescriptorFilePointer+=sizeof(IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    // write an empty import descriptor to end import descriptor array
    memset(&ImportDescriptor,0,sizeof(IMAGE_IMPORT_DESCRIPTOR));
    ::SetFilePointer(FileHandle,CurrentDescriptorFilePointer,0,FILE_BEGIN);
    ::WriteFile(FileHandle,&ImportDescriptor,sizeof(IMAGE_IMPORT_DESCRIPTOR),&NbBytes,NULL);

    // take the max of file/memory alignment to get only 1 size
    AddedSectionHeader.SizeOfRawData = CPE::RoundToUpperAlignment(SectionSize,Pe.NTHeader.OptionalHeader.FileAlignment);
    AddedSectionHeader.Misc.VirtualSize = SectionSize;
    AddedSectionHeader.Characteristics = IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;

#if (defined(UNICODE)||defined(_UNICODE))
    {
        CHAR TmpStr[IMAGE_SIZEOF_SHORT_NAME+1];
        CAnsiUnicodeConvert::UnicodeToAnsi(AddedSectionName,TmpStr,IMAGE_SIZEOF_SHORT_NAME+1);
        memcpy(AddedSectionHeader.Name,TmpStr,IMAGE_SIZEOF_SHORT_NAME);
    }
#else
    memcpy(AddedSectionHeader.Name,AddedSectionName,IMAGE_SIZEOF_SHORT_NAME);
#endif


    // write new section header
    SIZE_T SectionPosition = Pe.DosHeader.e_lfanew+Pe.IMAGE_NT_HEADERS_Size+Pe.NTHeader.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
    SIZE_T StartOfRemainingData = SectionPosition + sizeof(IMAGE_SECTION_HEADER);
    SIZE_T NextRemainingData = StartOfRemainingData;
    // with have to check Pe.NTHeader.OptionalHeader.SizeOfHeaders against MinRawSection because Pe.NTHeader.OptionalHeader.SizeOfHeaders is not correct somtimes
    SIZE_T RemainingDataSize = __min(MinRawSection ,Pe.NTHeader.OptionalHeader.SizeOfHeaders ) - SectionPosition;
    PBYTE RemainingData = new BYTE[RemainingDataSize];
    ::SetFilePointer(FileHandle,SectionPosition,0,FILE_BEGIN);
    ::ReadFile(FileHandle,RemainingData,RemainingDataSize,&NbBytes,NULL);
    RemainingDataSize = RemainingDataSize - sizeof(IMAGE_SECTION_HEADER);

    ::SetFilePointer(FileHandle,StartOfRemainingData,0,FILE_BEGIN);
    // in case data directory store data in header (like bound import)
    for (SIZE_T Cnt =0 ;Cnt < CPE_MAX_IMAGE_DIRECTORY_ENTRY;Cnt++)
    {
        if (Cnt == IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT) //  don't care of IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT as we will remove it
            continue;
        if (Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].VirtualAddress == 0)
            continue;
        if (Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].VirtualAddress>=MinVirtualSection)
            continue;

        if (Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].Size < RemainingDataSize)
        {
            ::WriteFile(FileHandle,
                        &RemainingData[Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].VirtualAddress-SectionPosition],
                        Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].Size,
                        &NbBytes,
                        NULL
                        );

            Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].VirtualAddress = NextRemainingData;
            NextRemainingData += Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].Size;
            RemainingDataSize -= Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].Size;
        }
        else
        {
#ifdef _DEBUG
            if (::IsDebuggerPresent())
                ::DebugBreak();
#endif
            Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].VirtualAddress = 0;
            Pe.NTHeader.OptionalHeader.DataDirectory[Cnt].Size = 0;
            bSuccess = FALSE;
        }


    }
    delete[] RemainingData;

    // fill end of header with 0
    RemainingData = new BYTE[RemainingDataSize];
    memset(RemainingData,0,RemainingDataSize);
    ::WriteFile(FileHandle,RemainingData,RemainingDataSize,&NbBytes,NULL);
    delete[] RemainingData;

    // write new section informations at the end of section table
    ::SetFilePointer(FileHandle,SectionPosition,0,FILE_BEGIN);
    ::WriteFile(FileHandle,&AddedSectionHeader,sizeof(AddedSectionHeader),&NbBytes,NULL);

    // fill the remaining of raw section with 0
    ::SetFilePointer(FileHandle,CurrentDllNameAndFunctionHintNameTable,0,FILE_BEGIN);
    SIZE_T EmptySizeToFill = AddedSectionHeader.SizeOfRawData - (CurrentDllNameAndFunctionHintNameTable - AddedSectionHeader.PointerToRawData);
    PBYTE Empty = new BYTE[EmptySizeToFill];
    memset(Empty,0,EmptySizeToFill);
    ::WriteFile(FileHandle,Empty,EmptySizeToFill,&NbBytes,NULL);
    delete[] Empty;

    // close file handle
    ::CloseHandle(FileHandle);

    // update NT headers
    Pe.NTHeader.FileHeader.NumberOfSections++;
    Pe.NTHeader.OptionalHeader.SizeOfImage+=AddedSectionHeader.SizeOfRawData;
    //if (SectionHeader.Characteristics & IMAGE_SCN_CNT_CODE)
    //    Pe.NTHeader.OptionalHeader.SizeOfCode+=SectionHeader.SizeOfRawData;
    //else
        Pe.NTHeader.OptionalHeader.SizeOfInitializedData+=AddedSectionHeader.SizeOfRawData;

    // change content of IMAGE_DIRECTORY_ENTRY_IMPORT
    Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = AddedSectionHeader.VirtualAddress;
    // size : (number of imported dll + 1)*sizeof(IMAGE_IMPORT_DESCRIPTOR)    // +1 for the ending array null import descriptor
    Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (ListImportEntry.GetItemsCount() + ImportLibraryArraySize + 1)*sizeof(IMAGE_IMPORT_DESCRIPTOR);

    // small trick to avoid to be disturb by bound softwares : simply trash bound informations
    if (Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress)
    {
        Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
        Pe.NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
    }


    // save NT headers
    bSuccess = bSuccess && Pe.SaveIMAGE_NT_HEADERS();

    // update file checksum and remove certificates if any
    if (Pe.NTHeader.OptionalHeader.CheckSum !=0)
        bSuccess = bSuccess && Pe.CorrectChecksum();
    bSuccess = bSuccess && CPE::RemoveCertificate(FileName);

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: RoundToUpperAlignment
// Object: get required size rounded to alignment
// Parameters :
//          SIZE_T Size
//          SIZE_T Alignment
//     return : required aligned size
//-----------------------------------------------------------------------------
SIZE_T CPE::RoundToUpperAlignment(SIZE_T Size,SIZE_T Alignment)
{
    SIZE_T NbPages = Size/Alignment;
    if ( Size - ( NbPages * Alignment ) !=0 )
        NbPages++;

    return NbPages*Alignment;
}

//-----------------------------------------------------------------------------
// Name: LoadImageHelpLibrary
// Object: load image help library (avoid to be statically linked to it)
// Parameters :
//     return : Imagehlp.dll module handle
//-----------------------------------------------------------------------------
HMODULE CPE::LoadImageHelpLibrary()
{
    HMODULE hModule;
    hModule = ::GetModuleHandle(_T("Imagehlp.dll"));
    if (!hModule)
        hModule = ::LoadLibrary(_T("Imagehlp.dll"));

    return hModule;
}
