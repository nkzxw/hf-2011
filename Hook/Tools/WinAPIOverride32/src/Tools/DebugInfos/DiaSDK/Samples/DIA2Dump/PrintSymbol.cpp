// PrintSymbol.cpp : Defines the printing procedures for the symbols
//
// This is a part of the Debug Interface Access SDK 
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Debug Interface Access SDK and related electronic documentation 
// provided with the library.
// See these sources for detailed information regarding the
// Debug Interface Access SDK API.
//

#include "stdafx.h"
#include "malloc.h"
#include "dia2.h"
#include "regs.h"
#include "PrintSymbol.h"

// Basic types
const wchar_t * const rgBaseType[] = {
    L"<NoType>",             // btNoType = 0,
    L"void",                 // btVoid = 1,
    L"char",                 // btChar = 2,
    L"wchar_t",              // btWChar = 3,
    L"signed char",
    L"unsigned char",
    L"int",                  // btInt = 6,
    L"unsigned int",         // btUInt = 7,
    L"float",                // btFloat = 8,
    L"<BCD>",                // btBCD = 9,
    L"bool",                 // btBool = 10,
    L"short",
    L"unsigned short",
    L"long",                 // btLong = 13,
    L"unsigned long",        // btULong = 14,
    L"__int8",
    L"__int16",
    L"__int32",
    L"__int64",
    L"__int128",
    L"unsigned __int8",
    L"unsigned __int16",
    L"unsigned __int32",
    L"unsigned __int64",
    L"unsigned __int128",
    L"<currency>",           // btCurrency = 25,
    L"<date>",               // btDate = 26,
    L"VARIANT",              // btVariant = 27,
    L"<complex>",            // btComplex = 28,
    L"<bit>",                // btBit = 29,
    L"BSTR",                 // btBSTR = 30,
    L"HRESULT"              // btHresult = 31
};

// Tags returned by Dia
const wchar_t * const rgTags[] = {
    L"(SymTagNull)",
    L"Executable (Global)",
    L"Compiland",
    L"CompilandDetails",
    L"CompilandEnv",
    L"Function",
    L"Block",
    L"Data",
    L"Annotation",
    L"Label",
    L"PublicSymbol",
    L"UDT",
    L"Enum",
    L"FunctionType",
    L"PointerType",
    L"ArrayType",
    L"BaseType",
    L"Typedef",
    L"BaseClass",
    L"Friend",
    L"FunctionArgType",
    L"FuncDebugStart",
    L"FuncDebugEnd",
    L"UsingNamespace",
    L"VTableShape",
    L"VTable",
    L"Custom",
    L"Thunk",
    L"CustomType",
    L"ManagedType",
    L"Dimension",
    L"???(1f)",
    L"???(1g)",
    L"???(1h)",
    L"???(20)"
};


// Processors
const wchar_t * const rgFloatPackageStrings[] = {
    L"hardware processor (80x87 for Intel processors)",  // CV_CFL_NDP
    L"emulator",                                         // CV_CFL_EMU
    L"altmath",                                          // CV_CFL_ALT
    L"???"
};

const wchar_t * const rgProcessorStrings[] = {
    L"8080",                            //  CV_CFL_8080
    L"8086",                            //  CV_CFL_8086
    L"80286",                           //  CV_CFL_80286
    L"80386",                           //  CV_CFL_80386
    L"80486",                           //  CV_CFL_80486
    L"Pentium",                         //  CV_CFL_PENTIUM
    L"Pentium Pro/Pentium II",          //  CV_CFL_PENTIUMII/CV_CFL_PENTIUMPRO
    L"Pentium III",                     //  CV_CFL_PENTIUMIII
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"MIPS (Generic)",                  //  CV_CFL_MIPSR4000
    L"MIPS16",                          //  CV_CFL_MIPS16
    L"MIPS32",                          //  CV_CFL_MIPS32
    L"MIPS64",                          //  CV_CFL_MIPS64
    L"MIPS I",                          //  CV_CFL_MIPSI
    L"MIPS II",                         //  CV_CFL_MIPSII
    L"MIPS III",                        //  CV_CFL_MIPSIII
    L"MIPS IV",                         //  CV_CFL_MIPSIV
    L"MIPS V",                          //  CV_CFL_MIPSV
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"M68000",                          //  CV_CFL_M68000
    L"M68010",                          //  CV_CFL_M68010
    L"M68020",                          //  CV_CFL_M68020
    L"M68030",                          //  CV_CFL_M68030
    L"M68040",                          //  CV_CFL_M68040
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"Alpha 21064",                     // CV_CFL_ALPHA, CV_CFL_ALPHA_21064
    L"Alpha 21164",                     // CV_CFL_ALPHA_21164
    L"Alpha 21164A",                    // CV_CFL_ALPHA_21164A
    L"Alpha 21264",                     // CV_CFL_ALPHA_21264
    L"Alpha 21364",                     // CV_CFL_ALPHA_21364
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"PPC 601",                         // CV_CFL_PPC601
    L"PPC 603",                         // CV_CFL_PPC603
    L"PPC 604",                         // CV_CFL_PPC604
    L"PPC 620",                         // CV_CFL_PPC620
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"SH3",                             // CV_CFL_SH3
    L"SH3E",                            // CV_CFL_SH3E
    L"SH3DSP",                          // CV_CFL_SH3DSP
    L"SH4",                             // CV_CFL_SH4
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"ARM3",                            // CV_CFL_ARM3
    L"ARM4",                            // CV_CFL_ARM4
    L"ARM4T",                           // CV_CFL_ARM4T
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"Omni",                            // CV_CFL_OMNI
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"IA64",                            // CV_CFL_IA64
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"CEE",                            // CV_CFL_CEE
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"AM33",                            // CV_CFL_AM33
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"M32R",                            // CV_CFL_M32R
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"TRICORE",                         // CV_CFL_TRICORE
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"X64",                             // CV_CFL_X64
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"EBC",                             // CV_CFL_EBC
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"THUMB",                           // CV_CFL_THUMB
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
    L"???",
};

const wchar_t * const rgDataKind[] =
{
    L"Unknown",
    L"Local",
    L"Static Local",
    L"Param",
    L"Object Ptr",
    L"File Static",
    L"Global",
    L"Member",
    L"Static Member",
    L"Constant",
};

const wchar_t * const rgUdtKind[] =
{
    L"struct",
    L"class",
    L"union",
    L"enum",
};

