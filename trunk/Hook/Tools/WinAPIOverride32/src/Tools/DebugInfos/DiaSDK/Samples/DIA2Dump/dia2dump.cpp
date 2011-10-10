// Dia2Dump.cpp : Defines the entry point for the console application.
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
#include "Dia2Dump.h"
#include "PrintSymbol.h"

#include "Callback.h"

#pragma warning (disable : 4100)

BSTR g_wszFilename = NULL;
IDiaDataSource* g_pDiaDataSource = NULL;
IDiaSession* g_pDiaSession = NULL;
IDiaSymbol* g_pGlobalSymbol = NULL;
DWORD g_dwMachineType = CV_CFL_80386;

////////////////////////////////////////////////////////////
// 
int wmain(int argc, wchar_t* argv[])
{
  FILE* pFile;
  
  if(argc < 2){
    PrintHelpOptions();
    return -1;
  }
  pFile=_wfopen( argv[argc - 1],L"r");
  if(!pFile){
    // invalid file name or file does not exist
    PrintHelpOptions();
    return -1;
  }
  fclose(pFile);
  g_wszFilename = argv[argc - 1];
  
  // CoCreate() and initialize COM objects
  if(!LoadDataFromPdb(g_wszFilename,&g_pDiaDataSource,&g_pDiaSession,&g_pGlobalSymbol)){
    return -1;
  }
  if(argc == 2){
    // no options passed; print all pdb info
    DumpAllPdbInfo(g_pDiaSession, g_pGlobalSymbol);
  }else if(!_wcsicmp(argv[1],L"-all")){
    DumpAllPdbInfo(g_pDiaSession, g_pGlobalSymbol);
  }else if(!ParseArg(argc-2,&argv[1])){
    Cleanup();
    return -1;
  }
  // release COM objects and CoUninitialize()
  Cleanup();
    return 0;
}

