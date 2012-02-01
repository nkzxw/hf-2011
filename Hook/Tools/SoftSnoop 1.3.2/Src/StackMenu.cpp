
#include "StackMenu.h"

// function protos
BOOL     CreateStackDlg(HANDLE hDlgOwner, DWORD dwApiStack);
BOOL     CALLBACK StackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT  CALLBACK StackLVProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT  CALLBACK StackEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL     ShowApiStack(HWND hDlg);
BOOL     ProcessStackMod(DWORD iItemIndex);

// constants
#define WM_STACKLVSELITEM            WM_USER + 0x2000  // wParam = Item index
#define SHOW_STACK_DD_NUM            16

// global variables
DWORD     dwApiStackBase;
WNDPROC   pOrgLVWndProc, pOrgEditWndProc;
HWND      hStackDlg, hNewStackVal, hSaveNewStackVal;


BOOL CreateStackDlg(HANDLE hDlgOwner, DWORD dwApiStack)
{
	dwApiStackBase = dwApiStack;
	DialogBox(
		hInst,
		MAKEINTRESOURCE(IDD_STACKMOD),
		(HWND)hDlgOwner,
		StackDlgProc);
	return TRUE;
}

BOOL CALLBACK StackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD         dwStackVal, i;
	CHAR          cItemText[9];
	LVITEM        lvItem;
	static int    iCurItem = -1;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		// get handles and disable some things
		hStackDlg         = hDlg;
		hNewStackVal      = GetDlgItem(hDlg, IDC_NEWSTACKVALUE);
		hSaveNewStackVal  = GetDlgItem(hDlg, IDC_SAVESTACKCHANGE);
		EnableWindow(
			hNewStackVal,
			FALSE);
		EnableWindow(
			hSaveNewStackVal,
			FALSE);
		SendMessage(hNewStackVal, EM_SETLIMITTEXT, 8, 0);
		ShowApiStack(hDlg);
		return TRUE;

	case WM_STACKLVSELITEM:
		iCurItem = i = wParam;                    // i -> iItem
		// get corresponding stack value out of list
		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.iSubItem   = 1;
		lvItem.pszText    = cItemText;
		lvItem.cchTextMax = sizeof(cItemText);
		SDIM(hDlg, IDC_STACKLIST, LVM_GETITEMTEXT, i, (LPARAM)&lvItem);
		if (Hexstr2Int(cItemText, dwStackVal))
		{
			SetWindowText(hNewStackVal, cItemText);
			EnableWindow(
				hNewStackVal,
				TRUE);
			EnableWindow(
				hSaveNewStackVal,
				TRUE);
		}
		else
		{
			iCurItem = -1;
			SetWindowText(hNewStackVal, "");
			EnableWindow(
				hNewStackVal,
				FALSE);
			EnableWindow(
				hSaveNewStackVal,
				FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_SAVESTACKCHANGE:
			if (!ProcessStackMod(iCurItem))
				MessageBox(
				hStackDlg,
				"Couldn't write to process memory :(",
				"ERROR",
				MB_ICONERROR);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

LRESULT CALLBACK StackLVProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	POINT          p;
	LVHITTESTINFO  lvHitInfo;

	switch(Msg)
	{
	case WM_LBUTTONDOWN:
		// was a Item selected ?
		GetCursorPos(&p);
		ScreenToClient(hWnd, &p);
		lvHitInfo.pt = p;
		if (SendMessage(hWnd, LVM_HITTEST, 0, (LPARAM)&lvHitInfo) != -1)
			SendMessage(hStackDlg, WM_STACKLVSELITEM, lvHitInfo.iItem, 0);		
		break;
	}
	return CallWindowProc(pOrgLVWndProc, hWnd, Msg, wParam, lParam);
}

LRESULT CALLBACK StackEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc(pOrgEditWndProc, hWnd, Msg, wParam, lParam);
}

BOOL ShowApiStack(HWND hDlg)
{
	LVITEM         lvItem;
	LVCOLUMN       lvCol;
	char           cTitle[30];
	char           cLVText[40];
	HWND           hLV;
	DWORD          i, dwVal, dwBytesRead;

	hLV = GetDlgItem(hDlg, IDC_STACKLIST);

	// title -> api stack base
	GetWindowText(hDlg, cTitle, sizeof(cTitle));
	wsprintf(
		(char*)((DWORD)&cTitle + lstrlen(cTitle)),
		"%08lX",
		dwApiStackBase);
	SetWindowText(hDlg, cTitle);

	// create columns
	memset(&lvCol, 0, sizeof(lvCol));
	lvCol.mask      = LVCF_TEXT | LVCF_WIDTH;
	lvCol.cx        = 126;
	lvCol.pszText   = "Item";
	SendMessage(hLV, LVM_INSERTCOLUMN, 0, (LPARAM)&lvCol);
	lvCol.mask      = LVCF_TEXT | LVCF_WIDTH;
	lvCol.cx        = 78;
	lvCol.pszText   = "Value";
	SendMessage(hLV, LVM_INSERTCOLUMN, 1, (LPARAM)&lvCol);

	// hook listview Wnd Proc
	pOrgLVWndProc = (WNDPROC)GetWindowLong(hLV, GWL_WNDPROC);
	SetWindowLong(hLV, GWL_WNDPROC, (DWORD)&StackLVProc);

	// hook editbox Wnd Proc
	pOrgEditWndProc = (WNDPROC)GetWindowLong(hNewStackVal, GWL_WNDPROC);
	SetWindowLong(hNewStackVal, GWL_WNDPROC, (DWORD)&StackEditProc);

	// show stack items
	memset(&lvItem, 0, sizeof(lvItem));
	lvItem.mask     = LVIF_TEXT;
	lvItem.iItem    = 0;
	lvItem.pszText  = cLVText;
	for (i=0; i < SHOW_STACK_DD_NUM; i++)
	{
		lvItem.iSubItem = 0;
		if (i==0)
			wsprintf(cLVText, "ESP+%02lX - Ret address", i * 4);
		else
			wsprintf(cLVText, "ESP+%02lX - Argument %d", i * 4, i);
		SendMessage(hLV, LVM_INSERTITEM, 0, (LPARAM)&lvItem); // add it
		// add stack value
		if (ReadProcessMemory(
			PI.hProcess,
			(void*)(dwApiStackBase + i * 4),
			&dwVal,
			4,
			&dwBytesRead))
			wsprintf(cLVText, "%08lX", dwVal);
		else
			lstrcpy(cLVText, "????????");
		lvItem.iSubItem = 1;
		SendMessage(hLV, LVM_SETITEM, 0, (LPARAM)&lvItem);

		++lvItem.iItem;
	}
	return TRUE;
}

BOOL ProcessStackMod(DWORD iItemIndex)
{
	DWORD    dwStackVal, dwBytesWritten;
	char     cItemText[9];
	LVITEM   lvItem;

	GetWindowText(hNewStackVal, cItemText, sizeof(cItemText));
	Hexstr2Int(cItemText, dwStackVal);
	if (WriteProcessMemory(
		PI.hProcess,
		(void*)(dwApiStackBase + iItemIndex * 4),
		&dwStackVal,
		4,
		&dwBytesWritten))
	{
		memset(&lvItem, 0, sizeof(lvItem));
		++lvItem.iSubItem;
		wsprintf(cItemText, "%08lX", dwStackVal);
		lvItem.pszText    = cItemText;
		SDIM(hStackDlg, IDC_STACKLIST, LVM_SETITEMTEXT, iItemIndex, (LPARAM)&lvItem);
	}
	else
		return FALSE;
	return TRUE;
}