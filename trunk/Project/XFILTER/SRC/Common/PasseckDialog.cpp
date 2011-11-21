//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
// PasseckDialog.cpp

#include "stdafx.h"
#include "PasseckDialog.h"

CPasseckDialog::CPasseckDialog(UINT nID, CWnd* pParent /*=NULL*/, LPCTSTR lpszWindowName)
	: CDialog(nID, pParent)
{
	//{{AFX_DATA_INIT(CMainDlg)
	//}}AFX_DATA_INIT

	m_bHaveHead = TRUE;
	m_sCaption = lpszWindowName;
	m_bMoveParent = TRUE;
}

BEGIN_MESSAGE_MAP(CPasseckDialog, CDialog)
	//{{AFX_MSG_MAP(CPasseckDialog)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPasseckDialog::OnOK() 
{
}

void CPasseckDialog::OnCancel() 
{
}


BOOL CPasseckDialog::OnInitDialog() 
{
	BOOL bRet = CDialog::OnInitDialog();
	return bRet;
}

void CPasseckDialog::OnLButtonDown(UINT nFlags, CPoint point) 
{	
	CDialog::OnLButtonDown(nFlags, point);

	if(point.y < 24)
	{
		if(m_bMoveParent)
			GetParent()->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
		else
			PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
	}
}

void CPasseckDialog::CreateCaption(LPCTSTR lpszWindowName)
{
	if(lpszWindowName == NULL) return;

	if(TRUE || lpszWindowName[0] < 0 && m_bHaveHead)
	{
		m_StaticCaption2.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(10, 8, 360, 24), this);
		m_StaticCaption2.SetColor(RGB(0, 0, 0));
		m_StaticCaption2.SetLogoFont(CHINESE_FONT_DEFAULT, DEFAULT_HEIGHT, FW_BOLD, false);

		m_StaticCaption.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(9, 7, 360, 24), this);
		m_StaticCaption.SetColor(RGB(255, 255, 255));
		m_StaticCaption.SetLogoFont(CHINESE_FONT_DEFAULT, DEFAULT_HEIGHT, FW_BOLD, false);
	}
	else
	{
		m_StaticCaption2.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(8, 4, 360, 18), this);
		m_StaticCaption2.SetColor(RGB(0, 0, 0));
		m_StaticCaption2.SetLogoFont(ENGLISH_FONT_DEFUALT, 18, FW_NORMAL, false, false, FALSE);

		m_StaticCaption.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(6, 2, 360, 18), this);
		m_StaticCaption.SetColor(RGB(255, 255, 255));
		m_StaticCaption.SetLogoFont(ENGLISH_FONT_DEFUALT, 18, FW_NORMAL, false, false, FALSE);
	}
}

void CPasseckDialog::CreateCaptionEx(
	LPCTSTR lpszWindowName,
	LPCTSTR lpszFont,
	COLORREF nFkColor,
	int nHeight,
	int nWeight,
	BYTE bItalic,
	BYTE bUnderline
)
{
	if(lpszWindowName != NULL)
	{
		m_StaticCaption2.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(2, 4, 200, 18), this, 0xFFFF);
		m_StaticCaption2.SetColor(COLOR_SHADOW);
		m_StaticCaption2.SetLogoFont(lpszFont, nHeight, nWeight, bItalic, bUnderline);

		m_StaticCaption.Create(lpszWindowName, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(1, 3, 200, 18), this, 0xFFFF);
		m_StaticCaption.SetColor(nFkColor);
		m_StaticCaption.SetLogoFont(lpszFont, nHeight, nWeight, bItalic, bUnderline);
	}
}

void CPasseckDialog::SetWindowCaption(LPCTSTR lpszWindowName)
{
	m_StaticCaption2.ShowWindow(SW_HIDE);
	m_StaticCaption.ShowWindow(SW_HIDE);
	m_StaticCaption2.SetWindowText(lpszWindowName);
	m_StaticCaption.SetWindowText(lpszWindowName);
	m_StaticCaption2.ShowWindow(SW_SHOW);
	m_StaticCaption.ShowWindow(SW_SHOW);
}

void CPasseckFrame::AddDialog(CPasseckDialog* pDialog, UINT nID, LPCTSTR lpszWindowName)
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
				
	SetWindowText(lpszWindowName);

	pDialog->Create(nID, this);

	pDialog->CreateCaption(lpszWindowName);

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


#pragma comment( exestr, "B9D3B8FD2A7263757567656D666B636E71692B")
