#ifndef _NTAPI_H
#define _NTAPI_H

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG	Reserved[2];
    PVOID	BaseAddress;
    ULONG	Size; 
    ULONG	Flags;
    USHORT  Index;
    USHORT  Rank;
    USHORT  LoadCount;
    USHORT  NameOffset;
    CHAR	Name [256];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

NTSYSAPI 
NTSTATUS 
NTAPI
ZwQuerySystemInformation(
	ULONG	sic, 
	PVOID	pData, 
	ULONG	dSize, 
	PULONG	pdSize
	);

extern
BOOLEAN 
GetNdisModuleAddress(
	);

void 
PrintModule(
	PSYSTEM_MODULE_INFORMATION pModule
	);

extern 
VOID 
PrintBlock(
	PVOID start, 
	ULONG offset
	);

extern
VOID 
PrintBlockEx(
	PVOID start, 
	ULONG offset
	);

extern 
PVOID 
HookFunction(
	PVOID pBaseAddress, 
	PCSTR Name,
	PVOID InFunc,
	ULONG* OutFunc
	);

extern
PVOID 
UnHookFunction(
	PVOID pBaseAddress, 
	PCSTR Name, 
	PVOID UnHookFunc
	);



extern PVOID m_NdisBaseAddress;



#endif //_NTAPI_H	