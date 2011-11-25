//////////////////////////////////////////////////////////
// NetTime.cpp�ļ�


#include "../common/InitSock.h"
#include <stdio.h>
CInitSock initSock;	

void SetTimeFromTP(ULONG ulTime)	// ����ʱ��Э�鷵�ص�ʱ������ϵͳʱ��
{
	// Windows�ļ�ʱ����һ��64λ��ֵ�����Ǵ�1601��1��1������12:00�����ڵ�ʱ������
	// ��λ��1/1000 0000�룬��1000���֮1�루100-nanosecond ��
	FILETIME ft;
	SYSTEMTIME st;

	// ���Ƚ���׼ʱ�䣨1900��1��1��0��0��0��0���룩ת��ΪWindows�ļ�ʱ��	
	st.wYear = 1900;
	st.wMonth = 1;
	st.wDay = 1;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft);

	// Ȼ��Time Protocolʹ�õĻ�׼ʱ������Լ���ȥ��ʱ�䣬��ulTime
	LONGLONG *pLLong = (LONGLONG *)&ft;
	// ע�⣬�ļ�ʱ�䵥λ��1/1000 0000�룬��1000���֮1�루100-nanosecond ��
	*pLLong += (LONGLONG)10000000 * ulTime; 

	// �ٽ�ʱ��ת������������ϵͳʱ��
	FileTimeToSystemTime(&ft, &st);	
	SetSystemTime(&st);
}

int main()
{
	SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s == INVALID_SOCKET)
	{
		printf(" Failed socket() \n");
		return 0;
	}

	// ��дԶ�̵�ַ��Ϣ�����ӵ�ʱ�������
	sockaddr_in servAddr; 
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(37); 
	// ����ʹ�õ�ʱ���������129.132.2.21�������ַ��ο�http://tf.nist.gov/service/its.htm
	servAddr.sin_addr.S_un.S_addr = inet_addr("129.132.2.21");
	if(::connect(s, (sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		printf(" Failed connect() \n");
		return 0;
	}
	
	// �ȴ�����ʱ��Э�鷵�ص�ʱ�䡣ѧϰ��Winsock I/Oģ��֮�����ʹ���첽I/O���Ա����ó�ʱ
	ULONG ulTime = 0;
	int nRecv = ::recv(s, (char*)&ulTime, sizeof(ulTime), 0);
	if(nRecv > 0)
	{
		ulTime = ntohl(ulTime);
		SetTimeFromTP(ulTime);
		printf(" �ɹ���ʱ���������ʱ��ͬ����\n");
	}
	else
	{
		printf(" ʱ�����������ȷ����ǰʱ�䣡\n");
	}
	
	::closesocket(s);

	getchar ();

	return 0;
}


