#include "stdafx.h"
#include "extra_code.h"

//
//New Section
//
BYTE vg_new_section[_MAX_SECTION_SIZE];

//
//Memory used by NewEntryPoint
//
BYTE vg_data_ep_data[_MAX_SECTION_DATA_SIZE_];

//
//Used by PwdWindow to link to vg_data_ep_data
//
BYTE vg_data_pw_data[_MAX_SEC_SECTION_DATA_SIZE_];

#pragma optimize( "", off ) // Disable all optimizations - we need code "as is"!

#pragma code_seg(".extcd")  // Lets put all functions in a separated code segment

int __stdcall NewEntryPoint()
{
	// * ------------------------------------------------ *   
	//  STEP 01: Find Section Data Address
	// * ------------------------------------------------ *   
	DWORD dwCurrentAddr = 0;  
	DWORD dwMagic       = 0; 
	DWORD dwDataSection = 0;
	DWORD dwPwdWindowDS = 0;

	//
	//The following code gets EIP register value and stores into dwCurrentAddr
	//
	__asm{
		 call lbl_ref1
		 lbl_ref1:
		 pop dwCurrentAddr
	  }

	//
	//Find the ending of data section <E> 
	//
	while ( dwMagic != 0x3E453C00 )
		 dwMagic = (DWORD)(*(DWORD *)(--dwCurrentAddr));

	//
	//Here we got address of Data Section
	//
	dwDataSection = dwCurrentAddr - (_MAX_SECTION_DATA_SIZE_ - 4);

	// * ------------------------------------------------------------------ *   
	//  STEP 02: Let's save Data Section Address in PwdWindow data section
	// * ------------------------------------------------------------------ *   
	dwPwdWindowDS = *((DWORD *)(dwDataSection+12));
	dwPwdWindowDS += (dwDataSection + _MAX_SECTION_DATA_SIZE_);   
	_MEMCPY_( (void *)dwPwdWindowDS, &dwDataSection, sizeof(DWORD) );

	// * ------------------------------------------------------------------ *   
	//  STEP 03: Map Original Entry point, LoadLibrary and GetProcAddress
	// * ------------------------------------------------------------------ *   
	WINSTARTFUNC   pfn_OriginalEntryPoint = NULL;
	LOADLIBRARY    pfn_LoadLibrary	     = NULL;
	GETPROCADDRESS pfn_GetProcAddress     = NULL;
	DWORD          dwWrk                  = 0;

	//
	//Original EntryPoint
	//
	pfn_OriginalEntryPoint = (WINSTARTFUNC)(*((DWORD *)dwDataSection));

	//
	//LoadLibrary and GetProcAddress: Get real address in two steps
	//
	dwWrk = *((DWORD *)(dwDataSection+4));
	pfn_LoadLibrary        = (LOADLIBRARY)(*((DWORD *)(dwWrk)));

	dwWrk = *((DWORD *)(dwDataSection+8));
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

	//
	//Let's point to WindowProc function (PwdWindow)
	//
	dwWrk  = (dwDataSection + _MAX_SECTION_DATA_SIZE_);
	dwWrk += *((DWORD *)(dwDataSection+12));
	dwWrk += _MAX_SEC_SECTION_DATA_SIZE_;
	pfn_WindowProc = (WINDOWPROCEDURE)dwWrk;

	//
	//Let's point to program strings
	//
	dwAddrEPStrings = dwDataSection + _OFFSET_STRINGS;

	//
	//Now lets create the Dialog Window and show it
	//
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

int __stdcall NewEntryPoint_End()
{
   return 0;
}

LRESULT CALLBACK PwdWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// * ------------------------------------------------ *   
	//  STEP 01: Find Section Data Address
	// * ------------------------------------------------ *   

	DWORD dwCurrentAddr = 0;  
	DWORD dwMagic       = 0; 
	DWORD dwDataSection = 0;
	DWORD dwSecDataSection = 0;

	//
	//The following code gets EIP register value and stores into dwCurrentAddr
	//
	__asm{
		 call lbl_ref1
		 lbl_ref1:
		 pop dwCurrentAddr
	  }

	//
	//Find the ending of data section <E> 
	//
	while ( dwMagic != 0x3E453C00 )
		 dwMagic = (DWORD)(*(DWORD *)(--dwCurrentAddr));

	//
	//Here we got address of Secondary Data Section
	//
	dwSecDataSection = dwCurrentAddr - (_MAX_SEC_SECTION_DATA_SIZE_ - 4);

	//
	//Here we got address of Data Section
	//
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

LRESULT __stdcall PwdWindow_End(char *_not_used)
{
   return 0;
}

// *** ATTENTION: 
//  
//  NewEntryPoint_End and PwdWindow_End must not have the same prototype. 
//  For instance, in this case:
//
//  void _stdcall NewEntryPoint_End();
//  void _stdcall PwdWindow_End();
//
//  compiler will assume one of the functions only!
//
//  Dont ask me why.
//

#pragma code_seg()

#pragma optimize( "", on )

