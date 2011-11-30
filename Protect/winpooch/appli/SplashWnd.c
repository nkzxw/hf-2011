/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "SplashWnd.h"

// libraries' headers
#define _WINDOWS_ // <-- workaround needed for freeimage 3.9.1
#include <FreeImage.h>

// project's headers
#include "Assert.h"
#include "Config.h"
#include "Resources.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// window class name
#define WC_SPLASHWND TEXT("SplashWnd")

#define MIN_SHOW_TIME 25

#define SHOW_SPEED	32
#define HIDE_SPEED	64


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _SplashWnd_LayeredWndProc (HWND, UINT, WPARAM, LPARAM) ;

LRESULT CALLBACK _SplashWnd_RegionnedWndProc (HWND, UINT, WPARAM, LPARAM) ;

BOOL	_SplashWnd_CanUsePerPixelAlpha () ;

HRGN _SplashWnd_CreateRgnFromImage (FIBITMAP * pfibImage, int nThreshold) ;


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _SplashWnd_CanUsePerPixelAlpha () 
{
  HDC	hdc ;
  int	nDevCaps ;

  hdc = GetDC (NULL);
  nDevCaps = GetDeviceCaps (hdc, BITSPIXEL) ;
  ReleaseDC (NULL, hdc) ;

  return nDevCaps>=32 ;
}


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL SplashWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;	    
  wndclass.lpfnWndProc   = _SplashWnd_CanUsePerPixelAlpha ()  ?
    _SplashWnd_LayeredWndProc : _SplashWnd_RegionnedWndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = NULL ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = NULL ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_SPLASHWND ;

  return 0!=RegisterClass (&wndclass) ;
}