////////////////////////////////////////////////////////////
// Create an IDiaData source and open a PDB file
//
bool LoadDataFromPdb(
    wchar_t          *wszFilename, 
    IDiaDataSource  **ppSource,
    IDiaSession     **ppSession,
    IDiaSymbol      **ppGlobal){

  HRESULT hr;
  wchar_t wszExt[MAX_PATH];
  wchar_t* wszSearchPath = L"SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data
  DWORD dwMachType = 0;
  
  hr = CoInitialize(NULL);
  // Obtain Access To The Provider
  hr = CoCreateInstance(__uuidof(DiaSource),//CLSID_DiaSource, 
                        NULL, 
                        CLSCTX_INPROC_SERVER,
                        __uuidof(IDiaDataSource),
                        (void **) ppSource);
  if(hr != S_OK){
// regsvr32 msdia80.dll
    wprintf(L"CoCreateInstance failed - HRESULT = %x\n",hr);
    return false;
  }
  
  _wsplitpath(wszFilename,0,0,0,wszExt);
  if(!_wcsicmp(wszExt,L".pdb")){
    // Open and prepare a program database (.pdb) file as a debug data source
    hr = (*ppSource)->loadDataFromPdb(wszFilename);
    if(hr != S_OK){
      wprintf(L"loadDataFromPdb failed - HRESULT = %x\n",hr);
      return false;
    }
  }else{
    CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
                        // thus enabling a user interface to report on the progress of 
                        // the location attempt. The client application may optionally 
                        // provide a reference to its own implementation of this 
                        // virtual base class to the IDiaDataSource::loadDataForExe method.
    callback.AddRef();
    // Open and prepare the debug data associated with the .exe/.dll file
    hr = (*ppSource)->loadDataForExe(wszFilename,wszSearchPath,&callback);
    if(hr != S_OK){
      wprintf(L"loadDataForExe failed - HRESULT = %x\n",hr);
      return false;
    }
  }
  // Open a session for querying symbols
  hr = (*ppSource)->openSession(ppSession);
  if(hr != S_OK){
    wprintf(L"openSession failed - HRESULT = %x\n",hr);
    return false;
  }
  // Retrieve a reference to the global scope
  hr = (*ppSession)->get_globalScope(ppGlobal);
  if(hr != S_OK){
    wprintf(L"get_globalScope failed\n");
    return false;
  }
  // Set Machine type for getting correct register names
  if((*ppGlobal)->get_machineType(&dwMachType) == S_OK){
    switch(dwMachType){
      case IMAGE_FILE_MACHINE_I386 : g_dwMachineType = CV_CFL_80386; break;
      case IMAGE_FILE_MACHINE_IA64 : g_dwMachineType = CV_CFL_IA64; break;
      case IMAGE_FILE_MACHINE_AMD64 : g_dwMachineType = CV_CFL_AMD64; break;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////
// Release DIA objects and CoUninitialize
//
void Cleanup(void){
  if(g_pGlobalSymbol){
    g_pGlobalSymbol->Release();
    g_pGlobalSymbol = NULL;
  }
  if(g_pDiaSession){
    g_pDiaSession->Release();
    g_pDiaSession = NULL;
  }
  CoUninitialize();
}

////////////////////////////////////////////////////////////
// Parse the arguments of the program
//
bool ParseArg(int argc, wchar_t* argv[]){
  int iCount = 0;
  bool bReturn = true;
  
  if(!argc){
    return true;
  }
  if(!_wcsicmp(argv[0],L"-?")){
    PrintHelpOptions();
    return true;
  }else
  if(!_wcsicmp(argv[0],L"-help")){
    PrintHelpOptions();
    return true;
  }else
  if(!_wcsicmp(argv[0],L"-m")){
    // -m                : print all the mods
    iCount = 1;
    bReturn = bReturn && DumpAllMods(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-p")){
    // -p                : print all the publics
    iCount = 1;
    bReturn = bReturn && DumpAllPublics(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-s")){
    // -s                : print symbols
    iCount = 1;
    bReturn = bReturn && DumpAllSymbols(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-g")){
    // -g                : print all the globals
    iCount = 1;
    bReturn = bReturn && DumpAllGlobals(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-t")){
    // -t                : print all the types
    iCount = 1;
    bReturn = bReturn && DumpAllTypes(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-f")){
    // -f                : print all the files
    iCount = 1;
    bReturn = bReturn && DumpAllFiles( g_pDiaSession, g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-l")){
    if(argc > 1 && *argv[1] != L'-'){
      // -l RVA [bytes]    : print line number info at RVA address in the bytes range
      DWORD dwRVA = 0;
      DWORD dwRange = MAX_RVA_LINES_BYTES_RANGE;
      
      swscanf(argv[1], L"%x", &dwRVA);
      if(argc > 2 && *argv[2] != L'-'){
        swscanf(argv[2], L"%d", &dwRange);
        iCount = 3;
      }else{
        iCount = 2;
      }
      bReturn = bReturn && DumpAllLines(g_pDiaSession, dwRVA, dwRange);
    }else{
      // -l            : print line number info
      bReturn = bReturn && DumpAllLines( g_pDiaSession, g_pGlobalSymbol);
      iCount = 1;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-c")){
    // -c                : print section contribution info
    iCount = 1;
    bReturn = bReturn && DumpAllSecContribs(g_pDiaSession);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-dbg")){
    // -dbg              : dump debug streams
    iCount = 1;
    bReturn = bReturn && DumpAllDebugStreams(g_pDiaSession);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-injsrc")){
    if(argc > 1 && *argv[1] != L'-'){
      // -injsrc filename          : dump injected source filename
      bReturn = bReturn && DumpInjectedSource(g_pDiaSession, argv[1]);
      iCount = 2;
    }else{
      // -injsrc           : dump all injected source
      bReturn = bReturn && DumpAllInjectedSources(g_pDiaSession);
      iCount = 1;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-sf")){
    // -sf               : dump all source files
    iCount = 1;
    bReturn = bReturn && DumpAllSourceFiles(g_pDiaSession, g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-oem")){
    // -oem              : dump all OEM specific types
    iCount = 1;
    bReturn = bReturn && DumpAllOEMs(g_pGlobalSymbol);
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-fpo")){
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      if(iswdigit(*argv[1])){
        // -fpo [RVA]        : dump frame pointer omission information for a function address
        swscanf(argv[1], L"%x", &dwRVA);
        bReturn = bReturn && DumpFPO(g_pDiaSession, dwRVA);
      }else{
        // -fpo [symbolname] : dump frame pointer omission information for a function symbol
        bReturn = bReturn && DumpFPO(g_pDiaSession, g_pGlobalSymbol, argv[1]);
      }
      iCount = 2;
    }else{
      bReturn = bReturn && DumpAllFPO(g_pDiaSession);
      iCount = 1;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-compiland")){
    if(argc > 1 && *argv[1] != L'-'){
      // -compiland [name] : dump symbols for this compiland
      bReturn = bReturn && DumpCompiland(g_pGlobalSymbol, argv[1]);
      argc -= 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-line'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-lines")){
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      if(iswdigit(*argv[1])){
        // -lines <RVA>                  : dump line numbers for this address\n"
        swscanf(argv[1], L"%x", &dwRVA);
        bReturn = bReturn && DumpLines(g_pDiaSession, dwRVA);
      }else{
        // -lines <symbolname>           : dump line numbers for this function
        bReturn = bReturn && DumpLines(g_pDiaSession, g_pGlobalSymbol, argv[1]);
      }
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-compiland'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-type")){
    // -type <symbolname>: dump this type in detail
    if(argc > 1 && *argv[1] != L'-'){
      bReturn = bReturn && DumpType(g_pGlobalSymbol, argv[1]);
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-type'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-label")){
    // -label <RVA>      : dump label at RVA
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      swscanf(argv[1], L"%x", &dwRVA);
      bReturn = bReturn && DumpLabel(g_pDiaSession, dwRVA);
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-label'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-sym")){
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      BSTR wszChildname = NULL;
      
      iCount = 2;
      if(argc > 2 && *argv[2] != L'-'){
        wszChildname = argv[2];
        iCount = 3;
      }
      if(iswdigit(*argv[1])){
        // -sym <RVA> [childname]        : dump child information of symbol at this address
        swscanf(argv[1], L"%x", &dwRVA);
        bReturn = bReturn && DumpSymbolWithRVA(g_pDiaSession, dwRVA, wszChildname);
      }else{
        // -sym <symbolname> [childname] : dump child information of this symbol
        bReturn = bReturn && DumpSymbolsWithRegEx(g_pGlobalSymbol, argv[1], wszChildname);
      }
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-sym'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-lsrc")){
    // -lsrc  <file> [line]          : dump line numbers for this source file
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwLine = 0;
      
      iCount = 2;
      if(argc > 2 && *argv[2] != L'-'){
        swscanf(argv[1], L"%d", &dwLine);
        iCount = 3;
      }
      bReturn = bReturn && DumpLinesForSourceFile(g_pDiaSession, argv[1], dwLine);
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-lsrc'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-ps")){
    // -ps <RVA> [-n <number>]       : dump symbols after this address, default 16
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      DWORD dwRange;
      
      swscanf(argv[1], L"%x", &dwRVA);
      if(argc > 3 && !_wcsicmp(argv[2],L"-n")){
        swscanf(argv[3], L"%d", &dwRange);
        iCount = 4;
      }else{
        dwRange = 16;
        iCount = 2;
      }
      bReturn = bReturn && DumpPublicSymbolsSorted(g_pDiaSession, dwRVA, dwRange, true);
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-ps'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-psr")){
    // -psr <RVA> [-n <number>]       : dump symbols before this address, default 16
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      DWORD dwRange;
      
      swscanf(argv[1], L"%x", &dwRVA);
      if(argc > 3 && !_wcsicmp(argv[2],L"-n")){
        swscanf(argv[3], L"%d", &dwRange);
        iCount = 4;
      }else{
        dwRange = 16;
        iCount = 2;
      }
      bReturn = bReturn && DumpPublicSymbolsSorted(g_pDiaSession, dwRVA, dwRange, false);
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-psr'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-annotations")){
    // -annotations <RVA>: dump annotation symbol for this RVA
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      swscanf(argv[1], L"%x", &dwRVA);
      bReturn = bReturn && DumpAnnotations(g_pDiaSession, dwRVA);
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-maptosrc'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-maptosrc")){
    // -maptosrc <RVA>   : dump src RVA for this image RVA
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      swscanf(argv[1], L"%x", &dwRVA);
      bReturn = bReturn && DumpMapToSrc(g_pDiaSession, dwRVA);
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-maptosrc'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else
  if(!_wcsicmp(argv[0],L"-mapfromsrc")){
    // -mapfromsrc <RVA> : dump image RVA for src RVA
    if(argc > 1 && *argv[1] != L'-'){
      DWORD dwRVA = 0;
      
      swscanf(argv[1], L"%x", &dwRVA);
      bReturn = bReturn && DumpMapFromSrc(g_pDiaSession, dwRVA);
      iCount = 2;
    }else{
      wprintf(L"ERROR - ParseArg(): missing argument for option '-mapfromsrc'");
      return false;
    }
    argc -= iCount;
    bReturn = bReturn && ParseArg(argc,&argv[iCount]);
  }else{
    wprintf(L"ERROR - unknown option %s\n",argv[0]);
    PrintHelpOptions();
    return false;
  }
  return bReturn;
}

////////////////////////////////////////////////////////////
// Display the usage
//
void PrintHelpOptions(){
  wchar_t *helpString = L"usage: DiaSample.exe [ options ] <filename>\n"
                        L"  -?                : print this help\n"
                        L"  -all              : print all the debug info\n"
                        L"  -m                : print all the mods\n"
                        L"  -p                : print all the publics\n"
                        L"  -g                : print all the globals\n"
                        L"  -t                : print all the types\n"
                        L"  -f                : print all the files\n"
                        L"  -s                : print symbols\n"
                        L"  -l [RVA [bytes]]  : print line number info at RVA address in the bytes range\n"
                        L"  -c                : print section contribution info\n"
                        L"  -dbg              : dump debug streams\n"
                        L"  -injsrc [file]    : dump injected source\n"
                        L"  -sf               : dump all source files\n"
                        L"  -oem              : dump all OEM specific types\n"
                        L"  -fpo [RVA]        : dump frame pointer omission information for a func addr\n"
                        L"  -fpo [symbolname] : dump frame pointer omission information for a func symbol\n"
                        L"  -compiland [name] : dump symbols for this compiland\n"
                        L"  -lines <funcname> : dump line numbers for this function\n"
                        L"  -lines <RVA>      : dump line numbers for this address\n"
                        L"  -type <symbolname>: dump this type in detail\n"
                        L"  -label <RVA>      : dump label at RVA\n"
                        L"  -sym <symbolname> [childname] : dump child information of this symbol\n"
                        L"  -sym <RVA> [childname]        : dump child information of symbol at this addr\n"
                        L"  -lsrc  <file> [line]          : dump line numbers for this source file\n"
                        L"  -ps <RVA> [-n <number>]       : dump symbols after this address, default 16\n"
                        L"  -psr <RVA> [-n <number>]      : dump symbols before this address, default 16\n"
                        L"  -annotations <RVA>: dump annotation symbol for this RVA\n"
                        L"  -maptosrc <RVA>   : dump src RVA for this image RVA\n"
                        L"  -mapfromsrc <RVA> : dump image RVA for src RVA\n";

  wprintf(helpString);
}

////////////////////////////////////////////////////////////
// Dump all the data stored in a PDB
//
void DumpAllPdbInfo(IDiaSession* pSession, IDiaSymbol* pGlobal){
  DumpAllMods(pGlobal);
  DumpAllPublics(pGlobal);
  DumpAllSymbols(pGlobal);
  DumpAllGlobals(pGlobal);
  DumpAllTypes(pGlobal);
  DumpAllFiles(pSession, pGlobal);
  DumpAllLines(pSession, pGlobal);
  DumpAllSecContribs(pSession);
  DumpAllDebugStreams(pSession);
  DumpAllInjectedSources(pSession);
  DumpAllFPO(pSession);
  DumpAllOEMs(pGlobal);
}

////////////////////////////////////////////////////////////
// Dump all the modules information
//
bool DumpAllMods(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pCompiland;
  ULONG celt = 0;
  ULONG iMod = 1;
  
  wprintf(L"\n\n*** MODULES\n\n");
  
  // Retrieve all the compiland symbols
  if(pGlobal->findChildren(SymTagCompiland,NULL,nsNone,&pEnumSymbols) != S_OK){
    return false;
  }
  while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1){
    BSTR wszName;
    if(pCompiland->get_name(&wszName) != S_OK){
      wprintf(L"ERROR - Failed to get the compiland's name\n");
      pCompiland->Release();
      pEnumSymbols->Release();
      return false;
    }
    wprintf(L"%04X %s\n", iMod++, wszName);
    // Deallocate the string allocated previously by the call to get_name
    SysFreeString(wszName);
    
    pCompiland->Release();
  }
  pEnumSymbols->Release();
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the public symbols - SymTagPublicSymbol
//
bool DumpAllPublics(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n*** PUBLICS\n\n");
  // Retrieve all the public symbols
  if(pGlobal->findChildren(SymTagPublicSymbol,NULL,nsNone,&pEnumSymbols) != S_OK){
    return false;
  }
  while(pEnumSymbols->Next(1, &pSymbol, &celt) == S_OK && celt == 1){
    PrintPublicSymbol(pSymbol);
    pSymbol->Release();
  }
  pEnumSymbols->Release();

  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the symbol information stored in the compilands
//
bool DumpAllSymbols(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaEnumSymbols* pEnumChildren;
  IDiaSymbol* pCompiland;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n*** SYMBOLS\n\n\n");
  
  // Retrieve the compilands first
  if(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols) != S_OK){
    return false;
  }
  while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1) {
    BSTR wszName;
    // Retrieve the name of the module
    wprintf(L"\n** Module: ");
    if(pCompiland->get_name(&wszName) != S_OK){
      wprintf(L"(???)\n\n");
    }else{
      wprintf(L"%s\n\n",wszName);
      SysFreeString(wszName);
    }
    // Find all the symbols defined in this compiland and print their info
    if(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK) {
      ULONG celt_ = 0;
      while (pEnumChildren->Next(1, &pSymbol, &celt_) == S_OK && celt_ == 1) {
        PrintSymbol(pSymbol, 0);
        pSymbol->Release();
      }
      pEnumChildren->Release();
    }
    pCompiland->Release();
  }
  pEnumSymbols->Release();
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the global symbols - SymTagFunction,
//  SymTagThunk and SymTagData
//
bool DumpAllGlobals( IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  enum SymTagEnum dwSymTags[] = {SymTagFunction, SymTagThunk, SymTagData};
  ULONG celt = 0;
  
  wprintf(L"\n\n*** GLOBALS\n\n");
  for(int i = 0; i < sizeof(dwSymTags)/sizeof(dwSymTags[0]); i++, pEnumSymbols = NULL){
    if(pGlobal->findChildren( dwSymTags[i], NULL, nsNone, &pEnumSymbols) == S_OK){
      while(pEnumSymbols->Next(1, &pSymbol, &celt) == S_OK && celt == 1) {
        PrintGlobalSymbol(pSymbol);
        pSymbol->Release();
      }
      pEnumSymbols->Release();
    }else{
      wprintf(L"ERROR - DumpAllGlobals() returned no symbols\n");
      return false;
    }
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the types information
//  (type symbols can be UDTs, enums or typedefs)
//
bool DumpAllTypes( IDiaSymbol* pGlobal ){
  wprintf(L"\n\n*** TYPES\n");
  return DumpAllUDTs(pGlobal) || DumpAllEnums(pGlobal) || DumpAllTypedefs(pGlobal);
}

////////////////////////////////////////////////////////////
// Dump all the user defined types (UDT)
//
bool DumpAllUDTs(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n** UDTs\n\n");
  if(pGlobal->findChildren(SymTagUDT, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) ==S_OK && celt == 1){
      PrintTypeInDetail(pSymbol, 0);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
  }else{
    wprintf(L"ERROR - DumpAllUDTs() returned no symbols\n");
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the enum types from the pdb
//
bool DumpAllEnums(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n** ENUMS\n\n");
  if(pGlobal->findChildren(SymTagEnum, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) ==S_OK && celt == 1){
      PrintTypeInDetail(pSymbol, 0);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
  }else{
    wprintf(L"ERROR - DumpAllEnums() returned no symbols\n");
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the typedef types from the pdb
//
bool DumpAllTypedefs(IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n** TYPEDEFS\n\n");
  if(pGlobal->findChildren(SymTagTypedef, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) ==S_OK && celt == 1){
      PrintTypeInDetail(pSymbol, 0);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
  }else{
    wprintf(L"ERROR - DumpAllTypedefs() returned no symbols\n");
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump OEM specific types
//
bool DumpAllOEMs( IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  wprintf(L"\n\n*** OEM Specific types\n\n");
  if(pGlobal->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) ==S_OK && celt == 1){
      PrintTypeInDetail(pSymbol, 0);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
  }else{
    wprintf(L"ERROR - DumpAllOEMs() returned no symbols\n");
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// For each compiland in the PDB dump all the source files
//
bool DumpAllFiles(IDiaSession* pSession, IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumCompiland;
  IDiaSymbol* pCompiland;
  IDiaEnumSourceFiles* pEnumSrcFile;
  ULONG celt = 0;
  BSTR wszCompilandName;
  
  wprintf(L"\n\n*** FILES\n\n");
  // In order to find the source files, we have to look at the image's compilands/modules
  if(pGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumCompiland) == S_OK){
    while(pEnumCompiland->Next( 1, &pCompiland, &celt) == S_OK){
      if(pCompiland->get_name(&wszCompilandName) == S_OK){
        wprintf(L"\nCompiland = %s\n", wszCompilandName);
        SysFreeString(wszCompilandName);
      }
      // Every compiland could contain multiple references to the source files which were used to build it
      // Retrieve all source files by compiland by passing NULL for the name of the source file
		  if(pSession->findFile(pCompiland, NULL, nsNone, &pEnumSrcFile) == S_OK){
        ULONG celt = 0;
		    IDiaSourceFile* pSrcFile;
        
        while(pEnumSrcFile->Next( 1, &pSrcFile, &celt ) == S_OK){
          PrintSourceFile(pSrcFile);
          wprintf(L"\n");
          pSrcFile->Release();
        }
        pEnumSrcFile->Release();
      }
      pCompiland->Release();
	  }
    pEnumCompiland->Release();
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the line numbering information contained in the PDB
//  Only function symbols have corresponding line numbering information
bool DumpAllLines(IDiaSession* pSession, IDiaSymbol* pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pCompiland;
  ULONG celt = 0;
  
  wprintf(L"\n\n*** LINES\n\n");
  // First retrieve the compilands/modules
  if(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1){
      IDiaEnumSymbols* pEnumFunction;
      
      // for every function symbol defined in the compiland, retrieve and print the line numbering info
      if(pCompiland->findChildren(SymTagFunction, NULL, nsNone, &pEnumFunction) == S_OK){
        IDiaSymbol* pFunction;
        ULONG celt = 0;
        
        while(pEnumFunction->Next(1, &pFunction, &celt) == S_OK && celt == 1){
          PrintLines(pSession, pFunction);
          pFunction->Release();
        }
        pEnumFunction->Release();
      }
      pCompiland->Release();
    }
    pEnumSymbols->Release();
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the line numbering information for a given RVA
// and a given range
//
bool DumpAllLines(IDiaSession* pSession, DWORD dwRVA, DWORD dwRange){
  IDiaEnumLineNumbers* pLines;
  
  // Retrieve and print the lines that corresponds to a specified RVA
  if(pSession->findLinesByRVA(dwRVA, dwRange, &pLines) == S_OK){
    PrintLines(pLines);
    pLines->Release();
  }else{
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the section contributions from the PDB
//
//  Section contributions are stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through 
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
bool DumpAllSecContribs( IDiaSession* pSession){
  IDiaEnumSectionContribs* pEnumSecContribs;
  
  wprintf(L"\n\n*** SECTION CONTRIBUTION\n\n");
  if(GetTable(pSession, __uuidof(IDiaEnumSectionContribs), (void **)&pEnumSecContribs) == S_OK){
    IDiaSectionContrib* pSecContrib;
    ULONG celt = 0;
        
    while(pEnumSecContribs->Next(1, &pSecContrib, &celt) == S_OK && celt == 1){
      PrintSecContribs(pSecContrib);
      pSecContrib->Release();
    }
    pEnumSecContribs->Release();
  }else{
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all debug data streams contained in the PDB
//
bool DumpAllDebugStreams( IDiaSession* pSession){
  IDiaEnumDebugStreams* pEnumStreams;
  
  wprintf(L"\n\n*** DEBUG STREAMS\n\n");
  // Retrieve an enumerated sequence of debug data streams
  if(pSession->getEnumDebugStreams(&pEnumStreams) == S_OK){
    IDiaEnumDebugStreamData* pStream;
    ULONG celt = 0;
    
    for(;pEnumStreams->Next(1, &pStream, &celt) == S_OK; pStream = NULL){
      PrintStreamData(pStream);
      pStream->Release();
    }
    pEnumStreams->Release();
  }else{
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the injected source from the PDB
//
//  Injected sources data is stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through 
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
bool DumpAllInjectedSources( IDiaSession* pSession){
  IDiaEnumInjectedSources* pEnumInjSources;
  
  wprintf(L"\n\n*** INJECTED SOURCES TABLE\n\n");
  if(GetTable(pSession, __uuidof(IDiaEnumInjectedSources), (void **)&pEnumInjSources) == S_OK){
    IDiaInjectedSource* pInjSource;
    ULONG celt = 0;
        
    while(pEnumInjSources->Next(1, &pInjSource, &celt) == S_OK && celt == 1){
      PrintGeneric(pInjSource);
      pInjSource->Release();
    }
    pEnumInjSources->Release();
  }else{
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump info corresponing to a given injected source filename
//
bool DumpInjectedSource(IDiaSession* pSession, wchar_t *wszName){
  IDiaEnumInjectedSources* pEnumInjSources;
  IDiaInjectedSource* pInjSource;
  ULONG celt = 0;
  
  // Retrieve a source that has been placed into the symbol store by attribute providers or
  //  other components of the compilation process
  if(pSession->findInjectedSource(wszName, &pEnumInjSources) == S_OK){
    while(pEnumInjSources->Next(1, &pInjSource, &celt) == S_OK && celt == 1){
      PrintGeneric(pInjSource);
      pInjSource->Release();
    }
    pEnumInjSources->Release();
  }else{
    wprintf(L"ERROR - DumpInjectedSources() could not find %s",wszName);
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the source file information stored in the PDB
// We have to go through every compiland in order to retrieve
//   all the information otherwise checksums for instance
//   will not be available
// Compilands can have multiple source files with the same 
//   name but different content which produces diffrent 
//   checksums
//
bool DumpAllSourceFiles(IDiaSession *pSession, IDiaSymbol *pGlobal){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pCompiland;
  ULONG celt = 0;
  BSTR wszCompName;
  
  wprintf(L"\n\n*** SOURCE FILES\n\n");
  // To get the complete source file info we must go through the compiland first
  //  By passing NULL instead all the source file names only will be retrieved
  if(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1){
      IDiaEnumSourceFiles* pEnumSourceFiles;
      
      pCompiland->get_name(&wszCompName);
      wprintf(L"Compiland = %s\n", wszCompName);
      SysFreeString(wszCompName);
      // Retrieve all the source files from the given compiland 
      if(pSession->findFile(pCompiland, NULL, nsNone, &pEnumSourceFiles) == S_OK){
        IDiaSourceFile* pSrcFile;
        
        while(pEnumSourceFiles->Next(1, &pSrcFile, &celt) == S_OK && celt == 1){
          PrintSourceFile(pSrcFile);
          wprintf(L"\n");
          pSrcFile->Release();
        }
        pEnumSourceFiles->Release();
      }
      wprintf(L"\n");
      pCompiland->Release();
    }
    pEnumSymbols->Release();
  }else{
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the FPO info
//
//  FPO data stored in a table which will be retrieved via
//    IDiaSession->getEnumTables through QueryInterface()
//    using the REFIID of the IDiaEnumFrameData
//
bool DumpAllFPO(IDiaSession* pSession){
  IDiaEnumFrameData* pEnumFrameData;
  
  wprintf(L"\n\n*** FPO\n\n");
  if(GetTable(pSession, __uuidof(IDiaEnumFrameData), (void **)&pEnumFrameData) == S_OK){
    IDiaFrameData* pFrameData;
    ULONG celt = 0;
        
    while(pEnumFrameData->Next(1, &pFrameData, &celt) == S_OK && celt == 1){
      PrintFrameData(pFrameData);
      pFrameData->Release();
    }
    pEnumFrameData->Release();
  }else{
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump FPO info for a function at the specified RVA
//
bool DumpFPO(IDiaSession* pSession, DWORD dwRVA){
  IDiaEnumFrameData* pEnumFrameData;
  
  // Retrieve first the table holding all the FPO info
  if(dwRVA != 0 && GetTable(pSession, __uuidof(IDiaEnumFrameData), (void **)&pEnumFrameData) == S_OK){
    IDiaFrameData* pFrameData;
    
    // Retrieve the frame data corresponding to the given RVA
    if(pEnumFrameData->frameByRVA(dwRVA, &pFrameData) == S_OK){
      PrintGeneric(pFrameData);
      pFrameData->Release();
    }else{
      // Some function might not have FPO data available (see ASM funcs like strcpy)
      wprintf(L"ERROR - DumpFPO() frameByRVA invalid RVA: 0x%x\n", dwRVA);
      pEnumFrameData->Release();
      return false;
    }
    pEnumFrameData->Release();
  }else{
    wprintf(L"ERROR - DumpFPO() GetTable\n");
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump FPO info for a specified function symbol using its 
//  name (a regular expression string is used for the search)
//
bool DumpFPO(IDiaSession* pSession, IDiaSymbol* pGlobal, BSTR wszSymbolName){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  DWORD dwRVA;
    
  // Find first all the function symbols that their names matches the search criteria
  if(pGlobal->findChildren(SymTagFunction, wszSymbolName, nsRegularExpression, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) == S_OK && celt == 1){
      if(pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK){
        PrintPublicSymbol(pSymbol);
        DumpFPO(pSession, dwRVA);
      }
      pSymbol->Release();
    }
    pEnumSymbols->Release();
  }else{
    wprintf(L"ERROR - DumpFPO() findChildren could not find symol %s\n", wszSymbolName);
    return false;
  }
  wprintf(L"\n");
  return true;
}

////////////////////////////////////////////////////////////
// Dump a specified compiland and all the symbols defined in it
//
bool DumpCompiland(IDiaSymbol* pGlobal, BSTR wszCompName){
  IDiaEnumSymbols* pEnumSymbols;
  
  if(pGlobal->findChildren(SymTagCompiland, wszCompName, nsCaseInsensitive, &pEnumSymbols) == S_OK){
    IDiaSymbol* pCompiland;
    ULONG celt = 0;
    BSTR wszName;
    
    while(pEnumSymbols->Next(1, &pCompiland, &celt) == S_OK && celt == 1){
      // Retrieve the name of the module
      wprintf(L"\n** Module: ");
      if(pCompiland->get_name(&wszName) != S_OK){
        wprintf(L"(???)\n\n");
      }else{
        wprintf(L"%s\n\n",wszName);
        SysFreeString(wszName);
      }
      IDiaEnumSymbols* pEnumChildren;
      if(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren) == S_OK) {
        ULONG celt_ = 0;
        IDiaSymbol* pSymbol;
        
        while (pEnumChildren->Next(1, &pSymbol, &celt_) == S_OK && celt_ == 1) {
          PrintSymbol(pSymbol, 0);
          pSymbol->Release();
        }
        pEnumChildren->Release();
      }
      pCompiland->Release();
    }
    pEnumSymbols->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump the line numbering information for a specified RVA
//
bool DumpLines(IDiaSession* pSession, DWORD dwRVA){
  IDiaEnumLineNumbers* pLines;
  
  if(pSession->findLinesByRVA(dwRVA, MAX_RVA_LINES_BYTES_RANGE, &pLines) == S_OK){
    PrintLines(pLines);
    pLines->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump the all line numbering information for a specified
//  function symbol name (as a regular expression string)
//
bool DumpLines(IDiaSession* pSession, IDiaSymbol* pGlobal, BSTR wszFuncName){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pFunction;
  ULONG celt = 0;
  
  if(pGlobal->findChildren(SymTagFunction, wszFuncName, nsRegularExpression, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pFunction, &celt) == S_OK && celt == 1){
      PrintLines(pSession, pFunction);
      pFunction->Release();
    }
    pEnumSymbols->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump the symbol information corresponding to a specified RVA
//
bool DumpSymbolWithRVA(IDiaSession* pSession, DWORD dwRVA, BSTR wszChildname){
  IDiaSymbol* pSymbol;
  LONG lDisplacement;
  bool bReturn;
  
  if(pSession->findSymbolByRVAEx(dwRVA, SymTagNull, &pSymbol, &lDisplacement) == S_OK){
    wprintf(L"Displacement = 0x%x\n", lDisplacement);
    PrintGeneric(pSymbol);
    bReturn = DumpSymbolWithChildren(pSymbol, wszChildname);
    while(pSymbol){
      IDiaSymbol* pParent;
      
      if(pSymbol->get_lexicalParent(&pParent) == S_OK && pParent){
        wprintf(L"\nParent\n");
        PrintSymbol(pParent, 0);
        pSymbol->Release();
        pSymbol = pParent;
      }else{
        break;
      }
    }
    if(pSymbol){
      pSymbol->Release();
    }
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump the symbols information where their names matches a
//  specified regular expression string
//
bool DumpSymbolsWithRegEx(IDiaSymbol* pGlobal, BSTR wszRegEx, BSTR wszChildname){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  bool bReturn = true;
  
  if(pGlobal->findChildren(SymTagNull, wszRegEx, nsRegularExpression, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) == S_OK && celt == 1){
      PrintGeneric(pSymbol);
      bReturn = DumpSymbolWithChildren(pSymbol, wszChildname);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
    return bReturn;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump the information corresponding to a symbol name which
//  is a children of the specified parent symbol
//
bool DumpSymbolWithChildren(IDiaSymbol* pSymbol, BSTR wszChildname){
  IDiaEnumSymbols* pEnumSyms;
  IDiaSymbol* pChild;
  DWORD celt = 1;
  DWORD dwSymTag;
  
  if(wszChildname != NULL){
    if(pSymbol->findChildren(SymTagNull, wszChildname, nsRegularExpression, &pEnumSyms ) == S_OK){
      while(pEnumSyms->Next(celt, &pChild, &celt) == S_OK){
        PrintGeneric(pChild);
        PrintSymbol(pChild, 0);
        pChild->Release();
      }
      pEnumSyms->Release();
    }else{
      return false;
    }
  }else{
    // If the specified name is NULL then only the parent symbol data is displayed
    if(pSymbol->get_symTag(&dwSymTag) == S_OK && dwSymTag == SymTagPublicSymbol){
      PrintPublicSymbol(pSymbol);
    }else{
      PrintSymbol(pSymbol, 0);
    }
  }
  return true;
}

////////////////////////////////////////////////////////////
// Dump all the type symbols information that matches their
//  names to a specified regular expression string
//
bool DumpType(IDiaSymbol* pGlobal, BSTR wszRegEx){
  IDiaEnumSymbols* pEnumSymbols;
  IDiaSymbol* pSymbol;
  ULONG celt = 0;
  
  if(pGlobal->findChildren(SymTagUDT, wszRegEx, nsRegularExpression, &pEnumSymbols) == S_OK){
    while(pEnumSymbols->Next(1, &pSymbol, &celt) == S_OK && celt == 1){
      PrintTypeInDetail(pSymbol, 0);
      pSymbol->Release();
    }
    pEnumSymbols->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump line numbering information for a given file name and
//  an optional line number
//
bool DumpLinesForSourceFile( IDiaSession* pSession, BSTR wszFileName, DWORD dwLine){
  IDiaEnumSourceFiles* pEnumSrcFiles;
  IDiaSourceFile* pSrcFile;
  ULONG celt;
  
  if(pSession->findFile(NULL, wszFileName, nsFNameExt, &pEnumSrcFiles) == S_OK){
    celt = 0;
    while(pEnumSrcFiles->Next(1, &pSrcFile, &celt) == S_OK && celt == 1){
      IDiaEnumSymbols* pEnumCompilands;
      
      if(pSrcFile->get_compilands(&pEnumCompilands ) == S_OK){
        IDiaSymbol* pCompiland;
        BSTR wszCompName;
        
        celt = 0;
        while(pEnumCompilands->Next(1, &pCompiland, &celt) == S_OK && celt == 1){
          IDiaEnumLineNumbers* pLines;
          
          if(pCompiland->get_name(&wszCompName) == S_OK){
            wprintf(L"Compiland = %s\n", wszCompName);
            SysFreeString(wszCompName);
          }else{
            wprintf(L"Compiland = (???)\n");
          }
          if(dwLine != 0){
            if(pSession->findLinesByLinenum(pCompiland, pSrcFile, dwLine, 0, &pLines) == S_OK){
              PrintLines(pLines);
              pLines->Release();
            }
          }else{
            if(pSession->findLines(pCompiland, pSrcFile, &pLines) == S_OK){
              PrintLines(pLines);
              pLines->Release();
            }
          }
          pCompiland->Release();
        }
        pEnumCompilands->Release();
      }
      pSrcFile->Release();
    }
    pEnumSrcFiles->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump public symbol information for a given number of 
//  symbols around a given RVA address
//
bool DumpPublicSymbolsSorted(IDiaSession* pSession, DWORD dwRVA, DWORD dwRange, bool bReverse){
  IDiaEnumSymbolsByAddr* pEnumSymsByAddr;
  IDiaSymbol* pSymbol;
  ULONG celt;
  ULONG i;
  
  if(pSession->getSymbolsByAddr(&pEnumSymsByAddr) == S_OK){
    if(pEnumSymsByAddr->symbolByRVA(dwRVA, &pSymbol) == S_OK){
      if(dwRange == 0){
        PrintPublicSymbol(pSymbol);
      }
      if(bReverse){
        pSymbol->Release();
        i = 0;
        for(pSymbol = NULL; i < dwRange && pEnumSymsByAddr->Next(1, &pSymbol, &celt) == S_OK; i++){
          PrintPublicSymbol(pSymbol);
          pSymbol->Release();
        }
      }else{
        PrintPublicSymbol(pSymbol);
        pSymbol->Release();
        i = 1;
        for(pSymbol = NULL; i < dwRange && pEnumSymsByAddr->Prev(1, &pSymbol, &celt) == S_OK ; i++){
          PrintPublicSymbol(pSymbol);
        }
      }
    }
    pEnumSymsByAddr->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump label symbol information at a given RVA
//
bool DumpLabel(IDiaSession* pSession, DWORD dwRVA){
  IDiaSymbol* pSymbol;
  LONG lDisplacement;
  
  if(pSession->findSymbolByRVAEx(dwRVA, SymTagLabel, &pSymbol, &lDisplacement) == S_OK && pSymbol != NULL){
    wprintf(L"Displacement = 0x%x\n", lDisplacement);
    PrintGeneric(pSymbol);
    pSymbol->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Dump annotation symbol information at a given RVA
//
bool DumpAnnotations(IDiaSession* pSession, DWORD dwRVA){
  IDiaSymbol* pSymbol;
  LONG lDisplacement;
  
  if(pSession->findSymbolByRVAEx(dwRVA, SymTagAnnotation, &pSymbol, &lDisplacement) == S_OK && pSymbol != NULL){
    wprintf(L"Displacement = 0x%x\n", lDisplacement);
    PrintGeneric(pSymbol);
    pSymbol->Release();
    return true;
  }else{
    return false;
  }
}

struct OMAP_DATA{
  DWORD dwRVA;
  DWORD dwRVATo;
};
////////////////////////////////////////////////////////////
// 
bool DumpMapToSrc(IDiaSession* pSession, DWORD dwRVA){
  IDiaEnumDebugStreams* pEnumStreams;
  IDiaEnumDebugStreamData* pStream;
  ULONG celt;
  BSTR wszName;
  
  if(pSession->getEnumDebugStreams(&pEnumStreams) == S_OK){
    celt = 0;
    for(;pEnumStreams->Next(1, &pStream, &celt) == S_OK; pStream = NULL){
      if(pStream->get_name(&wszName) != S_OK){
        wszName = NULL;
      }
      if(wszName && wcscmp(wszName, L"OMAPTO") == 0){
        OMAP_DATA data, datasav;
        DWORD cbData, celt;
        DWORD dwRVATo = 0;
        unsigned int i;
        
        datasav.dwRVATo = 0;
        datasav.dwRVA = 0;
        while(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt) == S_OK){
          if(dwRVA > data.dwRVA){
             datasav = data;
             continue;
          }else if(dwRVA == data.dwRVA){
             dwRVATo = data.dwRVATo;
          }else if(datasav.dwRVATo){
             dwRVATo = datasav.dwRVATo + (dwRVA - datasav.dwRVA);
          }
          break;
        }
        wprintf(L"image rva = %08x ==> source rva = %08x\n\nRelated OMAP entries:\n", dwRVA, dwRVATo);
        wprintf(L"image rva ==> source rva\n");
        wprintf(L"%08x  ==> %08x\n", datasav.dwRVA, datasav.dwRVATo);
        i = 0;
        do{
          wprintf(L"%08x  ==> %08x\n", data.dwRVA, data.dwRVATo);
        }while((++i) < 5 && pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt) == S_OK);
      }
      if(wszName){
        SysFreeString(wszName);
      }
      pStream->Release();
    }
    pEnumStreams->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// 
bool DumpMapFromSrc(IDiaSession* pSession, DWORD dwRVA){
  IDiaEnumDebugStreams* pEnumStreams;
  IDiaEnumDebugStreamData* pStream;
  ULONG celt;
  BSTR wszName;

  if(pSession->getEnumDebugStreams(&pEnumStreams) == S_OK){
    celt = 0;
    for(;pEnumStreams->Next(1, &pStream, &celt) == S_OK; pStream = NULL){
      if(pStream->get_name(&wszName) != S_OK){
        wszName = NULL;
      }
      if(wszName && wcscmp(wszName, L"OMAPFROM") == 0){
        OMAP_DATA data, datasav;
        DWORD cbData, celt;
        DWORD dwRVATo = 0;
        unsigned int i;
        
        datasav.dwRVATo = 0;
        datasav.dwRVA = 0;
        while(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt) == S_OK){
          if(dwRVA > data.dwRVA){
             datasav = data;
             continue;
          }else if(dwRVA == data.dwRVA){
             dwRVATo = data.dwRVATo;
          }else if(datasav.dwRVATo){
             dwRVATo = datasav.dwRVATo + (dwRVA - datasav.dwRVA);
          }
          break;
        }
        wprintf(L"source rva = %08x ==> image rva = %08x\n\nRelated OMAP entries:\n", dwRVA, dwRVATo);
        wprintf(L"source rva ==> image rva\n");
        wprintf(L"%08x  ==> %08x\n", datasav.dwRVA, datasav.dwRVATo);
        i = 0;
        do{
          wprintf(L"%08x  ==> %08x\n", data.dwRVA, data.dwRVATo);
        }while((++i) < 5 && pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt) == S_OK);
      }
      if(wszName){
        SysFreeString(wszName);
      }
      pStream->Release();
    }
    pEnumStreams->Release();
    return true;
  }else{
    return false;
  }
}

////////////////////////////////////////////////////////////
// Retreive the table that matches the given iid
//
//  A PDB table could store the section contributions, the frame data,
//  the injected sources
//
HRESULT GetTable(IDiaSession *pSession, REFIID iid, void **ppUnk){
  IDiaEnumTables* pEnumTables;
  IDiaTable* pTable;
  ULONG celt = 0;
  
  if(pSession->getEnumTables(&pEnumTables) != S_OK){
    wprintf(L"ERROR - GetTable() getEnumTables\n");
    return E_FAIL;
  }
  while(pEnumTables->Next(1, &pTable, &celt) == S_OK && celt == 1){
    // Thre's only one table that matches the given iid
    if(pTable->QueryInterface(iid, (void**)ppUnk) == S_OK){
      pTable->Release();
      pEnumTables->Release();
      return S_OK;
    }
    pTable->Release();
  }
  pEnumTables->Release();
  return E_FAIL;
}


