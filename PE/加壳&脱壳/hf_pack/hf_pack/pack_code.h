#pragma once

#include <windows.h>

#define _OFFSET_STRINGS         32
#define _OFFSET_DLL_NAMES       200
#define _OFFSET_FUNCTION_NAMES  250
#define _OFFSET_FUNCTION_ADDR   600
#define _MAX_SECTION_DATA_SIZE_ 1024
#define _MAX_SECTION_SIZE       4096
#define _GAP_FUNCTION_NAMES     5
#define _MAX_SEC_SECTION_DATA_SIZE_ 16

extern BYTE vg_new_section[_MAX_SECTION_SIZE];

extern BYTE vg_data_ep_data[_MAX_SECTION_DATA_SIZE_];

extern BYTE vg_data_pw_data[_MAX_SEC_SECTION_DATA_SIZE_];

/**
*Entry Point 
*/
int __stdcall NewEntryPoint();
int __stdcall NewEntryPoint_End();

/**
*Window Function
*/
LRESULT CALLBACK  PwdWindow(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall PwdWindow_End(char *_not_used);

#pragma region CUSTOM_INLINE_FUNCTIONS


/**
*Important: Compiler must set /O2 (Maximize Speed) to ensure inline functions
*Although compiler provides #pragma intrinsic it is not 100% reliable
*/
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


