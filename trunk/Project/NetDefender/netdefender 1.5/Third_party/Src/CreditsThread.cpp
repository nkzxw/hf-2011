//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "CreditsThread.h"
//#include "OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// define mask color
#define MASK_RGB	(COLORREF)0xFFFFFF

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCreditsThread, CGDIThread)

BEGIN_MESSAGE_MAP(CCreditsThread, CGDIThread)
	//{{AFX_MSG_MAP(CCreditsThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCreditsThread::CCreditsThread(CWnd* pWnd, HDC hDC, CRect rectScreen)
    : CGDIThread(pWnd,hDC)
{
	m_rectScreen = rectScreen;
	m_rgnScreen.CreateRectRgnIndirect(m_rectScreen);
	m_nScrollPos = 0;
	m_pbmpOldBk = NULL;
	m_pbmpOldCredits = NULL;
	m_pbmpOldScreen = NULL;
	m_pbmpOldMask = NULL;
	m_nCreditsBmpWidth = 0;
	m_nCreditsBmpHeight = 0;
}

CCreditsThread::~CCreditsThread() 
{
}

BOOL CCreditsThread::InitInstance()
{
	BOOL bResult = CGDIThread::InitInstance();

	// NOTE: Because this is a separate thread, we have to delete our GDI objects here (while
	// the handle maps are still available.)
	if(m_dcBk.m_hDC != NULL && m_pbmpOldBk != NULL)
	{
		m_dcBk.SelectObject(m_pbmpOldBk);
		m_pbmpOldBk = NULL;
		m_bmpBk.DeleteObject();
	}

	if(m_dcScreen.m_hDC != NULL && m_pbmpOldScreen != NULL)
	{
		m_dcScreen.SelectObject(m_pbmpOldScreen);
		m_pbmpOldScreen = NULL;
		m_bmpScreen.DeleteObject();
	}

	if(m_dcCredits.m_hDC != NULL && m_pbmpOldCredits != NULL)
	{
		m_dcCredits.SelectObject(m_pbmpOldCredits);
		m_pbmpOldCredits = NULL;
		m_bmpCredits.DeleteObject();
	}

	if(m_dcMask.m_hDC != NULL && m_pbmpOldMask != NULL)
	{
		m_dcMask.SelectObject(m_pbmpOldMask);
		m_pbmpOldMask = NULL;
		m_bmpMask.DeleteObject();
	}

	// clean up the fonts we created
	for(int n = 0; n < m_arFonts.GetSize(); n++)
	{
		m_arFonts.GetAt(n)->DeleteObject();
		delete m_arFonts.GetAt(n);
	}
	m_arFonts.RemoveAll();

	return bResult;
}

// wait for vertical retrace
// makes scrolling smoother, especially at fast speeds
// NT does not like this at all
void waitvrt(void)
{
	__asm {
			mov	dx,3dah
	VRT:
			in		al,dx
			test	al,8
			jnz		VRT
	NoVRT:
			in		al,dx
			test	al,8
			jz		NoVRT
	}
}

void CCreditsThread::SingleStep()
{
	// if this is our first time, initialize the credits
	if(m_dcCredits.m_hDC == NULL)
	{
		CreateCredits();
	}

	// track scroll position
	static int nScrollY = 0;

	// timer variables
	LARGE_INTEGER nFrequency;
	LARGE_INTEGER nStart;
	LARGE_INTEGER nEnd;
	int nTimeInMilliseconds;
	BOOL bTimerValid;

	nStart.QuadPart = 0;

	if(!QueryPerformanceFrequency(&nFrequency))
	{
		bTimerValid = FALSE;
	}
	else
	{
		bTimerValid = TRUE;

		// get start time
		QueryPerformanceCounter(&nStart);
	}

	CGDIThread::m_csGDILock.Lock();
	{
		PaintBk(&m_dcScreen);

		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, nScrollY, SRCINVERT);
		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcMask, 0, nScrollY, SRCAND);
		m_dcScreen.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, nScrollY, SRCINVERT);

		// wait for vertical retrace
		if(m_bWaitVRT) waitvrt();

		m_dc.BitBlt(m_rectScreen.left, m_rectScreen.top, m_rectScreen.Width(), m_rectScreen.Height(), &m_dcScreen, 0, 0, SRCCOPY);

		GdiFlush();
	}
	CGDIThread::m_csGDILock.Unlock();

	// continue scrolling
	nScrollY += m_nScrollInc;
	if(nScrollY >= m_nCreditsBmpHeight) nScrollY = 0;	// scrolling up
	if(nScrollY < 0) nScrollY = m_nCreditsBmpHeight;	// scrolling down

	// delay scrolling by the specified time
	if(bTimerValid)
	{
		QueryPerformanceCounter(&nEnd);
		nTimeInMilliseconds = (int)((nEnd.QuadPart - nStart.QuadPart) * 1000 / nFrequency.QuadPart);

		if(nTimeInMilliseconds < m_nDelay)
		{
			Sleep(m_nDelay - nTimeInMilliseconds);
		}
	}
	else
	{
		Sleep(m_nDelay);
	}
}

