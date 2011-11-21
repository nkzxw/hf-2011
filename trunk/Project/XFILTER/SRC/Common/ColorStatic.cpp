//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
// ColorStatic.cpp

#include "stdafx.h"
#include "ColorStatic.h"

CColorStatic::CColorStatic(COLORREF nColor)
{
	m_nColor = nColor;
	m_bIsTransParent = TRUE;
	m_nBkColor = PASSECK_DIALOG_BKCOLOR;
}

BEGIN_MESSAGE_MAP(CColorStatic, CStatic)
    //{{AFX_MSG_MAP(CHyperLink)
    ON_WM_CTLCOLOR_REFLECT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CColorStatic::SetColor(COLORREF nColor)
{
	m_nColor = nColor;
}

void CColorStatic::SetLabel(UINT nID, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);
	m_bIsTransParent = FALSE;
	m_nBkColor = COLOR_TORJAN_BK;
	SetLogoFont(DEFAULT_FONT, DEFAULT_HEIGHT);
	SetColor(COLOR_TORJAN_TEXT);
}

void CColorStatic::SetLabelEx(UINT nID, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);
	m_bIsTransParent = FALSE;
	m_nBkColor = COLOR_TORJAN_BK;
	SetLogoFont(DEFAULT_FONT, DEFAULT_HEIGHT);
	SetColor(~COLOR_TORJAN_BK);
}

void CColorStatic::SetLabelQuery(UINT nID, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);
	m_bIsTransParent = FALSE;
	m_nBkColor = PASSECK_DIALOG_BKCOLOR;
	SetLogoFont(DEFAULT_FONT, DEFAULT_HEIGHT);
	SetColor(COLOR_TEXT_NORMAL);
}

void CColorStatic::SetLogoFont(
	CString			Name,
	int				nHeight,
	int				nWeight,
	BYTE			bItalic,
	BYTE			bUnderline,
	BOOL			bIsChinese
)
{
	if(m_Font.m_hObject)
		m_Font.Detach();

	if(bIsChinese)
	{
		LOGFONT LogFont ;
		CFont *pFont = GetParent()->GetFont();
		pFont->GetLogFont(&LogFont);
		LogFont.lfHeight = nHeight;
		LogFont.lfWeight = nWeight;
		LogFont.lfItalic = bItalic;
		LogFont.lfUnderline = bUnderline;
		_tcscpy(LogFont.lfFaceName, Name);
		m_Font.CreateFontIndirect(&LogFont);
	}
	else
	{
		m_Font.CreateFont(nHeight,0,0,0,nWeight,bItalic,bUnderline,0,0,0,0,0,0,Name);
	}
}

HBRUSH CColorStatic::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	//return CStatic::OnCtlColor(pDC, this->GetParent(), nCtlColor);

    ASSERT(nCtlColor == CTLCOLOR_STATIC);
	
	if(m_bIsTransParent)
	{
		pDC->SetBkMode(TRANSPARENT);
	}
	else
	{
		pDC->SetBkColor(m_nBkColor);
	}

    pDC->SetTextColor(m_nColor);
	pDC->SelectObject(&m_Font);

	return (HBRUSH)GetStockObject(NULL_BRUSH);
}


BEGIN_MESSAGE_MAP(CBkStatic, CStatic)
    //{{AFX_MSG_MAP(CBkStatic)
	ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CBkStatic::CBkStatic(COLORREF m_nBkColor)
{
	m_nBkColor = m_nBkColor;
}

void CBkStatic::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(m_nBkColor);
	dc.FillRect(&rect, &brush);
}

void CBkStatic::SetBkColor(COLORREF nBkColor)
{
	m_nBkColor = nBkColor;
	Invalidate();
	//OnPaint();
}

COsilloGraph::COsilloGraph()
{
	m_MaxPos = DEFAULT_MAX_POS;
	m_PosUnit = DEFAULT_POS_UNIT;
	m_nColorLine = COLOR_GRENN;
	m_nColorGrid = COLOR_WHITE;
	m_ShowPos = 0;
	m_GridUnit = DEFAULT_GRID_UNIT;
	m_MoveUnit = 3;//DEFAULT_MOVE_UNIT;
	m_CurrentPos = 0;

	m_ThreadHandle = NULL;
}

