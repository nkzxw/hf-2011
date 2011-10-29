#include <ntddk.h>
#include "OpcodeSize.h"

ULONG uSysenter;           //sysenter��ַ
UCHAR uOrigSysenterHead[8];//����ԭ���İ˸��ֽں���ͷ
PUCHAR pMovedSysenterCode; //��ԭ����KiFastCall����ͷ����������
ULONG i;                   //��¼����ID

__declspec(naked) void MyKiFastCallEntry(void)
{
	  __asm
	  {
				pop  edi     //��Ϊ�õ���edi����ת ����ָ�
				mov  i, eax  //�õ�����ID
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
    	  jmp pMovedSysenterCode //�ڶ���,��ת��ԭ���ĺ���ͷ���� 
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

  memcpy((PVOID)uSysenter,uOrigSysenterHead,8);//��ԭ������ͷ�İ˸��ֽڻָ�

  __asm
  {
			mov  eax,cr0
			or   eax,10000h
			mov  cr0,eax
			sti
  }

  ExFreePool(pMovedSysenterCode); // �ͷŷ�����ڴ�
}

VOID HookSysenter ()
{
		//
		//��һ��,��KiFastCall����MyKiFastCallEntry.���ƹ�rootkit��⹤�߼��
		//
		UCHAR  cHookCode[8] = { 0x57,          //push edi       
		                        0xBF,0,0,0,0,  //mov  edi,0000
		                        0xFF,0xE7};    //jmp  edi
		

		//
		//jmp 0000 ������,��KiFastCall����ͷ������ת��ԭ��KiFastCall+N
		//
		UCHAR  JmpCode[] = {0xE9,0,0,0,0};     

  	int nCopyLen = 0;
 	 	int nPos = 0;

  	__asm
  	{
       	mov ecx,0x176
        rdmsr
  		  mov uSysenter,eax  //�õ�KiFastCallEntry��ַ
  	}
	
	  KdPrint(("sysenter: 0x%08X",uSysenter));

	  //
	  //����Ҫ��д�ĺ���ͷ������Ҫ8�ֽ� 
	  //�������ʵ����ҪCOPY�Ĵ��볤�� ��Ϊ���ǲ��ܰ�һ��������ָ����
	  //
	  nPos = uSysenter;
  	while(nCopyLen<8){ 
	    nCopyLen += GetOpCodeSize((PVOID)nPos);  //�ο�1
	    nPos = uSysenter + nCopyLen;
	  }
  
  	pMovedSysenterCode = ExAllocatePool(NonPagedPool,20);

  	//
  	//����ԭ��8�ֽڴ���
  	//
  	memcpy(uOrigSysenterHead,(PVOID)uSysenter,8);
  	
  	//
  	//������ת��ַ, �����ת��ַ
  	//
	  *((ULONG*)(JmpCode+1)) = (uSysenter + nCopyLen) - ((ULONG)pMovedSysenterCode + nCopyLen)- 5;

		//
		//��ԭ���ĺ���ͷ�ŵ��·�����ڴ�
		//
  	memcpy(pMovedSysenterCode,(PVOID)uSysenter,nCopyLen); 
  	
  	//
  	//����ת����COPY��ȥ
  	//
  	memcpy((PVOID)(pMovedSysenterCode + nCopyLen),JmpCode,5); 

		//
		//HOOK��ַ
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

 		 memcpy((PVOID)uSysenter,cHookCode,8);//�Ѹ�дԭ������ͷ

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