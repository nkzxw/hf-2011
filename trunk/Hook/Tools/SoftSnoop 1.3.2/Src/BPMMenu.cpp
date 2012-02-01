
#include "BPMMenu.h"

void  WriteBPMStatus(char* szStr);
void  CreateBPMDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL  CALLBACK BPMDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL  ShowDrxInfo(HWND hDlg, DWORD dwThID);
//BOOL  IsThreadSuspended(HANDLE hTh);
BOOL  SetThreadBPMs();
BOOL  HandleBPM(DWORD dwThID);

// constants
#define CON_DRX                            CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS
#define MAX_RW_OPCODE                      20

const char*            szH           = "%08lX";
const char*            szExec        = "execution";
const char*            szWrite       = "write";
const char*            szRW          = "read/write";
const char*            szErr         = "Error!";
const char*            szOk          = "All Ok.";

// global variables
HWND          hBPMDlg;
LONG          pOrgBPMEditWndProc;


void WriteBPMStatus(char* szStr)
{
	SDIT(hBPMDlg, IDC_BPMSTATUS, szStr);
	return;
}

LRESULT CALLBACK BPMAddrEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc((WNDPROC)pOrgBPMEditWndProc, hWnd, Msg, wParam, lParam);
}

void CreateBPMDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	DialogBox(hInst,MAKEINTRESOURCE(IDD_BPM),(HWND)hDlgOwner,BPMDialogFunct);
	return;
}

BOOL CALLBACK BPMDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char   cBuff[9];
	DWORD  i;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hBPMDlg = hDlg;
		WriteBPMStatus((char*)szOk);
		// hook edit boxe procedures and set max text
		pOrgBPMEditWndProc = SetWindowLong(
			GetDlgItem(hDlg, IDC_BPM0ADDR),
			GWL_WNDPROC,
			(LONG)BPMAddrEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_BPM1ADDR),
			GWL_WNDPROC,
			(LONG)BPMAddrEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_BPM2ADDR),
			GWL_WNDPROC,
			(LONG)BPMAddrEditProc);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_BPM3ADDR),
			GWL_WNDPROC,
			(LONG)BPMAddrEditProc);
		SDIM(hDlg, IDC_BPM0ADDR, EM_SETLIMITTEXT, 8, 0);
		SDIM(hDlg, IDC_BPM1ADDR, EM_SETLIMITTEXT, 8, 0);
		SDIM(hDlg, IDC_BPM2ADDR, EM_SETLIMITTEXT, 8, 0);
		SDIM(hDlg, IDC_BPM3ADDR, EM_SETLIMITTEXT, 8, 0);
		// paste thread IDs
		for (i=0; i < dwThreadCount; i++)
		{
			wsprintf(cBuff, szH, ThreadList[i].dwThreadID);
			SDIM(hDlg, IDC_BPMTHREAD, CB_ADDSTRING, 0, (LPARAM)cBuff);
		}
		SDIM(hDlg, IDC_BPMTHREAD, CB_SETCURSEL, 0, 0);
		// fill DrX type CB's
		SDIM(hDlg, IDC_BPM0TYPE, CB_ADDSTRING, 0, (LPARAM)szExec);
		SDIM(hDlg, IDC_BPM0TYPE, CB_ADDSTRING, 0, (LPARAM)szWrite);
		SDIM(hDlg, IDC_BPM0TYPE, CB_ADDSTRING, 0, (LPARAM)szRW);
		SDIM(hDlg, IDC_BPM0TYPE, CB_SETCURSEL, 0, 0);
		SDIM(hDlg, IDC_BPM1TYPE, CB_ADDSTRING, 0, (LPARAM)szExec);
		SDIM(hDlg, IDC_BPM1TYPE, CB_ADDSTRING, 0, (LPARAM)szWrite);
		SDIM(hDlg, IDC_BPM1TYPE, CB_ADDSTRING, 0, (LPARAM)szRW);
		SDIM(hDlg, IDC_BPM1TYPE, CB_SETCURSEL, 0, 0);
		SDIM(hDlg, IDC_BPM2TYPE, CB_ADDSTRING, 0, (LPARAM)szExec);
		SDIM(hDlg, IDC_BPM2TYPE, CB_ADDSTRING, 0, (LPARAM)szWrite);
		SDIM(hDlg, IDC_BPM2TYPE, CB_ADDSTRING, 0, (LPARAM)szRW);
		SDIM(hDlg, IDC_BPM2TYPE, CB_SETCURSEL, 0, 0);
		SDIM(hDlg, IDC_BPM3TYPE, CB_ADDSTRING, 0, (LPARAM)szExec);
		SDIM(hDlg, IDC_BPM3TYPE, CB_ADDSTRING, 0, (LPARAM)szWrite);
		SDIM(hDlg, IDC_BPM3TYPE, CB_ADDSTRING, 0, (LPARAM)szRW);
		SDIM(hDlg, IDC_BPM3TYPE, CB_SETCURSEL, 0, 0);
		// show DrX Info
		if (!ShowDrxInfo(hDlg, ThreadList[0].dwThreadID))
			WriteBPMStatus((char*)szErr);
		else
			WriteBPMStatus((char*)szOk);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_SETBPMS:
			if (!SetThreadBPMs())
				WriteBPMStatus((char*)szErr);
			else
				WriteBPMStatus((char*)szOk);
			break;

		case ID_BPMDLG_OK:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_BPMTHREAD:
			if (HIWORD(wParam) == CBN_SELCHANGE) // another thread was selected
			{
				i = SDIM(hDlg, IDC_BPMTHREAD, CB_GETCURSEL, 0, 0);
				if (!ShowDrxInfo(hDlg, ThreadList[i].dwThreadID))
					WriteBPMStatus((char*)szErr);
				else
					WriteBPMStatus((char*)szOk);
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	}
	return FALSE;
}

