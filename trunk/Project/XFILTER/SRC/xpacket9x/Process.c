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
// process.c
//
// ��飺
//		�õ���ǰ���̵Ľ�����������·��
//
//

#include "xprecomp.h"
#pragma hdrstop

//
// �õ���ǰ���̵�����·�����߽�����
// ������
//		buf:	���ؽ�������������·��
//		nSize:	buf�Ĵ�С
//		IsOnlyName��
//				TRUE:	���õ�������
//				FALSE:	�õ����̵�����·��
// ����ֵ��
//		PS_INVALID_PARAMETER��	��Ч�Ĳ���
//		PS_BUFFER_SO_SMALL��	������̫С
//		PS_SYSTEM_PROCESS��		ϵͳ����
//		PS_USER_PROCESS��		�û�����
//
//
INT GetProcessFileName(char* buf, DWORD nSize, BOOL IsOnlyName)
{
	PVOID pCommandLine;
	char* sCommandLine;
	DWORD i;

	if(buf == 0 || nSize == 0)
		return PS_INVALID_PARAMETER;

	if((!IsOnlyName && nSize < MAX_PATH) || nSize < 16)
		return PS_BUFFER_SO_SMALL;

	if(IsOnlyName)
	{
		GetProcessName(buf);
		return PS_SYSTEM_PROCESS;
	}

	pCommandLine = GetCurrentCommandLine(NULL);

	i = 0;
	sCommandLine = (char*)pCommandLine;

	if(sCommandLine == 0 || 
		(sCommandLine[0] != '\"' 
			&& (sCommandLine[0] < 'A' || sCommandLine[0] > 'z')
		)
	  )
	{
		strcpy(buf, "System");
		return PS_SYSTEM_PROCESS;
	}

	if(IsOnlyName)
	{
		while(sCommandLine[i] != '.' && sCommandLine[i] != 0 && i < MAX_PATH)
			i++;
		if(sCommandLine[i] == '.')
		{
			static int iIndex;
			iIndex = i - 1;
			while(sCommandLine[0] != '\\' && iIndex >= 0)
				iIndex--;
			if(sCommandLine[iIndex] == '\\')
			{
				static int iTemp;
				iTemp = 0;
				while(iIndex < i && iTemp < nSize)
					buf[iTemp++] = sCommandLine[iIndex++];
				if(iTemp == nSize)
					sCommandLine[iTemp - 1] = 0;
				else
					sCommandLine[iTemp] = 0;
			}
			else
			{
				strcpy(buf, "SYSTEM");
				//GetProcessName(buf);
			}
		}
		else
		{
			strcpy(buf, "SYSTEM");
			//GetProcessName(buf);
		}
		return PS_USER_PROCESS;
	}

	if(sCommandLine[0] == '\"')
	{
		while(sCommandLine[i + 1] != '\"' 
			&& sCommandLine[i + 1] != 0 && i < nSize)
		{
			buf[i] = sCommandLine[i + 1];
			i++;
		}
	}
	else
	{
		while(sCommandLine[i] != ' ' 
			&& sCommandLine[i] != 0 && i < nSize)
		{
			buf[i] = sCommandLine[i];
			i++;
		}
	}
	buf[i] = 0;
	
	return PS_USER_PROCESS;
}

//
// �õ���ǰ���̵������У�Ȼ��ͨ�������з��������·����
//
PVOID GetCurrentCommandLine(HANDLE ProcessHandle)
{
	static DWORD dwAddress;
	if(ProcessHandle == 0 || ProcessHandle == (HANDLE)0xFFFFFFFF)
		dwAddress = (DWORD)VWIN32_GetCurrentProcessHandle();
	else
		dwAddress = (DWORD)ProcessHandle;

	if(dwAddress == 0 || dwAddress == 0xFFFFFFFF)
		return 0;

	dwAddress += BASE_PDB_PEDB_OFFSET;
	if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;

	dwAddress += BASE_EDB_COMMAND_LINE_OFFSET;
	if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;

	if(*(DWORD*)dwAddress == 0) return 0;

	return (PVOID)dwAddress;
}

//
// �����õ���ǰ���̵Ľ�����
//
VOID GetProcessName(PCHAR ProcessName)
{
    PVOID       CurProc;
    PVOID       ring3proc;
    char        *name;

    // Get the ring0 process pointer.
    CurProc = VWIN32_GetCurrentProcessHandle();

    // Now, map the ring3 PCB 
    ring3proc = (PVOID) SelectorMapFlat(
		Get_Sys_VM_Handle(), 
        (DWORD)(*(PDWORD)((char *) CurProc + 0x38)) | 0x7,
		0
		);

    if( ring3proc == (PVOID) -1 )
	{
        strcpy( ProcessName, "SYSTEM");
    }
	else
	{
        name = ((char *)ring3proc) + 0xF2;
        if( name[0] >= 'A' && name[0] <= 'z' )
		{
            strcpy( ProcessName, name );
            ProcessName[8] = 0;
        }
		else
		{
            strcpy( ProcessName, "SYSTEM" );
        }
    }
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
	  dprintf("0x%08X	", tmpstr + (i * 16));
	  dprintf("%04X	", i * 16);
	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  dprintf("%02X ", tmpstr[(i * 16) + j] & 0x00FF);
	  }
	  for(; j < 16; j++) dprintf(".. ");

	  for (j = 0; j < (i < m ? 16 : n); j ++)
	  {
		  if(tmpstr[(i * 16) + j] >= 32)
			dprintf("%c", tmpstr[(i * 16) + j]);
		  else
			dprintf(".");
	  }
	  for(; j < 16; j++) dprintf(".. ");

	  dprintf("\n");
	}
}

#pragma comment( exestr, "B9D3B8FD2A727471656775752B")
