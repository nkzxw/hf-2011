#include <ntifs.h>
#include "winnt.h"

PEPROCESS
GetProcessByName(
	PWCHAR wszProcessName,
	PETHREAD *FirstThread OPTIONAL
	);


PSERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTableShadow;
PVOID W32BaseAddress;
PVOID g_SelfBase;
PVOID WinStbBase;


BOOLEAN
FindShadowTable(
	)
/**
	Find KeServiceDescriptorTableShadow
*/
{
	ULONG Sdt = (ULONG) KeServiceDescriptorTable;

	KdPrint(("KeServiceDescriptorTable = %X\n", Sdt));

	for (ULONG Shadow = Sdt - PAGE_SIZE; Shadow < Sdt + PAGE_SIZE; Shadow += sizeof(SERVICE_DESCRIPTOR_TABLE))
	{
		if (MmIsAddressValid((PVOID)Shadow))
		{
			SERVICE_DESCRIPTOR_TABLE *ShadowPtr = (SERVICE_DESCRIPTOR_TABLE*) Shadow;

			if (ShadowPtr != KeServiceDescriptorTable &&
				ShadowPtr->ServiceTable == KeServiceDescriptorTable->ServiceTable &&
				ShadowPtr->ArgumentTable == KeServiceDescriptorTable->ArgumentTable &&
				ShadowPtr->TableSize == KeServiceDescriptorTable->TableSize)
			{
				KeServiceDescriptorTableShadow = ShadowPtr;
				return TRUE;
			}
		}
	}

	return FALSE;
}


PVOID
RtlFindImageProcedureByNameOrPointer(
	IN PVOID Base,
	IN PCHAR FunctionName OPTIONAL,
	IN PVOID FunctionEntry OPTIONAL
	)
{
	PIMAGE_DOS_HEADER mz;
	PIMAGE_FILE_HEADER pfh;
	PIMAGE_OPTIONAL_HEADER poh;
	PIMAGE_EXPORT_DIRECTORY pexd;
	PULONG AddressOfFunctions;
	PULONG AddressOfNames;
	PUSHORT AddressOfNameOrdinals;
	ULONG i;
	
	// Get headers
	*(PUCHAR*)&mz = (PUCHAR)Base;
	*(PUCHAR*)&pfh = (PUCHAR)Base + mz->e_lfanew + sizeof(IMAGE_NT_SIGNATURE);
	*(PIMAGE_FILE_HEADER*)&poh = pfh + 1;

	// Get export
	*(PUCHAR*)&pexd = (PUCHAR)Base + poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	*(PUCHAR*)&AddressOfFunctions = (PUCHAR)Base + pexd->AddressOfFunctions;
	*(PUCHAR*)&AddressOfNames = (PUCHAR)Base + pexd->AddressOfNames;
	*(PUCHAR*)&AddressOfNameOrdinals = (PUCHAR)Base + pexd->AddressOfNameOrdinals;

	// Find function
	for( i=0; i<pexd->NumberOfNames; i++ ) 
	{
		PCHAR name = ((char*)Base + AddressOfNames[i]);
		PVOID addr = (PVOID*)((ULONG)Base + AddressOfFunctions[AddressOfNameOrdinals[i]]);

		if (ARGUMENT_PRESENT (FunctionName))
		{
			if( !strcmp( name, FunctionName ) ) 
			{
				return addr;
			}
		}
		else if (ARGUMENT_PRESENT (FunctionEntry))
		{
			if (FunctionEntry == addr)
				return name;
		}
		else
		{
			ASSERTMSG ("SHOULD NOT REACH HERE", ARGUMENT_PRESENT(FunctionName) || ARGUMENT_PRESENT(FunctionEntry));
		}
	}
	
	return NULL;
}




PEPROCESS CsrProcess;
KAPC_STATE ApcState;

VOID
W32PrepareCall(
	)
{
	KeStackAttachProcess ((PKPROCESS)CsrProcess, &ApcState);
}

