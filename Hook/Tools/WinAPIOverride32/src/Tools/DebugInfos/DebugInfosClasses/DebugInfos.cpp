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

#include "debuginfos.h"
#include "../../LinkList/LinkListTemplate.cpp"

#include "../DiaSDK/include/dia2.h"
//#pragma comment (lib, "../DiaSDK/lib/diaguids.lib") // avoid as used in multiple projects
// we just define the 2 required values CLSID_DiaSource and IID_IDiaDataSource to avoid to link with lib

/////////////////////////////////////////////////////////////////////////////////////
// /!\ update associated DIA_DLL_NAME in DebugInfo.h on CLSID_DiaSource value change
/////////////////////////////////////////////////////////////////////////////////////
extern const GUID CLSID_DiaSource = { 0XBCE36434, 0X2C24, 0X499E, { 0xBF, 0x49, 0x8B, 0xD9, 0x9B, 0x0E, 0xEB, 0x68 } };  // msdia80.dll
//extern const GUID CLSID_DiaSource = { 0xB86AE24D, 0xBF2F, 0x4AC9, { 0xB5, 0xA2, 0x34, 0xB1, 0x4E, 0x4C, 0xE1, 0x1D } }; // msdia100.dll

extern const GUID IID_IDiaDataSource = { 0x79F1BB5F, 0xB66E, 0x48E5, { 0xB6, 0xA9, 0x15, 0x45, 0xC3, 0x23, 0xCA, 0x3D } };

DWORD g_RegistersMachineType;

CDebugInfos::CDebugInfos(TCHAR* FileName,BOOL DisplayErrorMessages)
{
    this->CommonConstructor(FileName);
    this->bDisplayErrorMessages=DisplayErrorMessages;
}

CDebugInfos::CDebugInfos(TCHAR* FileName)
{
    this->CommonConstructor(FileName);
}

void CDebugInfos::CommonConstructor(TCHAR* FileName)
{
    this->bDisplayErrorMessages=TRUE;
    this->pDiaGlobalSymbol=NULL;
    this->pDiaSession=NULL;
    this->bComInitialized=SUCCEEDED(CoInitialize(NULL));
    this->MachineType = CV_CFL_80386;
    this->pDiaDataSource=NULL;
    *this->DiaDllPath=0;
    *this->FileName=0;
    this->pLinkListModules=new CLinkListSimple();
    if (FileName)
        _tcscpy(this->FileName,FileName);
    else
        *this->FileName=0;
}

CDebugInfos::~CDebugInfos(void)
{
    this->FreeMemory();

    delete this->pLinkListModules;

    if (this->bComInitialized)
        CoUninitialize();
    // if com dia has been registered by this app, unregister it
    if (*this->DiaDllPath)
        CRegisterComComponent::UnregisterComponent(this->DiaDllPath);
}

//-----------------------------------------------------------------------------
// Name: GetFileName
// Object: get name of file currently parsed
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
const TCHAR* CDebugInfos::GetFileName()
{
    return this->FileName;
}

void CDebugInfos::FreeMemory()
{
    CLinkListItem* pItem;
    CModuleInfos* pModuleInfos;

    // for each item of this->pLinkListModules
    for (pItem=this->pLinkListModules->Head;pItem;pItem=pItem->NextItem)
    {
        pModuleInfos=(CModuleInfos*)pItem->ItemData;
        // delete item
        delete pModuleInfos;
    }
    this->pLinkListModules->RemoveAllItems();

    if(this->pDiaGlobalSymbol)
    {
        this->pDiaGlobalSymbol->Release();
        this->pDiaGlobalSymbol = NULL;
    }
    if(this->pDiaSession)
    {
        this->pDiaSession->Release();
        this->pDiaSession = NULL;
    }
    if (this->pDiaDataSource)
    {
        this->pDiaDataSource->Release();
        this->pDiaDataSource = NULL;
    }
}
//-----------------------------------------------------------------------------
// Name: ReportError
// Object: report error 
// Parameters :
//     in  : TCHAR* Msg : error message
//           HRESULT hr : error number
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDebugInfos::ReportError(TCHAR* Msg,HRESULT hr)
{
    if (!this->bDisplayErrorMessages)
        return;
    else
    {
        TCHAR* psz=(TCHAR*)_alloca((_tcslen(Msg)+1 +50)*sizeof(PBYTE));// +50 for _T("\r\n") & _T("Error: 0x%.8X")
        _stprintf(  psz,
                    _T("%s\r\n")
                    _T("Error 0x%.8X"),
                    Msg,
                    hr
                    );
        MessageBox(NULL,psz,_T("Error"),MB_OK|MB_ICONERROR);
    }
}

