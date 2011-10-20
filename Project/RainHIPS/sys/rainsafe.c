/*
	 停车,内裤留下
　    ● 　　  ╭══╮
　 　█┳　 ╭╯ΘΘ║
　 ┛┗　　╰⊙═⊙╯。oо○
*/
#include "dbghelp.h"
#include "rainsafe.h"
#include <stdio.h>
#include <stdarg.h>
#include <ntimage.h>
#include <ntiologc.h>
#include <string.h>


int    index_CreateProcess=-1;
int    index_CreateProcessEx=-1;
int    index_SetValueKey=-1;
int	   index_LoadDriver=-1;
int    index_SetSystemTime=-1;
int    index_SetSystemInformation=-1;
int    index_SystemDebugControl=-1;
int    index_OpenProcess=-1;
int    index_OpenThread=-1;
int    index_WriteFile=-1;
int    index_CreateUserProcess=-1;
int    index_UserSetInformationThread=-1;
HANDLE Appevent;
PVOID  AppeventObject = NULL;
HANDLE Sysevent;
PVOID  SyseventObject = NULL;
BOOLEAN ISRun=FALSE;
char   *output;
ULONG  HipsCurrentProcess;
ULONG  HipsCurrentProcessId;
ULONG  SSDTTableBase;
ULONG  ShadowSSDTTableBase;
WIN_VER_DETAIL yourWinVer;
char   SafePatch[MAXPATHLEN]="NULL:\\NULL\\NULL";
int    SafePatchLen;

NTSTATUS FakedZwCreateProcess (OUT PHANDLE ProcessHandle,
									  IN ACCESS_MASK DesiredAccess,
									  IN POBJECT_ATTRIBUTES ObjectAttributes,
									  IN HANDLE ParentProcess,
									  IN BOOLEAN InheritObjectTable,
									  IN HANDLE SectionHandle,
									  IN HANDLE DebugPort,
					                  IN HANDLE ExceptionPort);

NTSTATUS FakedZwCreateProcessEx(
							  OUT PHANDLE ProcessHandle,
							  IN ACCESS_MASK DesiredAccess,
							  IN POBJECT_ATTRIBUTES ObjectAttributes,
							  IN HANDLE InheritFromProcessHandle,
							  IN BOOLEAN InheritHandles,
							  IN HANDLE SectionHandle OPTIONAL,
							  IN HANDLE DebugPort OPTIONAL,
							  IN HANDLE ExceptionPort OPTIONAL,
							  IN HANDLE Unknown 
							  );

NTSTATUS FakedNtCreateUserProcess (PHANDLE ProcessHandle,
										PHANDLE ThreadHandle,
										PVOID Parameter2,
										PVOID Parameter3,
										PVOID ProcessSecurityDescriptor,
										PVOID ThreadSecurityDescriptor,
										PVOID Parameter6,
										PVOID Parameter7,
										PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
										PVOID Parameter9,
										PVOID pProcessUnKnow);

NTSTATUS FakedZwSetValueKey
(
							  IN HANDLE  KeyHandle,
							  IN PUNICODE_STRING  ValueName,
							  IN ULONG  TitleIndex  OPTIONAL,
							  IN ULONG  Type,
							  IN PVOID  Data,
							  IN ULONG  DataSize
 );

 NTSTATUS FakedZwLoadDriver
(
							  IN PUNICODE_STRING DriverServiceName
 );

 NTSTATUS FakedZwSetSystemTime(PLARGE_INTEGER NewTime,PLARGE_INTEGER OldTime);

  NTSTATUS FakedZwLoadDriver
(
							  IN PUNICODE_STRING DriverServiceName
 );


NTSTATUS FakedNtSetSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);

NTSTATUS FakedNtSystemDebugControl(
	IN DEBUG_CONTROL_CODE ControlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnLength OPTIONAL
	);

NTSTATUS FakedNtOpenProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
);

NTSTATUS FakedNtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
);

NTSTATUS FakedNtWriteFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN PULONG Key OPTIONAL
	);

NTSTATUS FakedNtUserSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);


VOID UnHOOkAll();
VOID ProcMoniterOn();
VOID ProcMoniterOff();
VOID RegMoniterOn();
VOID RegMoniterOff();
VOID ModMonitorOn();
VOID ModMonitorOff();
VOID TimeSafeOn();
VOID TimeSafeOff();
VOID NtSystemDebugOn();
VOID NtSystemDebugOff();
VOID NtOpenProOn();
VOID NtOpenProOff();
VOID NtOpenThrOn();
VOID NtOpenThrOff();
VOID NtWriteFileHookOn();
VOID NtWriteFileHookOff();
VOID NtUserSetInformationThreadHookOn();
VOID NtUserSetInformationThreadHookOff();

ULONG bPrMon         = 0;
ULONG bProcMon       = 0;
ULONG bUsPrMon       = 0;
ULONG bRegMon        = 0;
ULONG bModMon        = 0;
ULONG bTimeMon       = 0;
ULONG bSsiMon        = 0;
ULONG bsdc           = 0;
ULONG bopr           = 0;
ULONG botr           = 0;
ULONG bWriteFile     = 0;
ULONG bUsit     = 0;

ULONG RealIoVolumeDeviceToDosName=0;
ULONG RealZwCreateProcess;
ULONG RealZwCreateProcessEx;
ULONG RealNtCreateUserProcess;
ULONG RealZwSetValueKey;
ULONG RealZwLoadDriver;
ULONG RealZwSetSystemTime;
ULONG RealNtSetSystemInformation;
ULONG RealNtSystemDebugControl;
ULONG RealNtOpenProcess;
ULONG RealNtOpenThread;
ULONG RealNtWriteFile;
ULONG RealNtUserSetInformationThread;




NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	);

NTSTATUS
RainsafeDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
RainsafeDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
RainsafeDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

VOID
RainsafeUnload(
	IN PDRIVER_OBJECT		DriverObject
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, RainsafeDispatchCreate)
#pragma alloc_text(PAGE, RainsafeDispatchClose)
#pragma alloc_text(PAGE, RainsafeDispatchDeviceControl)
#pragma alloc_text(PAGE, RainsafeUnload)
#endif // ALLOC_PRAGMA





NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	)
{
	NTSTATUS			status = STATUS_SUCCESS;    
    UNICODE_STRING		ntDeviceName;
	UNICODE_STRING		dosDeviceName;
    PDEVICE_EXTENSION	deviceExtension;
	PDEVICE_OBJECT		deviceObject = NULL;
	BOOLEAN				fSymbolicLink = FALSE;

	KdBreakPoint();
    RtlInitUnicodeString(&ntDeviceName, RAINSAFE_DEVICE_NAME_W);
	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),
		&ntDeviceName,
		FILE_DEVICE_RAINSAFE,
		0,
		TRUE,
		&deviceObject
		);

    if (!NT_SUCCESS(status))
	{
		goto __failed;
	}

	deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
	RtlInitUnicodeString(&dosDeviceName, RAINSAFE_DOS_DEVICE_NAME_W);
	status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);
	if (!NT_SUCCESS(status))
	{
		goto __failed;
    }

	fSymbolicLink = TRUE;

	DriverObject->MajorFunction[IRP_MJ_CREATE]         = RainsafeDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = RainsafeDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RainsafeDispatchDeviceControl;
    DriverObject->DriverUnload                         = RainsafeUnload;
	
    if (NT_SUCCESS(status))
	    return status;

__failed:

	if (fSymbolicLink)
		IoDeleteSymbolicLink(&dosDeviceName);

	if (deviceObject)
		IoDeleteDevice(deviceObject);

	return status;
}

int ConvertFileNameWCHARToCHAR(PWCHAR pWChar, PCHAR pChar)
{
UNICODE_STRING usFileName;
ANSI_STRING asFileName;

RtlInitUnicodeString(&usFileName, pWChar);

asFileName.Length = 0;
asFileName.MaximumLength = 256;
asFileName.Buffer = pChar;

RtlUnicodeStringToAnsiString(&asFileName, &usFileName, FALSE);
pChar[asFileName.Length] = 0;
return asFileName.Length;
}

int ConvertFileNameUNISTRToCHAR(PUNICODE_STRING usFileName, PCHAR pChar)
{
ANSI_STRING asFileName;

asFileName.Length = 0;
asFileName.MaximumLength = 256;
asFileName.Buffer = pChar;

RtlUnicodeStringToAnsiString(&asFileName, usFileName, FALSE);
pChar[asFileName.Length] = 0;
return asFileName.Length;
}


NTSTATUS   GetFullNameVista(HANDLE  KeyHandle,char   *fullname)  //取子进程文件名
{
 PFILE_OBJECT   pKey=NULL;
 ULONG              retSize;
 NTSTATUS   ns;
 UNICODE_STRING   dosName;
 char aPathName[MAXPATHLEN];

 if (KeyHandle==0) return STATUS_ACCESS_DENIED;
 ns=ObReferenceObjectByHandle(KeyHandle,0,NULL,KernelMode,&pKey,NULL);
 if (!NT_SUCCESS(ns)) return ns;

__try
 {
		if ((RealIoVolumeDeviceToDosName!=0)&&(RealIoVolumeDeviceToDosName!=-1))
		{
		  ns=((TypeIoVolumeDeviceToDosName)RealIoVolumeDeviceToDosName)(pKey->DeviceObject,&dosName);
		}
		else
		{
		  ns=RtlVolumeDeviceToDosName(pKey->DeviceObject,&dosName);
		}
 if (!NT_SUCCESS(ns)) return ns;
 ConvertFileNameWCHARToCHAR(dosName.Buffer,fullname);
 ConvertFileNameWCHARToCHAR(pKey->FileName.Buffer,aPathName);
 strcat(fullname,aPathName);
 }
__except (EXCEPTION_EXECUTE_HANDLER)
{

}
return STATUS_SUCCESS;
}

NTSTATUS   GetFullName2000_2003(HANDLE     KeyHandle,char   *fullname)  //取子进程文件名
{
	NTSTATUS   ns;
	PVOID   pKey=NULL,pFile=NULL;
	UNICODE_STRING                   fullUniName;
	ANSI_STRING                           akeyname;
	ULONG   actualLen;   
	UNICODE_STRING   dosName;

	if (KeyHandle==0) return   STATUS_ACCESS_DENIED;
	
	fullUniName.Buffer=NULL;
	fullUniName.Length=0;   
	fullname[0]=0x00;   
	ns=ObReferenceObjectByHandle(KeyHandle,0,NULL,KernelMode,&pKey,NULL);
	if(!NT_SUCCESS(ns))   return ns;
	fullUniName.Buffer = ExAllocatePool(   PagedPool,   MAXPATHLEN*2);//1024*2
	fullUniName.MaximumLength =MAXPATHLEN*2;
	__try
	{
		pFile=(PVOID)*(ULONG   *)((char   *)pKey+20);
		pFile=(PVOID)*(ULONG   *)((char   *)pFile);
		pFile=(PVOID)*(ULONG   *)((char   *)pFile+36);
	  
		ObReferenceObjectByPointer(pFile,   0,   NULL,   KernelMode);
		//DbgPrintEx(DPFLTR_IHVBUS_ID ,DPFLTR_ERROR_LEVEL,"%08x",RealIoVolumeDeviceToDosName);

		if ((RealIoVolumeDeviceToDosName!=0)&&(RealIoVolumeDeviceToDosName!=-1))
		{
		  ((TypeIoVolumeDeviceToDosName)RealIoVolumeDeviceToDosName)(((PFILE_OBJECT)pFile)->DeviceObject,&dosName);
		}
		else
		{
		RtlVolumeDeviceToDosName(((PFILE_OBJECT)pFile)->DeviceObject,&dosName);
		}
		RtlCopyUnicodeString(&fullUniName,   &dosName);
		RtlAppendUnicodeStringToString(&fullUniName,&((PFILE_OBJECT)pFile)->FileName);
		ObDereferenceObject(pFile);
		ObDereferenceObject(pKey);
		RtlUnicodeStringToAnsiString(&akeyname,&fullUniName,TRUE);
		if(akeyname.Length<MAXPATHLEN)     
		{   
			memcpy(fullname,akeyname.Buffer,akeyname.Length);   
			fullname[akeyname.Length]=0x00;   
		}   
		else   
		{   
			memcpy(fullname,akeyname.Buffer,MAXPATHLEN);   
			fullname[MAXPATHLEN-1]=0x00;   
		}
		
		RtlFreeAnsiString(&akeyname);   
		ExFreePool(dosName.Buffer);   
		ExFreePool(fullUniName.Buffer);  
		return   STATUS_SUCCESS;
	}
	__except(1)   
	{   
		if(fullUniName.Buffer) ExFreePool(fullUniName.Buffer);   
		if(pKey)   ObDereferenceObject(pKey   );   
		return   STATUS_SUCCESS;
	}
}

