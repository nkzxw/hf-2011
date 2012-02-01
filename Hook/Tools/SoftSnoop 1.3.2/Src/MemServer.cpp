
#include <windows.h>
#include "MemServer.h"
#include "lib.h"
#include "resource.h"

// functions
VOID     AddLB(PSTR szText);
LRESULT  CALLBACK EditBoxProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
VOID     CreateMemServerDlg(HINSTANCE hInst,HANDLE hDlgOwner,DWORD PID);
LRESULT  CALLBACK MemServerWndProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam);
VOID     ShowCommands();
BOOL     ProcessCommand();
BOOL     ProcessArgs();
VOID     DisplayMem();
VOID     FillMem();
VOID     SearchInMem();
VOID     DumpMem();
VOID     PatchMem();
VOID     LoadToMem();
VOID     SaveList();

// constants
#define        WM_CMDEDITENTER          WM_USER + 0x1001
#define        CX_MS_WND                560
#define        CY_MS_WND                295
#define        MS_LB_HEIGHT             CY_MS_WND - CMD_MS_EDIT_HEIGHT - 24
#define        CMD_MS_EDIT_HEIGHT       24
#define        MAX_COMMAND_ARGS         5
#define        MAX_ARG_SIZE             40
#define        LINE_NUM                 16
#define        CMD_SAVE_NUM             5

#define        ARG1                     cCmdArgs[0]
#define        ARG2                     cCmdArgs[1]
#define        ARG3                     cCmdArgs[2]
#define        ARG4                     cCmdArgs[3]
#define        ARG5                     cCmdArgs[4]

CONST CHAR*    szMSCaption           = "Memory Server";
CONST CHAR*    SC_EDIT_CLASSNAME     = "SS_EDIT";
CONST CHAR*    szBAD_COMMAND         = "Unkown Command !";
CONST CHAR*    szCOMMANDS            = "Commands:\0"\
                                       " d [Address]                - show the memory at ADDRESS\0"\
									   " s [Address] [Size] [Bytes] - search BYTES at ADDRESS\0"\
									   "                              (if BYTES are in brackets then it's a string)\0"\
									   " dp [Address] [Size]        - save SIZE byte at ADDRESS to disk\0"\
									   " l [Address] (Size) (f)     - load file content to ADDRESS\0"\
									   " p [Address] [Bytes] (f)    - patch the memory at ADDRESS with BYTES\0"\
									   "                              (if BYTES are in brackets then it's a string)\0"\
									   " f [Address] [Size] [c] (f) - fill SIZE memory at ADDRESS with a character\0"\
									   " save                       - save ListBox contents to file\0"\
                                       " cls                        - clear ListBox\0"\
                                       " q                          - close the Memory Server\0"\
									   " ?                          - show this command list\0"\
									   " \0"\
									   " [] - must be entered  () - can be entered\0"\
									   " (f) - can be passed to the parameters to force the write operations\0"\
									   " You can use the up and down cursor keys to access the last commands.\0";

// variables
HWND           hMSWnd = 0,hLBMS,hMSCmd,hParent;
VOID*          pOldEditProc;
CHAR           cCmdArgs[MAX_COMMAND_ARGS][MAX_ARG_SIZE];
CHAR           cLastCmds[CMD_SAVE_NUM][MAX_COMMAND_ARGS * MAX_ARG_SIZE];
BYTE           ArgNum;
HANDLE         hProcR,hProcRW;
CHAR           Buff[300];
int            iLastCmdIndex;


VOID AddLB(PSTR szText)
{
	// add the text
	SendMessage(hLBMS,LB_ADDSTRING,0,(LPARAM)szText);
	// scroll down
	if (SendMessage(hLBMS,LB_GETCOUNT,0,0) > LINE_NUM)
		SendMessage(hLBMS,WM_VSCROLL,SB_LINEDOWN,0);
	return;
}

