#include "ModuleIntegrityChecking.h"
#include "../Tools/LinkList/SingleThreaded/LinkListTemplateSingleThreaded.cpp"
#include "../Tools/File/StdFileOperations.h"

CModuleIntegrityChecking::CModuleIntegrityChecking(void)
{
    this->OriginalFile=NULL;
    this->hFileOriginalFile=NULL;

    this->DllBaseAddress=NULL;
    this->DllSizeInMemory=0;
    *this->DllPath=0;
    this->DllLocalCopy=NULL;

    *this->DllName=0;
    this->ProcessId=0;

    this->pSectionsChangesLinkList=new CLinkListSimple();
}

void CModuleIntegrityChecking::ClearChangesLinkList()
{
    CLinkListItem* pSectionItem;
    CLinkListItem* pItem;
    CModuleIntegrityCheckingChanges* pChange;
    CModuleIntegrityCheckingSectionChanges* pSectionChange;

    this->pSectionsChangesLinkList->Lock();
    // for each section
    for (pSectionItem=this->pSectionsChangesLinkList->Head;pSectionItem;pSectionItem=pSectionItem->NextItem)
    {
        pSectionChange=(CModuleIntegrityCheckingSectionChanges*)pSectionItem->ItemData;

        pSectionChange->pChangesLinkList->Lock();
        // for each change of the section
        for (pItem=pSectionChange->pChangesLinkList->Head;pItem;pItem=pItem->NextItem)
        {
            pChange=(CModuleIntegrityCheckingChanges*)pItem->ItemData;
            if (pChange->FileBuffer)
                delete pChange->FileBuffer;

            if (pChange->MemoryBuffer)
                delete pChange->MemoryBuffer;

            delete pChange;
        }
        pSectionChange->pChangesLinkList->Unlock();
        delete pSectionChange;
    }

    this->pSectionsChangesLinkList->RemoveAllItems(TRUE);
    this->pSectionsChangesLinkList->Unlock();
}


CModuleIntegrityChecking::~CModuleIntegrityChecking(void)
{
    this->ClearChangesLinkList();
    delete this->pSectionsChangesLinkList;
}

PBYTE CModuleIntegrityChecking::GetModuleBaseAddress()
{
    return this->DllBaseAddress;
}
TCHAR* CModuleIntegrityChecking::GetModulePath()
{
    return this->DllPath;
}
DWORD CModuleIntegrityChecking::GetProcessId()
{
    return this->ProcessId;
}
BOOL CModuleIntegrityChecking::CheckIntegrity(DWORD ProcessId,TCHAR* DllName,BOOL CheckOnlyCodeAndExecutableSections,BOOL DontCheckWritableSections,BOOL ShowRebasing,DWORD MinMatchingSizeAfterReplacement)
{
    BOOL bRet;

    this->ClearChangesLinkList();

    this->ProcessId=ProcessId;
    _tcsncpy(this->DllName,DllName,MAX_PATH);
    this->DllNameContainsFullPath=(_tcschr(this->DllName,'\\')!=NULL);

    if (!this->GetDllInformations())
        return FALSE;

    if (!this->OpenFileModule())
        return FALSE;

    if (!this->MakeLocalCopyOfDll())
        return FALSE;

    bRet=this->CheckIntegrity(CheckOnlyCodeAndExecutableSections,DontCheckWritableSections,ShowRebasing,MinMatchingSizeAfterReplacement);

    this->CloseFileModule();
    delete this->DllLocalCopy;

    return bRet;
}

BOOL CModuleIntegrityChecking::ModuleFoundCallbackStatic(MODULEENTRY* pModuleEntry,PVOID UserParam)
{
    return ((CModuleIntegrityChecking*)UserParam)->ModuleFoundCallback(pModuleEntry);
}

BOOL CModuleIntegrityChecking::ModuleFoundCallback(MODULEENTRY* pModuleEntry)
{
    BOOL bFound=FALSE;
    TCHAR szPath[MAX_PATH];
    TCHAR* ModuleFullPath;

    // check for \??\ flags
    if (_tcsnicmp(pModuleEntry->szExePath,_T("\\??\\"),4)==0)
        ModuleFullPath=&pModuleEntry->szExePath[4];

    // check for \SystemRoot\ flag
    else if(_tcsnicmp(pModuleEntry->szExePath,_T("\\SystemRoot\\"),12)==0)
    {
        SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,szPath);
        _tcscat(szPath,&pModuleEntry->szExePath[11]);
        ModuleFullPath=szPath;
    }
    else
        ModuleFullPath=pModuleEntry->szExePath;

    if (this->DllNameContainsFullPath)
        bFound=(_tcscmp(ModuleFullPath,this->DllName)==0);
    else
        bFound=(_tcscmp(ModuleFullPath,this->DllName)==0);

    // if module has been found
    if (bFound)    
    {
        // fill needed informations (path, base address and size)
        _tcscpy(this->DllPath,ModuleFullPath);

        this->DllBaseAddress=pModuleEntry->modBaseAddr;
        this->DllSizeInMemory=pModuleEntry->modBaseSize;

        // continue to fill module list
        // // module found, stop parsing module list
        // return FALSE;
    }
    this->ProcessModuleList.AddItem(new MODULEENTRY(*pModuleEntry));// make a local copy to keep data
    return TRUE;
}

