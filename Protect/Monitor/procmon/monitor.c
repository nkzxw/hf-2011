#include "ntddk.h"
#include "string.h"

#define ProcessNameOffset  0x1fc

static NTSTATUS  MydrvDispatch (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS DriverEntry( 
	IN PDRIVER_OBJECT DriverObject,  
	IN PUNICODE_STRING RegistryPath
	) 
{ 
	UNICODE_STRING  nameString, linkString;
	PDEVICE_OBJECT  deviceObject;
	NTSTATUS status;
	int	i;
    

  //建立设备
  RtlInitUnicodeString( &nameString, L"\\Device\\WssProcMon" );
    
  status = IoCreateDevice(DriverObject,
													0,
													&nameString,
													FILE_DEVICE_UNKNOWN,
													0,
													TRUE,
													&deviceObject);                           
  if (!NT_SUCCESS(status)){
      return status;
  }
   
  RtlInitUnicodeString( &linkString, L"\\DosDevices\\WssProcMon" );
  status = IoCreateSymbolicLink (&linkString, &nameString);
	if (!NT_SUCCESS(status))
	{
	  IoDeleteDevice (DriverObject->DeviceObject);
	  return status;
	}    
    
	status = PsSetLoadImageNotifyRoutine(ImageCreateMon);
	if (!NT_SUCCESS( status ))
	{
    DbgPrint("PsSetLoadImageNotifyRoutine()\n");
    return status;
	}    

  status = PsSetCreateThreadNotifyRoutine(ThreadCreateMon);
  if (!NT_SUCCESS( status ))
  {
    DbgPrint("PsSetCreateThreadNotifyRoutine()\n");
    return status;
  }    

  status = PsSetCreateProcessNotifyRoutine(ProcessCreateMon, FALSE);
  if (!NT_SUCCESS( status ))
  {
    DbgPrint("PsSetCreateProcessNotifyRoutine()\n");
    return status;
  }    
  
	for ( i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)    {	
    DriverObject->MajorFunction[i] = MydrvDispatch;
	}
     
  return STATUS_SUCCESS; 
} 

//处理设备对象操作
static NTSTATUS MydrvDispatch (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{ 
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest( Irp, 0 );
    return Irp->IoStatus.Status;
    
}

VOID ProcessCreateMon ( 
	IN HANDLE hParentId, 
	IN HANDLE PId,
	IN BOOLEAN bCreate )
{
	PEPROCESS  EProcess;
	ULONG      ulCurrentProcessId;
	LPTSTR       lpCurProc;
	NTSTATUS   status;

	status = PsLookupProcessByProcessId( (ULONG)PId, &EProcess);
	if (!NT_SUCCESS( status ))
	{
    DbgPrint("PsLookupProcessByProcessId()\n");
    return ;
	}
    
	if (bCreate)
	{
		lpCurProc = (LPTSTR)EProcess;
		lpCurProc = lpCurProc + ProcessNameOffset;
		
		DbgPrint( "CREATE PROCESS = PROCESS NAME: %s , PROCESS PARENTID: %d, PROCESS ID: %d, PROCESS ADDRESS %x:\n", lpCurProc, hParentId, PId, EProcess );
	}	 
	else
	{
		DbgPrint( "TERMINATED == PROCESS ID: %d\n", PId);
	}
}

VOID ThreadCreateMon (
	IN HANDLE PId, 
	IN HANDLE TId, 
	IN BOOLEAN  bCreate)
{
	PEPROCESS   EProcess;
	ULONG        ulCurrentProcessId;
	LPTSTR        lpCurProc;
	NTSTATUS    status;

	status = PsLookupProcessByProcessId((ULONG)PId, &EProcess);
	if (!NT_SUCCESS( status ))
	{
    DbgPrint("PsLookupProcessByProcessId()\n");
    return ;
	}    

	if ( bCreate )
	{
	  lpCurProc    = (LPTSTR)EProcess;
	  lpCurProc    = lpCurProc + ProcessNameOffset;
	
	  DbgPrint( "CREATE THREAD = PROCESS NAME: %s PROCESS ID: %d, THREAD ID: %d\n", lpCurProc, PId, TId );                              
	}     
  else
  {
    DbgPrint( "TERMINATED == THREAD ID: %d\n", TId);
  }

}

VOID ImageCreateMon (
	IN PUNICODE_STRING  FullImageName, 
	IN HANDLE  ProcessId, 
	IN PIMAGE_INFO  ImageInfo 
	)
{
	DbgPrint("FullImageName: %S,Process ID: %d\n",FullImageName->Buffer, ProcessId);
	DbgPrint("ImageBase: %x,ImageSize: %d\n",ImageInfo->ImageBase, ImageInfo->ImageSize);
}