/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>

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

// DOCUMENTATION 
// extract of Geoff Chappell - Software Analyst / The API Set Schema 
//
// Windows 7 brings a significant reorganisation of the lower levels of the Win32 subsystem. Long-familiar ADVAPI32 functions are moved to KERNEL32 and from both to a new DLL named KERNELBASE. Other ADVAPI32 functions are moved to a new DLL named SECHOST. Very many executables in Windows 7 import functions from new DLLs that have unusually long names such as “API-MS-Win-Core-LocalRegistry-L1-1-0.dll”. This importing is done by ADVAPI32 and KERNEL32, by DLLs for general support such as MFC42, MSVCRT and OLE32, by many services, and by all sorts of other executables all through the lower levels of Windows out as far as SHELL32 and SHLWAPI. 
//    There is not much official documentation of this. The original Software Development Kit (SDK) for Windows 7 makes just the one mention of KERNELBASE, in a brief page about  New Low-Level Binaries. If not much more ever is documented about it, then in one sense there should not be much surprise. After all, higher-level executables distributed with Windows continue to import as before from such DLLs as KERNEL32, and since the SDK has no import libraries for the new DLLs, the intention is surely that programs written outside Microsoft, and probably also most that are written inside Microsoft, will know nothing of the new DLLs and should be unaffected. The new DLLs with the long names are anyway just stubs in which all exported functions are implemented no more than needed for hard-coded failure. Moreover, these failing implementations have not all received great care: see for instance that CreateFileW in API-MS-Win-Core-File-L1-1-0.dll returns a hard-coded NULL (0) instead of INVALID_HANDLE_VALUE (-1). 
//    In another sense, the lack of documentation may astonish, depending on what one expects to be told about the Windows architecture in order to assess its security or robustness. These new DLLs are part of a small but significant embellishment of how NTDLL resolves imports when loading user-mode modules. It turns out that all imports from any DLL whose (case-insensitive) module name starts with API- are subject to a new form of redirection. Since very many Windows executables import from such modules and especially since KERNEL32 and ADVAPI32 do so for the initial handling of several hundred of the most commonly used Windows API functions, software that can interfere with this new redirection could be very powerful in terms of modifying behaviour throughout Windows for relatively little effort. 
//    This new redirection is managed by NTDLL as a preferred alternative to isolation through activation contexts. Whether the imports from any particular API- module are redirected depends entirely on the contents of a new file, named ApiSetSchema.dll in the System32 directory. Although ApiSetSchema.dll is a DLL, it is wanted only for data. The whole file is mapped into system space by the NT kernel during phase 1 of system initialisation. From there, the wanted data is mapped into the user-mode space of each newly initialised process and a pointer to this data is placed in a new member, named ApiSetMap at offset 0x38, of the process’s semi-documented PEB structure. The kernel recognises the data only as the whole contents of a section that is named “.apiset” and is aligned to 64KB (i.e., whose VirtualAddress member in the IMAGE_SECTION_HEADER has the low 16 bits clear). The kernel has nothing to do with interpreting these contents: it just provides them for NTDLL to interpret. Conversely, NTDLL knows nothing of where the contents came from. To NTDLL, whenever it is to resolve an import to one module from another, whatever is at the address given by ApiSetMap is accepted as a map, in an assumed format (described below), for importing from somewhere else instead. 
//
//    DATA FORMAT 
//    The map begins with an 8-byte header which is followed immediately by an array of 12-byte entries which each describe one API Set module: 
//
//Offset  Size  Description  
//    0x00  dword  ignored, but perhaps intended as a version number  
//    0x04  dword  number of API Set modules  
//    0x08  unsized  array of entries for API Set modules  
//
//    The entries are assumed to be already sorted in case-insensitive alphabetical order. Each API Set module is named without the API- prefix and without a file extension: 
//
//Offset  Size  Description  
//    0x00  dword  offset from start of map to name of API Set module  
//    0x04  word  size, in bytes, of name of API Set module  
//    0x08  dword  offset from start of map to description of host modules  
//
//    Names are in Unicode and are not null-terminated. If the module to be imported from is an API Set module as found in the schema, then the import is redirected to some host module. The list of possible hosts is assumed to contain at least one to use by default. However, there may be exceptions, which are selected according to the name of the importing module. Each host is described by a 0x10-byte structure. The possible hosts are described by a structure that begins with a count and a description of the default host, optionally followed by descriptions of the exceptional hosts. Entries for the exceptional hosts are assumed to be already sorted in case-insensitive alphabetical order of the importing module. 
//
//    Offset  Size  Description  
//    0x00  dword  number of hosts  
//    0x04  unsized  array of entries for hosts, default host first, then exceptional hosts  
//
//    The 0x10-byte structure that describes any one host provides two case-insensitive names, one for the importing module that selects this host and one for the host. 
//
//    Offset  Size  Description  
//    0x00  dword  offset from start of map to name of importing module, in Unicode  
//    0x04  word  size, in bytes, of name of importing module  
//    0x08  dword  offset from start of map to name of host module, in Unicode  
//    0x0C  word  size, in bytes, of name of host module  
//
//    Both names are in Unicode and are not null-terminated. For a default host, the members that describe the importing module are irrelevant