NTSTATUS   GetFullName(HANDLE     KeyHandle,char   *fullname)
{
  if (yourWinVer==WINDOWS_VERSION_VISTA)
  {
	  return GetFullNameVista(KeyHandle,fullname);
  }
  else
  {
       return GetFullName2000_2003(KeyHandle,fullname);
  }
}

BOOLEAN GetFullName2(HANDLE handle,char * pch)   //取注册表路径
{

	ULONG uactLength;
	POBJECT_NAME_INFORMATION  pustr;
	ANSI_STRING astr;
	PVOID pObj;
	NTSTATUS ns;
	if (handle==0) return FALSE;
	ns = ObReferenceObjectByHandle( handle, 0, NULL, KernelMode, &pObj, NULL );
	if (!NT_SUCCESS(ns))
	{
		return FALSE;
	}
	pustr = ExAllocatePool(NonPagedPool,1024+4);

	if (pObj==NULL||pch==NULL)
		return FALSE;

__try {
	ns = ObQueryNameString(pObj,pustr,512,&uactLength);

	if (NT_SUCCESS(ns))
	{
		RtlUnicodeStringToAnsiString(&astr,(PUNICODE_STRING)pustr,TRUE);
		strcpy(pch,astr.Buffer);
	}
	ExFreePool(pustr);
	RtlFreeAnsiString( &astr );
	if (pObj)
	{
		ObDereferenceObject(pObj);
	}
}
__except (EXCEPTION_EXECUTE_HANDLER)
{

}
	return TRUE;
}



LONG GetProcessName2( PCHAR theName )     //取父进程路径
{
ULONG i;
ULONG dwAddress = (ULONG)PsGetCurrentProcess();

if(dwAddress == 0 || dwAddress == 0xFFFFFFFF)
return FALSE;

if (yourWinVer==WINDOWS_VERSION_2K3)
{
dwAddress += 0x190;
}
else
{
 if (yourWinVer==WINDOWS_VERSION_2K3_SP1_SP2)
 {
  dwAddress += 0x1A0;
 }
 else
 {
   if (yourWinVer==WINDOWS_VERSION_VISTA)
   {
   dwAddress += 0x188;
   }
   else
   {
   dwAddress += 0x1B0;
   }
 }
}


if((dwAddress = *(ULONG*)dwAddress) == 0) return FALSE;
dwAddress += 0x10;
if((dwAddress = *(ULONG*)dwAddress) == 0) return FALSE;
dwAddress += 0x3C;
if((dwAddress = *(ULONG*)dwAddress) == 0) return FALSE;


ConvertFileNameWCHARToCHAR((PWCHAR)dwAddress,theName);
return TRUE;
}

PULONG PsGetProcessObjectTable(ULONG mPEPROCESS)
{
  ULONG result;
  ULONG TempPid;
  result=0;
  if (!MmIsAddressValid((PVOID)mPEPROCESS))  return (PULONG)result;

  if (yourWinVer==WINDOWS_VERSION_2K)
  {
   memmove(&TempPid,(PULONG)(mPEPROCESS+0x9c),4);
   if (TempPid==HipsCurrentProcessId) memmove(&result,(PULONG)(mPEPROCESS+0x128),4);
  }
  else
  {
   if ((yourWinVer==WINDOWS_VERSION_XP)||(yourWinVer==WINDOWS_VERSION_2K3))
   {
	 memmove(&TempPid,(PULONG)(mPEPROCESS+0x84),4);
	 if (TempPid==HipsCurrentProcessId) memmove(&result,(PULONG)(mPEPROCESS+0x0c4),4);
   }
   else
   {
	 if (yourWinVer==WINDOWS_VERSION_2K3_SP1_SP2)
	 {
	   memmove(&TempPid,(PULONG)(mPEPROCESS+0x94),4);
	   if (TempPid==HipsCurrentProcessId) memmove(&result,(PULONG)(mPEPROCESS+0x0d4),4);
	 }
	 else
	 {
	   if (yourWinVer==WINDOWS_VERSION_VISTA)
	   {
		 memmove(&TempPid,(PULONG)(mPEPROCESS+0x9c),4);
		 if (TempPid==HipsCurrentProcessId) memmove(&result,(PULONG)(mPEPROCESS+0x0dc),4);
	   }
	   else
	   {
         result=0;
       }
     }
   }
  }
  return (PULONG)result;
}


LONG GoOrNot(char *fathername,char *procname)
{
	char buff[512] = {0};
	ULONG a;
	PULONG TempObjectTable;
	LARGE_INTEGER li;li.QuadPart=-10000;

	TempObjectTable=PsGetProcessObjectTable(HipsCurrentProcess);

	if (TempObjectTable!=NULL)
	{
		while (ISRun)     //防止多线程同步执行 
		{
          KeDelayExecutionThread(KernelMode,0,&li);
		}
		ISRun=TRUE;
		strcpy(buff,fathername);
		strcat(buff,procname);
		strncpy(&output[8],buff,sizeof(buff));
		a = 1;
		memmove(&output[0],&a,4);
		KeSetEvent((PRKEVENT)AppeventObject,0,0);
		KeWaitForSingleObject((PRKEVENT)SyseventObject,Executive,KernelMode,0,0);
		KeResetEvent((PRKEVENT)SyseventObject);
		memmove(&a,&output[4],4);
		ISRun=FALSE;
	}
	else
	{
		a=1;
	}
	return a;
}

