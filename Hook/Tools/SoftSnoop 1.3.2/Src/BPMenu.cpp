
#include <windows.h>
#include "resource.h"
#include "BPMenu.h"
#include "SoftSnoop.h"
#include "OptionMenu.h"

// the functions
int     CreateBPDlg(HINSTANCE hInst,HANDLE hDlgOwner);
LRESULT  CALLBACK VAEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL     CALLBACK BPDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL     SaveBPXList();

#define MAX_TRAP_API_NAME_LENGTH           MAX_APILISTITEMLENGTH * 2

// variables
CHAR    cEditBuff[MAX_TRAP_API_NAME_LENGTH],cBuff[40];
DWORD   i,dwListCnt;
HWND    hBPDlg;
WNDPROC pOrgVaEditWndProc;

int CreateBPDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	// create a dlg;
	WNDCLASS wc;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_BPMenu";
	RegisterClass(&wc);
	return DialogBox(hInst,MAKEINTRESOURCE(IDD_BP),(HWND)hDlgOwner,BPDialogFunct);
}

LRESULT CALLBACK VAEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc(pOrgVaEditWndProc, hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK BPDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD  dwSizeCheck,dwStrLen,dwBPX;
	CHAR*  pCH;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hBPDlg = hDlg;
		// add the trapped api names to the listbox
		pCH = (PSTR)TrapApiNameBuff;
		while (*pCH != 0)
		{
			SendDlgItemMessage(hDlg,IDC_TRAPAPILIST,LB_ADDSTRING,NULL,(LPARAM)pCH);
			pCH += lstrlen(pCH);
			pCH++;
		}
		// add the breakpoint VA's to its listbox
		for(i=0; (INT)i<iBPXCnt; i++)
		{
			wsprintf(cBuff,"%X",(DWORD)BPXInfo[i].pVA);
			SendDlgItemMessage(hDlg,IDC_BPXLIST,LB_ADDSTRING,NULL,(LPARAM)(PSTR)cBuff);
		}
		// set text limits
		SendDlgItemMessage(hDlg,IDC_EDITTRAPAPI,EM_SETLIMITTEXT,MAX_TRAP_API_NAME_LENGTH-1,0);
		SendDlgItemMessage(hDlg,IDC_EDITBPX,EM_SETLIMITTEXT,8,0);

		if (Option.bRestoreBP)
			CheckDlgButton(hDlg, IDC_RESTOREBP, TRUE);

		// hook VA edit box
		pOrgVaEditWndProc = (WNDPROC)SetWindowLong(
			GetDlgItem(hDlg, IDC_EDITBPX),
			GWL_WNDPROC,
			(LONG)VAEditProc);

		SetDlgItemText(hDlg,IDC_EDITTRAPAPI,"ReadProcessMemory");
		SetDlgItemText(hDlg,IDC_EDITTRAPDLL,"Kernel32.dll");

		// insert default Apis
		DefApiToCombo(hDlg, IDC_EDITTRAPAPI);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_ADDTRAPAPI:
			if (GetDlgItemText(
				hDlg,
				IDC_EDITTRAPAPI,
				(PSTR)cEditBuff,
				sizeof(cEditBuff)) == 0)
			{
				MessageBeep(MB_ICONERROR);
				return TRUE;
			}
			pCH = cEditBuff;
			if (pCH[0]>='1' && pCH[0]<='9')
			{
				if (!Str2Int(pCH, dwBPX))
				{
					MessageBox(
						hDlg,
						"Invalid api export number !",
						"ERROR",
						MB_ICONERROR);
					return TRUE;
				}
			}
			wsprintf(cEditBuff, "%s@", cEditBuff);
			pCH = cEditBuff+strlen(cEditBuff);
			if (GetDlgItemText(
				hDlg,
				IDC_EDITTRAPDLL,
				(PSTR)(pCH),
				sizeof(cEditBuff)-strlen(cEditBuff)-1) == 0)
			{
				MessageBeep(MB_ICONERROR);
				return TRUE;
			}
			SendDlgItemMessage(hDlg,IDC_TRAPAPILIST,LB_ADDSTRING,NULL,(LPARAM)cEditBuff);
			SetDlgItemText(hDlg,IDC_EDITTRAPAPI,"");
			break;

		case IDC_DELETETRAPAPI:
			i = SendDlgItemMessage(hDlg,IDC_TRAPAPILIST,LB_GETCURSEL,NULL,NULL);
			if ((int)i < 0)
			{
				MessageBeep(MB_ICONERROR);
				return TRUE;
			}
			SendDlgItemMessage(hDlg,IDC_TRAPAPILIST,LB_DELETESTRING,i,NULL);
			return TRUE;

		case IDC_ADDBPX:
			if (GetDlgItemText(
				hDlg,
				IDC_EDITBPX,
				(PSTR)cEditBuff,
				sizeof(cEditBuff)) == 0)
			{
				MessageBeep(MB_ICONERROR);
				return TRUE;
			}
			if (!Hexstr2Int((PSTR)cEditBuff,dwBPX))
			{
				MessageBox(
					hDlg,
					"Invalid hex number !",
					"ERROR",
					MB_ICONERROR);
				return TRUE;
			}
			SendDlgItemMessage(hDlg,IDC_BPXLIST,LB_ADDSTRING,NULL,(LPARAM)cEditBuff);
			SetDlgItemText(hDlg,IDC_EDITBPX,"");
			return TRUE;

		case IDC_DELETEBPX:
			i = SendDlgItemMessage(hDlg,IDC_BPXLIST,LB_GETCURSEL,NULL,NULL);
			if ((int)i < 0)
			{
				MessageBeep(MB_ICONERROR);
				return TRUE;
			}
			SendDlgItemMessage(hDlg,IDC_BPXLIST,LB_DELETESTRING,i,NULL);
			return TRUE;

		case ID_SETBPS:
			if (!bDebugging)
			{
				MessageBox(hDlg,"This is only possible while debugging !","ERROR",\
					MB_ICONERROR);
				return TRUE;
			}
			if (!SaveBPXList())
				return TRUE;
			if (ProcessBPXList())
				MessageBox(hDlg,"Done",":)",MB_ICONINFORMATION);
			return TRUE;

		case ID_BPDLG_OK:
			// - save the api names -
			dwSizeCheck = 0;
			pCH = (CHAR*)TrapApiNameBuff;
			dwListCnt = SendDlgItemMessage(hDlg,IDC_TRAPAPILIST,LB_GETCOUNT,NULL,NULL);
			if (dwListCnt != 0)
			{
				for(i=0; i<dwListCnt; i++)
				{
					dwStrLen = SendDlgItemMessage(
						hDlg,
						IDC_TRAPAPILIST,
						LB_GETTEXT,
						i,
						(LPARAM)(PSTR)cEditBuff);
					dwSizeCheck += ++dwStrLen;
					// to many names ?
					if (dwSizeCheck+1 > TRAPAPIBUFFSIZE)
					{
						wsprintf(
							cBuff,
							"The API names are limited to %i characters !",
							TRAPAPIBUFFSIZE);
						MessageBox(hDlg,cBuff,"ERROR",MB_ICONERROR);
						TrapApiNameBuff[0] = 0;
						return TRUE;
					}
					// copy the name into the buffer
					strcpy(pCH,(PSTR)cEditBuff);
					pCH += dwStrLen;
				}
			}
			// NULL terminate the buffer
			*pCH = 0;

			if (!SaveBPXList())
				return TRUE;

			// restore BP ?
			Option.bRestoreBP = IsDlgButtonChecked(hDlg, IDC_RESTOREBP);

			// quit
			EndDialog(hDlg,1);
			return TRUE;

		case ID_BPDLG_CANCEL:
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

BOOL SaveBPXList()
{
	DWORD dwVA;

	// - save the breakpoint VA's -
	dwListCnt = SendDlgItemMessage(hBPDlg,IDC_BPXLIST,LB_GETCOUNT,NULL,NULL);
	// are there to many ?
	if (dwListCnt > MAX_INT3BPS)
	{
		wsprintf(
			cBuff,
			"The number of Int3-breakpoints is limited to %i !",
			MAX_INT3BPS);
		MessageBox(hBPDlg,cBuff,"ERROR",MB_ICONERROR);
		return FALSE;
	}
	// process the list
	iBPXCnt = dwListCnt;
	for(i=0; i<dwListCnt; i++)
	{
		SendDlgItemMessage(hBPDlg,IDC_BPXLIST,LB_GETTEXT,i,(LPARAM)(PSTR)cBuff);
		Hexstr2Int(cBuff,dwVA);
		BPXInfo[i].pVA = (VOID*)dwVA;
	}			
	return TRUE;
}