LRESULT CALLBACK EditBoxProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			iLastCmdIndex = -1;
			SendMessage(hMSWnd,WM_CMDEDITENTER,0,0);
			return 0;
		}

		if (wParam == VK_ESCAPE)
		{
			SendMessage(hMSWnd,WM_CLOSE,0,0);
			return 0;
		}

		if (wParam == VK_UP)
		{
			if (iLastCmdIndex != CMD_SAVE_NUM-1)
				if (*(CHAR*)cLastCmds[iLastCmdIndex+1])
					++iLastCmdIndex;
			SetWindowText(hMSCmd,cLastCmds[iLastCmdIndex]);
			return 0;
		}

		if (wParam == VK_DOWN)
		{
			if (iLastCmdIndex > -1)
				--iLastCmdIndex;

			if (iLastCmdIndex == -1)
				SetWindowText(hMSCmd,"");
			else
				SetWindowText(hMSCmd,cLastCmds[iLastCmdIndex]);
			return 0;
		}
		break;
	}
	return CallWindowProc((WNDPROC)pOldEditProc,hWnd,Msg,wParam,lParam);
}

VOID CreateMemServerDlg(HINSTANCE hInst,HANDLE hDlgOwner,DWORD PID)
{
	WNDCLASS    wc;
	WNDCLASSEX  wcEx;
	DWORD       cx,cy;

	hParent = (HWND)hDlgOwner;

	memset(&cLastCmds,0,sizeof(cLastCmds));
	iLastCmdIndex = -1;

	// get process handles
	hProcR  = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION,FALSE,PID);
	hProcRW = OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
		FALSE,
		PID);
	if (!hProcR || !hProcRW)
	{
		MessageBox(
			(HWND)hDlgOwner,
			"Couldn't get full process access :(",
			"Memory Server",
			MB_ICONERROR);
		return;
	}

	ZeroMemory(&wc,sizeof(wc));
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInst;
	wc.lpfnWndProc   = MemServerWndProc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_MemServer";
	RegisterClass(&wc);

	// create the main wnd
	cx = (GetSystemMetrics(SM_CXFULLSCREEN) - CX_MS_WND) / 2;
	cy = (GetSystemMetrics(SM_CYFULLSCREEN) - CY_MS_WND) / 2;
	hMSWnd = CreateWindow(
		wc.lpszClassName,
		szMSCaption,
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		cx,
		cy,
		CX_MS_WND,
		CY_MS_WND,
		(HWND)hDlgOwner,
		0,
		hInst,
		NULL);
	// ...listbox
	hLBMS = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"LISTBOX",
		NULL,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT |
		LBS_HASSTRINGS,
		0,
		0,
		CX_MS_WND - 6,
		MS_LB_HEIGHT,
		hMSWnd,
		NULL,
		hInst,
		NULL);

	// SuperClass the EditBox (much thx to Icz !!!)
	ZeroMemory(&wcEx,sizeof(wcEx));
	wcEx.cbSize = sizeof(wcEx);
	GetClassInfoEx(NULL,"EDIT",&wcEx);
	pOldEditProc = (VOID*)wcEx.lpfnWndProc;
	wcEx.lpfnWndProc = &EditBoxProc;
	wcEx.lpszClassName = SC_EDIT_CLASSNAME;
	wcEx.hInstance = hInst;
	RegisterClassEx(&wcEx);

    // ...editbox for commands
	hMSCmd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		SC_EDIT_CLASSNAME,
		NULL,
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		0,
		MS_LB_HEIGHT,
		CX_MS_WND - 6,
		CMD_MS_EDIT_HEIGHT,
		hMSWnd,
		NULL,
		hInst,
		NULL);

	// show the MainWindow - cause the WM_SHOWWINDOW event
	ShowWindow(hMSWnd,SW_SHOW);
	UpdateWindow(hMSWnd);

	return;
}