NTSTATUS FakedZwCreateProcess (OUT PHANDLE ProcessHandle,
									  IN ACCESS_MASK DesiredAccess,
									  IN POBJECT_ATTRIBUTES ObjectAttributes,
									  IN HANDLE ParentProcess,
									  IN BOOLEAN InheritObjectTable,
									  IN HANDLE SectionHandle,
									  IN HANDLE DebugPort,
									  IN HANDLE ExceptionPort)
{
	char aProcessName[MAXPATHLEN];
	char aPathName[MAXPATHLEN];
	ULONG TempCurrentProcess;

	if (ProcessHandle==0) return RETURN_ERRO_NOBOX;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if (HipsCurrentProcess==TempCurrentProcess)
	{
		return ((ZWCREATEPROCESS)RealZwCreateProcess)(
			ProcessHandle,
			DesiredAccess,
			ObjectAttributes,
			ParentProcess,
			InheritObjectTable,
			SectionHandle,
			DebugPort,
			ExceptionPort);
	}
	else
	{
	 ZeroMemory(aProcessName,MAXPATHLEN);
	 __try
	 {
	 GetFullName(SectionHandle,aPathName);
	 }
	 __except (EXCEPTION_EXECUTE_HANDLER)
	 {
     }
	 GetProcessName2(aProcessName);
	 strcat(aProcessName,"##");

	 if (GoOrNot(aProcessName,aPathName))
	 {
		return ((ZWCREATEPROCESS)RealZwCreateProcess)(
			ProcessHandle,
			DesiredAccess,
			ObjectAttributes,
			ParentProcess,
			InheritObjectTable,
			SectionHandle,
			DebugPort,
			ExceptionPort);
	 }
	 else
	 {
		ProcessHandle = NULL;
		return RETURN_ERRO_NOBOX;
	 }
    }
}

NTSTATUS FakedZwCreateProcessEx(
							  OUT PHANDLE ProcessHandle,
							  IN ACCESS_MASK DesiredAccess,
							  IN POBJECT_ATTRIBUTES ObjectAttributes,
							  IN HANDLE InheritFromProcessHandle,
							  IN BOOLEAN InheritHandles,
							  IN HANDLE SectionHandle OPTIONAL,
							  IN HANDLE DebugPort OPTIONAL,
							  IN HANDLE ExceptionPort OPTIONAL,
							  IN HANDLE Unknown 
							  )
{
	char aProcessName[MAXPATHLEN];
	char aPathName[MAXPATHLEN];
	ULONG TempCurrentProcess;

	if (ProcessHandle==0) return RETURN_ERRO_NOBOX;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if (HipsCurrentProcess==TempCurrentProcess)
	{
		return ((ZWCREATEPROCESSEX)RealZwCreateProcessEx)(
			ProcessHandle,
			DesiredAccess,
			ObjectAttributes,
			InheritFromProcessHandle,
			InheritHandles,
			SectionHandle,
			DebugPort,
			ExceptionPort,
			Unknown
							  );
	}
	else
	{
	 ZeroMemory(aProcessName,MAXPATHLEN);
	 __try
	 {
	 GetFullName(SectionHandle,aPathName);
	 }
	 __except (EXCEPTION_EXECUTE_HANDLER)
	 {
	 }
	 GetProcessName2(aProcessName);
	 strcat(aProcessName,"##");



	 if (GoOrNot(aProcessName,aPathName))
	 {
		return ((ZWCREATEPROCESSEX)RealZwCreateProcessEx)(
			ProcessHandle,
			DesiredAccess,
			ObjectAttributes,
			InheritFromProcessHandle,
			InheritHandles,
			SectionHandle,
			DebugPort,
			ExceptionPort,
			Unknown
							  );
	 }
	 else
	 {
		ProcessHandle = NULL;
		//return STATUS_SUCCESS;
		return RETURN_ERRO_NOBOX;
	 }

	}
 return 0;
}

NTSTATUS FakedNtCreateUserProcess (PHANDLE ProcessHandle,
										PHANDLE ThreadHandle,
										PVOID Parameter2,
										PVOID Parameter3,
										PVOID ProcessSecurityDescriptor,
										PVOID ThreadSecurityDescriptor,
										PVOID Parameter6,
										PVOID Parameter7,
										PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
										PVOID Parameter9,
										PVOID pProcessUnKnow)
{
	char aProcessName[MAXPATHLEN];
	char aPathName[MAXPATHLEN];
	PWCHAR	wszFilePath ;
	ULONG TempCurrentProcess;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if (HipsCurrentProcess==TempCurrentProcess)
	{
	   return  ((NtCreateUserProcess)RealNtCreateUserProcess) (ProcessHandle,
										ThreadHandle,
										Parameter2,
										Parameter3,
										ProcessSecurityDescriptor,
										ThreadSecurityDescriptor,
										Parameter6,
										Parameter7,
										ProcessParameters,
										Parameter9,
										pProcessUnKnow);
	}
	else
	{
	 ZeroMemory(aProcessName,MAXPATHLEN);
	 __try
	 {
	 ConvertFileNameWCHARToCHAR(ProcessParameters->ImagePathName.Buffer,aPathName);
	 }
	 __except (EXCEPTION_EXECUTE_HANDLER)
	 {
     }
	 GetProcessName2(aProcessName);

	 strcat(aProcessName,"##");
	 if (GoOrNot(aProcessName,aPathName))
	 {
	   return  ((NtCreateUserProcess)RealNtCreateUserProcess) (ProcessHandle,
										ThreadHandle,
										Parameter2,
										Parameter3,
										ProcessSecurityDescriptor,
										ThreadSecurityDescriptor,
										Parameter6,
										Parameter7,
										ProcessParameters,
										Parameter9,
										pProcessUnKnow);
	 }
	 else
	 {
		ProcessHandle = NULL;
		return RETURN_ERRO_NOBOX;
	 }
	}
	return 0;
}


NTSTATUS GetUserSIDFromProcess(PEPROCESS pProcess, PCHAR pSID)
{
  NTSTATUS status;
  ULONG RetLen;
  HANDLE hToken;
  PTOKEN_USER tokenInfoBuffer;
  PACCESS_TOKEN Token;
  UNICODE_STRING  SidString;
  WCHAR           SidStringBuffer[256];

  Token = PsReferencePrimaryToken(pProcess);

  status = ObOpenObjectByPointer(Token, 0, NULL, TOKEN_QUERY, NULL, KernelMode, &hToken);
  ObDereferenceObject(Token);

  if(!NT_SUCCESS(status)) return FALSE;

  status = ZwQueryInformationToken(hToken, TokenUser, NULL, 0, &RetLen);
  if(status != STATUS_BUFFER_TOO_SMALL) {
    ZwClose(hToken);
    return FALSE;
  }

  tokenInfoBuffer = (PTOKEN_USER)ExAllocatePool(NonPagedPool, RetLen);
  if(tokenInfoBuffer)
      status = ZwQueryInformationToken(hToken, TokenUser, tokenInfoBuffer, RetLen, &RetLen);
 
  if(!NT_SUCCESS(status) || !tokenInfoBuffer ) {
	if(tokenInfoBuffer)
      ExFreePool(tokenInfoBuffer);
    ZwClose(hToken);
    return FALSE;
  }
  ZwClose(hToken);

   RtlZeroMemory( SidStringBuffer, sizeof(SidStringBuffer) );
   SidString.Buffer = (PWCHAR)SidStringBuffer;
   SidString.MaximumLength = sizeof( SidStringBuffer);

  status = RtlConvertSidToUnicodeString(&SidString, tokenInfoBuffer->User.Sid, FALSE);
  ExFreePool(tokenInfoBuffer);

  if(!NT_SUCCESS(status)) {
    return FALSE;
  }
  ConvertFileNameWCHARToCHAR(SidStringBuffer,pSID);
  return TRUE;
}


  
BOOLEAN IsImportanceRegPatch(PCHAR RegPatch)
{
  PCHAR nicmp=NULL;
  BOOLEAN Result=FALSE;

  nicmp= stristr(RegPatch,"Winlogon");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"BootExecute");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"run");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Objects");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"init");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"load");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Installed");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Policies");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Task");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Shellex");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Approved");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Internet Explorer");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Internet Settings");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Agent");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Security");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Clients");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Advanced");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Browser");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Safe");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"MountPoints2");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"homepage");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"\\shell\\open");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"Microsoft\\Ras");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"\\Net");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"\\Code");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"\\Protocols\\Filter");
  if (nicmp==NULL) nicmp=stristr(RegPatch,"\\Protocols\\Handler");

  if (nicmp!=NULL) Result=TRUE;
  return Result;
}										

