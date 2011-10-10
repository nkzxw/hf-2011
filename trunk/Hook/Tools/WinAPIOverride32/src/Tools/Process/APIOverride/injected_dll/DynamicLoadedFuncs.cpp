/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: avoid static linking with library other than kernel32
//-----------------------------------------------------------------------------

#include "DynamicLoadedFuncs.h"

extern CSetUserWindowStation* pSetUserWindowStation;

typedef BOOL (__stdcall *pfDeleteObject)(HGDIOBJ hObject);

typedef HFONT (__stdcall *pfCreateFont)(
  int nHeight,               // height of font
  int nWidth,                // average character width
  int nEscapement,           // angle of escapement
  int nOrientation,          // base-line orientation angle
  int fnWeight,              // font weight
  DWORD fdwItalic,           // italic attribute option
  DWORD fdwUnderline,        // underline attribute option
  DWORD fdwStrikeOut,        // strikeout attribute option
  DWORD fdwCharSet,          // character set identifier
  DWORD fdwOutputPrecision,  // output precision
  DWORD fdwClipPrecision,    // clipping precision
  DWORD fdwQuality,          // output quality
  DWORD fdwPitchAndFamily,   // pitch and family
  TCHAR* lpszFace           // typeface name
);
typedef INT_PTR (__stdcall *pfDialogBoxParam)(HINSTANCE hInstance,
    TCHAR* lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam
);

typedef BOOL (__stdcall *pfEndDialog)(HWND hDlg,
    INT_PTR nResult
);
typedef HWND (__stdcall *pfGetDlgItem)(HWND hDlg,
    int nIDDlgItem
);
typedef LRESULT (__stdcall *pfSendMessage)(HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
);
typedef LONG (__stdcall *pfGetWindowLong)(HWND hWnd,
    int nIndex
);
typedef LONG (__stdcall *pfSetWindowLongPtr)(HWND hWnd,
    int nIndex,
    LONG dwNewLong
);

typedef BOOL (__stdcall *pfEnableWindow)(HWND hWnd,BOOL bEnable);

HMODULE DynamicLoadedFuncs_hModUser32=NULL;
HMODULE DynamicLoadedFuncs_hModGdi=NULL;

pfCreateFont pCreateFont=NULL;
pfSendMessage pSendMessage=NULL;
pfGetDlgItem pGetDlgItem=NULL;
pfEndDialog pEndDialog=NULL;
pfDialogBoxParam pDialogBoxParam=NULL;
pfSetWindowLongPtr pSetWindowLongPtr=NULL;
pfGetWindowLong pGetWindowLong=NULL;
pfDeleteObject pDeleteObject=NULL;
pfEnableWindow pEnableWindow=NULL;

HMODULE DynamicLoadedFunction_LoadModule(TCHAR* pszModulName)
{
    HMODULE hMod=GetModuleHandle(pszModulName);
    if (!hMod)// if user32.dll was not loaded
        hMod=LoadLibrary(pszModulName);
    return hMod;
}

BOOL DynamicEnableWindow(HWND hWnd,BOOL bEnable)
{
    // get proc address of func
    if (!pEnableWindow)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return NULL;
        }
        pEnableWindow=(pfEnableWindow)GetProcAddress(DynamicLoadedFuncs_hModUser32,"EnableWindow");

        if (!pEnableWindow)
            return NULL;
    }
    // now pFunc is ok call it
    return pEnableWindow(hWnd,bEnable);
}

int __stdcall DynamicMessageBoxInDefaultStation(IN HWND hWnd,IN TCHAR* lpText,IN TCHAR* lpCaption,IN UINT uType)
{
    if (!pSetUserWindowStation->IsUserStation())
        uType|=MB_SERVICE_NOTIFICATION;
    return DynamicMessageBox(hWnd,lpText,lpCaption,uType);
}

LONG DynamicGetWindowLong(          HWND hWnd,
    int nIndex
)
{
    // get proc address of func
    if (!pGetWindowLong)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return NULL;
        }
#if (defined(UNICODE)||(_UNICODE))
        pGetWindowLong=(pfGetWindowLong)GetProcAddress(DynamicLoadedFuncs_hModUser32,"GetWindowLongW");
#else
        pGetWindowLong=(pfGetWindowLong)GetProcAddress(DynamicLoadedFuncs_hModUser32,"GetWindowLongA");
#endif

        if (!pGetWindowLong)
            return NULL;
    }

    // now pFunc is ok call it
    return pGetWindowLong(hWnd,nIndex);
}
LONG DynamicSetWindowLongPtr(          HWND hWnd,
    int nIndex,
    LONG_PTR dwNewLong
)
{
    // get proc address of func
    if (!pSetWindowLongPtr)
    {
#ifdef  _WIN64
        // to be implemented
#pragma message ("\r\n")
#pragma message (__FILE__ " WARNING : DynamicSetWindowLongPtr MUST BE IMPLEMENTED FOR 64 bits OS \r\n")
#pragma message ("\r\n")

#else

        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return NULL;
        }
#if (defined(UNICODE)||(_UNICODE))
        pSetWindowLongPtr=(pfSetWindowLongPtr)GetProcAddress(DynamicLoadedFuncs_hModUser32,"SetWindowLongW");
#else
        pSetWindowLongPtr=(pfSetWindowLongPtr)GetProcAddress(DynamicLoadedFuncs_hModUser32,"SetWindowLongA");
#endif
#endif
        if (!pSetWindowLongPtr)
            return NULL;
    }
    // now pFunc is ok call it
    return pSetWindowLongPtr(hWnd,nIndex,dwNewLong);
}

