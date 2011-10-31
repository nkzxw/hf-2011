#include <ntddk.h> 
#include "ntimage.h"
#include <windef.h>
#include "InlineHook.h"

#include "../include/engine.h"

extern void	AdjustStackCallPointer();

HOOK_INFO  hi_NtCreateProcessEx;
HOOK_INFO  hi_KeBugCheckEx;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

ULONG GetFunctionAddr( 
	IN PCWSTR FunctionName
	)
{
    UNICODE_STRING UniCodeFunctionName;
    RtlInitUnicodeString( &UniCodeFunctionName, FunctionName );
    return (ULONG)MmGetSystemRoutineAddress( &UniCodeFunctionName );   
}

typedef 
NTSTATUS 
(__stdcall *LPFUN_NTCREATEPROCESSEX)(
	OUT PHANDLE ProcessHandle, 
	IN ACCESS_MASK DesiredAccess, 
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, 
	IN HANDLE ParentProcess, 
	IN BOOLEAN InheritObjectTable, 
	IN HANDLE SectionHandle OPTIONAL, 
	IN HANDLE DebugPort OPTIONAL, 
	IN HANDLE ExceptionPort OPTIONAL, 
	IN BOOLEAN InJob); 

NTSTATUS __stdcall 
MyNtCreateProcessEx(
	__out PHANDLE ProcessHandle, 
	__in ACCESS_MASK DesiredAccess, 
	__in POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, 
	__in HANDLE ParentProcess, 
	__in BOOLEAN InheritObjectTable, 
	__in HANDLE SectionHandle OPTIONAL, 
	__in HANDLE DebugPort OPTIONAL, 
	__in HANDLE ExceptionPort OPTIONAL, 
	__in BOOLEAN InJob)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	
	DbgPrint ("MyNtCreateProcessEx Entry.\n");
	DbgPrint ("pOrigFunction is 0x%08x.\n", hi_NtCreateProcessEx.pOrigFunction);
	DbgPrint ("pHookFunction is 0x%08x.\n", hi_NtCreateProcessEx.pHookFunction);
	DbgPrint ("pTramFunction is 0x%08x.\n", hi_NtCreateProcessEx.pTramFunction);
	
	if (0 == hi_NtCreateProcessEx.pTramFunction)
		return ntStatus;
		
	DbgPrint ("MyNtCreateProcessEx after.\n");
		
	ntStatus = ((LPFUN_NTCREATEPROCESSEX)(hi_NtCreateProcessEx.pTramFunction))(ProcessHandle,
																																					DesiredAccess,
																																					ObjectAttributes,
																																					ParentProcess,
																																					InheritObjectTable,
																																					SectionHandle,
																																					DebugPort,
																																					ExceptionPort,
																																					InJob);
	return ntStatus;
}
	
DWORD GetDllFunctionAddress(PUNICODE_STRING pDllName, char* lpFunctionName) 
{
    HANDLE hSection, hFile, hMod;
    SECTION_IMAGE_INFORMATION sii;
    IMAGE_DOS_HEADER* dosheader;
    IMAGE_OPTIONAL_HEADER* opthdr;
    IMAGE_EXPORT_DIRECTORY* pExportTable;
    DWORD* arrayOfFunctionAddresses;
    DWORD* arrayOfFunctionNames;
    WORD* arrayOfFunctionOrdinals;
    DWORD functionOrdinal;
    DWORD Base, x, functionAddress;
    char* functionName;
    STRING ntFunctionName, ntFunctionNameSearch;
    PVOID BaseAddress = NULL;
    SIZE_T size=0;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK iosb;

    OBJECT_ATTRIBUTES oa = {sizeof oa, 0, pDllName, OBJ_CASE_INSENSITIVE};
    oa.ObjectName = 0;

    ntstatus = ZwOpenFile(&hFile, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
		if (!NT_SUCCESS (ntstatus)) {
				return 0;
		}
		
    ZwCreateSection(&hSection, SECTION_ALL_ACCESS, &oa, 0,PAGE_EXECUTE, SEC_IMAGE, hFile);
    ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 0, 1000, 0, &size, (SECTION_INHERIT)1, MEM_TOP_DOWN, PAGE_READWRITE); 
    ZwClose(hFile);
    
    hMod = BaseAddress;
    dosheader = (IMAGE_DOS_HEADER *)hMod;
    opthdr =(IMAGE_OPTIONAL_HEADER *) ((BYTE*)hMod+dosheader->e_lfanew+24);
    pExportTable =(IMAGE_EXPORT_DIRECTORY*)((BYTE*) hMod + opthdr->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT]. VirtualAddress);
    arrayOfFunctionAddresses = (DWORD*)( (BYTE*)hMod + pExportTable->AddressOfFunctions);
    arrayOfFunctionNames = (DWORD*)( (BYTE*)hMod + pExportTable->AddressOfNames);
    arrayOfFunctionOrdinals = (WORD*)( (BYTE*)hMod + pExportTable->AddressOfNameOrdinals);
    Base = pExportTable->Base;
    
    RtlInitString(&ntFunctionNameSearch, lpFunctionName);
    for(x = 0; x < pExportTable->NumberOfFunctions; x++)
    {
        functionName = (char*)( (BYTE*)hMod + arrayOfFunctionNames[x]);
        RtlInitString(&ntFunctionName, functionName);
        functionOrdinal = arrayOfFunctionOrdinals[x] + Base - 1; 
        functionAddress = (DWORD)( (BYTE*)hMod + arrayOfFunctionAddresses[functionOrdinal]);
        if (RtlCompareString(&ntFunctionName, &ntFunctionNameSearch, TRUE) == 0) 
        {
            ZwClose(hSection);
            return functionAddress;
        }
    }
    ZwClose(hSection);
    
    return 0;
}
	
