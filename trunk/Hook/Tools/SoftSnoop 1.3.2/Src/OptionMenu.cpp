
#include <windows.h>
#include "resource.h"
#include "OptionMenu.h"
#include "SoftSnoop.h"
#include "stdio.h"

// the functions
VOID CreateOptionDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL CALLBACK OptionDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL SetSSShellExtension(char* szSSExePath);
BOOL KillSSShellExtension();

// constants
const char*  szSSShellEntry = "Debug with SoftSnoop";

// variables
HINSTANCE    hOptInst;
HANDLE       hOptDlg,
             hAddApiB,hDeleteApiB,hEditApiDll,hEditApi,hAPIList,
             hAddRegionB,hDeleteRegionB,hEditRegionBegin,hEditRegionEnd,hRegionList,
             hAddToDllB,hDeleteToDllB,hToDllList,hEditToDll,
             hAddFromDllB,hDeleteFromDllB,hFromDllList,hEditFromDll,
			 hEditCmdLine;
HWND         hOutPutFileName;
char         cEditBoxBuff[MAX_PATH],MsgBuff[50];
WORD         i;
WNDPROC      pOrgRegionEditWndProc;


VOID CreateOptionDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	hOptInst = hInst;

	// create a dlg;
	WNDCLASS wc;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_Options";
	RegisterClass(&wc);
	DialogBox(hInst,MAKEINTRESOURCE(IDD_OPTIONS),(HWND)hDlgOwner,OptionDialogFunct);
	return;
}

