#include "stdafx.h"
#include "Encrypt.h"
#include <windows.h>

#define hWndAttachExStyle  0
#define hWndAttachStyle (WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_OVERLAPPED) //WS_SiZEBOX
#define dwWndAttachWidth  320
#define dwWndAttachHeight 120
#define IDC_EDIT_PASSWORD 100
#define IDC_BUTTON_OK  101
#define IDC_BUTTON_CANCEL 102
#define IDM_ATTACH_MENU_ABOUT  103
#define MAX_PASSWORD_LENGTH 16


#pragma region REQUIRED_IMPORTS
typedef
HMODULE
(WINAPI *TGetModuleHandle)(
    __in_opt LPCSTR lpModuleName
    );

typedef 
BOOL
(WINAPI *TGetMessage)(
    __out LPMSG lpMsg,
    __in_opt HWND hWnd,
    __in UINT wMsgFilterMin,
    __in UINT wMsgFilterMax);

typedef 
BOOL
(WINAPI *TTranslateMessage)(
    __in CONST MSG *lpMsg);

typedef 
LRESULT
(WINAPI *TDispatchMessage)(
    __in CONST MSG *lpMsg);

typedef 
int
(WINAPI *TGetSystemMetrics)(
    __in int nIndex);

typedef 
BOOL
(WINAPI *TPostMessage)(
    __in_opt HWND hWnd,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam);

typedef 
LRESULT
(WINAPI *TSendMessage)(
    __in HWND hWnd,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam);

typedef
BOOL
(WINAPI *TShowWindow)(
    __in HWND hWnd,
    __in int nCmdShow);

typedef 
BOOL
(WINAPI *TUpdateWindow)(
    __in HWND hWnd);

typedef 
HCURSOR
(WINAPI *TLoadCursor)(
    __in_opt HINSTANCE hInstance,
    __in LPCSTR lpCursorName);

typedef
VOID
(WINAPI *TExitProcess)(
    __in UINT uExitCode
    );

typedef
BOOL
(WINAPI *TFreeLibrary) (
    __in HMODULE hLibModule
    );

typedef
DWORD
(WINAPI * TGetProcAddress) (
    __in HMODULE hModule,
    __in LPCSTR lpProcName
    );

typedef
HMODULE
(WINAPI *TLoadLibrary)(
    __in LPCSTR lpLibFileName
    );

typedef
HICON
(WINAPI * TLoadicon)(
    __in_opt HINSTANCE hinstance,
    __in LPCSTR lpiconName);
typedef
VOID
(WINAPI * TPostQuitMessage)(
    __in int nExitCode);

typedef
int
(WINAPI * TMessageBox)(
    __in_opt HWND hWnd,
    __in_opt LPCSTR lpText,
    __in_opt LPCSTR lpCaption,
    __in UINT uType);

typedef 
ATOM
(WINAPI *TRegisterClassEx)(
    __in CONST WNDCLASSEXA *);

typedef
HWND
(WINAPI *TCreateWindowEx)(
    __in DWORD dwExStyle,
    __in_opt LPCSTR lpClassName,
    __in_opt LPCSTR lpWindowName,
    __in DWORD dwStyle,
    __in int X,
    __in int Y,
    __in int nWidth,
    __in int nHeight,
    __in_opt HWND hWndParent,
    __in_opt HMENU hMenu,
    __in_opt HINSTANCE hinstance,
    __in_opt LPVOID lpParam);

typedef
LRESULT 
(CALLBACK *TDefWindowProc)(
    __in HWND hWnd,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam);

typedef
HWND
(WINAPI *TSetFocus)(
    __in_opt HWND hWnd);

typedef
LONG
(WINAPI *TGetWindowLong)(
    __in HWND hWnd,
    __in int nindex);
 
typedef 
LONG
(WINAPI *TSetWindowLong)(
    __in HWND hWnd,
    __in int nindex,
    __in LONG dwNewLong);

typedef
UINT
(WINAPI *TGetDlgitemText)(
    __in HWND hDlg,
    __in int niDDlgitem,
    __out_ecount(cchMax) LPSTR lpString,
    __in int cchMax);

typedef
HMENU
(WINAPI *TGetSystemMenu)(
    __in HWND hWnd,
    __in BOOL bRevert);

typedef
BOOL
(WINAPI *TAppendMenu)(
    __in HMENU hMenu,
    __in UINT uFlags,
    __in UINT_PTR uiDNewitem,
    __in_opt LPCSTR lpNewitem);

typedef
HFONT   
(WINAPI *TCreateFontindirect)(
	__in CONST LOGFONTA *lplf);

typedef
BOOL 
(WINAPI *TDeleteObject)( 
	__in HGDIOBJ ho);

typedef
BOOL
(WINAPI *TisDialogMessage)(
    __in HWND hDlg,
    __in LPMSG lpMsg);

typedef
HWND
(WINAPI *TGetDlgitem)(
    __in_opt HWND hDlg,
    __in int niDDlgitem);

typedef
int
(WINAPI *Twsprintf)(
    __out LPSTR,
    __in __format_string LPCSTR,
    ...);

typedef
BOOL
(WINAPI *TSetWindowText)(
    __in HWND hWnd,
    __in_opt LPCSTR lpString);

typedef
int
(WINAPI *Tlstrlen)(
    __in LPCSTR lpString
    );
