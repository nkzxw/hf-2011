extern "C" 
{
#include <ntddk.h>
}

NTSTATUS BootStartUp();
VOID BootCleanUp();


// Unload routine
void DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[~] NGBOOT: DriverUnload enter\n"));
	BootCleanUp ();
	KdPrint(("[+] NGBOOT: DriverUnload exit\n"));
}

// Driver entry point
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("[~] NGBOOT: DriverEntry() enter\n"));
	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS Status = BootStartUp();

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[-] NGBOOT: ngdbg!BootStartUp() failed with status %X\n", Status));
		return Status;
	}

	KdPrint(("[+] NGBOOT: DriverEntry() exit\n"));
	return STATUS_SUCCESS;
}