#include "DllStub.h"
#include "../Version/OsVersion.h"
#include "../File/StdFileOperations.h"
#include "../PE/PE.h"
#include "../String/AnsiUnicodeConvert.h"

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <shlobj.h>

#define APISETSCHEMA_DLL _T("apisetschema.dll")
#define APISET_SECTION_NAME ".apiset" // must be ansi string

CDllStub::CDllStub(void)
{
    this->pApiSetModule = NULL;

    COsVersion Version;
    // only seven or newer have stub dll
    bOsHasNoStub  = !Version.IsSevenOrNewer();
    if (bOsHasNoStub)
        return;

    // forge apisetschema.dll path
    TCHAR Path[MAX_PATH];
    *Path=0;
    if (!::GetSystemDirectory(Path,MAX_PATH))
    {
        bError = TRUE;
        return;
    }

    _tcscat(Path,_T("\\"));
    _tcscat(Path,APISETSCHEMA_DLL);

    bError = !this->Parse(Path);
}

CDllStub::~CDllStub(void)
{
    this->FreeMemory();
}

BOOL CDllStub::FreeMemory()
{
    if (this->pApiSetModule)
    {
        CLinkListItem* pItem;
        for (pItem = this->pApiSetModule->Head;pItem;pItem=pItem->NextItem)
        {
            API_SET_MODULE_ENTRY_EX* pApiSetModuleEntryEx = (API_SET_MODULE_ENTRY_EX*)pItem->ItemData;
            delete pApiSetModuleEntryEx->pHostModuleEntry;
        }
        delete this->pApiSetModule;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse content of the apisetschema.dll according to documentation present at the top of this file
// Parameters :
//     in : TCHAR* ApiSetSchemaPath : apisetschema.dll path
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CDllStub::Parse(TCHAR* ApiSetSchemaPath)
{
    // check if apisetschema.dll file exists
    if (!CStdFileOperations::DoesFileExists(ApiSetSchemaPath))
        return FALSE;

    CPE ApiSetSchemaPE(ApiSetSchemaPath);
    IMAGE_SECTION_HEADER* pApiSetSection = ApiSetSchemaPE.GetSectionHeader(APISET_SECTION_NAME);
    if (!pApiSetSection)
        return FALSE;
    
    
    // read apiset section
    HANDLE hFile = ::CreateFile(ApiSetSchemaPath, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD Size = pApiSetSection->SizeOfRawData;
    PBYTE Buffer = new BYTE[Size];
    DWORD ReadSize = 0;
    ::SetFilePointer(hFile,pApiSetSection->PointerToRawData,0,FILE_BEGIN);
    if (!::ReadFile(hFile,Buffer,Size,&ReadSize,NULL))
    {
        ::CloseHandle(hFile);
        delete[] Buffer;
        return FALSE;
    }
    ::CloseHandle(hFile);
    


    // decode apiset section
    API_SET_HEADER* pApiSetHeader;
    pApiSetHeader = (API_SET_HEADER*)Buffer;
    this->FreeMemory();
    this->pApiSetModule = new CLinkList(sizeof(API_SET_MODULE_ENTRY_EX));

    
    SIZE_T CntSetModules;
    SIZE_T NumberOfHosts;
    SIZE_T CntHost;
    SIZE_T NameSizeInTCHARCount;
    API_SET_MODULE_ENTRY* pApiSetModuleEntry;
    API_SET_MODULE_ENTRY_EX ApiSetModuleEntryEx;
    HOST_MODULE_ENTRY* pHostModuleEntry;
    HOST_MODULE_ENTRY_EX HostModuleEntryEx;    

    // parse module entries array
    pApiSetModuleEntry = (API_SET_MODULE_ENTRY*)(Buffer + sizeof(API_SET_HEADER));
    for (CntSetModules = 0; CntSetModules<pApiSetHeader->NumberOfApiSetModules;CntSetModules++,pApiSetModuleEntry++)
    {
        ApiSetModuleEntryEx.ModuleEntry = *pApiSetModuleEntry;
        NameSizeInTCHARCount = __min(pApiSetModuleEntry->NameSize/sizeof(WCHAR),MAX_PATH-1);
#if (defined(UNICODE)||defined(_UNICODE))
        wcsncpy(ApiSetModuleEntryEx.Name,(WCHAR*)(Buffer+pApiSetModuleEntry->NameOffset),NameSizeInTCHARCount);
        // assume name to be terminated
        ApiSetModuleEntryEx.Name[NameSizeInTCHARCount] = 0;
#else
        CAnsiUnicodeConvert::UnicodeToAnsi((WCHAR*)(Buffer+pApiSetModuleEntry->NameOffset),ApiSetModuleEntryEx.Name,NameSizeInTCHARCount+1); // NameSizeInTCHARCount+1 because last param is buffer size
        // assume name to be terminated
        ApiSetModuleEntryEx.Name[NameSizeInTCHARCount] = 0;
#endif
        
        ApiSetModuleEntryEx.pHostModuleEntry = new CLinkList(sizeof(HOST_MODULE_ENTRY_EX));
        this->pApiSetModule->AddItem(&ApiSetModuleEntryEx);

        // parse host module entries array
        NumberOfHosts = *(DWORD*)(Buffer+pApiSetModuleEntry->HostModulesDescriptionOffset);
        pHostModuleEntry = (HOST_MODULE_ENTRY*)(Buffer+pApiSetModuleEntry->HostModulesDescriptionOffset + sizeof(HOST_MODULES_DESCRIPTION));
        for (CntHost = 0; CntHost<NumberOfHosts; CntHost++,pHostModuleEntry++)
        {
            HostModuleEntryEx.HostModuleEntry = *pHostModuleEntry;

#if (defined(UNICODE)||defined(_UNICODE))
            NameSizeInTCHARCount = __min(pHostModuleEntry->ImportingModuleNameSize/sizeof(WCHAR),MAX_PATH-1);
            wcsncpy(HostModuleEntryEx.ImportingModuleName,(WCHAR*)(Buffer+pHostModuleEntry->ImportingModuleNameOffset),NameSizeInTCHARCount);
            // assume name to be terminated
            HostModuleEntryEx.ImportingModuleName[NameSizeInTCHARCount] = 0;

            NameSizeInTCHARCount = __min(pHostModuleEntry->HostModuleNameSize/sizeof(WCHAR),MAX_PATH-1);
            wcsncpy(HostModuleEntryEx.HostModuleName,(WCHAR*)(Buffer+pHostModuleEntry->HostModuleNameOffset),NameSizeInTCHARCount);
            // assume name to be terminated
            HostModuleEntryEx.HostModuleName[NameSizeInTCHARCount] = 0;
            
#else
            NameSizeInTCHARCount = __min(pHostModuleEntry->ImportingModuleNameSize/sizeof(WCHAR),MAX_PATH-1);
            CAnsiUnicodeConvert::UnicodeToAnsi((WCHAR*)(Buffer+pHostModuleEntry->ImportingModuleNameOffset),HostModuleEntryEx.ImportingModuleName,NameSizeInTCHARCount+1);// NameSizeInTCHARCount+1 because last param is buffer size
            // assume name to be terminated
            HostModuleEntryEx.ImportingModuleName[NameSizeInTCHARCount] = 0;

            NameSizeInTCHARCount = __min(pHostModuleEntry->HostModuleNameSize/sizeof(WCHAR),MAX_PATH-1);
            CAnsiUnicodeConvert::UnicodeToAnsi((WCHAR*)(Buffer+pHostModuleEntry->HostModuleNameOffset),HostModuleEntryEx.HostModuleName,NameSizeInTCHARCount+1);// NameSizeInTCHARCount+1 because last param is buffer size
            // assume name to be terminated
            HostModuleEntryEx.HostModuleName[NameSizeInTCHARCount] = 0;
#endif


            ApiSetModuleEntryEx.pHostModuleEntry->AddItem(&HostModuleEntryEx);
        }
    }

    delete[] Buffer;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsStubDll
// Object: check if dll is a stub dll
// Parameters :
//     in : TCHAR* DllName : dll to check
// Return : TRUE if dll is a stub dll, FALSE else
//-----------------------------------------------------------------------------
BOOL CDllStub::IsStubDll(IN TCHAR* DllName)
{
    return (this->GetStubDllInfos(DllName)!=NULL);
}

//-----------------------------------------------------------------------------
// Name: GetStubDllInfos
// Object: get informations relative to a stub dll
// Parameters :
//     in : TCHAR* DllName : stub dll name (not full path, dll name only with or without the .dll extansion)
// Return : API_SET_MODULE_ENTRY_EX* if dll is a stub dll, NULL else
//-----------------------------------------------------------------------------
CDllStub::API_SET_MODULE_ENTRY_EX* CDllStub::GetStubDllInfos(IN TCHAR* DllName)
{
    TCHAR LocalBuffer[MAX_PATH];
    TCHAR* ShortName;
    TCHAR* psz;
    
    if(this->bOsHasNoStub || this->bError)
        return NULL;

    // assume ShortName doesn't contain ".dll"
    ShortName = DllName;
    psz=_tcsrchr(DllName,'.');
    if (psz)
    {
        _tcscpy(LocalBuffer,DllName);
        _tcslwr(LocalBuffer);
        psz = _tcsstr(LocalBuffer,_T(".dll"));
        if (psz)
        {
            *psz = 0;
            ShortName = LocalBuffer;
        }
    }

    if ( _tcslen(ShortName)<=4 )// if ( _tcslen(ShortName)<=_tcslen( _T("API-") ) )
        return NULL;

    if (_tcsnicmp(ShortName,_T("API-"),4) != 0) // if (_tcsnicmp(ShortName,_T("API-"), _tcslen( _T("API-") ) ) != 0) 
        return NULL;

    // point after "API-" prefix
    ShortName+=4; //ShortName+=_tcslen( _T("API-") );

    CLinkListItem* pItem;
    API_SET_MODULE_ENTRY_EX* pApiSetModuleEntryEx;
    // loop through list of api set module to find dll
    for (pItem = this->pApiSetModule->Head; pItem; pItem=pItem->NextItem)
    {
        pApiSetModuleEntryEx = (API_SET_MODULE_ENTRY_EX*) pItem->ItemData;
        if (_tcsicmp(pApiSetModuleEntryEx->Name,ShortName) == 0)
            return pApiSetModuleEntryEx;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: GetRealModuleNameFromStubName
// Object: Get the real forwarded module name instead of the stub dll name
// Parameters :
//      TCHAR* CurrentFileName : Name of file we are currently parsing export table ( == ImportingModuleName )
//      TCHAR* StubDllName : Export Table Forwarded Dll Name : Name of module physicaly present in export table
//                           or Import Table module name
//      OUT TCHAR* RealDllName : Name that should be in export table or import table if m$ stub shit wouldn't exists, with the ".dll" extension 
//                               MUST HAVE A LENGTH GREATER OR EQUAL TO MAX_PATH
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDllStub::GetRealModuleNameFromStubName(IN TCHAR* CurrentFileName,IN TCHAR* StubDllName,OUT TCHAR* RealDllName)
{
    // default return
    *RealDllName=0;

    API_SET_MODULE_ENTRY_EX* pApiSetModuleEntryEx;
    pApiSetModuleEntryEx = this->GetStubDllInfos(StubDllName);
    if (!pApiSetModuleEntryEx)
        return FALSE;

    CLinkListItem* pItem;
    HOST_MODULE_ENTRY_EX* pHostModuleEntryEx;
    TCHAR* DefaultHostModuleName;
    TCHAR* ShortCurrentFileName;

    ShortCurrentFileName = CStdFileOperations::GetFileName(CurrentFileName);
    DefaultHostModuleName = NULL;
    // search through module entry list 
    for (pItem = pApiSetModuleEntryEx->pHostModuleEntry->Head; pItem; pItem=pItem->NextItem)
    {
        pHostModuleEntryEx = (HOST_MODULE_ENTRY_EX*) pItem->ItemData;
        // if ImportingModuleName, HostModuleName is the default one
        if (*pHostModuleEntryEx->ImportingModuleName == 0)
        {
            DefaultHostModuleName = pHostModuleEntryEx->HostModuleName;
            continue;
        }
        // if ImportingModuleName is equal to CurrentFileName, we have found HostModuleName
        if (_tcsicmp(pHostModuleEntryEx->ImportingModuleName,ShortCurrentFileName) == 0)
        {
            _tcscpy(RealDllName,pHostModuleEntryEx->HostModuleName);
            return TRUE;
        }
    }

    // no matching ImportingModuleName --> use DefaultHostModuleName
    if (DefaultHostModuleName)
    {
        _tcscpy(RealDllName,DefaultHostModuleName);
        return TRUE;
    }
    else
        return FALSE;
}