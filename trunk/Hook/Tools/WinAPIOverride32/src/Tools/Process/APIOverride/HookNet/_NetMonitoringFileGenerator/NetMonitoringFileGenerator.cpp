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
// Object: generate NET. monitoring files from .NET assemblies
//-----------------------------------------------------------------------------

#include "NetMonitoringFileGenerator.h"

using namespace NET;

//using NET::CFunctionInfo;
//using NET::CParameterInfo;

//-----------------------------------------------------------------------------
// Name: CNetMonitoringFileGenerator
// Object: Constructor.
// Parameters :
//     in  : TCHAR* AssemblyFileName
//     out :
//     return : 
//-----------------------------------------------------------------------------
CNetMonitoringFileGenerator::CNetMonitoringFileGenerator(TCHAR* AssemblyFileName)
{
    this->pLinkListFunctions=new CLinkListSimple();

    // fill object assembly name
    _tcsncpy(this->szAssemblyName,AssemblyFileName,MAX_PATH);
    this->szAssemblyName[MAX_PATH-1]=0;

    // fill object assembly unicode name
#if (defined(UNICODE)||defined(_UNICODE))
    _tcscpy(this->wAssemblyName,this->szAssemblyName);
    this->szAssemblyName[MAX_PATH-1]=0;
#else
    CAnsiUnicodeConvert::AnsiToUnicode(AssemblyFileName,this->wAssemblyName,MAX_PATH);
#endif

    this->pIMetaDataImport = NULL;
}

//-----------------------------------------------------------------------------
// Name: CNetMonitoringFileGenerator
// Object: Destructor.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CNetMonitoringFileGenerator::~CNetMonitoringFileGenerator(void)
{
    CLinkListItem* pItem;
    CFunctionInfo* pFunctionInfo;
    for (pItem=this->pLinkListFunctions->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfo=(CFunctionInfo*)pItem->ItemData;
        delete pFunctionInfo;
    }
    delete this->pLinkListFunctions;
}

//-----------------------------------------------------------------------------
// Name: ReportError
// Object: report error to user.
// Parameters :
//     in  : TCHAR* ErrorMsg : error message
//           HRESULT hr : hresult error value
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CNetMonitoringFileGenerator::ReportError(TCHAR* ErrorMsg,HRESULT hr)
{
    UNREFERENCED_PARAMETER(hr);
    MessageBox(NULL,ErrorMsg,_T("Error"),MB_OK|MB_TOPMOST|MB_ICONERROR);
}

