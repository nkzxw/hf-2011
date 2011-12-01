#include <ntddk.h>


void DisplayInfo()
{
	PEPROCESS pEprocess = NULL, pTemp = NULL;
	pEprocess = PsGetCurrentProcess();
	pTemp = pEprocess;
	do 
	{
		DbgPrint("PId:%d\t", *(PULONG)((ULONG)pTemp+0x084) );
		DbgPrint("Path:%s\n", (PUCHAR)((ULONG)pTemp+0x174 ));
		pTemp = (PEPROCESS)( (ULONG)(*(PULONG)((ULONG)pTemp+0x088)) - 0x088 );

	} while ( pTemp != pEprocess );
}


void UnLoad( PDRIVER_OBJECT pDriverObject )
{
	DbgPrint("Driver UnLoad..");
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObect, IN PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;

	DisplayInfo();

	pDriverObect->DriverUnload = UnLoad;

	return Status;
}