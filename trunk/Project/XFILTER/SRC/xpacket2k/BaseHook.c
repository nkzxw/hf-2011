//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/01/31
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
// ��飺
//		Hook ϵͳ�����Ļ���������
//
//

#include <windows.h>
#include <winnt.h>

unsigned long CR0VALUE = 0;

//
// ����Windows NT/2000���ڴ汣����ʹֻ���ڴ�����д
//
void DisableProtection() 
{
	__asm
	{
		mov eax,cr0 
		mov CR0VALUE,eax 
		and eax,0fffeffffh 
		mov cr0,eax 
	}
}

//
// ����Windows NT/2000���ڴ汣��
//
void EnableProtection()
{
	__asm
	{
		mov eax,CR0VALUE 
		mov cr0,eax 
	}
}

//
// Hook һ��ϵͳ����
// ������
//		pBaseAddress��		ҪHook���������ļ����ڴ�ӳ��Ļ���ַ������NDIS.SYS�Ļ���ַ
//		Name:				ҪHook�ĺ�����
//		InFunc:				�Լ��ĺ�����ַ
//		OutFunc:			Hook�󱣴�ϵͳ�ĺ�����ַ
// ����ֵ��
//		NULL:				Hookʧ��
//		Not NULL:			����ϵͳ������ַ����*OutFunc��ͬ
//
//
PVOID HookFunction(PVOID pBaseAddress, PCSTR Name, PVOID InFunc, ULONG* OutFunc)
{
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_DATA_DIRECTORY pDirectory = NULL;
	PIMAGE_EXPORT_DIRECTORY pExports = NULL;
	ULONG nSize, Address, i;
	PULONG pFunctions = NULL;
	PSHORT pOrdinals = NULL;
	PULONG pNames = NULL;
	PVOID pFunction = NULL;
	ULONG Ordinal = 0;

	if(pBaseAddress == NULL)
		return NULL;

	pDosHeader = (PIMAGE_DOS_HEADER)pBaseAddress;
	pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)pBaseAddress + pDosHeader->e_lfanew);
	pDirectory = pNtHeader->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;

	nSize = pDirectory->Size;
	Address = pDirectory->VirtualAddress;

	pExports = (PIMAGE_EXPORT_DIRECTORY)((PCHAR)pBaseAddress + Address);

	pFunctions = (PULONG)((PCHAR)pBaseAddress + pExports->AddressOfFunctions);
	pOrdinals  = (PSHORT)((PCHAR)pBaseAddress + pExports->AddressOfNameOrdinals);
	pNames	   = (PULONG)((PCHAR)pBaseAddress + pExports->AddressOfNames);

	for(i = 0; i < pExports->NumberOfNames; i++)
	{
		Ordinal = pOrdinals[i];
		if(pFunctions[Ordinal] < Address || pFunctions[Ordinal] >= Address + nSize)
		{
			if(strcmp((PSTR)((PCHAR)pBaseAddress + pNames[i]), Name) == 0)
			{
				pFunction = (PCHAR)pBaseAddress + pFunctions[Ordinal];
				*OutFunc = (ULONG)pFunction;
				DisableProtection();
				pFunctions[Ordinal] = (ULONG)((ULONG)InFunc - (ULONG)pBaseAddress);
				EnableProtection();
				break;
			}
		}
	}

	return pFunction;
}

//
// �����ϵͳ������HOOK
// ������
//		pBaseAddress��		ҪHook���������ļ����ڴ�ӳ��Ļ���ַ������NDIS.SYS�Ļ���ַ
//		Name:				ҪHook�ĺ�����
//		UnHookFunc:			ϵͳ������ַ
// ����ֵ��
//		NULL:				���Hookʧ��
//		Not NULL:			����ԭ���ĺ�����ַ
//
PVOID UnHookFunction(PVOID pBaseAddress, PCSTR Name, PVOID UnHookFunc)
{
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_DATA_DIRECTORY pDirectory = NULL;
	PIMAGE_EXPORT_DIRECTORY pExports = NULL;
	ULONG nSize, Address, i;
	PULONG pFunctions = NULL;
	PSHORT pOrdinals = NULL;
	PULONG pNames = NULL;
	PVOID pFunction = NULL;
	ULONG Ordinal = 0;

	if(pBaseAddress == NULL)
		return NULL;

	pDosHeader = (PIMAGE_DOS_HEADER)pBaseAddress;
	pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)pBaseAddress + pDosHeader->e_lfanew);
	pDirectory = pNtHeader->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;

	nSize = pDirectory->Size;
	Address = pDirectory->VirtualAddress;

	pExports = (PIMAGE_EXPORT_DIRECTORY)((PCHAR)pBaseAddress + Address);

	pFunctions = (PULONG)((PCHAR)pBaseAddress + pExports->AddressOfFunctions);
	pOrdinals  = (PSHORT)((PCHAR)pBaseAddress + pExports->AddressOfNameOrdinals);
	pNames	   = (PULONG)((PCHAR)pBaseAddress + pExports->AddressOfNames);

	for(i = 0; i < pExports->NumberOfNames; i++)
	{
		Ordinal = pOrdinals[i];
		if(pFunctions[Ordinal] < Address || pFunctions[Ordinal] >= Address + nSize)
		{
			if(strcmp((PSTR)((PCHAR)pBaseAddress + pNames[i]), Name) == 0)
			{
				pFunction = (PCHAR)pBaseAddress + pFunctions[Ordinal];
				DisableProtection();
				pFunctions[Ordinal] = (ULONG)((ULONG)UnHookFunc - (ULONG)pBaseAddress);
				EnableProtection();
				break;
			}
		}
	}

	return pFunction;
}



#pragma comment( exestr, "B9D3B8FD2A646375676A71716D2B")
