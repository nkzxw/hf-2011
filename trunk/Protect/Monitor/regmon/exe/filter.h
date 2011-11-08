/******************************************************************************
*
* Regmon - Registry Monitor for Windows 95/98/Me/NT/2K/XP/IA64 
*		
* Copyright (c) 1996-2002 Mark Russinovich and Bryce Cogswell
* See readme.txt for terms and conditions.
*
* Displays Registry activity in real-time.
*
******************************************************************************/

extern DWORD			MaxLines;
extern DWORD			LastRow;

extern DWORD			HighlightFg;
extern DWORD			HighlightBg;

extern FILTER			FilterDefinition;

extern TCHAR			FilterString[MAXFILTERLEN];
extern TCHAR			ExcludeString[MAXFILTERLEN];
extern TCHAR			HighlightString[MAXFILTERLEN];


extern char				RecentInFilters[NUMRECENTFILTERS][MAXFILTERLEN];
extern char				RecentExFilters[NUMRECENTFILTERS][MAXFILTERLEN];
extern char				RecentHiFilters[NUMRECENTFILTERS][MAXFILTERLEN];

BOOLEAN MatchWithHighlightPattern( PCHAR String );