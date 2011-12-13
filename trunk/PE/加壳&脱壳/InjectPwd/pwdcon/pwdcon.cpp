#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4748)
#pragma warning(disable:4996)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma region CUSTOM_INLINE_FUNCTIONS

// Important: Compiler must set /O2 (Maximize Speed) to ensure inline functions
// Although compiler provides #pragma intrinsic it is not 100% reliable
__forceinline void _MEMSET_( void *_dst, int _val, size_t _sz )
{
   while ( _sz ) ((BYTE *)_dst)[--_sz] = _val;
}

__forceinline void _MEMCPY_( void *_dst, void *_src, size_t _sz )
{
   while ( _sz-- ) ((BYTE *)_dst)[_sz] = ((BYTE *)_src)[_sz];
}

__forceinline BOOL _MEMCMP_( void *_src1, void *_src2, size_t _sz )
{
   while ( _sz-- )
   {
      if ( ((BYTE *)_src1)[_sz] != ((BYTE *)_src2)[_sz] )
         return FALSE;
   }

   return TRUE;
}

__forceinline size_t _STRLEN_(char *_src)
{
   size_t count = 0;
   while( _src && *_src++ ) count++;
   return count;
}

__forceinline int _STRCMP_(char *_src1, char *_src2)
{
   size_t sz = _STRLEN_(_src1);

   if ( _STRLEN_(_src1) != _STRLEN_(_src2) )
      return 1;

   return _MEMCMP_(_src1, _src2, sz ) ? 0 :  1;
}

#pragma endregion 

#pragma region REQUIRED_IMPORTS

typedef FARPROC (WINAPI *GETPROCADDRESS)(HMODULE, LPCSTR);
typedef HMODULE (WINAPI *LOADLIBRARY)(LPCSTR);
typedef LRESULT (CALLBACK *WINDOWPROCEDURE)(HWND, UINT, WPARAM, LPARAM);
typedef ATOM (WINAPI *REGISTERCLASSEX)(CONST WNDCLASSEXA *);
typedef HWND (WINAPI *CREATEWINDOWEX)(__in DWORD dwExStyle, __in_opt LPCSTR lpClassName, __in_opt LPCSTR lpWindowName, __in DWORD dwStyle, __in int X, __in int Y, __in int nWidth, __in int nHeight, __in_opt HWND hWndParent, __in_opt HMENU hMenu, __in_opt HINSTANCE hInstance, __in_opt LPVOID lpParam);
typedef BOOL (WINAPI *SETWINDOWTEXT)(__in HWND hWnd, __in_opt LPCSTR lpString);
typedef BOOL (WINAPI *SHOWWINDOW)(__in HWND hWnd, __in int nCmdShow);
typedef BOOL (WINAPI *UPDATEWINDOW)(__in HWND hWnd);
typedef HWND (WINAPI *SETFOCUS)(__in_opt HWND hWnd);
typedef BOOL (WINAPI *GETMESSAGE)( __out LPMSG lpMsg, __in_opt HWND hWnd, __in UINT wMsgFilterMin, __in UINT wMsgFilterMax);
typedef BOOL (WINAPI *TRANSLATEMESSAGE)(__in CONST MSG *lpMsg);
typedef LRESULT (WINAPI *DISPATCHMESSAGE)(__in CONST MSG *lpMsg);
typedef int (WINAPI *GETWINDOWTEXT)(__in HWND hWnd, __out_ecount(nMaxCount) LPSTR lpString, __in int nMaxCount);
typedef int (WINAPI *MESSAGEBOX)(__in_opt HWND hWnd, __in_opt LPCSTR lpText, __in_opt LPCSTR lpCaption, __in UINT uType);
typedef VOID (WINAPI *POSTQUITMESSAGE)(__in int nExitCode);
typedef LRESULT (WINAPI *DEFWINDOWPROC)(__in HWND hWnd, __in UINT Msg, __in WPARAM wParam, __in LPARAM lParam);
typedef int (WINAPI *GETSYSTEMMETRICS)(__in int nIndex);
typedef HWND (WINAPI *GETDLGITEM)(__in_opt HWND hDlg, __in int nIDDlgItem);
typedef VOID (WINAPI *EXITPROCESS)(__in UINT uExitCode);
typedef BOOL (WINAPI *DESTROYWINDOW)(__in HWND hWnd);

typedef void (_stdcall *WINSTARTFUNC)(void);
typedef LRESULT (CALLBACK *WINDOWPROCEDURE)(HWND, UINT, WPARAM, LPARAM);

#pragma endregion

#pragma region REQUIRED_STRINGS

