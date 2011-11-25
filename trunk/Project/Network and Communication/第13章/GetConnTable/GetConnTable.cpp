//////////////////////////////////////////////////////////////////////
// GetInterfaceInfo.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")


PMIB_TCPTABLE MyGetTcpTable(BOOL bOrder);
PMIB_UDPTABLE MyGetUdpTable(BOOL bOrder);

void MyFreeTcpTable(PMIB_TCPTABLE pTcpTable);
void MyFreeUdpTable(PMIB_UDPTABLE pUdpTable);


int main()
{
	// ��ӡTCP���ӱ���Ϣ
	PMIB_TCPTABLE pTcpTable = MyGetTcpTable(TRUE);
	if(pTcpTable != NULL)
	{
		char    strState[128];
		struct  in_addr    inadLocal, inadRemote;
		DWORD   dwRemotePort = 0;
		char    szLocalIp[128];
		char    szRemIp[128];

		printf("TCP TABLE\n");
		printf("%20s %10s %20s %10s %s\n", "Loc Addr", "Loc Port", "Rem Addr",
			"Rem Port", "State");
		for(UINT i = 0; i < pTcpTable->dwNumEntries; ++i)
		{
			// ״̬
			switch (pTcpTable->table[i].dwState)
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
			// ���ص�ַ
			inadLocal.s_addr = pTcpTable->table[i].dwLocalAddr;
			
			// Զ�̶˿�
			if (strcmp(strState, "LISTEN") != 0)
			{
				dwRemotePort = pTcpTable->table[i].dwRemotePort;
			}
			else
				dwRemotePort = 0;
			// Զ��IP��ַ
			inadRemote.s_addr = pTcpTable->table[i].dwRemoteAddr;

			strcpy(szLocalIp, inet_ntoa(inadLocal));
			strcpy(szRemIp, inet_ntoa(inadRemote));
			printf("%20s %10u %20s %10u %s\n", 
				szLocalIp,  ntohs((unsigned short)(0x0000FFFF & pTcpTable->table[i].dwLocalPort)),
				szRemIp, ntohs((unsigned short)(0x0000FFFF & dwRemotePort)),
				strState);
		}
		MyFreeTcpTable(pTcpTable);
	}

	printf(" \n\n");

	// ��ӡUDP��������Ϣ
	PMIB_UDPTABLE pUdpTable = MyGetUdpTable(TRUE);
	if(pUdpTable != NULL)
	{
		struct in_addr inadLocal;
		printf("UDP TABLE\n");

        printf("%20s %10s\n", "Loc Addr", "Loc Port");
        for (UINT i = 0; i < pUdpTable->dwNumEntries; ++i)
        {
            inadLocal.s_addr = pUdpTable->table[i].dwLocalAddr;
			// ��ӡ������ڵ���Ϣ
            printf("%20s %10u \n", 
                inet_ntoa(inadLocal), ntohs((unsigned short)(0x0000FFFF & pUdpTable->table[i].dwLocalPort)));
        }

		MyFreeUdpTable(pUdpTable);
	}
		
	return 0;
}


PMIB_TCPTABLE MyGetTcpTable(BOOL bOrder)
{
	PMIB_TCPTABLE pTcpTable = NULL;
    DWORD dwActualSize = 0;


    // ��ѯ���軺�����Ĵ�С
    if(::GetTcpTable(pTcpTable, &dwActualSize, bOrder) == ERROR_INSUFFICIENT_BUFFER)
	{
        // ΪMIB_TCPTABLE�ṹ�����ڴ�
        pTcpTable = (PMIB_TCPTABLE)::GlobalAlloc(GPTR, dwActualSize);
        // ��ȡTCP���ӱ�
        if(::GetTcpTable(pTcpTable, &dwActualSize, bOrder) == NO_ERROR)
			return pTcpTable;
		::GlobalFree(pTcpTable);
    }
	return NULL;
}

void MyFreeTcpTable(PMIB_TCPTABLE pTcpTable)
{
	if(pTcpTable != NULL)
		::GlobalFree(pTcpTable);
}


PMIB_UDPTABLE MyGetUdpTable(BOOL bOrder)
{
	PMIB_UDPTABLE pUdpTable = NULL;
    DWORD dwActualSize = 0;

    // ��ѯ���軺�����Ĵ�С
    if(::GetUdpTable(pUdpTable, &dwActualSize, bOrder) == ERROR_INSUFFICIENT_BUFFER)
	{
        // ΪMIB_UDPTABLE�ṹ�����ڴ�
        pUdpTable = (PMIB_UDPTABLE)::GlobalAlloc(GPTR, dwActualSize);
        // ��ȡUDP������
        if(::GetUdpTable(pUdpTable, &dwActualSize, bOrder) == NO_ERROR)
			return pUdpTable;
		::GlobalFree(pUdpTable);
    }
	return NULL;
}

void MyFreeUdpTable(PMIB_UDPTABLE pUdpTable)
{
	if(pUdpTable != NULL)
		::GlobalFree(pUdpTable);
}


