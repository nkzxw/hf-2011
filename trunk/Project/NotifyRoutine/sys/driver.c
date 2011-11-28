//#include "ntddk.h"
#include <stdio.h>
#include <stdlib.h>
#include "ntifs.h"

#include "windef.h"
#include "define.h"
#include <ntimage.h>

#define SYSNAME "System"
#define VERSIONLEN 100

const WCHAR devLink[]  = L"\\??\\MyEvent";
const WCHAR devName[]  = L"\\Device\\MyEvent";
UNICODE_STRING          devNameUnicd;
UNICODE_STRING          devLinkUnicd;    
PVOID                    gpEventObject = NULL;            // 与应用程序通信的 Event 对象
ULONG                    ProcessNameOffset =0;
PVOID                    outBuf[255];
BOOL                    g_bMainThread; 
ULONG                    g_dwParentId;
CHECKLIST                CheckList;
ULONG                    BuildNumber;                    //系统版本号                    
ULONG                    SYSTEMID;                    //System进程的ID
PWCHAR                    Version[VERSIONLEN];

ULONG					g_ulPid;
HANDLE					g_psaveDes;
PEPROCESS				g_eprocess;
ULONG						oldCr0;

//NTSTATUS PsLookupProcessByProcessId(IN ULONG ulProcId, OUT PEPROCESS * pEProcess);

UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);  

VOID WPOFF ()
{
	  __asm {
		cli;
		mov eax, cr0;
		mov oldCr0, eax;
		and eax, not 10000h;
		mov cr0, eax
	  }
}

VOID WPON ()
{
      __asm {
        mov eax, oldCr0;
        mov cr0, eax;
        sti;
      }
}

ULONG GetProcessNameOffset()
{
    PEPROCESS curproc;
    int i;

    curproc = PsGetCurrentProcess();

    for( i = 0; i < 3*PAGE_SIZE; i++ ) 
    {
        if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) )) 
        {
            return i;
        }
    }

    return 0;
}

NTSTATUS GetRegValue(PCWSTR RegPath,PCWSTR ValueName,PWCHAR Value)
{
    int ReturnValue = 0;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfoP;
    ULONG valueInfoLength,returnLength;
    UNICODE_STRING UnicodeRegPath;
    UNICODE_STRING UnicodeValueName;

    RtlInitUnicodeString(&UnicodeRegPath, RegPath);
    RtlInitUnicodeString(&UnicodeValueName, ValueName);

    InitializeObjectAttributes(&ObjectAttributes,
        &UnicodeRegPath,
        OBJ_CASE_INSENSITIVE, // Flags
        NULL, // Root directory
        NULL); // Security descriptor

    Status = ZwOpenKey(&KeyHandle,
        KEY_ALL_ACCESS,
        &ObjectAttributes);
    if (Status != STATUS_SUCCESS)
    {
        DbgPrint("ZwOpenKey Wrong\n");
        return 0;
    }

    valueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION)+VERSIONLEN;
    valueInfoP =    (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool
                                                    (NonPagedPool, valueInfoLength);
    Status = ZwQueryValueKey(KeyHandle,
        &UnicodeValueName,
        KeyValuePartialInformation,
        valueInfoP,
        valueInfoLength,
        &returnLength);

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwQueryValueKey Wrong:%08x\n",Status);
        return Status;
    }
    else
    {
        RtlCopyMemory((PCHAR)Value, (PCHAR)valueInfoP->Data, valueInfoP->DataLength);
        ReturnValue = 1;
    }

    if(!valueInfoP);
        ExFreePool(valueInfoP);
    ZwClose(KeyHandle);
    return ReturnValue;
}

