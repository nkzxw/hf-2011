#include "ntifs.h"
#include "windef.h"
#include "file.h"

ULONG GlobalDeviceAndSymbole[32][3] = { 0 };
PVOID	GlobalDeviceObject = NULL;
//
// Object Type Structure
//

typedef struct _OBJECT_TYPE_INITIALIZER {
    USHORT Length;
    BOOLEAN UseDefaultObject;
    BOOLEAN CaseInsensitive;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    BOOLEAN MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    PVOID DumpProcedure;
    PVOID OpenProcedure;
    PVOID CloseProcedure;
	PVOID DeleteProcedure;
    PVOID ParseProcedure;
    PVOID SecurityProcedure;
    PVOID QueryNameProcedure;
    PVOID OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

#define OBJECT_LOCK_COUNT 4

typedef struct _OBJECT_TYPE {
    ERESOURCE Mutex;
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;            // Copy from object header for convenience
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
#ifdef POOL_TAGGING
    ULONG Key;
#endif //POOL_TAGGING
    ERESOURCE ObjectLocks[ OBJECT_LOCK_COUNT ];
} OBJECT_TYPE, *POBJECT_TYPE;

extern POBJECT_TYPE *IoDeviceObjectType;

void * pOldParseProcedure = NULL;

NTSTATUS
pNewParseProcedure(
	IN PVOID ParseObject,
	IN PVOID ObjectType,
	IN OUT PACCESS_STATE AccessState,
	IN KPROCESSOR_MODE AccessMode,
	IN ULONG Attributes,
	IN OUT PUNICODE_STRING CompleteName,
	IN OUT PUNICODE_STRING RemainingName,
	IN OUT PVOID Context OPTIONAL,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
	OUT PVOID *Object) 
{
	NTSTATUS ntStatus = STATUS_SUCCESS; 
	UNICODE_STRING usNewCompleteName ;
	UNICODE_STRING usNewRemainingName ;
	UNICODE_STRING usMyName;
	ULONG	i;
	PDEVICE_OBJECT DeviceObject = ParseObject;

	RtlInitUnicodeString( &usMyName,L"\\1.txt" );
	if (RemainingName->Buffer && 
		RtlEqualUnicodeString( RemainingName,&usMyName,TRUE ))
	{
		RtlInitUnicodeString( &usNewRemainingName, L"\\2.txt" );
		// 以下这个貌似可以直接不清空不用
		RtlInitUnicodeString( &usNewCompleteName, L"\\Device\\FileDisk\\FileDisk0\\2.txt" );
		DbgPrint("found match! out DeviceObject = 0x%x, %wZ, %wZ\n", 
			ParseObject, &(DeviceObject->DriverObject)->DriverName, 
			CompleteName);

		for (i = 0; i < 32; i++) {
			
			if (GlobalDeviceAndSymbole[i][0] == (ULONG)ParseObject) {

				if (*(WCHAR *)(&GlobalDeviceAndSymbole[i][1]) == L'C' || 
					*(WCHAR *)(&GlobalDeviceAndSymbole[i][1]) == L'c') {

					DbgPrint("found match! %x\n", GlobalDeviceObject);
					if (!GlobalDeviceObject) {

						ParseObject = GlobalDeviceObject;
					}
					break;
				}

				DbgPrint("pNewParseProcedure: Find ParseObject %S\n", &GlobalDeviceAndSymbole[i][1]);
			}
			if (GlobalDeviceAndSymbole[i][0] == (ULONG)NULL) {
				break;
			}
		}

		__asm
		{
			push eax
			push Object
			push SecurityQos
			push Context
			lea eax,usNewRemainingName
			push eax
			lea eax,usNewCompleteName
			push eax
			push Attributes
			movzx eax, AccessMode
			push eax
			push AccessState
			push ObjectType
			push ParseObject
			call pOldParseProcedure
			mov ntStatus, eax
			pop eax
		} 
	} else {

		__asm
		{
			push eax
			push Object
			push SecurityQos
			push Context
			push RemainingName
			push CompleteName
			push Attributes
			movzx eax, AccessMode
			push eax
			push AccessState
			push ObjectType
			push ParseObject
			call pOldParseProcedure
			mov ntStatus, eax
			pop eax
		}
	}
	return ntStatus;
}


NTSTATUS InstallHook()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	pOldParseProcedure = ((POBJECT_TYPE)*IoDeviceObjectType)->TypeInfo.ParseProcedure;
	if (!MmIsAddressValid(pOldParseProcedure))
	{
		KdPrint(("InstallHook: pOldParseProcedure = %x\n", pOldParseProcedure)); 
		return STATUS_UNSUCCESSFUL;
	}

	__asm
	{
		cli;
		mov eax, cr0;
		and eax, not 10000h;
		mov cr0, eax;
	}

	((POBJECT_TYPE)*IoDeviceObjectType)->TypeInfo.ParseProcedure = pNewParseProcedure;

	__asm
	{
		mov eax, cr0;
		or eax, 10000h;
		mov cr0, eax;
		sti;
	}

	KdPrint(("InstallHook: Success pOldParseProcedure = %x\n", pOldParseProcedure)); 
	return ntStatus;
}

