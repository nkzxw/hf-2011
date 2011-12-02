#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <string.h>
#include "HookShadowSSDT.h"


VOID UnloadDriver(IN PDRIVER_OBJECT DriverObject);
NTSTATUS  HideProcess_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS  HideProcess_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS  HideProcess_IoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
PVOID GetInfoTable(ULONG ATableType);
HANDLE GetCsrPid();
VOID InitCallNumber();


NTSTATUS PsLookupProcessByProcessId(IN ULONG ulProcId, OUT PEPROCESS * pEProcess);
                                               

///////////////声明Native API///////////////////////////////////////

typedef NTSTATUS (*NTUSERFINDWINDOWEX)(
				   IN HWND hwndParent, 
				   IN HWND hwndChild, 
				   IN PUNICODE_STRING pstrClassName OPTIONAL, 
				   IN PUNICODE_STRING pstrWindowName OPTIONAL, 
				   IN DWORD dwType);

typedef NTSTATUS (*NTUSERBUILDHWNDLIST)(
				   IN HDESK hdesk,
				   IN HWND hwndNext, 
				   IN ULONG fEnumChildren, 
				   IN DWORD idThread, 
				   IN UINT cHwndMax, 
				   OUT HWND *phwndFirst, 
				   OUT ULONG *pcHwndNeeded);

typedef UINT_PTR (*NTUSERQUERYWINDOW)(
		          IN ULONG WindowHandle,
				  IN ULONG TypeInformation);

typedef ULONG (*NTUSERGETFOREGROUNDWINDOW)(VOID);

typedef HWND (*NTUSERWINDOWFROMPOINT)(LONG, LONG);


NTSTATUS ZwQuerySystemInformation( 
		IN ULONG SystemInformationClass, 
		IN PVOID SystemInformation, 
		IN ULONG SystemInformationLength, 
		OUT PULONG ReturnLength);

NTSTATUS ZwDuplicateObject(
                 IN HANDLE                 SourceProcessHandle,
                 IN PHANDLE                 SourceHandle,
                 IN HANDLE                 TargetProcessHandle,
                 OUT PHANDLE               TargetHandle,
                 IN ACCESS_MASK             DesiredAccess OPTIONAL,
                 IN BOOLEAN                 InheritHandle,
                 IN ULONG                   Options );

NTSTATUS ZwQueryObject(
				IN HANDLE                ObjectHandle,
				IN ULONG                 ObjectInformationClass,
				OUT PVOID                ObjectInformation,
				IN ULONG                 ObjectInformationLength,
				OUT PULONG               ReturnLength OPTIONAL);


NTSTATUS PsLookupProcessByProcessId(
	   IN ULONG               ulProcId, 
	   OUT PEPROCESS *        pEProcess);


NTSTATUS KeAttachProcess(PEPROCESS pPeb);
NTSTATUS KeDetachProcess();

NTSTATUS MyNtUserFindWindowEx(
	   IN HWND hwndParent, 
	   IN HWND hwndChild, 
	   IN PUNICODE_STRING pstrClassName OPTIONAL, 
	   IN PUNICODE_STRING pstrWindowName OPTIONAL, 
	   IN DWORD dwType);

NTSTATUS MyNtUserBuildHwndList(
	   IN HDESK hdesk, 
	   IN HWND hwndNext, 
	   IN ULONG fEnumChildren, 
	   IN DWORD idThread, 
	   IN UINT cHwndMax,
	   OUT HWND *phwndFirst, 
	   OUT ULONG* pcHwndNeeded);

UINT_PTR MyNtUserQueryWindow(
	   IN ULONG WindowHandle,
	   IN ULONG TypeInformation);

ULONG MyNtUserGetForegroundWindow(VOID);

HWND MyNtUserWindowFromPoint(LONG x, LONG y);


__declspec(dllimport) _stdcall KeAddSystemServiceTable(PVOID, PVOID, PVOID, PVOID, PVOID);

////////////////////定义所用到的全局变量///////////////
__declspec(dllimport)  ServiceDescriptorTableEntry KeServiceDescriptorTable;

unsigned long OldCr0;
UNICODE_STRING DeviceNameString;
UNICODE_STRING LinkDeviceNameString;

NTUSERFINDWINDOWEX          g_OriginalNtUserFindWindowEx;
NTUSERBUILDHWNDLIST         g_OriginalNtUserBuildHwndList;
NTUSERQUERYWINDOW           g_OriginalNtUserQueryWindow;
NTUSERGETFOREGROUNDWINDOW   g_OriginalNtUserGetForegroundWindow;
NTUSERWINDOWFROMPOINT       g_OriginalNtUserWindowFromPoint;