NTSTATUS FakedZwSetValueKey
(
 IN HANDLE  KeyHandle,
 IN PUNICODE_STRING  ValueName,
 IN ULONG  TitleIndex  OPTIONAL,
 IN ULONG  Type,
 IN PVOID  Data,
 IN ULONG  DataSize
 )
{
	char pch[MAXPATHLEN];
	char regValue[MAXPATHLEN];
	ANSI_STRING ansi;
	char aProcessName[MAXPATHLEN];
	ULONG TempCurrentProcess;

	if (KeyHandle==0) return STATUS_SUCCESS;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();

	if (HipsCurrentProcess==TempCurrentProcess)
	{
	  return ((ZWSETVALUEKEY)RealZwSetValueKey)(
								 KeyHandle,
								 ValueName,
								 TitleIndex,
								 Type,
								 Data,
								 DataSize);
	}
	else
	{
	  __try
	  {
		  GetFullName2(KeyHandle,pch);
		  GetProcessName2(aProcessName);    //


			RtlUnicodeStringToAnsiString(&ansi,ValueName,TRUE);
			if(ansi.Length<MAXPATHLEN)
			{
				memcpy(regValue,ansi.Buffer,ansi.Length);
				regValue[ansi.Length]=0x00;
			}
			else
			{
				memcpy(regValue,ansi.Buffer,MAXPATHLEN);
				regValue[MAXPATHLEN-1]=0x00;
			}

			RtlFreeAnsiString(   &ansi   );
	  }
	  __except(EXCEPTION_EXECUTE_HANDLER)
	  {
	  }
		strcat(aProcessName,"$$");
		strcat(pch,"\\");
		strcat(pch,regValue);
		if (IsImportanceRegPatch(pch))
		{
		  if (GoOrNot(aProcessName,pch))
		  {
			 return ((ZWSETVALUEKEY)RealZwSetValueKey)(
								 KeyHandle,
								 ValueName,
								 TitleIndex,
								 Type,
								 Data,
								 DataSize);
		  }
		  else
		  {
		   return RETURN_ERRO_NOBOX;
		  }
		}
		else
		{
			 return ((ZWSETVALUEKEY)RealZwSetValueKey)(
								 KeyHandle,
								 ValueName,
								 TitleIndex,
								 Type,
								 Data,
								 DataSize);
        }
	}
}

NTSTATUS FakedZwSetSystemTime(PLARGE_INTEGER NewTime,PLARGE_INTEGER OldTime)
{
 return STATUS_SUCCESS;
}

NTSTATUS FakedZwLoadDriver(IN PUNICODE_STRING DriverServiceName )
{
	char aProcessName[MAXPATHLEN];
	char aDrvname[MAXPATHLEN];
	ANSI_STRING ansi ;
	ULONG TempCurrentProcess;

	if (DriverServiceName==NULL) return STATUS_SUCCESS;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if (HipsCurrentProcess==TempCurrentProcess)
    {
		return ((ZWLOADDRIVER)RealZwLoadDriver)(
								DriverServiceName
								);
	}
	else
	{
	  GetProcessName2(aProcessName);
	  __try
	  {
	  RtlUnicodeStringToAnsiString(&ansi,DriverServiceName,TRUE);
	  if(ansi.Length<MAXPATHLEN)
	  {
		memcpy(aDrvname,ansi.Buffer,ansi.Length);   
		aDrvname[ansi.Length]=0x00;   
	  }
	  else
	  {
		memcpy(aDrvname,ansi.Buffer,MAXPATHLEN);   
		aDrvname[MAXPATHLEN-1]=0x00;   
	  }
	
	  RtlFreeAnsiString(   &ansi   );
	  }
	  __except(1)
	  {
      }
	  strcat(aProcessName,"&&");
	  if (GoOrNot(aProcessName,aDrvname))
	  {
		return ((ZWLOADDRIVER)RealZwLoadDriver)(
								DriverServiceName
								);
	  }
	  else
	  {
			//return STATUS_ACCESS_DENIED;
			return RETURN_ERRO_NOBOX;
	  }
	}
}

NTSTATUS FakedNtSetSystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,IN OUT PVOID SystemInformation,IN ULONG SystemInformationLength)
{
	char aProcessName[MAXPATHLEN];
	char aDrvname[MAXPATHLEN];
	ULONG TempCurrentProcess;
	PSYSTEM_LOAD_AND_CALL_IMAGE newimage;

	if (SystemInformation=NULL) return STATUS_SUCCESS;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if ((HipsCurrentProcess==TempCurrentProcess) || (SystemInformationClass!=SystemLoadAndCallImage))
	{
		return ((NTSETSYSTEMINFORMATION)RealNtSetSystemInformation)(
								SystemInformationClass,
								SystemInformation,
								SystemInformationLength
								);
	}
	else
	{
	  GetProcessName2(aProcessName);
	  __try
	  {
	  newimage=(PSYSTEM_LOAD_AND_CALL_IMAGE)SystemInformation;
	  ConvertFileNameUNISTRToCHAR(&newimage->ModuleName,aDrvname);
	  }
	  __except(1)
	  {
      }
	  strcat(aProcessName,"&&");
	  if (GoOrNot(aProcessName,aDrvname))
	  {
		return ((NTSETSYSTEMINFORMATION)RealNtSetSystemInformation)(
								SystemInformationClass,
								SystemInformation,
								SystemInformationLength
								);
	  }
	  else
	  {
			return RETURN_ERRO_NOBOX;
	  }
    }
}

