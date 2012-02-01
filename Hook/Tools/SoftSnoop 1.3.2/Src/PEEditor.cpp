
#include "PEEditor.h"

// function protos
BOOL CreatePEEditorDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL CALLBACK PEEditorDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL ShowPEHeaderInfo(HWND hDlg, void* pMap);
BOOL CALLBACK SecTableDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL ShowSectionTable(HWND hDlg, void* pMap);
BOOL CALLBACK ImportDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL ShowImportInfo(HWND hDlg, void* pMap);

// constants
#define SECTION_VIEWER_COLUMN_SIZE 76

// global variables
HWND       hPEEditor;
VOID*      pPEImage;


BOOL CreatePEEditorDlg(HINSTANCE hInst,HANDLE hDlgOwner)
{
	DialogBoxParam(
		hInst,
		MAKEINTRESOURCE(IDD_PEEDITOR),
		(HWND)hDlgOwner,
		PEEditorDlgProc,
		0);
	return TRUE;
}

BOOL CALLBACK PEEditorDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHAR cTitleBuff[100];

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hPEEditor = hDlg;

		// target filename -> title
		GetWindowText(hDlg, cTitleBuff, sizeof(cTitleBuff));
		lstrcat(cTitleBuff, "[ ");
		lstrcat(
			cTitleBuff,
			ExtractFileName(szFname));
		lstrcat(cTitleBuff, " ]");
		SetWindowText(hDlg, cTitleBuff);

		// map the file and show infos
		pPEImage = MapFileR(szFname);
		if (!pPEImage || !ShowPEHeaderInfo(hDlg, pPEImage))
		{
			ShowError("Couldn't map file or invalid PE file !");
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_PE_OK:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;

		case ID_SECTIONTABLE:
			DialogBoxParam(
				hInst,
				MAKEINTRESOURCE(IDD_SECTIONTABLE),
				hDlg,
				SecTableDlgProc,
				0);
			return TRUE;

		case ID_IMPORTTABLE:
			DialogBoxParam(
				hInst,
				MAKEINTRESOURCE(IDD_IMPORTS),
				hDlg,
				ImportDlgProc,
				0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		// clean up
		UnmapViewOfFile(pPEImage);

		// close dlg
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

BOOL ShowPEHeaderInfo(HWND hDlg, void* pMap)
{
	PIMAGE_DOS_HEADER    pDosh;
	PIMAGE_NT_HEADERS    pNT;
	CHAR                 cEditBuff[10];

	if (!IsPE(pMap))
		return FALSE;

	// get PE header and show infos
	pDosh = (PIMAGE_DOS_HEADER)pMap;
	pNT   = (PIMAGE_NT_HEADERS)((DWORD)pMap + pDosh->e_lfanew);

	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.AddressOfEntryPoint);
	SetDlgItemText(hDlg, IDC_PE_ENTRYPOINT, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.ImageBase);
	SetDlgItemText(hDlg, IDC_PE_IMAGEBASE, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.SizeOfImage);
	SetDlgItemText(hDlg, IDC_PE_SIZEOFIMAGE, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.SectionAlignment);
	SetDlgItemText(hDlg, IDC_PE_SECTIONALIGN, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.FileAlignment);
	SetDlgItemText(hDlg, IDC_PE_FILEALIGN, cEditBuff);
	wsprintf(cEditBuff, "%04lX", pNT->OptionalHeader.Subsystem);
	SetDlgItemText(hDlg, IDC_PE_SUBSYSTEM, cEditBuff);
	wsprintf(cEditBuff, "%04lX", pNT->FileHeader.NumberOfSections);
	SetDlgItemText(hDlg, IDC_PE_NUMBEROFSECTIONS, cEditBuff);
	wsprintf(cEditBuff, "%04lX", pNT->FileHeader.Characteristics);
	SetDlgItemText(hDlg, IDC_PE_CHARAC, cEditBuff);

	// show data directory infos
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.DataDirectory[1].VirtualAddress);
	SetDlgItemText(hDlg, IDC_PE_IMPORTTABLE, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.DataDirectory[0].VirtualAddress);
	SetDlgItemText(hDlg, IDC_PE_EXPORTTABLE, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.DataDirectory[2].VirtualAddress);
	SetDlgItemText(hDlg, IDC_PE_RESOURCES, cEditBuff);
	wsprintf(cEditBuff, "%08lX", pNT->OptionalHeader.DataDirectory[5].VirtualAddress);
	SetDlgItemText(hDlg, IDC_PE_RELOCATION, cEditBuff);
	return TRUE;
}