const wchar_t * const rgAccess[] =
{
    L"",                     // No access specifier
    L"private",
    L"protected",
    L"public"
};

const wchar_t * const rgCallingConvention[] =
{
    L"CV_CALL_NEAR_C      ",
    L"CV_CALL_FAR_C       ",
    L"CV_CALL_NEAR_PASCAL ",
    L"CV_CALL_FAR_PASCAL  ",
    L"CV_CALL_NEAR_FAST   ",
    L"CV_CALL_FAR_FAST    ",
    L"CV_CALL_SKIPPED     ",
    L"CV_CALL_NEAR_STD    ",
    L"CV_CALL_FAR_STD     ",
    L"CV_CALL_NEAR_SYS    ",
    L"CV_CALL_FAR_SYS     ",
    L"CV_CALL_THISCALL    ",
    L"CV_CALL_MIPSCALL    ",
    L"CV_CALL_GENERIC     ",
    L"CV_CALL_ALPHACALL   ",
    L"CV_CALL_PPCCALL     ",
    L"CV_CALL_SHCALL      ",
    L"CV_CALL_ARMCALL     ",
    L"CV_CALL_AM33CALL    ",
    L"CV_CALL_TRICALL     ",
    L"CV_CALL_SH5CALL     ",
    L"CV_CALL_M32RCALL    ",
    L"CV_CALL_RESERVED    "
};

const wchar_t * const rgLanguage[] = {
  L"C",
  L"CPP",
  L"FORTRAN",
  L"MASM",
  L"PASCAL",
  L"BASIC",
  L"COBOL",
  L"LINK",
  L"CVTRES",
  L"CVTPGD",
  L"C#",
  L"VISUAL BASIC",
  L"ILASM"
};

const wchar_t * const rgLocationTypeString[] = {
  L"NULL",
  L"static",
  L"TLS",
  L"RegRel",
  L"ThisRel",
  L"Enregistered",
  L"BitField",
  L"Slot",
  L"IL Relative",
  L"In MetaData",
  L"Constant"
};


////////////////////////////////////////////////////////////
// Print a public symbol info: name, VA, RVA, SEG:OFF
//
void PrintPublicSymbol(IDiaSymbol* pSymbol){
  DWORD dwSymTag;
  DWORD dwRVA, dwSeg, dwOff;
  ULONGLONG ulVA;
  BSTR wszName;
  
  if(pSymbol->get_symTag(&dwSymTag) != S_OK){
    return;
  }
  if ( pSymbol->get_relativeVirtualAddress( &dwRVA ) != S_OK ){
    dwRVA = 0xFFFFFFFF;
  }
  if ( pSymbol->get_virtualAddress( &ulVA ) != S_OK ){
     ulVA = 0xFFFFFFFF;
  }
  pSymbol->get_addressSection(&dwSeg);
  pSymbol->get_addressOffset(&dwOff);

  wprintf(L"%s: [%08x][%08x][%04x:%08x] ", rgTags[dwSymTag], (unsigned long)(ulVA), dwRVA, dwSeg, dwOff);
  
  if(dwSymTag == SymTagThunk){
    if(pSymbol->get_name(&wszName) == S_OK){
      wprintf(L"%s\n",wszName);
      SysFreeString(wszName);
    }else{
      pSymbol->get_targetSection(&dwSeg);
      pSymbol->get_targetOffset(&dwOff);
      wprintf(L"target -> [%04X:%08X]\n", dwSeg, dwOff);
    }
  }else{
    // must be a function or a data symbol
    BSTR wszUndname;
    if(pSymbol->get_name(&wszName) == S_OK){
      if(pSymbol->get_undecoratedName(&wszUndname) == S_OK){
        wprintf(L"%s(%s)\n", wszName, wszUndname);
        SysFreeString(wszUndname);
      }else{
        wprintf(L"%ws\n", wszName);
      }
      SysFreeString(wszName);
    }
  }
}

////////////////////////////////////////////////////////////
// Print a global symbol info: name, VA, RVA, SEG:OFF
//
void PrintGlobalSymbol(IDiaSymbol* pSymbol){
  DWORD dwSymTag;
  DWORD dwRVA, dwSeg, dwOff;
  ULONGLONG ulVA;
  
  if(pSymbol->get_symTag(&dwSymTag) != S_OK){
    return;
  }
  if ( pSymbol->get_relativeVirtualAddress( &dwRVA ) != S_OK ){
    dwRVA = 0xFFFFFFFF;
  }
  if ( pSymbol->get_virtualAddress( &ulVA ) != S_OK ){
     ulVA = 0xFFFFFFFF;
  }
  pSymbol->get_addressSection(&dwSeg);
  pSymbol->get_addressOffset(&dwOff);

  wprintf(L"%s: [%08x][%08x][%04x:%08x] ", rgTags[dwSymTag], (unsigned long)(ulVA), dwRVA, dwSeg, dwOff);
  
  if(dwSymTag == SymTagThunk){
    BSTR wszName;
    if(pSymbol->get_name(&wszName) == S_OK){
      wprintf(L"%s\n",wszName);
      SysFreeString(wszName);
    }else{
      pSymbol->get_targetSection(&dwSeg);
      pSymbol->get_targetOffset(&dwOff);
      wprintf(L"target -> [%04X:%08X]\n", dwSeg, dwOff);
    }
  }else{
    BSTR wszName;
    BSTR wszUndname;
    if(pSymbol->get_name(&wszName) == S_OK){
      if(pSymbol->get_undecoratedName(&wszUndname) == S_OK){
        wprintf(L"%s(%s)\n", wszName, wszUndname);
        SysFreeString(wszUndname);
      }else{
        wprintf(L"%ws\n", wszName);
      }
      SysFreeString(wszName);
    }
  }
}