NTSTATUS FakedNtSystemDebugControl(IN DEBUG_CONTROL_CODE ControlCode,IN PVOID InputBuffer OPTIONAL,IN ULONG InputBufferLength,OUT PVOID OutputBuffer OPTIONAL,IN ULONG OutputBufferLength,OUT PULONG ReturnLength OPTIONAL)
{
	ULONG TempCurrentProcess;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if ((HipsCurrentProcess==TempCurrentProcess) || (ControlCode!=9))
	{
		return ((NtSystemDebugControl)RealNtSystemDebugControl)(
								ControlCode,
								InputBuffer,
								InputBufferLength,
								OutputBuffer,
								OutputBufferLength,
								ReturnLength
								);
	}
	else
	{
	   return RETURN_ERRO_NOBOX;
	}
}

NTSTATUS FakedNtOpenProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
)
{
	ULONG        PID;
	ULONG TempCurrentProcess;
	NTSTATUS    rc;


	TempCurrentProcess=(ULONG)PsGetCurrentProcess();

	rc = ((TypeNtOpenProcess)RealNtOpenProcess)( ProcessHandle, DesiredAccess,ObjectAttributes, ClientId );
	if (!NT_SUCCESS(rc )) return rc;
	if (HipsCurrentProcess!=TempCurrentProcess)
	{
		PID=(ULONG)ClientId->UniqueProcess;
		if (HipsCurrentProcessId==PID)
		{
		   ProcessHandle = NULL;
		   rc = STATUS_ACCESS_DENIED;
		}
	}
	return rc;
}


NTSTATUS FakedNtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
)
{
	ULONG        PID;
	ULONG TempCurrentProcess;
	NTSTATUS    rc;

	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	rc = ((TypeNtOpenThread)RealNtOpenThread)( ThreadHandle, DesiredAccess,ObjectAttributes, ClientId );
	if (!NT_SUCCESS(rc )) return rc;
	if (HipsCurrentProcess!=TempCurrentProcess)
	{
		PID=(ULONG)ClientId->UniqueProcess;
		if (HipsCurrentProcessId==PID)
		{
		   ThreadHandle = NULL;
		   rc = STATUS_ACCESS_DENIED;
		}
	}
	return rc;
}



NTSTATUS FakedNtWriteFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset OPTIONAL,
	IN PULONG Key OPTIONAL
	)
{
	char aFilename[MAXPATHLEN];
	int  nicmp;
	ULONG TempCurrentProcess;
	 if  (FileHandle==0)        return STATUS_SUCCESS;


	TempCurrentProcess=(ULONG)PsGetCurrentProcess();
	if (HipsCurrentProcess==TempCurrentProcess)
    {
	 return  ((NtWriteFile)RealNtWriteFile)(
										   FileHandle,
										   Event,
										   ApcRoutine,
										   ApcContext,
										   IoStatusBlock,
										   Buffer,
										   Length,
										   ByteOffset,
										   Key
										  );
	}
	__try
	{
	GetFullName(FileHandle,aFilename);
	}
	__except(1)
	{
    }
	nicmp=_strnicmp(aFilename,SafePatch,SafePatchLen);
	if (nicmp!=0)
	{
	 return  ((NtWriteFile)RealNtWriteFile)(
										   FileHandle,
										   Event,
										   ApcRoutine,
										   ApcContext,
										   IoStatusBlock,
										   Buffer,
										   Length,
										   ByteOffset,
										   Key
										  );
	}
	return STATUS_ACCESS_DENIED;
}

NTSTATUS FakedNtUserSetInformationThread(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	IN PVOID ThreadInformation,
	IN ULONG ThreadInformationLength
)
{

	if (ThreadInformationClass!=1)
	{
		return ((NtUserSetInformationThread)RealNtUserSetInformationThread)(
								ThreadHandle,
								ThreadInformationClass,
								ThreadInformation,
								ThreadInformationLength
								);
	}
	else
	{
	   return RETURN_ERRO_NOBOX;
	}
}