BOOL CModuleIntegrityChecking::GetDllInformations()
{
    *this->DllPath=0;
    this->ProcessModuleList.RemoveAllItems();
    if (!CModulesParser::Parse(this->ProcessId,CModuleIntegrityChecking::ModuleFoundCallbackStatic,this))
        return FALSE;
    // if module not found
    if (*this->DllPath==0)
        return FALSE;

    return TRUE;
}

// store in current process memory, the other process loaded dll address space
BOOL CModuleIntegrityChecking::MakeLocalCopyOfDll()
{
    CProcessMemory pm(this->ProcessId,TRUE,TRUE);

    this->DllLocalCopy=new BYTE[this->DllSizeInMemory];
    if (!this->DllLocalCopy)
        return FALSE;

    SIZE_T ReadSize=0;
    if (!pm.Read(this->DllBaseAddress,this->DllLocalCopy,(SIZE_T)this->DllSizeInMemory,&ReadSize))
        return FALSE;

    return TRUE;
}

BOOL CModuleIntegrityChecking::OpenFileModule()
{
    // Do not use file mapping has this->OriginalFile buffer may will be changed due to IAT rebasing

    // open file
    this->hFileOriginalFile = CreateFile(this->DllPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (this->hFileOriginalFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    SIZE_T FileSize = ::GetFileSize(this->hFileOriginalFile,NULL);
    SIZE_T ReadBytes =0;
    if ( FileSize == INVALID_FILE_SIZE)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    this->OriginalFile = new BYTE[FileSize];
    if (!this->OriginalFile)
    {
        TCHAR szMsg[2*MAX_PATH];
        _stprintf(szMsg,_T("Error allocating memory for %s"),this->DllPath);
        ::MessageBox(NULL,szMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    if (!::ReadFile(this->hFileOriginalFile,this->OriginalFile,FileSize,&ReadBytes,NULL))
    {
        CAPIError::ShowLastError();
        delete[] this->OriginalFile;
        this->OriginalFile=NULL;
        return FALSE;
    }

    return TRUE;
}

BOOL CModuleIntegrityChecking::CloseFileModule()
{
    // delete buffer
    if (this->OriginalFile)
    {
        delete[] this->OriginalFile;
        this->OriginalFile=NULL;
    }

    // close file
    if (this->hFileOriginalFile)
    {
        CloseHandle(this->hFileOriginalFile);
        this->hFileOriginalFile=NULL;
    }

    return TRUE;
}

BOOL CModuleIntegrityChecking::RestoreFullModuleIntegrity()
{
    BOOL bRet=TRUE;
    CLinkListItem* pSectionItem;
    CLinkListItem* pItem;
    CModuleIntegrityCheckingChanges* pChange;
    CModuleIntegrityCheckingSectionChanges* pSectionChange;

    this->pSectionsChangesLinkList->Lock();
    // for each section
    for (pSectionItem=this->pSectionsChangesLinkList->Head;pSectionItem;pSectionItem=pSectionItem->NextItem)
    {
        pSectionChange=(CModuleIntegrityCheckingSectionChanges*)pSectionItem->ItemData;

        pSectionChange->pChangesLinkList->Lock();
        // for each change of the section
        for (pItem=pSectionChange->pChangesLinkList->Head;pItem;pItem=pItem->NextItem)
        {
            pChange=(CModuleIntegrityCheckingChanges*)pItem->ItemData;
            // restore integrity
            bRet&=this->RestoreIntegrity(pChange);
        }
        pSectionChange->pChangesLinkList->Unlock();
    }
    this->pSectionsChangesLinkList->Unlock();
    return bRet;
}

BOOL CModuleIntegrityChecking::RestoreSectionIntegrity(CModuleIntegrityCheckingSectionChanges* pSectionChange)
{
    return pSectionChange->RestoreIntegrity(this->ProcessId,this->DllBaseAddress);
}

BOOL CModuleIntegrityChecking::RestoreIntegrity(CModuleIntegrityCheckingChanges* pChange)
{
    return pChange->RestoreIntegrity(this->ProcessId,this->DllBaseAddress);
}

// get module entry from dll name (not full path)
MODULEENTRY* CModuleIntegrityChecking::GetModuleEntry(TCHAR* LibraryName)
{
    // for each imported dll
    CLinkListItemTemplate<MODULEENTRY>* pItemModuleEntry;
    MODULEENTRY* pModuleEntry;

    // get module base address
    for (pItemModuleEntry = this->ProcessModuleList.GetHead();pItemModuleEntry;pItemModuleEntry = pItemModuleEntry->NextItem)
    {
        pModuleEntry = pItemModuleEntry->ItemData;
        if ( _tcsicmp(pModuleEntry->szModule,LibraryName) == 0 )
        {
            return pModuleEntry;
        }
    }
    return NULL;
}


CPE* CModuleIntegrityChecking::FindOrCreatePe(TCHAR* FileName)
{
    CLinkListItemTemplate<CPE>* pItem;
    CPE* pDllPE;
    TCHAR AlreadyParsedFileName[MAX_PATH];
    for (pItem = this->ParsedPeList.GetHead();pItem;pItem = pItem->NextItem)
    {
        pDllPE = pItem->ItemData;
        pDllPE->GetFileName(AlreadyParsedFileName);
        if (_tcsicmp(AlreadyParsedFileName,FileName)==0)
        {
            return pDllPE;
        }
    }
    // if not already parsed
    pDllPE = new CPE(FileName);
    if (!pDllPE->Parse(TRUE,FALSE,FALSE))
    {
        delete pDllPE;
        return 0;
    }
    this->ParsedPeList.AddItem(pDllPE);
    return pDllPE;
}

// DllName : short path not full path
ULONGLONG CModuleIntegrityChecking::GetRebasedAddressOfFunction(TCHAR* DllName, TCHAR* FunctionName)
{
    MODULEENTRY* pModuleEntry;
    WORD FunctionHint = 0;
    CLinkListItem* pItemExportedFunction;
    CPE::EXPORT_FUNCTION_ITEM* pExportedFunctionInfos;

    pModuleEntry = this->GetModuleEntry(DllName);
    if ( pModuleEntry == 0)
        return 0;// can occur for delay loaded dll

    CPE* pDllPE = this->FindOrCreatePe(pModuleEntry->szExePath);
    if (!pDllPE)
        return 0;

    for (pItemExportedFunction =pDllPE->pExportTable->Head;pItemExportedFunction;pItemExportedFunction = pItemExportedFunction->NextItem)
    {
        pExportedFunctionInfos = (CPE::EXPORT_FUNCTION_ITEM*)pItemExportedFunction->ItemData;
        if (_tcscmp(pExportedFunctionInfos->FunctionName,FunctionName) == 0)
        {
            return this->GetRebasedAddressOfFunction(pModuleEntry,pDllPE,pExportedFunctionInfos);
        }
    }
    // not found
    return 0;
}
// DllName : short path not full path
ULONGLONG CModuleIntegrityChecking::GetRebasedAddressOfFunction(TCHAR* DllName, WORD FunctionHintOrOrdinal, BOOL bOrdinal)
{
    MODULEENTRY* pModuleEntry;
    pModuleEntry = this->GetModuleEntry(DllName);
    if ( pModuleEntry == 0)
        return 0;// can occur for delay loaded dll

    CPE* pDllPE = this->FindOrCreatePe(pModuleEntry->szExePath);
    if (!pDllPE)
        return 0;

    return this->GetRebasedAddressOfFunction(pModuleEntry,pDllPE,FunctionHintOrOrdinal,bOrdinal);
}

ULONGLONG CModuleIntegrityChecking::GetRebasedAddressOfFunction(MODULEENTRY* pDllEntry,CPE* pDllPE, WORD FunctionHintOrOrdinal, BOOL bOrdinal)
{
    CLinkListItem* pItemExportedFunction;
    CPE::EXPORT_FUNCTION_ITEM* pExportedFunctionInfos = NULL;

    // search hint
    for (pItemExportedFunction =pDllPE->pExportTable->Head;pItemExportedFunction;pItemExportedFunction = pItemExportedFunction->NextItem)
    {
        pExportedFunctionInfos = (CPE::EXPORT_FUNCTION_ITEM*)pItemExportedFunction->ItemData;
        if (bOrdinal)
        {
            if (pExportedFunctionInfos->ExportedOrdinal == FunctionHintOrOrdinal)
            {
                return this->GetRebasedAddressOfFunction(pDllEntry,pDllPE,pExportedFunctionInfos);
            }
        }
        else if (pExportedFunctionInfos->Hint == FunctionHintOrOrdinal)// use hint instead 
        {
            return this->GetRebasedAddressOfFunction(pDllEntry,pDllPE,pExportedFunctionInfos);
        }
    }

    // not found
    return 0;
}
ULONGLONG CModuleIntegrityChecking::GetRebasedAddressOfFunction(MODULEENTRY* pDllEntry, CPE* pDllPE, CPE::EXPORT_FUNCTION_ITEM* pExportedFunctionInfos)
{
    // matching function has been found in export table (should be the case)

    // if exported function is a redirection
    if (pExportedFunctionInfos->Forwarded)
    {
        // check rebasing of redirected dll
        TCHAR ModifiedForwardedName[MAX_PATH];
        TCHAR DllName[MAX_PATH];
        TCHAR FunctionName[MAX_PATH];
        // extract function name and dll name from forwarded name
        _tcscpy(ModifiedForwardedName,pExportedFunctionInfos->ForwardedName);
        TCHAR* pc;
        pc = _tcschr(ModifiedForwardedName,'.');
        if (!pc)
            return 0;
        *pc=0;
        _tcscpy(FunctionName,pc+1);
        _tcscpy(DllName,ModifiedForwardedName);

        if(this->DllStub.IsStubDll(DllName))
        {
            TCHAR ImportingModuleName[MAX_PATH];
            TCHAR RealDllName[MAX_PATH];
            pDllPE->GetFileName(ImportingModuleName);
            if (!this->DllStub.GetRealModuleNameFromStubName(ImportingModuleName,DllName,RealDllName) )
                return 0;
            _tcscpy(DllName,RealDllName);
        }
        else
        {
            _tcscat(DllName,_T(".dll"));
        }
        
        // get forwarded function infos
        return this->GetRebasedAddressOfFunction(DllName,FunctionName);
    }

    // function is not forwarded

    return (ULONGLONG)(pExportedFunctionInfos->FunctionAddressRVA + pDllEntry->modBaseAddr);
}

// check rebasing for IAT (imported dll)
BOOL CModuleIntegrityChecking::UpdateOriginalIATFromRebasedDllAddresses(PBYTE pMemoryPEStartAddress ,CPE* pPE)
{
    BOOL bRetValue = TRUE;
    CLinkListItem* pItem;
    CLinkListItem* pItemFunction;
    CPE::IMPORT_LIBRARY_ITEM* pImportedModuleInfos;
    CPE::IMPORT_FUNCTION_ITEM* pImportedFunctionInfos;
    ULONGLONG RealAddress;
    ULONGLONG RawIATLocation;
    MODULEENTRY* pModuleEntry;

    // for each imported dll
    for (pItem = pPE->pImportTable->Head;pItem;pItem = pItem->NextItem)
    {
        pImportedModuleInfos = (CPE::IMPORT_LIBRARY_ITEM*)pItem->ItemData;

        // for delay loaded dll, if dll is not loaded, no iat patching is required
        if (pImportedModuleInfos->bDelayLoadDll)
        {
            pModuleEntry = this->GetModuleEntry(DllName);
            if ( pModuleEntry == 0)
            {
                // dll is not loaded --> nothing to patch for current module
                continue; // check next module
            }
        }


        // for each imported function of current imported dll
        for (pItemFunction =pImportedModuleInfos->pFunctions->Head;pItemFunction;pItemFunction = pItemFunction->NextItem)
        {
            pImportedFunctionInfos = (CPE::IMPORT_FUNCTION_ITEM*)pItemFunction->ItemData;

            RealAddress = 0;
            if (this->DllStub.IsStubDll(pImportedModuleInfos->LibraryName))
            {
                TCHAR RealDllName[MAX_PATH];
                if (this->DllStub.GetRealModuleNameFromStubName(CStdFileOperations::GetFileName(this->DllPath),pImportedModuleInfos->LibraryName,RealDllName))
                {
                    // we can't use ordinal --> use function name
                    RealAddress = this->GetRebasedAddressOfFunction(RealDllName,pImportedFunctionInfos->FunctionName);
                }
            }
            else
            {
                if (pImportedFunctionInfos->bOrdinalOnly)
                    RealAddress = this->GetRebasedAddressOfFunction(pImportedModuleInfos->LibraryName,pImportedFunctionInfos->Ordinal,pImportedFunctionInfos->bOrdinalOnly);
                else
                {
                    // use function name if any
                    if (*pImportedFunctionInfos->FunctionName)
                    {
                        RealAddress = this->GetRebasedAddressOfFunction(pImportedModuleInfos->LibraryName,pImportedFunctionInfos->FunctionName);
                    }
                    else // Hint is often the same but not always so use hint in last chance
                        RealAddress = this->GetRebasedAddressOfFunction(pImportedModuleInfos->LibraryName,pImportedFunctionInfos->Hint,pImportedFunctionInfos->bOrdinalOnly);
                }
            }

            if (RealAddress == 0)
            {
                bRetValue = FALSE;
                continue;
            }

            // update
            if (!pPE->RvaToRaw(pImportedFunctionInfos->PointerToIATEntryRVA,&RawIATLocation))// applies to PE of exe
                continue;

            if (pPE->Is64Bits())// applies to PE of exe
                *((ULONGLONG*)(pMemoryPEStartAddress + RawIATLocation)) = RealAddress;
            else
                *((DWORD*)(pMemoryPEStartAddress + RawIATLocation)) = (DWORD)RealAddress;
        }
    }
    return bRetValue;
}

// check rebasing for current module
BOOL __fastcall CModuleIntegrityChecking::IsRebasedAddress(ULONGLONG OriginalBaseAddress, ULONGLONG RebasedAddress, PBYTE MayOriginalPointer, PBYTE MayRebasedPointer, BOOL b64BitProcess, OUT PBYTE* pRebasedAddressBegin, OUT PBYTE* pRebasedAddressEnd)
{
    BOOL bRebasedAddress;
    SIZE_T AddressLen;
    PBYTE OriginalAddressBegin;
    PBYTE RebasedAddressBegin;

    bRebasedAddress = FALSE;
    if (b64BitProcess)
        AddressLen = 8;
    else
        AddressLen = 4;
    // look inside range  ]MayRebasedPointer - AddressLen ; MayRebasedPointer + AddressLen[
    // to check for potential matching
    for (RebasedAddressBegin = MayRebasedPointer - AddressLen + 1, OriginalAddressBegin = MayOriginalPointer  - AddressLen + 1 ; 
         RebasedAddressBegin <= MayRebasedPointer ;
         RebasedAddressBegin++ , OriginalAddressBegin++
         )
    {
        if ( ::IsBadReadPtr(RebasedAddressBegin,AddressLen) 
             || ::IsBadReadPtr(OriginalAddressBegin,AddressLen) 
            )
        {
            continue;
        }

        if (b64BitProcess)
        {
            if (*((ULONGLONG*)RebasedAddressBegin) - RebasedAddress == *((ULONGLONG*)OriginalAddressBegin) - OriginalBaseAddress)
            {
                bRebasedAddress = TRUE;
                break;
            }
        }
        else
        {
            if (*((DWORD*)RebasedAddressBegin) - (DWORD)RebasedAddress == *((DWORD*)OriginalAddressBegin) - (DWORD)OriginalBaseAddress)
            {
                bRebasedAddress = TRUE;
                break;
            }
        }
    }
    if (bRebasedAddress)
    {
        *pRebasedAddressBegin = RebasedAddressBegin;
        *pRebasedAddressEnd = *pRebasedAddressBegin + AddressLen - 1;
    }
    else
    {
        *pRebasedAddressBegin = 0;
        *pRebasedAddressEnd = 0;
    }
    return bRebasedAddress;
}

BOOL CModuleIntegrityChecking::CheckIntegrity(BOOL CheckOnlyCodeAndExecutableSections,BOOL DontCheckWritableSections,BOOL ShowRebasing,DWORD MinMatchingSizeAfterReplacement)
{
    CModuleIntegrityCheckingSectionChanges* pSectionChange;
    CModuleIntegrityCheckingChanges* pChange;
    BOOL HeaderChanges;
    BOOL RebasingDetected;
    BOOL b64BitProcess;
    PBYTE FunctionStartAddress;
    PBYTE MemoryInstructionRVA;
    PBYTE FileInstructionRVA;
    SIZE_T cntFile;
    SIZE_T cntMemory;
    SIZE_T MinSectionSize;

    CPE PeFromMemory;
    if (!PeFromMemory.ParseFromMemory(this->DllLocalCopy,FALSE,FALSE))
        return FALSE;
    
    CPE PeFromFile;
    PeFromFile.bDisplayErrorMessages=FALSE;
    if (!PeFromFile.ParseFromMemory((PBYTE)this->OriginalFile,TRUE,!ShowRebasing))// parse export to try to get function name in case of changes
                                                                                  // parse import if rebasing must be hide
    {
        // in case parsing error is due to export table parsing
        PeFromFile.bDisplayErrorMessages=TRUE;
        if (!PeFromFile.ParseFromMemory((PBYTE)this->OriginalFile,FALSE,FALSE))
            return FALSE;
    }

    RebasingDetected = (PeFromFile.NTHeader.OptionalHeader.ImageBase != PeFromMemory.NTHeader.OptionalHeader.ImageBase);
    b64BitProcess = PeFromFile.Is64Bits();

    if (!ShowRebasing)
    {
        // update IAT before checking
        this->UpdateOriginalIATFromRebasedDllAddresses(this->OriginalFile,&PeFromFile);
    }

    // check headers first
    HeaderChanges=FALSE;
    if (PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders!=PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders)
    {
        HeaderChanges=TRUE;
    }
    else if (memcmp(this->OriginalFile,this->DllLocalCopy,PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders))
    {
        HeaderChanges=TRUE;
    }
    // if change on header
    if (HeaderChanges)
    {
        pSectionChange=new CModuleIntegrityCheckingSectionChanges();
        _tcscpy(pSectionChange->SectionName,_T("PE"));

        // the following show full header content --> quite hard to find change
        // // report module header integrity failure
        //pChange=new CModuleIntegrityCheckingChanges();
        //pChange->ChangeType=CModuleIntegrityChecking::ChangesType_Header;

        //pChange->BufferLen= __max(PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders,PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders);
        //pChange->FileBuffer=new BYTE[pChange->BufferLen];
        //memset(pChange->FileBuffer,0,pChange->BufferLen); // assume if header size differs to be completed by 0
        //memcpy(pChange->FileBuffer,this->OriginalFile,PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders);
        //pChange->FileOffset=0;

        //pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
        //memset(pChange->MemoryBuffer,0,pChange->BufferLen);// assume if header size differs to be completed by 0
        //memcpy(pChange->MemoryBuffer,this->DllLocalCopy,PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders);
        //pChange->MemoryOffset=0;
        //pSectionChange->pChangesLinkList->AddItem(pChange);
        //this->pSectionsChangesLinkList->AddItem(pSectionChange);


        MinSectionSize=__min(PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders,PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders);

        // quick check of section content
        if (memcmp(this->OriginalFile,this->DllLocalCopy,MinSectionSize)!=0)
        {
            SIZE_T NbCurrentMatching=0;
            SIZE_T LastChange=0;
            BOOL  InsideChange=FALSE;
            SIZE_T cnt;

            // in case of bad checking, refine compare byte by byte
            for (cnt=0;cnt<MinSectionSize;cnt++)
            {
                // if byte is different
                if ( ((PBYTE)this->OriginalFile)[cnt]!=
                    ((PBYTE)this->DllLocalCopy)[cnt]
                )
                {
                    if (RebasingDetected && !ShowRebasing)
                    {
                        // if address match BaseAddress
                        if (b64BitProcess)
                        {
                            // if address belongs to the ImageBase field
                            if ( ( PeFromFile.DosHeader.e_lfanew + 0x30 <=cnt) && (cnt <= PeFromFile.DosHeader.e_lfanew + 0x38) )
                            {
                                continue;
                            }
                        }
                        else
                        {
                            // if address belongs to the ImageBase field
                            if ( ( PeFromFile.DosHeader.e_lfanew + 0x34 <=cnt) && (cnt <= PeFromFile.DosHeader.e_lfanew + 0x38) )
                            {
                                continue;
                            }
                        }
                    }

                    // if no change detected
                    if (!InsideChange)
                    {
                        LastChange=cnt;// save position of the change
                        InsideChange=TRUE;
                    }

                    // reset current matching bytes
                    NbCurrentMatching=0;
                }
                else
                {
                    // if change detected
                    if (InsideChange)
                    {
                        NbCurrentMatching++;
                        if (NbCurrentMatching>=MinMatchingSizeAfterReplacement)
                        {
                            // add change to linked list
                            pChange=new CModuleIntegrityCheckingChanges();
                            pChange->ChangeType=CModuleIntegrityChecking::ChangesType_ByteChangesInSection;
                            pChange->BufferLen=cnt-LastChange-MinMatchingSizeAfterReplacement+1;

                            // store memory buffer
                            pChange->MemoryOffset=(PBYTE)LastChange;
                            pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                            memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                            // store file buffer
                            pChange->FileOffset=(PBYTE)LastChange;
                            pChange->FileBuffer=new BYTE[pChange->BufferLen];
                            memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

                            // add item to list
                            pSectionChange->pChangesLinkList->AddItem(pChange);

                            // reset last change state
                            InsideChange=FALSE;
                        }
                    }
                }
            }
            // if inside change
            if (InsideChange)
            {
                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_ByteChangesInSection;

                pChange->BufferLen=cnt-LastChange+1;

                // store memory buffer
                pChange->MemoryOffset=(PBYTE)LastChange;
                pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                // store file buffer
                pChange->FileOffset=(PBYTE)LastChange;
                pChange->FileBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

                // add item to list
                pSectionChange->pChangesLinkList->AddItem(pChange);
            }
        }

        // in case of different size of address spaces
        if (PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders!=PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders)
        {
            if (PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders<PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders)
            {
                // module in memory contains n bytes more 

                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData;

                pChange->BufferLen=PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders-PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders;

                // store memory buffer
                pChange->MemoryOffset=(PBYTE)MinSectionSize;
                pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                pSectionChange->pChangesLinkList->AddItem(pChange);

            }
            else
            {
                // file contains n bytes more

                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData;

                pChange->BufferLen=PeFromFile.NTHeader.OptionalHeader.SizeOfHeaders-PeFromMemory.NTHeader.OptionalHeader.SizeOfHeaders;

                // store file buffer
                pChange->FileOffset=(PBYTE)MinSectionSize;
                pChange->FileBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

                pSectionChange->pChangesLinkList->AddItem(pChange);
            }
        }
        this->pSectionsChangesLinkList->AddItem(pSectionChange);
    }

    // check for new sections in memory
    for (cntMemory=0;cntMemory<PeFromMemory.NTHeader.FileHeader.NumberOfSections;cntMemory++)
    {
        for (cntFile=0;cntFile<PeFromFile.NTHeader.FileHeader.NumberOfSections;cntFile++)
        {
            if (strcmp((char*)PeFromFile.pSectionHeaders[cntFile].Name,(char*)PeFromMemory.pSectionHeaders[cntMemory].Name)==0)
            {
                break;
            }
            break;
        }
        // if not found
        if (cntFile==PeFromFile.NTHeader.FileHeader.NumberOfSections)
        {
            // report section xx not found in file
            pSectionChange=new CModuleIntegrityCheckingSectionChanges();
            pSectionChange->SectionStartAddress=PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress+this->DllBaseAddress;
            pSectionChange->SectionSize=PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData;
            pChange=new CModuleIntegrityCheckingChanges();
            pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionNotInBinary;
#if (defined(UNICODE)||defined(_UNICODE))
            CAnsiUnicodeConvert::AnsiToUnicode((char*)PeFromMemory.pSectionHeaders[cntMemory].Name,pSectionChange->SectionName,SECTION_NAME_STRING_SIZE);
#else
            strncpy(pSectionChange->SectionName,(char*)PeFromMemory.pSectionHeaders[cntMemory].Name,PE_SECTION_NAME_SIZE);
#endif
            pSectionChange->SectionName[PE_SECTION_NAME_SIZE]=0;
            pChange->BufferLen=PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData;
            pChange->MemoryOffset=(PBYTE)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress;
            pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
            memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

            pSectionChange->pChangesLinkList->AddItem(pChange);
            this->pSectionsChangesLinkList->AddItem(pSectionChange);
        }
    }

    // for each file section
    for (cntFile=0;cntFile<PeFromFile.NTHeader.FileHeader.NumberOfSections;cntFile++)
    {
        // if section is not part of image
        if ((PeFromFile.pSectionHeaders[cntFile].Characteristics & IMAGE_SCN_LNK_REMOVE) !=0)
        {
            // normal no need to report a change
            continue;
        }
        
        if (CheckOnlyCodeAndExecutableSections)
        {
            // if section don't contains code -> don't check it
            if (   ((PeFromFile.pSectionHeaders[cntFile].Characteristics & IMAGE_SCN_CNT_CODE) ==0)
                && ((PeFromFile.pSectionHeaders[cntFile].Characteristics & IMAGE_SCN_MEM_EXECUTE) ==0)
                )
            {
                continue;
            }
        }

        // if section is writable -> supposed to contains global vars don't check it
        if ((PeFromFile.pSectionHeaders[cntFile].Characteristics & IMAGE_SCN_MEM_WRITE) !=0)
        {
            if (DontCheckWritableSections)
                continue;
        }
        /////////////////////////////////////
        // find section in memory
        /////////////////////////////////////
        for (cntMemory=0;cntMemory<PeFromMemory.NTHeader.FileHeader.NumberOfSections;cntMemory++)
        {
            if (strcmp((char*)PeFromFile.pSectionHeaders[cntFile].Name,(char*)PeFromMemory.pSectionHeaders[cntMemory].Name)==0)
            {
                break;
            }
        }
        // if not found
        if (cntMemory==PeFromMemory.NTHeader.FileHeader.NumberOfSections)
        {
            // report section xx not found in memory
            pSectionChange=new CModuleIntegrityCheckingSectionChanges();
            pChange=new CModuleIntegrityCheckingChanges();
            pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionNotInMemory;
#if (defined(UNICODE)||defined(_UNICODE))
            CAnsiUnicodeConvert::AnsiToUnicode((char*)PeFromFile.pSectionHeaders[cntFile].Name,pSectionChange->SectionName,SECTION_NAME_STRING_SIZE);
#else
            strncpy(pSectionChange->SectionName,(char*)PeFromFile.pSectionHeaders[cntFile].Name,PE_SECTION_NAME_SIZE);
#endif
            pSectionChange->SectionName[PE_SECTION_NAME_SIZE]=0;
            pChange->BufferLen=PeFromFile.pSectionHeaders[cntFile].SizeOfRawData;
            pChange->FileOffset=(PBYTE)PeFromFile.pSectionHeaders[cntFile].PointerToRawData;
            pChange->FileBuffer=new BYTE[pChange->BufferLen];
            memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

            pSectionChange->pChangesLinkList->AddItem(pChange);
            this->pSectionsChangesLinkList->AddItem(pSectionChange);

            continue;
        }


        // section has been found --> add section to list
        pSectionChange=new CModuleIntegrityCheckingSectionChanges();
        pSectionChange->SectionStartAddress=PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress+this->DllBaseAddress;
        pSectionChange->SectionSize=PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData;
        this->pSectionsChangesLinkList->AddItem(pSectionChange);
#if (defined(UNICODE)||defined(_UNICODE))
        CAnsiUnicodeConvert::AnsiToUnicode((char*)PeFromFile.pSectionHeaders[cntFile].Name,pSectionChange->SectionName,SECTION_NAME_STRING_SIZE);
#else
        strncpy(pSectionChange->SectionName,(char*)PeFromFile.pSectionHeaders[cntFile].Name,PE_SECTION_NAME_SIZE);
#endif
        pSectionChange->SectionName[PE_SECTION_NAME_SIZE]=0;

        /////////////////////////////////////
        // check section
        /////////////////////////////////////
        MinSectionSize=__min(PeFromFile.pSectionHeaders[cntFile].SizeOfRawData,PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData);

        // quick check of section content
        if (memcmp((PBYTE)(this->OriginalFile+PeFromFile.pSectionHeaders[cntFile].PointerToRawData),
                (PBYTE)(this->DllLocalCopy+PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress),
                    MinSectionSize)
                    !=0)
        {

            SIZE_T NbCurrentMatching=0;
            SIZE_T LastChange=0;
            BOOL  InsideChange=FALSE;
            SIZE_T cnt;

            // in case of bad checking, refine compare byte by byte
            for (cnt=0;cnt<MinSectionSize;cnt++)
            {
                // if byte is different
                if ( ((PBYTE)this->OriginalFile+PeFromFile.pSectionHeaders[cntFile].PointerToRawData)[cnt]!=
                    ((PBYTE)this->DllLocalCopy+PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress)[cnt]
                )
                {
                    if (RebasingDetected && !ShowRebasing)// rebasing applies on code section replacing all IAT references --> instead we could do a loop searching iat references to replace them
                    {
                        PBYTE RebasedAddressBegin;
                        PBYTE RebasedAddressEnd;
                        if ( 
                            CModuleIntegrityChecking::IsRebasedAddress( PeFromFile.NTHeader.OptionalHeader.ImageBase,
                                                                        PeFromMemory.NTHeader.OptionalHeader.ImageBase,
                                                                        &(( (PBYTE)this->OriginalFile+PeFromFile.pSectionHeaders[cntFile].PointerToRawData )[cnt]),
                                                                        &(( (PBYTE)this->DllLocalCopy+PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress )[cnt]),
                                                                        b64BitProcess,
                                                                        &RebasedAddressBegin,
                                                                        &RebasedAddressEnd
                                                                        )
                            )
                        {
                            cnt+=(SIZE_T) ( (RebasedAddressEnd/*+1*/) - &( (PBYTE)this->DllLocalCopy+PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress )[cnt]);// the +1 to position after address will be done by the cnt++ instruction of the for loop
                            continue;
                        }
                    }

                    // if no change detected
                    if (!InsideChange)
                    {
                        LastChange=cnt;// save position of the change
                        InsideChange=TRUE;
                    }

                    // reset current matching bytes
                    NbCurrentMatching=0;
                }
                else
                {
                    // if change detected
                    if (InsideChange)
                    {
                        NbCurrentMatching++;
                        if (NbCurrentMatching>=MinMatchingSizeAfterReplacement)
                        {
                            // add change to linked list
                            pChange=new CModuleIntegrityCheckingChanges();
                            pChange->ChangeType=CModuleIntegrityChecking::ChangesType_ByteChangesInSection;
                            pChange->BufferLen=cnt-LastChange-MinMatchingSizeAfterReplacement+1;

                            // store memory buffer
                            pChange->MemoryOffset=(PBYTE)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress+LastChange;
                            pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                            memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                            // store file buffer
                            pChange->FileOffset=(PBYTE)PeFromFile.pSectionHeaders[cntFile].PointerToRawData+LastChange;
                            pChange->FileBuffer=new BYTE[pChange->BufferLen];
                            memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

                            // find function infos
                            FileInstructionRVA=(PBYTE)
                                pChange->MemoryOffset
                                -(SIZE_T)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress
                                +(SIZE_T)PeFromFile.pSectionHeaders[cntFile].VirtualAddress;
                            CDllFindFuncNameFromRVA::FindFuncNameFromRVA(
                                &PeFromFile,
                                FileInstructionRVA,
                                (TCHAR*)pChange->AssociatedFunctionName,
                                MAX_PATH,
                                &FunctionStartAddress);
                            MemoryInstructionRVA=(PBYTE)
                                (SIZE_T)FunctionStartAddress
                                -(SIZE_T)PeFromFile.pSectionHeaders[cntFile].VirtualAddress
                                +(SIZE_T)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress;
                            pChange->RelativeAddressFromFunctionStart=(PBYTE)((SIZE_T)pChange->MemoryOffset-(SIZE_T)MemoryInstructionRVA);

                            // add item to list
                            pSectionChange->pChangesLinkList->AddItem(pChange);

                            // reset last change state
                            InsideChange=FALSE;
                        }
                    }
                }
            }
            // if inside change
            if (InsideChange)
            {
                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_ByteChangesInSection;

                pChange->BufferLen=cnt-LastChange+1;

                // store memory buffer
                pChange->MemoryOffset=(PBYTE)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress+LastChange;
                pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                // store file buffer
                pChange->FileOffset=(PBYTE)PeFromFile.pSectionHeaders[cntFile].PointerToRawData+LastChange;
                pChange->FileBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);


                // find function infos
                FileInstructionRVA=(PBYTE)
                    pChange->MemoryOffset
                    -(SIZE_T)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress
                    +(SIZE_T)PeFromFile.pSectionHeaders[cntFile].VirtualAddress;
                CDllFindFuncNameFromRVA::FindFuncNameFromRVA(
                    &PeFromFile,
                    FileInstructionRVA,
                    (TCHAR*)pChange->AssociatedFunctionName,
                    MAX_PATH,
                    &FunctionStartAddress);
                MemoryInstructionRVA=(PBYTE)
                    (SIZE_T)FunctionStartAddress
                    -(SIZE_T)PeFromFile.pSectionHeaders[cntFile].VirtualAddress
                    +(SIZE_T)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress;
                pChange->RelativeAddressFromFunctionStart=(PBYTE)((SIZE_T)pChange->MemoryOffset-(SIZE_T)MemoryInstructionRVA);

                // add item to list
                pSectionChange->pChangesLinkList->AddItem(pChange);
            }
        }

        // in case of different size of address spaces
        if (PeFromFile.pSectionHeaders[cntFile].SizeOfRawData!=PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData)
        {
            if (PeFromFile.pSectionHeaders[cntFile].SizeOfRawData<PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData)
            {
                // module in memory contains n bytes more 

                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionMemoryContainsMoreData;

                pChange->BufferLen=PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData-PeFromFile.pSectionHeaders[cntFile].SizeOfRawData;

                // store memory buffer
                pChange->MemoryOffset=(PBYTE)PeFromMemory.pSectionHeaders[cntMemory].VirtualAddress+MinSectionSize;
                pChange->MemoryBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->MemoryBuffer,this->DllLocalCopy+(SIZE_T)pChange->MemoryOffset,pChange->BufferLen);

                pSectionChange->pChangesLinkList->AddItem(pChange);

            }
            else
            {
                // file contains n bytes more

                // add change to linked list
                pChange=new CModuleIntegrityCheckingChanges();
                pChange->ChangeType=CModuleIntegrityChecking::ChangesType_SectionBinaryContainsMoreData;

                pChange->BufferLen=PeFromFile.pSectionHeaders[cntFile].SizeOfRawData-PeFromMemory.pSectionHeaders[cntMemory].SizeOfRawData;

                // store file buffer
                pChange->FileOffset=(PBYTE)PeFromFile.pSectionHeaders[cntFile].PointerToRawData+MinSectionSize;
                pChange->FileBuffer=new BYTE[pChange->BufferLen];
                memcpy(pChange->FileBuffer,this->OriginalFile+(SIZE_T)pChange->FileOffset,pChange->BufferLen);

                pSectionChange->pChangesLinkList->AddItem(pChange);
            }
        }
    }
  
    return TRUE;
}