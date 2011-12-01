
#include <ntddk.h>
#include "HandleInfo.h"


void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Driver Unload");
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;

	DisplayInfo();

	pDriverObject->DriverUnload = UnLoad;

	return Status;
} 

void DisplayInfo()
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCHAR pBuff = NULL;
	ULONG uNums = 0;
	ULONG uRet = 0;
	PSYSTEM_HANDLE_INFORMATION_EX pSystemHandle = NULL;

	pBuff = (PCHAR)ExAllocatePool( NonPagedPool, 100 );

	Status = ZwQuerySystemInformation( 16, pBuff, 100, &uRet );
	ExFreePool( pBuff );

	pBuff = (PCHAR)ExAllocatePool( NonPagedPool, uRet );

	Status = ZwQuerySystemInformation( 16, pBuff, uRet, NULL );
	if( NT_SUCCESS(Status) )
	{
		ULONG index = 0;
		LONG PId = -1;
		pSystemHandle = (PSYSTEM_HANDLE_INFORMATION_EX)pBuff;
		uNums = pSystemHandle->NumberOfHandles;
	
		for(; index < uNums; index++ )
		{
			if( pSystemHandle->Information[index].ProcessId != PId )
			{
				PId =  pSystemHandle->Information[index].ProcessId;
				Display( pSystemHandle->Information[index].ProcessId );
			}
		}
			
	}
	else
	{
		DbgPrint("NtQuerySystemInformation False");
		return;
	}

}

void Display( ULONG PId )
{
	PEPROCESS pEprocess = 0;
	NTSTATUS Status = STATUS_SUCCESS;
	UCHAR name[20] = {0};
	Status = PsLookupProcessByProcessId( (HANDLE)PId, &pEprocess );
	if( !NT_SUCCESS(Status) )
	{
		DbgPrint("PsLookupProcessByProcessId False");
		return;
	}
	memcpy( name, (PCHAR)((ULONG)pEprocess+0x174), 16 );
	DbgPrint("PId:%d\t", PId );
	DbgPrint("Path:%s\n", name );

}