void UnInstallHook()
{
	
	if (!MmIsAddressValid(pOldParseProcedure))
	{
		return;
	}

	__asm
	{
		cli;
		mov eax, cr0;
		and eax, not 10000h;
		mov cr0, eax;
	}
	
	((POBJECT_TYPE)*IoDeviceObjectType)->TypeInfo.ParseProcedure = pOldParseProcedure;

	__asm
	{
		mov eax, cr0;
		or eax, 10000h;
		mov cr0, eax;
		sti;
	}
}
 
void DriverUnload( PDRIVER_OBJECT DriverObject )
{

	UnInstallHook();
}

void fnInitDeviceDosName();

NTSTATUS DriverEntry( 
	PDRIVER_OBJECT DriverObject, 
	PUNICODE_STRING RegistryPath 
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	DriverObject->DriverUnload = DriverUnload;

	fnInitDeviceDosName();

	status = InstallHook();

	if (NT_SUCCESS(status)) {

		KdPrint(("DriverEntry: InstallHook success\n"));
	} else {
	}

	KdPrint(("field TypeInfo = %x, ParseProcedure = %x\n", 
		FIELD_OFFSET(OBJECT_TYPE, TypeInfo), FIELD_OFFSET(OBJECT_TYPE_INITIALIZER, ParseProcedure)));
	return status;
}

void fnInitDeviceDosName()
{
	NTSTATUS			Status;
	PDEVICE_OBJECT		DeviceObject;
	PDRIVER_OBJECT		DriverObject;
	UNICODE_STRING		DeviceName;
	UNICODE_STRING		DosName = { 0 };
	ULONG				i;
	PVPB				Vpb = NULL;
	WCHAR				Buffer[0x200] = { 0 };
	POBJECT_NAME_INFORMATION Info = (POBJECT_NAME_INFORMATION)Buffer;
	ULONG				RetLength = 0;
	UNICODE_STRING		VolumeName = { 0 };

	RtlInitUnicodeString( &DeviceName, L"\\Driver\\Ftdisk" );
	Status = ObReferenceObjectByName(
		&DeviceName, 
		OBJ_CASE_INSENSITIVE, 
		NULL, 
		0, 
		*IoDriverObjectType, 
		KernelMode, 
		NULL, 
		(PVOID*)&DriverObject);

	if (!NT_SUCCESS(Status)) {

		RtlInitUnicodeString(&DeviceName, L"\\Driver\\volmgr");
		Status = ObReferenceObjectByName(
			&DeviceName, 
			OBJ_CASE_INSENSITIVE, 
			NULL, 
			0, 
			*IoDriverObjectType, 
			KernelMode, 
			NULL, 
			(PVOID*)&DriverObject);
		if (!NT_SUCCESS(Status)) {

			return;
		}
	}
	
	ObDereferenceObject(DriverObject);
	DeviceObject = DriverObject->DeviceObject;
	
	i = 0;
	RtlZeroMemory(GlobalDeviceAndSymbole, sizeof(GlobalDeviceAndSymbole));
	do {
		
		Status = IoVolumeDeviceToDosName(DeviceObject, &DosName);
		if (NT_SUCCESS(Status)) {

			GlobalDeviceAndSymbole[i][0] =  (ULONG )DeviceObject;
			GlobalDeviceAndSymbole[i][1] = *(ULONG*)DosName.Buffer;

			if (*(WCHAR *)DosName.Buffer == L'E' || *(WCHAR *)DosName.Buffer == L'e') {
				
				GlobalDeviceObject = (PVOID)DeviceObject;
			}

			if (MmIsAddressValid(DeviceObject->Vpb)) {

				if (MmIsAddressValid(DeviceObject->Vpb->DeviceObject)) {

					Vpb = DeviceObject->Vpb;
					GlobalDeviceAndSymbole[i][2] = (ULONG)Vpb->DeviceObject->DriverObject;
				}
			}
			i++;
		} else {

			//KdPrint(("fnInitDeviceDosName: IoVolumeDeviceToDosName Failed %X\n", Status));
		}
		DeviceObject = DeviceObject->NextDevice;
		if(DosName.Buffer)
			ExFreePool(DosName.Buffer);
		DosName.Buffer = NULL;
	} while (DeviceObject);
	
	if (GlobalDeviceObject) {

		return;
	}

	RtlInitUnicodeString( &DeviceName, L"\\Driver\\FileDisk" );
	Status = ObReferenceObjectByName(
		&DeviceName, 
		OBJ_CASE_INSENSITIVE, 
		NULL, 
		0, 
		*IoDriverObjectType, 
		KernelMode, 
		NULL, 
		(PVOID*)&DriverObject);

	if (!NT_SUCCESS(Status)) {

		return;
	}
	
	ObDereferenceObject(DriverObject);
	DeviceObject = DriverObject->DeviceObject;

	RtlInitUnicodeString( &VolumeName, L"\\Device\\FileDisk\\FileDisk0" );

	do {

		RtlZeroMemory(Buffer, 0x200*sizeof(WCHAR));
		Status = ObQueryNameString(DeviceObject, Info, 0x200*sizeof(WCHAR), &RetLength);
		if (NT_SUCCESS(Status) && RtlEqualUnicodeString( &Info->Name, &VolumeName, TRUE )) {
				
			KdPrint(("fnInitDeviceDosName: DeviceObject = %x %wZ\n", DeviceObject, &Info->Name));
			GlobalDeviceObject = (PVOID)DeviceObject;
			break;
		}

		DeviceObject = DeviceObject->NextDevice;
	} while (DeviceObject);
}