VOID
W32ReleaseCall(
	)
{
	KeUnstackDetachProcess (&ApcState);
}

NTSTATUS
W32FindAndSwapIAT(
	)

/*++

Routine Description

	This function performs search of WIN32K.SYS image and fill our
	 IAT enties of winstb.sys with win32k.sys export symbols.

Arguments

	None

Return Address
	
	NTSTATUS of operation

--*/

{
	NTSTATUS Status = STATUS_NOT_FOUND;

	if ((g_SelfBase = FindSelfBase()) == NULL)
		goto _exit0;

	KdPrint(("Found self base at %X\n", g_SelfBase));

	if (!CsrProcess)
	{
		CsrProcess = GetProcessByName (L"csrss.exe", NULL);
		if (!CsrProcess)
		{
			KdPrint(("Could not find csrss\n"));
			goto _exit0;
		}
	}

	KdPrint(("CsrProcess %x\n", CsrProcess));

	W32PrepareCall ();

	if (!FindShadowTable())
		goto _exit1;

	KdPrint(("Found shadow table at %X\n", KeServiceDescriptorTableShadow));

	if (!FindBaseAndSize (KeServiceDescriptorTableShadow[1].ServiceTable, &W32BaseAddress, NULL))
		goto _exit1;

	KdPrint(("Found win32k base at %X\n", W32BaseAddress));

	WinStbBase = __base();
	KdPrint(("winstb base %X\n", WinStbBase));

	if (!WinStbBase)
		goto _exit1;

	// Get nt headers
	PIMAGE_NT_HEADERS W32NtHeaders = (PIMAGE_NT_HEADERS) RtlImageNtHeader (W32BaseAddress);
	PIMAGE_NT_HEADERS SelfNtHeaders = (PIMAGE_NT_HEADERS) RtlImageNtHeader (g_SelfBase);
	ULONG ImportDescriptorSize = 0;

	// Get our import
	PIMAGE_IMPORT_DESCRIPTOR ImpDesc = (PIMAGE_IMPORT_DESCRIPTOR) 
		RtlImageDirectoryEntryToData( g_SelfBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportDescriptorSize);

	KdPrint(("ImpDesc = %X  Size = %X\n", ImpDesc, ImportDescriptorSize));

	if (ImpDesc == NULL)
	{
		Status = STATUS_INVALID_IMAGE_FORMAT;
		goto _exit1;
	}

	for (; ImpDesc->Name; ImpDesc++)
	{
		char *DllName = (LPSTR) RVATOVA (ImpDesc->Name, g_SelfBase);
		KdPrint(("Dllname %s\n", DllName));

		char LocalDllName[128];
		strcpy (LocalDllName, DllName);

		_strupr (LocalDllName);

		if (!strcmp (LocalDllName, "WINSTB.SYS"))
		{
			//
			// Find WINSTB.SYS
			//

			ULONG RvaOfThunks = ImpDesc->FirstThunk;

			ASSERT (ImpDesc->TimeDateStamp != -1);

			for (PIMAGE_THUNK_DATA Thunk = (PIMAGE_THUNK_DATA)RVATOVA(RvaOfThunks,g_SelfBase); Thunk->u1.Function; Thunk++)
			{
				PCHAR Name = (PCHAR) RtlFindImageProcedureByNameOrPointer (WinStbBase, 0, (PVOID)Thunk->u1.Function);

				KdPrint(("Function = %X, Name %s ", Thunk->u1.Function, Name));

				if (strcmp (Name, "__base") != 0)
				{
					PVOID Entry = RtlFindImageProcedureByNameOrPointer (W32BaseAddress, Name, 0);
					KdPrint(("W32 = %X", Entry));

					Thunk->u1.Function = (ULONG) Entry;
				}

				KdPrint(("\n"));
			}

			Status = STATUS_SUCCESS;
			break;
		}
	}

_exit1:
_exit0:
	return Status;
}