typedef struct _XREF_NAMES_
{
   char *dll;
   char *calls[64];
} XREFNAMES;

XREFNAMES vg_imports[] = {
   { "USER32.DLL", 
      {
         "RegisterClassExA",  // 00  / ref offset = 0 
         "CreateWindowExA",   // 17  / ref offset = 4
         "SetWindowTextA",    // 33  / ref offset = 8
         "ShowWindow",        // 48  / ref offset = 12
         "UpdateWindow",      // 59  / ref offset = 16
         "SetFocus",          // 72  / ref offset = 20
         "GetMessageA",       // 81  / ref offset = 24
         "TranslateMessage",  // 93  / ref offset = 28
         "DispatchMessageA",  // 110 / ref offset = 32 
         "GetWindowTextA",    // 127 / ref offset = 36
         "MessageBoxA",       // 142 / ref offset = 40
         "PostQuitMessage",   // 154 / ref offset = 44
         "DefWindowProcA",    // 170 / ref offset = 48
         "GetSystemMetrics",  // 185 / ref offset = 52
         "GetDlgItem",        // 202 / ref offset = 56 
         "DestroyWindow",     // 212 / ref offset = 60
         NULL 
      }
   }, // USER32.DLL

   { "KERNEL32.DLL", 
      {
         "ExitProcess",      // 213  / ref offset = 60
         NULL 
      }
   }, // KERNEL32.DLL

   { NULL, { NULL } }
};

char *vg_string_list[] = {
	"@PWDWIN@",                // offset = 0 bytes 
	" Type the password ...",  // offset = 9 bytes 
	"BUTTON",                  // offset = 32 bytes 
	"EDIT",                    // offset = 39 bytes 
	"OK",                      // offset = 44 bytes 
	"Cancel",                  // offset = 47 bytes 
	"Sorry! Wrong password.",  // offset = 54 bytes 
	"Password",                // offset = 77 bytes (not used in codeinject.exe)
	NULL 
};

#pragma endregion 

#pragma region DATA_AREAS_DEFINITION


#define _OFFSET_STRINGS         32
#define _OFFSET_DLL_NAMES       200
#define _OFFSET_FUNCTION_NAMES  250
#define _OFFSET_FUNCTION_ADDR   600
#define _MAX_SECTION_DATA_SIZE_ 1024
#define _MAX_SECTION_SIZE       4096
#define _GAP_FUNCTION_NAMES     5
#define _MAX_SEC_SECTION_DATA_SIZE_ 16

BYTE vg_data_ep_data[_MAX_SECTION_DATA_SIZE_];
BYTE vg_data_pw_data[_MAX_SEC_SECTION_DATA_SIZE_];

#pragma endregion

#pragma region MISC_OPERATIONS

LRESULT CALLBACK PwdWindow(HWND, UINT, WPARAM, LPARAM);
void fill_data_areas( char *_pwd );
int __stdcall NewEntryPoint();
int OriginalEntryPoint(void);

int OriginalEntryPoint(void)
{
   ::MessageBox( NULL, "Original Entry Point!", "Info", MB_ICONINFORMATION );
   return 0;
}

int main(int argc, char* argv[]) 
{
   fill_data_areas("TEST01");

   NewEntryPoint(); 

   return 0;
}

