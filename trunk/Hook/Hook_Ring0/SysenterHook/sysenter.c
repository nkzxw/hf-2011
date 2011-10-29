#include <ntddk.h>
#include "OpcodeSize.h"

ULONG uSysenter;           //sysenter地址
UCHAR uOrigSysenterHead[8];//保存原来的八个字节函数头
PUCHAR pMovedSysenterCode; //把原来的KiFastCall函数头保存在这里
ULONG i;                   //记录服务ID

__declspec(naked) void MyKiFastCallEntry(void)
{
	  __asm
	  {
				pop  edi     //因为用到了edi来跳转 这里恢复
				mov  i, eax  //得到服务ID
	  }
	  
	  __asm
	  {  
				pushad
				push fs
				push 0x30
				pop fs
  	}
  
  	__asm
  	{
				pop fs
				popad    
    	  jmp pMovedSysenterCode //第二跳,跳转到原来的函数头代码 
  	}
}

VOID OnUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{    
	__asm
	{
			cli
			mov  eax,cr0
			and  eax,not 10000h
			mov  cr0,eax
  }

  memcpy((PVOID)uSysenter,uOrigSysenterHead,8);//把原来函数头的八个字节恢复

  __asm
  {
			mov  eax,cr0
			or   eax,10000h
			mov  cr0,eax
			sti
  }

  ExFreePool(pMovedSysenterCode); // 释放分配的内存
}

VOID HookSysenter ()
{
		//
		//第一跳,从KiFastCall跳到MyKiFastCallEntry.并绕过rootkit检测工具检测
		//
		UCHAR  cHookCode[8] = { 0x57,          //push edi       
		                        0xBF,0,0,0,0,  //mov  edi,0000
		                        0xFF,0xE7};    //jmp  edi
		

		//
		//jmp 0000 第三跳,从KiFastCall函数头代码跳转到原来KiFastCall+N
		//
		UCHAR  JmpCode[] = {0xE9,0,0,0,0};     

  	int nCopyLen = 0;
 	 	int nPos = 0;

  	__asm
  	{
       	mov ecx,0x176
        rdmsr
  		  mov uSysenter,eax  //得到KiFastCallEntry地址
  	}
	
	  KdPrint(("sysenter: 0x%08X",uSysenter));

	  //
	  //我们要改写的函数头至少需要8字节 
	  //这里计算实际需要COPY的代码长度 因为我们不能把一条完整的指令打断
	  //
	  nPos = uSysenter;
  	while(nCopyLen<8){ 
	    nCopyLen += GetOpCodeSize((PVOID)nPos);  //参考1
	    nPos = uSysenter + nCopyLen;
	  }
  
  	pMovedSysenterCode = ExAllocatePool(NonPagedPool,20);

  	//
  	//备份原来8字节代码
  	//
  	memcpy(uOrigSysenterHead,(PVOID)uSysenter,8);
  	
  	//
  	//计算跳转地址, 相对跳转地址
  	//
	  *((ULONG*)(JmpCode+1)) = (uSysenter + nCopyLen) - ((ULONG)pMovedSysenterCode + nCopyLen)- 5;

		//
		//把原来的函数头放到新分配的内存
		//
  	memcpy(pMovedSysenterCode,(PVOID)uSysenter,nCopyLen); 
  	
  	//
  	//把跳转代码COPY上去
  	//
  	memcpy((PVOID)(pMovedSysenterCode + nCopyLen),JmpCode,5); 

		//
		//HOOK地址
		//
  	*((ULONG*)(cHookCode + 2)) = (ULONG)MyKiFastCallEntry; 
  
	  KdPrint(("Saved sysenter code:0x%08X",pMovedSysenterCode));
	  KdPrint(("MyKiFastCallEntry:0x%08X",MyKiFastCallEntry));

  	__asm
  	{
	    cli
	    mov  eax,cr0
	    and  eax,not 10000h
	    mov  cr0,eax
  	}

 		 memcpy((PVOID)uSysenter,cHookCode,8);//把改写原来函数头

  	__asm
  	{
	    mov  eax,cr0
	    or   eax,10000h
	    mov  cr0,eax
	    sti
	  }
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
	)
{
  DriverObject->DriverUnload = OnUnload;

  HookSysenter();

  return STATUS_SUCCESS;
} 