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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "PicBtnCtl.h"

// standard headers
#include <windows.h>
#include <tchar.h>

// libraries' headers
#define _WINDOWS_ // <-- workaround needed for freeimage 3.9.1
#include <FreeImage.h>

// project's headers
#include "Assert.h"
#include "Strlcpy.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

typedef struct {
  TCHAR		szLabel[32] ;
  HBITMAP	hbmImage ;
  int		xImg, yImg ;
  int		cxImg, cyImg ;
  int		xLabel, yLabel ;
  int		cxLabel, cyLabel ;
  int		nChildId ;
  BOOL		bMouseOver, bSelected ;
  HBRUSH	hbBackground ;
  BOOL		bUsePerPixelAlpha ;
  HRGN		hRegion ;
} WNDDATA ;

#define PBM_RECALC_POSITIONS	PBM_INTERNAL_1


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _PicBtnCtl_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

// TO BE MODIFIED !!
FIBITMAP * _SplashWnd_LoadFromResource (HINSTANCE hInstance, LPTSTR lpName) ;
#define _PicBtnCtl_LoadFromResource _SplashWnd_LoadFromResource 

// TO BE MODIFIED !!
void _SplashWnd_MultiplyByAlpha (FIBITMAP * pfibImage) ;
#define _PicBtnCtl_MultiplyByAlpha _SplashWnd_MultiplyByAlpha

// TO BE MODIFIED !!
BOOL _SplashWnd_CanUsePerPixelAlpha () ;
#define _PicBtnCtl_CanUsePerPixelAlpha _SplashWnd_CanUsePerPixelAlpha