void fill_data_areas( char *_pwd )
{
	DWORD dwEPSize = 0;
	DWORD iOffSet = 0;
	DWORD iOffSet2 = 0;
	DWORD dwOriginalEntryPoint = (DWORD)OriginalEntryPoint;
	DWORD dwLoadLibrary        = (DWORD)LoadLibrary;
	DWORD dwGetProcAddress	   = (DWORD)GetProcAddress;
	BYTE  pass[12];            // Password not more than 10 chars

	//Init data areas
	(void) memset( vg_data_ep_data, 0x00, sizeof(vg_data_ep_data) );
	(void) memset( vg_data_pw_data, 0x00, sizeof(vg_data_pw_data) );

	//Saves Origial Entry-Point 
	(void) memcpy( vg_data_ep_data, &dwOriginalEntryPoint, sizeof(DWORD) );  // offset = 0

	//Saves LoadLibrary and GetProcAddress
	(void) memcpy( &vg_data_ep_data[4], &dwLoadLibrary, sizeof(DWORD) );     // offset = 4
	(void) memcpy( &vg_data_ep_data[8], &dwGetProcAddress, sizeof(DWORD) );  // offset = 8

	//Let's save the size (in bytes) of NewEntryPoint
	dwEPSize = 0; 
	(void) memcpy( &vg_data_ep_data[12], &dwEPSize, sizeof(DWORD) );

	//Saves typed password
	(void) memset( pass, 0x00, sizeof(pass) );
	(void) strcpy( (char *)pass, _pwd );  
	(void) memcpy( &vg_data_ep_data[16], pass, 12 );  // offset = 16

	//Mark end of data area
	(void) memcpy( &vg_data_ep_data[_MAX_SECTION_DATA_SIZE_-3], "<E>", 3 );

	//Mark end of data area
	(void) memcpy( &vg_data_pw_data[_MAX_SEC_SECTION_DATA_SIZE_-3], "<E>", 3 );

	//Add strings used by injected code
	iOffSet = _OFFSET_STRINGS;
	for ( int iC = 0; vg_string_list[iC]; iC++ )
	{
	  (void) memcpy( &vg_data_ep_data[iOffSet], vg_string_list[iC], strlen(vg_string_list[iC]) );
	  iOffSet += ((UINT)strlen(vg_string_list[iC])+1);
	}

	//Add all DLLs and Functions to be imported
	iOffSet  = _OFFSET_DLL_NAMES;
	iOffSet2 = _OFFSET_FUNCTION_NAMES;
	for ( int iC = 0; vg_imports[iC].dll; iC++ )
	{
	  (void) memcpy( &vg_data_ep_data[iOffSet], vg_imports[iC].dll, strlen(vg_imports[iC].dll) );
	  iOffSet += ((UINT)strlen(vg_imports[iC].dll)+1);

	  for ( int iD = 0; vg_imports[iC].calls[iD]; iD++ )
	  {
		 (void) memcpy( &vg_data_ep_data[iOffSet2], vg_imports[iC].calls[iD], strlen(vg_imports[iC].calls[iD]) );
		 iOffSet2 += ((UINT)strlen(vg_imports[iC].calls[iD])+1);
	  }

	  //A small gap saparating functions groups
	  iOffSet2 += _GAP_FUNCTION_NAMES; 
	}
}

#pragma endregion

#pragma region CODE_THAT_WILL_BE_INJECTED

