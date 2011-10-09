#include "ntddk.h"
#include "eat_hook.h"
#pragma comment(lib,"ntdll.lib")

ULONG g_OriginalPsGetCurrentProcessId;
typedef HANDLE (*PSGETCURRENTPROCESSID)();

HANDLE
MyPsGetCurrentProcessId()
{
	HANDLE handle;
	DbgPrint("HOOK_PsGetCurrentProcessId called!\n");
  handle =((PSGETCURRENTPROCESSID)(g_OriginalPsGetCurrentProcessId))();
  
  return handle;
}  

PVOID GetModlueBaseAdress(
	char* ModlueName
	)
{
	ULONG size,index;
	PULONG buf;
	NTSTATUS status;
	PSYSTEM_MODULE_INFORMATION module;
	PVOID driverAddress=0;
	
	ZwQuerySystemInformation(SystemModuleInformation,&size, 0, &size);
	if(NULL==(buf = (PULONG)ExAllocatePool(PagedPool, size)))
	{
			DbgPrint("failed alloc memory failed  \n");
			return 0;
	}
	status=ZwQuerySystemInformation(SystemModuleInformation,buf, size , 0);
	if(!NT_SUCCESS( status ))
	{
     DbgPrint("failed  query\n");
	   return 0;
	}
	
	module = (PSYSTEM_MODULE_INFORMATION)(( PULONG )buf + 1);
	for (index = 0; index < *buf; index++)
	if (_stricmp(module[index].ImageName + module[index].ModuleNameOffset, ModlueName) == 0)  
	{
	    driverAddress = module[index].Base;
	    DbgPrint("Module found at:%x\n",driverAddress);
	}
	ExFreePool(buf);
	return driverAddress;
}

//
//StartHook_And_Unhook是安装钩子和卸载钩子,如果	test==1表示安装,否则表示卸载
//
VOID StartHook_And_Unhook(IN PCSTR funName, IN unsigned int test) 
{
		HANDLE   hMod;
		PUCHAR BaseAddress = NULL;
		IMAGE_DOS_HEADER * dosheader;
		IMAGE_OPTIONAL_HEADER * opthdr;
		PIMAGE_EXPORT_DIRECTORY exports;
	
		USHORT   index=0 ; 
		ULONG addr ,i;
	
		PUCHAR pFuncName = NULL;
		PULONG pAddressOfFunctions,pAddressOfNames;
		PUSHORT pAddressOfNameOrdinals;
    
		BaseAddress=  GetModlueBaseAdress("ntkrnlpa.exe");
    DbgPrint("Map BaseAddress is:%x\n",BaseAddress);
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
						addr=pAddressOfFunctions[index];
						break;
				}
		}
	
		if(test==1)	{	
			_asm
			{
					CLI					
					MOV	EAX, CR0		
					AND EAX, NOT 10000H 
					MOV	CR0, EAX		
			}	
			
			DbgPrint("PsGetCurrentProcessId is:%x\n",(PUCHAR)hMod + pAddressOfFunctions[index]);
			pAddressOfFunctions[index] = ( PCHAR )MyPsGetCurrentProcessId - BaseAddress;
			DbgPrint("g_OriginalPsGetCurrentProcessId is:%x\n",g_OriginalPsGetCurrentProcessId);
			g_OriginalPsGetCurrentProcessId=  (PUCHAR)hMod + pAddressOfFunctions[index] ;
			_asm 
			{
				MOV	EAX, CR0		
				OR	EAX, 10000H			
				MOV	CR0, EAX			
				STI					
			}	
		}
		else
		{
			_asm
			{
				CLI					
				MOV	EAX, CR0		
				AND EAX, NOT 10000H 
				MOV	CR0, EAX		
			}	
		
			pAddressOfFunctions[index] = ( PCHAR )g_OriginalPsGetCurrentProcessId - BaseAddress;
		
			_asm 
			{
				MOV	EAX, CR0		
				OR	EAX, 10000H			
				MOV	CR0, EAX			
				STI					
			}	
		}
} 

VOID Unload(PDRIVER_OBJECT DriverObject)
{	
		PCSTR myfunName="PsGetCurrentProcessId";
		StartHook_And_Unhook(myfunName,0);
		DbgPrint("Unload Callled\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING str)
{	
  PCSTR myfunName="PsGetCurrentProcessId";
  StartHook_And_Unhook(myfunName,1);
  
  DriverObject->DriverUnload = Unload;
	return STATUS_SUCCESS;
}

