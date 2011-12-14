#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"



#define	IOCTL_PEDIY		(ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN,0x00000100,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define	IOCTL_GETSTR	(ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN,0x00000101,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define ProcessDebugPort 7
#define SystemModuleInformation     11


typedef enum _OBJECT_INFORMATION_CLASS
{
	ObjectBasicInformation, 
	ObjectNameInformation,
	ObjectTypeInformation, 
	ObjectAllTypesInformation,
	ObjectDataInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING TypeName;
	ULONG TotalNumberOfHandles;
	ULONG TotalNumberOfObjects;
	WCHAR Unused1[8];
	ULONG HighWaterNumberOfHandles;
	ULONG HighWaterNumberOfObjects;
	WCHAR Unused2[8];
	ACCESS_MASK InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ACCESS_MASK ValidAttributes;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	USHORT MaintainTypeList;
	POOL_TYPE PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_ALL_INFORMATION {
	ULONG NumberOfObjectsTypes;
	OBJECT_TYPE_INFORMATION ObjectTypeInformation[1];
} OBJECT_ALL_INFORMATION, *POBJECT_ALL_INFORMATION;

typedef struct _OBJECT_ALL_TYPES_INFORMATION {
	ULONG NumberOfTypes;
	OBJECT_TYPE_INFORMATION TypeInformation[1];
} OBJECT_ALL_TYPES_INFORMATION, *POBJECT_ALL_TYPES_INFORMATION;


typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;                // Not filled in
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[ 256 ];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[ 1 ];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;


typedef unsigned long       DWORD;
typedef unsigned short WORD;
typedef wchar_t WCHAR;
typedef unsigned char BYTE;
typedef unsigned int    UINT;
typedef char            CHAR;
typedef char *          PCHAR;
typedef PVOID           POBJECT;
typedef long BOOL;


BYTE gstrinfo[0x200];
void SendMsg(const char* str);

typedef struct _INBUF
{
	DWORD dwDebugerPID;
	DWORD dwPID;
//	DWORD dwNum_NtDelayExecution;
	DWORD dwNum_NtGetContextThread;
	DWORD dwNum_NtSetContextThread;
//	DWORD dwNum_NtContinue;
}INBUF;

INBUF ginbsave;


typedef struct _SRVTABLE {
	PVOID *ServiceTable;
	ULONG LowCall;        
	ULONG HiCall;
	PVOID *ArgTable;
} SRVTABLE, *PSRVTABLE;

extern PSRVTABLE KeServiceDescriptorTable;

NTSYSAPI
NTSTATUS 
NTAPI ZwQuerySystemInformation(
							   IN       ULONG SystemInformationClass,
							   IN OUT    PVOID SystemInformation,
							   IN        ULONG SystemInformationLength,
							   OUT   PULONG ReturnLength
							   );


NTSYSAPI
NTSTATUS 
NTAPI ZwQueryInformationProcess(
								__in       HANDLE ProcessHandle,
								__in       PROCESSINFOCLASS ProcessInformationClass,
								__out      PVOID ProcessInformation,
								__in       ULONG ProcessInformationLength,
								__out_opt  PULONG ReturnLength
							); 



NTSTATUS (__stdcall *RealZwQueryInformationProcess)(
										  __in       HANDLE ProcessHandle,
										  __in       PROCESSINFOCLASS ProcessInformationClass,
										  __out      PVOID ProcessInformation,
										  __in       ULONG ProcessInformationLength,
										  __out_opt  PULONG ReturnLength
										  );

NTSYSAPI
NTSTATUS 
NTAPI ZwQueryObject(
					   __in_opt   HANDLE Handle,
					   __in       OBJECT_INFORMATION_CLASS ObjectInformationClass,
					   __out_opt  PVOID ObjectInformation,
					   __in       ULONG ObjectInformationLength,
					   __out_opt  PULONG ReturnLength
					   );

NTSTATUS (__stdcall* RealZwQueryObject)(
					__in_opt   HANDLE Handle,
					__in       OBJECT_INFORMATION_CLASS ObjectInformationClass,
					__out_opt  PVOID ObjectInformation,
					__in       ULONG ObjectInformationLength,
					__out_opt  PULONG ReturnLength
					);

NTSYSAPI
NTSTATUS 
NTAPI ZwSetInformationThread(
								__in  HANDLE ThreadHandle,
								__in  THREADINFOCLASS ThreadInformationClass,
								__in  PVOID ThreadInformation,
								__in  ULONG ThreadInformationLength
								);


NTSTATUS (__stdcall* RealZwSetInformationThread)(
							 __in  HANDLE ThreadHandle,
							 __in  THREADINFOCLASS ThreadInformationClass,
							 __in  PVOID ThreadInformation,
							 __in  ULONG ThreadInformationLength
							 );

NTSYSAPI
NTSTATUS 
NTAPI ZwWaitForSingleObject(
							__in  HANDLE Handle,
							__in  BOOLEAN Alertable,
							__in  PLARGE_INTEGER Timeout
							);

NTSTATUS (__stdcall* RealZwWaitForSingleObject)(
							__in  HANDLE Handle,
							__in  BOOLEAN Alertable,
							__in  PLARGE_INTEGER Timeout
							);

NTSYSAPI
NTSTATUS 
NTAPI ZwOpenProcess(
					   __out     PHANDLE ProcessHandle,
					   __in      ACCESS_MASK DesiredAccess,
					   __in      POBJECT_ATTRIBUTES ObjectAttributes,
					   __in_opt  PCLIENT_ID ClientId
					   );


NTSTATUS (__stdcall* RealZwOpenProcess)(
					__out     PHANDLE ProcessHandle,
					__in      ACCESS_MASK DesiredAccess,
					__in      POBJECT_ATTRIBUTES ObjectAttributes,
					__in_opt  PCLIENT_ID ClientId
					);



NTSYSAPI
NTSTATUS
NTAPI ZwGetContextThread(
				   IN HANDLE ThreadHandle,
				   OUT PCONTEXT Context
);


NTSTATUS (__stdcall* RealZwGetContextThread)(
						 IN HANDLE ThreadHandle,
						 OUT PCONTEXT Context
						 );


NTSYSAPI
NTSTATUS
NTAPI ZwSetContextThread(
						 IN HANDLE ThreadHandle,
						 OUT PCONTEXT Context
						 );


NTSTATUS (__stdcall* RealZwSetContextThread)(
											 IN HANDLE ThreadHandle,
											 OUT PCONTEXT Context
						 );

NTSYSAPI 
NTSTATUS
NTAPI ZwDelayExecution(
							 IN BOOLEAN              Alertable,
							 IN PLARGE_INTEGER       DelayInterval );

NTSTATUS (__stdcall* RealZwDelayExecution)(
					   IN BOOLEAN              Alertable,
					   IN PLARGE_INTEGER       DelayInterval );


NTSYSAPI 
NTSTATUS
NTAPI ZwContinue( 
				   IN PCONTEXT             ThreadContext,
				   IN BOOLEAN              RaiseAlert );

 
NTSTATUS (__stdcall* RealZwContinue)(		   
				   IN PCONTEXT             ThreadContext,
				   IN BOOLEAN              RaiseAlert );



#if defined(_ALPHA_)
#define SYSCALL(_function)  ServiceTable->ServiceTable[ (*(PULONG)_function)  & 0x0000FFFF ]
#else
#define SYSCALL(_function)  ServiceTable->ServiceTable[ *(PULONG)((PUCHAR)_function+1)]
#endif




PSRVTABLE  ServiceTable = NULL;

DWORD gdwDebugerPID = 0;
DWORD gdwPID = 0;

DWORD gdwDR0 = 0;
DWORD gdwDR1 = 0;
DWORD gdwDR2 = 0;
DWORD gdwDR3 = 0;
DWORD gdwDR6 = 0;
DWORD gdwDR7 = 0;

//KiUserExceptionDispatcher 和 ZwContinue 必须配对使用
int gKiUserExceptionDispatcher = 0;

BOOL gbHooked = FALSE;
BOOL gbHooked2 = FALSE;


void __stdcall DisableMemoryProction(void)
{
	__asm
	{
		//去掉内存保护
		cli
		mov eax,cr0
		and eax,not 10000h
		mov cr0,eax
	}
}

void __stdcall EnableMemoryProction(void)
{
	__asm
	{
		//恢复内存保护
		mov eax,cr0
		or eax,10000h
		mov cr0,eax
		sti
	}
}

NTSTATUS __stdcall HookZwQueryInformationProcess(
													__in       HANDLE ProcessHandle,
													__in       PROCESSINFOCLASS ProcessInformationClass,
													__out      PVOID ProcessInformation,
													__in       ULONG ProcessInformationLength,
													__out_opt  PULONG ReturnLength
													)
{
	NTSTATUS status = RealZwQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

	if(gdwPID == (DWORD)PsGetCurrentProcessId() && ProcessInformationClass == ProcessDebugPort && ProcessInformation != NULL)
	{
		DbgPrint("HookZwQueryInformationProcess\n");
		*(DWORD*)ProcessInformation = 0;

		SendMsg("ZwQueryInformationProcess(ProcessDebugPort)");
	}

	return status;
}

NTSTATUS __stdcall HookZwQueryObject(
										__in_opt   HANDLE Handle,
										__in       OBJECT_INFORMATION_CLASS ObjectInformationClass,
										__out_opt  PVOID ObjectInformation,
										__in       ULONG ObjectInformationLength,
										__out_opt  PULONG ReturnLength
										)
{
	NTSTATUS status = RealZwQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);

	if(gdwPID == (DWORD)PsGetCurrentProcessId() && 
		NT_SUCCESS(status) && 
		ObjectInformationClass == ObjectAllTypesInformation &&
		ObjectInformation != NULL)
	{
		OBJECT_ALL_TYPES_INFORMATION *Types = (OBJECT_ALL_TYPES_INFORMATION *)ObjectInformation;
		OBJECT_TYPE_INFORMATION *t;
		UINT i = 0;

		for (t = Types->TypeInformation,i=0; i < Types->NumberOfTypes; i++)
		{
			if(t->TypeName.Buffer != NULL)
			{
				if ( !_wcsicmp(t->TypeName.Buffer, L"DebugObject"))
				{
					//if(t->TotalNumberOfHandles > 0 || t->TotalNumberOfObjects > 0)
					//{
					//}

					DbgPrint("HookZwQueryObject\n");

					SendMsg("ZwQueryObject(DebugObject)");

					//改掉
					wcscpy(t->TypeName.Buffer, L"d1234x");

					break;
				}
			}

			if(t->TypeName.Buffer == NULL)
				break;

			t=(OBJECT_TYPE_INFORMATION *)((char*)t->TypeName.Buffer+((t->TypeName.MaximumLength+3)&~3));
		}
	}

	return status;
}