int __stdcall NewEntryPoint()
{
	// * ------------------------------------------------ *   
	//  STEP 01: Find NewEntryPoint Data Address
	// * ------------------------------------------------ *   

	DWORD dwCurrentAddr = 0;  
	DWORD dwDataSection = 0;
	DWORD dwPwdWindowDS = 0;

	dwCurrentAddr = (DWORD)&vg_data_ep_data[0]; 
	dwDataSection = dwCurrentAddr;

	// * ------------------------------------------------------------------ *   
	//  STEP 02: Let's save Data Section Address in PwdWindow data section
	// * ------------------------------------------------------------------ *   

	dwPwdWindowDS = (DWORD)&vg_data_pw_data[0];
	_MEMCPY_( (void *)dwPwdWindowDS, &dwDataSection, sizeof(DWORD) );

	// * ------------------------------------------------------------------ *   
	//  STEP 03: Map Original Entry point, LoadLibrary and GetProcAddress
	// * ------------------------------------------------------------------ *   

	WINSTARTFUNC   pfn_OriginalEntryPoint = NULL;
	LOADLIBRARY    pfn_LoadLibrary	     = NULL;
	GETPROCADDRESS pfn_GetProcAddress     = NULL;
	DWORD          dwWrk                  = 0;

	// *** Original EntryPoint
	pfn_OriginalEntryPoint = (WINSTARTFUNC)(*((DWORD *)dwDataSection));

	// *** LoadLibrary and GetProcAddress: Get real address in two steps
	dwWrk = (dwDataSection+4);
	pfn_LoadLibrary        = (LOADLIBRARY)(*((DWORD *)(dwWrk)));

	dwWrk = (dwDataSection+8);
	pfn_GetProcAddress     = (GETPROCADDRESS)(*((DWORD *)(dwWrk)));

	// * ------------------------------------------------------------------ *   
	//  STEP 04: Load DLLs and maps function addresses 
	// * ------------------------------------------------------------------ *   
	char *lpDllName = NULL;
	char *lpAPICall = NULL;
	DWORD dwOffSet0 = dwDataSection + _OFFSET_DLL_NAMES;
	DWORD dwOffSet1 = dwDataSection + _OFFSET_FUNCTION_NAMES;
	DWORD dwOffSet2 = dwDataSection + _OFFSET_FUNCTION_ADDR;
	DWORD dwAddr    = 0;
	HMODULE hMod = NULL;

	while ( *(lpDllName = ((char *)dwOffSet0)) )
	{
	  if ( (hMod = pfn_LoadLibrary(lpDllName)) == NULL )
		 goto OEP_CALL;
	  
	  while ( *(lpAPICall = ((char *)dwOffSet1)) )
	  {
		 dwAddr = (DWORD)pfn_GetProcAddress( hMod,  lpAPICall );
		 (void) _MEMCPY_( (void *)dwOffSet2, &dwAddr, sizeof(DWORD) ); 

		 dwOffSet2 += sizeof(DWORD);
		 dwOffSet1 += ((DWORD)_STRLEN_(lpAPICall)+1);
	  }

	  dwOffSet1 += _GAP_FUNCTION_NAMES;
	  dwOffSet0 += ((DWORD)_STRLEN_(lpDllName)+1);
	}

	// * ------------------------------------------------------------------ *   
	//  STEP 05: Map functions addresses
	// * ------------------------------------------------------------------ *   
	REGISTERCLASSEX   pfn_RegisterClassEx  = NULL;
	CREATEWINDOWEX    pfn_CreateWindowEx   = NULL;
	SETWINDOWTEXT     pfn_SetWindowText    = NULL;
	SHOWWINDOW        pfn_ShowWindow       = NULL;
	UPDATEWINDOW      pfn_UpdateWindow     = NULL;
	SETFOCUS          pfn_SetFocus         = NULL; 
	GETMESSAGE        pfn_GetMessage       = NULL; 
	TRANSLATEMESSAGE  pfn_TranslateMessage = NULL;
	DISPATCHMESSAGE   pfn_DispatchMessage  = NULL;
	GETSYSTEMMETRICS  pfn_GetSystemMetrics = NULL;
	EXITPROCESS       pfn_ExitProcess      = NULL;
	WINDOWPROCEDURE   pfn_WindowProc       = NULL;
	DESTROYWINDOW     pfn_DestroyWindow    = NULL;

	dwAddr = dwDataSection + _OFFSET_FUNCTION_ADDR;

	pfn_RegisterClassEx  = (REGISTERCLASSEX)(*((DWORD *)(dwAddr)));
	pfn_CreateWindowEx   = (CREATEWINDOWEX)(*((DWORD *)(dwAddr+4)));
	pfn_SetWindowText    = (SETWINDOWTEXT)(*((DWORD *)(dwAddr+8)));
	pfn_ShowWindow       = (SHOWWINDOW)(*((DWORD *)(dwAddr+12)));
	pfn_UpdateWindow     = (UPDATEWINDOW)(*((DWORD *)(dwAddr+16)));
	pfn_SetFocus         = (SETFOCUS)(*((DWORD *)(dwAddr+20)));
	pfn_GetMessage       = (GETMESSAGE)(*((DWORD *)(dwAddr+24)));
	pfn_TranslateMessage = (TRANSLATEMESSAGE)(*((DWORD *)(dwAddr+28)));
	pfn_DispatchMessage  = (DISPATCHMESSAGE)(*((DWORD *)(dwAddr+32)));
	pfn_GetSystemMetrics = (GETSYSTEMMETRICS)(*((DWORD *)(dwAddr+52)));
	pfn_DestroyWindow    = (DESTROYWINDOW)(*((DWORD *)(dwAddr+60)));
	pfn_ExitProcess      = (EXITPROCESS)(*((DWORD *)(dwAddr+64)));

	// * ------------------------------------------------------------------ *   
	//  STEP 06: Program Starts Here
	// * ------------------------------------------------------------------ *   
	WNDCLASSEX wcex;
	DWORD dwAddrEPStrings = 0;
	HWND  hWnd = NULL;
	HWND  hEdit = NULL;
	MSG   winMsg;

	//Let's point to WindowProc function (PwdWindow)
	pfn_WindowProc = (WINDOWPROCEDURE)PwdWindow;

	//Let's point to program strings
	dwAddrEPStrings = dwDataSection + _OFFSET_STRINGS;

	//Now lets create the Dialog Window and show it
	(void) _MEMSET_( &wcex, 0x00, sizeof(WNDCLASSEX) );
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.style		    = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	 = pfn_WindowProc;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+2);
	wcex.lpszClassName = (char *)(dwAddrEPStrings);
	wcex.cbWndExtra    = sizeof(DWORD);

	pfn_RegisterClassEx(&wcex);

	hWnd = pfn_CreateWindowEx( 0,
							 (char *)(dwAddrEPStrings), 
							 NULL, 
							 WS_OVERLAPPED, 
							 pfn_GetSystemMetrics(SM_CXSCREEN)/2-100, 
							 pfn_GetSystemMetrics(SM_CYSCREEN)/2-75, 
							 200, 150, 
							 NULL, NULL, NULL, NULL);

	if (!hWnd)
	  return 0;

	pfn_CreateWindowEx( 0,
					   (char *)(dwAddrEPStrings+32), 
					   (char *)(dwAddrEPStrings+44), 
					   WS_CHILD | WS_VISIBLE | BS_TEXT, 
					   10, 80, 70, 30, 
					   hWnd, 
					   (HMENU)10123, 
					   NULL, 
					   NULL); 

	pfn_CreateWindowEx( 0,
					  (char *)(dwAddrEPStrings+32), 
					  (char *)(dwAddrEPStrings+47), 
					  WS_CHILD | WS_VISIBLE | BS_TEXT, 
					  110, 80, 70, 30, 
					  hWnd, 
					  (HMENU)10456, 
					  NULL, 
					  NULL); 

	hEdit = pfn_CreateWindowEx( 0,
							   (char *)(dwAddrEPStrings+39), 
							   NULL, 
							   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL, 
							   10, 20, 170, 25, 
							   hWnd, 
							   (HMENU)10789, 
							   NULL, 
							   NULL);

	pfn_SetWindowText(hWnd, (char *)(dwAddrEPStrings+9) );
	pfn_ShowWindow(hWnd, SW_SHOW);
	pfn_UpdateWindow(hWnd);

	pfn_SetFocus(hEdit);

	while ( pfn_GetMessage(&winMsg, NULL, 0, 0) )
	{
		pfn_TranslateMessage(&winMsg);
		pfn_DispatchMessage(&winMsg);
	}

	if ( (int)winMsg.wParam == 0 )
	  pfn_ExitProcess(0); // *** If password is invalid or cancel was clicked

	pfn_DestroyWindow(hWnd);