VOID ThreadCreateMon (
	IN HANDLE PId, 
	IN HANDLE TId, 
	IN BOOLEAN  bCreate)
{

    PEPROCESS   EProcess,PEProcess;
    NTSTATUS    status;
    HANDLE        dwParentPID;
	CHAR *pName;   //进程名

    status = PsLookupProcessByProcessId(PId, &EProcess);
    if (!NT_SUCCESS( status ))
    {
        //DbgPrint("PsLookupProcessByProcessId()\n");
        return ;
    }    
	
	pName = (CHAR*)PsGetProcessImageFileName(EProcess); //获取进程名
	DbgPrint ("ThreadCreateMon PId = %d, TId = %d, name = %s.\n", PId, TId, pName);

	ObDereferenceObject(EProcess); 

    if ( bCreate )
    {
        dwParentPID = PsGetCurrentProcessId();
        status = PsLookupProcessByProcessId(dwParentPID, &PEProcess);
        if (!NT_SUCCESS( status ))
        {
            //DbgPrint("PsLookupProcessByProcessId()\n");
            return ;
        }

		ObDereferenceObject(PEProcess); 

        if((ULONG)PId == 4)    //System进程创建的东东我们不管
            return;
            
        if((g_bMainThread==TRUE)
            &&(g_dwParentId != (ULONG)dwParentPID)
            &&(dwParentPID != PId)
            )
        {
            g_bMainThread=FALSE;
            if(gpEventObject!=NULL)
                KeSetEvent((PRKEVENT)gpEventObject, 0, FALSE);
        }
        if(CheckList.ONLYSHOWREMOTETHREAD)    //只显示远线程
           return;
        if(gpEventObject!=NULL)
            KeSetEvent((PRKEVENT)gpEventObject, 0, FALSE);
    }
    else if(CheckList.SHOWTERMINATETHREAD)
    {
        DbgPrint( "TERMINATED == THREAD ID: %d\n", TId);
        if(gpEventObject!=NULL)
            KeSetEvent((PRKEVENT)gpEventObject, 0, FALSE);
    }
}


VOID ProcessCreateMon ( 
	HANDLE hParentId, 
	HANDLE PId, 
	BOOLEAN bCreate
	)
{
    PEPROCESS  EProcess, PProcess;
    NTSTATUS	status;
    HANDLE	TId;
	CHAR *pCName;   //进程名
	CHAR *pPName;

    g_dwParentId = (ULONG)hParentId;
    status = PsLookupProcessByProcessId(PId, &EProcess);
    if (!NT_SUCCESS( status ))
    {
        //DbgPrint("PsLookupProcessByProcessId()\n");
        return ;
    }

	pPName = (CHAR*)PsGetProcessImageFileName(EProcess); //获取进程名
	DbgPrint ("ProcessCreateMon hParentId = %d, PId = %d, name = %s.\n", hParentId, PId, pPName);

	ObDereferenceObject(EProcess);

    status = PsLookupProcessByProcessId(hParentId, &PProcess);
    if (!NT_SUCCESS( status ))
    {
        //DbgPrint("PsLookupProcessByProcessId()\n");
        return ;
    }

	pCName = (CHAR*)PsGetProcessImageFileName(EProcess); //获取进程名
	DbgPrint ("ProcessCreateMon hParentId = %d, PId = %d, name = %s.\n", hParentId, PId, pCName);

	ObDereferenceObject(PProcess); 

    if ( bCreate )
    {
        g_bMainThread = TRUE;
        if(gpEventObject!=NULL)
            KeSetEvent((PRKEVENT)gpEventObject, 0, FALSE);
    }
    else if(CheckList.SHOWTERMINATEPROCESS)
    {
        //DbgPrint( "TERMINATED == PROCESS ID: %d\n", PId);
      if(gpEventObject!=NULL)
          KeSetEvent((PRKEVENT)gpEventObject, 0, FALSE);
    }
}

VOID
LoadImageMon(
    IN PUNICODE_STRING  FullImageName,
    IN HANDLE  ProcessId, // where image is mapped
    IN PIMAGE_INFO  ImageInfo
    )
{
	CHAR *pName;   //进程名
	NTSTATUS status;
	PEPROCESS EProcess;

	status = PsLookupProcessByProcessId(ProcessId, &EProcess);
    if (!NT_SUCCESS( status ))
    {
        //DbgPrint("PsLookupProcessByProcessId()\n");
        return ;
    }

	pName = (CHAR*)PsGetProcessImageFileName(EProcess); //获取进程名
	DbgPrint ("LoadImageMon  image = %wZ, id = %d, name = %s.\n", FullImageName, ProcessId, pName);

	ObDereferenceObject(EProcess); 
}