LRESULT CALLBACK RegionEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc(pOrgRegionEditWndProc, hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK OptionDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int   iSelIndex;
	char  cBuff[MAX_PATH];
	BOOL  bButtonState, bButtonState2;
	DWORD dwRegionBegin, dwRegionEnd;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_OUTPUTFILENAME,EM_SETLIMITTEXT,
				MAX_OUTFILENAME_LENGTH-1,0);

			// get some window handles
			hOptDlg = hDlg;

			hDeleteApiB    = GetDlgItem(hDlg,IDC_DELETEAPI);
			hAddApiB       = GetDlgItem(hDlg,IDC_ADDAPI);
			hEditApi      = GetDlgItem(hDlg,IDC_EDITAPI);
			hEditApiDll      = GetDlgItem(hDlg,IDC_EDITAPIDLL);
			hAPIList         = GetDlgItem(hDlg,IDC_APILIST);

			hDeleteRegionB    = GetDlgItem(hDlg,IDC_DELETEREGION);
			hAddRegionB       = GetDlgItem(hDlg,IDC_ADDREGION);
			hEditRegionBegin      = GetDlgItem(hDlg,IDC_EDITREGIONBEGIN);
			hEditRegionEnd      = GetDlgItem(hDlg,IDC_EDITREGIONEND);
			hRegionList         = GetDlgItem(hDlg,IDC_REGIONLIST);

			hAddToDllB         = GetDlgItem(hDlg,IDC_ADDTODLL);
			hDeleteToDllB      = GetDlgItem(hDlg,IDC_DELETETODLL);
			hToDllList         = GetDlgItem(hDlg,IDC_TODLLLIST);
			hEditToDll         = GetDlgItem(hDlg,IDC_EDITTODLL);
			
			hAddFromDllB         = GetDlgItem(hDlg,IDC_ADDFROMDLL);
			hDeleteFromDllB      = GetDlgItem(hDlg,IDC_DELETEFROMDLL);
			hFromDllList         = GetDlgItem(hDlg,IDC_FROMDLLLIST);
			hEditFromDll         = GetDlgItem(hDlg,IDC_EDITFROMDLL);
			
			hOutPutFileName  = GetDlgItem(hDlg,IDC_OUTPUTFILENAME);
			hEditCmdLine     = GetDlgItem(hDlg,IDC_CMDLINESTR);


			// process the options
			if (Option.wIgnoreAPINum != 0)
			{
				for (i=0; i<Option.wIgnoreAPINum; i++)
					SendDlgItemMessage(hDlg,IDC_APILIST,LB_ADDSTRING,NULL, \
					(LPARAM)Option.cIgnoreAPIs[i]);
			}
			if (Option.wIgnoreToDllNum != 0)
			{
				for (i=0; i<Option.wIgnoreToDllNum; i++)
					SendDlgItemMessage(hDlg,IDC_TODLLLIST,LB_ADDSTRING,NULL, \
					(LPARAM)Option.cIgnoreToDlls[i]);
			}
			if (Option.wIgnoreFromDllNum != 0)
			{
				for (i=0; i<Option.wIgnoreFromDllNum; i++)
					SendDlgItemMessage(hDlg,IDC_FROMDLLLIST,LB_ADDSTRING,NULL, \
					(LPARAM)Option.cIgnoreFromDlls[i]);
			}
			if (Option.wIgnoreRegionNum != 0)
			{
				for (i=0; i<Option.wIgnoreRegionNum; i++)
				{
					wsprintf(cBuff, "0x%08X ~ 0x%08X", Option.dwIgnoreRegionBegin[i], Option.dwIgnoreRegionEnd[i]);
					SendDlgItemMessage(hDlg,IDC_REGIONLIST,LB_ADDSTRING,NULL, (LPARAM)cBuff);
				}
			}

			// special API list
			if (!Option.dwIgnoreAPIs != NO_SPECIAL_APIS)
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_IGNOREAPI,0),NULL);
			else
			{
				switch(Option.dwIgnoreAPIs)
				{
				case DONT_CALL_SPECIAL_APIS:
					CheckDlgButton(hDlg,IDC_IGNOREAPI,BST_CHECKED);
					break;

				case JUST_CALL_SPECIAL_APIS:
					CheckDlgButton(hDlg, IDC_ONLYAPI, BST_CHECKED);
					break;
				}
			}

			// special dll list
			if (Option.dwIgnoreToDlls == NO_SPECIAL_DLLS)
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_IGNORETODLL,0),NULL);
			else if (Option.dwIgnoreToDlls == DONT_SPECIAL_DLL_CALLS)
				CheckDlgButton(hDlg,IDC_IGNORETODLL,BST_CHECKED);
			else
				CheckDlgButton(hDlg, IDC_JUSTTODLL, BST_CHECKED);

			if (Option.dwIgnoreFromDlls == NO_SPECIAL_DLLS)
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_IGNOREFROMDLL,0),NULL);
			else if (Option.dwIgnoreFromDlls == DONT_SPECIAL_DLL_CALLS)
				CheckDlgButton(hDlg,IDC_IGNOREFROMDLL,BST_CHECKED);
			else
				CheckDlgButton(hDlg, IDC_JUSTFROMDLL, BST_CHECKED);

			if (Option.dwIgnoreRegions == NO_SPECIAL_DLLS)
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_IGNOREREGION,0),NULL);
			else if (Option.dwIgnoreRegions == DONT_SPECIAL_DLL_CALLS)
				CheckDlgButton(hDlg,IDC_IGNOREREGION,BST_CHECKED);
			else
				CheckDlgButton(hDlg, IDC_JUSTREGION, BST_CHECKED);
			CheckDlgButton(hDlg,
				IDC_DEBUGPROCESS,
				Option.bDebugProcess ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(
				hDlg,
				IDC_APIPARAMS,
				Option.bShowApiParams ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg,
				IDC_SHOWAPICALLS,
				Option.bShowAPICall ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(
				hDlg,
				IDC_SHOWAPIRET,
				Option.bShowApiReturn ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton(hDlg,
				IDC_MOVEWIN,
				Option.bMoveWindow ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg,
				IDC_SHOWAPI,
				Option.bReportNameAPI ? BST_CHECKED : BST_UNCHECKED);			
			CheckDlgButton(hDlg,
				IDC_SHOWAPIORDINAL,
				Option.bReportOrdinalAPI ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg,
				IDC_SHOWDEBUG,
				Option.bShowDebugEvents ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg,
				IDC_HANDLEEXCEPT,
				Option.bHandleExceptions ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(
				hDlg,
				IDC_DEBUGBREAK,
				Option.bStopAtDB ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(
				hDlg,
				IDC_GUIOUTPUT,
				Option.bGUIOutPut ? BST_CHECKED : BST_UNCHECKED);
			if(Option.bAdditionalCmdLine)
				CheckDlgButton(hDlg,IDC_CMDLINE,TRUE);
			else
				EnableWindow((HWND)hEditCmdLine,FALSE);
			if (Option.bFileOutPut)
				CheckDlgButton(hDlg,IDC_FILEOUTPUT,TRUE);
			else
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_FILEOUTPUT,0),0);
			SetDlgItemText(hDlg,IDC_CMDLINESTR,Option.cCmdLine);
			SetDlgItemText(hDlg,IDC_OUTPUTFILENAME,Option.cOutFile);

			CheckDlgButton(hDlg, IDC_STOPATENTRY, Option.bStopAtEntry);
			CheckDlgButton(hDlg, IDC_DLGONBPM, Option.bDlgOnBPM);
			CheckDlgButton(hDlg, IDC_SHELLEXTENSION, Option.bShellExtension);
			CheckDlgButton(hDlg, IDC_SHOWTIDS, Option.bShowTIDs);

			// set edit length limits
			SendDlgItemMessage(hDlg,IDC_EDITAPI,EM_SETLIMITTEXT, MAX_APILISTITEMLENGTH-1,0);
			SendDlgItemMessage(hDlg,IDC_EDITTODLL,EM_SETLIMITTEXT, MAX_DLLNAMELENGTHOPT-1,0);
			SendDlgItemMessage(hDlg,IDC_EDITFROMDLL,EM_SETLIMITTEXT, MAX_DLLNAMELENGTHOPT-1,0);
			SendDlgItemMessage(hDlg,IDC_EDITAPIDLL,EM_SETLIMITTEXT, MAX_DLLNAMELENGTHOPT-1,0);
			SendDlgItemMessage(hDlg,IDC_EDITREGIONBEGIN,EM_SETLIMITTEXT, MAX_REGIONADDRLENGTH,0);
			SendDlgItemMessage(hDlg,IDC_EDITREGIONEND,EM_SETLIMITTEXT, MAX_REGIONADDRLENGTH,0);
			SendDlgItemMessage(hDlg,IDC_CMDLINESTR,EM_SETLIMITTEXT,	sizeof(Option.cCmdLine)-1,0);

			// hook region edit box
			pOrgRegionEditWndProc = (WNDPROC)SetWindowLong(
				GetDlgItem(hDlg, IDC_EDITREGIONBEGIN),
				GWL_WNDPROC,
				(LONG)RegionEditProc);
			pOrgRegionEditWndProc = (WNDPROC)SetWindowLong(
				GetDlgItem(hDlg, IDC_EDITREGIONEND),
				GWL_WNDPROC,
				(LONG)RegionEditProc);

			SDIM(hDlg, IDC_EDITAPIDLL, CB_RESETCONTENT, 0, 0);
			SDIM(hDlg, IDC_EDITTODLL, CB_RESETCONTENT, 0, 0);
			SDIM(hDlg, IDC_EDITFROMDLL, CB_RESETCONTENT, 0, 0);
			SDIM(hDlg, IDC_EDITAPI, CB_RESETCONTENT, 0, 0);

			// set default Modules
			ModuleListToCombo(hDlg, IDC_EDITAPIDLL);
			ModuleListToCombo(hDlg, IDC_EDITTODLL);
			ModuleListToCombo(hDlg, IDC_EDITFROMDLL);

			SetFocus(NULL);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_OK:
					// - process the Api to ignore
					Option.wIgnoreAPINum = (WORD)SendDlgItemMessage(hDlg,IDC_APILIST,LB_GETCOUNT, \
						NULL,NULL);
					// are there too many APIs ???
					if (Option.wIgnoreAPINum > MAX_IGNOREAPINUM)
					{
						wsprintf(MsgBuff,"You can ignore a maximum of %i API's !", \
							MAX_IGNOREAPINUM);
						MessageBox(hDlg,MsgBuff,"Error",MB_ICONERROR);
						break;
					}
					// save the Api names
					for (i=0; i<Option.wIgnoreAPINum; i++)
						SendDlgItemMessage(hDlg,IDC_APILIST,LB_GETTEXT,i, \
						(LPARAM)Option.cIgnoreAPIs[i]);
					Option.wIgnoreAPINum = (WORD)SendDlgItemMessage(
						hDlg,
						IDC_APILIST,
						LB_GETCOUNT,
						NULL,
						NULL);

					// - process the To Dlls to ignore
					Option.wIgnoreToDllNum = (WORD)SendDlgItemMessage(
						hDlg,
						IDC_TODLLLIST,
						LB_GETCOUNT,
						NULL,
						NULL);
					// are there too many Dlls ???
					if (Option.wIgnoreToDllNum > MAX_IGNOREDLLNUM)
					{
						wsprintf(MsgBuff,"You can add a maximum of %i Dll's here !", \
							MAX_IGNOREDLLNUM);
						MessageBox(hDlg,MsgBuff,"Error",MB_ICONERROR);
						break;
					}
					// save the Dll names
					for (i=0; i<Option.wIgnoreToDllNum; i++)
						SendDlgItemMessage(hDlg,IDC_TODLLLIST,LB_GETTEXT,i, \
						(LPARAM)Option.cIgnoreToDlls[i]);
					Option.dwIgnoreToDlls = NO_SPECIAL_DLLS;
					if (IsDlgButtonChecked(hDlg,IDC_IGNORETODLL))
						Option.dwIgnoreToDlls = DONT_SPECIAL_DLL_CALLS;
					if (IsDlgButtonChecked(hDlg, IDC_JUSTTODLL))
						Option.dwIgnoreToDlls = JUST_SPECIAL_DLL_CALLS;

					// - process the from Dlls to ignore
					Option.wIgnoreFromDllNum = (WORD)SendDlgItemMessage(
						hDlg,
						IDC_FROMDLLLIST,
						LB_GETCOUNT,
						NULL,
						NULL);
					// are there to many Dlls ???
					if (Option.wIgnoreFromDllNum > MAX_IGNOREDLLNUM)
					{
						wsprintf(MsgBuff,"You can add a maximum of %i Dll's here !", \
							MAX_IGNOREDLLNUM);
						MessageBox(hDlg,MsgBuff,"Error",MB_ICONERROR);
						break;
					}
					// save the from Dll names
					for (i=0; i<Option.wIgnoreFromDllNum; i++)
						SendDlgItemMessage(hDlg,IDC_FROMDLLLIST,LB_GETTEXT,i, \
						(LPARAM)Option.cIgnoreFromDlls[i]);
					Option.dwIgnoreFromDlls = NO_SPECIAL_DLLS;
					if (IsDlgButtonChecked(hDlg,IDC_IGNOREFROMDLL))
						Option.dwIgnoreFromDlls = DONT_SPECIAL_DLL_CALLS;
					if (IsDlgButtonChecked(hDlg, IDC_JUSTFROMDLL))
						Option.dwIgnoreFromDlls = JUST_SPECIAL_DLL_CALLS;

					// - process the Regions to ignore
					Option.wIgnoreRegionNum = (WORD)SendDlgItemMessage(
						hDlg,
						IDC_REGIONLIST,
						LB_GETCOUNT,
						NULL,
						NULL);
					// are there to many Dlls ???
					if (Option.wIgnoreRegionNum > MAX_IGNOREREGIONNUM)
					{
						wsprintf(MsgBuff,"You can add a maximum of %i region's here !", \
							MAX_IGNOREDLLNUM);
						MessageBox(hDlg,MsgBuff,"Error",MB_ICONERROR);
						break;
					}
					// save the Regions
					for (i=0; i<Option.wIgnoreRegionNum; i++)
					{
						SendDlgItemMessage(hDlg,IDC_REGIONLIST,LB_GETTEXT,i, (LPARAM)cBuff);
						char tmp[9];
						strncpy(tmp, &cBuff[2], 8);
						tmp[8] = 0;
						if (!Hexstr2Int(tmp, Option.dwIgnoreRegionBegin[i]))
							 Option.dwIgnoreRegionBegin[i] = 0;
						strncpy(tmp, &cBuff[15], 8);
						tmp[8] = 0;
						if (!Hexstr2Int(tmp, Option.dwIgnoreRegionEnd[i]))
							 Option.dwIgnoreRegionEnd[i] = 0;
					}
					Option.dwIgnoreRegions = NO_SPECIAL_DLLS;
					if (IsDlgButtonChecked(hDlg,IDC_IGNOREREGION))
						Option.dwIgnoreRegions = DONT_SPECIAL_DLL_CALLS;
					if (IsDlgButtonChecked(hDlg, IDC_JUSTREGION))
						Option.dwIgnoreRegions = JUST_SPECIAL_DLL_CALLS;

					// process the rest
					Option.dwIgnoreAPIs = NO_SPECIAL_APIS;
					if (IsDlgButtonChecked(hDlg, IDC_IGNOREAPI))
						Option.dwIgnoreAPIs = DONT_CALL_SPECIAL_APIS;
					if (IsDlgButtonChecked(hDlg, IDC_ONLYAPI))
						Option.dwIgnoreAPIs = JUST_CALL_SPECIAL_APIS;

					Option.bDebugProcess         = IsDlgButtonChecked(hDlg,IDC_DEBUGPROCESS);
					Option.bShowAPICall          = IsDlgButtonChecked(hDlg,IDC_SHOWAPICALLS);
					Option.bShowApiParams        = IsDlgButtonChecked(hDlg,IDC_APIPARAMS);
					Option.bShowApiReturn        = IsDlgButtonChecked(hDlg,IDC_SHOWAPIRET);

					Option.bShowDebugEvents      = IsDlgButtonChecked(hDlg,IDC_SHOWDEBUG);
					Option.bMoveWindow           = IsDlgButtonChecked(hDlg,IDC_MOVEWIN);
					Option.bReportNameAPI        = IsDlgButtonChecked(hDlg,IDC_SHOWAPI);
					Option.bReportOrdinalAPI     = IsDlgButtonChecked(hDlg,IDC_SHOWAPIORDINAL);
					Option.bHandleExceptions     = IsDlgButtonChecked(hDlg,IDC_HANDLEEXCEPT);
					Option.bStopAtDB             = IsDlgButtonChecked(hDlg,IDC_DEBUGBREAK);
					Option.bGUIOutPut            = IsDlgButtonChecked(hDlg,IDC_GUIOUTPUT);
					Option.bFileOutPut           = IsDlgButtonChecked(hDlg,IDC_FILEOUTPUT);
					Option.bStopAtEntry          = IsDlgButtonChecked(hDlg,IDC_STOPATENTRY);
					Option.bDlgOnBPM             = IsDlgButtonChecked(hDlg,IDC_DLGONBPM);
					Option.bShellExtension       = IsDlgButtonChecked(hDlg,IDC_SHELLEXTENSION);
					Option.bShowTIDs             = IsDlgButtonChecked(hDlg,IDC_SHOWTIDS);

					// no trap dll list
					//Option.bDllNoTrapList        = IsDlgButtonChecked(hDlg,IDC_NOTRAPDLL);
					//GetDlgItemText(hDlg, IDC_NOTRAPDLLEDIT, Option.cDllNoTrapList,
					//	sizeof(Option.cDllNoTrapList));

					// process the additional commandline
					Option.bAdditionalCmdLine = IsDlgButtonChecked(hDlg,IDC_CMDLINE);
					GetDlgItemText(hDlg,IDC_CMDLINESTR,Option.cCmdLine,\
						sizeof(Option.cCmdLine));
					GetDlgItemText(hDlg,IDC_OUTPUTFILENAME,Option.cOutFile,
						sizeof(Option.cOutFile));

					// handle shell extension option
					if (Option.bShellExtension)
					{
						GetModuleFileName(NULL, cBuff, sizeof(cBuff));
						SetSSShellExtension(cBuff);
					}
					else
						KillSSShellExtension();

					// notify the ApiSnoop dll the option has changed
					NotifyOptionChange();

					// quit dialog
					EndDialog(hDlg,0);
					return TRUE;

				case ID_CANCEL:
					EndDialog(hDlg,0);
					return TRUE;

				case IDC_IGNOREAPI:
				case IDC_ONLYAPI:
					bButtonState = IsDlgButtonChecked(hDlg,IDC_IGNOREAPI) ? TRUE : FALSE;
					bButtonState2 = IsDlgButtonChecked(hDlg,IDC_ONLYAPI) ? TRUE : FALSE;

					EnableWindow((HWND)hAddApiB,bButtonState || bButtonState2);
					EnableWindow((HWND)hDeleteApiB,bButtonState || bButtonState2);
					EnableWindow((HWND)hEditApi,bButtonState || bButtonState2);
					EnableWindow((HWND)hAPIList,bButtonState || bButtonState2);

					if (bButtonState && bButtonState2) // no double check !
						if (LOWORD(wParam) == IDC_IGNOREAPI)
							CheckDlgButton(hDlg, IDC_ONLYAPI, FALSE);
						else
							CheckDlgButton(hDlg, IDC_IGNOREAPI, FALSE);

					SetFocus((HWND)hEditApi);
					return TRUE;

				case IDC_ADDAPI:
					// was sth entered ?
					GetDlgItemText(hDlg,IDC_EDITAPI,cEditBoxBuff,sizeof(cEditBoxBuff));
					if (cEditBoxBuff[0] == 0)
					{
						MessageBeep(MB_ICONERROR);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_APILIST,LB_ADDSTRING,NULL,\
						(LPARAM)cEditBoxBuff);
					SetDlgItemText(hDlg,IDC_EDITAPI,"");
					return TRUE;

				case IDC_DELETEAPI:
					iSelIndex = SendDlgItemMessage(hDlg,IDC_APILIST,LB_GETCURSEL,NULL,NULL);
					if (iSelIndex < 0)
					{
						MessageBeep(MB_ICONERROR);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_APILIST,LB_DELETESTRING,iSelIndex,NULL);
					return TRUE;

				case IDC_IGNORETODLL:
				case IDC_JUSTTODLL:
					bButtonState  = IsDlgButtonChecked(hDlg,IDC_IGNORETODLL);
					bButtonState2 = IsDlgButtonChecked(hDlg,IDC_JUSTTODLL);

					EnableWindow((HWND)hAddToDllB,bButtonState || bButtonState2);
					EnableWindow((HWND)hDeleteToDllB,bButtonState || bButtonState2);
					EnableWindow((HWND)hEditToDll,bButtonState || bButtonState2);
					EnableWindow((HWND)hToDllList,bButtonState || bButtonState2);

					if (bButtonState && bButtonState2)
						if (LOWORD(wParam) == IDC_IGNORETODLL)
							CheckDlgButton(hDlg, IDC_JUSTTODLL, FALSE);
						else
							CheckDlgButton(hDlg, IDC_IGNORETODLL, FALSE);

					SetFocus((HWND)hEditToDll);
					return TRUE;

				case IDC_ADDTODLL:
					// was sth entered ?
					GetDlgItemText(hDlg,IDC_EDITTODLL,cEditBoxBuff,sizeof(cEditBoxBuff));
					if (cEditBoxBuff[0] == 0)
					{
						MessageBeep(0);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_TODLLLIST,LB_ADDSTRING,NULL,\
						(LPARAM)cEditBoxBuff);
					SetDlgItemText(hDlg,IDC_EDITTODLL,"");
					return TRUE;

				case IDC_DELETETODLL:
					iSelIndex = SendDlgItemMessage(hDlg,IDC_TODLLLIST,LB_GETCURSEL,NULL,\
						NULL);
					if (iSelIndex < 0)
					{
						MessageBeep(MB_ICONERROR);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_TODLLLIST,LB_DELETESTRING,iSelIndex,NULL);
					return TRUE;

				case IDC_IGNOREFROMDLL:
				case IDC_JUSTFROMDLL:
					bButtonState  = IsDlgButtonChecked(hDlg,IDC_IGNOREFROMDLL);
					bButtonState2 = IsDlgButtonChecked(hDlg,IDC_JUSTFROMDLL);

					EnableWindow((HWND)hAddFromDllB,bButtonState || bButtonState2);
					EnableWindow((HWND)hDeleteFromDllB,bButtonState || bButtonState2);
					EnableWindow((HWND)hEditFromDll,bButtonState || bButtonState2);
					EnableWindow((HWND)hFromDllList,bButtonState || bButtonState2);

					if (bButtonState && bButtonState2)
						if (LOWORD(wParam) == IDC_IGNOREFROMDLL)
							CheckDlgButton(hDlg, IDC_JUSTFROMDLL, FALSE);
						else
							CheckDlgButton(hDlg, IDC_IGNOREFROMDLL, FALSE);

					SetFocus((HWND)hEditFromDll);
					return TRUE;

				case IDC_ADDFROMDLL:
					// was sth entered ?
					GetDlgItemText(hDlg,IDC_EDITFROMDLL,cEditBoxBuff,sizeof(cEditBoxBuff));
					if (cEditBoxBuff[0] == 0)
					{
						MessageBeep(0);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_FROMDLLLIST,LB_ADDSTRING,NULL,\
						(LPARAM)cEditBoxBuff);
					SetDlgItemText(hDlg,IDC_EDITFROMDLL,"");
					return TRUE;

				case IDC_DELETEFROMDLL:
					iSelIndex = SendDlgItemMessage(hDlg,IDC_FROMDLLLIST,LB_GETCURSEL,NULL,\
						NULL);
					if (iSelIndex < 0)
					{
						MessageBeep(MB_ICONERROR);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_FROMDLLLIST,LB_DELETESTRING,iSelIndex,NULL);
					return TRUE;

				case IDC_IGNOREREGION:
				case IDC_JUSTREGION:
					bButtonState  = IsDlgButtonChecked(hDlg,IDC_IGNOREREGION);
					bButtonState2 = IsDlgButtonChecked(hDlg,IDC_JUSTREGION);

					EnableWindow((HWND)hAddRegionB,bButtonState || bButtonState2);
					EnableWindow((HWND)hDeleteRegionB,bButtonState || bButtonState2);
					EnableWindow((HWND)hEditRegionBegin,bButtonState || bButtonState2);
					EnableWindow((HWND)hEditRegionEnd,bButtonState || bButtonState2);
					EnableWindow((HWND)hRegionList,bButtonState || bButtonState2);

					if (bButtonState && bButtonState2)
						if (LOWORD(wParam) == IDC_IGNOREREGION)
							CheckDlgButton(hDlg, IDC_JUSTREGION, FALSE);
						else
							CheckDlgButton(hDlg, IDC_IGNOREREGION, FALSE);

					SetFocus((HWND)hEditRegionBegin);
					return TRUE;

				case IDC_ADDREGION:
					// was sth entered ?
					GetDlgItemText(hDlg,IDC_EDITREGIONBEGIN,cEditBoxBuff,sizeof(cEditBoxBuff));
					if (cEditBoxBuff[0] == 0)
					{
						MessageBeep(0);
						return TRUE;
					}
					if (!Hexstr2Int(cEditBoxBuff, dwRegionBegin))
					{
						MessageBox(
							hDlg,
							"Invalid region base address !",
							"ERROR",
							MB_ICONERROR);
						return TRUE;
					}
					GetDlgItemText(hDlg,IDC_EDITREGIONEND,cEditBoxBuff,sizeof(cEditBoxBuff));
					if (cEditBoxBuff[0] == 0)
					{
						MessageBeep(0);
						return TRUE;
					}
					if (!Hexstr2Int(cEditBoxBuff, dwRegionEnd))
					{
						MessageBox(
							hDlg,
							"Invalid region end address !",
							"ERROR",
							MB_ICONERROR);
						return TRUE;
					}
					wsprintf(cEditBoxBuff, "0x%08X ~ 0x%08X", dwRegionBegin, dwRegionEnd);
					SendDlgItemMessage(hDlg,IDC_REGIONLIST,LB_ADDSTRING,NULL,\
						(LPARAM)cEditBoxBuff);
					SetDlgItemText(hDlg,IDC_EDITREGIONBEGIN,"");
					SetDlgItemText(hDlg,IDC_EDITREGIONEND,"");
					return TRUE;

				case IDC_DELETEREGION:
					iSelIndex = SendDlgItemMessage(hDlg,IDC_REGIONLIST,LB_GETCURSEL,NULL,\
						NULL);
					if (iSelIndex < 0)
					{
						MessageBeep(MB_ICONERROR);
						return TRUE;
					}
					SendDlgItemMessage(hDlg,IDC_REGIONLIST,LB_DELETESTRING,iSelIndex,NULL);
					return TRUE;

				case IDC_EDITAPIDLL:
					if (HIWORD(wParam) == CBN_CLOSEUP)
					{
						GetDlgItemText(hDlg, LOWORD(wParam), cEditBoxBuff, sizeof(cEditBoxBuff));
						SDIM(hDlg, IDC_EDITAPI, CB_RESETCONTENT, 0, 0);
						ApiListToCombo(hDlg, LOWORD(IDC_EDITAPI), GetModuleIndex(cEditBoxBuff));
					}
					return TRUE;

				case IDC_CMDLINE:
					bButtonState = IsDlgButtonChecked(hDlg,IDC_CMDLINE);
					EnableWindow(
						(HWND)hEditCmdLine,
						bButtonState);		
					if(bButtonState)
						SetFocus((HWND)hEditCmdLine);
					return TRUE;

				case IDC_FILEOUTPUT:
					bButtonState = IsDlgButtonChecked(hDlg,IDC_FILEOUTPUT);
					EnableWindow(
						hOutPutFileName,
						bButtonState);
					if (bButtonState)
						SetFocus(hOutPutFileName);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL SetSSShellExtension(char* szSSExePath)
{
	const  char* szKeyFmtValue = "%s %1";
	HKEY   hKey;
	DWORD  dwBlah;
	char   cBuff[MAX_PATH + 10];

	wsprintf(cBuff, "exefile\\shell\\%s\\command", szSSShellEntry);
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, cBuff,
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwBlah))
		return FALSE;
	wsprintf(cBuff, "\"%s\" %%1", szSSExePath);
	if (RegSetValueEx(hKey, NULL, 0, REG_SZ, (CONST BYTE*)&cBuff, lstrlen(cBuff)))
		return FALSE;

	// clean up
	RegCloseKey(hKey);

	return TRUE;
}

BOOL KillSSShellExtension()
{
	HKEY  hKey;
	char  cBuff[MAX_PATH];

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "exefile\\shell", 0, KEY_ALL_ACCESS, &hKey))
		return FALSE;
	wsprintf(cBuff, "%s\\command", szSSShellEntry);
	if (RegDeleteKey(hKey, cBuff))
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	if (RegDeleteKey(hKey, szSSShellEntry))
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	// clean up
	RegCloseKey(hKey);

	return TRUE;
}
