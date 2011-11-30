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

#define TRACE_LEVEL	2 /* = warnings */


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "PicMenuCtl.h"

// standard headers
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "PicBtnCtl.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define WC_PICMENUCTL		TEXT("PicMenu")

#define MAX_ITEMS		16

#define WM_ADDITEM		(WM_USER+1)
#define WM_SETITEMLABEL		(WM_USER+2)
#define WM_SELECTITEM		(WM_USER+3)
#define WM_UPDATEITEMSPOS	(WM_USER+4)

#define CX_BUTTON		150
#define CY_BUTTON		100


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct 
{
  HWND		hwndButton ;
} MENUITEM ;

typedef struct
{
  MENUITEM	aItems[MAX_ITEMS] ;
  int		nItems ;
  int		iSelected ;
  SIZE		sizVirtual ;
  SIZE		sizReal ;
  BOOL		bVScroll ;
} WNDDATA ;

typedef struct
{
  UINT		nId ;
  LPCTSTR	szLabel ;
  LPCTSTR	szPicRsrc ;
} ADDITEM ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _PicMenuCtl_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL PicMenuCtl_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS	wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _PicMenuCtl_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = NULL ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_PICMENUCTL ;

  return RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HWND PicMenuCtl_CreateWindow (HINSTANCE hInstance, HWND hwndParent, UINT nStyle) 
{
  return CreateWindowEx (WS_EX_CLIENTEDGE, WC_PICMENUCTL, NULL,
			 WS_CHILD|WS_CLIPSIBLINGS|WS_VSCROLL|nStyle, 
			 0, 0, 1, 1, hwndParent, 
			 NULL, hInstance, NULL) ; 

}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL PicMenuCtl_AddItem (HWND hwnd, UINT nId, LPCTSTR szLabel, LPCTSTR szPicRsrc) 
{
  ADDITEM additem = { nId, szLabel, szPicRsrc } ;

  return SendMessage (hwnd, WM_ADDITEM, 0, (LPARAM)&additem) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL PicMenuCtl_SelectItem (HWND hwndMenu, UINT nId) 
{
  return SendMessage(hwndMenu, WM_SELECTITEM, nId, 0)==0 ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL PicMenuCtl_SetItemText (HWND hwndMenu, UINT nId, LPCTSTR szLabel) 
{
  return SendMessage (hwndMenu, WM_SETITEMLABEL, nId, (LPARAM)szLabel) ;
}


/******************************************************************/
/* Internal function :                                            */
/******************************************************************/

LRESULT CALLBACK _PicMenuCtl_WndProc (HWND hwnd, UINT message, 
				      WPARAM wParam, LPARAM lParam)
{ 
  static HINSTANCE	g_hInstance ;

  WNDDATA * pData = (WNDDATA*) GetWindowLong (hwnd, GWL_USERDATA) ;
    
  switch( message )
    {
    case WM_CREATE:
      {
	g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;
	
	pData = (WNDDATA*) malloc (sizeof(WNDDATA)) ;
	SetWindowLong (hwnd, GWL_USERDATA, (LONG)pData) ;
		
	pData->iSelected = -1 ;
	pData->nItems = 0 ;
	pData->sizVirtual.cx = CX_BUTTON ;
	pData->sizVirtual.cy = 0 ;

	SetScrollRange (hwnd, SB_VERT, 0, 0, FALSE) ;
	SetScrollPos (hwnd, SB_VERT, 0, FALSE) ;
      }
      return 0 ;// case WM_CREATE:

    case WM_DESTROY:
      {
	free (pData) ;
      }
      return 0 ;

    case WM_SIZE:
      {
	BOOL bNeedVScroll ;

   	pData->sizReal.cx = LOWORD(lParam) ;
	pData->sizReal.cy = HIWORD(lParam) ;

	bNeedVScroll = pData->sizReal.cy < pData->sizVirtual.cy ;

	if( pData->bVScroll!=bNeedVScroll )
	  {
	    RECT	rect ;

	    pData->bVScroll = bNeedVScroll ;

	    ShowScrollBar (hwnd, SB_VERT, pData->bVScroll) ;

	    GetClientRect (hwnd, &rect) ;

	    pData->sizReal.cx = rect.right ;
	    pData->sizReal.cy = rect.bottom ;
	  }

	if( pData->bVScroll )
	  {
	    int dy = pData->sizVirtual.cy-pData->sizReal.cy ;

	    SetScrollRange (hwnd, SB_VERT, 0, dy, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, GetScrollPos(hwnd,SB_VERT)) ;
	  }
	else
	  SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, 0) ;
      }      
      return 0 ; 
	
    case WM_COMMAND:

      PicMenuCtl_SelectItem (hwnd, LOWORD(wParam)) ;

      PostMessage (GetParent(hwnd), WM_COMMAND, MAKEWPARAM(LOWORD(wParam),0), (LPARAM)hwnd) ;
      
      return 0 ;

    case WM_UPDATEITEMSPOS:
      {
	int i ;

	int cx = pData->sizReal.cx ;
	int cy = CY_BUTTON ;
	int x = 0 ;
	int y = -lParam ;

	for( i=0 ; i<pData->nItems ; i++ )
	  {
	    MoveWindow (pData->aItems[i].hwndButton,
			x, y, cx, cy, TRUE) ;

	    y += cy ;
	  }
      }
      return 0 ;

    case WM_ADDITEM:
      {
	MENUITEM	*pItem ;
	ADDITEM		*pAddItem = (ADDITEM*)lParam ;
	INT		y ;

	ASSERT (pData->nItems+1<MAX_ITEMS) ;

	pItem = &pData->aItems[pData->nItems++] ;
	
	y = pData->sizVirtual.cy ;
	pData->sizVirtual.cy += CY_BUTTON ;

	pItem->hwndButton = CreateWindow (WC_PICBTNCTL, pAddItem->szLabel,
					  WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE, 
					  0, y, CX_BUTTON, CY_BUTTON, hwnd, 
					  (HMENU)pAddItem->nId, g_hInstance, NULL) ; 
	PicBtnCtl_SetImage (pItem->hwndButton, pAddItem->szPicRsrc) ;

      }
      return 0 ;


    case WM_SELECTITEM:
      {
	HWND	hwndCurButton ;
	int i ;

	if( pData->iSelected==wParam )
	  return 0 ;

	for( i=0 ; i<pData->nItems ; i++ )
	  {
	    hwndCurButton = pData->aItems[i].hwndButton ;

	    if( (HMENU)pData->iSelected==GetMenu(hwndCurButton) )
	      PicBtnCtl_SetSelected (hwndCurButton, FALSE) ;

	    if( (HMENU)wParam==GetMenu(hwndCurButton) )
	      PicBtnCtl_SetSelected (hwndCurButton, TRUE) ;
	  }

	pData->iSelected = wParam ;
      }
      return  0 ;


    case WM_SETITEMLABEL:
      {
	HWND	hwndCurButton ;
	int i ;

	for( i=0 ; i<pData->nItems ; i++ )
	  {
	    hwndCurButton = pData->aItems[i].hwndButton ;

	    if( (HMENU)wParam==GetMenu(hwndCurButton) )
	      SetWindowText (hwndCurButton, (LPCTSTR)lParam) ;
	  }
      }
      return  0 ;

    case WM_MOUSEWHEEL:
      {
	int iPos ;

	TRACE_INFO ("Mouse wheel %d\n", (SHORT)HIWORD(wParam)) ;
      
	iPos = GetScrollPos(hwnd,SB_VERT) - (SHORT)HIWORD(wParam) / 12 ;

	iPos = min(pData->sizVirtual.cy-pData->sizReal.cy, iPos) ;
	iPos = max(0, iPos) ;
		   
	SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;	
      }
      return 0 ;

    case WM_VSCROLL:
      {
	int iPos ;

	switch( LOWORD(wParam) )
	  {
	  case SB_TOP:
	    iPos = 0 ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_BOTTOM:
	    iPos = pData->sizVirtual.cy-pData->sizReal.cy ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_LINEUP:
	    iPos = max(0, GetScrollPos(hwnd,SB_VERT)-10) ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_LINEDOWN:
	    iPos = min(pData->sizVirtual.cy-pData->sizReal.cy, 
		       GetScrollPos(hwnd,SB_VERT)+10) ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_PAGEUP:
	    iPos = max(0, GetScrollPos(hwnd,SB_VERT)-100) ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_PAGEDOWN:
	    iPos = min(pData->sizVirtual.cy-pData->sizReal.cy, 
		       GetScrollPos(hwnd,SB_VERT)+100) ;
	    SetScrollPos (hwnd, SB_VERT, iPos, TRUE) ;
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, iPos) ;
	    break ;
	  case SB_THUMBPOSITION:
	    SetScrollPos (hwnd, SB_VERT, HIWORD(wParam), FALSE) ;
	  case SB_THUMBTRACK:
	    SendMessage (hwnd, WM_UPDATEITEMSPOS, 0, HIWORD(wParam)) ;
	    break ;
	    
	  }	
      }
      return 0 ;
      
    } 

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}