LRESULT CALLBACK MemServerWndProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	static HFONT hFont;

	switch(Msg)
	{
	case WM_SHOWWINDOW:
		// set cursor to the editbox
		SetFocus(hMSCmd);
		// set the font of the listbox
  //      hFont = CreateFont(14,8,0,0,200,0,0,0,OEM_CHARSET,OUT_DEFAULT_PRECIS,
		//	CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_SCRIPT,"Courier New");
		//SendMessage(hLBMS,WM_SETFONT,(WPARAM)hFont,0);
		ShowCommands();
		return 0;

	case WM_CMDEDITENTER:
		// Enter was pressed in the editbox
		ProcessCommand();
		return 0;

	case WM_ACTIVATE:
		SetFocus(hMSCmd);
		return 0;

	case WM_MS_NEWPROCESS:
		CloseHandle(hProcR);
		CloseHandle(hProcRW);
		// get process handles
		hProcR  = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION,FALSE,wParam);
		hProcRW = OpenProcess(
			PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
			FALSE,
			wParam);
		if (!hProcR || !hProcRW)
		{
			MessageBox(
				(HWND)hWnd,
				"Couldn't get full process access :(",
				"Memory Server",
				MB_ICONERROR);
			SendMessage(hWnd,WM_CLOSE,0,0);
			return 0;
		}
		AddLB("Process information updated...");		
		return 0;

	case WM_CLOSE:
		// clean up
		DeleteObject(hFont);
		DestroyWindow(hWnd);
		CloseHandle(hProcR);
		CloseHandle(hProcRW);
		return 0;

	case WM_DESTROY:
		hMSWnd = 0;
		// set the focus on the main window
		SetForegroundWindow(hParent);
		return 0;
	}
	return DefWindowProc(hWnd,Msg,wParam,lParam);
}

VOID ShowCommands()
{
	CHAR *pCH;

	pCH = (CHAR*)szCOMMANDS;
	while (*pCH)
	{
		AddLB(pCH);
		pCH += lstrlen(pCH) + 1;
	}
	AddLB("");
	return;
}

BOOL ProcessCommand()
{
	DWORD dwStrLen;
	CHAR  CmdBuff[200],*pCH,*pchArg,cCmdCopy[200];
	int   i,iArg;

	// save the entered command
	for (i=CMD_SAVE_NUM-2; i != -1; i--)
		lstrcpyn(cLastCmds[i+1],cLastCmds[i],MAX_ARG_SIZE);
	GetWindowText(hMSCmd,cLastCmds[0],MAX_ARG_SIZE);
	// split the commandline into parts
	memset(&cCmdArgs,0,sizeof(cCmdArgs));
	memset(&CmdBuff,0,sizeof(CmdBuff));
	ArgNum = 0;
	GetWindowText(hMSCmd,CmdBuff,sizeof(CmdBuff));
	dwStrLen = lstrlen(CmdBuff);
	pCH = (CHAR*)CmdBuff;
	for (i=0; i < MAX_COMMAND_ARGS; i++)
	{
		// get the start of the next parameter
		if (!*pCH)
			break;  // end of the commandline
		while((UINT)*pCH == 0x20)  // skip spaces
			++pCH;
		if (!*pCH)
			break;

		pchArg = (CHAR*)&cCmdArgs[i][0];
		++ArgNum;
		for (
			iArg=0;
			(UINT)*pCH > 0x20;
			iArg++)
			{
				if (iArg > MAX_ARG_SIZE)
				{
					wsprintf(
						Buff,
						"Each parameter is limited to %i characters !",
						MAX_ARG_SIZE);
					AddLB(Buff);
					return FALSE;
				}
				*pchArg++ = *pCH++;
			}
	}
	SetWindowText(hMSCmd,"");  // clear commandline editbox
	// show the entered text
	memcpy((PVOID)&cCmdCopy[1],&CmdBuff[0],lstrlen(CmdBuff) + 1);
	cCmdCopy[0] = ':';
	AddLB(cCmdCopy);
	// process the entered command
	if (iArg)
		if (!ProcessArgs())
			AddLB((CHAR*)szBAD_COMMAND);
	return TRUE;
}

