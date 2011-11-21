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
// process.c

#include "stdafx.h"
#include "Process.h"

//
// return full path name
//
INT GetProcessFileName(char* buf, DWORD nSize, HANDLE ProcessHandle)
{
	PVOID pCommandLine;
	char* sCommandLine;
	DWORD i;

	if(buf == 0 || nSize == 0)
		return PS_INVALID_PARAMETER;

	if(nSize < MAX_PATH)
		return PS_BUFFER_SO_SMALL;

	pCommandLine = GetCurrentCommandLine(ProcessHandle);

	if(pCommandLine == 0)
	{
		strcpy(buf, "SYSTEM");
		return PS_SYSTEM_PROCESS;
	}

	i = 0;
	sCommandLine = (char*)pCommandLine;
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

PVOID GetCurrentCommandLine(HANDLE ProcessHandle)
{
	static DWORD dwAddress;
	dwAddress = (DWORD)ProcessHandle;

	if(dwAddress == 0 || ProcessHandle == (HANDLE)0xFFFFFFFF)
		return 0;

	try
	{
		dwAddress += BASE_PDB_PEDB_OFFSET;
		if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;

		dwAddress += BASE_EDB_COMMAND_LINE_OFFSET;
		if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;

		if(*(DWORD*)dwAddress == 0) return 0;
	}
	catch(...)
	{
		return 0;
	}
	return (PVOID)dwAddress;
}


#pragma comment( exestr, "B9D3B8FD2A727471656775752B")