COsilloGraph::~COsilloGraph()
{
	Stop();
}

BEGIN_MESSAGE_MAP(COsilloGraph, CStatic)
    //{{AFX_MSG_MAP(COsilloGraph)
	ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL	m_bIsPaint = FALSE;
CBrush	m_GraphBk(PASSECK_DIALOG_BKCOLOR);
CRect	m_GraphRect(0, 0, 0, 0);

void COsilloGraph::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	DrawGrid();
	//DrawPos();

	dc.BitBlt(0, 0, m_Width, m_Height, &m_MemDC, 0, 0, SRCCOPY);
}

void COsilloGraph::DrawGrid()
{
}

void COsilloGraph::DrawPos()
{
	CDC *pDC = &m_MemDC;
	if(m_bIsPaint)
	{
		pDC->BitBlt(0, 0, m_Width - m_MoveUnit, m_Height, pDC, m_MoveUnit, 0, SRCCOPY);
		pDC->FillRect(&m_GraphRect, &m_GraphBk);

		static int x = m_Width - 1;
		static int y = m_Height - 1;
		static int cury;

		if(m_ShowPos > 0 && m_ShowPos < m_Unit)
			m_ShowPos = m_Unit;
		cury = m_Height - (m_ShowPos / m_Unit) - 1;
		if(cury < 0) cury = 0;

		pDC->MoveTo(x - m_MoveUnit, y);
		pDC->LineTo(x , cury);

		m_ShowPos = 0;
		y = cury;
		
		m_bIsPaint = FALSE;
	}

}

void COsilloGraph::SetCurrentPos(int CurrentPos)
{
	if(m_CurrentPos == CurrentPos)
		return;
	m_ShowPos += (CurrentPos - m_CurrentPos);
	m_CurrentPos = CurrentPos;
}

BOOL m_bGraphThread = FALSE;
BOOL m_bInGraphThread = FALSE;

void COsilloGraph::Startup()
{
	GetClientRect(&m_Rect);
	m_Height = m_Rect.Height();
	m_Width = m_Rect.Width();

	m_Unit = m_MaxPos / m_Height;

	m_GraphRect.bottom = m_Rect.bottom;
	m_GraphRect.top = m_Rect.top;
	m_GraphRect.left = m_Width - m_MoveUnit;
	m_GraphRect.right = m_Rect.right;

	CWindowDC hWndDC(this);
	m_MemDC.CreateCompatibleDC(&hWndDC);
	m_Bitmap.CreateCompatibleBitmap(&hWndDC, m_Width, m_Height);
	m_MemDC.SelectObject(&m_Bitmap);
	m_MemDC.FillRect(&m_Rect, &m_GraphBk);
	m_penLine.CreatePen(PS_SOLID, 0, m_nColorLine);
	m_MemDC.SelectObject(&m_penLine);

	m_bGraphThread = TRUE;
	DWORD dwThreadId;
	m_ThreadHandle = CreateThread(0, 0, RefreshGraph, this, 0, &dwThreadId);
}


void COsilloGraph::Stop()
{
	if(m_bInGraphThread)
	{
		m_bGraphThread = FALSE;
		while(m_bInGraphThread)Sleep(50);
	}
}

DWORD WINAPI RefreshGraph(PVOID pVoid)
{
	int nCount;
	COsilloGraph* pGraph = (COsilloGraph*)pVoid;
	if(pGraph == NULL)
		return 0 ;
	m_bInGraphThread = TRUE;
	while(m_bGraphThread)
	{
		m_bIsPaint = TRUE;
		//pGraph->PostMessage(WM_PAINT, 0, 0);
		pGraph->DrawPos();
		pGraph->Invalidate(FALSE);
		nCount = 20;
		while(m_bGraphThread && nCount-- > 0)Sleep(50);
	}
	m_bInGraphThread = FALSE;
	return 0;
}



#pragma comment( exestr, "B9D3B8FD2A65716E7174757663766B652B")
