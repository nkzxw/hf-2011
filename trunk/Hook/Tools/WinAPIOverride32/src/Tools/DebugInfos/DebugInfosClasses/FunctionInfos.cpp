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
// Object: manages debug informations for functions
//-----------------------------------------------------------------------------
#include "functioninfos.h"

CFunctionInfos::CFunctionInfos(void)
{
    this->Name=NULL;
    this->UndecoratedName=NULL;
    this->RelativeVirtualAddress=0;
    this->Length=0;
    this->SectionIndex=0;
    this->Offset=0;
    this->CallingConvention=(DWORD)-1;
    this->pLinkListParams=new CLinkListSimple();
    this->pLinkListBlocks=new CLinkListSimple();
    this->pLinkListLabels=new CLinkListSimple();
    this->pReturnInfos=NULL;
    this->Token=0;
}

CFunctionInfos::~CFunctionInfos(void)
{
    if (this->Name)
        free(this->Name);
    if (this->UndecoratedName)
        free(this->UndecoratedName);

    CLinkListItem* pItem;
    CFunctionBlockInfos* pFunctionBlockInfos;
    CTypeInfos* pParamInfos;
    CLabelInfos* pLabelInfos;

    if (this->pReturnInfos)
        delete this->pReturnInfos;

    // for each item of this->pLinkListParams
    for (pItem=this->pLinkListParams->Head;pItem;pItem=pItem->NextItem)
    {
        pParamInfos=(CTypeInfos*)pItem->ItemData;
        // delete item
        delete pParamInfos;
    }
    delete this->pLinkListParams;

    // for each item of this->pLinkListBlocks
    for (pItem=this->pLinkListBlocks->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionBlockInfos=(CFunctionBlockInfos*)pItem->ItemData;
        // delete item
        delete pFunctionBlockInfos;
    }
    delete this->pLinkListBlocks;

    // for each item of this->pLinkListLabels
    for (pItem=this->pLinkListLabels->Head;pItem;pItem=pItem->NextItem)
    {
        pLabelInfos=(CLabelInfos*)pItem->ItemData;
        // delete item
        delete pLabelInfos;
    }
    delete this->pLinkListLabels;

}
BOOL CFunctionInfos::GetCallingConvention(TCHAR* StrCallingConvention,SIZE_T StrCallingConventionMaxSize)
{
    *StrCallingConvention=0;
    StrCallingConvention[StrCallingConventionMaxSize-1]=0;
    StrCallingConventionMaxSize--;

    switch(this->CallingConvention)
    {
    case CV_CALL_NEAR_C      : // near right to left push, caller pops stack
    case CV_CALL_FAR_C       : // far right to left push, caller pops stack
        _tcsncpy(StrCallingConvention,_T("__cdecl"),StrCallingConventionMaxSize);
        break;

    case CV_CALL_NEAR_FAST   : // near left to right push with regs, callee pops stack
    case CV_CALL_FAR_FAST    : // far left to right push with regs, callee pops stack
        // left to right push ??? 
        // MSDN __fastcall Specifics
        // Some of a __fastcall function’s arguments are passed in registers 
        // x86 Specific —> ECX and EDX END x86 Specific, 
        // and the rest are pushed onto the stack FROM RIGHT TO LEFT. 
        // The called routine pops these arguments from the stack before it returns. 
        // Typically, /Gr decreases execution time.

        // use __fastcall keyword anyway
        _tcsncpy(StrCallingConvention,_T("__fastcall"),StrCallingConventionMaxSize);
        break;

    case CV_CALL_NEAR_STD    : // near standard call
    case CV_CALL_FAR_STD     : // far standard call
        _tcsncpy(StrCallingConvention,_T("__stdcall"),StrCallingConventionMaxSize);
        break;

    case CV_CALL_THISCALL    :  // this call (this passed in register)
        _tcsncpy(StrCallingConvention,_T("__thiscall"),StrCallingConventionMaxSize);
        break;

    case CV_CALL_NEAR_PASCAL : // near left to right push, callee pops stack
    case CV_CALL_FAR_PASCAL  : // far left to right push, callee pops stack

    case CV_CALL_NEAR_SYS    :  // near sys call
    case CV_CALL_FAR_SYS     :  // far sys call

    case CV_CALL_SKIPPED     :  // skipped (unused) call index
    case CV_CALL_MIPSCALL    :  // Mips call
    case CV_CALL_GENERIC     :  // Generic call sequence
    case CV_CALL_ALPHACALL   :  // Alpha call
    case CV_CALL_PPCCALL     :  // PPC call
    case CV_CALL_SHCALL      :  // Hitachi SuperH call
    case CV_CALL_ARMCALL     :  // ARM call
    case CV_CALL_AM33CALL    :  // AM33 call
    case CV_CALL_TRICALL     :  // TriCore Call
    case CV_CALL_SH5CALL     :  // Hitachi SuperH-5 call
    case CV_CALL_M32RCALL    :  // M32R Call
    case CV_CALL_CLRCALL     :  // clr call
    case CV_CALL_RESERVED    :  // first unused call enumeration
        return FALSE;
    default:
        // this->CallingConvention should be (DWORD)-1
        // try to extract informations from undecorated name
        if (this->UndecoratedName)
        {
            if (_tcsstr(this->UndecoratedName,_T("__cdecl")))
                _tcsncpy(StrCallingConvention,_T("__cdecl"),StrCallingConventionMaxSize);
            else if (_tcsstr(this->UndecoratedName,_T("__thiscall")))
                _tcsncpy(StrCallingConvention,_T("__thiscall"),StrCallingConventionMaxSize);
            else if (_tcsstr(this->UndecoratedName,_T("__stdcall")))
                _tcsncpy(StrCallingConvention,_T("__stdcall"),StrCallingConventionMaxSize);      
            else if (_tcsstr(this->UndecoratedName,_T("__fastcall")))
                _tcsncpy(StrCallingConvention,_T("__fastcall"),StrCallingConventionMaxSize); 
        }
        return FALSE;
    }
    return TRUE;
}

