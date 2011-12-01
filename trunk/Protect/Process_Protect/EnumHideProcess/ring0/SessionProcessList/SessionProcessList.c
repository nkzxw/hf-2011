#include <ntddk.h>

void DisplayInfo();
void UnLoad( PDRIVER_OBJECT pDriverobject );

NTSTATUS DriverEntry( PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;
	pDriverObject->DriverUnload = UnLoad;
	DisplayInfo();
	return Status;
}

void DisplayInfo()
{
	PEPROCESS pEProcess = PsGetCurrentProcess();
	PEPROCESS pTemp = pEProcess;
	DbgPrint("PId:%d\tPath:%s\n", *(PULONG)((ULONG)pTemp+0x84), (PUCHAR)((ULONG)pTemp+0x174) );
    pTemp = (PEPROCESS)( *(PULONG)(*(PULONG)((ULONG)pTemp + 0x8c) + 0x4) - 0x88 );//取用户进程
	pEProcess = pTemp;
	do 
	{
	
		if( MmIsAddressValid(pTemp) )
		{
			DbgPrint("PId:%d\tPath:%s\n", *(PLONG)((LONG)pTemp+0x84), (PUCHAR)((ULONG)pTemp+0x174) );
			pTemp = (PEPROCESS)(*(PULONG)((ULONG)pTemp+0xb4) - 0xb4 );
		}
		else
			break;
		
	} while ( pEProcess != pTemp );

}


void UnLoad( PDRIVER_OBJECT pDriverobject )
{
	DbgPrint("Driver Unload..");
}