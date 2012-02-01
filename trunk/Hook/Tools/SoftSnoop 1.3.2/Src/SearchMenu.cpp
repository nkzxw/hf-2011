
#include <string.h>
#include "SearchMenu.h"
#include "SoftSnoop.h"
#include "resource.h"

// functions
VOID  CreateSearchDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL  CALLBACK SearchDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL  DoListSearch(HWND hTarLB);

// variables
sSearchOptions  SearchOptions;


VOID CreateSearchDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	WNDCLASS wc;

	// create a dlg;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_Search";
	RegisterClass(&wc);

	DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_SEARCH),(HWND)hDlgOwner,SearchDlgProc,NULL);
	return;
}

BOOL CALLBACK SearchDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		// process button checks
		if (SearchOptions.SearchInWholeList)
			CheckDlgButton(hDlg,IDC_SEARCHWHOLELIST,TRUE);
		else
			CheckDlgButton(hDlg,IDC_SEARCHCURPOS,TRUE);
		if (SearchOptions.CaseSensitive)
			CheckDlgButton(hDlg,IDC_CASESENSITIVE,TRUE);
		SetDlgItemText(hDlg,IDC_SEARCHSTRING,SearchOptions.SearchBuff);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_DOSEARCH:
			// update the search options struct
			GetDlgItemText(
				hDlg,
				IDC_SEARCHSTRING,
				SearchOptions.SearchBuff,
				sizeof(SearchOptions.SearchBuff));
			SearchOptions.CaseSensitive = IsDlgButtonChecked(hDlg,IDC_CASESENSITIVE);
			SearchOptions.SearchInWholeList = IsDlgButtonChecked(hDlg,IDC_SEARCHWHOLELIST);
			// start the search
			if (DoListSearch(hLBDbg))
				SendMessage(hDlg,WM_CLOSE,0,0);
			else
				MessageBox(hDlg,"Text not found !","Search Result",MB_ICONERROR);
			return TRUE;

		case ID_CANCELSEARCH:
			EndDialog(hDlg,0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg,0);
		return TRUE;
	}

	return FALSE;
}

BOOL DoListSearch(HWND hTarLB)
{
	int     i,iItemNum,iStartIndex;
	CHAR    cLBItemBuff[300],cToFind[MAX_SEARCHSTRINGLENGTH],*pCH;

	strcpy(cToFind,SearchOptions.SearchBuff);
	iItemNum = SendMessage(hTarLB,LB_GETCOUNT,0,0);
	
	if (SearchOptions.SearchInWholeList)
		iStartIndex = 0;
	else
	{
		iStartIndex = SendMessage(hTarLB,LB_GETCURSEL,0,0);
		if (iStartIndex < 0)
			iStartIndex = 0;
		else
			++iStartIndex;
	}

	for (i=iStartIndex; i < iItemNum; i++)
	{
		SendMessage(hTarLB,LB_GETTEXT,i,(LPARAM)cLBItemBuff);
		if (!SearchOptions.CaseSensitive)
		{
			_strupr(cToFind);
			_strupr(cLBItemBuff);
		}
		pCH = strstr(cLBItemBuff,cToFind);
		if (pCH)
		{
			// the string was found
			SendMessage(hTarLB,LB_SETCURSEL,i,0);
			return TRUE;
		}
	}

	return FALSE;
}