BOOL ProcessArgs()
{
	// check the first arg and start the corresponding routine
	// cls
	if (lstrcmpi((LPCTSTR)&ARG1,"cls") == 0)
	{
		SendMessage(hLBMS,LB_RESETCONTENT,0,0);
		return TRUE;
	}
	// q
	if (lstrcmpi((PCSTR)&ARG1,"q") == 0)
	{
		SendMessage(hMSWnd,WM_CLOSE,0,0);
		return TRUE;
	}
	// ?
	if (lstrcmpi((PCSTR)&ARG1,"?") == 0)
	{
		ShowCommands();
		return TRUE;
	}
	// d
	if (lstrcmpi((PCSTR)&ARG1,"d") == 0)
	{
		DisplayMem();
		return TRUE;
	}
	// f
	if (lstrcmpi((PCSTR)&ARG1,"f") == 0)
	{
		FillMem();
		return TRUE;
	}
	// s
	if (lstrcmpi((PCSTR)&ARG1,"s") == 0)
	{
		SearchInMem();
		return TRUE;
	}
	// dp
	if (lstrcmpi((PCSTR)&ARG1,"dp") == 0)
	{
		DumpMem();
		return TRUE;
	}
	// p
	if (lstrcmpi((PCSTR)&ARG1,"p") == 0)
	{
		PatchMem();
		return TRUE;
	}
	// l
	if (lstrcmpi((PCSTR)&ARG1,"l") == 0)
	{
		LoadToMem();
		return TRUE;
	}
	// save
	if (lstrcmpi((PCSTR)&ARG1, "save") == 0)
	{
		SaveList();
		return TRUE;
	}
	return FALSE;
}

VOID DisplayMem()
{
	DWORD  dwAddr,i,dwBytesRead,ii;
	CHAR   cHexOut[100],cCharOut[17],cOutBuff[100];
	BYTE   by;

	// check the entered values
	if (ArgNum != 2)
	{
		AddLB("Invalid number of parameters !");
		return;
	}
	if (lstrlen(ARG2) > 8)
	{
		AddLB("The Memory Address is limited to 8 characters !");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddr))
	{
		AddLB("Invalid address !");
		return;
	}
	// show it
	for (ii=0; ii < LINE_NUM; ii++)
	{
		memset(&cCharOut,0,sizeof(cCharOut));
		memset(&cHexOut,0,sizeof(cHexOut));
		for (i=0; i < 16; i++)
		{
			if (i)
				cHexOut[i*2+i-1] = 0x20;
			if (ReadProcessMemory(
				hProcR,
				(VOID*)(dwAddr + i),
				&by,
				1,
				&dwBytesRead))
			{
				wsprintf((CHAR*)&cHexOut[i*2+i],"%02lX",by);
				if (by < 32)
					cCharOut[i] = '.';
				else
					cCharOut[i] = (CHAR)by;
			}
			else
			{
				cHexOut[i*3]   = '?';
				cHexOut[i*3+1] = '?';
				cCharOut[i] = '.';
			}
		}
		// build the whole line...
		wsprintf(
			cOutBuff,
			"%08lX  %s  %s",
			dwAddr,
			&cHexOut,
			&cCharOut);
		// ...and show it
		AddLB(cOutBuff);
		// prepare the address for the next line
		dwAddr += 16;
	}
	return;
}

