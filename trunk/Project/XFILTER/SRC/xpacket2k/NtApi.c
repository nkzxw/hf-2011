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
// ��飺
//		����NativeAPI��ȡModule�Ļ���ַ���Ӷ����HOOK��
//
//

#include <ntddk.h>
#include "NtApi.h"

#ifndef dprintf
#define dprintf		KdPrint
#endif

PVOID m_NdisBaseAddress = NULL;

// 
// �õ�Ndis.sys���ڴ��еĻ���ַ(�����ַ)
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
// ���Ժ�������ӡSYSTEM_MODULE_INFORMATION
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
// ���Ժ����������������������ݰ�BYTE��16�������
// ������
//		start��	��ʼָ��
//		offset��������ֽ���Ŀ
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
