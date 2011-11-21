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
//		得到当前时间和当前进程的进程名和完整路径
//
//

#include "xprecomp.h"
#pragma hdrstop

#define ONE_DAY_SECONDS			86400
#define SECONDS_OF_1970_1601	3054539008	// 1970-01-01 - 1601-01-01 's seconds
#define WEEK_OF_1970_01_01		5			// 1970-01-01 is Saturday
#define HOUR_8_SECONDS			28800

//
// 得到当前日期/时间、星期和时间
//
// 参数：
//		Week:	返回星期，1、2 ... 7 分别表示 星期日、星期一 ... 星期6
//		pTime:	返回当前时间在当天的秒数
//
// 返回值：
//		ULONG数值表示的日期/时间，保存的是从1970/1/1到现在的秒数，与CTime表示形式相同
//
//
//
ULONG GetCurrentTime(unsigned char* Week, ULONG* pTime)
{
	LARGE_INTEGER SystemTime;
	ULONG GmtDateTime, DateTime, Date, Time;
	KeQuerySystemTime(&SystemTime);
	GmtDateTime = (ULONG)(SystemTime.QuadPart / 10000000 - SECONDS_OF_1970_1601);
	ExSystemTimeToLocalTime(&SystemTime, &SystemTime);
	DateTime = (ULONG)(SystemTime.QuadPart / 10000000 - SECONDS_OF_1970_1601);
	Date = DateTime / ONE_DAY_SECONDS;
	Time = DateTime % ONE_DAY_SECONDS;
	*Week = (unsigned char)(Date % 7 + WEEK_OF_1970_01_01);
	if(*Week > 7)*Week -= 7;
	*pTime = Time;
	return GmtDateTime;
}

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
// 备注：
//		由于在 Windows 2000 得到完整路径名称时有时会发生Dispatch Level的蓝屏错误
//		所以在这里不返回完整路径
//
//
INT GetProcessFileName(char* buf, DWORD nSize, BOOL IsOnlyName)
{
	PCWSTR pFullName = NULL;
	int i, j;

	if(buf == 0 || nSize == 0)
		return PS_INVALID_PARAMETER;

	if((!IsOnlyName && nSize < MAX_PATH) || nSize < 16)
		return PS_BUFFER_SO_SMALL;

	//
	// 2002/08/11 remove
	//
	// not get full path name
	//{
	//	char* pName = PsGetProcessName();
	//	if(pName == NULL)
	//		strcpy(buf, "SYSTEM");
	//	else
	//		strcpy(buf, pName);
	//	return PS_SYSTEM_PROCESS;
	//}

	//
	// 2002/08/11 add
	//
	if(KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
		strcpy(buf, "SYSTEM");
		return PS_SYSTEM_PROCESS;
	}


	if(IsOnlyName)
	{
		char* pName = PsGetProcessName();
		if(pName == NULL)
			strcpy(buf, "SYSTEM");
		else
			strcpy(buf, pName);
		return PS_SYSTEM_PROCESS;
	}

	pFullName = PsGetModuleFileNameW();
	if(pFullName == NULL)
	{
		char* pName = PsGetProcessName();
		if(pName == NULL)
			strcpy(buf, "SYSTEM");
		else
			strcpy(buf, pName);
		return PS_SYSTEM_PROCESS;
	}
	else
	{
		UNICODE_STRING usFileName;
		ANSI_STRING asFileName;

		RtlInitUnicodeString(&usFileName, pFullName);

		asFileName.Length = 0;
		asFileName.MaximumLength = MAX_PATH;
		asFileName.Buffer = buf;

		RtlUnicodeStringToAnsiString(&asFileName, &usFileName, FALSE);
	}

	return PS_USER_PROCESS;
}

//
// 得到完整路径的地址，完整路径为UNICODE字符串
//
PCWSTR PsGetModuleFileNameW()
{
	DWORD dwAddress;

	//
	// 2002/06/05 add
	//
	if(KeGetCurrentIrql() != PASSIVE_LEVEL)
		return NULL;

	//
	// 2002/08/11 IoGetCurrentProcess 修改为 PsGetCurrentProcess 
	//
	dwAddress = (DWORD)PsGetCurrentProcess();

	if(dwAddress == 0 || dwAddress == 0xFFFFFFFF)
		return NULL;
	dwAddress += BASE_PROCESS_PEB_OFFSET;
	if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;
	dwAddress += BASE_PEB_PROCESS_PARAMETER_OFFSET;
	if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;
	dwAddress += BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME;
	if((dwAddress = *(DWORD*)dwAddress) == 0) return 0;
	return (PCWSTR)dwAddress;
}

//
// 2002/06/05 add
// 得到当前操作系统版本
//
#define WINDOWS_VERSION_NONE		0
#define WINDOWS_VERSION_2000		1
#define WINDOWS_VERSION_XP			2
int GetWindowsVersion()
{
	ULONG MajorVersion, MinorVersion;

	PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);

	if(MajorVersion == 5 && MinorVersion == 0)
		return WINDOWS_VERSION_2000;
	else
		return WINDOWS_VERSION_XP;

	return WINDOWS_VERSION_NONE;
}

//
// 得到当前进程的进程名
//
char* PsGetProcessName()
{
	char* pImageName;
	static int WindowsVersion = WINDOWS_VERSION_NONE;

	//
	// 2002/06/05 add
	//
	if(KeGetCurrentIrql() != PASSIVE_LEVEL)
		return NULL;

	if(WindowsVersion == WINDOWS_VERSION_NONE)
		WindowsVersion = GetWindowsVersion();

	pImageName = (char*)PsGetCurrentProcess();
	if(pImageName == NULL || pImageName == (char*)0xFFFFFFFF)
		return NULL;

	//
	// 2002/06/05 modify for Windows XP
	//
	switch(WindowsVersion)
	{
	case WINDOWS_VERSION_2000:
		pImageName += BASE_PROCESS_NAME_OFFSET;
		break;
	case WINDOWS_VERSION_XP:
		pImageName += BASE_PROCESS_NAME_OFFSET_XP;
		break;
	default:
		return NULL;
	}

	return pImageName;
}


#pragma comment( exestr, "B9D3B8FD2A727471656775752B")