////////////////////////////////////////////////////////////
// Print a symbol info: name, type etc.
//
void PrintSymbol(IDiaSymbol *pSymbol, DWORD dwIndent){
  IDiaEnumSymbols* pEnumChildren;
  IDiaSymbol* pType;
  DWORD dwSymTag;
  ULONGLONG ulLen;
  DWORD dwCall;
  
  if(pSymbol->get_symTag(&dwSymTag) != S_OK){
    wprintf(L"ERROR - PrintSymbol get_symTag() failed\n");
    return;
  }
  if(dwSymTag == SymTagFunction){
    wprintf(L"\n");
  }
  PrintSymTag(dwSymTag);
  for(DWORD i = 0; i < dwIndent; i++){
    wprintf(L" ");
  }
  switch(dwSymTag){
    case SymTagCompilandDetails:
      PrintCompilandDetails(pSymbol);
      break;
    case SymTagCompilandEnv:
      PrintCompilandEnv(pSymbol);
      break;
    case SymTagData:
      PrintData(pSymbol, dwIndent + 2);
      break;
    case SymTagFunction:
    case SymTagBlock:
      PrintLocation(pSymbol);
      if(pSymbol->get_length(&ulLen) == S_OK){
        wprintf(L", len = %08x, ",ulLen);
      }
      if(dwSymTag == SymTagFunction && pSymbol->get_callingConvention(&dwCall) == S_OK){
        wprintf(L", %s", SafeDRef(rgCallingConvention, dwCall));
      }
      PrintUndName(pSymbol);
      wprintf(L"\n");
      if(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK){
        IDiaSymbol* pChild;
        ULONG celt = 0;
        
        while(pEnumChildren->Next(1, &pChild, &celt) == S_OK && celt == 1) {
          PrintSymbol(pChild, dwIndent + 2);
          pChild->Release();
        }
        pEnumChildren->Release();
      }
      return;
    case SymTagAnnotation:
      PrintLocation(pSymbol);
      wprintf(L"\n");
      break;
    case SymTagLabel:
      PrintLocation(pSymbol);
      wprintf(L", ");
      PrintName(pSymbol);
      break;
    case SymTagEnum:
    case SymTagTypedef:
    case SymTagUDT:
    case SymTagBaseClass:
      PrintUDT(pSymbol);
      break;
    case SymTagFuncDebugStart:
    case SymTagFuncDebugEnd:
      PrintLocation(pSymbol);
      break;
    case SymTagFunctionArgType:
    case SymTagFunctionType:
    case SymTagPointerType:
    case SymTagArrayType:
    case SymTagBaseType:
      if(pSymbol->get_type(&pType) == S_OK){
        PrintType(pType);
        pType->Release();
      }
      wprintf(L"\n");
      break;
    case SymTagThunk:
        PrintThunk(pSymbol);
        break;
    case SymTagPublicSymbol:
    case SymTagFriend:
    case SymTagUsingNamespace:
    case SymTagVTableShape:
    case SymTagVTable:
    case SymTagCustom:
    case SymTagMax:
    case SymTagExe:
    default:
      PrintName(pSymbol);
      IDiaSymbol* pType;
      if(pSymbol->get_type(&pType) == S_OK){
        wprintf(L" has type ");
        PrintType(pType);
        pType->Release();
      }
  }
  if((dwSymTag == SymTagUDT) || (dwSymTag == SymTagAnnotation)){
    IDiaEnumSymbols* pEnumChildren;
    
    wprintf(L"\n");
    if(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK){
      IDiaSymbol* pChild;
      ULONG celt = 0;
      
      while(pEnumChildren->Next(1, &pChild, &celt) == S_OK && celt == 1){
        PrintSymbol(pChild, dwIndent + 2);
        pChild->Release();
      }
      pEnumChildren->Release();
    }
  }
  wprintf(L"\n");
}

////////////////////////////////////////////////////////////
// Print the string coresponding to the symbol's tag property
// 
void PrintSymTag(DWORD dwSymTag){
    wprintf(L"%-15s: ", SafeDRef(rgTags, dwSymTag));
}

////////////////////////////////////////////////////////////
// Print the name of the symbol
//
void PrintName(IDiaSymbol* pSymbol){
  BSTR wszName;
  BSTR wszUndName;
  
  if(pSymbol->get_name(&wszName) != S_OK){
    wprintf(L"(none)");
    return;
  }
  if(pSymbol->get_undecoratedName(&wszUndName) == S_OK){
    if(wcscmp(wszName, wszUndName) == 0){
      wprintf(L"%s", wszName);
    }else{
      wprintf(L"%s(%s)", wszUndName, wszName);
    }
    SysFreeString(wszUndName);
  }else{
    wprintf(L"%s", wszName);
  }
  SysFreeString(wszName);
}

////////////////////////////////////////////////////////////
// Print the undecorated name of the symbol
//  - only SymTagFunction, SymTagData and SymTagPublicSymbol
//    can have this property set
//
void PrintUndName(IDiaSymbol* pSymbol){
  BSTR wszName;
  
  if(pSymbol->get_undecoratedName(&wszName) != S_OK){
    if(pSymbol->get_name(&wszName) == S_OK){
      // Print the name of the symbol instead
      wprintf(L"%s", (wszName[0] != L'\0')?wszName:L"(none)");
      SysFreeString(wszName);
    }else{
      wprintf(L"(none)");
    }
    return;
  }
  if(wszName[0] != L'\0') {
    wprintf(L"%s", wszName);
  }
  SysFreeString(wszName);
}

////////////////////////////////////////////////////////////
// Print a SymTagThunk symbol's info
//
void PrintThunk(IDiaSymbol* pSymbol){
  DWORD dwISect, dwOffset;
  DWORD dwRVA;
  
  if(pSymbol->get_addressSection(&dwISect) == S_OK &&
    pSymbol->get_addressOffset(&dwOffset) == S_OK ) {
    wprintf(L"[0x%04x:0x%08x]", dwISect, dwOffset);
  }
  if(pSymbol->get_targetSection(&dwISect) == S_OK &&
    pSymbol->get_targetOffset( &dwOffset ) == S_OK && 
    pSymbol->get_targetRelativeVirtualAddress( &dwRVA ) == S_OK ) {
    wprintf(L", target [0x%04x:0x%08x]", dwISect, dwOffset );
  }else{
    wprintf(L", target ");
    PrintName(pSymbol);
  }
}