#pragma endregion 

typedef struct _TAttachData
{ 
	DWORD _EntryPoint;
	DWORD hKernel32;

    HMODULE hLibUser32;
    HMODULE hLibGDi32;
    TGetProcAddress _GetProcAddress;
    TLoadLibrary _LoadLibrary;
    TFreeLibrary _FreeLibrary;
    TExitProcess _ExitProcess;
    TGetModuleHandle _GetModuleHandle;
    Tlstrlen _lstrlen;
    TGetMessage _GetMessage;
    TTranslateMessage _TranslateMessage;
    TDispatchMessage _DispatchMessage;
    TGetSystemMetrics _GetSystemMetrics;
    TPostMessage _PostMessage;
    TSendMessage _SendMessage;
    TShowWindow _ShowWindow;
    TUpdateWindow _UpdateWindow;
    TLoadCursor _LoadCursor;
    TLoadicon _Loadicon;
    TPostQuitMessage _PostQuitMessage;
    TMessageBox _MessageBox;
    TRegisterClassEx _RegisterClassEx;
    TCreateWindowEx _CreateWindowEx;
    TDefWindowProc _DefWindowProc;
    TSetFocus _SetFocus;
    TGetWindowLong _GetWindowLong;
    TSetWindowLong _SetWindowLong;
    TGetDlgitemText _GetDlgitemText;
    TGetSystemMenu _GetSystemMenu;
    TAppendMenu _AppendMenu;
    TCreateFontindirect _CreateFontindirect;
    TDeleteObject _DeleteObject;
    TisDialogMessage _isDialogMessage;
    TGetDlgitem _GetDlgitem;
    Twsprintf _wsprintf;
    TSetWindowText _SetWindowText;

	char szLibUser32[7]; 
	char szLibGDi32[6];
	char szLoadLibrary[13];
    char szFreeLibrary[12];
    char szExitProcess[12];
    char szGetModuleHandle[17];
    char szlstrlen[9];
    char szGetMessage[12];
    char szTranslateMessage[17];
    char szDispatchMessage[17];
    char szGetSystemMetrics[17];
    char szPostMessage[13];
    char szSendMessage[13];
    char szShowWindow[11];
    char szUpdateWindow[13];
    char szLoadCursor[12];
    char szLoadicon[10];
    char szPostQuitMessage[16];
    char szMessageBox[12];
    char szRegisterClassEx[17];
    char szCreateWindowEx[16];
    char szDefWindowProc[15];
    char szSetFocus[9];
    char szGetWindowLong[15];
    char szSetWindowLong[15];
    char szGetDlgitemText[16];
    char szGetSystemMenu[14];
    char szAppendMenu[12];
    char szisDialogMessage[16];
    char szGetDlgitem[11];
    char szwsprintf[11];
    char szSetWindowText[15];

    char szCreateFontindirect[20];
    char szDeleteObject[13];

    char _szAppClass[11];
    char _szAppTitle[21];
    char _szMenuAbout[21];
    char _szMsgAbout[151];

    char _szClassEdit[5];
    char _szClassStatic[7];
    char _szClassButton[7];
    char _szTitlePassword[12];
    char _szOK[9];
    char _szCancel[9];
    char _szWrongPassword[25];
    char _szTemplate[30];
    DWORD _dwPasswordCrc32;                 
    char _szChanceCount[256];
    
    HWND _hWndAttach;
    LOGFONT _fnt;
    HANDLE _hFont;
    BYTE _bCorrect;
    HWND _hWndChanceCount;
    WNDCLASSEX _wc;
    DWORD _nCount;

    DWORD _imageBase;
  
	_TAttachData ()
	{
		memset(szLibUser32, 0, 7); 
		memset(szLibGDi32, 0, 6);
		memset(szLoadLibrary,0,13);
		memset(szFreeLibrary, 0, 13);
		memset(szExitProcess, 0, 13);
		memset(szGetModuleHandle, 0, 18);
		memset(szlstrlen, 0, 10);
		memset(szGetMessage, 0, 13);
		memset(szTranslateMessage, 0, 18);
		memset(szDispatchMessage, 0, 18);
		memset(szGetSystemMetrics, 0, 18);
		memset(szPostMessage, 0, 14);
		memset(szSendMessage, 0, 14);
		memset(szShowWindow, 0, 12);
		memset(szUpdateWindow, 0, 14);
		memset(szLoadCursor, 0, 13);
		memset(szLoadicon, 0, 11);

		memset(szPostQuitMessage, 0, 17);
		memset(szMessageBox, 0, 13);
		memset(szRegisterClassEx, 0, 18);
		memset(szCreateWindowEx, 0, 17);
		memset(szDefWindowProc, 0, 16);
		memset(szSetFocus, 0, 10);
		memset(szGetWindowLong, 0, 16);
		memset(szSetWindowLong, 0, 16);
		memset(szGetDlgitemText, 0, 17);
		memset(szGetSystemMenu, 0, 15);
		memset(szAppendMenu, 0, 13);
		memset(szisDialogMessage, 0, 17);
		memset(szGetDlgitem, 0, 12);
		memset(szwsprintf, 0, 12);
		memset(szSetWindowText, 0, 16);

		memset(szCreateFontindirect, 0, 21);
		memset(szDeleteObject, 0, 14);

		memset(_szAppClass, 0, 12);
		memset(_szAppTitle, 0, 22);
		memset(_szMenuAbout, 0, 22);
		memset(_szMsgAbout, 0, 152);

		memset(_szClassEdit, 0, 6);
		memset(_szClassStatic, 0, 8);
		memset(_szClassButton, 0, 8);
		memset(_szTitlePassword, 0, 13);
		memset(_szOK, 0, 10);
		memset(_szCancel, 0, 10);
		memset(_szWrongPassword, 0, 26);
		memset(_szTemplate, 0, 31);
		memset(_szChanceCount, 0, 256);

	    _dwPasswordCrc32 = 0;    

		memset (this, 0, sizeof (_TAttachData));
	}
}TAttachData, *PAttachData;
 