void CCreditsThread::PaintBk(CDC* pDC)
{
	//save background the first time
	if (m_dcBk.m_hDC == NULL)
	{
		m_dcBk.CreateCompatibleDC(&m_dc);
		m_bmpBk.CreateCompatibleBitmap(&m_dc, m_rectScreen.Width(), m_rectScreen.Height());
		m_pbmpOldBk = m_dcBk.SelectObject(&m_bmpBk);
		m_dcBk.BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &m_dc, m_rectScreen.left, m_rectScreen.top, SRCCOPY);
	}

	pDC->BitBlt(0, 0, m_rectScreen.Width(), m_rectScreen.Height(), &m_dcBk, 0, 0, SRCCOPY);
}

void CCreditsThread::CreateCredits()
{
	InitFonts();
	InitColors();
	InitText();

	m_dc.SelectClipRgn(&m_rgnScreen);

	m_dcScreen.CreateCompatibleDC(&m_dc);
	m_bmpScreen.CreateCompatibleBitmap(&m_dc, m_rectScreen.Width(), m_rectScreen.Height());
	m_pbmpOldScreen = m_dcScreen.SelectObject(&m_bmpScreen);

	m_nCreditsBmpWidth = m_rectScreen.Width();
	m_nCreditsBmpHeight = CalcCreditsHeight();

	m_dcCredits.CreateCompatibleDC(&m_dc);
	m_bmpCredits.CreateCompatibleBitmap(&m_dc, m_nCreditsBmpWidth, m_nCreditsBmpHeight);
	m_pbmpOldCredits = m_dcCredits.SelectObject(&m_bmpCredits);

	m_dcCredits.FillSolidRect(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, MASK_RGB);

	CFont* pOldFont;

	pOldFont  = m_dcCredits.SelectObject(m_arFonts.GetAt(0));

	m_dcCredits.SetBkMode(TRANSPARENT);

	int y = 0;

	int nFont;
	int nColor;

	int nLastFont = -1;
	int nLastColor = -1;

	int nTextHeight = m_dcCredits.GetTextExtent(_T("Wy")).cy;

	for(int n = 0; n < m_arCredits.GetSize(); n++)
	{
		CString sType = m_arCredits.GetAt(n).Left(1);

		if(sType == 'B')
		{
			// it's a bitmap

			CBitmap bmp;
			if(! bmp.LoadBitmap(m_arCredits.GetAt(n).Mid(2)))
			{
				CString str; 
				str.Format(_T("Could not find bitmap resource \"%s\". Be sure to assign the bitmap a QUOTED resource name"), m_arCredits.GetAt(n).Mid(2)); 
				AfxMessageBox(str,MB_ICONINFORMATION); 
				return; 
			}

			BITMAP bmInfo;
			bmp.GetBitmap(&bmInfo);

			CDC dc;
			dc.CreateCompatibleDC(&m_dcCredits);
			CBitmap* pOldBmp = dc.SelectObject(&bmp);

			// draw the bitmap
			m_dcCredits.BitBlt((m_rectScreen.Width() - bmInfo.bmWidth) / 2, y, bmInfo.bmWidth, bmInfo.bmHeight, &dc, 0, 0, SRCCOPY);

			dc.SelectObject(pOldBmp);
			bmp.DeleteObject();

			y += bmInfo.bmHeight;
		}
		else if(sType == 'S')
		{
			// it's a vertical space

			y += _ttoi(m_arCredits.GetAt(n).Mid(2));
		}
		else
		{
			// it's a text string

			nFont = _ttoi(m_arCredits.GetAt(n).Left(2));
			nColor = _ttoi(m_arCredits.GetAt(n).Mid(3,2));

			if(nFont != nLastFont)
			{
				m_dcCredits.SelectObject(m_arFonts.GetAt(nFont));
				nTextHeight = m_arFontHeights.GetAt(nFont);
			}

			if(nColor != nLastColor)
			{
				m_dcCredits.SetTextColor(m_arColors.GetAt(nColor));
			}

			CRect rect(0, y, m_rectScreen.Width(), y + nTextHeight);

			m_dcCredits.DrawText(m_arCredits.GetAt(n).Mid(6), &rect, DT_CENTER);

			y += nTextHeight;
		}
	}

	m_dcCredits.SetBkColor(MASK_RGB);
	m_dcCredits.SelectObject(pOldFont);

	// create the mask bitmap
	m_dcMask.CreateCompatibleDC(&m_dcScreen);
	m_bmpMask.CreateBitmap(m_nCreditsBmpWidth, m_nCreditsBmpHeight, 1, 1, NULL);

	// select the mask bitmap into the appropriate dc
	m_pbmpOldMask = m_dcMask.SelectObject(&m_bmpMask);

	// build mask based on transparent color
	m_dcMask.BitBlt(0, 0, m_nCreditsBmpWidth, m_nCreditsBmpHeight, &m_dcCredits, 0, 0, SRCCOPY);
}

