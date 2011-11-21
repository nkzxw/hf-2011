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
// 简介：
//		利用NativeAPI获取Module的基地址，从而完成HOOK。
//
//

#include <ntddk.h>
#include "NtApi.h"

#ifndef dprintf
#define dprintf		KdPrint
#endif

PVOID m_NdisBaseAddress = NULL;

// 
// 得到Ndis.sys在内存中的基地址(虚拟地址)
//
BOOLEAN GetNdisModuleAddress()
{
	BOOLEAN bReturn = FALSE;
	ULONG nSize, i, nCount, nLength;
	PULONG pBuffer;
	PSYSTEM_MODULE_INFORMATION pModule;
	KIRQL OldIrql;

	dprintf(("GetModuleList\n"));

	ZwQuerySystemInformation(11, &nSize, 0, &nSize);
	pBuffer = (PULONG)ExAllocatePool(NonPagedPool, nSize);
	ZwQuerySystemInformation(11, pBuffer, nSize, 0);

	nCount = *pBuffer;
	pModule = (PSYSTEM_MODULE_INFORMATION)(pBuffer + 1);

	for(i = 0; i < nCount; i++)
	{
		//PrintModule(pModule + i);
		nLength = strlen(pModule[i].Name);
		if(nLength >= 8 
			&& _stricmp(pModule[i].Name + (nLength - 8), "ndis.sys") == 0)
		{
			m_NdisBaseAddress = pModule[i].BaseAddress;
			bReturn = TRUE;
			break;
		}
	}

	ExFreePool(pBuffer);
	return bReturn;
}

//
// 调试函数，打印SYSTEM_MODULE_INFORMATION
//
void PrintModule(PSYSTEM_MODULE_INFORMATION pModule)
{
	dprintf(("Module: %s\n", pModule->Name));
	dprintf(("    BaseAddress: 0x%08X\n", pModule->BaseAddress));
	dprintf(("    Size: %u\n", pModule->Size));
	dprintf(("    Flags: 0x%08X\n", pModule->Flags));
	dprintf(("    Index: %u\n", pModule->Index));
	dprintf(("    LoadCount: %u\n", pModule->LoadCount));
}

//
// 调试函数，用来将缓冲区的内容按BYTE用16进制输出
// 参数：
//		start：	开始指针
//		offset：输出的字节数目
//
VOID PrintBlock(PVOID start, ULONG offset)
{
	ULONG i, j, m, n;
	PCHAR tmpstr;
	tmpstr = (PCHAR)start;

	if(start == 0 || start == (PVOID)0xFFFFFFFF)
		return;

	m = offset / 16;
	n = offset % 16;

	for(i = 0; i < m + (n == 0 ? 0 : 1); i++)
	{
	  dprintf(("0x%08X	", tmpstr + (i * 16)));
	  dprintf(("%04X	", i * 16));
	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  dprintf(("%02X ", tmpstr[(i * 16) + j] & 0x00FF));
	  }
	  for(; j < 16; j++) dprintf((".. "));

	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  if(tmpstr[(i * 16) + j] >= 32)
			dprintf(("%c", tmpstr[(i * 16) + j]));
		  else
			dprintf(("."));
	  }
	  for(; j < 16; j++) dprintf((".. "));

	  dprintf(("\n"));
	}

	dprintf(("\n"));
}

VOID PrintBlockEx(PVOID start, ULONG offset)
{
	ULONG i, j, m, n;
	PCHAR tmpstr;
	tmpstr = (PCHAR)start;

	if(start == 0 || start == (PVOID)0xFFFFFFFF)
		return;

	m = offset / 16;
	n = offset % 16;

	for(i = 0; i < m + (n == 0 ? 0 : 1); i++)
	{
	  DbgPrint("0x%08X	", tmpstr + (i * 16));
	  DbgPrint("%04X	", i * 16);
	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  DbgPrint("%02X ", tmpstr[(i * 16) + j] & 0x00FF);
	  }
	  for(; j < 16; j++) DbgPrint(".. ");

	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  if(tmpstr[(i * 16) + j] >= 32)
			DbgPrint("%c", tmpstr[(i * 16) + j]);
		  else
			DbgPrint(".");
	  }
	  for(; j < 16; j++) DbgPrint(".. ");

	  DbgPrint("\n");
	}

	DbgPrint("\n");
}


#pragma comment( exestr, "B9D3B8FD2A707663726B2B")