NTSTATUS OnUnload( IN PDRIVER_OBJECT pDriverObject )
{
    NTSTATUS   status;
    if(gpEventObject)
      ObDereferenceObject(gpEventObject); 

    PsSetCreateProcessNotifyRoutine(ProcessCreateMon, TRUE);
	PsRemoveCreateThreadNotifyRoutine (ThreadCreateMon);
	PsRemoveLoadImageNotifyRoutine (LoadImageMon);

    if(pDriverObject->DeviceObject != NULL)
    {
      status=IoDeleteSymbolicLink( &devLinkUnicd );
      if (!NT_SUCCESS(status))
      {
        //DbgPrint((  "IoDeleteSymbolicLink() failed\n" ));
        return status; 
      }
      IoDeleteDevice( pDriverObject->DeviceObject );
    }
    return STATUS_SUCCESS;
}

NTSTATUS DeviceIoControlDispatch(
	IN  PDEVICE_OBJECT  DeviceObject,
	IN  PIRP            pIrp
	)
{
    PIO_STACK_LOCATION              irpStack;
    NTSTATUS                        status;
    PVOID                           inputBuffer;
    ULONG                           inputLength;
    PVOID                           outputBuffer;
    ULONG                           outputLength;
    OBJECT_HANDLE_INFORMATION        objHandleInfo;

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation(pIrp);
    switch (irpStack->MajorFunction)
    {
    case IRP_MJ_CREATE :
        //DbgPrint("Call IRP_MJ_CREATE\n");
        break;
    case IRP_MJ_CLOSE:
        //DbgPrint("Call IRP_MJ_CLOSE\n");
        break;
    case IRP_MJ_DEVICE_CONTROL:
        //DbgPrint("IRP_MJ_DEVICE_CONTROL\n");
        inputLength=irpStack->Parameters.DeviceIoControl.InputBufferLength;
        outputLength=irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) 
        {
        case IOCTL_PASSEVENT:    //用事件做通信
            inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
            //DbgPrint("inputBuffer:%08x\n", (HANDLE)inputBuffer);
            status = ObReferenceObjectByHandle(*(HANDLE *)inputBuffer,
                GENERIC_ALL,
                NULL,
                KernelMode,
                &gpEventObject,
                &objHandleInfo);

            if(status!=STATUS_SUCCESS)
            {
                //DbgPrint("wrong\n");
                break;
            }
            break;
        case IOCTL_UNPASSEVENT:
            if(gpEventObject)
                ObDereferenceObject(gpEventObject); 
            //DbgPrint("UNPASSEVENT called\n");
            break;
        case IOCTL_PASSBUF:
            RtlCopyMemory(pIrp->UserBuffer, outBuf, outputLength);
            break;
        case IOCTL_PASSEVSTRUCT:
            inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
            memset(&CheckList, 0, sizeof(CheckList));
            RtlCopyMemory(&CheckList, inputBuffer, sizeof(CheckList));
            //DbgPrint("%d:%d\n", CheckList.ONLYSHOWREMOTETHREAD, CheckList.SHOWTHREAD);
            break;
        default:
            break;
        }
        break;
    default:
        //DbgPrint("Call IRP_MJ_UNKNOWN\n");
        break;
    }

    pIrp->IoStatus.Status = status; 
    pIrp->IoStatus.Information = 0; 
    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    return status;
}


