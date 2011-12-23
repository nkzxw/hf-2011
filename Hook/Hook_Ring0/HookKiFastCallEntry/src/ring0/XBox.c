/*++

Module Name:

    XBox.c [sys module]

    
Abstract:

    记录系统服务被某个特定进程调用的情况


Author: 

    Fypher [nmn714@163.com]

    http://hi.baidu.com/nmn714


Time:
    2010/04/16

--*/
#include<ntifs.h>

#define IOCTL_XBOX_SETPID\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xa01, METHOD_BUFFERED,\
	FILE_WRITE_DATA )

#define IOCTL_XBOX_READ\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xa02, METHOD_BUFFERED,\
	FILE_READ_DATA )

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	PULONG ServiceTableBase;
	PULONG ServiceCounterTableBase;
	ULONG NumberOfServices;
	PUCHAR ParamTableBase;
} SSDT_ENTRY;
#pragma pack()
__declspec(dllimport) SSDT_ENTRY KeServiceDescriptorTable;

typedef struct _XBoxData_ {
	ULONG pid;
	ULONG tid;
	ULONG sid;
	UCHAR argc;
	UCHAR bFromUser;
	UCHAR bFromSSDT;
	ULONG args[16];
	LARGE_INTEGER time;
	LIST_ENTRY ListEntry;
} XBoxData, *PXBoxData, **PPXBoxData;

ULONG g_ulHookPoint, g_ulRetAddr;
UCHAR g_HookBytes[5] = {0xE9, 0x00, 0x00, 0x00, 0x00};
UCHAR g_OrigBytes[5];

ULONG g_nPid = 0xFFFFFFFF;
ULONG g_nVad = 0;

//记录信息所需的锁、链表、内存池
LIST_ENTRY g_linkListHead;
NPAGED_LOOKASIDE_LIST g_nPageList;
KSPIN_LOCK g_lock;
KSEMAPHORE g_keySemaphore;


_declspec (naked) void __stdcall MyMemCpy_NoPaged(PUCHAR pDst, PUCHAR pSrc, ULONG count) {
	__asm {
		pushfd
		pushad
		cld
		mov edi, [esp + 0x4 + 0x24]
		mov esi, [esp + 0x8 + 0x24]
		mov ecx, [esp + 0xC + 0x24]
		mov eax, CR0
		mov ebx, eax
		and eax, 0xFFFEFFFF
		cli
		mov CR0, eax
		rep movsb
		mov CR0, ebx
		sti
		popad
		popfd
		retn 0xC
	}
}


VOID __stdcall Filter(ULONG ServiceId, ULONG TableBase, ULONG Argc, ULONG StackAddr) {
	ULONG pid = (ULONG)PsGetCurrentProcessId();
	if (pid == g_nPid) {
		ULONG i;
		PXBoxData pData=(PXBoxData)ExAllocateFromNPagedLookasideList(&g_nPageList);
		if(!pData)
			return;
		
		if (StackAddr < MmUserProbeAddress)
			pData->bFromUser = 1;
		else
			pData->bFromUser = 0;
		
		if (TableBase == (ULONG)KeServiceDescriptorTable.ServiceTableBase)
			pData->bFromSSDT = 1;
		else
			pData->bFromSSDT = 0;

		if (Argc > 16)
			Argc = 16;
		pData->argc = (UCHAR)Argc;
		for (i = 0; i < Argc; ++i)
			pData->args[i] = ((PULONG)StackAddr)[i];

		pData->pid = (ULONG)pid;
		pData->tid = (ULONG)PsGetCurrentThreadId();
		pData->sid = ServiceId;
		KeQuerySystemTime(&pData->time);
		ExInterlockedInsertTailList(&g_linkListHead, &pData->ListEntry, &g_lock);
		KeReleaseSemaphore( &g_keySemaphore, 0, 1, FALSE );
	}
}


_declspec (naked) void MyDetour() {
	__asm {
		sub esp,ecx
		shr ecx, 2

		pushfd
		pushad
		push esi
		push ecx
		push edi
		push eax
		call Filter;
		popad
		popfd

		jmp g_ulRetAddr;
	}
}