OEP_CALL:

   pfn_OriginalEntryPoint();

   return 0;
}

LRESULT CALLBACK PwdWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// * ------------------------------------------------ *   
	//  STEP 01: Find NewEntryPoint Data Address
	// * ------------------------------------------------ *   

	DWORD dwCurrentAddr = 0;  
	DWORD dwDataSection = 0;
	DWORD dwSecDataSection = 0;

	dwCurrentAddr = (DWORD)&vg_data_pw_data[0]; 
	dwSecDataSection = dwCurrentAddr;

	//Here we got address of Data Section
	dwDataSection = (*((DWORD *)dwSecDataSection));

	// * ------------------------------------------------------------------ *   
	//  STEP 02: Map functions addresses
	// * ------------------------------------------------------------------ *   
	DWORD dwAddr  = dwDataSection + _OFFSET_FUNCTION_ADDR;

	DEFWINDOWPROC     pfn_DefWindowProc    = NULL;
	MESSAGEBOX        pfn_MessageBox       = NULL;
	POSTQUITMESSAGE   pfn_PostQuitMessage  = NULL;
	GETWINDOWTEXT     pfn_GetWindowText    = NULL;
	GETDLGITEM        pfn_GetDlgItem       = NULL;

	pfn_MessageBox       = (MESSAGEBOX)(*((DWORD *)(dwAddr+40)));
	pfn_PostQuitMessage  = (POSTQUITMESSAGE)(*((DWORD *)(dwAddr+44)));
	pfn_GetWindowText    = (GETWINDOWTEXT)(*((DWORD *)(dwAddr+36)));
	pfn_DefWindowProc    = (DEFWINDOWPROC)(*((DWORD *)(dwAddr+48)));
	pfn_GetDlgItem       = (GETDLGITEM)(*((DWORD *)(dwAddr+56)));

	// * ------------------------------------------------------------------ *   
	//  STEP 03: Program Starts Here
	// * ------------------------------------------------------------------ *   
	DWORD dwAddrEPStrings  = dwDataSection + _OFFSET_STRINGS; 
	WORD wmId = 0;
	char pwd[64];

	switch (message)
	{
	  case WM_COMMAND:
	  {   
		 wmId = LOWORD(wParam);

		 switch (wmId)
		 {
			case 10123:
			{
			   _MEMSET_(pwd, 0x00, sizeof(pwd));
			   pfn_GetWindowText( pfn_GetDlgItem(hWnd, 10789), pwd, 32 );
			   if ( _STRCMP_( pwd, (char *)(dwDataSection+16) ) )
			   {
				  pfn_MessageBox( hWnd, (char *)(dwAddrEPStrings+54), (char *)(dwAddrEPStrings+77), MB_ICONERROR );
				  pfn_PostQuitMessage(0);
			   }
			   else
				  pfn_PostQuitMessage(1);
			}                 
			  break;

			case 10456:
			   pfn_PostQuitMessage(0);
			   break;

			default:
			   break;
		 }
	  }
		  break;

		default:
			return pfn_DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

#pragma endregion


