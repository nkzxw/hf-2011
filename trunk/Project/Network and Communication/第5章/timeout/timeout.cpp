//////////////////////////////////////////////////////////
// timeout.cpp�ļ�


#include "../common/InitSock.h"
#include <stdio.h>
CInitSock initSock;		// ��ʼ��Winsock��

int main()
{
	// �����׽���
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == INVALID_SOCKET)
	{
		printf("Failed socket() \n");
		return 0;
	}
	
	// Ҳ�������������bind������һ�����ص�ַ
	// ����ϵͳ�����Զ�����
		
	// ��дԶ�̵�ַ��Ϣ
	sockaddr_in addr; 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(4567);
	// ע�⣬����Ҫ��д����������TCPServer�������ڻ�����IP��ַ
	// �����ļ����û��������ֱ��ʹ��127.0.0.1����
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	// ��������
	char szText[] = " TCP Server Demo! \r\n";
	::sendto(s, szText, strlen(szText), 0, (sockaddr*)&addr, sizeof(addr));

	::closesocket(s);
	return 0;
}