//-----------------------------------------------------------------------------
// Name: ParseMethods
// Object: parse method
// Parameters :
//     in  : mdTypeDef inTypeDef : method type def
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::ParseMethods(mdTypeDef inTypeDef)
{
    HCORENUM methodEnum = NULL;
    mdToken methodsToken[CNetMonitoringFileGenerator_ENUM_BUFFER_SIZE];
    ULONG count;
    HRESULT hr;
    CFunctionInfo* pFunctionInfo;

    // for each method in class
    while (SUCCEEDED(hr = pIMetaDataImport->EnumMethods( &methodEnum, inTypeDef,
        methodsToken, CNetMonitoringFileGenerator_ENUM_BUFFER_SIZE, &count)) &&
        count > 0)
    {
        for (ULONG i = 0; i < count; i++)
        {
            // parse method
            pFunctionInfo=CFunctionInfo::Parse(pIMetaDataImport,inTypeDef,methodsToken[i]);
            // on parsing success
            if (pFunctionInfo)
            {
                // add method infos to list
                this->pLinkListFunctions->AddItem(pFunctionInfo);
            }
        }
    }
    pIMetaDataImport->CloseEnum( methodEnum);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseGlobalFunctions
// Object: parse global functions
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::ParseGlobalFunctions()
{
    return this->ParseMethods(mdTokenNil);
}

//-----------------------------------------------------------------------------
// Name: GetClassName
// Object: get class name
// Parameters :
//     in  : mdTypeDef inTypeDef : type def
//           DWORD pszClassNameMaxSize : class name max size in WCHAR
//     in/out : WCHAR* pszClassName : classe name
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::GetClassName(mdTypeDef inTypeDef,WCHAR* pszClassName,DWORD pszClassNameMaxSize)
{
    HRESULT hr;
    ULONG nameLen;
    DWORD flags;
    mdToken extends;

    hr = pIMetaDataImport->GetTypeDefProps(
                                inTypeDef,              // [IN] TypeDef token for inquiry.
                                pszClassName,           // [OUT] Put name here.
                                pszClassNameMaxSize,    // [IN] size of name buffer in wide chars.
                                &nameLen,               // [OUT] put size of name (wide chars) here.
                                &flags,                 // [OUT] Put flags here.
                                &extends);              // [OUT] Put base class TypeDef/TypeRef here.

    if (FAILED(hr))
    {
        this->ReportError(_T("GetTypeDefProps failed."), hr);
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseTypeDefInfo
// Object: parse typedef (currently only methods)
// Parameters :
//     in  : mdTypeDef inTypeDef : type def
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::ParseTypeDefInfo(mdTypeDef inTypeDef)
{
    return this->ParseMethods(inTypeDef);
}

//-----------------------------------------------------------------------------
// Name: ParseAssembly
// Object: display the information about all typedefs in the object.
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::ParseTypeDefs()
{
    HCORENUM typeDefEnum = NULL;
    mdTypeDef typeDefs[CNetMonitoringFileGenerator_ENUM_BUFFER_SIZE];
    ULONG count;
    HRESULT hr;

    // for each type def, ParseTypeDefInfo
    while (SUCCEEDED(hr = pIMetaDataImport->EnumTypeDefs( &typeDefEnum,
        typeDefs, CNetMonitoringFileGenerator_ENUM_BUFFER_SIZE, &count)) &&
        count > 0)
    {
        for (ULONG i = 0; i < count; i++)
        {
            this->ParseTypeDefInfo(typeDefs[i]);
        }
    }
    pIMetaDataImport->CloseEnum( typeDefEnum);

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ParseAssembly
// Object: parse assembly file contained in this->wAssemblyName
// Parameters :
//     in  : 
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::ParseAssembly()
{
   
    IMetaDataDispenserEx* pDispenser = NULL;
    BOOL bRet=TRUE;
    BOOL bComShouldBeUninitialized;
    HRESULT hr;
    WCHAR szScope[MAX_PATH+50];

    // Init and run.
    bComShouldBeUninitialized=(CoInitialize(0)==S_OK);

    hr = CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_INPROC_SERVER, 
        IID_IMetaDataDispenserEx, (void **) &pDispenser);
    if(FAILED(hr)) 
    {
        this->ReportError(_T("Unable to CoCreate Meta-data Dispenser"), hr);
        return FALSE;
    }

    wcscpy(szScope, L"file:");
    wcscat(szScope, this->wAssemblyName);

    // Attempt to open scope on given file
    hr = pDispenser->OpenScope(szScope, 0, IID_IMetaDataImport, (IUnknown**)&pIMetaDataImport);

    if (hr == CLDB_E_BADUPDATEMODE)
    {
        VARIANT value;
        V_VT(&value) = VT_UI4;
        V_UI4(&value) = MDUpdateIncremental;
        if (FAILED(hr = pDispenser->SetOption(MetaDataSetUpdate, &value)))
        {
            this->ReportError(_T("SetOption failed."), hr);
            pDispenser->Release();
            return FALSE;
        }
        hr = pDispenser->OpenScope(szScope, 0, IID_IMetaDataImport, (IUnknown**)&pIMetaDataImport);
    }
    if (FAILED(hr))
    {
        this->ReportError(_T("OpenScope failed"), hr);
        pDispenser->Release();
        return FALSE;
    }

    bRet=bRet && this->ParseGlobalFunctions();
    bRet=bRet && this->ParseTypeDefs();

    pIMetaDataImport->Release();
    pDispenser->Release();

    if (bComShouldBeUninitialized)
        CoUninitialize();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GenerateMonitoringFile
// Object: generate monitoring file
// Parameters :
//     in  : TCHAR* OutputFileName
//     out :
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CNetMonitoringFileGenerator::GenerateMonitoringFile(TCHAR* OutputFileName)
{
    TCHAR sz[2*MAX_PATH];
    TCHAR* psz;
    CLinkListItem* pItemFunction;
    CLinkListItem* pItemParam;
    CFunctionInfo* pFunctionInfo;
    CParameterInfo* pParamInfo;
    HANDLE hFile;
    // create file
    if (!CTextFile::CreateTextFile(OutputFileName,&hFile))
    {
        _stprintf(sz,_T("Error creating file %s"),OutputFileName);
        this->ReportError(sz,GetLastError());
        return FALSE;
    }

    // for each function
    for (pItemFunction=this->pLinkListFunctions->Head;pItemFunction;pItemFunction=pItemFunction->NextItem)
    {
        pFunctionInfo=(CFunctionInfo*)pItemFunction->ItemData;

    
        // .NET@AssemblyName@FunctionToken|
        _stprintf(sz,
                  _T(".NET@%s@0x%.8X|"),
                  szAssemblyName,
                  pFunctionInfo->FunctionToken);
#ifdef _WIN64
        to check : mdToken is still UINT32 
#endif
        CTextFile::WriteText(hFile,sz);

        // return
        CTextFile::WriteText(hFile,CSupportedParameters::GetParamName(pFunctionInfo->pReturnInfo->GetWinAPIOverrideType()));

        // calling convention
        if (pFunctionInfo->GetStringCallingConvention(sz,MAX_PATH))
            CTextFile::WriteText(hFile,sz);

        CTextFile::WriteText(hFile,_T(" "));

        // function name
        CTextFile::WriteText(hFile,pFunctionInfo->szName);

        CTextFile::WriteText(hFile,_T("("));
        // for each parameter
        for (pItemParam=pFunctionInfo->pParameterInfoList->Head;pItemParam;pItemParam=pItemParam->NextItem)
        {
            pParamInfo=(CParameterInfo*)pItemParam->ItemData;
            // if not first param
            if (pItemParam!=pFunctionInfo->pParameterInfoList->Head)
                CTextFile::WriteText(hFile,_T(","));

            // get type
            psz=CSupportedParameters::GetParamName(pParamInfo->GetWinAPIOverrideType() & SIMPLE_TYPE_FLAG_MASK);

            // get name (in case of typed arg)
            pParamInfo->GetName(sz,MAX_PATH);

            // if type and name are different
            if (_tcscmp(psz,sz)!=0)
            {
                // show both
                CTextFile::WriteText(hFile,psz);
                CTextFile::WriteText(hFile,_T(" "));
                CTextFile::WriteText(hFile,sz);
            }
            else // only show one
                CTextFile::WriteText(hFile,sz);
        }
        CTextFile::WriteText(hFile,_T(")"));

        // end line
        CTextFile::WriteText(hFile,_T("\r\n"));
    }
    // close file
    CloseHandle(hFile);
    return TRUE;
}