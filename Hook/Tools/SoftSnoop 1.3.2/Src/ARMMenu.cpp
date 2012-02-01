
#include "ARMMenu.h"

// protos
void     CreateMARDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL     CALLBACK ARMDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT  CALLBACK RetValEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
void     MarSetCols(HWND hDlg, HWND hLV);
void     MarInfoToList(HWND hDlg, HWND hLV);
void     MarSetList(HWND hDlg, HWND hLV);
BOOL     MarSaveList(HWND hDlg, HWND hLV);
BOOL     HandleRetValMod(char* szApi, void* pEax, DWORD *dwNewRetVal);

// constants
#define LV_APINAME_WIDTH  132
#define LV_DD_WIDTH       78
#define MAX_STR           200
#define BUFF_SIZE_STEP    300
#define MAX_MAR_ITEMS     20

const char*       szErr_                           = "ERROR";

// global variables
WNDPROC        pOrgRetValEditProc;
BOOL           bMAREnabled                         = TRUE;

char           cMARBuff[MAX_MAR_ITEMS * 30]        = {0};
DWORD          dwMARVals[MAX_MAR_ITEMS];

void CreateMARDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	DialogBox(hInst,MAKEINTRESOURCE(IDD_APIRETMOD),(HWND)hDlgOwner,ARMDlgProc);
	return;
}

BOOL CALLBACK ARMDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT i;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		// init dlg
		DefApiToCombo(hDlg, IDC_MAR_API);
		MarSetCols(hDlg, GetDlgItem(hDlg, IDC_MARLIST));
		SendDlgItemMessage(hDlg, IDC_NEWRETURNVALUE, EM_SETLIMITTEXT, 8, 0);
		pOrgRetValEditProc = (WNDPROC)SetWindowLong(
			GetDlgItem(hDlg, IDC_NEWRETURNVALUE),
			GWL_WNDPROC,
			(LONG)RetValEditProc);
		MarSetList(
				hDlg,
				GetDlgItem(hDlg, IDC_MARLIST));
		SetFocus(GetDlgItem(hDlg, IDC_MAR_API));
		CheckDlgButton(hDlg, IDC_MARON, bMAREnabled);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_MAR_OK:
			if (MarSaveList(
				hDlg,
				GetDlgItem(hDlg, IDC_MARLIST)))
			{   
				// quit dlg
				bMAREnabled = IsDlgButtonChecked(hDlg, IDC_MARON);
				EndDialog(hDlg, TRUE);
			}
			return TRUE;

		case ID_MAR_CANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;

		case IDC_ADDMAR:
			if (!GetWindowTextLength(GetDlgItem(hDlg, IDC_MAR_API)) ||
				!GetWindowTextLength(GetDlgItem(hDlg, IDC_NEWRETURNVALUE)))
				MessageBeep(MB_ICONERROR);
			else
			{
				MarInfoToList(hDlg, GetDlgItem(hDlg, IDC_MARLIST));
				SetDlgItemText(hDlg, IDC_MAR_API, NULL);
			}
			return TRUE;

		case IDC_DELETEMAR:
			i = SendMessage(GetDlgItem(hDlg, IDC_MARLIST), LVM_GETSELECTIONMARK, 0, 0);
			if (i != -1)
				SendDlgItemMessage(hDlg, IDC_MARLIST, LVM_DELETEITEM, i, 0);
			else
				MessageBeep(MB_ICONERROR);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

LRESULT CALLBACK RetValEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc(pOrgRetValEditProc, hWnd, Msg, wParam, lParam);
}

void MarSetCols(HWND hDlg, HWND hLV)
{
	LV_COLUMN Col;

	// create columns
	Col.mask     = LVCF_TEXT | LVCF_WIDTH;
	Col.pszText  = "API";
	Col.cx       = LV_APINAME_WIDTH;
	SendMessage(hLV, LVM_INSERTCOLUMN, 0, (LPARAM)&Col);
	Col.pszText  = "RET-value";
	Col.cx       = LV_DD_WIDTH;
	SendMessage(hLV, LVM_INSERTCOLUMN, 1, (LPARAM)&Col);

	return;
}

