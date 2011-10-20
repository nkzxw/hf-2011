#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

extern "C"
{


typedef struct MOD_SYM *PMOD_SYM;

NTSTATUS
SymGetSymbolByAddress(
	IN PMOD_SYM Sym,
	IN PVOID Address,
	OUT PCHAR Symbol,
	OUT ULONG *SymLen
	);


NTSTATUS
SymGetSymbolByName(
	IN PMOD_SYM Sym,
	IN PCHAR Symbol,
	OUT ULONG *SymAddr
	);

VOID
SymInitialize(
	BOOLEAN CanUseW32
	);

NTSTATUS
SymLoadSymbolFile(
	IN PWSTR ModuleName,
	IN PVOID ImageBase OPTIONAL
	);

VOID
SymUnloadSymbolFile(
	PMOD_SYM Sym
	);

VOID
SymFreeSymbolTables(
	);

NTSTATUS
SymGlobGetSymbolByName(
	IN PCHAR Symbol,
	OUT ULONG *SymAddr
	);

NTSTATUS
SymGlobGetSymbolByAddress(
	IN PVOID Address,
	OUT PCHAR Symbol,
	IN OUT ULONG *SymLen
	);

NTSTATUS
SymGlobGetNearestSymbolByAddress(
	IN PVOID Address,
	OUT PCHAR Symbol,
	IN OUT ULONG *SymLen,
	OUT ULONG *Distance
	);

NTSTATUS
SymWrGetNearestSymbolByAddress(
	IN PVOID Address,
	OUT PCHAR Symbol,
	IN OUT ULONG *SymLen
	);

}

#endif