////////////////////////////////////////////////////////////
// Print the compiland/module details: language, platform...
//
void PrintCompilandDetails(IDiaSymbol* pSymbol){
  DWORD dwLanguage;
  DWORD dwPlatform;
  BOOL bENCEnabled = FALSE;
  DWORD dwVerMajor, dwVerMinor, dwVerBuild;
  
  if(pSymbol->get_language( &dwLanguage ) == S_OK){
    wprintf(L"\n\tLanguage: %s\n", SafeDRef(rgLanguage, dwLanguage));
  }
  if(pSymbol->get_platform(&dwPlatform) == S_OK){
    wprintf(L"\tTarget Processor: %s\n", SafeDRef(rgProcessorStrings, dwPlatform));
  }
  if(pSymbol->get_editAndContinueEnabled(&bENCEnabled) == S_OK && bENCEnabled){
    wprintf(L"\tCompiled for EnC: Yes\n");
  }else{
    wprintf(L"\tCompiled for EnC: No\n");
  }
  wprintf(L"\tCompiler Version: ");
  if(pSymbol->get_frontEndMajor( &dwVerMajor ) == S_OK && 
     pSymbol->get_frontEndMinor( &dwVerMinor ) == S_OK && 
     pSymbol->get_frontEndBuild( &dwVerBuild ) == S_OK ){
    wprintf(L"FE %d.%d.%d ", dwVerMajor, dwVerMinor, dwVerBuild);
  }
  if(pSymbol->get_backEndMajor( &dwVerMajor ) == S_OK && 
     pSymbol->get_backEndMinor( &dwVerMinor ) == S_OK && 
     pSymbol->get_backEndBuild( &dwVerBuild ) == S_OK ){
    wprintf(L"BE %d.%d.%d ", dwVerMajor, dwVerMinor, dwVerBuild );
  }
  wprintf(L"\n");  
}

////////////////////////////////////////////////////////////
// Print the compilan/module env
//
void PrintCompilandEnv(IDiaSymbol* pSymbol){
  PrintName(pSymbol);
  wprintf(L" =");
  VARIANT vt = { VT_EMPTY };
  if (pSymbol->get_value(&vt) == S_OK)
    PrintVariant(vt);
}

////////////////////////////////////////////////////////////
// Print a string corespondig to a location type
//
void PrintLocation(IDiaSymbol* pSymbol){
  DWORD dwLocType;
  DWORD dwRVA, dwSect, dwOff, dwReg, dwBitPos, dwSlot;
  LONG lOffset;
  ULONGLONG ulLen;
  VARIANT vt = { VT_EMPTY };
  
  if(pSymbol->get_locationType(&dwLocType) != S_OK){
    // It must be a symbol in optimized code
    wprintf(L"symbol in optmized code");
    return;
  }
  switch(dwLocType){
    case LocIsStatic: 
      if(pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK &&
         pSymbol->get_addressSection(&dwSect) == S_OK &&
         pSymbol->get_addressOffset(&dwOff) == S_OK ){
        wprintf(L"%s, [0x%08x][0x%04x:0x%08x]",SafeDRef(rgLocationTypeString, dwLocType), dwRVA, dwSect, dwOff);
      }
      break;
    case LocIsTLS:
    case LocInMetaData:
    case LocIsIlRel:
      if(pSymbol->get_addressSection(&dwSect) == S_OK &&
         pSymbol->get_addressOffset(&dwOff) == S_OK ){
        printf("%s, [0x%04x:0x%08x]",SafeDRef(rgLocationTypeString, dwLocType), dwSect, dwOff);
      }
      break;
    case LocIsRegRel:
      if(pSymbol->get_registerId(&dwReg) == S_OK &&
         pSymbol->get_offset(&lOffset) == S_OK ){
        wprintf(L"%s Relative, [0x%08x]",SzNameC7Reg((USHORT)dwReg), lOffset);
      }
      break;
    case LocIsThisRel:
      if(pSymbol->get_offset(&lOffset) == S_OK ) {
        wprintf(L"this+0x%x", lOffset);
      }
      break;
    case LocIsBitField:
      if(pSymbol->get_offset(&lOffset) == S_OK &&
         pSymbol->get_bitPosition(&dwBitPos) == S_OK &&
         pSymbol->get_length(&ulLen) == S_OK ){
        wprintf(L"this(bf)+0x%x:0x%x len(0x%x)",lOffset, dwBitPos, ulLen);
      }
      break;
    case LocIsEnregistered:
      if(pSymbol->get_registerId(&dwReg) == S_OK ){
        wprintf(L"enregistered %s", SzNameC7Reg((USHORT)dwReg));
      }
      break;
    case LocIsNull:
      wprintf(L"pure");
      break;
    case LocIsSlot:
      if(pSymbol->get_slot(&dwSlot) == S_OK ){
        wprintf(L"%s, [0x%08x]", SafeDRef(rgLocationTypeString, dwLocType), dwSlot);
      }
      break;
    case LocIsConstant:
      wprintf(L"constant");
      if(pSymbol->get_value(&vt) == S_OK){
        PrintVariant(vt);
      }
      break;
    default :
      wprintf(L"Error - invalid location type: 0x%x", dwLocType);
      break;
    }
}

////////////////////////////////////////////////////////////
// Print the type, value and the name of a const symbol
//
void PrintConst(IDiaSymbol* pSymbol){
  VARIANT vt = { VT_EMPTY };
  PrintSymbolType(pSymbol);
  if(pSymbol->get_value(&vt) == S_OK){
    PrintVariant(vt);
  }
  PrintName(pSymbol);
}

////////////////////////////////////////////////////////////
// Print the name and the type of an user defined type
//
void PrintUDT(IDiaSymbol* pSymbol){
  PrintName(pSymbol);
  PrintSymbolType(pSymbol);
}

////////////////////////////////////////////////////////////
// Print a string representing the type of a symbol
//
void PrintSymbolType(IDiaSymbol* pSymbol){
  IDiaSymbol* pType;
  if(pSymbol->get_type(&pType) == S_OK){
    wprintf(L", Type: ");
    PrintType(pType);
    pType->Release();
  }
}