PEPROCESS crsEProc;

CCHAR     outBuf[1024];                        //输入缓冲区大小

HANDLE ProcessIdToProtect = (HANDLE)0;        //保护的句柄

ULONG NtUserFindWindowEx_callnumber = 0;          //NtUserFindWindowEx的服号
ULONG NtUserGetForegroundWindow_callnumber = 0;
ULONG NtUserQueryWindow_callnumber = 0;
ULONG NtUserBuildHwndList_callnumber = 0;
ULONG NtUserWindowFromPoint_callnumber = 0;
ULONG LastForegroundWindow;

unsigned int getAddressOfShadowTable()
{
    unsigned int i;
    unsigned char *p;
    unsigned int dwordatbyte;

    p = (unsigned char*) KeAddSystemServiceTable;

    for(i = 0; i < 4096; i++, p++)
    {
        __try
        {
            dwordatbyte = *(unsigned int*)p;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }

        if(MmIsAddressValid((PVOID)dwordatbyte))
        {
            if(memcmp((PVOID)dwordatbyte, &KeServiceDescriptorTable, 16) == 0)
            {
                if((PVOID)dwordatbyte == &KeServiceDescriptorTable)
                {
                    continue;
                }

                return dwordatbyte;
            }
        }
    }

    return 0;
}

ULONG getShadowTable()
{
    KeServiceDescriptorTableShadow = (PServiceDescriptorTableEntry) getAddressOfShadowTable();

    if(KeServiceDescriptorTableShadow == NULL)
    {
        DbgPrint("hooker.sys: Couldnt find shadowtable!");
        
        return FALSE;
    }
    else
    {
        DbgPrint("hooker.sys: Shadowtable has been found!");
        
        DbgPrint("hooker.sys: Shadowtable entries: %d", KeServiceDescriptorTableShadow[1].NumberOfServices);
        return TRUE;
    }
} 

//根据操作系统来确定具体函数的服务号 
VOID InitCallNumber()
{
	ULONG majorVersion, minorVersion;
	PsGetVersion( &majorVersion, &minorVersion, NULL, NULL );
    if ( majorVersion == 5 && minorVersion == 2 )
    {
	  DbgPrint("comint32: Running on Windows 2003");
      NtUserFindWindowEx_callnumber = 0x179;
	  NtUserGetForegroundWindow_callnumber = 0x193;
	  NtUserBuildHwndList_callnumber = 0x137;
	  NtUserQueryWindow_callnumber = 0x1E1;
	  NtUserWindowFromPoint_callnumber = 0x24C;

	}
	else if ( majorVersion == 5 && minorVersion == 1 )
	{
	  DbgPrint("comint32: Running on Windows XP");
      NtUserFindWindowEx_callnumber = 0x17A;
	  NtUserGetForegroundWindow_callnumber = 0x194;
	  NtUserBuildHwndList_callnumber = 0x138;
	  NtUserQueryWindow_callnumber = 0x1E3;
	  NtUserWindowFromPoint_callnumber = 0x250;
	}
	else if ( majorVersion == 5 && minorVersion == 0 )
	{
	  DbgPrint("comint32: Running on Windows 2000");
	  NtUserFindWindowEx_callnumber = 0x170;
	  NtUserGetForegroundWindow_callnumber = 0x189;
	  NtUserBuildHwndList_callnumber = 0x12E;
	  NtUserQueryWindow_callnumber = 0x1D2;
	  NtUserWindowFromPoint_callnumber = 0x238;
	}
}

PVOID GetInfoTable(ULONG ATableType)
{
  ULONG mSize = 0x4000;
  PVOID mPtr = NULL;
  NTSTATUS St;
  do
  {
     mPtr = ExAllocatePool(PagedPool, mSize);
     memset(mPtr, 0, mSize);
     if (mPtr)
     {
        St = ZwQuerySystemInformation(ATableType, mPtr, mSize, NULL);
     } else return NULL;
     if (St == STATUS_INFO_LENGTH_MISMATCH)
     {
        ExFreePool(mPtr);
        mSize = mSize * 2;
     }
  } while (St == STATUS_INFO_LENGTH_MISMATCH);
  if (St == STATUS_SUCCESS) return mPtr;
  ExFreePool(mPtr);
  return NULL;
}

