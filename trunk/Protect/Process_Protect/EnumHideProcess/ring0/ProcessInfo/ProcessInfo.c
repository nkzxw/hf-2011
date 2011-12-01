#include <ntddk.h>
#include "ProcessInfo.h"

void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Unload Driver Success...");
}


NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;

	pDriverObject->DriverUnload = UnLoad;
	
	DisplayInfo();

	return Status;

}


void DisplayInfo()
{
	char *pBuff = NULL, *pTemp = NULL;
	ULONG RealLen = 0;
	ULONG uRet = 0;
	ANSI_STRING ansi;

	NTSTATUS Status = STATUS_SUCCESS;
	NtQuerySystemInformation( 5, pBuff, 0, &uRet );

	pBuff = (PCHAR)ExAllocatePool( NonPagedPool, uRet );
	pTemp = pBuff;

	Status = NtQuerySystemInformation( 5, pBuff, uRet, NULL );
	if( NT_SUCCESS(Status) )
	{
		
		while( 1 )
		{
			PSYSTEM_PROCESS_INFORMATION pSystemProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pTemp;
			RtlUnicodeStringToAnsiString( &ansi, &pSystemProcessInfo->ImageName, TRUE );
			DbgPrint("PId:%d\t", pSystemProcessInfo->ProcessId);
			DbgPrint("Path:%s\n", ansi.Buffer );
			RtlFreeAnsiString( &ansi );

			if( pSystemProcessInfo->NextEntryOffset == 0 )
				break;
			pTemp = pTemp+pSystemProcessInfo->NextEntryOffset;

		}
	
	}
	else
	{
		DbgPrint("NtQuerySyetemInformation2 false");
		return;
	}

	ExFreePool( pBuff );
}