VOID FillMem()
{
	BYTE  byTarCh;
	DWORD dwAddress,dwSize,dwTarCh,dwBytesWritten,dwOldProt,dwBuff;
	BOOL  bForce;
	VOID  *pMem;

	// check the parameters
	if (ArgNum < 4)
	{
		AddLB("Too less parameters !");
		return;
	}
	if (lstrlen(ARG2) > 8 || lstrlen(ARG3) > 8)
	{
		AddLB("The memory address and size are limited to 8 characters !");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddress))
	{
		AddLB("Invalid address !");
		return;
	}
	if (!Hexstr2Int(ARG3,dwSize))
	{
		AddLB("Invalid size !");
		return;
	}
	if (!Hexstr2Int(ARG4,dwTarCh))
	{
		AddLB("The 4th parameter is invalid !");
		return;
	}
	byTarCh = (BYTE)dwTarCh;
	bForce = FALSE;
	if (lstrcmpi(ARG5,"f") == 0)
		bForce = TRUE;

	// start to fill mem
	pMem = GlobalAlloc(GMEM_FIXED,dwSize);
	if (!pMem)
	{
		AddLB("Not enough memory :(");
		return;
	}
	memset(pMem,byTarCh,dwSize);

	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddress,
		dwSize,
		PAGE_READWRITE,
		&dwOldProt);

	if (WriteProcessMemory(
		hProcRW,
		(VOID*)dwAddress,
		pMem,
		dwSize,
		&dwBytesWritten))
		AddLB("Done");
	else
		AddLB("Error");
	
	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddress,
		dwSize,
		dwOldProt,
		&dwBuff);
	// clean up
	GlobalFree(pMem);
	return;
}

VOID SearchInMem()
{
	DWORD  dwAddress,dwSize,dwByte,i,dwBytesRead;
	BYTE   bySearchBytes[MAX_ARG_SIZE/2],*pBy;
	CHAR   cSBBuff[3],*pCH;
	WORD   wSBNum,wIndex,wFoundNum;
	VOID   *pMem;
	BOOL   bString = FALSE;

	// check the parameters
	if (ArgNum != 4)
	{
		AddLB("Invalid number of parameters !");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddress))
	{
		AddLB("Invalid address !");
		return;
	}
	if (!Hexstr2Int(ARG3,dwSize))
	{
		AddLB("Invalid size !");
		return;
	}
	if (*(CHAR*)ARG4 == '"')
		bString = TRUE;

	if (!IsRoundedTo(lstrlen(ARG4),2) && !bString)
	{
		AddLB("The search bytes must be a even number of characters !");
		return;
	}
	pCH = (CHAR*)ARG4;
	wSBNum = 0;
	if (bString)
	{	// a string was entered
		++pCH;
		while (*pCH && *pCH != '"')
			bySearchBytes[wSBNum++] = (BYTE)*pCH++;
	}
	else // search bytes were entered
		while (*pCH)
		{
			lstrcpyn(cSBBuff,pCH,3);
			if (!Hexstr2Int(cSBBuff,dwByte))
			{
				AddLB("Invalid search bytes !");
				return;
			}
			bySearchBytes[wSBNum] = (BYTE)dwByte;
			++wSBNum;
			pCH += 2;
		}

	// start to search
	pMem = GlobalAlloc(GMEM_FIXED,dwSize);
	if (!pMem)
	{
		AddLB("Not enough memory :(");
		return;
	}
	if (!ReadProcessMemory(
		hProcR,
		(VOID*)dwAddress,
		pMem,
		dwSize,
		&dwBytesRead))
	{
		GlobalFree(pMem);
		AddLB("Could read process memory :(");
		return;
	}
	wFoundNum = 0;
	pBy = (BYTE*)pMem;
	wIndex = 0;
	for (i=0; i < dwSize; i++)
	{
		if (*pBy == bySearchBytes[wIndex])
			++wIndex;
		if (wIndex == wSBNum)
		{
			wIndex = 0;
			++wFoundNum;
			wsprintf(Buff,"Pattern found at 0x%08lX",dwAddress+i-wSBNum+1);
			AddLB(Buff);
		}
		++pBy;
	}
	GlobalFree(pMem);
	wsprintf(Buff,"The pattern was found %u time(s).",wFoundNum);
	AddLB(Buff);
	return;
}

