//////////////////////////////////////////////////////////
// P2PClient.cpp�ļ�



#include <winsock2.h>
#include <stdio.h>
#include "p2pclient.h"

class CMyP2P : public CP2PClient
{
public:

	void OnRecv(char *pszUserName, char *pszData, int nDataLen)
	{
		pszData[nDataLen] = '\0';
		printf(" Recv a Message from %s :  %s \n", pszUserName, pszData);
	}
};

void main()
{	
	CMyP2P client;
	if(!client.Init(0))
	{
		printf(" CP2PClient::Init() failed \n");
		return ;
	}

	// ��ȡ������IP��ַ���û���
	char szServerIp[20];
	char szUserName[MAX_USERNAME];
	printf(" Please input server ip: ");
	gets(szServerIp);
	printf(" Please input your name: ");
	gets(szUserName);
	// ��½������
	if(!client.Login(szUserName, szServerIp))
	{
		printf(" CP2PClient::Login() failed \n");
		return ;
	}
	// ��һ�ε�½�����ȸ����û��б�
	client.GetUserList();
	// ����ǰ״̬�ͱ�������÷�������û�
	printf(" %s has successfully logined server \n", szUserName);
	printf("\n Commands are: \"getu\", \"send\", \"exit\" \n");
	// ѭ�������û�����
	char szCommandLine[256];
	while(TRUE)
	{
		gets(szCommandLine);
		if(strlen(szCommandLine) < 4)
			continue;

		// ����������
		char szCommand[10];
		strncpy(szCommand, szCommandLine, 4);
		szCommand[4] = '\0';
		if(stricmp(szCommand, "getu") == 0)
		{
			// ��ȡ�û��б�
			if(client.GetUserList())
			{
				printf(" Have %d users logined server: \n", client.m_PeerList.m_nCurrentSize);
				for(int i=0; i<client.m_PeerList.m_nCurrentSize; i++)
				{
					PEER_INFO *pInfo = &client.m_PeerList.m_pPeer[i];
					printf(" Username: %s(%s:%ld) \n", pInfo->szUserName, 
						::inet_ntoa(*((in_addr*)&pInfo->addr[pInfo->AddrNum -1].dwIp)), pInfo->addr[pInfo->AddrNum - 1].nPort);
				}
			}
			else
			{
				printf(" Get User List Failure !\n");
			}
		}

		else if(stricmp(szCommand, "send") == 0)
		{
			// �������Է��û���
			char szPeer[MAX_USERNAME];
			int i=5;
			for(i;;i++)
			{
				if(szCommandLine[i] != ' ')
					szPeer[i-5] = szCommandLine[i];
				else
				{
					szPeer[i-5] = '\0';
					break;
				}	
			}
			
			// ������Ҫ���͵���Ϣ
			char szMsg[56];
			strcpy(szMsg, &szCommandLine[i+1]);

			// ������Ϣ
			if(client.SendText(szPeer, szMsg, strlen(szMsg)))
				printf(" Send OK! \n");
			else
				printf(" Send Failure! \n");
			
		}
		else if(stricmp(szCommand, "exit") == 0)
		{
			break;
		}
	}
}

