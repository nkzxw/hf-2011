/******************************************************************************
Module:  InjLib.cpp
Notices: Copyright (c) 2000 Jeffrey Richter
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include "Resource.h"

#include "../lib/injlib.h"

#if (defined(UNICODE)||defined(_UNICODE))
#pragma comment(lib,"../lib/ReleaseUnicode/injlib.lib")
#else
#pragma comment(lib,"../lib/Release/injlib.lib")
#endif

HINSTANCE mhinstExe;

// Sets the dialog box icons
inline void chSETDLGICONS(HWND hwnd, int idi) {
   SendMessage(hwnd, WM_SETICON, ICON_BIG,  (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
         MAKEINTRESOURCE(idi)));
   SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) 
      LoadIcon((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
      MAKEINTRESOURCE(idi)));
}

// The normal HANDLE_MSG macro in WindowsX.h does not work properly for dialog
// boxes because DlgProc return a BOOL instead of an LRESULT (like
// WndProcs). This chHANDLE_DLGMSG macro corrects the problem:
#define chHANDLE_DLGMSG(hwnd, message, fn)                 \
   case (message): return (SetDlgMsgResult(hwnd, uMsg,     \
      HANDLE_##message((hwnd), (wParam), (lParam), (fn))))

inline void chWindows9xNotAllowed() {
   OSVERSIONINFO vi = { sizeof(vi) };
   GetVersionEx(&vi);
   if (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
       MessageBox(NULL,_T("This application requires features not present in Windows 9x."),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
      ExitProcess(0);
   }
}


BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {

   chSETDLGICONS(hwnd, IDI_INJLIB);
   return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
void Dlg_OnBrowse(HWND hwnd)
{
    TCHAR szFileName[MAX_PATH]=_T("");
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=hwnd;
    ofn.hInstance=mhinstExe;
    ofn.lpstrFilter=_T("dll\0*.dll\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt=_T("exe");
    ofn.lpstrFile=szFileName;
    ofn.nMaxFile=MAX_PATH;
    if (!GetOpenFileName(&ofn))
        return;

    SetDlgItemText(hwnd,IDC_EDIT_LIBPATH,ofn.lpstrFile);
}
///////////////////////////////////////////////////////////////////////////////

DWORD GetProcessId(HWND hwnd)
{
    TCHAR psz[MAX_PATH];
    DWORD dwPID;
    int iScanfRes;
    GetDlgItemText(hwnd, IDC_PROCESSID, psz, MAX_PATH-1);
    // convert to lower case
    _tcslwr(psz);
    if(_tcsncmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz,_T("0x%x"),&dwPID);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&dwPID);
    if ((!iScanfRes)||(!dwPID))
        return 0;

    return dwPID;
}

void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
   
   switch (id) {
      case IDCANCEL:
         EndDialog(hwnd, id);
         break;
      case IDC_BUTTON_BROWSE:
          Dlg_OnBrowse(hwnd);
          break;
      case IDC_INJECT:
      case IDC_BUTTON_EJECT:
         DWORD dwProcessId = GetProcessId(hwnd);
         BOOL bRet;
         if (dwProcessId == 0) {
            // A process ID of 0 causes everything to take place in the 
            // local process; this makes things easier for debugging.
            dwProcessId = GetCurrentProcessId();
         }
         TCHAR szLibFile[MAX_PATH];
         GetDlgItemText(hwnd,IDC_EDIT_LIBPATH,szLibFile,MAX_PATH);
         if (id==IDC_INJECT)
           bRet=InjectLib(dwProcessId, szLibFile);
         else
           bRet=EjectLib(dwProcessId, szLibFile);

         if (bRet)
            MessageBox(hwnd,_T("Operation Completed Successfully"),_T("Information"),MB_OK|MB_ICONINFORMATION);
         else
            MessageBox(hwnd,_T("Operation failed"),_T("Error"),MB_OK|MB_ICONERROR);
         break;
   }
}


///////////////////////////////////////////////////////////////////////////////

INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
      chHANDLE_DLGMSG(hwnd, WM_INITDIALOG, Dlg_OnInitDialog);
      chHANDLE_DLGMSG(hwnd, WM_COMMAND,    Dlg_OnCommand);
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////////////


int WINAPI _tWinMain(HINSTANCE hinstExe, HINSTANCE, PTSTR pszCmdLine, int) {
    mhinstExe=hinstExe;
   chWindows9xNotAllowed();
   DialogBox(hinstExe, MAKEINTRESOURCE(IDD_INJLIB), NULL, Dlg_Proc);
   return(0);
}


//////////////////////////////// End of File //////////////////////////////////