/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND _SplashWnd_CreateWindow (HINSTANCE hInstance)
{  
  return  CreateWindowEx (WS_EX_NOACTIVATE/*|WS_EX_TOPMOST*/,
			  WC_SPLASHWND, NULL,
			  WS_POPUP|WS_VISIBLE,
			  CW_USEDEFAULT, CW_USEDEFAULT,
			  CW_USEDEFAULT, CW_USEDEFAULT, 
			  NULL/*hwndParent*/, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

FIBITMAP * _SplashWnd_LoadFromResource (HINSTANCE hInstance, LPTSTR lpName)
{
  HRSRC		hrRes ;
  HGLOBAL	hgRes ;
  VOID		*pResData ;
  UINT		nResSize ;
  FIMEMORY	*pfimRes ;
  FREE_IMAGE_FORMAT	fifImageType ;
  FIBITMAP	*pfibImage ;
  
  hrRes = FindResource (hInstance, lpName, RT_IMAGE) ;
  if( !hrRes  ) {
    TRACE_ERROR (TEXT("Failed to find resource\n")) ;
    return NULL ;
  }
  
  hgRes = LoadResource (hInstance, hrRes) ;
  if( !hgRes ) {
    TRACE_ERROR (TEXT("Failed to load resource\n")) ;
    return NULL ;
  }

  nResSize = SizeofResource(hInstance,hrRes) ;
  pResData = LockResource (hgRes) ;
  if( !pResData ) {
    TRACE_ERROR (TEXT("Failed to lock resource\n")) ;
    return NULL ;  
  }

  pfimRes = FreeImage_OpenMemory (pResData, nResSize) ;
  if( !pfimRes ) {
    TRACE_ERROR (TEXT("Failed to create memory descriptor\n")) ;
    return NULL ;
  }

  fifImageType = FreeImage_GetFileTypeFromMemory (pfimRes, nResSize) ;
  if( fifImageType==FIF_UNKNOWN ) {
    TRACE_ERROR (TEXT("Failed to get file type\n")) ;
    return NULL ;    
  }

  pfibImage = FreeImage_LoadFromMemory (fifImageType, pfimRes, 0) ; 
  if( ! pfibImage )
    TRACE_ERROR (TEXT("Failed to load image\n")) ;
  
  FreeImage_CloseMemory (pfimRes) ;
  
  return pfibImage ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

void _SplashWnd_MultiplyByAlpha (FIBITMAP * pfibImage)
{
  int	n ;
  BYTE	*p ;
  
  n = FreeImage_GetHeight(pfibImage) * FreeImage_GetWidth(pfibImage) ;
  p = FreeImage_GetBits (pfibImage) ;

  while( n-- )
    {
      p[0] = p[0]*p[3]/255;
      p[1] = p[1]*p[3]/255;
      p[2] = p[2]*p[3]/255;	  
      p += 4 ;    
    }
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _SplashWnd_SetImage (HINSTANCE hInstance, HWND hwnd, 
			  FIBITMAP *pfibImage, BYTE nOpacity)
{
  BOOL		bSuccess ;
  HDC		hdcImage ;
  HDC		hdcScreen ;
  HBITMAP	hbmImage ; 

  ASSERT (pfibImage!=NULL) ;
  
  TRACE_INFO (TEXT("res = %d x %d\n"), 
	      FreeImage_GetWidth(pfibImage),
	      FreeImage_GetHeight(pfibImage)) ;
  
  _SplashWnd_MultiplyByAlpha (pfibImage) ;
  
  hdcScreen	= GetDC(hwnd) ;
  ASSERT (hdcScreen!=NULL) ;
  
  hdcImage	= CreateCompatibleDC (hdcScreen) ;
  ASSERT (hdcImage!=NULL) ;			  
  
  hbmImage = CreateDIBitmap (hdcScreen, FreeImage_GetInfoHeader(pfibImage),
			     CBM_INIT, FreeImage_GetBits(pfibImage),
			     FreeImage_GetInfo(pfibImage), DIB_RGB_COLORS) ;
  
  if( hbmImage ) 
    {
      BLENDFUNCTION bf =  { AC_SRC_OVER, 0, nOpacity, AC_SRC_ALPHA };
      POINT		ptSrc, ptWin ;
      SIZE		szWin ;
	  
      ptSrc.x = 0 ;
      ptSrc.y = 0 ;
      
      szWin.cx = FreeImage_GetWidth(pfibImage) ;
      szWin.cy = FreeImage_GetHeight(pfibImage) ;
      
      ptWin.x = (GetSystemMetrics(SM_CXFULLSCREEN) - szWin.cx) / 2 ;
      ptWin.y = (GetSystemMetrics(SM_CYFULLSCREEN) - szWin.cy) / 2 ;
      
      SelectObject (hdcImage, hbmImage) ;
	  
      SetLastError (0) ;
      
      bSuccess = UpdateLayeredWindow (hwnd, hdcScreen, &ptWin, &szWin, 
				      hdcImage, &ptSrc, 0, &bf, ULW_ALPHA) ;
      
      if(  ! bSuccess ) 
	TRACE_ERROR (TEXT("UpdateLayeredWindow failed (error=%d)\n"),
		     GetLastError()) ;
    }
  else TRACE_ERROR (TEXT("Failed to create bitmap\n")) ;
  
  ReleaseDC (hwnd, hdcScreen) ;
  DeleteDC (hdcImage) ;
      
  return bSuccess ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

HRGN _SplashWnd_CreateRgnFromImage (FIBITMAP * pfibImage, int nThreshold)
{
  RGNDATA* pRgnData ;
  UINT	nBlockSize = 1024 ;
  UINT	w, h ;
  BYTE	*pPixels ;
  int	x, y ;

  if( ! pfibImage ) return NULL ;

  pPixels = FreeImage_GetBits (pfibImage) ;
  w = FreeImage_GetWidth(pfibImage) ;
  h = FreeImage_GetHeight(pfibImage) ;

  pRgnData = (RGNDATA*) malloc (sizeof(RGNDATAHEADER)+nBlockSize) ;
  if( ! pRgnData ) return NULL ;

  pRgnData->rdh.dwSize		= sizeof(RGNDATAHEADER) ;
  pRgnData->rdh.iType		= RDH_RECTANGLES;
  pRgnData->rdh.nCount		= 0 ;
  pRgnData->rdh.nRgnSize	= nBlockSize ;
  pRgnData->rdh.rcBound.left	= 0 ;
  pRgnData->rdh.rcBound.top	= 0 ;  
  pRgnData->rdh.rcBound.right	= w ;
  pRgnData->rdh.rcBound.bottom	= h ;

  for( y=0 ; y<h ; y++ )
    {
      int xFirst = -1 ;
      
      for( x=0 ; x<w ; x++ ) 
	{
	  BOOL	bOpaque = pPixels[(x+y*w)*4+3] > nThreshold ;

	  // is it the first opaque pixel ?
	  if( bOpaque && xFirst<0 ) 
	    {
	      xFirst = x ;
	    }
	  // is it the first transparent pixel ?
	  // or is it the end ? 
	  if( xFirst>=0 && ( !bOpaque || x==w-1 ) )
	    {
	      // get offset to RECT array if RGNDATA buffer
	      RECT * pRect = (RECT*)pRgnData->Buffer + pRgnData->rdh.nCount++ ;
	      
	      // save current RECT
	      pRect->left	= xFirst ;
	      pRect->top	= h - y - 1 ;
	      pRect->right	= x - bOpaque ; // +1 if end of line
	      pRect->bottom	= h - y ;
	      
	      // buffer full ?
	      if( pRgnData->rdh.nCount*sizeof(RECT) >= pRgnData->rdh.nRgnSize )
	      {
		// resize buffer
		pRgnData->rdh.nRgnSize += nBlockSize ;
		pRgnData = (RGNDATA*) realloc (pRgnData, 
					       sizeof(RGNDATAHEADER)+pRgnData->rdh.nRgnSize) ;
		if( ! pRgnData ) return NULL ;
	      }
	      
	      // reset first index
	      xFirst = -1 ;
	    } 
	}
    }
  
  HRGN hRgn = ExtCreateRegion (NULL, 
			       sizeof(RGNDATAHEADER)+pRgnData->rdh.nCount*sizeof(RECT), 
			       pRgnData) ;
  
  free (pRgnData) ;

  return hRgn;
}


VOID _SplashWnd_BltImage (HWND hwnd, HDC hdc, FIBITMAP * pfibImage)
{
  HDC		hdcImage ;
  UINT		w, h ;
  BOOL		bRes ;
  HBITMAP	hbmImage ;
  TCHAR		szBuffer[1024] ;

  if( FreeImage_GetColorType(pfibImage)!=FIC_RGBALPHA )
    MessageBox (hwnd, TEXT("No alpha channel"), NULL, 0) ;

  hdcImage = CreateCompatibleDC (hdc) ;

  hbmImage = CreateDIBitmap (hdc, FreeImage_GetInfoHeader(pfibImage),
			     CBM_INIT, FreeImage_GetBits(pfibImage),
			     FreeImage_GetInfo(pfibImage), DIB_RGB_COLORS) ;
  
  if( FreeImage_GetInfo(pfibImage)->bmiHeader.biBitCount!=32 ) 
    MessageBox (hwnd, TEXT("No alpha channel"), NULL, 0) ;

  if( ! hbmImage ) {
    MessageBox (hwnd, TEXT("Failed to create bitmap"), NULL, 0) ;
    return ;
  }
  
  w = FreeImage_GetWidth(pfibImage) ;
  h = FreeImage_GetHeight(pfibImage) ;

  SelectObject (hdcImage, hbmImage) ;

  SetLastError (0) ;     
  bRes = BitBlt (hdc, 0, 0, w, h, hdcImage, 0, 0, SRCCOPY) ;
  
  if(  ! bRes ) 
    {
      wsprintf (szBuffer, TEXT("BitBlt failed (error=%d)"), GetLastError()) ;
      MessageBox (hwnd, szBuffer, NULL, 0) ;
    }
}



LRESULT CALLBACK _SplashWnd_LayeredWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static int		g_nOpacity ;
  static HINSTANCE	g_hInstance ;
  static int		g_nDelta ;
  static BOOL		g_bWantClose ;
  static int		g_nShowTime ;
  
  LONG		nStyle ;
  BLENDFUNCTION bf ;
  FIBITMAP	*pfibImage ;

  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;
      g_nDelta		= SHOW_SPEED ;
      g_bWantClose	= FALSE ;
      g_nShowTime	= 0 ;
      g_nOpacity	= 0 ;

      nStyle = GetWindowLong (hwnd, GWL_EXSTYLE) ;
      SetWindowLong (hwnd, GWL_EXSTYLE, WS_EX_LAYERED|nStyle) ;
      
      pfibImage = _SplashWnd_LoadFromResource (g_hInstance, MAKEINTRESOURCE(IDB_SPLASH)) ;
      if( ! pfibImage ) {
	DestroyWindow (hwnd) ;
	return 0 ;
      }

      _SplashWnd_SetImage (g_hInstance, hwnd, 
			   pfibImage, g_nOpacity) ;  
      FreeImage_Unload (pfibImage) ;      
    
      SetTimer (hwnd, 0, 70, NULL) ;
  
      return 0 ;

    case WM_TIMER:
      {	
	if( g_nDelta )
	  {
	    g_nOpacity += g_nDelta ;

	    if( g_nOpacity>255 ) {
	      g_nOpacity = 255 ;
	      g_nDelta = 0 ;
	    }

	    if( g_nOpacity<0 ) {
	      g_nOpacity = 0 ;
	      g_nDelta = 0 ;
	      DestroyWindow (hwnd) ;
	    }
	    
	    bf.BlendOp = AC_SRC_OVER ;
	    bf.BlendFlags = 0 ;
	    bf.SourceConstantAlpha = g_nOpacity ;
	    bf.AlphaFormat  = AC_SRC_ALPHA ;

	    UpdateLayeredWindow (hwnd, NULL, NULL, NULL, 
				 NULL, NULL, 0, &bf, ULW_ALPHA);
	  }
	else 
	  {
	    g_nShowTime++ ;
	    
	    if( g_bWantClose && g_nShowTime>MIN_SHOW_TIME )
	      g_nDelta = -HIDE_SPEED ;
	  }
      }
      return 0 ;

    case WM_CLOSE:
      g_bWantClose = TRUE ;
      return 0 ;

    case WM_DESTROY:
      PostQuitMessage (0) ;
      return 0 ;
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


LRESULT CALLBACK _SplashWnd_RegionnedWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static int		g_nOpacity ;
  static HINSTANCE	g_hInstance ;
  static int		g_nDelta ;
  static BOOL		g_bWantClose ;
  static int		g_nShowTime ;
  static FIBITMAP	*g_pfibImage ;
  
  int		x, y, w, h ;
  HDC		hdc ;
  PAINTSTRUCT	ps ;
    

  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;
      g_nDelta		= SHOW_SPEED ;
      g_bWantClose	= FALSE ;
      g_nShowTime	= 0 ;
      g_nOpacity	= 0 ;
      
      g_pfibImage = _SplashWnd_LoadFromResource (g_hInstance, MAKEINTRESOURCE(IDB_SPLASH)) ;
      if( ! g_pfibImage ) {
	DestroyWindow (hwnd) ;
	return 0 ;
      }

      w = FreeImage_GetWidth (g_pfibImage) ;
      h = FreeImage_GetHeight (g_pfibImage) ;
      x = (GetSystemMetrics(SM_CXFULLSCREEN) - w) / 2 ;
      y = (GetSystemMetrics(SM_CYFULLSCREEN) - h) / 2 ;

      MoveWindow (hwnd, x, y, w, h, TRUE) ;

      SetTimer (hwnd, 0, 70, NULL) ;
      SetWindowRgn (hwnd, _SplashWnd_CreateRgnFromImage (g_pfibImage, 128), TRUE) ;
        
      return 0 ;


    case WM_PAINT:

      hdc = BeginPaint (hwnd, &ps) ;

      _SplashWnd_BltImage (hwnd, hdc, g_pfibImage) ;

      EndPaint (hwnd, &ps) ;

      return 0 ;
     

    case WM_TIMER:
      {	
	if( g_nDelta )
	  {
	    g_nOpacity += g_nDelta ;

	    if( g_nOpacity>255 ) {
	      g_nOpacity = 255 ;
	      g_nDelta = 0 ;
	    }

	    if( g_nOpacity<0 ) {
	      g_nOpacity = 0 ;
	      g_nDelta = 0 ;
	      DestroyWindow (hwnd) ;
	    }   
	  }
	else 
	  {
	    g_nShowTime++ ;
	    
	    if( g_bWantClose && g_nShowTime>MIN_SHOW_TIME )
	      g_nDelta = -HIDE_SPEED ;
	  }
      }
      return 0 ;

    case WM_CLOSE:
      g_bWantClose = TRUE ;
      return 0 ;

    case WM_DESTROY:
      FreeImage_Unload (g_pfibImage) ; 
      PostQuitMessage (0) ;
      return 0 ;
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


DWORD WINAPI _SplashWnd_Thread (void * pParam)
{
  MSG	msg ;
  HINSTANCE hInstance = GetModuleHandle(NULL) ;
  HWND hwnd = _SplashWnd_CreateWindow (hInstance) ;

  while( GetMessage (&msg,NULL,0,0) )
    {
      if( !msg.hwnd ) msg.hwnd = hwnd ;	
      TranslateMessage (&msg) ;
      DispatchMessage (&msg) ;
    }

  DestroyWindow (hwnd) ;

  return 0 ;
}

static DWORD g_dwThreadId ;
static HANDLE g_hThread ;

VOID SplashWnd_Show () 
{
  if( Config_GetInteger(CFGINT_SPLASH_SCREEN) )
    g_hThread = CreateThread (NULL, 0, _SplashWnd_Thread, NULL, 0, &g_dwThreadId) ;
}

VOID SplashWnd_Hide (BOOL bWait) 
{
  if( g_hThread )
    {
      PostThreadMessage (g_dwThreadId, WM_CLOSE, 0, 0) ;
      
      if( bWait )
	WaitForSingleObject (g_hThread, INFINITE) ;
      CloseHandle (g_hThread) ;
      
      g_dwThreadId = 0 ;
      g_hThread = NULL ;
    }
}
