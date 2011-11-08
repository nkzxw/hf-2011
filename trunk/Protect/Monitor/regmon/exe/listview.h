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

extern HWND					hWndList;
extern HFONT				hFont;
extern LOGFONT				LogFont;
extern HWND					hBalloon;
extern HWND					hWndFind;

extern HCURSOR 				hSaveCursor;
extern HCURSOR 				hHourGlass;

extern FINDREPLACE			FindTextInfo;
extern DWORD				FindFlags;
extern BOOLEAN				PrevMatch;



HWND CreateList( HWND hWndParent );
BOOLEAN FindInListview(HWND hWnd, LPFINDREPLACE FindInfo );
void DeleteSelection( HWND hWnd );
void CopySelection( HWND hWnd );
void SaveFile( HWND hWnd, HWND ListBox, BOOLEAN SaveAs );
void DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem);
LRESULT APIENTRY BalloonDialog( HWND hDlg, UINT message, UINT wParam, LPARAM lParam );
void PopFindDialog(HWND hWnd);
VOID SelectHighlightColors( HWND hWnd );
BOOL APIENTRY HistoryProc( HWND hDlg, UINT message, UINT wParam, LONG lParam );
BOOL APIENTRY FilterProc( HWND hDlg, UINT message, UINT wParam, LONG lParam );
