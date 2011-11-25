////////////////////////////////////
// ServerShutdown.cpp�ļ�


#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{	
	HANDLE hEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, "ShutdownEvent");
	if(hEvent != NULL)
	{
		::SetEvent(hEvent);
		::CloseHandle(hEvent);
		::MessageBox(NULL, " �������رճɹ���\n", "ServerShutdown", 0);
	}
	else
	{
		::MessageBox(NULL, " ��������û��������\n", "ServerShutdown", 0);
	}
	return 0;
}