BOOLEAN HookSysenter() {
	ULONG i;

	
	__asm {
		pushfd
		pushad
		mov ecx, 0x176;
		rdmsr;
		mov g_ulHookPoint, eax; // KiFastCallEntry
		popad;
		popfd;
	}

	for (i = 0; i <= 0x100; ++ i) {
		if (MmIsAddressValid((PVOID)g_ulHookPoint) && *(PULONG)g_ulHookPoint == 0x871c8b3f) {
			break;
		}
		g_ulHookPoint++;
	}
	if (i > 0x100) {
		DbgPrint("g_ulHookPoint not found!\r\n");
		g_ulHookPoint = 0;
		return FALSE;
	}
	
	g_ulHookPoint += 4;
	g_ulRetAddr = g_ulHookPoint + 5;

	if (MmIsAddressValid((PVOID)g_ulHookPoint) && *(PULONG)g_ulHookPoint == 0xe9c1e12b) {
		// hook point found
		MyMemCpy_NoPaged(g_OrigBytes, (PUCHAR)g_ulHookPoint, 5);
		*(PULONG)(g_HookBytes + 1) = (ULONG)MyDetour - g_ulHookPoint - 5;
		// Hook
		MyMemCpy_NoPaged((PUCHAR)g_ulHookPoint, g_HookBytes, 5);

		return TRUE;
	} else if (MmIsAddressValid((PVOID)g_ulHookPoint) && *(PUCHAR)g_ulHookPoint == 0xe9) {
		// 360 found
		g_ulHookPoint = g_ulHookPoint + 5 + *(PULONG)(g_ulHookPoint + 1);

		MyMemCpy_NoPaged(g_OrigBytes, (PUCHAR)g_ulHookPoint, 5);
		*(PULONG)(g_HookBytes + 1) = (ULONG)MyDetour - g_ulHookPoint - 5;
		// Hook
		MyMemCpy_NoPaged((PUCHAR)g_ulHookPoint, g_HookBytes, 5);

		return TRUE;
	}

	return FALSE;
}


VOID UnHookSysenter() {
	MyMemCpy_NoPaged((PUCHAR)g_ulHookPoint, (PUCHAR)g_OrigBytes, sizeof(g_OrigBytes));
}

 
VOID OnUnload(IN PDRIVER_OBJECT DriverObject) {
	LARGE_INTEGER interval;

	UNICODE_STRING uniSymbLink;
	PDEVICE_OBJECT pDeviceObject;

	//
	// delete symbolic link
	//
	RtlInitUnicodeString(&uniSymbLink, L"\\DosDevices\\XBox");
	IoDeleteSymbolicLink(&uniSymbLink);	

	//
	// delete device
	//
	pDeviceObject = DriverObject->DeviceObject;
	IoDeleteDevice(pDeviceObject);

	UnHookSysenter();
	ExDeleteNPagedLookasideList(&g_nPageList);

	// reduce the probability of BSOD
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	interval.QuadPart = -500LL * 1000LL * 10LL;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);

	DbgPrint("end! :)\r\n");
}

BOOLEAN isMsrEnabled() {
	BOOLEAN bRet;
	__asm {
		pushad
		mov eax, 1
		cpuid
		shr edx, 5
		and edx, 1
		mov bRet, dl
		popad
	}
	return bRet;
}


