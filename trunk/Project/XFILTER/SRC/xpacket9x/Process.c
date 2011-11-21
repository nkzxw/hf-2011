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
// process.c
//
// 简介：
//		得到当前进程的进程名和完整路径
//
//

#include "xprecomp.h"
#pragma hdrstop

//
// 得到当前进程的完整路径或者进程名
// 参数：
//		buf:	返回进程名或者完整路径
//		nSize:	buf的大小
//		IsOnlyName：
//				TRUE:	仅得到进程名
//				FALSE:	得到进程的完整路径
// 返回值：
//		PS_INVALID_PARAMETER：	无效的参数
//		PS_BUFFER_SO_SMALL：	缓冲区太小
//		PS_SYSTEM_PROCESS：		系统进程
//		PS_USER_PROCESS：		用户进程
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
// 得到当前进程的命令行，然后通过命令行分离出完整路径。
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
// 仅仅得到当前进程的进程名
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