HANDLE GetCsrPid()
{
	HANDLE Process, hObject;
	HANDLE CsrId = (HANDLE)0;
	OBJECT_ATTRIBUTES obj;
	CLIENT_ID cid;
	UCHAR Buff[0x100];
	POBJECT_NAME_INFORMATION ObjName = (PVOID)&Buff;
	PSYSTEM_HANDLE_INFORMATION_EX Handles;
	ULONG r;

	Handles = GetInfoTable(SystemHandleInformation);

	if (!Handles) return CsrId;

	for (r = 0; r < Handles->NumberOfHandles; r++)
	{
		if (Handles->Information[r].ObjectTypeNumber == 21) //Port object
		{
			InitializeObjectAttributes(&obj, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

			cid.UniqueProcess = (HANDLE)Handles->Information[r].ProcessId;
			cid.UniqueThread = 0;

			if (NT_SUCCESS(NtOpenProcess(&Process, PROCESS_DUP_HANDLE, &obj, &cid)))
			{
				if (NT_SUCCESS(ZwDuplicateObject(Process, (HANDLE)Handles->Information[r].Handle,NtCurrentProcess(), &hObject, 0, 0, DUPLICATE_SAME_ACCESS)))
				{
					if (NT_SUCCESS(ZwQueryObject(hObject, ObjectNameInformation, ObjName, 0x100, NULL)))
					{
						if (ObjName->Name.Buffer && !wcsncmp(L"\\Windows\\ApiPort", ObjName->Name.Buffer, 20))
						{
						  CsrId = (HANDLE)Handles->Information[r].ProcessId;
						} 
					}

					ZwClose(hObject);
				}

				ZwClose(Process);
			}
		}
	}

	ExFreePool(Handles);
	return CsrId;
}

BOOLEAN Sleep(ULONG MillionSecond)
{
	NTSTATUS st;
	LARGE_INTEGER DelayTime;
	DelayTime = RtlConvertLongToLargeInteger(-10000*MillionSecond);
	st=KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );
	return (NT_SUCCESS(st));
}

NTSTATUS DriverEntry (IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
  NTSTATUS status;
  PDEVICE_OBJECT   deviceObject;
 
   RtlInitUnicodeString( &DeviceNameString,    HIDE_PROCESS_WIN32_DEV_NAME );
   RtlInitUnicodeString( &LinkDeviceNameString,HIDE_PROCESS_DEV_NAME );

   KdPrint(("DriverEntry Enter............................\n"));
  
   status = IoCreateDevice(
                DriverObject,
                0,                      
                &DeviceNameString,
                FILE_DEVICE_DISK_FILE_SYSTEM,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                & deviceObject );

    if (!NT_SUCCESS( status )) 
    {

        KdPrint(( "DriverEntry: Error creating control device object, status=%08x\n", status ));
        return status;
    }

   status = IoCreateSymbolicLink(
                (PUNICODE_STRING) &LinkDeviceNameString,
                (PUNICODE_STRING) &DeviceNameString
                );

   if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }
 
  //获得shadow的地址
  getShadowTable();
  //根据不同的系统获得不同的函数服务号
  InitCallNumber();

  DriverObject->MajorFunction[IRP_MJ_CREATE] = HideProcess_Create;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = HideProcess_Close;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HideProcess_IoControl;

  DriverObject->DriverUnload=UnloadDriver;

  status = PsLookupProcessByProcessId((ULONG)GetCsrPid(), &crsEProc);
  if (!NT_SUCCESS( status ))
  {
	DbgPrint("PsLookupProcessByProcessId() error\n");
	return status;
  }
  KeAttachProcess(crsEProc);

  __try
  {
	  if ((KeServiceDescriptorTableShadow!=NULL) \
		  && (NtUserFindWindowEx_callnumber!=0) && (NtUserGetForegroundWindow_callnumber!=0) \
		  && (NtUserBuildHwndList_callnumber!=0) && (NtUserQueryWindow_callnumber!=0) \
		  && (NtUserWindowFromPoint_callnumber!=0)) 
	  {
		g_OriginalNtUserFindWindowEx     = (NTUSERFINDWINDOWEX)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserFindWindowEx_callnumber];
		g_OriginalNtUserQueryWindow=(NTUSERQUERYWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserQueryWindow_callnumber];						
		g_OriginalNtUserBuildHwndList=(NTUSERBUILDHWNDLIST)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserBuildHwndList_callnumber];
        g_OriginalNtUserGetForegroundWindow=(NTUSERGETFOREGROUNDWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserGetForegroundWindow_callnumber];
        g_OriginalNtUserWindowFromPoint = (NTUSERWINDOWFROMPOINT)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserWindowFromPoint_callnumber];	
	  }
	  else
		KeServiceDescriptorTableShadow=NULL;
		

	  _asm
	  {
		CLI                    //dissable interrupt
		MOV    EAX, CR0        //move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV    CR0, EAX        //write register back
	  }
	  if ((KeServiceDescriptorTableShadow!=NULL) && (NtUserFindWindowEx_callnumber!=0) && (NtUserGetForegroundWindow_callnumber!=0) && (NtUserBuildHwndList_callnumber!=0) && (NtUserQueryWindow_callnumber!=0))
	  {
		(NTUSERFINDWINDOWEX)(KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserFindWindowEx_callnumber]) = MyNtUserFindWindowEx;
        (NTUSERQUERYWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserQueryWindow_callnumber]  = MyNtUserQueryWindow;
		(NTUSERBUILDHWNDLIST)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserBuildHwndList_callnumber] = MyNtUserBuildHwndList;
        (NTUSERGETFOREGROUNDWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserGetForegroundWindow_callnumber] = MyNtUserGetForegroundWindow;
        (NTUSERWINDOWFROMPOINT)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserWindowFromPoint_callnumber] = MyNtUserWindowFromPoint;
	  }

	  _asm 
	  {
		MOV    EAX, CR0        //move CR0 register into EAX
		OR     EAX, 10000H        //enable WP bit     
		MOV    CR0, EAX        //write register back        
		STI                    //enable interrupt
	  }
  }
  __finally
  {
      KeDetachProcess(); 
  }

  KdPrint(("Hook ZwQuerySystemInformation'status is Succeessfully "));


  return status ;
}