void CCreditsThread::InitFonts()
{
	// create each font we'll need and add it to the fonts array

	CDC dcMem;
	dcMem.CreateCompatibleDC(&m_dc);
	CFont* pOldFont;
	int nTextHeight;

	LOGFONT lf;

	// font 0
	// SMALL ARIAL
	CFont* font0 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 12;
	lf.lfWeight = 500;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font0->CreateFontIndirect(&lf);
	m_arFonts.Add(font0);

	pOldFont = dcMem.SelectObject(font0);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 1
	// MEDIUM BOLD ARIAL
	CFont* font1 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 14;
	lf.lfWeight = 600;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font1->CreateFontIndirect(&lf);
	m_arFonts.Add(font1);

	dcMem.SelectObject(font1);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 2
	// LARGE ITALIC HEAVY BOLD TIMES ROMAN
	CFont* font2 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 16;
	lf.lfWeight = 700;
	//lf.lfItalic = TRUE;
	lf.lfQuality =  ANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font2->CreateFontIndirect(&lf);
	m_arFonts.Add(font2);

	dcMem.SelectObject(font2);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	// font 3
	CFont* font3 = new CFont;
	memset((void*)&lf, 0, sizeof(lf));
	lf.lfHeight = 25;
	lf.lfWeight = 900;
	lf.lfQuality = ANTIALIASED_QUALITY;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	font3->CreateFontIndirect(&lf);
	m_arFonts.Add(font3);

	dcMem.SelectObject(font3);
	nTextHeight = dcMem.GetTextExtent(_T("Wy")).cy;
	m_arFontHeights.Add(nTextHeight);

	dcMem.SelectObject(pOldFont);
}

void CCreditsThread::InitColors()
{
	// define each color we'll be using

	m_arColors.Add(PALETTERGB(0, 0, 0));	// 0 = BLACK
	m_arColors.Add(PALETTERGB(90, 90, 90));	// 1 = very dark gray
	m_arColors.Add(PALETTERGB(128, 128, 128));		// 2 = DARK GRAY
	m_arColors.Add(PALETTERGB(192, 192, 192));	// 3 = LIGHT GRAY
	m_arColors.Add(PALETTERGB(200, 50, 50));	// 4 = very light gray
	m_arColors.Add(PALETTERGB(255, 255, 128));	// 5 white
	m_arColors.Add(PALETTERGB(0, 0, 128));	// 6 dark blue
	m_arColors.Add(PALETTERGB(128, 128, 255));	// 7 light blue
	m_arColors.Add(PALETTERGB(0, 106, 0));	// 8 dark green
}