BOOL CFunctionInfos::Parse(IDiaSymbol *pSymbol)
{
    IDiaSymbol *pReturnSymbol;
    IDiaSymbol *pFunctionTypeSymbol;
    HRESULT hResult;
    DWORD dwLocationType;
    if(FAILED(pSymbol->get_locationType(&dwLocationType)))
        return FALSE;

    // function address should be static
    switch (dwLocationType)
    {
    case LocIsStatic:
        // get relative virtual address
        // pSymbol->get_relativeVirtualAddress gives relative address from section
        // and pSymbol->get_virtualAddress gives relative address from image base <-- the one which interest us and is often called RVA
        pSymbol->get_virtualAddress(&this->RelativeVirtualAddress);
        pSymbol->get_addressSection(&this->SectionIndex);
        pSymbol->get_addressOffset(&this->Offset);
        break;
    case LocInMetaData:
        pSymbol->get_token(&this->Token);
        break;
#ifdef _DEBUG
    default:
        if (IsDebuggerPresent())
            DebugBreak();
        break;
#endif
    }

    pSymbol->get_length(&this->Length);

    BSTR Name=NULL;
    BSTR UndecoratedName;
    pSymbol->get_undecoratedName(&UndecoratedName);
    if (UndecoratedName)
    {
#if (defined(UNICODE)||defined(_UNICODE))
        this->UndecoratedName=_tcsdup(UndecoratedName);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(UndecoratedName,&this->UndecoratedName);
#endif
        SysFreeString(UndecoratedName);
    }
    
    pSymbol->get_name(&Name);
    if (Name)
    {
#if (defined(UNICODE)||defined(_UNICODE))
        this->Name=_tcsdup(Name);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(Name,&this->Name);
#endif
        SysFreeString(Name);
    }
    else
        this->Name=_tcsdup(_T(""));

    CFunctionBlockInfos* pFunctionBlockInfos;
    pFunctionBlockInfos=new CFunctionBlockInfos();
    this->pLinkListBlocks->AddItem(pFunctionBlockInfos);
    pFunctionBlockInfos->SectionIndex=this->SectionIndex;
    pFunctionBlockInfos->Offset=this->Offset;
    pFunctionBlockInfos->RelativeVirtualAddress=this->RelativeVirtualAddress;
    pFunctionBlockInfos->Length=this->Length;

    pFunctionTypeSymbol=NULL;
    pSymbol->get_type(&pFunctionTypeSymbol);
    if(pFunctionTypeSymbol)
    {
        pReturnSymbol=NULL;
        pFunctionTypeSymbol->get_type(&pReturnSymbol);
        pFunctionTypeSymbol->get_callingConvention(&this->CallingConvention);
        if (pReturnSymbol)
        {
            this->pReturnInfos=new CTypeInfos();
            if (!this->pReturnInfos->Parse(pReturnSymbol))
            {
                delete this->pReturnInfos;
                this->pReturnInfos=NULL;
            }
            pReturnSymbol->Release();
        }
        pFunctionTypeSymbol->Release();
    }

    IDiaEnumSymbols* pEnumChildren;
    pEnumChildren=NULL;
    hResult=pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren);
    if( FAILED(hResult) || (pEnumChildren==NULL))
        return FALSE;

    IDiaSymbol* pChild;
    ULONG celt = 0;

    while((pEnumChildren->Next(1, &pChild, &celt) == S_OK) && (celt == 1) )
    {
        this->Parse(pChild,pFunctionBlockInfos);
        pChild->Release();
    }

    pEnumChildren->Release();

    return TRUE; 
}

