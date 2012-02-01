
#include <windows.h>

#ifndef __SearchMenu
#   define __SearchMenu

#   define MAX_SEARCHSTRINGLENGTH 150

    VOID CreateSearchDlg(HINSTANCE hInst,HANDLE hDlgOwner);
	BOOL DoListSearch(HWND hTarLB);

    typedef struct
	{
		CHAR  SearchBuff[MAX_SEARCHSTRINGLENGTH];
		BOOL  CaseSensitive;
		BOOL  SearchInWholeList;
	} sSearchOptions;

	extern sSearchOptions  SearchOptions;

#   endif