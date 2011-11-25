//////////////////////////////////////////////////////
// select.cpp�ļ�


#include "../common/initsock.h"
#include <stdio.h>

CInitSock theSock;		// ��ʼ��Winsock��
int main()
{
	USHORT nPort = 4567;	// �˷����������Ķ˿ں�

	// ���������׽���
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	// ���׽��ֵ����ػ���
	if(::bind(sListen, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf(" Failed bind() \n");
		return -1;
	}
	// �������ģʽ
	::listen(sListen, 5);

		// selectģ�ʹ������
	// 1����ʼ��һ���׽��ּ���fdSocket����Ӽ����׽��־�����������
	fd_set fdSocket;		// ���п����׽��ּ���
	FD_ZERO(&fdSocket);
	FD_SET(sListen, &fdSocket);
	while(TRUE)
	{
		// 2����fdSocket���ϵ�һ������fdRead���ݸ�select������
		// �����¼�����ʱ��select�����Ƴ�fdRead������û��δ��I/O�������׽��־����Ȼ�󷵻ء�
		fd_set fdRead = fdSocket;
		int nRet = ::select(0, &fdRead, NULL, NULL, NULL);
		if(nRet > 0)
		{
			// 3��ͨ����ԭ��fdSocket������select�������fdRead���ϱȽϣ�
			// ȷ��������Щ�׽�����δ��I/O������һ��������ЩI/O��
			for(int i=0; i<(int)fdSocket.fd_count; i++)
			{
				if(FD_ISSET(fdSocket.fd_array[i], &fdRead))
				{
					if(fdSocket.fd_array[i] == sListen)		// ��1�������׽��ֽ��յ�������
					{
						if(fdSocket.fd_count < FD_SETSIZE)
						{
							sockaddr_in addrRemote;
							int nAddrLen = sizeof(addrRemote);
							SOCKET sNew = ::accept(sListen, (SOCKADDR*)&addrRemote, &nAddrLen);
							FD_SET(sNew, &fdSocket);
							printf("���յ����ӣ�%s��\n", ::inet_ntoa(addrRemote.sin_addr));
						}
						else
						{
							printf(" Too much connections! \n");
							continue;
						}
					}
					else
					{
						char szText[256];
						int nRecv = ::recv(fdSocket.fd_array[i], szText, strlen(szText), 0);
						if(nRecv > 0)						// ��2���ɶ�
						{
							szText[nRecv] = '\0';
							printf("���յ����ݣ�%s \n", szText);
						}
						else								// ��3�����ӹرա����������ж�
						{
							::closesocket(fdSocket.fd_array[i]);
							FD_CLR(fdSocket.fd_array[i], &fdSocket);
						}
					}
				}
			}
		}
		else
		{
			printf(" Failed select() \n");
			break;
		}
	}
	return 0;
}