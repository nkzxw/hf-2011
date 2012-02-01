
#ifndef __MemServer
#   define __MemServer

#   include <windows.h>

#define WM_MS_NEWPROCESS   WM_USER + 0x1002

    VOID CreateMemServerDlg(HINSTANCE hInst,HANDLE hDlgOwner,DWORD PID);

	extern HWND            hMSWnd;

#   endif