NTSTATUS
RainsafeDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
    UNICODE_STRING  UStrName;
	NTSTATUS status = STATUS_SUCCESS;
	PSERVICE_DESCRIPTOR_TABLE  ShadowSSDTTable;

	Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	SSDTTableBase=(ULONG)((PSYSTEM_SERVICE_TABLE)KeServiceDescriptorTable)->ServiceTable;
	ShadowSSDTTable=GetKeServiceDescriptorTableShadow();
	ShadowSSDTTableBase=(ULONG)(ShadowSSDTTable->win32k.ServiceTable);

	yourWinVer=GetWindowsVersion();
	RtlInitUnicodeString (&UStrName,L"IoVolumeDeviceToDosName" );
	if (yourWinVer==WINDOWS_VERSION_XP)
	{
	  RealIoVolumeDeviceToDosName=(ULONG)MmGetSystemRoutineAddress( &UStrName );
	  index_CreateProcess=-1;
	  index_CreateUserProcess=-1;
	  index_CreateProcessEx=0x30*4;    //ZwCreateProcessEx
	  index_SetValueKey=0xf7*4;         //ZwSetValueKey
	  index_LoadDriver=0x61*4;          // ZwLoadDriver
	  index_SetSystemTime=0xf2*4;     //ZwSetSystemTime
	  index_SetSystemInformation=0xf0*4;         //NTSETSYSTEMINFORMATION
	  index_SystemDebugControl=0xff*4;        //NtSystemDebugControl
	  index_OpenProcess=0x7a*4; //NtOpenProcess
	  index_OpenThread=0x80*4; //NtOpenThread
	  index_WriteFile=0x112*4; //NtWriteFile
	  index_UserSetInformationThread=0x208*4; //NtUserSetInformationThread ShadowTable
	}
	else
	if ((yourWinVer==WINDOWS_VERSION_2K3) || (yourWinVer==WINDOWS_VERSION_2K3_SP1_SP2))
	{
	  RealIoVolumeDeviceToDosName=(ULONG)MmGetSystemRoutineAddress( &UStrName );
	  index_CreateProcess=-1;
	  index_CreateUserProcess=-1;
	  index_CreateProcessEx=0x32*4;    //ZwCreateProcessEx
	  index_SetValueKey=0x100*4;         //ZwSetValueKey
	  index_LoadDriver=0x65*4;          // ZwLoadDriver
	  index_SetSystemTime=0xfb*4;     //ZwSetSystemTime
	  index_SetSystemInformation=0xf9*4;         //NTSETSYSTEMINFORMATION
	  index_SystemDebugControl=-1;            //NtSystemDebugControl
	  index_OpenProcess=0x80*4; //NtOpenProcess
	  index_OpenThread=0x86*4; //NtOpenThread
	  index_WriteFile=0x11c*4; //NtWriteFile
	  index_UserSetInformationThread=-1; //NtUserSetInformationThread ShadowTable
	}
	else
	if (yourWinVer==WINDOWS_VERSION_VISTA)
	{
	  RealIoVolumeDeviceToDosName=(ULONG)MmGetSystemRoutineAddress( &UStrName );
	  index_CreateUserProcess=0x185*4;  //NtCreateUserProcess
	  index_CreateProcess=-1;
	  index_CreateProcessEx=0x49*4;    //ZwCreateProcessEx
	  index_SetValueKey=0x148*4;         //ZwSetValueKey
	  index_LoadDriver=0xA5*4;          // ZwLoadDriver
	  index_SetSystemTime=0x143*4;     //ZwSetSystemTime
	  index_SetSystemInformation=0x141*4;         //NTSETSYSTEMINFORMATION
	  index_SystemDebugControl=-1;        //NtSystemDebugControl
	  index_OpenProcess=0xc2*4; //NtOpenProcess
	  index_OpenThread=0xc9*4; //NtOpenThread
	  index_WriteFile=0x167*4; //NtWriteFile
	  index_UserSetInformationThread=-1; //NtUserSetInformationThread ShadowTable
	}
	else
	if (yourWinVer==WINDOWS_VERSION_2K)
	{
	  RealIoVolumeDeviceToDosName=-1;
	  index_CreateUserProcess=-1;
	  index_CreateProcess=0x29*4;          //ZwCreateProcess
	  index_CreateProcessEx=-1;       //ZwCreateProcessEx
	  index_SetValueKey=0xd7*4;          //ZwSetValueKey
	  index_LoadDriver=0xe3*4;           // ZwLoadDriver
	  index_SetSystemTime=0xd2*4;     //ZwSetSystemTime
	  index_SetSystemInformation=0xd0*4;         //NTSETSYSTEMINFORMATION
	  index_SystemDebugControl=0xde*4;        //NtSystemDebugControl
	  index_OpenProcess=0x6a*4; //NtOpenProcess
	  index_OpenThread=0x6f*4; //NtOpenThread
	  index_WriteFile=0xed*4; //NtWriteFile
	  index_UserSetInformationThread=0x1f5*4; //NtUserSetInformationThread ShadowTable
    }
	else
	{
	  RealIoVolumeDeviceToDosName=-1;
	  index_CreateUserProcess=-1;
	  index_CreateProcess=-1;
	  index_CreateProcessEx=-1;       //ZwCreateProcessEx
	  index_SetValueKey=-1;          //ZwSetValueKey
	  index_LoadDriver=-1;           // ZwLoadDriver
	  index_SetSystemTime=-1;     //ZwSetSystemTime
	  index_SetSystemInformation=-1;         //NTSETSYSTEMINFORMATION
	  index_SystemDebugControl=-1;        //NtSystemDebugControl
	  index_OpenProcess=-1; //NtOpenProcess
	  index_OpenThread=-1; //NtOpenThread
	  index_WriteFile=-1; //NtWriteFile
	  index_UserSetInformationThread=-1; //NtUserSetInformationThread ShadowTable
    }
	return status;
}

NTSTATUS
RainsafeDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}



VOID ProcMoniterOn()
{
  if (index_CreateProcessEx!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_CreateProcessEx),(ULONG)FakedZwCreateProcessEx,&RealZwCreateProcessEx,&bProcMon);

  if (index_CreateProcess!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_CreateProcess),(ULONG)FakedZwCreateProcess,&RealZwCreateProcess,&bPrMon);

  if (index_CreateUserProcess!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_CreateUserProcess),(ULONG)FakedNtCreateUserProcess,&RealNtCreateUserProcess,&bUsPrMon);

}

VOID ProcMoniterOff()
{
 UnFakeAnyPro((PULONG)(SSDTTableBase + index_CreateProcessEx),RealZwCreateProcessEx,&bProcMon);
 UnFakeAnyPro((PULONG)(SSDTTableBase + index_CreateProcess),RealZwCreateProcess,&bPrMon);
 UnFakeAnyPro((PULONG)(SSDTTableBase + index_CreateUserProcess),RealNtCreateUserProcess,&bUsPrMon);
}

VOID RegMoniterOn()
{
 if (index_SetValueKey!=-1)
 FakeAnyPro((PULONG)(SSDTTableBase + index_SetValueKey),(ULONG)FakedZwSetValueKey,&RealZwSetValueKey,&bRegMon);
}

VOID RegMoniterOff()
{
 UnFakeAnyPro((PULONG)(SSDTTableBase + index_SetValueKey),RealZwSetValueKey,&bRegMon);
}

VOID ModMonitorOn()
{
  if (index_LoadDriver!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_LoadDriver),(ULONG)FakedZwLoadDriver,&RealZwLoadDriver,&bModMon);
  if (index_SetSystemInformation!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_SetSystemInformation),(ULONG)FakedNtSetSystemInformation,&RealNtSetSystemInformation,&bSsiMon);

}

VOID ModMonitorOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_LoadDriver),RealZwLoadDriver,&bModMon);
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_SetSystemInformation),RealNtSetSystemInformation,&bSsiMon);
}

VOID TimeSafeOn()
{
  if (index_SetSystemTime!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_SetSystemTime),(ULONG)FakedZwSetSystemTime,&RealZwSetSystemTime,&bTimeMon);
}

VOID TimeSafeOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_SetSystemTime),RealZwSetSystemTime,&bTimeMon);
}


VOID NtSystemDebugOn()
{
  if (index_SystemDebugControl!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_SystemDebugControl),(ULONG)FakedNtSystemDebugControl,&RealNtSystemDebugControl,&bsdc);

}
VOID NtSystemDebugOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_SystemDebugControl),RealNtSystemDebugControl,&bsdc);
}