NTSTATUS HideProcess_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrint("HideProcess_Create\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Irp->IoStatus.Status;
}

NTSTATUS HideProcess_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrint("HideProcess_Close\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Irp->IoStatus.Status;
}


NTSTATUS HideProcess_IoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS                    status = STATUS_SUCCESS;
	ULONG						controlCode;
	PIO_STACK_LOCATION			irpStack;
	HANDLE						hEvent;
	OBJECT_HANDLE_INFORMATION	objHandleInfo;
	ULONG                       outputLength, inputLength;
	PVOID                       inputBuffer;
DWORD dd;
	
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	inputLength=irpStack->Parameters.DeviceIoControl.InputBufferLength;
	controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	
	DbgPrint("IN CONTROL\r\n");
	switch(controlCode)
	{
	case IO_PROTECT:
		ProcessIdToProtect = (HANDLE)irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
		DbgPrint("IO_PROTECT:%d", ProcessIdToProtect);
		break;
	default:
		break;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}


VOID UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING uniWin32NameString;
    UNICODE_STRING LinkNameString;
    PDEVICE_OBJECT deviceObject;
	NTSTATUS status;

	status = PsLookupProcessByProcessId((ULONG)GetCsrPid(), &crsEProc);
	if (!NT_SUCCESS( status ))
	{
	  DbgPrint("PsLookupProcessByProcessId() error\n");
	  return ;
	}
	KeAttachProcess(crsEProc);

//////////////////////UnHook ZwQuerySystemInformation/////////////////////////////////////////////////
 
	__try
	{
	  _asm
	 {
		CLI                    //dissable interrupt
		MOV    EAX, CR0        //move CR0 register into EAX
		AND EAX, NOT 10000H    //disable WP bit 
		MOV    CR0, EAX        //write register back
	  }

	  if ((KeServiceDescriptorTableShadow!=NULL) && (NtUserFindWindowEx_callnumber!=0) && (NtUserGetForegroundWindow_callnumber!=0) && (NtUserBuildHwndList_callnumber!=0) && (NtUserQueryWindow_callnumber!=0)) 
	  {
		(NTUSERFINDWINDOWEX)(KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserFindWindowEx_callnumber]) = g_OriginalNtUserFindWindowEx;
		(NTUSERQUERYWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserQueryWindow_callnumber]		= g_OriginalNtUserQueryWindow;						
		(NTUSERBUILDHWNDLIST)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserBuildHwndList_callnumber]	= g_OriginalNtUserBuildHwndList;
		(NTUSERGETFOREGROUNDWINDOW)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserGetForegroundWindow_callnumber]    = g_OriginalNtUserGetForegroundWindow;
		(NTUSERWINDOWFROMPOINT)KeServiceDescriptorTableShadow[1].ServiceTableBase[NtUserWindowFromPoint_callnumber] = g_OriginalNtUserWindowFromPoint;
	  }

	  _asm 
	  {
		MOV    EAX, CR0        //move CR0 register into EAX
		OR     EAX, 10000H     //enable WP bit     
		MOV    CR0, EAX        //write register back        
		STI                    //enable interrupt
	  }
    }
	__finally
   {
	 KeDetachProcess();
	 Sleep(50);
   }
  
    deviceObject= DriverObject->DeviceObject;
    IoDeleteSymbolicLink(&LinkDeviceNameString);
    ASSERT(!deviceObject->AttachedDevice);
    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }
}