TAttachData AttachData; // = *(PAttachData)((int)AttachStart - sizeof(AttachData));

void __stdcall InitAttachData ()
{
	memset ((PCHAR)&AttachData, 0, sizeof (TAttachData));
	memcpy (AttachData.szLibUser32, "user32", strlen ("user32"));
	memcpy (AttachData.szLibGDi32, "gdi32", strlen ("gdi32"));
    memcpy (AttachData.szLoadLibrary, "LoadLibraryA", strlen ("LoadLibraryA"));
    memcpy (AttachData.szFreeLibrary, "FreeLibrary", strlen("FreeLibrary"));
    memcpy (AttachData.szExitProcess, "ExitProcess", strlen ("ExitProcess"));
    memcpy (AttachData.szGetModuleHandle, "GetModuleHandleA", strlen("GetModuleHandleA"));
    memcpy (AttachData.szlstrlen, "lstrlenA", strlen ("lstrlenA"));
    memcpy (AttachData.szGetMessage, "GetMessageA", strlen ("GetMessageA"));
    memcpy (AttachData.szTranslateMessage, "TranslateMessage", strlen ("TranslateMessage"));
    memcpy (AttachData.szDispatchMessage, "DispatchMessageA", strlen ("DispatchMessageA"));
    memcpy (AttachData.szGetSystemMetrics, "GetSystemMetrics", strlen ("GetSystemMetrics"));
    memcpy (AttachData.szPostMessage, "PostMessageA", strlen ("PostMessageA"));
    memcpy (AttachData.szSendMessage, "SendMessageA", strlen ("SendMessageA"));
    memcpy (AttachData.szShowWindow, "ShowWindow", strlen ("ShowWindow"));
    memcpy (AttachData.szUpdateWindow, "UpdateWindow", strlen ("UpdateWindow"));
    memcpy (AttachData.szLoadCursor, "LoadCursorA", strlen ("LoadCursorA"));
    memcpy (AttachData.szLoadicon, "LoadIconA", strlen ("LoadIconA")); 
    memcpy (AttachData.szPostQuitMessage, "PostQuitMessage", strlen ("PostQuitMessage"));
    memcpy (AttachData.szMessageBox, "MessageBoxA", strlen ("MessageBoxA"));
    memcpy (AttachData.szRegisterClassEx, "RegisterClassExA", strlen ("RegisterClassExA"));
    memcpy (AttachData.szCreateWindowEx, "CreateWindowExA", strlen ("CreateWindowExA"));
    memcpy (AttachData.szDefWindowProc, "DefWindowProcA", strlen ("DefWindowProcA"));
    memcpy (AttachData.szSetFocus, "SetFocus", strlen ("SetFocus"));
    memcpy (AttachData.szGetWindowLong, "GetWindowLongA", strlen ("GetWindowLongA"));
    memcpy (AttachData.szSetWindowLong, "SetWindowLongA", strlen ("SetWindowLongA"));
    memcpy (AttachData.szGetDlgitemText, "GetDlgItemTextA", strlen ("GetDlgItemTextA"));
    memcpy (AttachData.szGetSystemMenu, "GetSystemMenu", strlen ("GetSystemMenu"));
    memcpy (AttachData.szAppendMenu, "AppendMenuA", strlen ("AppendMenuA"));
    memcpy (AttachData.szisDialogMessage, "IsDialogMessage", strlen ("IsDialogMessage"));
    memcpy (AttachData.szGetDlgitem, "GetDlgItem", strlen ("GetDlgItem"));
    memcpy (AttachData.szwsprintf, "wsprintfA", strlen ("wsprintfA"));         
    memcpy (AttachData.szSetWindowText, "SetWindowTextA", strlen ("SetWindowTextA"));
    memcpy (AttachData.szCreateFontindirect, "CreateFontIndirectA", strlen ("CreateFontIndirectA"));
    memcpy (AttachData.szDeleteObject, "DeleteObject", strlen ("DeleteObject"));

    AttachData._fnt.lfHeight = 12;
    AttachData._fnt.lfWidth = 0;
    AttachData._fnt.lfEscapement = 0;
    AttachData._fnt.lfOrientation = 0;
    AttachData._fnt.lfWeight = FW_NORMAL;
	AttachData._fnt.lfItalic = 0;
    AttachData._fnt.lfUnderline = 0;
    AttachData._fnt.lfStrikeOut = 0;
    AttachData._fnt.lfCharSet = DEFAULT_CHARSET;
    AttachData._fnt.lfOutPrecision = OUT_DEFAULT_PRECIS;
    AttachData._fnt.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    AttachData._fnt.lfQuality = PROOF_QUALITY;
    AttachData._fnt.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	memcpy (AttachData._fnt.lfFaceName, "宋体", strlen ("宋体"));

	memcpy(AttachData._szAppClass, "hf_pack", strlen("hf_pack"));
    memcpy(AttachData._szAppTitle, "hf_pack :: v1.0", strlen ("hf_pack :: v1.0"));
    memcpy(AttachData._szMenuAbout, "&About hf_pack...", strlen ("&About hf_pack..."));
    memcpy(AttachData._szMsgAbout, 
		   "[ PE Encrypt ]'#13#10 +'Version: 1.0'#13#10#13#10 +'作者: Liwuyue'#13#10 +'邮箱: smokingroom@sina.com'#13#10 +'主页: http://www.programmerlife.com",
		   strlen ("[ PE Encrypt ]'#13#10 +'Version: 1.0'#13#10#13#10 +'作者: Liwuyue'#13#10 +'邮箱: smokingroom@sina.com'#13#10 +'主页: http://www.programmerlife.com"));
    memcpy(AttachData._szClassEdit, "Edit", strlen ("Edit"));
    memcpy(AttachData._szClassStatic, "Static", strlen ("Static"));
    memcpy(AttachData._szClassButton, "Button", strlen ("Button"));
    memcpy(AttachData._szTitlePassword, "请输入密码:", strlen ("请输入密码"));
    memcpy(AttachData._szOK, "确定(&O)", strlen ("确定(&O)"));
    memcpy(AttachData._szCancel, "取消(&C)", strlen ("取消(&C)"));
    memcpy(AttachData._szWrongPassword, "密码不正确,请重新输入!", strlen ("密码不正确,请重新输入!"));
    memcpy(AttachData._szTemplate, "你还剩下 %d 次机会", strlen ("你还剩下 %d 次机会"));
    AttachData._nCount = 3;
}