// IRP_MJ_DEVICE_CONTROL 派遣例程
NTSTATUS MyDeviceControl (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp){

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
	ULONG inlen = irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outlen = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG UserDataSize = sizeof(XBoxData) - sizeof(LIST_ENTRY);
	
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

	if (code == IOCTL_XBOX_SETPID && inlen == sizeof(ULONG)) {
		PULONG pBuffer = Irp->AssociatedIrp.SystemBuffer;
		if (*pBuffer == 0xFFFFFFFF) {
			g_nVad = 0;
		}
		else {
			PEPROCESS pEProc;
			PsLookupProcessByProcessId((HANDLE)*pBuffer, &pEProc);
			ObDereferenceObject(pEProc);
			g_nVad = (ULONG)pEProc + 0x11C;
		}
		InterlockedExchange(&g_nPid, *pBuffer);
		Irp ->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
	}
	else if (code == IOCTL_XBOX_READ && outlen == UserDataSize) {
		LARGE_INTEGER liDelay;
		NTSTATUS status;

		PLIST_ENTRY pEntry;
		PXBoxData pData = NULL;
		PXBoxData pBuffer = Irp->AssociatedIrp.SystemBuffer;

		liDelay.QuadPart = -10 * 1000 * 1000;	// time-out value: 5s;
		do {
			status = KeWaitForSingleObject(&g_keySemaphore, Executive, KernelMode, FALSE, &liDelay);
			if (status != STATUS_SUCCESS) {
				if (!MmIsAddressValid((PVOID)g_nVad))
					break;
				else if (*(PULONG)g_nVad == 0)
					break;
				else {
					// just timeout
					Irp ->IoStatus.Information = 0;
					Irp->IoStatus.Status = STATUS_SUCCESS;
					break;
				}
			}
			pEntry = ExInterlockedRemoveHeadList(&g_linkListHead, &g_lock);
			pData = CONTAINING_RECORD(pEntry, XBoxData, ListEntry);
		} while(pData->pid != g_nPid);

		if (status == STATUS_SUCCESS) {
			RtlCopyMemory(pBuffer, pData, UserDataSize);
			ExFreeToNPagedLookasideList(&g_nPageList, pData);
			Irp ->IoStatus.Information = UserDataSize;
			Irp->IoStatus.Status = STATUS_SUCCESS;
		}
	}

	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}





// IRP_MJ_CREATE、IRP_MJ_CLOSE、IRP_MJ_CLEANUP 派遣例程
NTSTATUS MyCreateCloseCleanup (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp){
	InterlockedExchange(&g_nPid, 0xFFFFFFFF);
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest (Irp,IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}



// 普通 IRP 派遣例程
NTSTATUS GotoHell (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp) {
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest (Irp,IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}


// 驱动入口函数
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	NTSTATUS status;
	UNICODE_STRING uniDeviceName;
	UNICODE_STRING uniSymbLink;
	PDEVICE_OBJECT pDevObj;

	ULONG i;

	DbgPrint("start! :)\r\n");
	DriverObject->DriverUnload = OnUnload;

	if (!isMsrEnabled()) {
		DbgPrint("MSR not Supported!\r\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	if (!HookSysenter()) {
		DbgPrint("Hook KiFastCallEntry failed!\r\n");
		return STATUS_UNSUCCESSFUL;
	}

	//init
	KeInitializeSemaphore( &g_keySemaphore, 0 , MAXLONG );
	KeInitializeSpinLock(&g_lock );
	InitializeListHead(&g_linkListHead);
	ExInitializeNPagedLookasideList(&g_nPageList, NULL, NULL, 0, sizeof(XBoxData), 'XBox', 0);


	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = GotoHell;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MyCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceControl;

	//
	// 创建设备
	//
	RtlInitUnicodeString( &uniDeviceName, L"\\Device\\XBox" );

	status = IoCreateDevice(DriverObject, 0, &uniDeviceName, FILE_DEVICE_UNKNOWN, 
		FILE_DEVICE_SECURE_OPEN, FALSE, &pDevObj);

	if (!NT_SUCCESS( status )) {
		DbgPrint("create device obj failed\n");
		ExDeleteNPagedLookasideList(&g_nPageList);
		return status;
	}

	//
	// 创建符号链接
	//
	RtlInitUnicodeString( &uniSymbLink, L"\\DosDevices\\XBox" );
	status = IoCreateSymbolicLink(&uniSymbLink,&uniDeviceName);
	if(!NT_SUCCESS(status)) {
		DbgPrint("create symbolic link failed\n");
		IoDeleteDevice(pDevObj);
		ExDeleteNPagedLookasideList(&g_nPageList);
		return status;
	}


	pDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}    