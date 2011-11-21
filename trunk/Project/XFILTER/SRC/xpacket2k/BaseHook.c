//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

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
// 简介：
//		Hook 系统函数的基本操作。
//
//

#include <windows.h>
#include <winnt.h>

unsigned long CR0VALUE = 0;

//
// 禁用Windows NT/2000的内存保护，使只读内存区可写
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
// 启用Windows NT/2000的内存保护
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
// Hook 一个系统函数
// 参数：
//		pBaseAddress：		要Hook函数所在文件在内存映象的基地址，比如NDIS.SYS的基地址
//		Name:				要Hook的函数名
//		InFunc:				自己的函数地址
//		OutFunc:			Hook后保存系统的函数地址
// 返回值：
//		NULL:				Hook失败
//		Not NULL:			返回系统函数地址，与*OutFunc相同
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
// 解除对系统函数的HOOK
// 参数：
//		pBaseAddress：		要Hook函数所在文件在内存映象的基地址，比如NDIS.SYS的基地址
//		Name:				要Hook的函数名
//		UnHookFunc:			系统函数地址
// 返回值：
//		NULL:				解除Hook失败
//		Not NULL:			返回原来的函数地址
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
