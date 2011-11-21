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