PVOID GetModlueBaseAdress(char* ModlueName)
{
		ULONG size,index;
		PULONG buf;
		NTSTATUS status;
		PSYSTEM_MODULE_INFORMATION module;
		PVOID ModlueAddress=0;
		
		ZwQuerySystemInformation(SystemModuleInformation,&size, 0, &size);
		if(NULL==(buf = (PULONG)ExAllocatePool(PagedPool, size)))
		{
			return 0;
		}
		status=ZwQuerySystemInformation(SystemModuleInformation,buf, size , 0);
		if(!NT_SUCCESS( status ))
		{
			return 0;
		}
		module = (PSYSTEM_MODULE_INFORMATION)(( PULONG )buf);
		for (index = 0; index < *buf; index++){
			if (_stricmp(module->Module[index].ImageName + module->Module[index].ModuleNameOffset, ModlueName) == 0)  
			{
				  ModlueAddress = module->Module[index].Base;
			}
		}
		ExFreePool(buf);
		return ModlueAddress;
}
	
PVOID KernelGetModuleBase(
  PCHAR  pModuleName
  )
{
    PVOID pModuleBase = NULL;
    PULONG pSystemInfoBuffer = NULL;
    PSYSTEM_MODULE_INFORMATION module;

    __try
    {
        NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
        ULONG    SystemInfoBufferSize = 0;

        status = ZwQuerySystemInformation(SystemModuleInformation,
            &SystemInfoBufferSize,
            0,
            &SystemInfoBufferSize);

        if (!SystemInfoBufferSize)
            return NULL;

        pSystemInfoBuffer = (PULONG)ExAllocatePool(NonPagedPool, SystemInfoBufferSize*2);

        if (!pSystemInfoBuffer)
            return NULL;

        memset(pSystemInfoBuffer, 0, SystemInfoBufferSize*2);

        status = ZwQuerySystemInformation(SystemModuleInformation,
            pSystemInfoBuffer,
            SystemInfoBufferSize * 2,
            &SystemInfoBufferSize);

        if (NT_SUCCESS(status))
        {
            ULONG i;
        		module = (PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer);
            
            for (i = 0; i <((PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer))->Count; i++)
            {
                if (_stricmp(module->Module[i].ImageName + module->Module[i].ModuleNameOffset, 
                		pModuleName) == 0)
                {
                    pModuleBase = module->Module[i].Base;
                    break;
                }
            }
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pModuleBase = NULL;
    }
    if(pSystemInfoBuffer) {
        ExFreePool(pSystemInfoBuffer);
    }

    return pModuleBase;
} 

ULONG GetExportFunctionAddress (
	IN PCSTR moduleName,
	IN PCSTR funName
	) 
{
		HANDLE   hMod;
		PUCHAR BaseAddress = NULL;
		IMAGE_DOS_HEADER * dosheader;
		IMAGE_OPTIONAL_HEADER * opthdr;
		PIMAGE_EXPORT_DIRECTORY exports;	
		USHORT   index=0 ; 
		ULONG findAddr = 0;
		ULONG i;
		PUCHAR pFuncName = NULL;
		PULONG pAddressOfFunctions,pAddressOfNames;
		PUSHORT pAddressOfNameOrdinals;
		
		BaseAddress=  GetModlueBaseAdress(moduleName);
		if (0 == BaseAddress){
				return 0;
		}
			
		hMod = BaseAddress;
		dosheader = (IMAGE_DOS_HEADER *)hMod;
		opthdr =(IMAGE_OPTIONAL_HEADER *) ((BYTE*)hMod+dosheader->e_lfanew+24);
		exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)dosheader+ opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		
		pAddressOfFunctions=(ULONG*)((BYTE*)hMod+exports->AddressOfFunctions);   
		pAddressOfNames=(ULONG*)((BYTE*)hMod+exports->AddressOfNames);           
		pAddressOfNameOrdinals=(USHORT*)((BYTE*)hMod+exports->AddressOfNameOrdinals); 
		
		for (i = 0; i < exports->NumberOfNames; i++) 
		{
			index=pAddressOfNameOrdinals[i];
			pFuncName = (PUCHAR)( (BYTE*)hMod + pAddressOfNames[i]);
			if (_stricmp( (char*)pFuncName,funName) == 0)
			{
					findAddr = pAddressOfFunctions[index];
					break;
			}
		}
		
		return findAddr;
}