////////////////////////////////////////////////////////////
// Print the information details for a type symbol
//
void PrintType(IDiaSymbol* pSymbol){
  IDiaSymbol* pBaseType;
  IDiaEnumSymbols* pEnumSym;
  IDiaSymbol* pSym;
  DWORD dwTag;
  BSTR wszName;
  DWORD dwInfo;
  ULONGLONG ulLen;
  BOOL bSet;
  DWORD dwRank;
  LONG lCount = 0;
  ULONG celt = 1;
  
  if(pSymbol->get_symTag(&dwTag) != S_OK){
    wprintf(L"ERROR - can't retrieve the symbol's SymTag\n");
    return;
  }
  if(pSymbol->get_name(&wszName) != S_OK){
    wszName = NULL;
  }
  if(dwTag != SymTagPointerType){
    if(pSymbol->get_constType(&bSet) == S_OK && bSet){
      wprintf(L"const ");
    }
    if(pSymbol->get_volatileType(&bSet) == S_OK && bSet){
       wprintf(L"volatile ");
    }
    if(pSymbol->get_unalignedType(&bSet) == S_OK && bSet){
      wprintf(L"__unaligned ");
    }
  }
  pSymbol->get_length(&ulLen);
  switch(dwTag){
    case SymTagUDT:
      PrintUdtKind(pSymbol);
      PrintName(pSymbol);
      break;
    case SymTagEnum:
      wprintf(L"enum ");
      PrintName(pSymbol);
      break;
    case SymTagFunctionType:
      wprintf(L"function ");
      break;
    case SymTagPointerType:
      if(pSymbol->get_type(&pBaseType) != S_OK){
        wprintf(L"ERROR - SymTagPointerType get_type");
        return;
      }
      PrintType(pBaseType);
      pBaseType->Release();
      if(pSymbol->get_reference(&bSet) == S_OK && bSet){
        wprintf(L" &");
      }else{
        wprintf(L" *");
      }
      if(pSymbol->get_constType(&bSet) == S_OK && bSet){
        wprintf(L" const");
      }
      if(pSymbol->get_volatileType(&bSet) == S_OK && bSet){
        wprintf(L" volatile");
      }
      if(pSymbol->get_unalignedType(&bSet) == S_OK && bSet){
        wprintf(L" __unaligned");
      }
      break;
    case SymTagArrayType:
      if(pSymbol->get_type(&pBaseType) == S_OK){
        PrintType(pBaseType);
        if(pSymbol->get_rank(&dwRank) == S_OK){
          if(pSymbol->findChildren(SymTagDimension, NULL, nsNone, &pEnumSym) == S_OK && pEnumSym != NULL){
            while(pEnumSym->Next( 1, &pSym, &celt ) == S_OK && celt == 1){
              IDiaSymbol* pBound;
              
              wprintf(L"[");
              if(pSym->get_lowerBound(&pBound) == S_OK){
                PrintBound(pBound);
                wprintf(L"..");
                pBound->Release();
              }
              pBound = NULL;
              if(pSym->get_upperBound(&pBound) == S_OK){
                PrintBound(pBound);
                pBound->Release();
              }
              pSym->Release();
              pSym = NULL;
              wprintf(L"]");
            }
            pEnumSym->Release();
          } 
        }else if(pSymbol->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSym) == S_OK && pEnumSym != NULL && 
                 pEnumSym->get_Count(&lCount) == S_OK && lCount > 0){
          while(pEnumSym->Next( 1, &pSym, &celt ) == S_OK && celt == 1){
            wprintf(L"[");
            PrintType(pSym);
            printf( "]" );
            pSym->Release();
          }
          pEnumSym->Release();
        }else{
          DWORD dwCountElems;
          ULONGLONG ulLenArray;
          ULONGLONG ulLenElem;
          
          if(pSymbol->get_count(&dwCountElems) == S_OK){
            wprintf(L"[0x%x]", dwCountElems);
          }else if(pSymbol->get_length(&ulLenArray) == S_OK && pBaseType->get_length(&ulLenElem) == S_OK ){
            if(ulLenElem == 0){
              wprintf(L"[0x%lx]", ulLenArray );
            }else{
              wprintf(L"[0x%lx]", ulLenArray/ulLenElem);
            }
          }
        }
        pBaseType->Release();
      }else{
        wprintf(L"ERROR - SymTagArrayType get_type\n");
        return;
      }
      break;
    case SymTagBaseType:
      if(pSymbol->get_baseType(&dwInfo) != S_OK){
        wprintf(L"SymTagBaseType get_baseType\n");
        return;
      }
      switch(dwInfo){
        case btUInt :
          wprintf(L"unsigned ");
        // Fall through
        case btInt :
          switch(ulLen){
            case 1:
              if(dwInfo == btInt){
                wprintf(L"signed ");
              }
              wprintf(L"char");
              break;
            case 2:
              wprintf(L"short");
              break;
            case 4:
              wprintf(L"int");
              break;
            case 8:
              wprintf(L"__int64");
              break;
          }
          dwInfo = 0xFFFFFFFF;
          break;
        case btFloat :
          switch(ulLen){
            case 4:
              wprintf(L"float");
              break;
            case 8:
              wprintf(L"double");
              break;
          }
          dwInfo = 0xFFFFFFFF;
          break;
      }
      if(dwInfo == 0xFFFFFFFF){
         break;
      }
      wprintf(L"%s", rgBaseType[dwInfo]);
      break;
    case SymTagTypedef:
      PrintName(pSymbol);
      break;
    case SymTagCustomType: 
      {
        DWORD idOEM, idOEMSym;
        DWORD cbData = 0;
        DWORD count;
        
        if(pSymbol->get_oemId( &idOEM ) == S_OK){
          wprintf(L"OEMId = %x, ", idOEM);
        }
        if(pSymbol->get_oemSymbolId( &idOEMSym ) == S_OK){
          wprintf(L"SymbolId = %x, ", idOEMSym);
        }
        if(pSymbol->get_types(0, &count, NULL) == S_OK ){
          IDiaSymbol** rgpDiaSymbols = (IDiaSymbol**)_alloca( sizeof(IDiaSymbol*) * count );
          if(pSymbol->get_types(count, &count, rgpDiaSymbols) == S_OK ){
            for(ULONG i = 0; i < count; i++){
              PrintType(rgpDiaSymbols[i]);
              rgpDiaSymbols[i]->Release();
            }
          }
        }
        // print custom data 
        if(pSymbol->get_dataBytes( cbData, &cbData, NULL) == S_OK && cbData != 0){
          wprintf(L", Data: ");
          BYTE* pbData = new BYTE[cbData];
          pSymbol->get_dataBytes( cbData, &cbData, pbData );
          for(ULONG i = 0; i < cbData; i++){
            wprintf(L"0x%02x ", pbData[i]);
          }
          delete [] pbData;
        }
      }
      break;
    case SymTagData: // This really is member data, just print its location
      PrintLocation(pSymbol);
      break;
    default:
      break;
  }
  if(wszName != NULL){
    SysFreeString(wszName);
  }
}

////////////////////////////////////////////////////////////
// Print bound information
//
void PrintBound(IDiaSymbol* pSymbol){
  DWORD dwTag = 0;
  DWORD dwKind;
  
  if(pSymbol->get_symTag(&dwTag) != S_OK){
    wprintf(L"ERROR - PrintBound() get_symTag");
    return;
  }
  if(pSymbol->get_locationType(&dwKind) != S_OK){
    wprintf(L"ERROR - PrintBound() get_locationType");
    return;
  }
  if(dwTag == SymTagData && dwKind == LocIsConstant){
    VARIANT v;
    pSymbol->get_value(&v);
    PrintVariant(v);
  }else{ 
        PrintName(pSymbol);
  }
}