BOOL ShowDrxInfo(HWND hDlg, DWORD dwThID)
{
	CONTEXT  Con                 = {CON_DRX};
	char     cBuff[9];
	HANDLE   hTh;

	hTh = GetThreadHandle(dwThID);
	if (!hTh)
		return FALSE;

	if (!GetThreadContext(hTh, &Con))
		return FALSE;

	// BPM0
	wsprintf(cBuff, szH, Con.Dr0);
	SDIT(hDlg, IDC_BPM0ADDR, cBuff);
	CheckDlgButton(
		hDlg,
		IDC_BPM0ON,
		(Con.Dr7 & BPM0_LOCAL_ENABLED) == BPM0_LOCAL_ENABLED ? TRUE : FALSE);
	if ((Con.Dr7 & BPM0_RW) == BPM0_RW)
		SDIM(hDlg, IDC_BPM0TYPE, CB_SETCURSEL, 2, 0);
	else if ((Con.Dr7 & BPM0_W) == BPM0_W)
		SDIM(hDlg, IDC_BPM0TYPE, CB_SETCURSEL, 1, 0);
	else
		SDIM(hDlg, IDC_BPM0TYPE, CB_SETCURSEL, 0, 0);

	// BPM1
	wsprintf(cBuff, szH, Con.Dr1);
	SDIT(hDlg, IDC_BPM1ADDR, cBuff);
	CheckDlgButton(
		hDlg,
		IDC_BPM1ON,
		(Con.Dr7 & BPM1_LOCAL_ENABLED) == BPM1_LOCAL_ENABLED ? TRUE : FALSE);
	if ((Con.Dr7 & BPM1_RW) == BPM1_RW)
		SDIM(hDlg, IDC_BPM1TYPE, CB_SETCURSEL, 2, 0);
	else if ((Con.Dr7 & BPM1_W) == BPM1_W)
		SDIM(hDlg, IDC_BPM1TYPE, CB_SETCURSEL, 1, 0);
	else
		SDIM(hDlg, IDC_BPM1TYPE, CB_SETCURSEL, 0, 0);

	// BPM2
	wsprintf(cBuff, szH, Con.Dr2);
	SDIT(hDlg, IDC_BPM2ADDR, cBuff);
	CheckDlgButton(
		hDlg,
		IDC_BPM2ON,
		(Con.Dr7 & BPM2_LOCAL_ENABLED) == BPM2_LOCAL_ENABLED ? TRUE : FALSE);
	if ((Con.Dr7 & BPM2_RW) == BPM2_RW)
		SDIM(hDlg, IDC_BPM2TYPE, CB_SETCURSEL, 2, 0);
	else if ((Con.Dr7 & BPM2_W) == BPM2_W)
		SDIM(hDlg, IDC_BPM2TYPE, CB_SETCURSEL, 1, 0);
	else
		SDIM(hDlg, IDC_BPM2TYPE, CB_SETCURSEL, 0, 0);

	// BPM3
	wsprintf(cBuff, szH, Con.Dr3);
	SDIT(hDlg, IDC_BPM3ADDR, cBuff);
	CheckDlgButton(
		hDlg,
		IDC_BPM3ON,
		(Con.Dr7 & BPM3_LOCAL_ENABLED) == BPM3_LOCAL_ENABLED ? TRUE : FALSE);
	if ((Con.Dr7 & BPM3_RW) == BPM3_RW)
		SDIM(hDlg, IDC_BPM3TYPE, CB_SETCURSEL, 2, 0);
	else if ((Con.Dr7 & BPM3_W) == BPM3_W)
		SDIM(hDlg, IDC_BPM3TYPE, CB_SETCURSEL, 1, 0);
	else
		SDIM(hDlg, IDC_BPM3TYPE, CB_SETCURSEL, 0, 0);

	return TRUE;
}