VOID DumpMem()
{
	DWORD              dwSize,dwAddr,dwBytesRead,dwBytesWritten;
	VOID*              pMem;
	OPENFILENAME       ofn;
	CHAR               cOutFile[MAX_PATH];
	HANDLE             hFile;

	// check the parameters
	if (ArgNum != 3)
	{
		AddLB("Invalid number of parameters !");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddr))
	{
		AddLB("Invalid address !");
		return;
	}
	if (!Hexstr2Int(ARG3,dwSize))
	{
		AddLB("Invalid size !");
		return;
	}
	// start
	// read target mem into our mem and address space :)
	if (!(pMem = GlobalAlloc(GMEM_FIXED,dwSize)))
	{
		AddLB("Not enough memory :(");
		return;
	}
	if (!ReadProcessMemory(
		hProcR,
		(VOID*)dwAddr,
		pMem,
		dwSize,
		&dwBytesRead))
	{
		GlobalFree(pMem);
		AddLB("Couldn't read process memory :(");
		return;
	}
	// save the file
	MakeOfn(ofn);
	lstrcpy(cOutFile,"dumped.bin");
	ofn.lpstrFile    = cOutFile;
	ofn.lpstrTitle   = "Save to...";
	ofn.hwndOwner    = hMSWnd;
	ofn.lpstrFilter  = "All files\0*.*\0";
	ofn.Flags       |= OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		// save the dumped mem to disk
		hFile = CreateFile(
			cOutFile,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			AddLB("Error during output file creation !");
			goto SkipSave;
		}
		WriteFile(hFile,pMem,dwSize,&dwBytesWritten,NULL);
		CloseHandle(hFile);
		wsprintf(Buff,"Memory saved to %s",&cOutFile);
		AddLB(Buff);
	}
SkipSave:
	// clean up
	GlobalFree(pMem);
	// set cursor to the editbox
	SetFocus(hMSCmd);
	return;
}

VOID PatchMem()
{
	DWORD  dwAddr,dwByte,dwOldProt,dwNewProt,dwSize,dwBytesWritten;
	BYTE   byPatch[MAX_ARG_SIZE/2];
	WORD   wPatches;
	BOOL   bString = FALSE,bForce = FALSE;
	CHAR   *pCH,cPatchBuff[3];

	// check the parameters
	if (ArgNum < 3)
	{
		AddLB("Too less parameters !");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddr))
	{
		AddLB("Invalid address !");
		return;
	}
	if (*(CHAR*)ARG3 == '"')
		bString = TRUE;

	if (lstrcmpi(ARG4,"f") == 0)
		bForce = TRUE;

	if (!IsRoundedTo(lstrlen(ARG3),2) && !bString)
	{
		AddLB("Invalid patch bytes !");
		return;
	}

	pCH = (CHAR*)ARG3;
	wPatches = 0;
	if (bString)
	{	// a string was entered
		++pCH;
		while (*pCH && *pCH != '"')
			byPatch[wPatches++] = (BYTE)*pCH++;
	}
	else // search bytes were entered
		while (*pCH)
		{
			lstrcpyn(cPatchBuff,pCH,3);
			if (!Hexstr2Int(cPatchBuff,dwByte))
			{
				AddLB("Invalid search bytes !");
				return;
			}
			byPatch[wPatches] = (BYTE)dwByte;
			++wPatches;
			pCH += 2;
		}
	dwSize = wPatches;

	// start to patch
	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddr,
		dwSize,
		PAGE_READWRITE,
		&dwOldProt);

	if (!WriteProcessMemory(
		hProcRW,
		(VOID*)dwAddr,
		&byPatch,
		dwSize,
		&dwBytesWritten))
		AddLB("Write error :(");
	else
		AddLB("Done");

	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddr,
		dwSize,
		dwOldProt,
		&dwNewProt);
	return;
}


