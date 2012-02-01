
#include <windows.h>
#include "SoftSnoop.h"
#include "lib.h"
#include "resource.h"

// function proto's
VOID     CreateRegsDlg(HINSTANCE hInst,HANDLE hDlgOwner);
LRESULT  CALLBACK RegEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
VOID     WriteRegDlgStatus(CHAR* szText);
BOOL     CALLBACK RegsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL     ShowRegs(HWND hDlg,HANDLE hThread);
BOOL     SaveRegs(HWND hDlg);

// constants
#define          MAX_REGVALUESIZE   8
CONST  CHAR*     szOkStatus       = "All is OK...";
CONST  CHAR*     szGetRegsErr     = "Error while receiving Registers :(";
CONST  CHAR*     szRegsSaved      = "Registers saved successfully :)";
CONST  CHAR*     szSaveRegsErr    = "Error while saving Registers :(";

// variables
HWND             hRegDlg;
WNDPROC          pOrgRegEditProc;

VOID CreateRegsDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	WNDCLASS wc;

	// create a dlg;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_RegsMenu";
	RegisterClass(&wc);
		
	DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_REGS),(HWND)hDlgOwner,RegsDlgProc,NULL);
	return;
}

LRESULT CALLBACK RegEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	CHAR  c = 0;

	switch(Msg)
	{
	case WM_CHAR:
		// force hex characters
		if (wParam != VK_BACK)
		{
			c = toupper(wParam);
			if ( (c < '0' || c > '9') &&
				 (c < 'A' || c > 'F'))
				 return 0;
		}
		break;
	}
	return CallWindowProc(pOrgRegEditProc, hWnd, Msg, wParam, lParam);
}

VOID WriteRegDlgStatus(CHAR* szText)
{
	SetDlgItemText(hRegDlg,IDC_REGDLGSTATUS,szText);
	return;
}

BOOL CALLBACK RegsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DWORD  i;
	CHAR   cCBBuff[30];

	switch(Msg)
	{
	case WM_INITDIALOG:
		hRegDlg = hDlg;
		WriteRegDlgStatus((CHAR*)szOkStatus);
		// put thread id's in the combobox
		for (i=0; i < dwThreadCount; i++)
		{
			if (!i)
			{
				wsprintf(cCBBuff,"%X (Main Thread)",ThreadList[i].dwThreadID);
				if (!ShowRegs(hDlg,ThreadList[i].hThread))
					WriteRegDlgStatus((CHAR*)szGetRegsErr);
				else
					WriteRegDlgStatus((CHAR*)szOkStatus);
			}
			else
				wsprintf(cCBBuff,"%08lX",ThreadList[i].dwThreadID);
			SendDlgItemMessage(hDlg,IDC_THREADLIST,CB_ADDSTRING,0,(LPARAM)cCBBuff);
		}
		SendDlgItemMessage(hDlg,IDC_THREADLIST,CB_SETCURSEL,0,0);
		// limit the text of the register editbox's to 8
		SendDlgItemMessage(hDlg,IDC_REGEAX,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGEBX,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGECX,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGEDX,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGEDI,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGESI,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGESP,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGEBP,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGEIP,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		SendDlgItemMessage(hDlg,IDC_REGFLAGS,EM_SETLIMITTEXT,MAX_REGVALUESIZE,0);
		// hook edit procs
		pOrgRegEditProc = (WNDPROC)SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEAX),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEBX),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGECX),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEDX),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEDI),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGESI),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGESP),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEBP),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGEIP),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_REGFLAGS),
			GWL_WNDPROC,
			(LONG)RegEditProc);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_SAVEREGS:
			if (SaveRegs(hRegDlg))
				WriteRegDlgStatus((CHAR*)szRegsSaved);
			else
				WriteRegDlgStatus((CHAR*)szSaveRegsErr);
			return TRUE;

		case ID_CANCELREGSDLG:
			EndDialog(hDlg,0);
			return TRUE;
		}
		// process notification msg's
		switch(HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			if (LOWORD(wParam) == IDC_THREADLIST)
			{
				i = SendDlgItemMessage(hRegDlg,IDC_THREADLIST,CB_GETCURSEL,0,0);
				if (ShowRegs(
					hRegDlg,
					ThreadList[i].hThread))
					WriteRegDlgStatus((CHAR*)szOkStatus);
				else
					WriteRegDlgStatus((CHAR*)szGetRegsErr);					
			}
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg,0);
		return TRUE;
	}

	return FALSE;
}

BOOL ShowRegs(HWND hDlg,HANDLE hThread)
{
	CONTEXT  Regs;
	CHAR     cRegBuff[10];

	// get register snapshot
	Regs.ContextFlags = CONTEXT_FULL;
	if (!GetThreadContext(hThread,&Regs))
		return FALSE;

	// show register values
	wsprintf(cRegBuff,"%08lX",Regs.Eax);
	SetDlgItemText(hDlg,IDC_REGEAX,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Ebx);
	SetDlgItemText(hDlg,IDC_REGEBX,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Ecx);
	SetDlgItemText(hDlg,IDC_REGECX,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Edx);
	SetDlgItemText(hDlg,IDC_REGEDX,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Edi);
	SetDlgItemText(hDlg,IDC_REGEDI,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Esi);
	SetDlgItemText(hDlg,IDC_REGESI,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Esp);
	SetDlgItemText(hDlg,IDC_REGESP,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Ebp);
	SetDlgItemText(hDlg,IDC_REGEBP,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.Eip);
	SetDlgItemText(hDlg,IDC_REGEIP,cRegBuff);
	wsprintf(cRegBuff,"%08lX",Regs.EFlags);
	SetDlgItemText(hDlg,IDC_REGFLAGS,cRegBuff);

	return TRUE;
}

BOOL SaveRegs(HWND hDlg)
{
	DWORD    iIndex;
	CONTEXT  Regs;
	CHAR     cRegBuff[10];

	// get the context of the target thread
	iIndex = SendDlgItemMessage(hDlg,IDC_THREADLIST,CB_GETCURSEL,0,0);
	Regs.ContextFlags = CONTEXT_FULL;
	if (!GetThreadContext(ThreadList[iIndex].hThread,&Regs))
		return FALSE;

	// process all reg editbox's
	GetDlgItemText(hDlg,IDC_REGEAX,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Eax);
	GetDlgItemText(hDlg,IDC_REGEBX,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Ebx);
	GetDlgItemText(hDlg,IDC_REGECX,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Ecx);
	GetDlgItemText(hDlg,IDC_REGEDX,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Edx);
	GetDlgItemText(hDlg,IDC_REGESI,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Esi);
	GetDlgItemText(hDlg,IDC_REGEDI,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Edi);
	GetDlgItemText(hDlg,IDC_REGEBP,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Ebp);
	GetDlgItemText(hDlg,IDC_REGESP,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Esp);
	GetDlgItemText(hDlg,IDC_REGEIP,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.Eip);
	GetDlgItemText(hDlg,IDC_REGFLAGS,cRegBuff,sizeof(cRegBuff));
	Hexstr2Int(cRegBuff,Regs.EFlags);

	// set the new CONTEXT
	if (!SetThreadContext(ThreadList[iIndex].hThread,&Regs))
		return FALSE;

	return TRUE;
}