#include <windows.h>

HWND     g_hWnd;
HMODULE  hHookLib = NULL;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance,
    LPSTR     lpszCmdLine, 
	int       nCmdShow
	)
{
    HWND     hwnd;
    MSG      msg ;
    WNDCLASS wndclass ;

	if(!hPrevInstance) 
	{
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = WndProc ;
		wndclass.cbClsExtra    = 0 ;
		wndclass.cbWndExtra    = 0 ;
		wndclass.hInstance     = hInstance ;
		wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION) ;
		wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
		wndclass.hbrBackground =(HBRUSH) GetStockObject(WHITE_BRUSH) ;
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "DemoClass" ;
		RegisterClass(&wndclass) ;
	}

	hwnd = ::CreateWindow(
		"DemoClass",			// LPCTSTR lpClassName
		"Test Application",		// LPCTSTR lpWindowName
   		WS_OVERLAPPEDWINDOW,	// DWORD dwStyle
		CW_USEDEFAULT,			// int x
		0,						// int y 
		CW_USEDEFAULT,			// int nWidth
		0,						// int nHeight
		NULL,					// HWND hWndParent
		NULL,					// HMENU hMenu
		hInstance,				// HANDLE hInstance
		NULL                    // PVOID lpParam 
		);				
	g_hWnd = hwnd;
	
	::ShowWindow(hwnd, nCmdShow) ;
	::UpdateWindow(hwnd) ;

	while(::GetMessage(&msg, NULL, 0, 0))
	{
		::TranslateMessage(&msg) ;
		::DispatchMessage(&msg) ;
	}
	
	return msg.wParam ;
}

LRESULT CALLBACK WndProc(
	HWND   hwnd, 
	UINT   message, 
	WPARAM wParam, 
	LPARAM lParam
	)
{
	static char    pszLine0[80] = "Hello from TestApp!";
	static wchar_t pszLine1[80] = L"Hello from TestApp!";
    HDC hDC;                               
    PAINTSTRUCT ps;
	
	switch(message)
    {
    case WM_PAINT:
        hDC = ::BeginPaint(hwnd, &ps);
		::TextOutA(hDC, 0, 0, pszLine0, lstrlen(pszLine0));
		::TextOutW(hDC, 0, 20, pszLine1, wcslen(pszLine1));
        ::EndPaint(hwnd, &ps);
		return 0;
    case WM_DESTROY :
        ::PostQuitMessage(0) ;
		return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam) ;
}