//-----------------------------------------------------------------------------
// Name: HasDebugInfos
// Object: check if file has debug informations
// Parameters :
//     in  : 
//     out :
//     return : TRUE if file has debug informations
//-----------------------------------------------------------------------------
BOOL CDebugInfos::HasDebugInfos()
{
    if (this->pDiaSession)
        return TRUE;
    else
    {
        BOOL bRet;
        // save current error message output
        BOOL DisplayErrorMessages=this->bDisplayErrorMessages;
        // avoid error report
        this->bDisplayErrorMessages=FALSE;
        // try to load debugs informations
        bRet=this->LoadDataFromPdb();
        // restore error message output
        this->bDisplayErrorMessages=DisplayErrorMessages;
        // return debug informations loading result
        return bRet;
    }
}

//-----------------------------------------------------------------------------
// Name: LoadDataFromPdb
// Object: load debug symbols from files
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::LoadDataFromPdb()
{
    if (this->pDiaSession)
        return TRUE;

    HRESULT hr;
    wchar_t wszFileName[MAX_PATH];
    wchar_t* wszExt;
    wchar_t* wszSearchPath = L"SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data
    DWORD dwMachType = 0;

    // release old members if not first parsing
    this->FreeMemory();

    // Obtain Access To The Provider
    hr = CoCreateInstance(CLSID_DiaSource, //__uuidof(DiaSource), 
                        NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID_IDiaDataSource, //__uuidof(IDiaDataSource),
                        (void **) &this->pDiaDataSource);
    if(FAILED(hr) || (this->pDiaDataSource==0))
    {
        CStdFileOperations::GetAppPath(this->DiaDllPath,MAX_PATH);
        _tcscat(this->DiaDllPath,DIA_DLL_NAME);

        // assume com dia dll has been registered
        if (!CRegisterComComponent::RegisterComponent(this->DiaDllPath))
        {
            TCHAR psz[2*MAX_PATH];
            _stprintf(psz,
                    _T("Make sur %s exists.\r\n")
                    _T("If this file is existing, launch this software with administrator rights, or manually register this dll"),
                    DIA_DLL_NAME);
            this->ReportError(psz,E_FAIL);
            *this->DiaDllPath=0;
            return FALSE;
        }

        // try again after having registered dll
        hr = CoCreateInstance(CLSID_DiaSource, //__uuidof(DiaSource), 
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IDiaDataSource, //__uuidof(IDiaDataSource),
                            (void **) &this->pDiaDataSource);
        if(FAILED(hr) || (this->pDiaDataSource==0))
        {
            this->ReportError(_T("Error creating IDiaDataSource instance"),hr);
            return FALSE;
        }
    }
#if (defined(UNICODE)||defined(_UNICODE))
    _tcscpy(wszFileName,this->FileName);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(this->FileName,wszFileName,MAX_PATH);
#endif
    wszExt=wcsrchr(wszFileName,'.');
    if (!wszExt)
        wszExt=wszFileName;

    // if a .pdb opening was queried
    if(!_wcsicmp(wszExt,L".pdb"))
    {
        // Open and prepare a program database (.pdb) file as a debug data source
        hr = this->pDiaDataSource->loadDataFromPdb(wszFileName);
        if(FAILED(hr))
        {
            this->ReportError(_T("Error loading debug symbols (IDiaDataSource::loadDataFromPdb failed)"),hr);
            return FALSE;
        }
    }
    else
    {
        CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
                            // thus enabling a user interface to report on the progress of 
                            // the location attempt. The client application may optionally 
                            // provide a reference to its own implementation of this 
                            // virtual base class to the IDiaDataSource::loadDataForExe method.
        callback.AddRef();
        // Open and prepare the debug data associated with the .exe/.dll file
        hr = this->pDiaDataSource->loadDataForExe(wszFileName,wszSearchPath,&callback);
        if(FAILED(hr))
        {
            this->ReportError(_T("Error loading debug symbols (IDiaDataSource::loadDataForExe failed)"),hr);
            return FALSE;
        }
    }
    // Open a session for querying symbols
    hr = this->pDiaDataSource->openSession(&this->pDiaSession);
    if(FAILED(hr) || (this->pDiaSession==0))
    {
        this->ReportError(_T("Error loading debug symbols (IDiaDataSource::openSession failed)"),hr);
        return FALSE;
    }
    // Retrieve a reference to the global scope
    hr = this->pDiaSession->get_globalScope(&this->pDiaGlobalSymbol);
    if(FAILED(hr) || (this->pDiaGlobalSymbol==0))
    {
        this->ReportError(_T("Error loading debug symbols (IDiaSession::get_globalScope failed)"),hr);
        return FALSE;
    }
    // Set Machine type for getting correct register names
    if(this->pDiaGlobalSymbol->get_machineType(&dwMachType) == S_OK)
    {
        switch(dwMachType)
        {
        case IMAGE_FILE_MACHINE_I386 : 
            this->MachineType = CV_CFL_80386; 
            break;
        case IMAGE_FILE_MACHINE_IA64 : 
            this->MachineType = CV_CFL_IA64; 
            break;
        case IMAGE_FILE_MACHINE_AMD64 : 
            this->MachineType = CV_CFL_AMD64; 
            break;
        }
    }
    g_RegistersMachineType=this->MachineType;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FindFunctionByRVA
// Object: find function informations according to a RVA
// Parameters :
//     in  : RelativeVirtualAddress : VirtualAddress - pseudo header (PE) image base address
//     out : CFunctionInfos** ppFunctionInfo : found function informations (if any), NULL on failure
//                                             must be delete by caller using delete *ppFunctionInfo;
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::FindFunctionByRVA(ULONGLONG RelativeVirtualAddress,CFunctionInfos** ppFunctionInfo)
{
    *ppFunctionInfo=NULL;
    if (!this->pDiaSession)
    {
        if(!this->LoadDataFromPdb())
            return FALSE;
    }

    IDiaSymbol* pDiaSymbol=NULL;
    HRESULT hr=this->pDiaSession->findSymbolByVA( RelativeVirtualAddress, SymTagFunction, &pDiaSymbol);
    if (FAILED(hr) || (!pDiaSymbol))
        return FALSE;

    CFunctionInfos* pFunctionInfo=new CFunctionInfos();
    if(!pFunctionInfo->Parse(pDiaSymbol))
    {
        delete pFunctionInfo;
        return FALSE;
    }
    *ppFunctionInfo=pFunctionInfo;
    pDiaSymbol->Release();
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse debug informations
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::Parse()
{
    if (!this->LoadDataFromPdb())
        return FALSE;

    return this->ParseModules();
}

//-----------------------------------------------------------------------------
// Name: ParseModules
// Object: parse all compilands
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::ParseModules()
{
    HRESULT hr;
    IDiaEnumSymbols* pEnumSymbols;
    IDiaEnumSymbols* pEnumChildren;
    IDiaSymbol* pCompiland;
    IDiaSymbol* pSymbol;
    ULONG celt = 0;
    CModuleInfos* pModuleInfo;
    BSTR Name;

    // Retrieve the compilands first
    if(FAILED(this->pDiaGlobalSymbol->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols)))
        return FALSE;

    // for each compiland
    while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1) 
    {
        // Retrieve the name of the module
        Name=NULL;
        pCompiland->get_name(&Name);

#if (REMOVE_LINKER_COMPILAND==1)
        // avoid to display "* Linker *" infos
        if (Name)
        {
            if (wcsicmp(Name,LINKER_COMPILAND_NAME)==0)
            {
                SysFreeString(Name);
                pCompiland->Release();
                continue;
            }
        }
#endif

        // create module info
        pModuleInfo=new CModuleInfos();
        // add it to modules list
        this->pLinkListModules->AddItem(pModuleInfo);
        // assume module name is set
        if (Name)
        {
#if (defined(UNICODE)||defined(_UNICODE))
            pModuleInfo->Name=_tcsdup(Name);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(Name,&pModuleInfo->Name);
#endif
            SysFreeString(Name);
        }
        else
            pModuleInfo->Name=_tcsdup(_T(""));

        // Find all the symbols defined in this compiland and get their info
        if(SUCCEEDED(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
        {
            ULONG celt_ = 0;
            while (pEnumChildren->Next(1, &pSymbol, &celt_) == S_OK && celt_ == 1) 
            {
                hr=pModuleInfo->Parse(pSymbol);
                if (FAILED(hr))
                    this->ReportError(_T("Error parsing module information"),hr);
                pSymbol->Release();
            }
            pEnumChildren->Release();
        }
        pCompiland->Release();
    }
    pEnumSymbols->Release();
    return TRUE;
}


// PBYTE SectionBaseAddress
//-----------------------------------------------------------------------------
// Name: GetFunctionLines
// Object: Get function lines number BUT NOT LINES CONTENT
// Parameters :
//     in  : IDiaSymbol* pSymbol : debug symbol
//     inout : CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos : list of CDebugInfosSourcesInfos
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::GetFunctionLines( IDiaSymbol* pSymbol, CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos)
{
    HRESULT hr;
    ULONGLONG length = 0;
    DWORD isect = 0;
    DWORD offset = 0;
    CDebugInfosSourcesInfos* pSourceInfos;
    IDiaEnumLineNumbers* pLines=NULL;
    DWORD celt;
    IDiaLineNumber* pLine=NULL;
    IDiaSourceFile* pSrc;
    BSTR FileName;

    pSymbol->get_addressSection( &isect );
    pSymbol->get_addressOffset( &offset );
    pSymbol->get_length( &length );
    if (( isect == 0 ) || ( length == 0 ))
        return FALSE;

    hr=this->pDiaSession->findLinesByAddr( isect, offset, static_cast<DWORD>( length ), &pLines ) ;

    if ( FAILED(hr) || (!pLines))
        return FALSE;

    pLine = NULL;
    while ( SUCCEEDED( pLines->Next( 1, &pLine, &celt ) ) && celt == 1 )
    {
        if (!pLine)
            break;

        pSourceInfos=new CDebugInfosSourcesInfos();

        //get file name
        pLine->get_sourceFile( &pSrc );
        pSrc->get_fileName(&FileName);
#if (defined(UNICODE)||defined(_UNICODE))
        _tcscpy(pSourceInfos->FileName,FileName);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(FileName,pSourceInfos->FileName,MAX_PATH);
#endif
        SysFreeString(FileName);

        // get line number
        pLine->get_lineNumber( &pSourceInfos->LineNumber );

        // get section index
        pLine->get_addressSection( &pSourceInfos->SectionIndex );

        // get address offset
        pLine->get_addressOffset( &pSourceInfos->Offset );

        // and pSymbol->get_virtualAddress gives relative address from image base <-- the one which interest us and is often called RVA
        pLine->get_virtualAddress( &pSourceInfos->RelativeVirtualAddress );

#ifdef _DEBUG
        TCHAR Output[2*MAX_PATH];
        _stprintf(Output, _T("%s\r\n\tline %d at 0x%x:0x%x\r\n"),pSourceInfos->FileName, pSourceInfos->LineNumber, pSourceInfos->SectionIndex, pSourceInfos->Offset );
        OutputDebugString(Output);                
#endif

        // add source infos to list
        pLinkListSourcesInfos->AddItem(pSourceInfos);

        pSrc->Release();
        pSrc=NULL;
        pLine->Release();
        pLine = NULL;
    }

    if (pLine)
        pLine->Release();

    pLines->Release();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CallBackSourceParseLine
// Object: source code line parsing callback
// Parameters :
//     in  : TCHAR* FileName : parsed file name
//           TCHAR* Line : line content
//           DWORD dwLineNumber : line number (1 based)
//           CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos : linked list of CDebugInfosSourcesInfos
//     return : TRUE to continue parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CDebugInfos::CallBackSourceParseLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(FileName);
    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pItemSourcesInfos;
    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pNextItemSourcesInfos;
    CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos=(CLinkListTemplate<CDebugInfosSourcesInfos>*)UserParam;
    // for each remaining debug info
    for (pItemSourcesInfos=pLinkListSourcesInfos->GetHead();pItemSourcesInfos;pItemSourcesInfos=pNextItemSourcesInfos)
    {
        pNextItemSourcesInfos=pItemSourcesInfos->NextItem;
        // if line number match
        if (pItemSourcesInfos->ItemData->LineNumber==dwLineNumber)
        {
            // fill item LineContent
            pItemSourcesInfos->ItemData->LineContent=_tcsdup(Line);
            // remove item from local list
            pLinkListSourcesInfos->RemoveItem(pItemSourcesInfos);

            // DON'T BREAK : as for inlined function, a single src line can be used more than once in code
        }
    }
    // if no more debug info to fill
    if (pLinkListSourcesInfos->GetItemsCount()==0)
        // stop file parsing
        return FALSE;

    // continue file parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FindLinesByRVA
// Object: find source code lines content and informations according to a RVA
// Parameters :
//     in  : RelativeVirtualAddress : VirtualAddress - pseudo header (PE) image base address
//     out : CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos : linked list of CDebugInfosSourcesInfos
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDebugInfos::FindLinesByRVA(ULONGLONG RelativeVirtualAddress,CLinkListTemplate<CDebugInfosSourcesInfos>* pLinkListSourcesInfos)
{
    if (!this->pDiaSession)
    {
        if(!this->LoadDataFromPdb())
            return FALSE;
    }

    IDiaSymbol* pDiaSymbol=NULL;
    HRESULT hr=this->pDiaSession->findSymbolByVA(RelativeVirtualAddress, SymTagNull, &pDiaSymbol);
    if (FAILED(hr) || (!pDiaSymbol))
        return FALSE;

    pLinkListSourcesInfos->RemoveAllItems();

    if (!this->GetFunctionLines(pDiaSymbol,pLinkListSourcesInfos))
    {
        pDiaSymbol->Release();
        return FALSE;
    }

    pDiaSymbol->Release();

    //////////////////
    // avoid to parse files multiple times --> group CDebugInfosSourcesInfos by file
    //////////////////
    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pItemSourcesInfos;
    CLinkListItemTemplate<CDebugInfosSourcesInfos>* pItemSourcesInfosTmp;
    CLinkListItem* pLocalItemSourceFileNeededLines;
    CLinkList* pLocalLinkListSourceFileNeededLines=new CLinkList(sizeof(SOURCE_FILE_NEEDED_LINES));
    SOURCE_FILE_NEEDED_LINES* pLocalSourceFileNeededLines;
    BOOL FileFound;
    DWORD LineNumber;
    DWORD LineNumberToFillFrom=0;
    BOOL FillLineGapIfNeeded;
    
    for (pItemSourcesInfos=pLinkListSourcesInfos->GetHead();pItemSourcesInfos;pItemSourcesInfos=pItemSourcesInfos->NextItem)
    {
        FileFound=FALSE;

        // find matching filename
        for (pLocalItemSourceFileNeededLines=pLocalLinkListSourceFileNeededLines->Head;pLocalItemSourceFileNeededLines;pLocalItemSourceFileNeededLines=pLocalItemSourceFileNeededLines->NextItem)
        {
            pLocalSourceFileNeededLines=(SOURCE_FILE_NEEDED_LINES*)pLocalItemSourceFileNeededLines->ItemData;
            if (_tcsicmp(pLocalSourceFileNeededLines->FileName,pItemSourcesInfos->ItemData->FileName)==0)
            {
                
                FileFound=TRUE;
                FillLineGapIfNeeded=TRUE;


                // assume there is no hole in source file; else for instructions on multiple lines
                // only last line will be taken into account

                // assume current item is not taken from a next instruction (optimization)
                // if item comes from an optimization, one of the next instructions will have a source code line number lower
                for (pItemSourcesInfosTmp=pItemSourcesInfos->NextItem;pItemSourcesInfosTmp;pItemSourcesInfosTmp=pItemSourcesInfosTmp->NextItem)
                {
                    // if current instruction line number is upper than one of the next instructions line number
                    if (pItemSourcesInfos->ItemData->LineNumber>pItemSourcesInfosTmp->ItemData->LineNumber)
                    {
                        // don't fill source code gap
                        FillLineGapIfNeeded=FALSE;
                        break;
                    }
                }

                if (FillLineGapIfNeeded)
                {
                    for (LineNumber=LineNumberToFillFrom+1;
                        LineNumber<pItemSourcesInfos->ItemData->LineNumber;
                        LineNumber++)
                    {
                        CDebugInfosSourcesInfos* pDebugInfosSources=new CDebugInfosSourcesInfos();
                        _tcscpy(pDebugInfosSources->FileName,pItemSourcesInfos->ItemData->FileName);
                        pDebugInfosSources->SectionIndex=pItemSourcesInfos->ItemData->SectionIndex;
                        pDebugInfosSources->RelativeVirtualAddress=pItemSourcesInfos->ItemData->RelativeVirtualAddress;
                        pDebugInfosSources->Offset=pItemSourcesInfos->ItemData->Offset;
                        pDebugInfosSources->LineNumber=LineNumber;
                        // insert into returned list
                        pLinkListSourcesInfos->InsertItem(pItemSourcesInfos->PreviousItem,pDebugInfosSources);
                        // add to local list
                        pLocalSourceFileNeededLines->pLinkListSourcesInfos->AddItem(pDebugInfosSources);
                    }
                }

                pLocalSourceFileNeededLines->pLinkListSourcesInfos->AddItem(pItemSourcesInfos->ItemData);
                // update LineNumberToFillFrom only if not a next instruction insertion
                if (FillLineGapIfNeeded)
                    LineNumberToFillFrom=pItemSourcesInfos->ItemData->LineNumber;
                break;
            }
        }
        // if file not found
        if (!FileFound)
        {
            // add a new item into pLocalLinkListSourceFileNeededLines list
            pLocalItemSourceFileNeededLines=pLocalLinkListSourceFileNeededLines->AddItem();
            // fill matching informations
            pLocalSourceFileNeededLines=(SOURCE_FILE_NEEDED_LINES*)pLocalItemSourceFileNeededLines->ItemData;
            pLocalSourceFileNeededLines->FileName=pItemSourcesInfos->ItemData->FileName;
            // create a template list
            pLocalSourceFileNeededLines->pLinkListSourcesInfos=new CLinkListTemplate<CDebugInfosSourcesInfos>();
            // template list musn't remove memory on deletion
            pLocalSourceFileNeededLines->pLinkListSourcesInfos->bDeleteObjectMemoryWhenRemovingObjectFromList=FALSE;
            // add item to template list
            pLocalSourceFileNeededLines->pLinkListSourcesInfos->AddItem(pItemSourcesInfos->ItemData);

            LineNumberToFillFrom=pItemSourcesInfos->ItemData->LineNumber;
        }
    }

    //////////////////
    // do file(s) parsing
    //////////////////
    for (pLocalItemSourceFileNeededLines=pLocalLinkListSourceFileNeededLines->Head;pLocalItemSourceFileNeededLines;pLocalItemSourceFileNeededLines=pLocalItemSourceFileNeededLines->NextItem)
    {
        pLocalSourceFileNeededLines=(SOURCE_FILE_NEEDED_LINES*)pLocalItemSourceFileNeededLines->ItemData;
        // parse file. The callback will fill CDebugInfosSourcesInfos.LineContent
        CTextFile::ParseLines(pLocalSourceFileNeededLines->FileName,CallBackSourceParseLine,pLocalSourceFileNeededLines->pLinkListSourcesInfos);
    }

    //////////////////
    // free memory
    //////////////////
    for (pLocalItemSourceFileNeededLines=pLocalLinkListSourceFileNeededLines->Head;pLocalItemSourceFileNeededLines;pLocalItemSourceFileNeededLines=pLocalItemSourceFileNeededLines->NextItem)
    {
        pLocalSourceFileNeededLines=(SOURCE_FILE_NEEDED_LINES*)pLocalItemSourceFileNeededLines->ItemData;
        // assume that pLocalSourceFileNeededLines->pLinkListSourcesInfos->bDeleteObjectMemoryWhenRemovingObjectFromList=FALSE;
        // else destroyed memory will be returned to the caller :(
        delete pLocalSourceFileNeededLines->pLinkListSourcesInfos;
    }
    delete pLocalLinkListSourceFileNeededLines;

    return TRUE;
}