void CCreditsThread::InitText()
{
	// 1st pair of digits identifies the font to use
	// 2nd pair of digits identifies the color to use
	// B = Bitmap
	// S = Space (moves down the specified number of pixels)

	CString sTmp;
	
	/*
		You may NOT modify this copyright message. You may add your name, if you
		changed or improved this code, but you mot not delete any part of this message,
		make it invisible etc.
	*/

	// start at the bottom of the screen
	sTmp.Format(_T("S:%d"), m_rectScreen.Height());
	m_arCredits.Add(sTmp);

	m_arCredits.Add(_T("03:00:Netdefender 1.5"));
	//sTmp.Format(_T("02:01:Version %s"),theApp.m_strCurVersionLong);
	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	//sTmp += _T(" [") + theApp.m_strModLongVersion + _T("]");
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]

	m_arCredits.Add(_T("S:25"));
	m_arCredits.Add(_T("01:06:Copyright (C) 2002-2007 http://Programmerworld.Net"));
	m_arCredits.Add(_T("S:25"));

    //Commander - Added: Netdefender - Start
	//////////////////////////////////////////////////////////////////////////
	m_arCredits.Add(_T("02:04:Developed by")); 
	m_arCredits.Add(_T("S:6"));
	m_arCredits.Add(_T("02:04:Sudhir Mangla"));
	m_arCredits.Add(_T("S:3"));
	m_arCredits.Add(_T("01:02:(sudhirmangla@gmail.com)"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:08:Rajender Singh"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:with help from"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:http://CodeProject.com and "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:http://CodeGuru.com "));
	m_arCredits.Add(_T("S:25"));
	//////////////////////////////////////////////////////////////////////////
	m_arCredits.Add(_T("02:04:Technology Used")); 
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:A Packet Filtering Based Firewall"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Developed using VC++ 7.1, "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06: MFC 7.1 and is Unicode enable"));
	m_arCredits.Add(_T("S:6"));
	m_arCredits.Add(_T("01:06: Platform Supported : Windows XP and 2000"));
	m_arCredits.Add(_T("S:25"));
	//////////////////////////////////////////////////////////////////////////
	m_arCredits.Add(_T("02:04:Special thanks to")); 
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:08:Jesús O. for providing his "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:Driver and "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:article Developing Firewalls"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:for Windows 2000/XP"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:on Codeproject.com"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:and to Y0DA"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:for EnetstatX - An Enhance netstat "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:on Codeproject.com"));
	m_arCredits.Add(_T("S:25"));
	//////////////////////////////////////////////////////////////////////////
	m_arCredits.Add(_T("02:04:For latest updated ")); 
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06:on NetDefender "));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:06: Visit us on"));
	m_arCredits.Add(_T("S:5"));
	m_arCredits.Add(_T("01:08:http://Programmerworld.Net"));

	// pause before repeating
	m_arCredits.Add(_T("S:50"));
}

int CCreditsThread::CalcCreditsHeight()
{
	int nHeight = 0;

	for(int n = 0; n < m_arCredits.GetSize(); n++)
	{
		CString sType = m_arCredits.GetAt(n).Left(1);

		if(sType == 'B')
		{
			// it's a bitmap

			CBitmap bmp;
			if (! bmp.LoadBitmap(m_arCredits.GetAt(n).Mid(2)))
			{
				CString str; 
				str.Format(_T("Could not find bitmap resource \"%s\". Be sure to assign the bitmap a QUOTED resource name"), m_arCredits.GetAt(n).Mid(2)); 
				AfxMessageBox(str,MB_ICONINFORMATION); 
				return -1; 
			}

			BITMAP bmInfo;
			bmp.GetBitmap(&bmInfo);

			nHeight += bmInfo.bmHeight;
		}
		else if(sType == 'S')
		{
			// it's a vertical space

			nHeight += _ttoi(m_arCredits.GetAt(n).Mid(2));
		}
		else
		{
			// it's a text string

			int nFont = _ttoi(m_arCredits.GetAt(n).Left(2));
			nHeight += m_arFontHeights.GetAt(nFont);
		}
	}

	return nHeight;
}
