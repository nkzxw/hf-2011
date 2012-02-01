
#include <windows.h>
#include <commdlg.h>
#include <imagehlp.h>
#include <stdio.h>
#include "resource.h"
#include "SoftSnoop.h"
#include "DumpMenu.h"
#include "lib.h"

// functions
VOID     CreateDumpDlg(HINSTANCE hInst,HANDLE hDlgOwner,DWORD PID,DWORD dwBase,DWORD SizeOfImage);
LRESULT  CALLBACK DumpEditProcs(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL     CALLBACK DumpDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL     CheckEnteredValues();
BOOL     DumpProcess(DWORD PID,DWORD ImageBase,DWORD SizeOfImage);
BOOL     FixSections(CHAR* szTargetFname);
DWORD    Rva2Offset(PIMAGE_NT_HEADERS pNT,VOID* Base,DWORD dwTargetRVA);
BOOL     PasteOrgIT(CHAR* szOrgITPE,CHAR* szTargetPE);

// global variables
DWORD      dwPID,dwImageBase,dwSizeOfImage;
HWND       hDumpDlg;
CHAR       szFilePath[MAX_PATH],szDumpData[10];
WNDPROC    pOrgDumpEditProc;


VOID CreateDumpDlg(HINSTANCE hInst,HANDLE hDlgOwner,DWORD PID,DWORD dwBase,DWORD SizeOfImage)
{
	dwPID = PID;
	dwImageBase = dwBase;
	dwSizeOfImage = SizeOfImage;

	// create a dlg;
	WNDCLASS wc;
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "SS_DumpMenu";
	RegisterClass(&wc);
	DialogBox(hInst,MAKEINTRESOURCE(IDD_DUMP),(HWND)hDlgOwner,DumpDialogFunct);
	return;
}

LRESULT CALLBACK DumpEditProcs(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
	return CallWindowProc(pOrgDumpEditProc, hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK DumpDialogFunct(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL          bCheck;
	static HWND   hEditVA,hEditSize,hSecFix,hPasteOrgIt;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		// save some handles
		hSecFix = GetDlgItem(hDlg,IDC_FIXSECTIONS);
		hEditVA = GetDlgItem(hDlg,IDC_VA);
		hEditSize = GetDlgItem(hDlg,IDC_DUMPSIZE);
		hPasteOrgIt = GetDlgItem(hDlg,IDC_PASTEORGIT);
		hDumpDlg = hDlg;
		// set the default
		CheckDlgButton(hDlg,IDC_DUMPFULL,BST_CHECKED);
		SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_DUMPFULL,0),NULL);
		CheckDlgButton(hDlg,IDC_FIXSECTIONS,BST_CHECKED);
		CheckDlgButton(hDlg,IDC_PASTEORGIT,TRUE);
		// set max text length
		SendDlgItemMessage(hDlg,IDC_VA,EM_LIMITTEXT,8,0);
		SendDlgItemMessage(hDlg,IDC_DUMPSIZE,EM_LIMITTEXT,8,0);
		// hook edit procs
		pOrgDumpEditProc = (WNDPROC)SetWindowLong(
			GetDlgItem(hDlg, IDC_VA),
			GWL_WNDPROC,
			(DWORD)DumpEditProcs);
		SetWindowLong(
			GetDlgItem(hDlg, IDC_DUMPSIZE),
			GWL_WNDPROC,
			(LONG)DumpEditProcs);
		break;

	case WM_COMMAND:;
		switch(LOWORD(wParam))
		{
		case ID_DUMPDLG_DUMP:
			// check the entered values
			if (!CheckEnteredValues())
			{
				MessageBox(
					hDumpDlg,
					"Invalid dump values !",
					"ERROR",
					MB_ICONERROR);
				return TRUE;
			}
			if (!DumpProcess(dwPID,dwImageBase,dwSizeOfImage))
			{
				MessageBox(hDumpDlg,"Couldn't dump process :(","ERROR",MB_ICONERROR);
				return TRUE;
			}

			// fix the sections ?
			if (IsDlgButtonChecked(hDumpDlg,IDC_DUMPFULL) && 
				IsDlgButtonChecked(hDumpDlg,IDC_FIXSECTIONS))
				if (!FixSections((CHAR*)&szFilePath))
				{
					MessageBox(
						hDumpDlg,
						"Dumping done but there was\nan error while fixing the sections !",
						"ERROR",
						MB_ICONERROR);
					EndDialog(hDumpDlg,0);
					return TRUE;
				}
			//  rebuild the import table ?
			if (IsDlgButtonChecked(hDumpDlg,IDC_DUMPFULL) && 
				IsDlgButtonChecked(hDumpDlg,IDC_PASTEORGIT))
				if (!PasteOrgIT((CHAR*)szFname,(CHAR*)&szFilePath))
				{
					MessageBox(
						hDumpDlg,
						"Dumping done but there was\nan error while rebuilding the Import Table !",
						"ERROR",
						MB_ICONERROR);
					EndDialog(hDumpDlg,0);
					return TRUE;
				}
			MessageBox(hDumpDlg,"Dumping done.",":)",MB_ICONINFORMATION);
			EndDialog(hDumpDlg,0);
			return TRUE;

		case ID_DUMPDLG_CANCEL:
			EndDialog(hDlg,0);
			return TRUE;

		case IDC_DUMPFULL:
		case IDC_DUMPPARTIAL:
			bCheck = IsDlgButtonChecked(hDlg,IDC_DUMPFULL);
			EnableWindow(hSecFix,bCheck);
			EnableWindow(hPasteOrgIt,bCheck);
			EnableWindow(hEditVA,!bCheck);
			EnableWindow(hEditSize,!bCheck);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CheckEnteredValues()
{
	DWORD dwVal;

	if (IsDlgButtonChecked(hDumpDlg,IDC_DUMPFULL))
		return TRUE; // nothing to check

	// check the entered information
	GetDlgItemText(hDumpDlg,IDC_VA,(PSTR)&szDumpData,sizeof(szDumpData));
	Hexstr2Int(szDumpData,dwVal);
	if (!dwVal)
		return FALSE;
	GetDlgItemText(hDumpDlg,IDC_DUMPSIZE,(PSTR)&szDumpData,sizeof(szDumpData));
	Hexstr2Int(szDumpData,dwVal);
	if (!dwVal)
		return FALSE;

	return TRUE;
}

BOOL DumpProcess(DWORD PID,DWORD ImageBase,DWORD SizeOfImage)
{
	HANDLE         hProc;
	DWORD          dwOldProt,dwNewProt,dwBytesRead,dwBytesWritten,dwDumpSize,dwDumpStart;
	VOID*          pMem;
	OPENFILENAME   ofn;
	HANDLE         hFile;
	BOOL           bResult;

	szFilePath[0] = 0;
	memset(&ofn,0,(size_t)sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);

	// get a fitting handle
	hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION,FALSE,PID);
	if (!hProc)
		return FALSE;

	if (IsDlgButtonChecked(hDumpDlg,IDC_DUMPFULL))
	{
		// dump process memory FULL
		dwDumpSize = SizeOfImage;
		pMem = GlobalAlloc(GMEM_FIXED,SizeOfImage);
		if (!pMem)
		{
			CloseHandle(hProc);
			return FALSE;
		}
		VirtualProtectEx(
			hProc,
			(VOID*)ImageBase,
			SizeOfImage,
			PAGE_READONLY,
			&dwOldProt);
		bResult = ReadProcessMemory(
			hProc,
			(VOID*)ImageBase,
			pMem,
			SizeOfImage,
			&dwBytesRead);
		VirtualProtectEx(
			hProc,
			(VOID*)ImageBase,
			SizeOfImage,
			dwOldProt,
			&dwNewProt);
		if (!bResult)
		{
			CloseHandle(hProc);
			GlobalFree(pMem);
			return FALSE;
		}
		ofn.lpstrFilter = "ExE files\0*.exe\0All files\0*.*\0\0";
		strcpy(szFilePath,"dumped.exe");
	}
	else
	{
		// get VA and size
		ZeroMemory(&szDumpData,sizeof(szDumpData));
		GetDlgItemText(hDumpDlg,IDC_VA,(PSTR)szDumpData,sizeof(szDumpData));
		Hexstr2Int((PSTR)szDumpData,dwDumpStart);
		ZeroMemory(&szDumpData,sizeof(szDumpData));		
		GetDlgItemText(hDumpDlg,IDC_DUMPSIZE,(PSTR)szDumpData,sizeof(szDumpData));
		Hexstr2Int((PSTR)szDumpData,dwDumpSize);

		// dump process PARTIAL
		pMem = GlobalAlloc(GMEM_FIXED,dwDumpSize);
		if (!pMem)
		{
			CloseHandle(hProc);
			return FALSE;
		}
		VirtualProtectEx(
			hProc,
			(VOID*)dwDumpStart,
			dwDumpSize,
			PAGE_READONLY,
			&dwOldProt);
		bResult = ReadProcessMemory(
			hProc,
			(VOID*)dwDumpStart,
			pMem,
			dwDumpSize,
			&dwBytesRead);
		VirtualProtectEx(
			hProc,
			(VOID*)ImageBase,
			SizeOfImage,
			dwOldProt,
			&dwNewProt);
		if (!bResult)
		{
			CloseHandle(hProc);
			GlobalFree(pMem);
			return FALSE;
		}
		ofn.lpstrFilter = "All files\0*.*\0\0";
		strcpy(szFilePath,"dumped.bin");
	}

	// let the user choose a file path and write the proces memory into it
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.hwndOwner = hDumpDlg;
	ofn.lpstrFile = (CHAR*)&szFilePath;
	ofn.nMaxFile = sizeof(szFilePath);
	ofn.lpstrInitialDir = ".";
	ofn.lpstrTitle = "Save process memory to...";
	if (!GetSaveFileName(&ofn))
	{
		CloseHandle(hProc);
		GlobalFree(pMem);
		return FALSE;
	}
	hFile = CreateFile (szFilePath, GENERIC_READ | GENERIC_WRITE, \
		FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (!hFile)
	{
		CloseHandle(hProc);
		GlobalFree(pMem);
		return FALSE;
	}
	WriteFile(hFile,pMem,dwDumpSize,&dwBytesWritten,NULL);
	CloseHandle(hFile);

	// clean up
	CloseHandle(hProc);
	GlobalFree(pMem);
	return TRUE;;
}

BOOL FixSections(CHAR* szTargetFname)
{
	VOID*                    pMap;
	PIMAGE_DOS_HEADER        pDosh;
	PIMAGE_NT_HEADERS        pPeh;
	PIMAGE_SECTION_HEADER    pSech;
	int                      i;

	// map the file
	if (!(pMap = MapFileRW(szTargetFname)))
		return FALSE;
	if (!IsPE(pMap))
	{
		UnmapViewOfFile(pMap);
		return FALSE;
	}
	// start the correction
	pDosh = (PIMAGE_DOS_HEADER)(pMap);
	pPeh = (PIMAGE_NT_HEADERS)((DWORD)pDosh + pDosh->e_lfanew);
	pSech = (PIMAGE_SECTION_HEADER)((DWORD)pDosh + pDosh->e_lfanew + 0xF8);
	i = pPeh->FileHeader.NumberOfSections;
	do
	{
		pSech->PointerToRawData = pSech->VirtualAddress;
		pSech->SizeOfRawData = pSech->Misc.VirtualSize;
		++pSech;
		--i;
	} while (i != 0);
	// clean up
	UnmapViewOfFile(pMap);
	return TRUE;
}

DWORD Rva2Offset(PIMAGE_NT_HEADERS pNT,VOID* Base,DWORD dwTargetRVA)
{
	PIMAGE_SECTION_HEADER pSech;

	pSech = ImageRvaToSection(pNT,Base,dwTargetRVA);
	if (!pSech)
		return 0;
	return (dwTargetRVA - pSech->VirtualAddress + pSech->PointerToRawData);
}

BOOL PasteOrgIT(CHAR* szOrgITPE,CHAR* szTargetPE)
{
	VOID                      *pOrgMem,*pTargetMem;
	PIMAGE_DOS_HEADER         pDosh;
	PIMAGE_NT_HEADERS         pPeh;
	PIMAGE_SECTION_HEADER     pSech;
	PIMAGE_IMPORT_DESCRIPTOR  pOrgIID,pTarIID;
	DWORD                     *pdwOrg,*pdwTar;

	// map the files and fill some PE structs
	pOrgMem = MapFileR(szOrgITPE);
	if (!pOrgMem)
		return FALSE;
	pTargetMem = MapFileRW(szTargetPE);
	if (!pTargetMem)
	{
		UnmapViewOfFile(pOrgMem);
		return FALSE;
	}
	__try
	{
		if (!IsPE(pTargetMem))
		{
			UnmapViewOfFile(pOrgMem);
			UnmapViewOfFile(pTargetMem);
			return FALSE;
		}
		pDosh = (PIMAGE_DOS_HEADER)(pOrgMem);
		pPeh  = (PIMAGE_NT_HEADERS)((DWORD)pDosh + pDosh->e_lfanew);
		pSech = (PIMAGE_SECTION_HEADER)((DWORD)pPeh + 0xF8);
		pOrgIID = (PIMAGE_IMPORT_DESCRIPTOR)(
			(DWORD)pOrgMem + 
			Rva2Offset(
			pPeh,
			pDosh,
			pPeh->OptionalHeader.DataDirectory[1].VirtualAddress));
		pTarIID = (PIMAGE_IMPORT_DESCRIPTOR)(
			(DWORD)pTargetMem + 
			pPeh->OptionalHeader.DataDirectory[1].VirtualAddress);

		// START THE FIX
		while(pOrgIID->FirstThunk)
		{
			pdwOrg = (DWORD*)((DWORD)pOrgMem + Rva2Offset(pPeh,pDosh,pOrgIID->FirstThunk));
			pdwTar = (DWORD*)((DWORD)pTargetMem + pTarIID->FirstThunk);
			pTarIID->ForwarderChain = 0; // This is need for W9X ! The PE loader wouldn't
			pTarIID->TimeDateStamp = 0;  // initialize the Import Table without it.
			while(*pdwTar)
			{
				*pdwTar = *pdwOrg;
				++pdwTar;
				++pdwOrg;
			}
			++pOrgIID;
			++pTarIID;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		UnmapViewOfFile(pOrgMem);
		UnmapViewOfFile(pTargetMem);
		return FALSE;
	}
	// clean up
	UnmapViewOfFile(pOrgMem);
	UnmapViewOfFile(pTargetMem);
	return TRUE;
}