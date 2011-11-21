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
//		�õ���ǰʱ��͵�ǰ���̵Ľ�����������·��
//
//

#include "xprecomp.h"
#pragma hdrstop

#define ONE_DAY_SECONDS			86400
#define SECONDS_OF_1970_1601	3054539008	// 1970-01-01 - 1601-01-01 's seconds
#define WEEK_OF_1970_01_01		5			// 1970-01-01 is Saturday
#define HOUR_8_SECONDS			28800

//
// �õ���ǰ����/ʱ�䡢���ں�ʱ��
//
// ������
//		Week:	�������ڣ�1��2 ... 7 �ֱ��ʾ �����ա�����һ ... ����6
//		pTime:	���ص�ǰʱ���ڵ��������
//
// ����ֵ��
//		ULONG��ֵ��ʾ������/ʱ�䣬������Ǵ�1970/1/1�����ڵ���������CTime��ʾ��ʽ��ͬ
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
// ��ע��
//		������ Windows 2000 �õ�����·������ʱ��ʱ�ᷢ��Dispatch Level����������
//		���������ﲻ��������·��
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
// �õ�����·���ĵ�ַ������·��ΪUNICODE�ַ���
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
	// 2002/08/11 IoGetCurrentProcess �޸�Ϊ PsGetCurrentProcess 
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
// �õ���ǰ����ϵͳ�汾
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
// �õ���ǰ���̵Ľ�����
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