NTSTATUS MyNtUserFindWindowEx(
	   IN HWND hwndParent, 
	   IN HWND hwndChild, 
	   IN PUNICODE_STRING pstrClassName OPTIONAL, 
	   IN PUNICODE_STRING pstrWindowName OPTIONAL, 
	   IN DWORD dwType)
{
	ULONG result;

	result = g_OriginalNtUserFindWindowEx(hwndParent, hwndChild, pstrClassName, pstrWindowName, dwType);

	if (PsGetCurrentProcessId()!=ProcessIdToProtect)
	{
		ULONG ProcessID;
		
		ProcessID = g_OriginalNtUserQueryWindow(result, 0);
		DbgPrint("ProcessID:%d", ProcessID);
		if (ProcessID==(ULONG)ProcessIdToProtect)
			return 0;
	}
	return result;
}

NTSTATUS MyNtUserBuildHwndList(IN HDESK hdesk, IN HWND hwndNext, IN ULONG fEnumChildren, IN DWORD idThread, IN UINT cHwndMax, OUT HWND *phwndFirst, OUT ULONG* pcHwndNeeded)
{
	NTSTATUS result;

	if (PsGetCurrentProcessId()!=ProcessIdToProtect)
	{
		ULONG ProcessID;
		
		if (fEnumChildren==1)
		{
            ProcessID = g_OriginalNtUserQueryWindow((ULONG)hwndNext, 0);
			if (ProcessID==(ULONG)ProcessIdToProtect)
				return STATUS_UNSUCCESSFUL;
		}
		result = g_OriginalNtUserBuildHwndList(hdesk,hwndNext,fEnumChildren,idThread,cHwndMax,phwndFirst,pcHwndNeeded);

		if (result==STATUS_SUCCESS)
		{
			ULONG i=0;
			ULONG j;

			while (i<*pcHwndNeeded)
			{
				ProcessID=g_OriginalNtUserQueryWindow((ULONG)phwndFirst[i],0);
				if (ProcessID==(ULONG)ProcessIdToProtect)
				{
					for (j=i; j<(*pcHwndNeeded)-1; j++)					
						phwndFirst[j]=phwndFirst[j+1]; 

					phwndFirst[*pcHwndNeeded-1]=0; 

					(*pcHwndNeeded)--;
					continue; 
				}
                i++;				
			}
			
		}
		return result;
	}
	return g_OriginalNtUserBuildHwndList(hdesk,hwndNext,fEnumChildren,idThread,cHwndMax,phwndFirst,pcHwndNeeded);
}

ULONG MyNtUserGetForegroundWindow(VOID)
{
	ULONG result;

	result= g_OriginalNtUserGetForegroundWindow();	

	if (PsGetCurrentProcessId()!=ProcessIdToProtect)
	{
		ULONG ProcessID;
		
		ProcessID=g_OriginalNtUserQueryWindow(result, 0);
		if (ProcessID == (ULONG)ProcessIdToProtect)
			result=LastForegroundWindow;
		else
            LastForegroundWindow=result;
	}	
	return result;
}

UINT_PTR MyNtUserQueryWindow(IN ULONG WindowHandle,IN ULONG TypeInformation)
{
	ULONG WindowHandleProcessID;

	if (PsGetCurrentProcessId()!=ProcessIdToProtect)
	{
		WindowHandleProcessID = g_OriginalNtUserQueryWindow(WindowHandle,0);
		if (WindowHandleProcessID==(ULONG)ProcessIdToProtect)
			return 0;
	}
	return g_OriginalNtUserQueryWindow(WindowHandle,TypeInformation);
}

HWND MyNtUserWindowFromPoint(LONG x, LONG y)
{
	return 0;
}