// TO BE MODIFIED !
HRGN _SplashWnd_CreateRgnFromImage (FIBITMAP * pfibImage, int nThreshold) ;
#define _PicBtnCtl_CreateRgnFromImage _SplashWnd_CreateRgnFromImage


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL PicBtnCtl_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS	wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _PicBtnCtl_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = NULL ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = NULL ;//(HBRUSH)(COLOR_WINDOW+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_PICBTNCTL ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _PicBtnCtl_WndProc (HWND hwnd, UINT message, 
				     WPARAM wParam, LPARAM lParam)
{  
  static HINSTANCE	g_hInstance ;
  
  WNDDATA * pData = (WNDDATA*) GetWindowLong (hwnd, GWL_USERDATA) ;

  HDC		hdcImage ;
  HDC		hdcScreen ;
  FIBITMAP	*pfibImage ;
  PAINTSTRUCT	ps ;
  BLENDFUNCTION	bf ;
  BOOL		bMouseOver ;
  int		x, y ;
  RECT		rect ;
  
  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;

      pData = calloc (1, sizeof(WNDDATA)) ;
      SetWindowLong (hwnd, GWL_USERDATA, (LONG)pData) ;
      
      pData->bUsePerPixelAlpha = _PicBtnCtl_CanUsePerPixelAlpha () ;
      pData->nChildId = (int) ((CREATESTRUCT*)lParam)->hMenu ;
      pData->hbBackground = (HBRUSH) GetClassLong (GetParent(hwnd), GCL_HBRBACKGROUND) ;

      pData->szLabel[0] = 0 ;
      GetWindowText (hwnd, pData->szLabel, 32) ;

      SendMessage (hwnd, PBM_RECALC_POSITIONS, 0, 0) ;
      	
      return 0 ; // case WM_CREATE:

    case WM_DESTROY: 
      
      if( pData->hbmImage )
	DeleteObject (pData->hbmImage) ;
 
      return 0 ; // case WM_DESTROY:   

    case WM_SIZE:

      SendMessage (hwnd, PBM_RECALC_POSITIONS, 0, 0) ;

      return 0 ;

    case WM_SETTEXT:

      _tcslcpy (pData->szLabel, (LPCTSTR)lParam, 32) ;

      SendMessage (hwnd, PBM_RECALC_POSITIONS, 0, 0) ;
      InvalidateRect (hwnd, NULL, TRUE) ; 

      break ;   // case WM_SETTEXT:

    case PBM_RECALC_POSITIONS:

      if( pData->szLabel[0] ) {
	rect.left = rect.top = 0 ;
	hdcScreen	= GetDC (hwnd) ;
	DrawText (hdcScreen, pData->szLabel, -1, &rect, DT_CALCRECT) ;
	ReleaseDC (hwnd, hdcScreen) ;
	pData->cxLabel = rect.right ;
	pData->cyLabel = rect.bottom ;
      }
      else {
	pData->cxLabel = 0 ;
	pData->cyLabel = 0 ;
      }

      if( pData->hRegion )
      	OffsetRgn (pData->hRegion, -pData->xImg, -pData->yImg) ;

      GetClientRect (hwnd, &rect) ;
      pData->xImg = ( rect.right - pData->cxImg ) / 2 ;
      pData->yImg = ( rect.bottom - pData->cyImg - pData->cyLabel ) / 2 ;
      pData->xLabel = ( rect.right - pData->cxLabel ) / 2 ;
      pData->yLabel = pData->yImg + pData->cyImg ;

      if( pData->hRegion )
        OffsetRgn (pData->hRegion, pData->xImg, pData->yImg) ;

      return 0 ; //  case PBM_RECALC_POSITIONS:

    case PBM_SETSELETED:

      pData->bSelected = lParam ;

      InvalidateRect (hwnd, NULL, TRUE) ; 

      return 0 ;

    case PBM_SETIMAGE:

      if( pData->hbmImage )
	DeleteObject (pData->hbmImage) ;
      if( pData->hRegion )
	DeleteObject (pData->hRegion) ;
             
      pfibImage = _PicBtnCtl_LoadFromResource (g_hInstance, (LPTSTR)lParam) ;
      ASSERT (pfibImage!=NULL) ;

      _PicBtnCtl_MultiplyByAlpha (pfibImage) ;
      
      hdcScreen	= GetDC (hwnd) ;

      pData->hbmImage = CreateDIBitmap (hdcScreen, FreeImage_GetInfoHeader(pfibImage),
					CBM_INIT, FreeImage_GetBits(pfibImage),
					FreeImage_GetInfo(pfibImage), DIB_RGB_COLORS) ;
      ASSERT (pData->hbmImage) ; 

      pData->cxImg = FreeImage_GetWidth(pfibImage) ;
      pData->cyImg = FreeImage_GetHeight(pfibImage) ;

      if( ! pData->bUsePerPixelAlpha ) 
	pData->hRegion = _PicBtnCtl_CreateRgnFromImage (pfibImage, 128) ;
      else
	pData->hRegion = NULL ;

      FreeImage_Unload (pfibImage) ;         
  
      ReleaseDC (hwnd, hdcScreen) ;

      pData->xImg = 0 ;
      pData->yImg = 0 ;
      SendMessage (hwnd, PBM_RECALC_POSITIONS, 0, 0) ;

      InvalidateRect (hwnd, NULL, TRUE) ;      

      return 0 ; // case PBM_SETIMAGE:

    case PBM_SETBACKGROUND:

      pData->hbBackground = (HBRUSH) lParam ;

      InvalidateRect (hwnd, NULL, TRUE) ; 

      return 0 ;

    case WM_ERASEBKGND:
      
      if( pData->hbBackground )
	{
	  hdcScreen = (HDC) wParam ;

	  GetClientRect (hwnd, &rect) ;

	  FillRect (hdcScreen, &rect, pData->hbBackground) ;	 
	}

      return TRUE ;


    case WM_PAINT:
      
      hdcScreen = BeginPaint (hwnd, &ps) ;

      if( pData->szLabel && pData->szLabel[0] )
	{
	  SetBkMode (hdcScreen, TRANSPARENT) ;

	  if( pData->bSelected )
	    SetTextColor (hdcScreen, RGB(0,0,0)) ;
	  else if( pData->bMouseOver )
	    SetTextColor (hdcScreen, RGB(128,128,128)) ;
	  else
	    SetTextColor (hdcScreen, RGB(192,192,192)) ;
	  
	  rect.left		= pData->xLabel ;
	  rect.top		= pData->yLabel ;
	  rect.right	= pData->xLabel + pData->cxLabel ;
	  rect.bottom	= pData->yLabel + pData->cyLabel ;
	  DrawText (hdcScreen, pData->szLabel, -1, &rect, 0) ;
	}
      
      hdcImage	= CreateCompatibleDC (hdcScreen) ;
      ASSERT (hdcImage!=NULL) ;
  
      ASSERT (pData->hbmImage!=NULL) ;
      SelectObject (hdcImage, pData->hbmImage) ;

      if( pData->bUsePerPixelAlpha )
	{
	  bf.BlendOp	= AC_SRC_OVER ;
	  bf.BlendFlags	= 0 ;
	  bf.SourceConstantAlpha = pData->bSelected ? 255 : pData->bMouseOver ? 128 : 64 ;
	  bf.AlphaFormat	= AC_SRC_ALPHA ;
	  
	  AlphaBlend (hdcScreen, 
		      pData->xImg, pData->yImg,  
		      pData->cxImg, pData->cyImg, 
		      hdcImage, 
		      0, 0, 
		      pData->cxImg, pData->cyImg, 
		      bf) ;
	}
      else
	{
	  ASSERT (pData->hRegion!=NULL) ;
	  
	  SelectObject (hdcScreen, pData->hRegion) ;

	  BitBlt (hdcScreen, 
		  pData->xImg, pData->yImg, 
		  pData->cxImg, pData->cyImg, 
		  hdcImage, 0, 0, SRCCOPY) ;
	}
               
      DeleteDC (hdcImage) ;

      EndPaint (hwnd, &ps) ;
      
      return 0 ;

    case WM_MOUSEMOVE:

      x = LOWORD (lParam) ;
      y = HIWORD (lParam) ;

      bMouseOver = 
	x>=pData->xImg && y>=pData->yImg 
	&& x<pData->xImg+pData->cxImg && y<pData->yImg+pData->cyImg ;

      if( bMouseOver && !pData->bMouseOver )
	SetCapture (hwnd) ;
            
      if( !bMouseOver && pData->bMouseOver )
	ReleaseCapture () ;

      if( bMouseOver != pData->bMouseOver )
	InvalidateRect (hwnd, NULL, TRUE) ;

      pData->bMouseOver = bMouseOver ;

      return 0 ;

    case WM_LBUTTONDOWN:

      if( pData->bMouseOver )
	PostMessage (GetParent(hwnd), WM_COMMAND, 
		     MAKEWPARAM(pData->nChildId,BN_CLICKED), 
		     (LPARAM)hwnd) ;

      SetFocus (hwnd) ;
	
      return 0 ;
    }      

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


