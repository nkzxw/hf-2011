#include "stdafx.h"

LRESULT CALLBACK	PasswordWindow(HWND, UINT, WPARAM, LPARAM);

#define WM_BUTTON_OK   10123
#define WM_BUTTON_CANCEL   10456
#define WM_EDIT_PASSWORD   10789

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
   MSG msg;
   WNDCLASSEX wcex;
   HWND hWnd = NULL;
   HWND hEdit = NULL;

   (void) memset( &wcex, 0x00, sizeof(WNDCLASSEX) );

   wcex.cbSize        = sizeof(WNDCLASSEX);
   wcex.style			 = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc	 = PasswordWindow;
   wcex.hCursor		 = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH)COLOR_WINDOW;
   wcex.lpszClassName = "@PWDWIN@";

	RegisterClassEx(&wcex);

   hWnd = CreateWindow( "@PWDWIN@", 
                        " Type the password ...", 
                        WS_OVERLAPPED, 
                        GetSystemMetrics(SM_CXSCREEN)/2-100, 
                        GetSystemMetrics(SM_CYSCREEN)/2-75, 
                        400, 120, 
                        NULL, NULL, NULL, NULL);
   if (!hWnd)
      return 0;

   CreateWindow("BUTTON", "OK", BS_FLAT | BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP |  WS_CHILD, 140, 60, 70, 30, hWnd, (HMENU)WM_BUTTON_OK, NULL, NULL); 
   CreateWindow("BUTTON", "Cancel", BS_FLAT | BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP |  WS_CHILD, 240, 60, 70, 30, hWnd, (HMENU)WM_BUTTON_CANCEL, NULL, NULL); 
   hEdit = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL, 200, 15, 180, 25, hWnd, (HMENU)WM_EDIT_PASSWORD, NULL, NULL);
   CreateWindow ("Static","Please enter the password:",SS_RIGHT | SS_CENTERIMAGE | WS_VISIBLE | WS_CHILD, 2,15,180,25,hWnd, 0, NULL, NULL);

   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);
   
   SetFocus(hEdit);

   while ( GetMessage(&msg, NULL, 0, 0) )
   {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
   }

	if ( (int)msg.wParam == 0 )
      exit(0);

   DestroyWindow(hWnd);

   return 0;
}


LRESULT CALLBACK PasswordWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId = -1;
	char pwd[32];

	switch (message)
	{
		case WM_COMMAND:
		{   
			wmId = LOWORD(wParam);
			switch (wmId)
			{
				case WM_BUTTON_OK:
				{
				   (void) memset( pwd, 0x00, sizeof(pwd) );
				   GetWindowText( GetDlgItem(hWnd, WM_EDIT_PASSWORD), pwd, 32 );
				   if (strcmp( pwd, "PASSWORD" ) )
				   {
					  MessageBox( hWnd, "Sorry! Wrong password.", "Password", MB_ICONERROR );
					  PostQuitMessage(0);
				   }
				   else
					  PostQuitMessage(1);
				}                 
				break;

				case WM_BUTTON_CANCEL:
				   PostQuitMessage(0);
				   break;

				default:
				   break;
			}
		}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