////////////////////////////////////////////////////////////
// 
void PrintData(IDiaSymbol* pSymbol, DWORD dwIndent){
  DWORD dwDataKind;
  DWORD dwLiveRanges = 0;
  DWORD dwRVAStart, dwSectStart, dwOffsetStart;
  DWORD dwRVAEnd, dwSectEnd, dwOffsetEnd;
  BSTR wszProgram;
  
  PrintLocation(pSymbol);
  if(pSymbol->get_dataKind(&dwDataKind) != S_OK){
    wprintf(L"ERROR - PrintData() get_dataKind");
    return;
  }
  wprintf(L", %s", SafeDRef(rgDataKind, dwDataKind));
  PrintSymbolType(pSymbol);
  wprintf(L", ");
  PrintName(pSymbol);
  if(pSymbol->get_liveLVarInstances( 0, dwLiveRanges, &dwLiveRanges, NULL) == S_OK){
    // Symbol has live ranges
    wprintf(L"\n");
    
    // Dump Live ranges
    IDiaLVarInstance **rgpLiveRanges = new IDiaLVarInstance*[dwLiveRanges];
    if(pSymbol->get_liveLVarInstances( 0, dwLiveRanges, &dwLiveRanges, rgpLiveRanges) != S_OK){
      wprintf(L"ERROR - PrintData() get_liveLVarInstances\n");
      return;
    }
    for(DWORD i = 0; i < dwLiveRanges; i++){
      IDiaLVarInstance* pRange = rgpLiveRanges[i];
      
      wprintf(L"             ");
      for(DWORD j = 0; j < dwIndent; j++)
        wprintf(L"  ");
        if(pRange->get_rvaStart(&dwRVAStart) == S_OK &&
           pRange->get_sectionStart(&dwSectStart) == S_OK &&
           pRange->get_offsetStart(&dwOffsetStart) == S_OK &&
           pRange->get_rvaEnd(&dwRVAEnd) == S_OK &&
           pRange->get_sectionEnd(&dwSectEnd) == S_OK &&
           pRange->get_offsetEnd(&dwOffsetEnd) == S_OK &&
           pRange->get_program(&wszProgram) == S_OK ) {
          wprintf(L"[0x%08x][0x%04x:0x%08x]-[0x%08x][0x%04x:0x%08x] %s\n",
                  dwRVAStart,dwSectStart,dwOffsetStart,dwRVAEnd,dwSectEnd,dwOffsetEnd,wszProgram);
          SysFreeString(wszProgram);
        }
        rgpLiveRanges[i]->Release();
      }
    delete [] rgpLiveRanges;
  }
}

////////////////////////////////////////////////////////////
// Print a VARIANT 
//
void PrintVariant(VARIANT var){
  switch(var.vt){
    case VT_UI1:
    case VT_I1:
      wprintf(L" 0x%x", var.bVal);
      break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
      wprintf(L" 0x%x", var.iVal);
      break;
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
      wprintf(L" 0x%x", var.lVal);
      break;
    case VT_R4:
      wprintf(L" %f", var.fltVal);
      break;
    case VT_R8:
      wprintf(L" %dn", var.dblVal);
      break;
    case VT_BSTR:
      wprintf(L" \"%s\"", var.bstrVal );
      break;
    default:
      wprintf(L" ??");
    }
}

////////////////////////////////////////////////////////////
// Print a string corresponding to a UDT kind
//
void PrintUdtKind(IDiaSymbol* pSymbol){
  DWORD dwKind = 0;
  if(pSymbol->get_udtKind( &dwKind ) == S_OK){
    wprintf(L"%s ", rgUdtKind[dwKind]);
  }
}

////////////////////////////////////////////////////////////
// Print type informations is details
//
void PrintTypeInDetail(IDiaSymbol *pSymbol, DWORD dwIndent){
  IDiaEnumSymbols* pEnumChildren;
  IDiaSymbol* pType;
  IDiaSymbol* pChild;
  DWORD dwSymTag;
  DWORD dwSymTagType;
  ULONG celt = 0;
  BOOL bFlag;
  
  if(dwIndent > MAX_TYPE_IN_DETAIL){
        return;
  }
  if(pSymbol->get_symTag(&dwSymTag) != S_OK){
    wprintf(L"ERROR - PrintTypeInDetail() get_symTag\n");
    return;
  }
  PrintSymTag(dwSymTag);
  for(DWORD i = 0;i < dwIndent; i++){
    wprintf(L" ");
  }
  switch(dwSymTag){
    case SymTagData:
      PrintData(pSymbol, dwIndent);
      if(pSymbol->get_type(&pType) == S_OK){
        if(pType->get_symTag(&dwSymTagType) == S_OK){
          if(dwSymTagType == SymTagUDT){
            wprintf(L"\n");
            PrintTypeInDetail( pType, dwIndent + 2);
          }
        }
        pType->Release();
      }
      break;
    case SymTagTypedef:
    case SymTagVTable:
      PrintSymbolType(pSymbol);
      break;
    case SymTagEnum:
    case SymTagUDT:
      PrintUDT(pSymbol);
      wprintf(L"\n");
      if(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK){
        while(pEnumChildren->Next(1, &pChild, &celt) == S_OK && celt == 1){
          PrintTypeInDetail(pChild, dwIndent + 2);
          pChild->Release();
        }
        pEnumChildren->Release();
      }
      return;
      break;
    case SymTagFunction:
      PrintFunctionType(pSymbol);
      return;
      break;
    case SymTagPointerType:
      PrintName(pSymbol);
      wprintf(L" has type ");
      PrintType(pSymbol);
      break;
    case SymTagArrayType:
    case SymTagBaseType:
    case SymTagFunctionArgType:
    case SymTagUsingNamespace:
    case SymTagCustom:
    case SymTagFriend:
      PrintName(pSymbol);
      PrintSymbolType(pSymbol);
      break;
    case SymTagVTableShape:
    case SymTagBaseClass:
      PrintName(pSymbol);
      if(pSymbol->get_virtualBaseClass(&bFlag) == S_OK && bFlag){
        IDiaSymbol* pVBTableType;
        LONG ptrOffset;
        DWORD dispIndex;
        
        if(pSymbol->get_virtualBaseDispIndex(&dispIndex ) == S_OK &&
           pSymbol->get_virtualBasePointerOffset(&ptrOffset) == S_OK){
          wprintf(L" virtual, offset = 0x%x, pointer offset = %ld, virtual base pointer type = ", dispIndex, ptrOffset);
          if(pSymbol->get_virtualBaseTableType(&pVBTableType) == S_OK){
            PrintType(pVBTableType);
            pVBTableType->Release();
          }else{
            wprintf(L"(unknown)");
          }
        }
      }else{
        LONG offset;
        
        if(pSymbol->get_offset( &offset ) == S_OK){
          wprintf(L", offset = 0x%x", offset);
        }
      }
      wprintf(L"\n");
      if(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK){
        while(pEnumChildren->Next(1, &pChild, &celt) == S_OK && celt == 1){
          PrintTypeInDetail(pChild, dwIndent + 2);
          pChild->Release();
        }
        pEnumChildren->Release();
      }
      break;
    case SymTagFunctionType: 
      if(pSymbol->get_type(&pType) == S_OK){
        PrintType(pType);
      }
      break;
    case SymTagThunk:
      // Happens for functions which only have S_PROCREF
      PrintThunk(pSymbol);
      break;
    default:
      wprintf(L"ERROR - PrintTypeInDetail() invalid SymTag\n");
    }
    wprintf(L"\n");
}