VOID InjectDll (
      IN PUNICODE_STRING  FullImageName,
      IN HANDLE  ProcessId, // where image is mapped
      IN PIMAGE_INFO  ImageInfo
      )
{
	NTSTATUS ntStatus;
	PIMAGE_IMPORT_DESCRIPTOR pImportNew;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_DOS_HEADER pDos;
	PIMAGE_NT_HEADERS	pHeader = NULL; 
	HANDLE hProcessHandle = NULL;
	int nImportDllCount = 0;
	int size;
	ULONG 	ulBaseImage = 0;
	BOOLEAN bAttached = FALSE;
	KAPC_STATE apcState;
	PVOID lpBuffer;
	PVOID lpDllName;
	PVOID lpExportApi;
	PVOID lpTemp;
	PVOID lpTemp2;
	//NTSTATUS ntStatus;


	//
	// PID改变时，可以说明已经注入DLL了。
	//
	if(g_ulPid != (ULONG)ProcessId && 
	 g_ulPid != 0 && g_psaveDes != NULL)
	{

		
		if(g_psaveDes != NULL)
		{
			if(PsGetCurrentProcess() != g_eprocess) 
			{
				KeStackAttachProcess((PRKPROCESS)g_eprocess, &apcState);
				bAttached = TRUE;
			}
				
			WPOFF ();

			// 改导出表
			pHeader->OptionalHeader.DataDirectory[1].Size -= sizeof(IMAGE_IMPORT_DESCRIPTOR);
			pHeader->OptionalHeader.DataDirectory[1].VirtualAddress = (ULONG)g_psaveDes - ulBaseImage;

			WPON ();

			g_psaveDes = NULL;

			if(bAttached)
				KeUnstackDetachProcess(&apcState);
		}

		 return ;
	}

	//
	// 注入进程没退出。
	//
	if(g_eprocess != NULL)
	  return;

	if(_stricmp(PsGetProcessImageFileName(PsGetCurrentProcess()), "dbgview.exe") == 0) 
	{
		g_eprocess = PsGetCurrentProcess();
		g_ulPid = (ULONG )ProcessId;
		
		ulBaseImage = (ULONG)ImageInfo->ImageBase;// 进程基地址
		
		pDos =(PIMAGE_DOS_HEADER) ulBaseImage;
		pHeader = (PIMAGE_NT_HEADERS)(ulBaseImage+(ULONG)pDos->e_lfanew);
		pImportDesc  = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG)pHeader->OptionalHeader.DataDirectory[1].VirtualAddress + ulBaseImage);
		
		//
		// 导入DLL个数    
		//
		nImportDllCount = pHeader->OptionalHeader.DataDirectory[1].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);

		//
		// 把原始值保存。
		//
		g_psaveDes = pImportDesc;

		ntStatus = ObOpenObjectByPointer(g_eprocess, OBJ_KERNEL_HANDLE, NULL, 0x008, //PROCESS_VM_OPERATION
	  																 NULL, KernelMode, &hProcessHandle);

		if(!NT_SUCCESS(ntStatus))  
	  	return ;

		//
		// 加上一个自己的结构。
		//
		size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (nImportDllCount + 1);

		//
		//分配导入表
		//
		ntStatus = ZwAllocateVirtualMemory(hProcessHandle, &lpBuffer, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!NT_SUCCESS(ntStatus)) {
		  ZwClose(hProcessHandle);
		  return ;
		}
		RtlZeroMemory(lpBuffer,sizeof(IMAGE_IMPORT_DESCRIPTOR) * (nImportDllCount + 1));

		size = 20;

		//
		// 分配当前进程空间。
		//
		ntStatus = ZwAllocateVirtualMemory(hProcessHandle, &lpDllName, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!NT_SUCCESS(ntStatus)) {
		  ZwClose(hProcessHandle);
		  return ;
		}
		RtlZeroMemory(lpDllName,20);

		/* 分配导出函数的进程地址空间。
		  注意这里是分配高位地址的。如分配低位地址，得到PIMAGE_IMPORT_BY_NAME结构的地址会以，如ff等开头的负数地址，
		  这样，系统就会默认按序号查找API地址，会弹出找不到序号的框框。
		*/

		size = 20;
		ntStatus = ZwAllocateVirtualMemory(hProcessHandle, &lpExportApi, 0, &size, MEM_COMMIT|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
		if(!NT_SUCCESS(ntStatus)) {
		  ZwClose(hProcessHandle);
		  return ;
		}
		RtlZeroMemory(lpExportApi,20);


		// 分配当前进程空间。
		size = 20;
		ntStatus = ZwAllocateVirtualMemory(hProcessHandle, &lpTemp, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!NT_SUCCESS(ntStatus)) {
		  ZwClose(hProcessHandle);
		  return ;
		}
		RtlZeroMemory(lpTemp,20);

		//
		// 分配当前进程空间。
		size = 20;
		ntStatus = ZwAllocateVirtualMemory(hProcessHandle, &lpTemp2, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!NT_SUCCESS(ntStatus)) {
		  ZwClose(hProcessHandle);
		  return ;
		}
		RtlZeroMemory(lpTemp2,20);
		
		pImportNew = lpBuffer;

		// 把原来数据保存好。
		RtlCopyMemory(pImportNew+1, pImportDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR) * nImportDllCount );


		//
		// 构造自己的DLL   
		// IMAGE_IMPORT_DESCRIPTOR结构
		//
		{
		  IMAGE_IMPORT_DESCRIPTOR Add_ImportDesc;
		  PULONG ulAddress;
		  ULONG oldCr0;
		  ULONG Func;
		  PIMAGE_IMPORT_BY_NAME ptmp;
		  IMAGE_THUNK_DATA  *pThunkData=(PIMAGE_THUNK_DATA)lpTemp2;
		
		  //
		  // ThunkData结构的地址
		  //
		  ptmp  =  (PIMAGE_IMPORT_BY_NAME)lpExportApi;
		  ptmp->Hint=0;
		  RtlCopyMemory(ptmp->Name,"ExportedFunction",18);
		
		  *(PULONG)lpTemp =(DWORD)ptmp-ulBaseImage;
		  pThunkData->u1.AddressOfData  =(DWORD)ptmp-ulBaseImage;
		  Add_ImportDesc.Characteristics = (DWORD)pThunkData-ulBaseImage;
		
		  Add_ImportDesc.TimeDateStamp = 0;
		  Add_ImportDesc.ForwarderChain = 0;
		
		  //
		  // DLL名字的RVA
		  //    
		  RtlCopyMemory(lpDllName,"mydll.dll",20);
		  Add_ImportDesc.Name = (DWORD)lpDllName-ulBaseImage;
		  Add_ImportDesc.FirstThunk = Add_ImportDesc.Characteristics;
		
		
		  RtlCopyMemory(pImportNew, &Add_ImportDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		
			WPOFF ();
			
		  // 改导出表
		  pHeader->OptionalHeader.DataDirectory[1].Size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		  pHeader->OptionalHeader.DataDirectory[1].VirtualAddress = (ULONG)pImportNew - ulBaseImage;
		
			WPON ();
		}
		
		ZwClose(hProcessHandle);
		hProcessHandle = NULL;
	}
}