int __stdcall AttachStart ()
{
	__asm{
	  call nextAddr
	nextAddr:
	  pop eax
	  sub eax, 8
	}
}

void __stdcall AttachProc()
{
	MSG	msg;
	int aLeft,aTop;
	DWORD	EntryPoint;
 
	DWORD dwStartAddr = AttachStart ();
	PAttachData pAttachData = (PAttachData)(dwStartAddr - sizeof(TAttachData));
	
	EntryPoint = (DWORD)pAttachData->_EntryPoint + pAttachData->_imageBase;

	DWORD dwKernel32 = pAttachData->hKernel32;
	DWORD dwGetProcAddress = (DWORD)pAttachData->_GetProcAddress;
	DWORD dwLoadlibrary = (DWORD)pAttachData->_LoadLibrary;

	TGetProcAddress pfn_GetProcAddress = (TGetProcAddress)dwGetProcAddress;
	TLoadLibrary pfn_LoadLibrary = (TLoadLibrary)dwLoadlibrary;
	HMODULE hKernel32 = pfn_LoadLibrary ("kernel32.dll");
	HMODULE hUser32 = pfn_LoadLibrary ("user32.dll");
	HMODULE hGdi32 = pfn_LoadLibrary ("gdi32.dll");

	TMessageBox pfn_MessageBox = (TMessageBox)pfn_GetProcAddress (hUser32, "MessageBoxA");
	//pfn_MessageBox (NULL, "111", "222", 0);
	__asm{
		mov eax, EntryPoint
			jmp eax
	}
	
	HMODULE hLibUser32 = (HMODULE)pfn_LoadLibrary("user32.dll");
	pAttachData->hLibUser32 = hLibUser32;

    pAttachData->_LoadLibrary  =  (TLoadLibrary)pAttachData->_GetProcAddress((HMODULE)dwKernel32, pAttachData->szLoadLibrary);

    //pAttachData->hLibUser32	= (HMODULE)pAttachData->_LoadLibrary(pAttachData->szLibUser32);   //更user32.dll
	pAttachData->_MessageBox = (TMessageBox)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szMessageBox);
	pAttachData->_MessageBox (NULL, "1111", "2222", 0);

    pAttachData->hLibGDi32 = (HMODULE)pAttachData->_LoadLibrary(pAttachData->szLibGDi32);    //更gdi32.dll