/*
BOOL IsThreadSuspended(HANDLE hTh)
{
	DWORD dw;

	dw = SuspendThread(hTh);
	ResumeThread(hTh);
	return (dw != 0);
}
*/

BOOL SetThreadBPMs()
{
	DWORD              i, dwVal, dwType;
	CONTEXT            Con                = {CON_DRX};
	char               cBuff[9];

	// i -> selected thread
	i = SendDlgItemMessage(hBPMDlg, IDC_BPMTHREAD, CB_GETCURSEL, 0, 0);
	if (!GetThreadContext(ThreadList[i].hThread, &Con))
		return FALSE;

	Con.Dr7 |= BPM_LOCAL_EXACT;

	// BPM0
	dwType = SDIM(hBPMDlg, IDC_BPM0TYPE, CB_GETCURSEL, 0, 0);
	GetDlgItemText(hBPMDlg, IDC_BPM0ADDR, cBuff, sizeof(cBuff));
	if (!Hexstr2Int(cBuff, dwVal))
		return FALSE;
	Con.Dr0 = dwVal;
	if (IsDlgButtonChecked(hBPMDlg, IDC_BPM0ON))
		Con.Dr7 = Con.Dr7 | BPM0_LOCAL_ENABLED;
	else
		if ((Con.Dr7 & BPM0_LOCAL_ENABLED) == BPM0_LOCAL_ENABLED)
			Con.Dr7 -= BPM0_LOCAL_ENABLED;
	// BPM0 type
	switch(dwType)
	{
	case 1: // write
		Con.Dr7 += BPM0_W;
		break;
	case 2: // RW
		Con.Dr7 += BPM0_RW;
		break;
	}

	// BPM1
	dwType = SDIM(hBPMDlg, IDC_BPM1TYPE, CB_GETCURSEL, 0, 0);
	GetDlgItemText(hBPMDlg, IDC_BPM1ADDR, cBuff, sizeof(cBuff));
	if (!Hexstr2Int(cBuff, dwVal))
		return FALSE;
	Con.Dr1 = dwVal;
	if (IsDlgButtonChecked(hBPMDlg, IDC_BPM1ON))
		Con.Dr7 = Con.Dr7 | BPM1_LOCAL_ENABLED;
	else
		if ((Con.Dr7 & BPM1_LOCAL_ENABLED) == BPM1_LOCAL_ENABLED)
			Con.Dr7 -= BPM1_LOCAL_ENABLED;

	// type
	switch(dwType)
	{
	case 1: // write
		Con.Dr7 += BPM1_W;
		break;
	case 2: // RW
		Con.Dr7 += BPM1_RW;
		break;
	}

	// BPM2
	dwType = SDIM(hBPMDlg, IDC_BPM2TYPE, CB_GETCURSEL, 0, 0);
	GetDlgItemText(hBPMDlg, IDC_BPM2ADDR, cBuff, sizeof(cBuff));
	if (!Hexstr2Int(cBuff, dwVal))
		return FALSE;
	Con.Dr2 = dwVal;
	if (IsDlgButtonChecked(hBPMDlg, IDC_BPM2ON))
		Con.Dr7 = Con.Dr7 | BPM2_LOCAL_ENABLED;
	else
		if ((Con.Dr7 & BPM2_LOCAL_ENABLED) == BPM2_LOCAL_ENABLED)
			Con.Dr7 -= BPM2_LOCAL_ENABLED;

	// type
	switch(dwType)
	{
	case 1: // write
		Con.Dr7 += BPM2_W;
		break;
	case 2: // RW
		Con.Dr7 += BPM2_RW;
		break;
	}

	// BPM3
	dwType = SDIM(hBPMDlg, IDC_BPM3TYPE, CB_GETCURSEL, 0, 0);
	GetDlgItemText(hBPMDlg, IDC_BPM3ADDR, cBuff, sizeof(cBuff));
	if (!Hexstr2Int(cBuff, dwVal))
		return FALSE;
	Con.Dr3 = dwVal;
	if (IsDlgButtonChecked(hBPMDlg, IDC_BPM3ON))
		Con.Dr7 = Con.Dr7 | BPM3_LOCAL_ENABLED;
	else
		if ((Con.Dr7 & BPM3_LOCAL_ENABLED) == BPM3_LOCAL_ENABLED)
			Con.Dr7 -= BPM3_LOCAL_ENABLED;

	// type
	switch(dwType)
	{
	case 1: // write
		Con.Dr7 += BPM3_W;
		break;
	case 2: // RW
		Con.Dr7 += BPM3_RW;
		break;
	}

	if (!SetThreadContext(ThreadList[i].hThread, &Con))
		return FALSE;

	return TRUE;
}

