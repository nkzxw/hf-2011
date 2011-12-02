#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "..\\IOCTL.h"

#pragma comment( lib, "shlwapi" )

#define		EXE_DRIVER_NAME		_T("SSDT")
#define		DISPLAY_NAME			_T("SSDT Driver")
#define		SystemModuleInfo	0x0B
//#define		QuerySize			0xFA00

typedef ULONG \
(__stdcall *pNtQuerySystemInformationProto)( \
			IN ULONG SysInfoClass,
			IN OUT PVOID SystemInformation,
			IN ULONG SystemInformationLength,
			OUT PULONG nRet
			);

#pragma pack( push, 1 )
typedef struct _SSDTEntry
{
	BYTE byMov;		//B8 => MOV EAX, XXXX
	ULONG ulIndex;	//Service Number
} SSDTEntry, *pSSDTEntry;
#pragma pack( pop )

typedef struct _SYSTEM_MODULE_INFORMATION { 
    ULONG Reserved[2]; 
    PVOID Base; 
    ULONG Size; 
    ULONG Flags; 
    USHORT Index; 
    USHORT Unknown; 
    USHORT LoadCount; 
    USHORT ModuleNameOffset; 
    CHAR ImageName[256]; 
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _tagSysModuleList {
    ULONG ulCount;
    SYSTEM_MODULE_INFORMATION smi[1];
} SYSMODULELIST, *PSYSMODULELIST;

typedef struct _SSDTSaveTable		//����SSDT��Ϣ
{
	ULONG ulServiceNumber;				//�����
	ULONG ulCurrentFunctionAddress;		//��ǰ������ַ
	ULONG ulOriginalFunctionAddress;	//ԭʼ������ַ
	char ServiceFunctionName[MAX_PATH+1];	//��������
	char ModuleName[MAX_PATH+1];		//ģ����
} SSDTSaveTable, *pSSDTSaveTable;
///////////////////////////////////////////////////
//              ��������
///////////////////////////////////////////////////
//��������
HANDLE LoadDriver( IN LPCTSTR lpFileName );
//ж������
void UnloadDriver( IN HANDLE hDriver );
//�õ�SSDT
BOOL GetSSDT( IN HANDLE hDriver, OUT PSSDT ssdt );
//����SSDT
BOOL SetSSDT( IN HANDLE hDriver, IN PSSDT ssdt );
//�õ�SSDT����HOOK�ĵ�ַ
BOOL GetHook( IN HANDLE hDriver, IN ULONG ulIndex, OUT PULONG ulAddr );
//����SSDT����HOOK�ĵ�ַ
BOOL SetHook( IN HANDLE hDriver, IN ULONG ulIndex, IN OUT PULONG ulAddr );
//ö��SSDT
BOOL EnumSSDT( IN HANDLE hDriver );
//�ָ�SSDT
BOOL ReSSDT( IN HANDLE hDriver );
//�ָ�SSDT��ȥ����ϵͳ��
BOOL ReSSDTAndThrowSpilth( IN HANDLE hDriver );

///////////////////////////////////////////////////