ULONG_PTR GetUnExportFunctionAddress  ( 
	IN ULONG FirstFeature,  
	IN ULONG SecondFeature,  
	IN ULONG ThirdFeature,  
	IN ULONG FourthFeature  
	)  
{  
		NTSTATUS ntStatus = STATUS_SEVERITY_SUCCESS;  
		ULONG SystemInformationLength = 0;  
		ULONG Index = 0;  
		ULONG Loop = 0;  
		ULONG_PTR ModuleBeginAddress = 0;  
		ULONG_PTR ModuleFinishAddress = 0;  
		PULONG SystemInformationBuffer = NULL;  
		PSYSTEM_MODULE_INFORMATION SystemModulePointer = NULL;  
		ULONG_PTR Value = 0;
		
		//ULONG64 Value = 0xfffff800012d32c0;  

			
		ZwQuerySystemInformation(SystemModuleInformation,NULL,0,&SystemInformationLength);  
		SystemInformationBuffer = ExAllocatePool(PagedPool,SystemInformationLength);  
		if (SystemInformationBuffer == NULL)  
		{  
				DbgPrint ("GetUnExportFunctionAddress ExAllocatePool error.\n");
		    return 0;  
		}   

		ntStatus = ZwQuerySystemInformation (SystemModuleInformation,  
			                                SystemInformationBuffer,  
			                                SystemInformationLength,  
			                                NULL);  
		if (!NT_SUCCESS(ntStatus))  
		{  
			DbgPrint ("GetUnExportFunctionAddress ZwQuerySystemInformation error.\n");
		    ExFreePool(SystemInformationBuffer);  
		    return 0;  
		}  
		
		if (MmIsAddressValid(SystemInformationBuffer) == FALSE)  
		{  
			DbgPrint ("GetUnExportFunctionAddress MmIsAddressValid error.\n");
		    ExFreePool(SystemInformationBuffer);  
		    return 0;  
		}  

		SystemModulePointer=(PSYSTEM_MODULE_INFORMATION)(SystemInformationBuffer);  
		for (Index=0;Index<*(ULONG*)SystemInformationBuffer;Index++)  
		{  
				ULONG_PTR ulAddr;
				ULONG i = 1;
				DbgPrint ("Module Name = %s.\n", SystemModulePointer->Module[Index].ImageName + SystemModulePointer->Module[Index].ModuleNameOffset);		
							
				if (_stricmp(SystemModulePointer->Module[Index].ImageName + SystemModulePointer->Module[Index].ModuleNameOffset, "ntkrnlpa.exe") != 0 &&
						_stricmp(SystemModulePointer->Module[Index].ImageName + SystemModulePointer->Module[Index].ModuleNameOffset, "ntoskrnl.exe") != 0 )
					continue;  
		
		    ModuleBeginAddress=(ULONG_PTR)SystemModulePointer->Module[Index].Base;  
		    ModuleFinishAddress=(ULONG_PTR)SystemModulePointer->Module[Index].Base+SystemModulePointer->Module[Index].Size;  
		    for (ulAddr = ModuleBeginAddress; ulAddr < ModuleFinishAddress; ((PULONG)ulAddr)++)  
		    {  
		    		DbgPrint ("address = %08x.\n", (ULONG_PTR)ulAddr);
		    		
		        if(*((ULONG *)(ulAddr + 0)) == FirstFeature&&  
		           *((ULONG *)(ulAddr + 4)) == SecondFeature&&  
		           *((ULONG *)(ulAddr + 8)) == ThirdFeature&&  
		           *((ULONG *)(ulAddr + 12)) == FourthFeature)  
		        {  
		        		DbgPrint ("GetUnExportFunctionAddress found.\n");
		            Value = ulAddr;  
		            break;
		        } 
		    }  
		}  
		ExFreePool(SystemInformationBuffer);  
		
		return Value;  
} 