BOOL CALLBACK SecTableDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		if (!ShowSectionTable(hDlg, pPEImage))
		{
			MessageBox(hPEEditor, "Couldn't map file :(", NULL, MB_ICONERROR);
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

BOOL ShowSectionTable(HWND hDlg, void* pMap)
{
	PIMAGE_DOS_HEADER       pDosh;
	PIMAGE_NT_HEADERS       pNT;
	PIMAGE_SECTION_HEADER   pSec;
	LVCOLUMN                Col;
	LVITEM                  lvItem;
	char                    cSecName[9], cEditBuff[9];
	WORD                    i;

	// create columns
	memset(&Col, 0, sizeof(Col));
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "Name";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 0, (LPARAM)&Col);
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "VAddress";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 1, (LPARAM)&Col);
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "VSize";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 2, (LPARAM)&Col);
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "ROffset";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 3, (LPARAM)&Col);
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "RSize";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 4, (LPARAM)&Col);
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = SECTION_VIEWER_COLUMN_SIZE;
	Col.pszText   = "Flags";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 5, (LPARAM)&Col);	

	// get some addresses
	pDosh = (PIMAGE_DOS_HEADER)pMap;
	pNT   = (PIMAGE_NT_HEADERS)((DWORD)pMap + pDosh->e_lfanew);
	pSec  = (PIMAGE_SECTION_HEADER)((DWORD)pNT + 0xF8);

	// show section table
	for (i=0; i < pNT->FileHeader.NumberOfSections; i++)
	{
		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask    = LVIF_TEXT;
		lvItem.iItem   = i;

		// section name
		memset(&cSecName, 0, sizeof(cSecName));
		memcpy(&cSecName, pSec->Name, 8);
		lvItem.pszText = cSecName;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

		// VA
		lvItem.pszText  = cEditBuff;
		wsprintf(cEditBuff, "%08lX", pSec->VirtualAddress);
		lvItem.iSubItem = 1;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_SETITEM, 0, (LPARAM)&lvItem);

		// VS
		wsprintf(cEditBuff, "%08lX", pSec->Misc.VirtualSize);
		lvItem.iSubItem = 2;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_SETITEM, 0, (LPARAM)&lvItem);

		// RO
		wsprintf(cEditBuff, "%08lX", pSec->PointerToRawData);
		lvItem.iSubItem = 3;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_SETITEM, 0, (LPARAM)&lvItem);

		// RS
		wsprintf(cEditBuff, "%08lX", pSec->SizeOfRawData);
		lvItem.iSubItem = 4;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_SETITEM, 0, (LPARAM)&lvItem);

		// Characteristics
		wsprintf(cEditBuff, "%08lX", pSec->Characteristics);
		lvItem.iSubItem = 5;
		SDIM(hDlg, IDC_SECTIONLIST, LVM_SETITEM, 0, (LPARAM)&lvItem);

		++pSec;
	}

	return TRUE;
}

BOOL CALLBACK ImportDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		if (!ShowImportInfo(hDlg, pPEImage))
		{
			ShowError("Couldn't grab Import Information :(");
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	}
	return FALSE;
}

BOOL ShowImportInfo(HWND hDlg, void* pMap)
{
	PIMAGE_DOS_HEADER         pDos;
	PIMAGE_NT_HEADERS         pNT;
	PIMAGE_IMPORT_DESCRIPTOR  pIID;
	TV_INSERTSTRUCT           TreeIns;
	HTREEITEM                 htiCurrDll;
	DWORD                     *pDW;
	CHAR                      cBuff[MAX_PATH];

	pDos = (PIMAGE_DOS_HEADER)pMap;
	pNT = (PIMAGE_NT_HEADERS)((DWORD)pMap + pDos->e_lfanew);

	// get the offset to the IT
	pIID = (PIMAGE_IMPORT_DESCRIPTOR)ImageRvaToVa(
		pNT,
		pMap,
		pNT->OptionalHeader.DataDirectory[1].VirtualAddress,
		NULL);
	if (!pIID)
		return FALSE;

	// trace the IT
	while (pIID->Name)
	{
		// show the dll name
		memset(&TreeIns, 0, sizeof(TreeIns));
		TreeIns.item.mask    = TVIF_TEXT;
		TreeIns.item.pszText = (CHAR*)ImageRvaToVa(
			pNT,
			pMap,
			pIID->Name,
			NULL);
		if (!TreeIns.item.pszText)
			return FALSE;
		htiCurrDll = (HTREEITEM)SDIM(
			hDlg,
			IDC_IMPTREE,
			TVM_INSERTITEM,
			NULL,
			(LPARAM)&TreeIns);

		// update tree add struct
		TreeIns.item.mask   = TVIF_TEXT;
		TreeIns.hParent     = htiCurrDll;

		// show the dll function names/ordinals
		__try
		{
			pDW = (DWORD*)pIID->OriginalFirstThunk;
			if (!pDW)
				pDW = (DWORD*)pIID->FirstThunk;
			pDW = (DWORD*)ImageRvaToVa(
				pNT,
				pMap,
				(DWORD)pDW,
				NULL);
			while (*pDW)
			{
				//process the current thunk
				if (*pDW < IMAGE_ORDINAL_FLAG)
				{
					// name import
					wsprintf(
						cBuff,
						"%s Ordinal: %Xh",
						(CHAR*)ImageRvaToVa(
						pNT,
						pMap,
						*pDW+2,
						NULL),
						pIID->OriginalFirstThunk);
					TreeIns.item.pszText = cBuff;
					SDIM(hDlg, IDC_IMPTREE, TVM_INSERTITEM, NULL, (LPARAM)&TreeIns);
				}
				else if (*pDW >= IMAGE_ORDINAL_FLAG && *pDW < IMAGE_ORDINAL_FLAG+0x10000)
				{
					// ordinal import
					wsprintf(
						cBuff,
						"Ordinal: %Xh",
						*pDW - IMAGE_ORDINAL_FLAG);
					TreeIns.item.pszText = cBuff;
					SDIM(hDlg, IDC_IMPTREE, TVM_INSERTITEM, NULL, (LPARAM)&TreeIns);
				}
				else
				{
					// memory address
					wsprintf(
						cBuff,
						"Memory Address: %Xh",
						*pDW);
					TreeIns.item.pszText = cBuff;
					SDIM(hDlg, IDC_IMPTREE, TVM_INSERTITEM, NULL, (LPARAM)&TreeIns);
				}
				++pDW;
			}
		}
		__except(1)
		{
			TreeIns.item.pszText = "Access violation :(";
			SDIM(hDlg, IDC_IMPTREE, TVM_INSERTITEM, NULL, (LPARAM)&TreeIns);
		}
		++pIID;
	}
	return TRUE;
}