BOOL HandleBPM(DWORD dwThID)
{
	HANDLE   hTh;
	CONTEXT  Con                = {CON_DRX};
	BOOL     bHandle            = FALSE;
	DWORD    dwEip;
	char     cStatusBuff[50];

	hTh = GetThreadHandle(dwThID);
	if (!hTh)
		return FALSE;

	if (!GetThreadContext(hTh, &Con))
		return FALSE;

	// is EIP == a BPM address ?
	dwEip = Con.Eip;

	// BPM0 ?
	if ((Con.Dr6 & BPM0_DETECTED) == BPM0_DETECTED &&
		(dwEip >= Con.Eip && dwEip <= Con.Eip + MAX_RW_OPCODE) &&
		(Con.Dr7 & BPM0_LOCAL_ENABLED) == BPM0_LOCAL_ENABLED)
	{
		wsprintf(cStatusBuff, "BPM - %08lXh ", Con.Dr0);
		// build info string
		if ((Con.Dr7 & BPM0_RW) == BPM0_RW)
			lstrcat(cStatusBuff, "RW");
		else if ((Con.Dr7 & BPM0_W) == BPM0_W)
			lstrcat(cStatusBuff, "W");
		else // x !
			lstrcat(cStatusBuff, "X");
		lstrcat(cStatusBuff, " (Dr0)");
		lstrcat(cStatusBuff, " reached...");
		bHandle = TRUE;
	}

	// BPM1 ?
	if ((Con.Dr6 & BPM1_DETECTED) == BPM1_DETECTED &&
		(dwEip >= Con.Eip && dwEip <= Con.Eip + MAX_RW_OPCODE) &&
		(Con.Dr7 & BPM1_LOCAL_ENABLED) == BPM1_LOCAL_ENABLED)
	{
		wsprintf(cStatusBuff, "BPM - %08lXh ", Con.Dr1);
		// build info string
		if ((Con.Dr7 & BPM1_RW) == BPM1_RW)
			lstrcat(cStatusBuff, "RW");
		else if ((Con.Dr7 & BPM1_W) == BPM1_W)
			lstrcat(cStatusBuff, "W");
		else // x !
			lstrcat(cStatusBuff, "X");
		lstrcat(cStatusBuff, " (Dr1)");
		lstrcat(cStatusBuff, " reached...");
		bHandle = TRUE;
	}

	// BPM2 ?
	if ((Con.Dr6 & BPM2_DETECTED) == BPM2_DETECTED &&
		(dwEip >= Con.Eip && dwEip <= Con.Eip + MAX_RW_OPCODE) &&
		(Con.Dr7 & BPM2_LOCAL_ENABLED) == BPM2_LOCAL_ENABLED)
	{
		wsprintf(cStatusBuff, "BPM - %08lXh ", Con.Dr2);
		// build info string
		if ((Con.Dr7 & BPM2_RW) == BPM2_RW)
			lstrcat(cStatusBuff, "RW");
		else if ((Con.Dr7 & BPM2_W) == BPM2_W)
			lstrcat(cStatusBuff, "W");
		else // x !
			lstrcat(cStatusBuff, "X");
		lstrcat(cStatusBuff, " (Dr2)");
		lstrcat(cStatusBuff, " reached...");
		bHandle = TRUE;
	}

	// BPM3 ?
	if ((Con.Dr6 & BPM3_DETECTED) == BPM3_DETECTED &&
		(dwEip >= Con.Eip && dwEip <= Con.Eip + MAX_RW_OPCODE) &&
		(Con.Dr7 & BPM3_LOCAL_ENABLED) == BPM3_LOCAL_ENABLED)
	{
		wsprintf(cStatusBuff, "BPM - %08lXh ", Con.Dr3);
		// build info string
		if ((Con.Dr7 & BPM3_RW) == BPM3_RW)
			lstrcat(cStatusBuff, "RW");
		else if ((Con.Dr7 & BPM3_W) == BPM3_W)
			lstrcat(cStatusBuff, "W");
		else // x !
			lstrcat(cStatusBuff, "X");
		lstrcat(cStatusBuff, " (Dr3)");
		lstrcat(cStatusBuff, " reached...");
		bHandle = TRUE;
	}

	if (bHandle)
	{
		SuspendProcess();
		UpdateStatus(cStatusBuff);
		if (Option.bDlgOnBPM)
			CreateBPMDlg(hInst, hDlg_);		// open BPM Menu
		WaitForUser();
	}

	return bHandle;
}