
#include "ModuleMenu.h"

BOOL         IsShowModuleList;
WNDPROC      pOrgModuleListWndProc;
HWND         hModuleDlg;

// function protos
BOOL CreateModuleDlg(HINSTANCE hInst,HANDLE hDlgOwner, BOOL bShowModuleList);
BOOL CALLBACK ModuleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CreateModuleDlg(HINSTANCE hInst,HANDLE hDlgOwner, BOOL bShowModuleList)
{
	IsShowModuleList = bShowModuleList;
	return DialogBoxParam(
		hInst,
		MAKEINTRESOURCE(IDD_MODULES),
		(HWND)hDlgOwner,
		ModuleDlgProc,
		0);
}

LRESULT CALLBACK ModuleListProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	LVHITTESTINFO   info;
	LVITEM          TheItem;
	char            cID[20];

	switch(Msg)
	{
	case WM_LBUTTONDBLCLK:
		// force hex characters
		info.pt.x = LOWORD(lParam);
		info.pt.y = HIWORD(lParam);
		SendMessage(
			hWnd,
			LVM_HITTEST,
			0,
			(LPARAM)&info);
		if (info.flags==LVHT_ONITEMLABEL)
		{
			memset(&TheItem, 0, sizeof(TheItem));
			TheItem.iSubItem = 0;
			TheItem.pszText = cID;
			TheItem.cchTextMax = 20;
			if (SendMessage(hWnd, LVM_GETITEMTEXT, info.iItem, (LPARAM)&TheItem)>0)
			{
				if (!Str2Int(TheItem.pszText, dwInjectedProcessID))
					dwInjectedProcessID = 0;
				else
					SendMessage(hModuleDlg, WM_CLOSE, 0, 0); 
			}
		}
		break;
	}
	return CallWindowProc(pOrgModuleListWndProc, hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK ModuleDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		hModuleDlg = hDlg;
		if (IsShowModuleList)
		{
			if (!ShowProcessModules(hDlg))
			{
				ShowError("Couldn't get module information :(");
				SendMessage(hDlg, WM_CLOSE, 0, 0);
			}
		} else
		{
			if (!ShowProcessList(hDlg))
			{
				ShowError("Couldn't get process list :(");
				SendMessage(hDlg, WM_CLOSE, 0, 0);
			}
	
			// hook process list
			pOrgModuleListWndProc = (WNDPROC)SetWindowLong(
				GetDlgItem(hDlg, IDC_MODULELIST),
				GWL_WNDPROC,
				(LONG)ModuleListProc);

		}

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_MODULELIST:

				return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