NTSTATUS DriverEntry( 
	IN PDRIVER_OBJECT pDriverObject, 
	IN PUNICODE_STRING theRegistryPath 
	)
{
    NTSTATUS		Status;    
    PDEVICE_OBJECT	pDevice;

    g_bMainThread = FALSE;

    if(1 != GetRegValue(L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
    									L"CSDVersion", 
    									(PWCHAR)Version))
    {
        DbgPrint("GetRegValue Wrong\n");
    }
    
    
    PsGetVersion(NULL, NULL, &BuildNumber, NULL);
    RtlInitUnicodeString (&devNameUnicd, devName );
    RtlInitUnicodeString (&devLinkUnicd, devLink );

    Status = IoCreateDevice ( pDriverObject,
					        0,
					        &devNameUnicd,
					        FILE_DEVICE_UNKNOWN,
					        0,
					        TRUE,
					        &pDevice );
    if( !NT_SUCCESS(Status)) 
    {
        return Status;
    }

    Status = IoCreateSymbolicLink (&devLinkUnicd, &devNameUnicd);
    if( !NT_SUCCESS(Status)) 
    {
        DbgPrint(("IoCreateSymbolicLink Error.\n"));
        IoDeleteDevice (pDevice);
        return Status;
    }

    //ProcessNameOffset = GetProcessNameOffset();

    pDriverObject->DriverUnload  = OnUnload; 
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = 
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] =
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlDispatch;

    Status = PsSetCreateProcessNotifyRoutine(ProcessCreateMon, FALSE);
    if (!NT_SUCCESS( Status ))
    {
        DbgPrint("PsSetCreateProcessNotifyRoutine() Error.\n");
        return Status;
    }

    Status = PsSetCreateThreadNotifyRoutine(ThreadCreateMon);
    if (!NT_SUCCESS( Status ))
    {
        DbgPrint("PsSetCreateThreadNotifyRoutine() Error.\n");
        return Status;
    }

	Status = PsSetLoadImageNotifyRoutine (LoadImageMon);
	if (!NT_SUCCESS (Status))
	{
		DbgPrint("PsSetLoadImageNotifyRoutine() Error.\n");
		return Status;
	}

    return STATUS_SUCCESS;
}