static ULONG ThreadStartRoutineOffset = 0;

typedef 
VOID 
(* LPFUN_KEBUGCHECKEX)(
  __in  ULONG BugCheckCode,
  __in  ULONG_PTR BugCheckParameter1,
  __in  ULONG_PTR BugCheckParameter2,
  __in  ULONG_PTR BugCheckParameter3,
  __in  ULONG_PTR BugCheckParameter4
	);

VOID MyKeBugCheckEx(
  __in  ULONG BugCheckCode,
  __in  ULONG_PTR BugCheckParameter1,
  __in  ULONG_PTR BugCheckParameter2,
  __in  ULONG_PTR BugCheckParameter3,
  __in  ULONG_PTR BugCheckParameter4)
{
    PUCHAR LockedAddress;
    PCHAR  ReturnAddress;
    PMDL   Mdl = NULL;


    //
    // Call the real KeBugCheckEx if this isn't the bug check code we're looking
    // for.
    //
    if (BugCheckCode != 0x109)
    {
        DbgPrint(("Passing through bug check %.4x to %p.",
                BugCheckCode,
                hi_KeBugCheckEx.pTramFunction));

				((LPFUN_KEBUGCHECKEX)(hi_KeBugCheckEx.pTramFunction))(BugCheckCode,
																															BugCheckParameter1,
																															BugCheckParameter2,
																															BugCheckParameter3,
																															BugCheckParameter4);
    }
    else
    {
    		//
    		//ThreadStartRoutineOffset = nt!_ETHREAD + StartAddress
    		//
        PCHAR CurrentThread = (PCHAR)PsGetCurrentThread();
        PVOID StartRoutine  = *(PVOID **)(CurrentThread + ThreadStartRoutineOffset);
        PVOID StackPointer  = IoGetInitialStack();

        DbgPrint(("Restarting the current worker thread %p at %p (SP=%p, off=%lu).",
                PsGetCurrentThread(),
                StartRoutine,
                StackPointer,
                ThreadStartRoutineOffset));

        //
        // Shift the stack pointer back to its initial value and call the routine.  We
        // subtract eight to ensure that the stack is aligned properly as thread
        // entry point routines would expect.
        //
        AdjustStackCallPointer(
                (ULONG_PTR)StackPointer - 0x8,
                StartRoutine,
                NULL);
    }

    //
    // In either case, we should never get here.
    //
    __debugbreak();
}


VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	UnInstallHook (&hi_NtCreateProcessEx);
}

NTSTATUS DriverEntry( 
	IN PDRIVER_OBJECT pDriverObject, 
	IN PUNICODE_STRING pRegistryPath 
	)
{
	/*
	ULONG_PTR ulNtCreateProcessEx = GetUnExportFunctionAddress (0xb0680c6a,
																													0xe8804d9e,
																													0xfff714a2,
																													0x0124a164);
	*/
	//ULONG_PTR ulNtCreateProcessEx = 0x805c5c32;
	//ULONG_PTR ulNtCreateProcessEx = 0x805c6c32;
	ULONG_PTR ulNtCreateProcessEx = 0xfffff800012d32c0;
	ULONG_PTR ulKeBugCheckEx = 0x0000;
	hi_NtCreateProcessEx.pOrigFunction = ulNtCreateProcessEx;
	hi_NtCreateProcessEx.pHookFunction = (ULONG_PTR)((PCHAR)MyNtCreateProcessEx);
	InstallHook (&hi_NtCreateProcessEx);

	
	hi_KeBugCheckEx.pOrigFunction = ulKeBugCheckEx;
	hi_KeBugCheckEx.pHookFunction = (ULONG_PTR)((PCHAR)MyKeBugCheckEx);
	InstallHook (&hi_KeBugCheckEx);
	
  pDriverObject->DriverUnload = OnUnload;  
  																										   
  return STATUS_SUCCESS;
}