NTSTATUS __stdcall HookZwSetInformationThread(
												 __in  HANDLE ThreadHandle,
												 __in  THREADINFOCLASS ThreadInformationClass,
												 __in  PVOID ThreadInformation,
												 __in  ULONG ThreadInformationLength
												 )
{
	if(ThreadInformationClass == ThreadHideFromDebugger && gdwPID == (DWORD)PsGetCurrentProcessId())
	{
		DbgPrint("HookZwSetInformationThread\n");

		SendMsg("ZwSetInformationThread(ThreadHideFromDebugger)");

		return STATUS_SUCCESS;
	}
	return RealZwSetInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

/*
NTSTATUS __stdcall HookZwWaitForSingleObject(
												__in  HANDLE Handle,
												__in  BOOLEAN Alertable,
												__in  PLARGE_INTEGER Timeout
												)
{
	if(gdwPID == (DWORD)PsGetCurrentProcessId() && Timeout != NULL)
	{
		if(Timeout->QuadPart != 0)
		{
			LARGE_INTEGER tm;
	//		int ms = (int)(Timeout->QuadPart / (-10000));
			

			//设置为 1ms
			tm.QuadPart = -(1*10000);		//改为1ms
			return RealZwWaitForSingleObject(Handle, Alertable, &tm);
		}
	}
	return RealZwWaitForSingleObject(Handle, Alertable, Timeout);
}
*/

/*
NTSTATUS __stdcall HookZwOpenProcess(
										__out     PHANDLE ProcessHandle,
										__in      ACCESS_MASK DesiredAccess,
										__in      POBJECT_ATTRIBUTES ObjectAttributes,
										__in_opt  PCLIENT_ID ClientId
										)
{
	if(gdwPID == (DWORD)PsGetCurrentProcessId() && ClientId != NULL)
	{
		if((DWORD)(ClientId->UniqueProcess) == gdwDebugerPID)
		{
			//被调试进程打开调试器进程
			return STATUS_ACCESS_DENIED;
		}
	}

	return RealZwOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}
*/


NTSTATUS __stdcall HookZwGetContextThread(
											 IN HANDLE ThreadHandle,
											 OUT PCONTEXT Context
											 )
{
	NTSTATUS status = RealZwGetContextThread(ThreadHandle, Context);

	if(gdwPID == (DWORD)PsGetCurrentProcessId() && Context != NULL)
	{
		DbgPrint("HookZwGetContextThread\n");
		SendMsg("ZwGetContextThread()");

		//去掉 drx
		Context->Dr0 = 
		Context->Dr1 = 
		Context->Dr2 = 
		Context->Dr3 = 
		Context->Dr6 = 
		Context->Dr7 = 0;
	}
	return status;
}

//如果设置了CONTEXT_DEBUG_REGISTERS，就去掉
NTSTATUS __stdcall HookZwSetContextThread(
										  IN HANDLE ThreadHandle,
										  OUT PCONTEXT Context
										  )
{
	if(gdwPID == (DWORD)PsGetCurrentProcessId() && Context != NULL)
	{
		DbgPrint("HookZwSetContextThread\n");
		SendMsg("ZwSetContextThread()");
		if(Context->ContextFlags & 0x10)
		{
			Context->ContextFlags &= ~0x10;
		}

	}
	return RealZwSetContextThread(ThreadHandle, Context);
}

/*
//Sleep
NTSTATUS __stdcall HookZwDelayExecution(
					   IN BOOLEAN              Alertable,
					   IN PLARGE_INTEGER       DelayInterval )
{
	if(gdwPID == (DWORD)PsGetCurrentProcessId() && 
		DelayInterval != NULL// &&
		//!Alertable	//Sleep
		)
	{
		//直接返回可能出问题
		
	//	int ms = (int)(DelayInterval->QuadPart / (-10000));


		
		if(DelayInterval->QuadPart != 0)
		{
			DelayInterval->QuadPart = -(1*10000);		//改为1ms
		}
		

		return STATUS_SUCCESS;

	}
	return RealZwDelayExecution(Alertable, DelayInterval);
}
*/


NTSTATUS __stdcall HookZwContinue(		   
								IN PCONTEXT             ThreadContext,
								IN BOOLEAN              RaiseAlert )
{
	if(gdwPID == (DWORD)PsGetCurrentProcessId() && ThreadContext != NULL)
	{

		//恢复
		if(gKiUserExceptionDispatcher == 1)
		{
			ThreadContext->ContextFlags |= 0x10;

			ThreadContext->Dr0 = gdwDR0;
			ThreadContext->Dr1 = gdwDR1;
			ThreadContext->Dr2 = gdwDR2;
			ThreadContext->Dr3 = gdwDR3;
			ThreadContext->Dr6 = gdwDR6;
			ThreadContext->Dr7 = gdwDR7;
		}
		gKiUserExceptionDispatcher = 0;
	}

	return RealZwContinue(ThreadContext, RaiseAlert);
}


void RestoreSSDTHook()
{
	if(gbHooked)
	{
		DisableMemoryProction();
		
		SYSCALL( ZwQueryInformationProcess ) = (PVOID) RealZwQueryInformationProcess;
		SYSCALL( ZwQueryObject)              = (PVOID) RealZwQueryObject;
		SYSCALL( ZwSetInformationThread)     = (PVOID) RealZwSetInformationThread;
		//	SYSCALL( ZwWaitForSingleObject)      = (PVOID) HookZwWaitForSingleObject;
		//	SYSCALL( ZwOpenProcess)              = (PVOID) HookZwOpenProcess;
		
		EnableMemoryProction();

	}
	if(gbHooked2)
	{
		DisableMemoryProction();

		ServiceTable->ServiceTable[ginbsave.dwNum_NtGetContextThread] = (PVOID) RealZwGetContextThread;
		ServiceTable->ServiceTable[ginbsave.dwNum_NtSetContextThread] = (PVOID) RealZwSetContextThread;

		EnableMemoryProction();
	}
}


NTSTATUS DriverCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp , IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrint("DriverClose\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	
	IoCompleteRequest(Irp , IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

VOID DriverUnload(IN PDRIVER_OBJECT	DriverObject)
{
    UNICODE_STRING dosDeviceName;
	
	WCHAR wszDeviceLink[128] = L"\\DosDevices\\accessd_at_pediy";

	DbgPrint("DriverUnload\n");

	RestoreSSDTHook();
	
	RtlInitUnicodeString(&dosDeviceName,wszDeviceLink);
    IoDeleteSymbolicLink(&dosDeviceName);

    IoDeleteDevice(DriverObject->DeviceObject);
}


void SendMsg(const char* str)
{
	int i;
	LARGE_INTEGER	TimerTime;
	//等待 gstrinfo 为空的时候写入
	for(i=0; i<10; i++)
	{
		if(gstrinfo[0] == 0)
			break;

		TimerTime.QuadPart = -6*1000*10;	//6ms
		KeDelayExecutionThread(KernelMode, FALSE, &TimerTime);

	}

	strcpy(gstrinfo, str);

	//等待 gstrinfo 为空
	for(i=0; i<10; i++)
	{
		if(gstrinfo[0] == 0)
			break;
		
		TimerTime.QuadPart = -6*1000*10;	//6ms
		KeDelayExecutionThread(KernelMode, FALSE, &TimerTime);
		
	}

}

NTSTATUS DriverDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION		irpSp;
	ULONG					ioControlCode;
	PVOID					inputBuffer;
	ULONG 					inputBufferLength;
	PVOID					outputBuffer;
	ULONG					outputBufferLength;
				
	irpSp 				= IoGetCurrentIrpStackLocation(Irp);
	ioControlCode 		= irpSp->Parameters.DeviceIoControl.IoControlCode;
	inputBuffer			= Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength 	= irpSp->Parameters.DeviceIoControl.InputBufferLength;
	outputBuffer        = Irp->AssociatedIrp.SystemBuffer;
	outputBufferLength  = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
				
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	switch (ioControlCode)
	{
	case IOCTL_PEDIY:
		{
			//inputBuffer :  保存被调试进程pid + NtDelayExecution 的ssdt号
			INBUF* pInfo = (INBUF*)inputBuffer;
			gdwDebugerPID = pInfo->dwDebugerPID;
			gdwPID = pInfo->dwPID;

			DbgPrint("start process\n");

			if(!gbHooked2)
			{

				ginbsave.dwNum_NtGetContextThread = pInfo->dwNum_NtGetContextThread;
				ginbsave.dwNum_NtSetContextThread = pInfo->dwNum_NtSetContextThread;

				DisableMemoryProction();
				//if(pInfo->dwNum_NtDelayExecution)	//有整数溢出，如果不合法的话
				//{
				//	RealZwDelayExecution = ServiceTable->ServiceTable[pInfo->dwNum_NtDelayExecution];
				//	ServiceTable->ServiceTable[pInfo->dwNum_NtDelayExecution] = (PVOID) HookZwDelayExecution;
				//}
				if(pInfo->dwNum_NtGetContextThread)
				{
					RealZwGetContextThread = ServiceTable->ServiceTable[pInfo->dwNum_NtGetContextThread];
					ServiceTable->ServiceTable[pInfo->dwNum_NtGetContextThread] = (PVOID) HookZwGetContextThread;
				}
				if(pInfo->dwNum_NtSetContextThread)
				{
					RealZwSetContextThread = ServiceTable->ServiceTable[pInfo->dwNum_NtSetContextThread];
					ServiceTable->ServiceTable[pInfo->dwNum_NtSetContextThread] = (PVOID) HookZwSetContextThread;
				}
				//if(pInfo->dwNum_NtContinue)
				//{
				//	RealZwContinue = ServiceTable->ServiceTable[pInfo->dwNum_NtContinue];
				//	ServiceTable->ServiceTable[pInfo->dwNum_NtContinue] = (PVOID)HookZwContinue;
				//}

				EnableMemoryProction();

				gbHooked2 = TRUE;
			}


		}


		break;
	case IOCTL_GETSTR:

		//如果这个时候有事件的话信息就会不正确
		strcpy(outputBuffer, gstrinfo);
		Irp->IoStatus.Information = strlen(gstrinfo);
		memset(gstrinfo, 0, sizeof(gstrinfo));

		break;
	default:
		break;
	}

	IoCompleteRequest(Irp , IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*
PCHAR MyGetModuleBaseAddress( const char* pszModuleName , OUT DWORD* pSize) 
{
    PRTL_PROCESS_MODULES    pProcessModules;    
	PRTL_PROCESS_MODULE_INFORMATION pModuleInfo;
	
    ULONG            uReturn;
    ULONG            uCount;
    PCHAR            pBuffer = NULL;
    PCHAR            pName    = NULL;
    NTSTATUS        status;
    UINT            ui;
	
    CHAR            szBuffer[16];
    PCHAR            pBaseAddress;
    
    status = ZwQuerySystemInformation( SystemModuleInformation, szBuffer, 16, &uReturn );
	
    pBuffer = ( PCHAR )ExAllocatePool( NonPagedPool, uReturn );
	
    if ( pBuffer )
    {
        status = ZwQuerySystemInformation( SystemModuleInformation, pBuffer, uReturn, &uReturn );
		
        if( status == STATUS_SUCCESS )
        {
			pProcessModules = (PRTL_PROCESS_MODULES)pBuffer;
            uCount = pProcessModules->NumberOfModules;
            pModuleInfo = pProcessModules->Modules;
			
            for ( ui = 0; ui < uCount; ui++ )
            {
                pName = strrchr( pModuleInfo->FullPathName, '\\' );
				
                if ( pName == NULL ) 
                {
                    pName = pModuleInfo->FullPathName;
                }
                else 
				{
                    pName++;
                }
				
                if( !_stricmp( pName, pszModuleName ) )
                {
                    pBaseAddress = ( PCHAR )pModuleInfo->ImageBase;
					*pSize = pModuleInfo->ImageSize;
                    ExFreePool( pBuffer );
                    return pBaseAddress;
                }
				
                pModuleInfo++;
            }
        }
		
        ExFreePool( pBuffer );
    }
	
    return NULL;
}


void* GetNTOSKRNLBaseAndSize(OUT DWORD* pdwSize)
{
	void* osknlbase = MyGetModuleBaseAddress("ntoskrnl.exe", pdwSize);
	if(osknlbase == NULL)
	{
		DbgPrint("ntoskrnl.exe base not found\n");
		osknlbase = MyGetModuleBaseAddress("ntkrnlpa.exe", pdwSize);
	}
	
	return osknlbase;
}
*/
/*
搜索 ： F6 D8 1B C0 83 E0 03 83 C0 38  (10 bytes)
  F6 D8                 neg         al
  1B C0                 sbb         eax,eax
  83 E0 03              and         eax,3
  83 C0 38              add         eax,0x38 

  下面的代码是：
   2003下是EBX， vista是 ESI

  .text:00444314                 mov     [ebx+50h], eax							89 43 50  	  / 89 46 50
  .text:00444317                 and     dword ptr [ebx+30h], 0					83 63 30 00   / 83 66 30 00
  .text:0044431B                 mov     eax, ds:_KeUserExceptionDispatcher     A1 XX XX XX XX
  .text:00444320                 mov     [ebx+68h], eax							89 43 68      / 89 46 68

*/

/*
BOOL gbIsEBX = 1;

void* BruteSearch()
{
	UINT i;
	DWORD dwSize = 0;
	BYTE* pBase = GetNTOSKRNLBaseAndSize(&dwSize);
	if(pBase == NULL || dwSize < 4096)
		return NULL;
	dwSize -= 4096;
	for(i=0; i<dwSize; i++)
	{
		if(pBase[0] == 0xF6 && pBase[1] == 0xD8)
		{
			if( *(DWORD*)(pBase+2) == 0xE083C01B &&
				*(DWORD*)(pBase+2+4) == 0x38c08303)
			{
				//判断下面几条对不对
				if( pBase[10] == 0x89 && 
					(pBase[11] == 0x43 || pBase[11] == 0x46) &&
					pBase[12] == 0x50 &&
					pBase[17] == 0xA1)
				{
					if(pBase[11] == 0x46)
					{
						gbIsEBX = 0;
					}

					return pBase;
				}
			}
		}
		pBase++;
	}
	return NULL;
}


//用户态发生异常后，在调用KiUserExceptionDispatcher前先经过本函数。
//本函数将DRx清0，防止test.exe检测DRx
//在ZwContinue时，再进行恢复
void __stdcall OnBeforeCALL_KiUserExceptionDispatcher(PCONTEXT Context)
{
	if(Context != NULL && gdwPID == (DWORD)PsGetCurrentProcessId())
	{
		//保存
		gdwDR0 = Context->Dr0;
		gdwDR1 = Context->Dr1;
		gdwDR2 = Context->Dr2;
		gdwDR3 = Context->Dr3;
		gdwDR6 = Context->Dr6;
		gdwDR7 = Context->Dr7;

		//清0

		Context->Dr0 = 0;
		Context->Dr1 = 0;
		Context->Dr2 = 0;
		Context->Dr3 = 0;
		Context->Dr6 = 0;
		Context->Dr7 = 0;

		gKiUserExceptionDispatcher = 1;

		DbgPrint("KiUserExceptionDispatcher dr0 = 0x%X\n", gdwDR0);
	}
}


void __declspec(naked) hooked_code()
{
	_asm 
	{
		//ebx+0x68 指向 r3的返回地址，因此 ebx+0x6c = cs, ebx+0x70 = eflag, ebx+0x74 = esp
		//在r3，[esp+4] 指向context

		pushad

		cmp gbIsEBX, 1
		je ISEBX
		cmp esi, 0
		je ERR
		mov eax, [esi+0x74]			//esp
		jmp CONTINUE
ISEBX:
		cmp ebx, 0
		je ERR
		mov eax, [ebx+0x74]			//esp
CONTINUE:
		cmp eax, 0
		je ERR
		mov eax, [eax+4]			//CONTEXT

		push eax
		call OnBeforeCALL_KiUserExceptionDispatcher

ERR:

		popad

		//原指令：
		neg         al
		sbb         eax,eax
		and         eax,3
		add         eax,0x38 

		ret
	}
}


BOOL  InlineHook()
{
	BYTE cbJmp[16];
	void* addr = BruteSearch();
	if(addr == NULL)
		return FALSE;

	memset(cbJmp, 0x90, sizeof(cbJmp));
	cbJmp[0] = 0xe8;	//call
	*(DWORD*)(cbJmp+1) = (DWORD)hooked_code-(DWORD)(addr)-5;

	DisableMemoryProction();
	memcpy(addr, cbJmp, 10);
	EnableMemoryProction();

	return TRUE;
}
*/

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{	
	NTSTATUS		ntStatus;
	UNICODE_STRING	uniDeviceName;
	UNICODE_STRING	uniDeviceLink;
	PDEVICE_OBJECT	pDeviceObject;
	
	WCHAR wszDeviceName[128] = L"\\Device\\accessd_at_pediy";
	WCHAR wszDeviceLink[128] = L"\\DosDevices\\accessd_at_pediy";	
		
	RtlInitUnicodeString(&uniDeviceName,wszDeviceName);
	RtlInitUnicodeString(&uniDeviceLink,wszDeviceLink);

	memset(gstrinfo, 0, sizeof(gstrinfo));

	//hook 

	ServiceTable = KeServiceDescriptorTable;

	if(!gbHooked)
	{
//		InlineHook();

		RealZwQueryInformationProcess = SYSCALL( ZwQueryInformationProcess );
		RealZwQueryObject             = SYSCALL( ZwQueryObject );
		RealZwSetInformationThread    = SYSCALL( ZwSetInformationThread );
	//	RealZwWaitForSingleObject     = SYSCALL( ZwWaitForSingleObject);
	//	RealZwOpenProcess             = SYSCALL( ZwOpenProcess);
		
		DbgPrint("driver loaded.\n");
		
		DisableMemoryProction();
		
		SYSCALL( ZwQueryInformationProcess ) = (PVOID) HookZwQueryInformationProcess;
		SYSCALL( ZwQueryObject)              = (PVOID) HookZwQueryObject;
		SYSCALL( ZwSetInformationThread)     = (PVOID) HookZwSetInformationThread;
	//	SYSCALL( ZwWaitForSingleObject)      = (PVOID) HookZwWaitForSingleObject;
	//	SYSCALL( ZwOpenProcess)              = (PVOID) HookZwOpenProcess;
		
		EnableMemoryProction();

		gbHooked = TRUE;
	}

	
	ntStatus = IoCreateDevice(DriverObject, 
		0, 
		&uniDeviceName,
		FILE_DEVICE_UNKNOWN, 
		0,
		TRUE,
		&pDeviceObject);
	if(! NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}
	
	ntStatus = IoCreateSymbolicLink(&uniDeviceLink , &uniDeviceName);
	if(! NT_SUCCESS(ntStatus) )
	{
		IoDeleteDevice(pDeviceObject);
		return ntStatus;
	}
	
	DriverObject->MajorFunction[IRP_MJ_CREATE] 			= DriverCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]  			= DriverClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = DriverDeviceControl;
	DriverObject->DriverUnload							= DriverUnload;
		
	return STATUS_SUCCESS;
}

