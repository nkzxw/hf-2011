// PasseckDialog.cpp

#include "stdafx.h"
#include "PasseckDialog.h"

void CPasseckDialog::AddDialog(CDialog* pDialog, UINT nID, LPCTSTR lpszWindowName)
{
	CRect rect(0, 0, 231, 304);
	CreateEx(	0,
				AfxRegisterWndClass(0),
				lpszWindowName,
				WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
				rect,
				NULL,
				NULL,
				NULL );

	pDialog->Create(nID, this);
	pDialog->ShowWindow(SW_SHOW);
	pDialog->UpdateWindow();
	pDialog->GetWindowRect(&rect);
	rect.bottom -= 2;
	rect.right -= 2;

	MoveWindow(&rect, TRUE);
	CenterWindow();

	ShowWindow(SW_SHOW);
	UpdateWindow();
}

#pragma comment( exestr, "B9D3B8FD2A657263757567656D666B636E71692B")