////////////////////////////////////////////////////////////
// Print a function type
//
void PrintFunctionType(IDiaSymbol* pSymbol){
  IDiaSymbol* pFuncType;
  DWORD dwAccess = 0;
  BOOL  bIsStatic = false;
  BSTR wszName;
  
  if(pSymbol->get_access(&dwAccess) == S_OK){
    wprintf(L"%s ", SafeDRef(rgAccess,dwAccess));
  }
  if(pSymbol->get_isStatic(&bIsStatic) == S_OK && bIsStatic ) {
    wprintf(L"static ");
  }
  if(pSymbol->get_type(&pFuncType) == S_OK){
    IDiaSymbol* pReturnType;
    
    if(pFuncType->get_type(&pReturnType) == S_OK){
      PrintType(pReturnType);
      wprintf(L" ");
      if(pSymbol->get_name(&wszName) == S_OK){
        wprintf(L"%s", wszName);
        SysFreeString(wszName);
      }
      
      IDiaEnumSymbols* pEnumChildren;
      if(pFuncType->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK){
        IDiaSymbol* pChild;
        ULONG celt = 0, nParam = 0;
        
        wprintf(L"(");
        while(pEnumChildren->Next(1, &pChild, &celt) == S_OK && celt == 1){
          IDiaSymbol* pType;
          if(pChild->get_type(&pType) == S_OK){
            if(nParam++){
              wprintf(L", ");
            }
            PrintType(pType);
            pType->Release();
          }
          pChild->Release();
        }
        pEnumChildren->Release();
        wprintf(L")\n");
      }
      pReturnType->Release();
    }
    pFuncType->Release();
  }
}

////////////////////////////////////////////////////////////
// 
void PrintSourceFile(IDiaSourceFile* pSource){
  BSTR wszSourceName;
  BYTE checksum[0x256];
  DWORD cbChecksum = sizeof( checksum );
  
  if(pSource->get_fileName(&wszSourceName) == S_OK){
    wprintf(L"\t%s", wszSourceName);
    SysFreeString(wszSourceName);
  }else{
    wprintf(L"ERROR - PrintSourceFile() get_fileName");
    return;
  }
  if(pSource->get_checksum( cbChecksum, &cbChecksum, checksum) == S_OK){
    wprintf(L" (checksum = ");
    for(DWORD i = 0;i < cbChecksum; i++){
      wprintf(L"0x%02x ", checksum[i]);
    }
    wprintf(L")");
  }
}

////////////////////////////////////////////////////////////
// 
void PrintLines(IDiaSession* pSession,IDiaSymbol* pFunction){
  DWORD dwSymTag;
  BSTR wszName;
  ULONGLONG ulLength;
  DWORD dwRVA;
  IDiaEnumLineNumbers* pLines;

  if(pFunction->get_symTag(&dwSymTag) != S_OK || dwSymTag != SymTagFunction){
    wprintf(L"ERROR - PrintLines() dwSymTag != SymTagFunction");
    return;
  }
  if(pFunction->get_name(&wszName) == S_OK){
    wprintf(L"\n** %s\n\n", wszName);
    SysFreeString(wszName);
  }
  if(pFunction->get_length(&ulLength) != S_OK){
    wprintf(L"ERROR - PrintLines() get_length");
    return;
  }
  if(pFunction->get_relativeVirtualAddress(&dwRVA) == S_OK){
    if(pSession->findLinesByRVA(dwRVA,static_cast<DWORD>(ulLength),&pLines) == S_OK){
      PrintLines(pLines);
      pLines->Release();
    }
  }else{
    DWORD dwSect, dwOffset;
    
    if(pFunction->get_addressSection(&dwSect) == S_OK &&
       pFunction->get_addressOffset(&dwOffset) == S_OK){
      if(pSession->findLinesByAddr(dwSect, dwOffset, static_cast<DWORD>(ulLength), &pLines) == S_OK){
        PrintLines(pLines);
        pLines->Release();
      }
    }
  }
}

////////////////////////////////////////////////////////////
// 
void PrintLines(IDiaEnumLineNumbers* pLines){
  IDiaLineNumber* pLine;
  DWORD celt;
  DWORD dwSrcIdLast = (DWORD)(-1);
  DWORD dwRVA, dwOffset, dwSeg, dwLinenum, dwSrcId, dwLength;
  
  while(pLines->Next(1, &pLine, &celt) == S_OK && celt == 1){
    if(pLine->get_relativeVirtualAddress(&dwRVA) == S_OK &&
       pLine->get_addressSection(&dwSeg) == S_OK &&
       pLine->get_addressOffset(&dwOffset) == S_OK &&
       pLine->get_lineNumber(&dwLinenum) == S_OK &&
       pLine->get_sourceFileId(&dwSrcId) == S_OK &&
       pLine->get_length(&dwLength) == S_OK ) {
      wprintf(L"\tline %d at [0x%x][0x%x:0x%x], len = 0x%x", dwLinenum, dwRVA, dwSeg, dwOffset, dwLength );
      if(dwSrcId != dwSrcIdLast){
        IDiaSourceFile* pSource;
        if(pLine->get_sourceFile(&pSource) == S_OK){
          PrintSourceFile(pSource);
          dwSrcIdLast = dwSrcId;
          pSource->Release();
        }
      }
      pLine->Release();
      wprintf(L"\n");
    }
  }
}

