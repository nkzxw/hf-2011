/*

  TEST_KIDISPAT.c
  
  Author: Adly
  Last Updated: 2008-11-16
	
*/

#include <ntddk.h>

#include "TEST_KIDISPAT.h"

//
// A structure representing the instance information associated with
// a particular device
//

typedef struct _DEVICE_EXTENSION
{
	ULONG  StateVariable;
	
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Function Declare
//
extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
);

NTSTATUS
Test_kidispatDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

NTSTATUS
Test_kidispatDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

NTSTATUS
Test_kidispatDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

VOID
Test_kidispatUnload(
	IN PDRIVER_OBJECT		DriverObject
);


#include "commhdr.h"


//////////////////////////////////////////////////////////////////////////
LIST_ENTRY	g_HookInfoListHead;
ULONG		   g_TargetEP;
#define		SemaphoreCount 0x1000
KSEMAPHORE		*g_Ksmp;

//////////////////////////////////////////////////////////////////////////
#define DECL_DYNCFUN(x) \
{#x,(DWORD*)&x,0,0}

#define DECL_DYNCFUN_HOOK(x) \
{#x,(DWORD*)&x,0,1}

#define DECL_DYNCFUN_HOOK_Old2New(x) \
{#x,0,0,1}
//////////////////////////////////////////////////////////////////////////

//这是hook新的os的hook,涉及到DebugPort判断的，转到自己实现中
HX_DYNC_FUNCTION dync_New2myIMPL[]={
			//	DECL_DYNCFUN_HOOK(NtDebugActiveProcess),
				DECL_DYNCFUN_HOOK(DbgkpSetProcessDebugObject),
				DECL_DYNCFUN_HOOK(NtCreateDebugObject),
				DECL_DYNCFUN_HOOK(NtWaitForDebugEvent),
				DECL_DYNCFUN_HOOK(NtDebugContinue),
				DECL_DYNCFUN_HOOK(DbgkCreateThread),
				DECL_DYNCFUN_HOOK(DbgkExitThread),
				DECL_DYNCFUN_HOOK(DbgkExitProcess),
				DECL_DYNCFUN_HOOK(DbgkMapViewOfSection),
				DECL_DYNCFUN_HOOK(DbgkForwardException),
};

//////////////////////////////////////////////////////////////////////////
//这是为了HOOK原来的ntos，转到新的os中
VOID (WINAPI *DUMMYFUCK )(IN  PVOID   Object);
// VOID (WINAPI *PspUserThreadStartup )(IN  PVOID   Object);
// VOID (WINAPI *PspSystemThreadStartup )(IN  PVOID   Object);


HX_DYNC_FUNCTION dync_Old2New[]={
			DECL_DYNCFUN_HOOK_Old2New(PspUserThreadStartup),
				DECL_DYNCFUN_HOOK_Old2New(PspSystemThreadStartup),
				DECL_DYNCFUN_HOOK_Old2New(ObCloseHandle),
				DECL_DYNCFUN_HOOK_Old2New(PspProcessDelete),
				DECL_DYNCFUN_HOOK_Old2New(pIofCallDriver),
				DECL_DYNCFUN_HOOK_Old2New(KiTrap03),
				DECL_DYNCFUN_HOOK_Old2New(ObpCreateHandle),	//为了跳过ObCheckObjectAccess
/*
				kd> dps nt!pIofCallDriver l8
				8054c400  804eedc8 nt!IopfCallDriver		//就搞这个。归类到fengyue驱动去	
				8054c404  804f12c0 nt!IopfCompleteRequest
				8054c408  804f0a00 nt!IopAllocateIrpPrivate
				8054c40c  804ef0e6 nt!IopFreeIrp
				8054c410  00000000
8054c414  00000000
*/
	//	DECL_DYNCFUN_HOOK_Old2New(KeStackAttachProcess),
	//	DECL_DYNCFUN_HOOK_Old2New(KeAttachProcess),
		
		};


//这个需要另外HOOK的函数 ，old os 中的，把这些不经过hookport的转到自己的实现中
HX_DYNC_FUNCTION dync_funs_hook[]={
				DECL_DYNCFUN_HOOK(DbgkCreateThread),
				DECL_DYNCFUN_HOOK(DbgkExitThread),
				DECL_DYNCFUN_HOOK(DbgkExitProcess),
				DECL_DYNCFUN_HOOK(DbgkMapViewOfSection),
				DECL_DYNCFUN_HOOK(DbgkpMarkProcessPeb),

	//	DECL_DYNCFUN_HOOK(NtCreateDebugObject),
		DECL_DYNCFUN_HOOK(DbgkForwardException),

//		DECL_DYNCFUN_HOOK(KiDispatchException),		
		//使用强大的特征码直接patch KiDispatchException中比较debugport部分
		//s -b 804d8000 806ce100 64 A1 24 01 00 00 8B 40  44 39 B8 BC 00 00 00
		};
//这是实现自己分发函数需要用到的
HX_DYNC_FUNCTION dync_funs[]={
		DECL_DYNCFUN(KeUserExceptionDispatcher),
		DECL_DYNCFUN(KeI386XMMIPresent),
		DECL_DYNCFUN(PsImageNotifyEnabled),
		DECL_DYNCFUN(PsNtDllPathName),
		DECL_DYNCFUN(KeFeatureBits),
		DECL_DYNCFUN(PsSystemDllBase),
		DECL_DYNCFUN(PsGetNextProcess),
		DECL_DYNCFUN(PsGetNextProcessThread),
		DECL_DYNCFUN(PsQuitNextProcessThread),
		DECL_DYNCFUN(PsResumeThread),
		DECL_DYNCFUN(PsTerminateProcess),
		DECL_DYNCFUN(PsSuspendThread),	
		DECL_DYNCFUN(PsCallImageNotifyRoutines),
		DECL_DYNCFUN(ObDuplicateObject),
		DECL_DYNCFUN(MmGetFileNameForAddress),
		DECL_DYNCFUN(MmGetFileNameForSection),
		DECL_DYNCFUN(KeFreezeAllThreads),
		DECL_DYNCFUN(KeThawAllThreads),	
		DECL_DYNCFUN(KiReadyThread),	
		DECL_DYNCFUN(KiSwapProcess),
		DECL_DYNCFUN(KiSwapThread),	
		DECL_DYNCFUN(KiSetSwapEvent),
		DECL_DYNCFUN(LpcRequestWaitReplyPortEx),
		DECL_DYNCFUN(KiUnlockDispatcherDatabase),
//		DECL_DYNCFUN(HalRequestSoftwareInterrupt),	//hal DLL
		DECL_DYNCFUN(MiMakeProtectionMask),
		DECL_DYNCFUN(MiProtectVirtualMemory),
		DECL_DYNCFUN(MmCopyVirtualMemory),
		DECL_DYNCFUN(PspCreateThread),
		DECL_DYNCFUN(IopDeallocateApc),

		DECL_DYNCFUN(KeContextFromKframes),
		DECL_DYNCFUN(KeContextToKframes),
		DECL_DYNCFUN(KiCheckForAtlThunk),
		DECL_DYNCFUN(RtlDispatchException),
		DECL_DYNCFUN(KdIsThisAKdTrap),
		DECL_DYNCFUN(KiSegSsToTrapFrame),
		DECL_DYNCFUN(KiEspToTrapFrame),
		DECL_DYNCFUN(KiCopyInformation),

// 		DECL_DYNCFUN(DbgkpProcessDebugPortMutex),
 		DECL_DYNCFUN(DbgkDebugObjectType),
		

};


BOOLEAN	HookByInline(VOID *src, VOID *dst, char *pFunName="");
BOOLEAN	HookByInline(ULONG src, ULONG, char *pFunName="");

BOOLEAN	HookByInline(ULONG target, ULONG myfake, char *pFunName)
{
		kprintf("Inline Hooking %s from %X to %X\r\n", pFunName, target, myfake);
		if (!MmIsAddressValid(PVOID(target)))
		{
				kprintf("Target is not available\r\n");
				return 1;
		}
		LONG	mysrc,mydst;
		mysrc	=	target;
		mydst	=	myfake;
		HOOKINFO	*pHI	=	(PHOOKINFO)kmalloc(sizeof(HOOKINFO));
		if (pHI==NULL)
		{
			return FALSE;
		}
		#define		JMPLEN	5
		RtlZeroMemory(pHI, sizeof(HOOKINFO));
		ULONG	itmp=0;
		UCHAR	JmpCode[JMPLEN]={0xe9,0,0,0,0};
		itmp	=	mydst-mysrc-JMPLEN;
		*(PULONG)&JmpCode[1]=	itmp;
		RtlCopyMemory(pHI->szOldCode, (PUCHAR)mysrc, JMPLEN);
		memcpy((PUCHAR)mysrc, JmpCode, JMPLEN);
		pHI->NewAddress		=	mydst;
		pHI->OldCodeSize		=	JMPLEN;
		pHI->OriAddress			=	mysrc;
		RtlCopyMemory(pHI->szFunName, pFunName, strlen(pFunName));

		InsertTailList(&g_HookInfoListHead,&pHI->Next );
	return 1;
}

BOOLEAN	HookByInline(VOID *target, VOID *myfake, char *pFunName)
{
	return HookByInline((ULONG)target, (ULONG)myfake, pFunName);
}




//////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
)
{
	NTSTATUS			Status = STATUS_SUCCESS;    
	UNICODE_STRING		ntDeviceName;
	UNICODE_STRING		dosDeviceName;
	PDEVICE_EXTENSION	deviceExtension;
	PDEVICE_OBJECT		deviceObject = NULL;
	
	
	kprintf(" DriverEntry: %wZ\n", RegistryPath);
	
	
	GetProcessNameOffset();
	RtlInitUnicodeString(&ntDeviceName, TEST_KIDISPAT_DEVICE_NAME_W);
		//initialize 
	 InitializeListHead(&g_HookInfoListHead); 

	g_Ksmp=	(KSEMAPHORE*)kmalloc(sizeof(KSEMAPHORE));
	// save old system call locations
	KeInitializeSemaphore(g_Ksmp, SemaphoreCount, SemaphoreCount);

	Status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),		// DeviceExtensionSize
		&ntDeviceName,					// DeviceName
		FILE_DEVICE_TEST_KIDISPAT,	// DeviceType
		0,								// DeviceCharacteristics
		TRUE,							// Exclusive
		&deviceObject					// [OUT]
		);
	
	if(!NT_SUCCESS(Status))
	{
		kprintf(" IoCreateDevice Error Code = 0x%X\n", Status);
		
		return Status;
	}
	
	deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
	
	//
	// Set up synchronization objects, state info,, etc.
	//
	
	//
	// Create a symbolic link that Win32 apps can specify to gain access
	// to this driver/device
	//
	
	RtlInitUnicodeString(&dosDeviceName, TEST_KIDISPAT_DOS_DEVICE_NAME_W);
	
	Status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);
	
	if(!NT_SUCCESS(Status))
	{
		kprintf(" IoCreateSymbolicLink Error Code = 0x%X\n", Status);
		
		//
		// Delete Object
		//

		IoDeleteDevice(deviceObject);
		
		return Status;
	}
	
	//
	// Create dispatch points for device control, create, close.
	//
	
	DriverObject->MajorFunction[IRP_MJ_CREATE]			= Test_kidispatDispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= Test_kidispatDispatchClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= Test_kidispatDispatchDeviceControl;
	DriverObject->DriverUnload							= Test_kidispatUnload;
	

	return Status;
}