;
    pAttachData->_FreeLibrary  =  (TFreeLibrary)pAttachData->_GetProcAddress((HMODULE)dwKernel32, pAttachData->szFreeLibrary);
    pAttachData->_ExitProcess  =  (TExitProcess)pAttachData->_GetProcAddress((HMODULE)dwKernel32, pAttachData->szExitProcess);
    pAttachData->_GetModuleHandle  = (TGetModuleHandle)pAttachData->_GetProcAddress((HMODULE)dwKernel32,  pAttachData->szGetModuleHandle);
    pAttachData->_lstrlen  = (Tlstrlen)pAttachData->_GetProcAddress((HMODULE)dwKernel32,pAttachData->szlstrlen);

	
    pAttachData->_GetMessage = (TGetMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetMessage);
    pAttachData->_TranslateMessage = (TTranslateMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szTranslateMessage);
    pAttachData->_DispatchMessage	= (TDispatchMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szDispatchMessage);
    pAttachData->_GetSystemMetrics = (TGetSystemMetrics)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetSystemMetrics);
    pAttachData->_PostMessage	 = (TPostMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szPostMessage);
    pAttachData->_SendMessage	= (TSendMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szSendMessage);
    pAttachData->_ShowWindow = (TShowWindow)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szShowWindow);
    pAttachData->_UpdateWindow = (TUpdateWindow)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szUpdateWindow);
    pAttachData->_LoadCursor = (TLoadCursor)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szLoadCursor);
    pAttachData->_Loadicon = (TLoadicon)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szLoadicon);
    pAttachData->_PostQuitMessage = (TPostQuitMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szPostQuitMessage);
    pAttachData->_RegisterClassEx = (TRegisterClassEx)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szRegisterClassEx);
    pAttachData->_CreateWindowEx	= (TCreateWindowEx)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szCreateWindowEx);
    pAttachData->_DefWindowProc = (TDefWindowProc)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szDefWindowProc);
    pAttachData->_SetFocus = (TSetFocus)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szSetFocus);
    pAttachData->_GetWindowLong = (TGetWindowLong)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetWindowLong);
    pAttachData->_SetWindowLong = (TSetWindowLong)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szSetWindowLong);
    pAttachData->_GetDlgitemText	= (TGetDlgitemText)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetDlgitemText);
    pAttachData->_GetSystemMenu	= (TGetSystemMenu)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetSystemMenu);
    pAttachData->_AppendMenu = (TAppendMenu)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szAppendMenu);
    pAttachData->_isDialogMessage = (TisDialogMessage)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szisDialogMessage);
    pAttachData->_GetDlgitem	= (TGetDlgitem)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szGetDlgitem);
    pAttachData->_wsprintf = (Twsprintf)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szwsprintf);
    pAttachData->_SetWindowText = (TSetWindowText)pAttachData->_GetProcAddress(pAttachData->hLibUser32,pAttachData->szSetWindowText);

    pAttachData->_CreateFontindirect = (TCreateFontindirect)pAttachData->_GetProcAddress(pAttachData->hLibGDi32, pAttachData->szCreateFontindirect);
    pAttachData->_DeleteObject = (TDeleteObject)pAttachData->_GetProcAddress(pAttachData->hLibGDi32, pAttachData->szDeleteObject);

	pAttachData->_wc.hInstance = pAttachData->_GetModuleHandle(NULL);
    pAttachData->_wc.cbSize = sizeof(WNDCLASSEX);
    pAttachData->_wc.style = CS_HREDRAW | CS_VREDRAW;

	//pAttachData->_wc.lpfnWndProc = (WNDPROC)(DWORD(AttachWindowProc) - DWORD(AttachStart) + (DWORD)AttachStart);  //???????
    pAttachData->_wc.lpfnWndProc = (WNDPROC)AttachWindowProc;
	pAttachData->_wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    pAttachData->_wc.lpszClassName = pAttachData->_szAppClass;
    pAttachData->_wc.hCursor = pAttachData->_LoadCursor(0,IDC_ARROW);
    pAttachData->_wc.hIcon = pAttachData->_Loadicon(0,IDI_WINLOGO);
    pAttachData->_wc.hIconSm = pAttachData->_wc.hIcon; 
    pAttachData->_RegisterClassEx(&pAttachData->_wc);
    aLeft = (pAttachData->_GetSystemMetrics(SM_CXSCREEN) - dwWndAttachWidth) / 2;
    aTop =  (pAttachData->_GetSystemMetrics(SM_CYSCREEN) - dwWndAttachHeight) / 2;
    pAttachData->_hWndAttach = pAttachData->_CreateWindowEx(hWndAttachExStyle, 
														pAttachData->_szAppClass, 
														pAttachData->_szAppTitle, 
														hWndAttachStyle,aLeft,aTop,dwWndAttachWidth,dwWndAttachHeight,0,0, 
														pAttachData->_wc.hInstance,
														NULL);

	pAttachData->_MessageBox (NULL, "1111", "111111", 0);

	pAttachData->_ShowWindow(pAttachData->_hWndAttach,SW_SHOW);
    pAttachData->_UpdateWindow(pAttachData->_hWndAttach);

	while(pAttachData->_GetMessage(&msg,0,0,0))
	{
      if(!pAttachData->_isDialogMessage(pAttachData->_hWndAttach, &msg))
	  {
        pAttachData->_TranslateMessage(&msg);
        pAttachData->_DispatchMessage(&msg);
	  }
	}
    pAttachData->_FreeLibrary(pAttachData->hLibGDi32);
    pAttachData->_FreeLibrary(pAttachData->hLibUser32);
    if(pAttachData->_bCorrect == 1)
	{
		//EntryPoint = (DWORD)pAttachData->_wc.hInstance +
		//			(DWORD)(pAttachData->_wc.hInstance - pAttachData->_imageBase) +
		//			(DWORD)pAttachData->_EntryPoint;
		EntryPoint = (DWORD)pAttachData->_EntryPoint + (DWORD)pAttachData->_imageBase;
		__asm{
			mov eax, EntryPoint
			jmp eax
		}
	}

    //pAttachData->_ExitProcess(0);
}

