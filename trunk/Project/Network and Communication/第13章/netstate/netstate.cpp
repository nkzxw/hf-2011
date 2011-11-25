//////////////////////////////////////////////////////////////////////
// netstate.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")




typedef struct 
{
  DWORD   dwState;        	// ����״̬
  DWORD   dwLocalAddr;    	// ���ص�ַ
  DWORD   dwLocalPort;    	// ���ض˿�
  DWORD   dwRemoteAddr;   	// Զ�̵�ַ
  DWORD   dwRemotePort;   	// Զ�̶˿�
  DWORD	  dwProcessId;		// ����ID��
} MIB_TCPEXROW, *PMIB_TCPEXROW;

typedef struct 
{
	DWORD			dwNumEntries;
	MIB_TCPEXROW	table[ANY_SIZE];
} MIB_TCPEXTABLE, *PMIB_TCPEXTABLE;

typedef struct 
{
  DWORD   dwLocalAddr;    	// ���ص�ַ
  DWORD   dwLocalPort;    	// ���ض˿�
  DWORD	  dwProcessId;		// ����ID��
} MIB_UDPEXROW, *PMIB_UDPEXROW;

typedef struct 
{
	DWORD			dwNumEntries;
	MIB_UDPEXROW	table[ANY_SIZE];
} MIB_UDPEXTABLE, *PMIB_UDPEXTABLE;


// ��չ����ԭ��
typedef DWORD (WINAPI *PFNAllocateAndGetTcpExTableFromStack)(
  PMIB_TCPEXTABLE *pTcpTable, 
  BOOL bOrder,             
  HANDLE heap,
  DWORD zero,
  DWORD flags
);

typedef DWORD (WINAPI *PFNAllocateAndGetUdpExTableFromStack)(
  PMIB_UDPEXTABLE *pUdpTable,  
  BOOL bOrder,              
  HANDLE heap,
  DWORD zero,
  DWORD flags
);


PCHAR ProcessPidToName(HANDLE hProcessSnap, DWORD ProcessId, PCHAR ProcessName);