VOID NtOpenProOn()
{
  if (index_OpenProcess!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_OpenProcess),(ULONG)FakedNtOpenProcess,&RealNtOpenProcess,&bopr);

}
VOID NtOpenProOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_OpenProcess),RealNtOpenProcess,&bopr);
}

VOID NtOpenThrOn()
{
  if (index_OpenThread!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_OpenThread),(ULONG)FakedNtOpenThread,&RealNtOpenThread,&botr);

}
VOID NtOpenThrOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_OpenThread),RealNtOpenThread,&botr);
}

VOID NtWriteFileHookOn()
{
  if (index_WriteFile!=-1)
  FakeAnyPro((PULONG)(SSDTTableBase + index_WriteFile),(ULONG)FakedNtWriteFile,&RealNtWriteFile,&bWriteFile);
}

VOID NtWriteFileHookOff()
{
  UnFakeAnyPro((PULONG)(SSDTTableBase + index_WriteFile),RealNtWriteFile,&bWriteFile);
}

VOID NtUserSetInformationThreadHookOn()
{
  ULONG OOXX;
	if (index_UserSetInformationThread!=-1)
	FakeAnyPro((PULONG)(ShadowSSDTTableBase + index_UserSetInformationThread),(ULONG)FakedNtUserSetInformationThread,&RealNtUserSetInformationThread,&bUsit);

}

VOID NtUserSetInformationThreadHookOff()
{
  UnFakeAnyPro((PULONG)(ShadowSSDTTableBase + index_UserSetInformationThread),RealNtUserSetInformationThread,&bUsit);
}

NTSTATUS
RainsafeDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS			status = STATUS_SUCCESS;
    PIO_STACK_LOCATION	irpStack;
    PDEVICE_EXTENSION	deviceExtension;
    PVOID				ioBuf;
    ULONG				inBufLength, outBufLength;
	ULONG				ioControlCode;
	UCHAR               *buff =0;
	ULONG               a;
	ULONG               HELLOReturn;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	Irp->IoStatus.Information = 0;
    ioBuf = Irp->AssociatedIrp.SystemBuffer;
	inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ioControlCode)
    {
	case IOCTL_RAINSAFE_HELLO:      //驱动通讯测试看是否成功返回770 判断驱动正常运转
		{
			buff=(UCHAR *)Irp->AssociatedIrp.SystemBuffer ;
			HELLOReturn=770;
			memmove(&buff[0],&HELLOReturn,4);
			Irp->IoStatus.Information = 4;
            break;
		}

	case 1000:
		{
		  if(!bProcMon)
		  {
			ProcMoniterOn();
          }
		  break;
		}
	case 1001:
		{
			if (bProcMon)
			{
				ProcMoniterOff();
			}
		  break;
		}
	case 1002:
		{
			if (!bRegMon)
			{
				RegMoniterOn();
			}
			break;
		}

	case 1003:
		{
			if (bRegMon)
			{
				RegMoniterOff();
			}
			break;
		}
	case 1004:
		{
			if (!bModMon)
			{

				ModMonitorOn();
			}
			break;
		}

	case 1005:
		{
			if (bModMon)
			{
				ModMonitorOff();
			}
			break;
		}

	case 1006:
		{
			if (!bTimeMon)
			{
				TimeSafeOn();
			}
			break;
		}

	case 1007:
		{
			if (bTimeMon)
			{
				TimeSafeOff();
			}
			break;
		}
	case 1008:
		{
			if (!bsdc)
			{
				NtSystemDebugOn();
			}
			if (!bopr)
			{
				NtOpenProOn();
			}
			if (!botr)
			{
				NtOpenThrOn();
			}
			if (!bWriteFile)
			{
				NtWriteFileHookOn();
			}
			if (!bUsit)
			{
				NtUserSetInformationThreadHookOn();
			}
			break;
		}
	case 1009:
		{
			if (bsdc)
			{
				NtSystemDebugOff();
			}
			if (bopr)
			{
				NtOpenProOff();
			}
			if (botr)
			{
				NtOpenThrOff();
			}
			if (bWriteFile)
			{
				NtWriteFileHookOff();
			}
			if (bUsit)
			{
				NtUserSetInformationThreadHookOff();
			}
			break;
		}
	 case 2000:                         //传入需要保护的文件夹
		 {
				buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ;
				memmove(&SafePatchLen,&buff[0],4);
				ZeroMemory(SafePatch,MAXPATHLEN);
				memmove(SafePatch,&buff[4],MAXPATHLEN);
				break;
		 }
	 case 2001:                         //传入共享内存
		 {
				buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ;
				memmove(&a,&buff[4],4);
				output=(char*)MmMapIoSpace(MmGetPhysicalAddress((void*)a),256,0);
				break;
		 }
	 case 2002:                         //传入信号灯
		 {
				buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ;
				memmove(&Appevent,&buff[0],4);
				memmove(&Sysevent,&buff[4],4);
				ObReferenceObjectByHandle(Appevent,GENERIC_ALL,NULL,KernelMode,&AppeventObject,NULL);
				ObReferenceObjectByHandle(Sysevent,GENERIC_ALL,NULL,KernelMode,&SyseventObject,NULL);
				break;
		 }
	 case 2003:                         //传入ring3进程信息
		 {
				  HipsCurrentProcess=(ULONG)PsGetCurrentProcess();
				  HipsCurrentProcessId=(ULONG)PsGetCurrentProcessId();
				break;
		 }
	 case 2004:                         //解除所有构子
		 {
				  UnHOOkAll();
				break;
		 }
	default:
        status = STATUS_INVALID_PARAMETER;
		break;
	}
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
VOID UnHOOkAll()
{
	ProcMoniterOff();
	RegMoniterOff();
	ModMonitorOff();
	TimeSafeOff();
	NtSystemDebugOff();
	NtOpenProOff();
	NtOpenThrOff();
	NtWriteFileHookOff();
	NtUserSetInformationThreadHookOff();
}

VOID
RainsafeUnload(
	IN PDRIVER_OBJECT		DriverObject
	)
{
	UNICODE_STRING dosDeviceName;
    UnHOOkAll();
	NtUserSetInformationThreadHookOff();
	RtlInitUnicodeString(&dosDeviceName, RAINSAFE_DOS_DEVICE_NAME_W);
    IoDeleteSymbolicLink(&dosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);
}