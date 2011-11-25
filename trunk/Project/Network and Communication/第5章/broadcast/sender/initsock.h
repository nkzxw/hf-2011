// initsock.h

#include <winsock2.h>
#pragma comment(lib, "WS2_32")

class CInitSock
{
public:
	CInitSock();
	~CInitSock();
};

inline CInitSock::CInitSock()
{
	// ��ʼ��WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if(::WSAStartup(sockVersion, &wsaData) != 0)
	{
		exit(0);
	}
}

inline CInitSock::~CInitSock()
{
	::WSACleanup();	
}