void MarInfoToList(HWND hDlg, HWND hLV)
{
	LV_ITEM Item;
	char    cBuff[MAX_STR];
	DWORD   dwVal;

	// add API string
	GetDlgItemText(hDlg, IDC_MAR_API, cBuff, sizeof(cBuff));
	Item.mask      = LVIF_TEXT;
	Item.iSubItem  = 0;
	Item.iItem     = SendMessage(hLV, LVM_GETITEMCOUNT, 0, 0);;
	Item.pszText   = cBuff;
	SendMessage(hLV, LVM_INSERTITEM, 0, (LPARAM)&Item);

	// add return value
	++Item.iSubItem;
	GetDlgItemText(hDlg, IDC_NEWRETURNVALUE, cBuff, sizeof(cBuff));
	Hexstr2Int(cBuff, dwVal);
	wsprintf(cBuff, "%08lX", dwVal);
	SendMessage(hLV, LVM_SETITEM, 0, (LPARAM)&Item);

	return;
}

void MarSetList(HWND hDlg, HWND hLV)
{
	UINT     i = 0;
	char     *pCH, cHex[9];
	LV_ITEM  lvItem;

	// paste info in LV
	pCH = cMARBuff;
	lvItem.iItem = 0;
	lvItem.mask  = LVIF_TEXT;
	while (*pCH)
	{
		// API name
		lvItem.pszText  = pCH;
		lvItem.iSubItem = 0;
		SendMessage(hLV, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

		// ret val
		lvItem.pszText  = cHex;
		lvItem.iSubItem = 1;
		wsprintf(cHex, "%08lX", dwMARVals[i]);
		SendMessage(hLV, LVM_SETITEM, 0, (LPARAM)&lvItem);

		pCH += lstrlen(pCH) + 1;
		i++;
		lvItem.iItem++;
	}

	return;
}

BOOL MarSaveList(HWND hDlg, HWND hLV)
{
	UINT     i, iItems, iBuffSize;
	char     cBuff[MAX_STR], *pCH;
	LV_ITEM  lvItem;

	cMARBuff[0] = 0;

	// check item num
	iItems = SendMessage(hLV, LVM_GETITEMCOUNT, 0, 0);
	if (iItems == -1)
		return TRUE;

	if (iItems > MAX_MAR_ITEMS)
	{
		wsprintf(cBuff, "A maximum of %d items are allowed !", MAX_MAR_ITEMS);
		MessageBox(hDlg, cBuff, szErr_, MB_ICONERROR);
		return FALSE;
	}

	// check if buffer won't exceed
	iBuffSize = 1;
	lvItem.pszText    = cBuff;
	lvItem.cchTextMax = sizeof(cBuff);
	lvItem.iSubItem   = 0;
	for (i=0; i < iItems; i++)
	{
		SendMessage(hLV, LVM_GETITEMTEXT, i, (LPARAM)&lvItem);
		iBuffSize += lstrlen(lvItem.pszText) + 1;
	}
	if (iBuffSize > sizeof(cMARBuff))
	{
		MessageBox(hDlg, "Buffer size exceeded !", szErr_, MB_ICONERROR);
		return FALSE;
	}

	// save the infos
	lvItem.pszText    = cBuff;
	lvItem.cchTextMax = sizeof(cBuff);
	pCH = cMARBuff;
	for (i=0; i < iItems; i++)
	{
		// save API name
		lvItem.iSubItem = 0;
		SendMessage(hLV, LVM_GETITEMTEXT, i, (LPARAM)&lvItem);
		lstrcpy(pCH, lvItem.pszText);
		pCH += lstrlen(lvItem.pszText) + 1;
		*pCH = 0; // NUL - terminate the shit
		// save ret-value
		lvItem.iSubItem = 1;
		SendMessage(hLV, LVM_GETITEMTEXT, i, (LPARAM)&lvItem);
		Hexstr2Int(lvItem.pszText, dwMARVals[i]);
	}

	return TRUE;
}

// return true if the return value was modified
BOOL HandleRetValMod(char* szApi, void* pEax, DWORD *dwNewRetVal)
{
	char   *pCH;
	UINT   i       = 0;
	DWORD  dwBuff;

	if (!bMAREnabled)
		return FALSE;

	pCH = cMARBuff;
	while (*pCH)
	{
		if (lstrcmpi(szApi, pCH) == 0)
		{
			*dwNewRetVal = dwMARVals[i];
			return WriteProcessMemory(
				PI.hProcess,
				pEax,
				&dwMARVals[i],
				4,
				&dwBuff);
		}

		// next one
		pCH += lstrlen(pCH) + 1;
		i++;
	}

	return FALSE;
}