int main()
{
	// ������չ����ָ��
	PFNAllocateAndGetTcpExTableFromStack pAllocateAndGetTcpExTableFromStack;
	PFNAllocateAndGetUdpExTableFromStack pAllocateAndGetUdpExTableFromStack;

	// ��ȡ��չ��������ڵ�ַ	
	HMODULE hModule = ::LoadLibrary("iphlpapi.dll");
	pAllocateAndGetTcpExTableFromStack = 
			(PFNAllocateAndGetTcpExTableFromStack)::GetProcAddress(hModule, 
									"AllocateAndGetTcpExTableFromStack");
	
	pAllocateAndGetUdpExTableFromStack = 
			(PFNAllocateAndGetUdpExTableFromStack)::GetProcAddress(hModule, 
									"AllocateAndGetUdpExTableFromStack");

	if(pAllocateAndGetTcpExTableFromStack == NULL || pAllocateAndGetUdpExTableFromStack == NULL)
	{
		printf(" Ex APIs are not present \n ");
		// ˵����Ӧ�õ�����ͨ��IP����APIȥ��ȡTCP���ӱ��UDP������
		return 0;
	}

	// ������չ��������ȡTCP��չ���ӱ��UDP��չ������

	PMIB_TCPEXTABLE pTcpExTable;
	PMIB_UDPEXTABLE pUdpExTable;

	// pTcpExTable��pUdpExTable��ָ�Ļ������Զ�����չ�����ڽ��̶�������
	if(pAllocateAndGetTcpExTableFromStack(&pTcpExTable, TRUE, GetProcessHeap(), 2, 2) != 0)
	{
			printf(" Failed to snapshot TCP endpoints.\n");
			return -1;
	}
	if(pAllocateAndGetUdpExTableFromStack(&pUdpExTable, TRUE, GetProcessHeap(), 2, 2) != 0)
	{
			printf(" Failed to snapshot UDP endpoints.\n");
			return -1;
	}

	// ��ϵͳ�ڵ����н�����һ������
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printf(" Failed to take process snapshot. Process names will not be shown.\n\n");
		return -1;
	}

	printf(" Active Connections \n\n");
	char	szLocalAddr[128];
	char	szRemoteAddr[128];
	char	szProcessName[128];
	in_addr inadLocal, inadRemote;
	char    strState[128];
	DWORD   dwRemotePort = 0;	

	// ��ӡTCP��չ���ӱ���Ϣ
	for(UINT i = 0; i < pTcpExTable->dwNumEntries; ++i)
	{
		// ״̬
		switch (pTcpExTable->table[i].dwState)
		{
		case MIB_TCP_STATE_CLOSED:
			strcpy(strState, "CLOSED");
			break;
		case MIB_TCP_STATE_TIME_WAIT:
			strcpy(strState, "TIME_WAIT");
			break;
		case MIB_TCP_STATE_LAST_ACK:
			strcpy(strState, "LAST_ACK");
			break;
		case MIB_TCP_STATE_CLOSING:
			strcpy(strState, "CLOSING");
			break;
		case MIB_TCP_STATE_CLOSE_WAIT:
			strcpy(strState, "CLOSE_WAIT");
			break;
		case MIB_TCP_STATE_FIN_WAIT1:
			strcpy(strState, "FIN_WAIT1");
			break;
		case MIB_TCP_STATE_ESTAB:
			strcpy(strState, "ESTAB");
			break;
		case MIB_TCP_STATE_SYN_RCVD:
			strcpy(strState, "SYN_RCVD");
			break;
		case MIB_TCP_STATE_SYN_SENT:
			strcpy(strState, "SYN_SENT");
			break;
		case MIB_TCP_STATE_LISTEN:
			strcpy(strState, "LISTEN");
			break;
		case MIB_TCP_STATE_DELETE_TCB:
			strcpy(strState, "DELETE");
			break;
		default:
			printf("Error: unknown state!\n");
			break;
		}
		// ����IP��ַ
		inadLocal.s_addr = pTcpExTable->table[i].dwLocalAddr;
		
		// Զ�̶˿�
		if(strcmp(strState, "LISTEN") != 0)
		{
			dwRemotePort = pTcpExTable->table[i].dwRemotePort;
		}
		else
			dwRemotePort = 0;

		// Զ��IP��ַ
		inadRemote.s_addr = pTcpExTable->table[i].dwRemoteAddr;
		

		sprintf(szLocalAddr, "%s:%u", inet_ntoa(inadLocal), 
					ntohs((unsigned short)(0x0000FFFF & pTcpExTable->table[i].dwLocalPort)));
		sprintf(szRemoteAddr, "%s:%u", inet_ntoa(inadRemote), 
					ntohs((unsigned short)(0x0000FFFF & dwRemotePort)));

		// ��ӡ������ڵ���Ϣ
		printf("%-5s %s:%d\n      State:   %s\n", "[TCP]", 
			ProcessPidToName(hProcessSnap, pTcpExTable->table[i].dwProcessId, szProcessName),
			pTcpExTable->table[i].dwProcessId,
			strState);

		printf("      Local:   %s\n      Remote:  %s\n",
			szLocalAddr, szRemoteAddr);
	}

	// ��ӡUDP��������Ϣ
	for(i = 0; i < pUdpExTable->dwNumEntries; ++i)
	{
		// ����IP��ַ
		inadLocal.s_addr = pUdpExTable->table[i].dwLocalAddr;

		sprintf(szLocalAddr,  "%s:%u", inet_ntoa(inadLocal), 
				ntohs((unsigned short)(0x0000FFFF & pUdpExTable->table[i].dwLocalPort)));

		// ��ӡ������ڵ���Ϣ
		printf("%-5s %s:%d\n", "[UDP]", 
			ProcessPidToName(hProcessSnap, pUdpExTable->table[i].dwProcessId, szProcessName),
			pUdpExTable->table[i].dwProcessId );
		printf("      Local:   %s\n      Remote:  %s\n",
			szLocalAddr, "*.*.*.*:*" );
	}
	
	
	::CloseHandle(hProcessSnap);
	::LocalFree(pTcpExTable);
	::LocalFree(pUdpExTable);
	::FreeLibrary(hModule);
	return 0;
}


// ������ID�ţ�PID��ת��Ϊ��������
PCHAR ProcessPidToName(HANDLE hProcessSnap, DWORD ProcessId, PCHAR ProcessName)
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(processEntry);
	// �Ҳ����Ļ���Ĭ�Ͻ�����Ϊ��???��
	strcpy(ProcessName, "???");
	if(!::Process32First(hProcessSnap, &processEntry)) 
		return ProcessName;
	do 
	{
		if(processEntry.th32ProcessID == ProcessId) // �����������
		{
			strcpy(ProcessName, processEntry.szExeFile);
			break;
		}
	}
	while(::Process32Next(hProcessSnap, &processEntry));
	return ProcessName;
}