VOID LoadToMem()
{
	DWORD          dwAddr,dwSize,dwFsize,dwBytesRead,dwBytesWritten,dwNewProt,dwOldProt;
	OPENFILENAME   ofn;
	BOOL           bForce = FALSE,bSize = FALSE;
	CHAR           cInFile[MAX_PATH];
	HANDLE         hFile;
	VOID*          pMem;

	// check the parameters
	if (ArgNum < 2)
	{
		AddLB("Too less parameters ");
		return;
	}
	if (!Hexstr2Int(ARG2,dwAddr))
	{
		AddLB("Invalid address !");
		return;
	}
	if (lstrcmpi(cCmdArgs[ArgNum-1],"f") == 0)
		bForce = TRUE;

	if (ArgNum > 2 && lstrcmpi(ARG3,"f") != 0)
	{
		bSize = TRUE;
		if (!Hexstr2Int(ARG3,dwSize))
		{
			AddLB("Invalid size !");
			return;
		}
	}

	// start
	MakeOfn(ofn);
	ofn.hwndOwner    = hMSWnd;
	cInFile[0] = 0;
	ofn.lpstrFile    = cInFile;
	ofn.lpstrTitle   = "Load...";
	if (!GetOpenFileName(&ofn))
		return;

	// map the file content
	hFile = CreateFile(cInFile,GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AddLB("File access error :(");
		return;
	}
	dwFsize = GetFileSize(hFile,NULL);
	if (bSize && dwFsize < dwSize)
	{
		AddLB("The file size is less than the entered size ! ...take the file size.");
		dwSize = dwFsize;
	}
	if (!bSize)
		dwSize = dwFsize;
	pMem = GlobalAlloc(GMEM_FIXED,dwSize);
	if (!pMem)
	{
		AddLB("Not enough memory !");
		CloseHandle(hFile);
		return;
	}
	if (!ReadFile(
		hFile,
		pMem,
		dwSize,
		&dwBytesRead,
		NULL))
	{
		AddLB("File read error !");
		goto CleanEnd;
	}
	// write the file memory into target process
	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddr,
		dwSize,
		PAGE_READWRITE,
		&dwOldProt);

	if (WriteProcessMemory(
		hProcRW,
		(VOID*)dwAddr,
		pMem,
		dwSize,
		&dwBytesWritten))
		AddLB("Done");
	else
		AddLB("Write error :(");

	if (bForce)
		VirtualProtectEx(
		hProcR,
		(VOID*)dwAddr,
		dwSize,
		dwOldProt,
		&dwNewProt);

CleanEnd:
	// clean up
	CloseHandle(hFile);
	GlobalFree(hFile);
	return;
}

void SaveList()
{
	HANDLE        hFile;
	OPENFILENAME  ofn;
	CHAR          cFname[MAX_PATH], cLine[300];
	DWORD         dwListCnt, i, dwBytesWritten;
	WORD          wLineEnd, wLength;

	// get a filepath
	MakeOfn(ofn);
	ofn.lpstrTitle          = "Save list to...";
	ofn.hwndOwner    = hMSWnd;
	lstrcpy(cFname, "memserv.txt");
	ofn.lpstrFile           = cFname;
	ofn.Flags               |= OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		hFile = CreateFile(
			cFname,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			0);
		// write the listbox lines to file
		dwListCnt = SendMessage(hLBMS, LB_GETCOUNT, 0, 0);
		wLineEnd = 0x0A0D;
		for (i=0; i < dwListCnt; i++)
		{
			// write the line into file
			wLength = (WORD)SendMessage(hLBMS, LB_GETTEXT, i, (LPARAM)&cLine);
			if (wLength == LB_ERR)
				continue;
			WriteFile(
				hFile,
				(VOID*)&cLine,
				wLength,
				&dwBytesWritten,
				NULL);
			// write end of line
			WriteFile(
				hFile,
				(VOID*)&wLineEnd,
				2,
				&dwBytesWritten,
				NULL);
		}
		// show ok msg
		wsprintf(Buff, "List saved to %s", cFname);
		AddLB(Buff);

		// clean up
		CloseHandle(hFile);
	} // end of: if GetOpenFileName
	return;
}