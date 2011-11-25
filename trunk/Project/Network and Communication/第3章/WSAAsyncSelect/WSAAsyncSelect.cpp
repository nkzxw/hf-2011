/////////////////////////////////////////////// 
// WSAAsyncSelect.cpp�ļ�

#include "../common/initsock.h"
#include <stdio.h>

#define WM_SOCKET WM_USER + 101		// �Զ�����Ϣ
CInitSock theSock;


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int main()
{
	char szClassName[] = "MainWClass";	
	WNDCLASSEX wndclass;
	// �����������ڵĲ������WNDCLASSEX�ṹ
	wndclass.cbSize = sizeof(wndclass);	
	wndclass.style = CS_HREDRAW|CS_VREDRAW;	
	wndclass.lpfnWndProc = WindowProc;	
	wndclass.cbClsExtra = 0;		
	wndclass.cbWndExtra = 0;		
	wndclass.hInstance = NULL;		
	wndclass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);	
	wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);		
	wndclass.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);	
	wndclass.lpszMenuName = NULL;		
	wndclass.lpszClassName = szClassName ;
	wndclass.hIconSm = NULL;	
	::RegisterClassEx(&wndclass); 
	// ����������
	HWND hWnd = ::CreateWindowEx( 
		0,						
		szClassName,			
		"",	
		WS_OVERLAPPEDWINDOW,	
		CW_USEDEFAULT,	
		CW_USEDEFAULT,		
		CW_USEDEFAULT,	
		CW_USEDEFAULT,			
		NULL,				
		NULL,		
		NULL,	
		NULL);		
	if(hWnd == NULL)
	{
		::MessageBox(NULL, "�������ڳ���", "error", MB_OK);
		return -1;
	}

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

	// ���׽�����Ϊ����֪ͨ��Ϣ���͡�
	::WSAAsyncSelect(sListen, hWnd, WM_SOCKET, FD_ACCEPT|FD_CLOSE);

	// �������ģʽ
	::listen(sListen, 5);

	// ����Ϣ������ȡ����Ϣ
	MSG msg;	
	while(::GetMessage(&msg, NULL, 0, 0))
	{
		// ת��������Ϣ
		::TranslateMessage(&msg);
		// ����Ϣ���͵���Ӧ�Ĵ��ں���
		::DispatchMessage(&msg);
	}
	// ��GetMessage����0ʱ�������
	return msg.wParam;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{	
	case WM_SOCKET:
		{
			// ȡ�����¼��������׽��־��
			SOCKET s = wParam;
			// �鿴�Ƿ����
			if(WSAGETSELECTERROR(lParam))
			{
				::closesocket(s);
				return 0;
			}
			// ���������¼�
			switch(WSAGETSELECTEVENT(lParam))
			{
			case FD_ACCEPT:		// �����е��׽��ּ�⵽�����ӽ���
				{
					SOCKET client = ::accept(s, NULL, NULL);
					::WSAAsyncSelect(client, hWnd, WM_SOCKET, FD_READ|FD_WRITE|FD_CLOSE);
				}
				break;
			case FD_WRITE:
				{
				}
				break;
			case FD_READ:
				{
					char szText[1024] = { 0 };
					if(::recv(s, szText, 1024, 0) == -1)
						::closesocket(s);
					else
						printf("�������ݣ�%s", szText);
				}
				break;
			case FD_CLOSE:
				{ 
					::closesocket(s);
				}
				break;
			}
		}
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0) ;
		return 0 ;
	}

	// �����ǲ��������Ϣ����ϵͳ��Ĭ�ϴ���
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}