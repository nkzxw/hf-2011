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
#include "functioninfoex.h"

namespace NET
{

CFunctionInfoEx::CFunctionInfoEx(void)
{
    this->FunctionId=0;
    this->ModuleId=0;
    this->ClassId=0;
    this->AsmCodeStart=0;
    this->AsmCodeSize=0;
    this->pItemApiInfo=NULL;
    this->pICorProfilerInfo=NULL;
    this->BaseAddress=0;
    *this->szModule=0;
}

CFunctionInfoEx::~CFunctionInfoEx(void)
{

}

BOOL CFunctionInfoEx::GetILFunctionBody(OUT PBYTE* ppMethodHeader,OUT ULONG* pcbMethodSize)
{
    if (!this->pICorProfilerInfo)
        return FALSE;
    return SUCCEEDED(this->pICorProfilerInfo->GetILFunctionBody(
                                                                this->ModuleId,
                                                                this->FunctionToken,
                                                                (LPCBYTE*)ppMethodHeader,
                                                                pcbMethodSize));
}

BOOL CFunctionInfoEx::GetModuleHandle(OUT PBYTE* pBaseAddress)
{
    TCHAR sz[MAX_PATH];
    return this->GetModuleNameAndHandle(sz,MAX_PATH,pBaseAddress);
}
BOOL CFunctionInfoEx::GetModuleName(OUT TCHAR* pszModuleName,DWORD ModuleNameMaxLen)
{
    PBYTE BaseAddress;
    return this->GetModuleNameAndHandle(pszModuleName,ModuleNameMaxLen,&BaseAddress);
}

BOOL CFunctionInfoEx::GetModuleNameAndHandle(OUT TCHAR* pszModuleName,DWORD ModuleNameMaxLen,OUT PBYTE* pBaseAddress)
{
    *pszModuleName=0;
    ULONG cchName;
    WCHAR* pwszName;
    AssemblyID AssemblyId;
    HRESULT hRes;
    BOOL bRet=FALSE;

#if (defined(UNICODE)||defined(_UNICODE))
    pwszName=pszModuleName;
#else
    pwszName=(WCHAR*)_alloca(ModuleNameMaxLen*sizeof(WCHAR));
#endif

    hRes=this->pICorProfilerInfo->GetModuleInfo(this->ModuleId,
                                                (LPCBYTE*)pBaseAddress,
                                                ModuleNameMaxLen,
                                                &cchName,
                                                pwszName,
                                                &AssemblyId);
    if (SUCCEEDED(hRes))
    {
        bRet=TRUE;
#if (!defined(UNICODE)&&!defined(_UNICODE))
        CAnsiUnicodeConvert::UnicodeToAnsi(pwszName,pszModuleName,ModuleNameMaxLen);
#endif
    }
    return bRet;
}

FunctionID CFunctionInfoEx::GetFunctionId()
{
    return this->FunctionId;
}

ClassID CFunctionInfoEx::GetClassId()
{
    return this->ClassId;
}


// see "MetaData Unmanaged API.doc" file available inside 
// Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\Tool Developers Guide\docs\ 
CFunctionInfoEx* CFunctionInfoEx::Parse(ICorProfilerInfo* pICorProfilerInfo,FunctionID FunctionId,BOOL bGetGeneratedCodeInformations)
{
    HRESULT hResult = S_OK;
    mdTypeDef ClassToken=NULL;
    IMetaDataImport *pMetaDataImport=NULL;
    mdToken	Token;
    CFunctionInfoEx* pFunctionInfo;

    pFunctionInfo=new CFunctionInfoEx();
    if (!pFunctionInfo)
        return NULL;

    pFunctionInfo->pICorProfilerInfo=pICorProfilerInfo;

    // check function id
    if (FunctionId==NULL)
    {
        // This corresponds to an unmanaged frame
        delete pFunctionInfo;
        return NULL;
    }
    pFunctionInfo->FunctionId=FunctionId;

    if (bGetGeneratedCodeInformations)
    {
        // get build code informations
        hResult = pICorProfilerInfo->GetCodeInfo(FunctionId,(LPCBYTE*)&pFunctionInfo->AsmCodeStart,&pFunctionInfo->AsmCodeSize);
        if (FAILED(hResult))
        {
            // if we can't access AsmCodeStart, we can't hook function --> return NULL
            delete pFunctionInfo;
            return NULL;
        }
    }

    // Get MetadataImport interface and the metadata token 
    hResult = pICorProfilerInfo->GetTokenAndMetaDataFromFunction(FunctionId, 
                                                        IID_IMetaDataImport, 
                                                        (IUnknown **)&pMetaDataImport,
                                                        &Token);
    if (FAILED(hResult))
    {
        // no meta data import --> we can't parse parameters
        delete pFunctionInfo;
        return NULL;
    }

    // Get classID 
    pFunctionInfo->ClassId=NULL;
    pFunctionInfo->ModuleId=NULL;
    hResult = pICorProfilerInfo->GetFunctionInfo( FunctionId,
                                                &pFunctionInfo->ClassId,
                                                &pFunctionInfo->ModuleId,
                                                NULL);
    // class id and module id are not important to monitor (only provide class name)
    // so we avoid to make function fail for this
    //if (FAILED(hResult))
    //{
    //    pMetaDataImport->Release();
    //    delete pFunctionInfo;
    //    return NULL;
    //}

    if (pFunctionInfo->ClassId)
    {
        // get class informations
        hResult = pICorProfilerInfo->GetClassIDInfo(pFunctionInfo->ClassId,NULL,&ClassToken);
        // ClassToken is not important to monitor (only provide class name)
        // so we avoid to make function fail for this
        //if (FAILED(hResult))
        //{
        //    pMetaDataImport->Release();
        //    delete pFunctionInfo;
        //    return NULL;
        //}
    }


    if (!pFunctionInfo->mParse(pMetaDataImport,ClassToken,Token))
    {
        pMetaDataImport->Release();
        delete pFunctionInfo;
        return NULL;
    }

    // release pMDImport
    pMetaDataImport->Release();

    // fill fields to allow fast access
    pFunctionInfo->GetModuleNameAndHandle(pFunctionInfo->szModule,MAX_PATH,&pFunctionInfo->BaseAddress);

    return pFunctionInfo;
}

}