////////////////////////////////////////////////////////////
// Print the section contribution data: name, Sec::Off, length
void PrintSecContribs(IDiaSectionContrib* pSegment){
  IDiaSymbol* pCompiland;
  BSTR wszName;
  DWORD dwSect;
  DWORD dwOffset;
  DWORD dwLen;
  
  if(pSegment->get_compiland(&pCompiland) == S_OK &&
     pSegment->get_addressSection(&dwSect) == S_OK &&
     pSegment->get_addressOffset(&dwOffset) == S_OK &&
     pSegment->get_length(&dwLen) == S_OK &&
     pCompiland->get_name(&wszName) == S_OK ) {
    wprintf(L"%s : [%04x:%08x], len = %08x\n", wszName, dwSect, dwOffset, dwLen);
    pCompiland->Release();
    SysFreeString(wszName);
  }
}

////////////////////////////////////////////////////////////
// Print a debug stream data
//
void PrintStreamData(IDiaEnumDebugStreamData* pStream){
  BSTR wszName;
  LONG dwElem;
  ULONG celt = 0;
  DWORD cbData, cbTotal = 0;
  BYTE data[1024];
  
  if(pStream->get_name(&wszName) != S_OK){
    wprintf(L"ERROR - PrintStreamData() get_name\n");
  }else{
    wprintf(L"Stream: %s", wszName);
    SysFreeString(wszName);
  }
  if(pStream->get_Count(&dwElem) != S_OK){
      wprintf(L"ERROR - PrintStreamData() get_Count\n");
  }else{
    wprintf(L"(%d)\n", dwElem);
  }
  while(pStream->Next(1, sizeof(data), &cbData, (BYTE *)&data, &celt) == S_OK){
    DWORD i;
    for(i = 0;i < cbData; i++) {
      wprintf(L"%02X ", data[i]);
      if(i && i % 8 == 7 && i+1 < cbData){
        wprintf(L"- ");
      }
    }
    wprintf(L"| ");
    for(i = 0;i < cbData; i++){
      wprintf(L"%c", iswprint(data[i]) ? data[i] : '.');
    }
    wprintf(L"\n");
    cbTotal += cbData;
  }
  wprintf(L"Summary :\n\tSizeof(Elem) = %d\n\tNo of Elems = %d\n\n", cbTotal/dwElem, dwElem);
}

////////////////////////////////////////////////////////////
// Print the FPO info for a given symbol;
//
void PrintFrameData(IDiaFrameData* pFrameData){
  DWORD dwSect;
  DWORD dwOffset;
  DWORD cbBlock;
  DWORD cbLocals; // Number of bytes reserved for the function locals
  DWORD cbParams; // Number of bytes reserved for the function arguments
  DWORD cbMaxStack;
  DWORD cbProlog;
  DWORD cbSavedRegs;
  BOOL bSEH, bEH, bStart;
  BSTR wszProgram;
  
  if(pFrameData->get_addressSection(&dwSect) == S_OK && 
     pFrameData->get_addressOffset(&dwOffset) == S_OK &&
     pFrameData->get_lengthBlock(&cbBlock) == S_OK &&
     pFrameData->get_lengthLocals(&cbLocals) == S_OK &&
     pFrameData->get_lengthParams(&cbParams) == S_OK &&
     pFrameData->get_maxStack(&cbMaxStack) == S_OK &&
     pFrameData->get_lengthProlog(&cbProlog) == S_OK &&
     pFrameData->get_lengthSavedRegisters(&cbSavedRegs) == S_OK &&
     pFrameData->get_systemExceptionHandling(&bSEH) == S_OK &&
     pFrameData->get_cplusplusExceptionHandling(&bEH) == S_OK &&
     pFrameData->get_functionStart(&bStart) == S_OK ) {
    wprintf(L"%04X:%08X   %8X %8X %8X %8X %8X %8X %c   %c   %c",
            dwSect, dwOffset, cbBlock, cbLocals, cbParams, cbMaxStack, cbProlog, cbSavedRegs,
            bSEH ? L'Y' : L'N',
            bEH ? L'Y' : L'N',
            bStart ? L'Y' : L'N');
    if(pFrameData->get_program(&wszProgram) == S_OK){
      wprintf(L" %s\n", wszProgram);
      SysFreeString(wszProgram);
    }else{
      wprintf(L"\n");
    }
  }
}

////////////////////////////////////////////////////////////
// Print all the valid properties associated to a symbol
//
void PrintPropertyStorage(IDiaPropertyStorage* pPropertyStorage){
  IEnumSTATPROPSTG* pEnumProps;
  STATPROPSTG prop;
  DWORD celt = 1;
  
  if(pPropertyStorage->Enum(&pEnumProps) == S_OK){
    while(pEnumProps->Next(celt, &prop, &celt) == S_OK){
      PROPSPEC pspec = { PRSPEC_PROPID, prop.propid };
      PROPVARIANT vt = { VT_EMPTY };
      
      if(pPropertyStorage->ReadMultiple( 1, &pspec, &vt) == S_OK){
        switch( vt.vt ){
          case VT_BOOL:
            wprintf( L"%32s:\t %s\n", prop.lpwstrName, vt.bVal ? L"true" : L"false" );
            break;
          case VT_I2:
            wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.iVal );
            break;
          case VT_UI2:
            wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.uiVal );
            break;
          case VT_I4:
            wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.intVal );
            break;
          case VT_UI4:
            wprintf( L"%32s:\t 0x%0x\n", prop.lpwstrName, vt.uintVal );
            break;
          case VT_UI8:
            wprintf( L"%32s:\t 0x%x\n", prop.lpwstrName, vt.uhVal.QuadPart );
            break;
          case VT_BSTR:
            wprintf( L"%32s:\t %s\n", prop.lpwstrName, vt.bstrVal );
            break;
          case VT_UNKNOWN:
            wprintf( L"%32s:\t %p\n", prop.lpwstrName, vt.punkVal );
            break;
          case VT_SAFEARRAY:
            break;
          default:
            break;
        }
        VariantClear((VARIANTARG*) &vt);
      }
    }
    pEnumProps->Release();
  }
}