NTSTATUS
Test_kidispatDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	Irp->IoStatus.Information = 0;
	

	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return Status;
}

NTSTATUS
Test_kidispatDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	Irp->IoStatus.Information = 0;
	
	kprintf((" IRP_MJ_CLOSE\n"));
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return Status;
}

ULONG g_uCr0 = 0;
void WPOFF()
{
    
    ULONG uAttr;
    
    _asm
    {
        push eax;
        mov eax, cr0;
        mov uAttr, eax;
        and eax, 0FFFEFFFFh; // CR0 16 BIT = 0
        mov cr0, eax;
        pop eax;
        cli
    };
    
    g_uCr0 = uAttr; //保存原有的 CRO 屬性
    
}

VOID WPON()
{
    
    _asm
    {
        sti
            push eax;
        mov eax, g_uCr0; //恢復原有 CR0 屬性
        mov cr0, eax;
        pop eax;
    };
    
}
ULONG	g_jmpback=0;
VOID	__declspec(naked) jmpstub()
{
	_asm
	{
		mov     edi,edi
		push    ebp
		mov     ebp,esp
		jmp g_jmpback
	}
};
static	ULONG	sNewOsAddress=0;
static	PULONG	sNT_IopfCallDriverAddress=0;
#define DECL_DYNCFUN_REPLACE(x,y) \
{#x,0,0,y}
REPLACEAPIINFO	g_ReplaceApiInfo[]=
{
	DECL_DYNCFUN_REPLACE(IopfCallDriver,0),
	DECL_DYNCFUN_REPLACE(IopfCompleteRequest,0),
	DECL_DYNCFUN_REPLACE(IopAllocateIrpPrivate,0),
	DECL_DYNCFUN_REPLACE(IopFreeIrp,0),
// 	DECL_DYNCFUN_REPLACE(fengyue_KeUnstackDetachProcess,1),
// 	DECL_DYNCFUN_REPLACE(fengyue_KeStackAttachProcess,1),
// 	DECL_DYNCFUN_REPLACE(fengyue_RtlFreeUnicodeString,1),
// 	DECL_DYNCFUN_REPLACE(fengyue_ZwClose,1),
// 	DECL_DYNCFUN_REPLACE(fengyue_ZwUnmapViewOfSection,1),
// 	DECL_DYNCFUN_REPLACE(fengyue_ZwMapViewOfSection,1),

};
NTSTATUS
Test_kidispatDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuf;
	ULONG				inBufLength, outBufLength;
	ULONG				ioControlCode;
	ULONG				itmp=0;
	ULONG				itmp2=0;
		ULONG				itmp3=0;
			ULONG				itmp4=0;
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	UCHAR	g_JmpCode[5]={0xe9,0,0,0,0};
	Irp->IoStatus.Information = 0;
	KIRQL Irql;

	//
	// Get the pointer to the input/output buffer and it's length
	//
	PHX_DYNC_FUNCTION	fn_ls=NULL;
	DWORD	number;
	ioBuf = Irp->AssociatedIrp.SystemBuffer;
	inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	// Irp->UserBuffer;		// If METHOD_NEITHER, This is Output Buffer
	
	switch (ioControlCode)
	{
	case IOCTL_TEST_KIDISPAT_NEW2MY:
		{
			Irql=KeRaiseIrqlToDpcLevel();
			kprintf("Enter IOCTL_TEST_KIDISPAT_NEW2MY\r\n");
			DWORD i=0;
				number=sizeof(dync_New2myIMPL)/sizeof(HX_DYNC_FUNCTION);
				fn_ls=(PHX_DYNC_FUNCTION)ioBuf;
			WPOFF();
				for ( i=0;i<number;i++)
				{
					if (dync_New2myIMPL[i].bNeedHook)
					{
						if (fn_ls[i].delta==0)
						{
							break;
						}
						HookByInline(fn_ls[i].FunAddr+fn_ls[i].delta,  (ULONG)dync_New2myIMPL[i].FunVar, dync_New2myIMPL[i].FunName);

					}
			}
			WPON();
			KeLowerIrql(Irql);
			break;
		}
	case IOCTL_TEST_KIDISPAT_HOOK_REPLACE:
		{
			itmp	=	*(PULONG)ioBuf;		//第一个4字节是偏移delta
			kprintf("NT_IopfCallDriverAddress at %X\r\n",sNT_IopfCallDriverAddress);

			WPOFF();
			for (int idx=0; idx<sizeof(g_ReplaceApiInfo)/sizeof(g_ReplaceApiInfo[0]); idx++)
			{
				//替换NT里面某些地方会使用全局函数指针调用函数的情况
				/*
				kd> dps nt!pIofCallDriver l8
				8054c400  804eedc8 nt!IopfCallDriver	
				8054c404  804f12c0 nt!IopfCompleteRequest
				8054c408  804f0a00 nt!IopAllocateIrpPrivate
				8054c40c  804ef0e6 nt!IopFreeIrp
				*/
				if (g_ReplaceApiInfo[idx].type==0)
				{
					g_ReplaceApiInfo[idx].uOriLocation	=	sNT_IopfCallDriverAddress+idx;
					kprintf("Replacing %s at %X\r\n", g_ReplaceApiInfo[idx].FunName, g_ReplaceApiInfo[idx].uOriLocation);
					g_ReplaceApiInfo[idx].uOriValue	=	*g_ReplaceApiInfo[idx].uOriLocation;	//保存原来的
					*g_ReplaceApiInfo[idx].uOriLocation	=	g_ReplaceApiInfo[idx].uOriValue+itmp;
				}

// 				if (g_ReplaceApiInfo[idx].type==1)
// 				{
// 					if (itmp2==0)
// 					{
// 						continue;
// 					}
// 
// 					g_ReplaceApiInfo[idx].uOriLocation	=	(PULONG)itmp2+itmp3;
// 					kprintf("Replacing %s at %X\r\n", g_ReplaceApiInfo[idx].FunName, g_ReplaceApiInfo[idx].uOriLocation);
// 					g_ReplaceApiInfo[idx].uOriValue	=	*g_ReplaceApiInfo[idx].uOriLocation;	//保存原来的
// 					*g_ReplaceApiInfo[idx].uOriLocation	=	g_ReplaceApiInfo[idx].uOriValue+itmp;
// 					itmp3++;
// 				}
			}

			WPON();

			break;
		}

	case IOCTL_TEST_KIDISPAT_HOOK_OLD:
		{
		kprintf("IOCTL_TEST_KIDISPAT_HOOK_OLD \r\n");
		//由于一些函数是不经过kifastcallEntry调用的。所以需要直接inline hook老函数。
		//然后转到新内核中去
		number=sizeof(dync_Old2New)/sizeof(HX_DYNC_FUNCTION);
		fn_ls=(PHX_DYNC_FUNCTION)ioBuf;
		DWORD i=0;
		WPOFF();
		Irql=KeRaiseIrqlToDpcLevel();
			for ( i=0;i<number;i++)
			{
				if (fn_ls[i].delta==0)
				{
						break;;	//如果是0，说明没新内核，如果也HOOK的话，就会造成jmp回自己，这样就死循环了
				}
				if (dync_Old2New[i].bNeedHook)
				{
					
					if (_stricmp(dync_Old2New[i].FunName, "pIofCallDriver")==0)
					{
						sNT_IopfCallDriverAddress	=	(PULONG)fn_ls[i].FunAddr;

						continue;
						
					}
					HookByInline(fn_ls[i].FunAddr,  (ULONG)fn_ls[i].FunAddr+fn_ls[i].delta, dync_Old2New[i].FunName);
				//	HookByInline(fn_ls[i].FunAddr,  (ULONG)dync_Old2New[i].FunVar, dync_Old2New[i].FunName);
				}
			}
			KeLowerIrql(Irql);
		WPON();
			break;
		}

	case IOCTL_TEST_KIDISPAT_PROTECT_INFO:
		{
			PPROTECT_INFO	pProInfo	=	(PPROTECT_INFO)ioBuf;
			if (!IdentiyTarget(pProInfo))
			{
				Status	=	STATUS_UNSUCCESSFUL;
			}
			//
			// Sample IO Control
			//

			break;
		}
		case IOCTL_TEST_KIDISPAT_NewOsAddress:
		{
			//
			// Sample IO Control
			//
			if (inBufLength!=sizeof(ULONG))
			{
				kprintf("invailid parmeter when IOCTL_TEST_KIDISPAT_NewOsAddress \r\n");
				Status	=	STATUS_INVALID_PARAMETER;
				break;
			}
			sNewOsAddress	=	*(PULONG)ioBuf;
			kprintf(" IOCTL_TEST_KIDISPAT_NewOsAddress: 0x%X\r\n", sNewOsAddress);
			
			break;
		}

		case IOCTL_TEST_KIDISPAT_UNHOOK:
		{
		//	PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine,TRUE);
			Irql=KeRaiseIrqlToDpcLevel();
			WithdrawWhenUnload();
			for (int idx=0; idx<sizeof(g_ReplaceApiInfo)/sizeof(g_ReplaceApiInfo[0]); idx++)
			{
				if (g_ReplaceApiInfo[idx].type==0)
				{
					kprintf("Recovering Replaced %s at %X\r\n", g_ReplaceApiInfo[idx].FunName, g_ReplaceApiInfo[idx].uOriLocation);
					*g_ReplaceApiInfo[idx].uOriLocation	=	g_ReplaceApiInfo[idx].uOriValue;
				}
			}
			KeLowerIrql(Irql);
			kprintf(" IOCTL_TEST_KIDISPAT_UNHOOK: 0x%X\r\n", ioControlCode);
			break;
		}
		//用户层传下来要Inline HOOK的函数
		//这些函数转到自己的实现中去
		case IOCTL_TEST_KIDISPAT_InlineHook:
		{
	
		//
//		PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine,false);
		DbgkInitialize();	//自己创建一个DebugObjectType	//所以上面获取的就不需要了
			Irql=KeRaiseIrqlToDpcLevel();
		number=sizeof(dync_funs_hook)/sizeof(HX_DYNC_FUNCTION);
		fn_ls=(PHX_DYNC_FUNCTION)ioBuf;
		DWORD i=0;
			for ( i=0;i<number;i++)
			{
				if (dync_funs_hook[i].bNeedHook)
				{
						//inline HOOK dync_funs_hook数组中需要HOOK的
						HookByInline(fn_ls[i].FunAddr,  (ULONG)dync_funs_hook[i].FunVar, dync_funs_hook[i].FunName);
				}
			}
			PatchDebugPortCheckInKiDispatch((PUCHAR)sNewOsAddress);
			if (sNewOsAddress)
			{
				PatchDebugPortCheckInKiDispatch();
			}
			KeLowerIrql(Irql);
			break;
		}

	case IOCTL_TEST_KIDISPAT_Set: //把用户层传递进来的未导出函数的地址设置到驱动来
		{


		number=sizeof(dync_funs)/sizeof(HX_DYNC_FUNCTION);
		fn_ls=(PHX_DYNC_FUNCTION)ioBuf;
		WPOFF();
			for (DWORD i=0;i<number;i++)
			{
				
				*(dync_funs[i].FunVar)=fn_ls[i].FunAddr;
			}

		WPON();
/*
		806379e8 nt!NtCreateDebugObject = <no type information>
80638b94 nt!NtRemoveProcessDebug = <no type information>
8063855e nt!NtSetInformationDebugObject = <no type information>


NtQueryDebugFilterState	//启动的时候好像会有，可以不用管，好像是dbgprint触发的
80638c14 nt!NtDebugContinue = <no type information>
806382c6 nt!NtWaitForDebugEvent = <no type information>
80638ac4 nt!NtDebugActiveProcess = <no type information>

nt!DbgkExitProcess??
*/


		break;
		}
	default:
		{
			Status = STATUS_INVALID_PARAMETER;
			
			kprintf(" Unknown IOCTL: 0x%X (%04X,%04X)",
				ioControlCode, DEVICE_TYPE_FROM_CTL_CODE(ioControlCode),
				IoGetFunctionCodeFromCtlCode(ioControlCode));
			
			break;
		}
	}
	
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	
	return Status;
}