LRESULT __stdcall AttachWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU hSysMenu;
	HWND TmpHwnd ;
	char szPassword[MAX_PASSWORD_LENGTH];

	LRESULT Result = 0;

	DWORD dwStartAddr = AttachStart ();
	PAttachData pAttachData = (PAttachData)(dwStartAddr - sizeof(TAttachData));

	switch (uMsg)
	{

	case  WM_CREATE:
	{
		  hSysMenu = pAttachData->_GetSystemMenu(hWnd,false);
		  pAttachData->_AppendMenu(hSysMenu,MF_SEPARATOR,0,NULL);
		  pAttachData->_AppendMenu(hSysMenu,MF_STRING,IDM_ATTACH_MENU_ABOUT,pAttachData->_szMenuAbout);
		  pAttachData->_hFont = pAttachData->_CreateFontindirect(&(pAttachData->_fnt));
		  pAttachData->_hWndChanceCount = pAttachData->_CreateWindowEx(0, pAttachData->_szClassStatic, 
														pAttachData->_szChanceCount,
														SS_CENTER | SS_CENTERIMAGE | WS_VISIBLE | WS_CHILD,
														10,32,300,22,
														hWnd,
														0,
														pAttachData->_wc.hInstance,
														NULL);
		  pAttachData->_SendMessage(pAttachData->_hWndChanceCount, WM_SETFONT, (WPARAM)pAttachData->_hFont,0);
		  pAttachData->_wsprintf(pAttachData->_szChanceCount, pAttachData->_szTemplate, pAttachData->_nCount);
		  pAttachData->_SetWindowText(pAttachData->_hWndChanceCount,pAttachData->_szChanceCount);

		  TmpHwnd = pAttachData->_CreateWindowEx(0,pAttachData->_szClassStatic,
												pAttachData->_szTitlePassword,
												SS_RIGHT | SS_CENTERIMAGE | WS_VISIBLE | WS_CHILD,
												5,10,80,22,
												hWnd,
												0,
												pAttachData->_wc.hInstance,
												NULL);

		  pAttachData->_SendMessage(TmpHwnd, WM_SETFONT, (WPARAM)pAttachData->_hFont,0);

		  TmpHwnd = pAttachData->_CreateWindowEx(WS_EX_STATICEDGE, 
									pAttachData->_szClassEdit,
									NULL,
									ES_AUTOHSCROLL | ES_PASSWORD | WS_VISIBLE | WS_TABSTOP | WS_CHILD,
									90,12,205,18,
									hWnd,
									0,
									pAttachData->_wc.hInstance,
									NULL);

		  pAttachData->_SendMessage(TmpHwnd, EM_SETLIMITTEXT, MAX_PASSWORD_LENGTH, 0);
		  pAttachData->_SetWindowLong(TmpHwnd, GWL_ID, IDC_EDIT_PASSWORD);
		  pAttachData->_SetFocus(TmpHwnd);
		  
		  TmpHwnd = pAttachData->_CreateWindowEx(0,pAttachData->_szClassButton,
												pAttachData->_szOK,
												BS_FLAT | BS_DEFPUSHBUTTON | WS_VISIBLE | WS_TABSTOP | WS_CHILD,
												80,60,80,20,hWnd,0,
												pAttachData->_wc.hInstance,NULL);
		  
		  pAttachData->_SendMessage(TmpHwnd, WM_SETFONT, (WPARAM)pAttachData->_hFont, 0);
		  pAttachData->_SetWindowLong(TmpHwnd, GWL_ID, IDC_BUTTON_OK);

		  TmpHwnd = pAttachData->_CreateWindowEx(0,pAttachData->_szClassButton,
												pAttachData->_szCancel,
												BS_FLAT | WS_VISIBLE | WS_TABSTOP | WS_CHILD,
												180,60,80,20,hWnd,0,
												pAttachData->_wc.hInstance,NULL);

		  pAttachData->_SendMessage(TmpHwnd, WM_SETFONT, (WPARAM)pAttachData->_hFont,0);
		  pAttachData->_SetWindowLong(TmpHwnd, GWL_ID, IDC_BUTTON_CANCEL);
	}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_BUTTON_OK)
			{
				pAttachData->_GetDlgitemText(hWnd, IDC_EDIT_PASSWORD, szPassword, MAX_PASSWORD_LENGTH);
				if (CalcCrc32(szPassword, pAttachData->_lstrlen(szPassword)) == pAttachData->_dwPasswordCrc32){
					pAttachData->_bCorrect = 1;
				}
				if ((pAttachData->_bCorrect == 1) || (pAttachData->_nCount == 1)){
					pAttachData->_PostMessage(hWnd,WM_CLOSE,0,0);
				}
				else {
				  pAttachData->_MessageBox(hWnd, pAttachData->_szWrongPassword, pAttachData->_szAppTitle,MB_OK + MB_ICONERROR);
				  TmpHwnd = pAttachData->_GetDlgitem(hWnd,IDC_EDIT_PASSWORD);
				  pAttachData->_SetFocus(TmpHwnd);
				  pAttachData->_SendMessage(hWnd, EM_SETSEL, 0,-1);
				  pAttachData->_nCount= pAttachData->_nCount - 1;
				  pAttachData->_wsprintf(pAttachData->_szChanceCount, pAttachData->_szTemplate, pAttachData->_nCount);
				  pAttachData->_SetWindowText(pAttachData->_hWndChanceCount, pAttachData->_szChanceCount);
				}
			}
			else if (LOWORD(wParam) == IDC_BUTTON_CANCEL){
				pAttachData->_PostMessage(hWnd, WM_CLOSE, 0, 0);
			}
			else{
				Result = pAttachData->_DefWindowProc(hWnd,uMsg,wParam,lParam);
			}
		}
	  case WM_SYSCOMMAND:
		  {
			  if (wParam == IDM_ATTACH_MENU_ABOUT){
				pAttachData->_MessageBox(hWnd,pAttachData->_szMsgAbout, pAttachData->_szAppTitle, MB_OK + MB_ICONINFORMATION);
			  }
			  else{
				Result = pAttachData->_DefWindowProc(hWnd,uMsg,wParam,lParam);
			  }
		  }
	  case WM_CLOSE:
		  {
			  pAttachData->_DeleteObject(pAttachData->_hFont);
			  Result = pAttachData->_DefWindowProc(hWnd,uMsg,wParam,lParam);
		  }
	  case WM_DESTROY:
		  {
			pAttachData->_PostQuitMessage(0);
		  }
	  default:
		Result = pAttachData->_DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	return Result;
}