BOOL CFunctionInfos::Parse(IDiaSymbol *pSymbol,CFunctionBlockInfos* pBlockInfo)
{
    DWORD dwSymTag;
    DWORD dwDataKind;
    CTypeInfos* pTypeInfos;

    if(FAILED(pSymbol->get_symTag(&dwSymTag)))
        return FALSE;

    switch(dwSymTag)
    {
    case SymTagFuncDebugStart:
    case SymTagFuncDebugEnd:
        // address of function without calling convention and stack checking
        // -> only user code, not compiler added data
        // PrintLocation(pSymbol);
        break;

    case SymTagData:// param or local

        pSymbol->get_dataKind(&dwDataKind);
        if (CTypeInfos::IsParam(dwDataKind))
        {
            pTypeInfos=new CTypeInfos();
            if (pTypeInfos->Parse(pSymbol))
            {
#ifdef _DEBUG
                if (!pTypeInfos->bFunction)
                {
                    if ( pTypeInfos->TypeName == 0) 
                    {
                        if (::IsDebuggerPresent())
                        {
                            ::DebugBreak();
                        }
                    }
                    else if (*pTypeInfos->TypeName == 0)
                    {
                        if (::IsDebuggerPresent())
                        {
                            ::DebugBreak();
                        }
                    }
                }
#endif
                this->pLinkListParams->AddItem(pTypeInfos);
            }
            else
                delete pTypeInfos;
        }
        else
        {
            pTypeInfos=new CTypeInfos();
            if (pTypeInfos->Parse(pSymbol))
                pBlockInfo->pLinkListVars->AddItem(pTypeInfos);
            else
                delete pTypeInfos;
        }
        break;
    case SymTagBlock:
        CFunctionBlockInfos* pFunctionBlockInfos;
        pFunctionBlockInfos=new CFunctionBlockInfos();
        if (pFunctionBlockInfos->Parse(pSymbol))
            this->pLinkListBlocks->AddItem(pFunctionBlockInfos);
        else
            delete pFunctionBlockInfos;
        break;
    case SymTagLabel:
        CLabelInfos* pLabelInfos=new CLabelInfos();
        if (pLabelInfos->Parse(pSymbol))
            this->pLinkListLabels->AddItem(pLabelInfos);
        else
            delete pLabelInfos;
        break;
    }
    return TRUE;
}