HFONT DynamicCreateFont(
  int nHeight,               // height of font
  int nWidth,                // average character width
  int nEscapement,           // angle of escapement
  int nOrientation,          // base-line orientation angle
  int fnWeight,              // font weight
  DWORD fdwItalic,           // italic attribute option
  DWORD fdwUnderline,        // underline attribute option
  DWORD fdwStrikeOut,        // strikeout attribute option
  DWORD fdwCharSet,          // character set identifier
  DWORD fdwOutputPrecision,  // output precision
  DWORD fdwClipPrecision,    // clipping precision
  DWORD fdwQuality,          // output quality
  DWORD fdwPitchAndFamily,   // pitch and family
  TCHAR* lpszFace           // typeface name
)
{
    // get proc address of func
    if (!pCreateFont)
    {
        // if we don't have a handle to gdi32.dll try to get it
        if (!DynamicLoadedFuncs_hModGdi)
        {
            DynamicLoadedFuncs_hModGdi=DynamicLoadedFunction_LoadModule(_T("GDI32.dll"));
            if (!DynamicLoadedFuncs_hModGdi)
                return NULL;
        }
#if (defined(UNICODE)||(_UNICODE))
        pCreateFont=(pfCreateFont)GetProcAddress(DynamicLoadedFuncs_hModGdi,"CreateFontW");
#else
        pCreateFont=(pfCreateFont)GetProcAddress(DynamicLoadedFuncs_hModGdi,"CreateFontA");
#endif
        if (!pCreateFont)
            return NULL;
    }
    // now pFunc is ok call it
    return pCreateFont(
                        nHeight,             
                        nWidth,              
                        nEscapement,         
                        nOrientation,        
                        fnWeight,            
                        fdwItalic,         
                        fdwUnderline,      
                        fdwStrikeOut,      
                        fdwCharSet,        
                        fdwOutputPrecision,
                        fdwClipPrecision,  
                        fdwQuality,        
                        fdwPitchAndFamily, 
                        lpszFace);
}


BOOL DynamicDeleteObject(HGDIOBJ hObject)
{
    if (!pDeleteObject)
    {
        // if we don't have a handle to gdi32.dll try to get it
        if (!DynamicLoadedFuncs_hModGdi)
        {
            DynamicLoadedFuncs_hModGdi=DynamicLoadedFunction_LoadModule(_T("GDI32.dll"));
            if (!DynamicLoadedFuncs_hModGdi)
                return FALSE;
        }
        pDeleteObject=(pfDeleteObject)GetProcAddress(DynamicLoadedFuncs_hModGdi,"DeleteObject");
        if (!pDeleteObject)
            return FALSE;
    }
    // now pFunc is ok call it
    return pDeleteObject(hObject);
}

INT_PTR DynamicDialogBoxParam(HINSTANCE hInstance,
    TCHAR* lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam
)

{
    // get proc address of func
    if (!pDialogBoxParam)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return -1;
        }
#if (defined(UNICODE)||(_UNICODE))
        pDialogBoxParam=(pfDialogBoxParam)GetProcAddress(DynamicLoadedFuncs_hModUser32,"DialogBoxParamW");
#else
        pDialogBoxParam=(pfDialogBoxParam)GetProcAddress(DynamicLoadedFuncs_hModUser32,"DialogBoxParamA");
#endif
        if (!pDialogBoxParam)
            return -1;
    }
    // now pFunc is ok call it
    return pDialogBoxParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
}

BOOL DynamicEndDialog(HWND hDlg,
                    INT_PTR nResult
                    )
{
    // get proc address of func
    if (!pEndDialog)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return FALSE;
        }
        pEndDialog=(pfEndDialog)GetProcAddress(DynamicLoadedFuncs_hModUser32,"EndDialog");

        if (!pEndDialog)
            return FALSE;
    }
    // now pFunc is ok call it
    return pEndDialog(hDlg,nResult);
}
HWND DynamicGetDlgItem(HWND hDlg,
    int nIDDlgItem
)

{
    // get proc address of func
    if (!pGetDlgItem)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return NULL;
        }
        pGetDlgItem=(pfGetDlgItem)GetProcAddress(DynamicLoadedFuncs_hModUser32,"GetDlgItem");

        if (!pGetDlgItem)
            return NULL;
    }
    // now pFunc is ok call it
    return pGetDlgItem(hDlg,nIDDlgItem);
}
LRESULT DynamicSendMessage(HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    // get proc address of func
    if (!pSendMessage)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!DynamicLoadedFuncs_hModUser32)
        {
            DynamicLoadedFuncs_hModUser32=DynamicLoadedFunction_LoadModule(_T("user32.dll"));
            if (!DynamicLoadedFuncs_hModUser32)
                return -1;
        }
#if (defined(UNICODE)||(_UNICODE))
        pSendMessage=(pfSendMessage)GetProcAddress(DynamicLoadedFuncs_hModUser32,"SendMessageW");
#else
        pSendMessage=(pfSendMessage)GetProcAddress(DynamicLoadedFuncs_hModUser32,"SendMessageA");
#endif
        if (!pSendMessage)
            return -1;
    }
    // now pFunc is ok call it
    return pSendMessage(hWnd,Msg,wParam,lParam);
}