DWORD __stdcall CalcCrc32(char *lpSource, int nLength)
{
	DWORD Crc32Table[256];
	int  i, j;
	DWORD crc;
	DWORD Result;

	for (i = 0; i < 256; i++) 
	{
		crc = i;
		for (j = 0; j < 7; j++)
		{
			if (crc & 1 > 0 ){
				crc = (crc << 1) ^ 0xEDB88320;
			}
			else{
				crc =crc << 1;
			}
		}
		Crc32Table[i] = crc;
	}

	Result = 8;
	for (i = 0; i < nLength - 1; i++)
	{
		Result = Crc32Table[(BYTE)(Result ^ DWORD(lpSource[i]))] ^ ((Result  << 8) & 0x00FFFFFF);
		Result = ~Result;
	}

	return Result;
}

void __stdcall AttachEnd ()
{
}

//#pragma code_seg()
//
//#pragma optimize( "", on )

void checkMessageBox ()
{
	InitAttachData ();
	AttachProc ();
}

void Encrypt(char *pchFileName, char *pchPassword, bool bBackup)
{
	HANDLE hFile;
	IMAGE_DOS_HEADER imgDosHeader;
	IMAGE_NT_HEADERS imgNtHeaders;
	IMAGE_SECTION_HEADER imgSectionHeader;

	DWORD dwPointerToRawData = 0;
	DWORD dwVirtualAddress = 0;

	int i = 0;
	DWORD dwBytesRead = 0;
	DWORD dwBytesWrite = 0;
	DWORD dwAttachSize = 0;

	char *DlgCaption = "hf_pack 1.0";

	hFile = CreateFile(pchFileName,
					GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL ,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
	   //TODO
	   return;
	}

	ReadFile(hFile, &imgDosHeader, sizeof(IMAGE_DOS_HEADER), &dwBytesRead, NULL);
	if (imgDosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
	   //TODO
	   return;
	}
  
	SetFilePointer(hFile, imgDosHeader.e_lfanew, NULL, FILE_BEGIN);
	ReadFile(hFile, &imgNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwBytesRead, NULL);
	if (imgNtHeaders.Signature != IMAGE_NT_SIGNATURE)
	{
		//TODO
		return;
	}

	for (i; i < imgNtHeaders.FileHeader.NumberOfSections; i++)
	{    
		// 
		// 检测文件是否已经加过了壳
		//
		ReadFile(hFile, &imgSectionHeader, sizeof(IMAGE_SECTION_HEADER), &dwBytesRead, NULL);
		if (strnicmp ((char *)imgSectionHeader.Name, ".YHF", strlen (".YHF")) == 0){
			//TODO
			return;
		} 

		if (dwPointerToRawData < imgSectionHeader.PointerToRawData + imgSectionHeader.SizeOfRawData){
			dwPointerToRawData = imgSectionHeader.PointerToRawData + imgSectionHeader.SizeOfRawData;
		}

		if (dwVirtualAddress < imgSectionHeader.VirtualAddress + imgSectionHeader.Misc.VirtualSize){
			dwVirtualAddress = imgSectionHeader.VirtualAddress+imgSectionHeader.Misc.VirtualSize;
		}
	}

	if(bBackup){
		char pchNewFileName[512];
		memset (pchNewFileName, 0, 512);
		wsprintf (pchNewFileName, "%s%s", pchFileName, ".bak");
		CopyFile(pchFileName,  pchNewFileName, false);
	}

    dwAttachSize = (int)AttachEnd - (int)AttachStart + sizeof(TAttachData);

	memcpy ((char *)imgSectionHeader.Name, ".YHF", sizeof (".YHF"));

    imgSectionHeader.Misc.VirtualSize = dwAttachSize;
    imgSectionHeader.VirtualAddress = dwVirtualAddress;
    imgSectionHeader.SizeOfRawData = dwAttachSize;
    imgSectionHeader.PointerToRawData = dwPointerToRawData;
    imgSectionHeader.PointerToRelocations = 0;
    imgSectionHeader.PointerToLinenumbers = 0;
    imgSectionHeader.NumberOfRelocations = 0;

    if(imgSectionHeader.VirtualAddress % 4096 > 0)
	{
		imgSectionHeader.VirtualAddress = (imgSectionHeader.VirtualAddress / 4096 + 1) * 4096;
	}

    if (imgSectionHeader.PointerToRawData % 512 > 0 )
	{
		imgSectionHeader.PointerToRawData = (imgSectionHeader.PointerToRawData / 512 + 1) * 512;
	}

    imgSectionHeader.Characteristics = 0xE0000040;
    WriteFile(hFile, &imgSectionHeader, sizeof(IMAGE_SECTION_HEADER), &dwBytesWrite, NULL);
    
	InitAttachData ();

	AttachData._EntryPoint = imgNtHeaders.OptionalHeader.AddressOfEntryPoint;
	AttachData._imageBase = imgNtHeaders.OptionalHeader.ImageBase;

	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeaders = NULL;
    PIMAGE_DATA_DIRECTORY pDirectory = NULL;
	PIMAGE_EXPORT_DIRECTORY pExports = NULL;
	ULONG nSize, Address;   
    PULONG pFunctions = NULL;   
    PSHORT pOrdinals = NULL;   
    PULONG pNames = NULL;   
    PVOID pFunction = NULL;   
    ULONG Ordinal = 0;   

	
	DWORD		dwNtHeaders;
	DWORD		dwExportEntry;
	DWORD		dwAddressOfNames;
	DWORD		dwAddressOfNameOrdinals;
	DWORD		dwAddressOfFunctions;
	DWORD		dwNumberOfNames;
	//__asm 
	//{
	//	//mov  eax, [fs:30h]
	//	mov eax, fs:[30h]
	//	mov  eax, [eax+0ch]
	//	mov  eax, [eax+1ch]
	//	mov  eax, [eax]
	//	mov  eax, [eax+8h]
	//	mov dwKernel32, eax
	//}
	  
	DWORD dwKernel32 = (DWORD)LoadLibrary ("kernel32.dll");
	AttachData.hKernel32 = dwKernel32;

    pDosHeader = (PIMAGE_DOS_HEADER)dwKernel32;   
    pNtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)dwKernel32 + pDosHeader->e_lfanew);//定位到PE文件头开始地址   
    pDirectory = pNtHeaders->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;// 指向IMAGE_DIRECTORY_ENTRY_EXPORT开始处   
   
    nSize = pDirectory->Size;   
    Address = pDirectory->VirtualAddress;   
   
    pExports = (PIMAGE_EXPORT_DIRECTORY)((PCHAR)dwKernel32 + Address);//定位到导出表开始处   
   
    pFunctions = (PULONG)((PCHAR)dwKernel32 + pExports->AddressOfFunctions);//得到导出函数地址   
    pOrdinals = (PSHORT)((PCHAR)dwKernel32 + pExports->AddressOfNameOrdinals);//得到AddressOfNameOrdinals（RVA）   
    pNames = (PULONG)((PCHAR)dwKernel32 + pExports->AddressOfNames);//得到函数名   

    for(i = 0; i < pExports->NumberOfNames; i++)//开始查找   
    {   
        Ordinal = pOrdinals[i];   
        if(pFunctions[Ordinal] < Address || pFunctions[Ordinal] >= Address + nSize)   
        {   
			char *pch = (char *)((PCHAR)dwKernel32 + pNames[i]);
            if(strcmp((PSTR)((PCHAR)dwKernel32 + pNames[i]), "GetProcAddress") == 0) {   
                pFunction = (PCHAR)dwKernel32 + pFunctions[Ordinal];   
				AttachData._GetProcAddress = (TGetProcAddress)(PULONG)pFunction;
            }   
			if (strcmp((PSTR)((PCHAR)dwKernel32 + pNames[i]), "LoadLibraryA") == 0){
				pFunction = (PCHAR)dwKernel32 + pFunctions[Ordinal];   
				AttachData._LoadLibrary = (TLoadLibrary)(PULONG)pFunction;
			}
        }   
    }   

	//
	//构建新的入口地址
	//
    imgNtHeaders.OptionalHeader.AddressOfEntryPoint = imgSectionHeader.VirtualAddress + sizeof(TAttachData) + (DWORD)AttachProc - (DWORD)AttachStart;
	imgNtHeaders.OptionalHeader.SizeOfImage = imgNtHeaders.OptionalHeader.SizeOfImage + dwAttachSize;
    AttachData._dwPasswordCrc32 = CalcCrc32(pchPassword, strlen(pchPassword));

	imgNtHeaders.FileHeader.NumberOfSections++;

	SetFilePointer(hFile, imgDosHeader.e_lfanew, NULL, FILE_BEGIN);
    WriteFile(hFile, &imgNtHeaders, sizeof(imgNtHeaders), &dwBytesWrite, NULL);
    SetFilePointer(hFile, imgSectionHeader.PointerToRawData, NULL, FILE_BEGIN);

    WriteFile(hFile, (LPVOID)&AttachData, sizeof(TAttachData), &dwBytesWrite, NULL);
    WriteFile(hFile, (LPVOID)AttachStart, (int)AttachEnd - (int)AttachStart, &dwBytesWrite, NULL);
}


LRESULT CALLBACK	PasswordWindow(HWND, UINT, WPARAM, LPARAM);

#define WM_BUTTON_OK   10123
#define WM_BUTTON_CANCEL   10456
#define WM_EDIT_PASSWORD   10789

int DlgEntry()
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