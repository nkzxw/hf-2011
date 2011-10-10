#include "dia2.h"

extern BSTR g_wszFilename;
extern IDiaDataSource* g_pDiaDataSource;
extern IDiaSession* g_pDiaSession;
extern IDiaSymbol* g_pGlobalSymbol;
extern DWORD g_dwMachineType;

int wmain( int , wchar_t*[] );
void PrintHelpOptions();
bool ParseArg( int , wchar_t*[] );

void Cleanup();
bool LoadDataFromPdb( wchar_t* , IDiaDataSource** , IDiaSession** , IDiaSymbol** );

void DumpAllPdbInfo( IDiaSession*, IDiaSymbol* );
bool DumpAllMods( IDiaSymbol* );
bool DumpAllPublics( IDiaSymbol* );
bool DumpCompiland( IDiaSymbol* , BSTR );
bool DumpAllSymbols( IDiaSymbol* );
bool DumpAllGlobals( IDiaSymbol* );
bool DumpAllTypes( IDiaSymbol* );
bool DumpAllUDTs( IDiaSymbol* );
bool DumpAllEnums( IDiaSymbol* );
bool DumpAllTypedefs( IDiaSymbol* );
bool DumpAllOEMs( IDiaSymbol* );
bool DumpAllFiles( IDiaSession* , IDiaSymbol* );
bool DumpAllLines( IDiaSession* , IDiaSymbol* );
bool DumpAllLines( IDiaSession* , DWORD , DWORD );
bool DumpAllSecContribs( IDiaSession* );
bool DumpAllDebugStreams( IDiaSession* );
bool DumpAllInjectedSources( IDiaSession* );
bool DumpInjectedSource( IDiaSession* , BSTR );
bool DumpAllSourceFiles( IDiaSession* , IDiaSymbol* );
bool DumpAllFPO( IDiaSession* );
bool DumpFPO( IDiaSession* , DWORD );
bool DumpFPO( IDiaSession* , IDiaSymbol* , BSTR );
bool DumpSymbolWithRVA( IDiaSession* , DWORD , BSTR );
bool DumpSymbolsWithRegEx( IDiaSymbol* , BSTR , BSTR );
bool DumpSymbolWithChildren( IDiaSymbol* , BSTR );
bool DumpLines( IDiaSession* , DWORD );
bool DumpLines( IDiaSession* , IDiaSymbol* , BSTR );
bool DumpType( IDiaSymbol* , BSTR );
bool DumpLinesForSourceFile( IDiaSession* , BSTR , DWORD );
bool DumpPublicSymbolsSorted( IDiaSession* , DWORD , DWORD , bool );
bool DumpLabel( IDiaSession* , DWORD );
bool DumpAnnotations( IDiaSession* , DWORD );
bool DumpMapToSrc( IDiaSession* , DWORD );
bool DumpMapFromSrc( IDiaSession* , DWORD );

HRESULT GetTable( IDiaSession* , REFIID , void** );

///////////////////////////////////////////////////////////////////
// Functions defined in regs.cpp
const wchar_t* SzNameC7Reg( USHORT , DWORD );
const wchar_t* SzNameC7Reg( USHORT );
