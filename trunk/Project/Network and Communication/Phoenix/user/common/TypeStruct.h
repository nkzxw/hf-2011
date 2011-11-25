/////////////////////////////////////////////
// TypeStruct.h�ļ�



#include "PMacRes.h"
#include "iocommon.h"

#ifndef __TYPESTRUCT_H__
#define __TYPESTRUCT_H__

////////////////////////////////////////////

// �û�����˹���
struct RULE_ITEM 
{
	TCHAR szApplication[MAX_PATH];	// Ӧ�ó�������
	UCHAR ucAction;					// ��ȡ�Ķ���
	UCHAR ucDirection;				// ����
	UCHAR ucServiceType;			// ��������
	USHORT usServicePort;			// ����˿�
	TCHAR sDemo[51];				// ����
};

// �ļ�ͷ�ṹ
struct RULE_FILE_HEADER
{
	TCHAR szSignature[12];	// �ļ�ǩ��
	ULONG ulHeaderLength;	// ͷ����

	UCHAR ucMajorVer;		// ���汾��
	UCHAR ucMinorVer;		// �ΰ汾��
	DWORD dwVersion;		// �ļ�ͷ�汾

	UCHAR ucLspWorkMode;	// ����ģʽ
	UCHAR ucKerWorkMode;
	UCHAR bAutoStart;		// �Ƿ񿪻��Զ�����

	TCHAR szWebURL[MAX_PATH];	// ������Webҳ��
	TCHAR szEmail[MAX_PATH];	// ������Email��ַ

	ULONG ulLspRuleCount;		// �û�����˹���ĸ���
	ULONG ulKerRuleCount;		// ���Ĳ���˹���ĸ���
};

// �ļ��ṹ
struct RULE_FILE
{
	RULE_FILE_HEADER header;
	RULE_ITEM LspRules[MAX_RULE_COUNT];
	PassthruFilter KerRules[MAX_RULE_COUNT];
};



///////////////////////////////////
// �Ự

struct SESSION 
{
	SOCKET s;
	int nProtocol;				// ʹ�õ�Э�飬��TCP��UDP��FTP��
	UCHAR ucDirection;			// ����
	ULONG ulLocalIP;			// ����IP��ַ
	ULONG ulRemoteIP;			// Զ��IP��ַ
	USHORT usLocalPort;			// ���ض˿ں�
	USHORT usRemotePort;		// Զ�̶˿ں�
	TCHAR szPathName[MAX_PATH];	// Ӧ�ó���
	UCHAR ucAction;				// �Դ�Session�ķ�Ӧ
	// others   �� ����ʱ�䡢���͵��ֽ�����
};


struct QUERY_SESSION  
{
	UCHAR bUsed;				// ָʾ�˽ṹ�Ƿ�ʹ��
	TCHAR szPathName[MAX_PATH];	// Ӧ�ó�����
	int nReturnValue;			// ��ģ��ķ���ֵ
};

// DLL IO���ƽṹ
struct LSP_IO_CONTROL     // IO���ƺ����Ĳ�������
{
	UCHAR ucWorkMode;				// ����ģʽ
	RULE_FILE *pRuleFile;			// �����ļ�

	HWND hPhoenixWnd;				// ����LSP��Ϣ�Ĵ���

	TCHAR szPath[MAX_PATH];			// ������·��

	SESSION *pSession;				// ����ȡ��һ��Session
	int nSessionIndex;
};

typedef int  (__stdcall * PFNLSPIoControl)(LSP_IO_CONTROL *pIoControl, int nType);



















#endif // __TYPESTRUCT_H__