VOID
Test_kidispatUnload(
	IN PDRIVER_OBJECT		DriverObject
)
{
	UNICODE_STRING dosDeviceName;

		//也许不要卸载，发现DbgkpQueueMessage 等API里面会waitfor,
		//要想Unload驱动，就要处理好HOOK

		LARGE_INTEGER         time;
		 time.QuadPart = -1000*10000;//1 seconds.
		 long	kSmpStateCount=0;
		while(1)
		{
				kSmpStateCount	=	KeReadStateSemaphore(g_Ksmp);
			if (kSmpStateCount==SemaphoreCount)
			{
			//	DbgPrint("Delay All release");
				break;
			}
		kprintf("waiting for pendding request in my hook code kSmpStateCount=%d. Total=%d\r\n", kSmpStateCount, SemaphoreCount);
		KeDelayExecutionThread(KernelMode, FALSE, &time);
		}

		kfree(DbgkpProcessDebugPortMutex);
	//
	// Free any resources
	//
	
	//
	// Delete the symbolic link
	//
	
	RtlInitUnicodeString(&dosDeviceName, TEST_KIDISPAT_DOS_DEVICE_NAME_W);
	
	IoDeleteSymbolicLink(&dosDeviceName);
	
	//
	// Delete the device object
	//
	
	IoDeleteDevice(DriverObject->DeviceObject);
	kprintf("Leave Unloaded\r\n");
}