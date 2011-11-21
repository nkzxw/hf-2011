// BtnST.cpp : implementation file
//

#include "stdafx.h"
#include "BtnST.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CButtonST

CButtonST::CButtonST()
{
  m_crDisableFg = COLOR_DISABLE_TEXT;
  m_bIsDrawNormalBorder = FALSE;

  m_bIsDrawDown = TRUE;
  m_bIsSelect = FALSE;
  m_sToolTipText = "";
  m_nHeightType = HEIGHTTYPE_DEFAULT;
  m_bIsTransparent = TRUE;

  m_bFocusDrawBorder = FALSE;
  m_bIsFocus = FALSE;

  m_MouseOnButton = FALSE;

  m_hIconIn = NULL;
  m_hIconOut = NULL;
  m_cxIcon = 0;
  m_cyIcon = 0;

  m_hBitmapIn = NULL;
  m_hBitmapOut = NULL;
  m_hBitmapSel = NULL;
  m_cyBitmap = 0;
  m_cxBitmap = 0;

  m_hCursor = NULL;
  
  // Default type is "flat" button
  m_bIsFlat = TRUE; 
  
  // By default draw border in "flat" button 
  m_bDrawBorder = TRUE; 
  
  // By default icon is aligned horizontally
  m_nAlign = ST_ALIGN_HORIZ; 
  
  // By default show the text button
  m_bShowText = TRUE; 
  
  // By default, for "flat" button, don't draw the focus rect
  m_bDrawFlatFocus = FALSE;
	
  SetDefaultInactiveBgColor();
  SetDefaultInactiveFgColor();
  SetDefaultActiveBgColor();
  SetDefaultActiveFgColor();
} // End of CButtonST


CButtonST::~CButtonST()
{
	// Destroy the icons (if any)
	if (m_hIconIn != NULL) ::DeleteObject(m_hIconIn);
	if (m_hIconOut != NULL) ::DeleteObject(m_hIconOut);

	if(m_hBitmapIn != NULL) ::DeleteObject(m_hBitmapIn);
	if(m_hBitmapOut != NULL) ::DeleteObject(m_hBitmapOut);
	if(m_hBitmapSel != NULL) ::DeleteObject(m_hBitmapSel);

	// Destroy the cursor (if any)
	if (m_hCursor != NULL) ::DestroyCursor(m_hCursor);
} // End of ~CButtonST


BEGIN_MESSAGE_MAP(CButtonST, CButton)
    //{{AFX_MSG_MAP(CButtonST)
	ON_WM_CAPTURECHANGED()
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CButtonST::SetIcon(int nIconInId, int nIconOutId, BYTE cx, BYTE cy)
{
	HINSTANCE hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nIconInId),
													RT_GROUP_ICON);
	// Set icon when the mouse is IN the button
	m_hIconIn = (HICON)::LoadImage(hInstResource/*AfxGetApp()->m_hInstance*/, MAKEINTRESOURCE(nIconInId), IMAGE_ICON, 0, 0, 0);
  
	// Set icon when the mouse is OUT the button
	m_hIconOut = (nIconOutId == NULL) ? m_hIconIn : (HICON)::LoadImage(hInstResource/*AfxGetApp()->m_hInstance*/, MAKEINTRESOURCE(nIconOutId), IMAGE_ICON, 0, 0, 0);
  
	m_cxIcon = cx;
	m_cyIcon = cy;

	RedrawWindow();
} // End of SetIcon

void CButtonST::SetBitmaps(int nBitmapInId, int nBitmapOutId, int nBitmapSelId, BYTE cx, BYTE cy)
{
	if(m_hBitmapIn != NULL) ::DeleteObject(m_hBitmapIn);
	if(m_hBitmapOut != NULL) ::DeleteObject(m_hBitmapOut);
	if(m_hBitmapSel != NULL) ::DeleteObject(m_hBitmapSel);
	m_hBitmapIn = NULL;
	m_hBitmapOut = NULL;
	m_hBitmapSel = NULL;

	HINSTANCE hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(CButtonST),
													RT_GROUP_BITMAP);
	// Set icon when the mouse is IN the button
	m_hBitmapIn = (HBITMAP)::LoadBitmap(hInstResource, MAKEINTRESOURCE(nBitmapInId));
  
	// Set icon when the mouse is OUT the button
	m_hBitmapOut = (nBitmapOutId == NULL) ? m_hBitmapIn 
		: (HBITMAP)::LoadBitmap(hInstResource, MAKEINTRESOURCE(nBitmapOutId));

	m_hBitmapSel = (nBitmapSelId == NULL) ? m_hBitmapIn 
		: (HBITMAP)::LoadBitmap(hInstResource, MAKEINTRESOURCE(nBitmapSelId));
  
	m_cxBitmap = cx;
	m_cyBitmap = cy;

	RedrawWindow();
} // End of SetIcon


BOOL CButtonST::SetBtnCursor(int nCursorId)
{
	HINSTANCE hInstResource;
	// Destroy any previous cursor
	if (m_hCursor != NULL) ::DestroyCursor(m_hCursor);
	m_hCursor = NULL;

	// If we want a cursor
	if (nCursorId != -1)
	{
		hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nCursorId),
											RT_GROUP_CURSOR);
		// Load icon resource
		m_hCursor = (HCURSOR)::LoadImage(hInstResource/*AfxGetApp()->m_hInstance*/, MAKEINTRESOURCE(nCursorId), IMAGE_CURSOR, 0, 0, 0);
		// If something wrong then return FALSE
		if (m_hCursor == NULL) return FALSE;
	}

	return TRUE;
} // End of SetBtnCursor


void CButtonST::SetFlat(BOOL bState)
{
  m_bIsFlat = bState;
  Invalidate();
} // End of SetFlat


BOOL CButtonST::GetFlat()
{
  return m_bIsFlat;
} // End of GetFlat


void CButtonST::SetAlign(int nAlign)
{
  switch (nAlign)
  {    
    case ST_ALIGN_HORIZ:
         m_nAlign = ST_ALIGN_HORIZ;
         break;
    case ST_ALIGN_VERT:
         m_nAlign = ST_ALIGN_VERT;
         break;
  }
  Invalidate();
} // End of SetAlign


int CButtonST::GetAlign()
{
  return m_nAlign;
} // End of GetAlign


void CButtonST::DrawBorder(BOOL bEnable)
{
  m_bDrawBorder = bEnable;
} // End of DrawBorder


const char* CButtonST::GetVersionC()
{
  return "2.3";
} // End of GetVersionC


const short CButtonST::GetVersionI()
{
  return 23; // Divide by 10 to get actual version
} // End of GetVersionI


void CButtonST::SetShowText(BOOL bShow)
{
  m_bShowText = bShow;
  Invalidate();
} // End of SetShowText


BOOL CButtonST::GetShowText()
{
  return m_bShowText;
} // End of GetShowText


void CButtonST::OnMouseMove(UINT nFlags, CPoint point)
{
	//CButton::OnMouseMove(nFlags, point);
	//return;

  CWnd* pWnd;  // Finestra attiva
  CWnd* pParent; // Finestra che contiene il bottone

  CButton::OnMouseMove(nFlags, point);

  // If the mouse enter the button with the left button pressed
  // then do nothing
  if (nFlags & MK_LBUTTON && m_MouseOnButton == FALSE) return;

  // If our button is not flat then do nothing
  if (m_bIsFlat == FALSE) return;

  pWnd = GetActiveWindow();
  pParent = GetOwner();

	if ((GetCapture() != this) && 
		(
#ifndef ST_LIKEIE
		pWnd != NULL && 
#endif
		pParent != NULL)) 
	{
		m_MouseOnButton = TRUE;
		//SetFocus();	// Thanks Ralph!
		SetCapture();
		Invalidate();
	}
	else
  {
    CRect rc;
    GetClientRect(&rc);
    if (!rc.PtInRect(point))
    {
      // Redraw only if mouse goes out
      if (m_MouseOnButton == TRUE && m_bIsFocus ? !m_bFocusDrawBorder : TRUE)
      {
        m_MouseOnButton = FALSE;
        Invalidate();
      }
      // If user is NOT pressing left button then release capture!
      if (!(nFlags & MK_LBUTTON)) ReleaseCapture();
    }
  }
} // End of OnMouseMove


void CButtonST::OnKillFocus(CWnd * pNewWnd)
{
  CButton::OnKillFocus(pNewWnd);
  m_bIsFocus = FALSE;

  // If our button is not flat then do nothing
  if (m_bIsFlat == FALSE) return;

  if (m_MouseOnButton == TRUE && m_bIsFocus ? !m_bFocusDrawBorder : TRUE)
  {
    m_MouseOnButton = FALSE;
    Invalidate();
  }
} // End of OnKillFocus


void CButtonST::OnCaptureChanged(CWnd *pWnd) 
{
	if (m_MouseOnButton == TRUE)
	{
		ReleaseCapture();
		Invalidate();
	}

	// Call base message handler
	CButton::OnCaptureChanged(pWnd);
} // End of OnCaptureChanged


void CButtonST::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	//CButton::DrawItem(lpDIS);
	//return;

  CDC* pDC = CDC::FromHandle(lpDIS->hDC);

  CPen *pOldPen;
  BOOL bIsPressed  = (lpDIS->itemState & ODS_SELECTED);
  BOOL bIsFocused  = (lpDIS->itemState & ODS_FOCUS);
  BOOL bIsDisabled = (lpDIS->itemState & ODS_DISABLED);

  CRect itemRect = lpDIS->rcItem;

  if (m_bIsFlat == FALSE)
  {
    if (bIsFocused)
    {
      CBrush br(RGB(0,0,0));  
      pDC->FrameRect(&itemRect, &br);
      itemRect.DeflateRect(1, 1);
	  br.DeleteObject();
    }
  }

  // Prepare draw... paint button's area with background color
  COLORREF bgColor;

  if(m_bIsSelect)
	bgColor = m_crSelectBg;
  else if ((m_MouseOnButton == TRUE) || (bIsPressed))
    bgColor = GetActiveBgColor();
  else
    bgColor = GetInactiveBgColor();

  CBrush br(bgColor);
  pDC->FillRect(&itemRect, &br);
  br.DeleteObject();
	// Disegno lo sfondo del bottone
//CBrush br(GetSysColor(COLOR_BTNFACE));  
//pDC->FillRect(&itemRect, &br);

  // Read the button title
  CString sTitle;
  GetWindowText(sTitle);

  // If we don't want the title displayed
  if (m_bShowText == FALSE) sTitle.Empty();

  CRect captionRect = lpDIS->rcItem;

  if(m_hBitmapIn != NULL)
  {
    DrawTheBitmap(pDC, &sTitle, &lpDIS->rcItem, &captionRect, bIsPressed, bIsDisabled);
  }

  // Draw the icon
  if (m_hIconIn != NULL)
  {
    DrawTheIcon(pDC, &sTitle, &lpDIS->rcItem, &captionRect, bIsPressed, bIsDisabled);
  }

  // Draw pressed button
  if (bIsPressed)
  {
    if (m_bIsFlat == TRUE)
    {
      if (m_bDrawBorder == TRUE)
      {
	    CPen penBtnHiLight(PS_SOLID, 0, GetSysColor(COLOR_BTNHILIGHT)); // Bianco
        CPen penBtnShadow(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));   // Grigio scuro

        // Disegno i bordi a sinistra e in alto
        // Dark gray line
        pOldPen = pDC->SelectObject(&penBtnShadow);
        pDC->MoveTo(itemRect.left, itemRect.bottom-1);
        pDC->LineTo(itemRect.left, itemRect.top);
        pDC->LineTo(itemRect.right, itemRect.top);
        // Disegno i bordi a destra e in basso
        // White line
        pDC->SelectObject(penBtnHiLight);
        pDC->MoveTo(itemRect.left, itemRect.bottom-1);
        pDC->LineTo(itemRect.right-1, itemRect.bottom-1);
        pDC->LineTo(itemRect.right-1, itemRect.top-1);
        //
        pDC->SelectObject(pOldPen);

		penBtnHiLight.DeleteObject();
		penBtnShadow.DeleteObject();
      }
    }
    else    
    {
      CBrush brBtnShadow(GetSysColor(COLOR_BTNSHADOW));
      pDC->FrameRect(&itemRect, &brBtnShadow);
	  brBtnShadow.DeleteObject();
    }
  }
  else // ...else draw non pressed button
  {
    CPen penBtnHiLight(PS_SOLID, 0, GetSysColor(COLOR_BTNHILIGHT)); // White
    CPen pen3DLight(PS_SOLID, 0, GetSysColor(COLOR_3DLIGHT));       // Light gray
    CPen penBtnShadow(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));   // Dark gray
    CPen pen3DDKShadow(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW)); // Black

    if (m_bIsFlat == TRUE)
    {
      if (m_MouseOnButton == TRUE && m_bDrawBorder == TRUE)
      {
  	    // Disegno i bordi a sinistra e in alto
        // White line
        pOldPen = pDC->SelectObject(&penBtnHiLight);
        pDC->MoveTo(itemRect.left, itemRect.bottom-1);
        pDC->LineTo(itemRect.left, itemRect.top);
        pDC->LineTo(itemRect.right, itemRect.top);
        // Disegno i bordi a destra e in basso
        // Dark gray line
        pDC->SelectObject(penBtnShadow);
        pDC->MoveTo(itemRect.left, itemRect.bottom-1);
        pDC->LineTo(itemRect.right-1, itemRect.bottom-1);
        pDC->LineTo(itemRect.right-1, itemRect.top-1);
        //
        pDC->SelectObject(pOldPen);
      }
    }
    else
    {
      // Disegno i bordi a sinistra e in alto
      // White line
      pOldPen = pDC->SelectObject(&penBtnHiLight);
      pDC->MoveTo(itemRect.left, itemRect.bottom-1);
      pDC->LineTo(itemRect.left, itemRect.top);
      pDC->LineTo(itemRect.right, itemRect.top);
      // Light gray line
      pDC->SelectObject(pen3DLight);
      pDC->MoveTo(itemRect.left+1, itemRect.bottom-1);
      pDC->LineTo(itemRect.left+1, itemRect.top+1);
      pDC->LineTo(itemRect.right, itemRect.top+1);
      // Disegno i bordi a destra e in basso
      // Black line
      pDC->SelectObject(pen3DDKShadow);
      pDC->MoveTo(itemRect.left, itemRect.bottom-1);
      pDC->LineTo(itemRect.right-1, itemRect.bottom-1);
      pDC->LineTo(itemRect.right-1, itemRect.top-1);
      // Dark gray line
      pDC->SelectObject(penBtnShadow);
      pDC->MoveTo(itemRect.left+1, itemRect.bottom-2);
      pDC->LineTo(itemRect.right-2, itemRect.bottom-2);
      pDC->LineTo(itemRect.right-2, itemRect.top);
      //
      pDC->SelectObject(pOldPen);
    }

	penBtnHiLight.DeleteObject();
	pen3DLight.DeleteObject();
	penBtnShadow.DeleteObject();
	pen3DDKShadow.DeleteObject();
  }

  if(m_bIsDrawNormalBorder)
  {
	  CPen penBody(PS_SOLID, 1, m_nNormalBorderColor); //
	  pOldPen = pDC->SelectObject(&penBody);
	  pDC->MoveTo(itemRect.left, itemRect.bottom);
	  pDC->LineTo(itemRect.left, itemRect.top);
	  pDC->LineTo(itemRect.right, itemRect.top);
	  pDC->MoveTo(itemRect.left, itemRect.bottom-1);
	  pDC->LineTo(itemRect.right-1, itemRect.bottom-1);
	  pDC->LineTo(itemRect.right-1, itemRect.top-1);

	  pDC->SelectObject(pOldPen);
  }

//
  // Write the button title (if any)
  if (sTitle.IsEmpty() == FALSE)
  {
    // Disegno la caption del bottone
    // Se il bottone e' premuto muovo la captionRect di conseguenza
    if (bIsPressed )
      captionRect.OffsetRect(1, 1);
    
    // ONLY FOR DEBUG 
    // Evidenzia il rettangolo in cui verra' centrata la caption 
    //CBrush brBtnShadow(RGB(255, 0, 0));
    //pDC->FrameRect(&captionRect, &brBtnShadow);

	if(m_bIsSelect)
	{
      pDC->SetTextColor(GetSelectFgColor());
      pDC->SetBkColor(GetSelectBgColor());
	}
    else if ((m_MouseOnButton == TRUE) || (bIsPressed)) 
	{
      pDC->SetTextColor(GetActiveFgColor());
      pDC->SetBkColor(GetActiveBgColor());
    } 
	else 
	{
      pDC->SetTextColor(GetInactiveFgColor());
      pDC->SetBkColor(GetInactiveBgColor());
    }

	if(bIsDisabled)
	{
		pDC->SetTextColor(GetDisableFgColor());
	}

	if(m_bIsTransparent)
	{
		pDC->SetBkMode(TRANSPARENT);
	}

    // Center text
    CRect centerRect = captionRect;
    pDC->DrawText(sTitle, -1, captionRect, DT_SINGLELINE|DT_CALCRECT);
	switch(m_nHeightType)
	{
	case HEIGHTTYPE_2_3:
		captionRect.OffsetRect((centerRect.Width() - captionRect.Width())/2, (centerRect.Height() - captionRect.Height())*2/3);
		break;
	case HEIGHTTYPE_3_4:
		captionRect.OffsetRect((centerRect.Width() - captionRect.Width())/2, (centerRect.Height() - captionRect.Height())*3/4);
		break;
	default:
		captionRect.OffsetRect((centerRect.Width() - captionRect.Width())/2, (centerRect.Height() - captionRect.Height())/2);
		break;
	}
	/* RFU
    captionRect.OffsetRect(0, (centerRect.Height() - captionRect.Height())/2);
    captionRect.OffsetRect((centerRect.Width() - captionRect.Width())-4, (centerRect.Height() - captionRect.Height())/2);
	*/

    pDC->DrawState(captionRect.TopLeft(), captionRect.Size(), (LPCTSTR)sTitle, DSS_NORMAL, //(bIsDisabled ? DSS_DISABLED : DSS_NORMAL), 
                   TRUE, 0, (CBrush*)NULL);
  }

  if (m_bIsFlat == FALSE || (m_bIsFlat == TRUE && m_bDrawFlatFocus == TRUE))
  {
    // Draw the focus rect
    if (bIsFocused)
    {
      CRect focusRect = itemRect;
      focusRect.DeflateRect(3, 3);
      pDC->DrawFocusRect(&focusRect);
    }
  }
} // End of DrawItem


void CButtonST::DrawTheIcon(CDC* pDC, CString* title, RECT* rcItem, CRect* captionRect, BOOL IsPressed, BOOL IsDisabled)
{
  CRect iconRect = rcItem;

  switch (m_nAlign)
  {
    case ST_ALIGN_HORIZ:
         if (title->IsEmpty())
         {
           // Center the icon horizontally
           iconRect.left += ((iconRect.Width() - m_cxIcon)/2);
         }
         else
         {
           // L'icona deve vedersi subito dentro il focus rect
           iconRect.left += 3;  
           captionRect->left += m_cxIcon + 3;
         }
         // Center the icon vertically
         iconRect.top += ((iconRect.Height() - m_cyIcon)/2);
         break;
    case ST_ALIGN_VERT:
         // Center the icon horizontally
         iconRect.left += ((iconRect.Width() - m_cxIcon)/2);
         if (title->IsEmpty())
         {
           // Center the icon vertically
           iconRect.top += ((iconRect.Height() - m_cyIcon)/2);           
         }
         else
         {
           captionRect->top += m_cyIcon;
         }
         break;
  }
    
  // If button is pressed then press the icon also
  if (IsPressed) iconRect.OffsetRect(1, 1);
  // Ole'!
  pDC->DrawState(iconRect.TopLeft(), 
	               iconRect.Size(), 
				         (m_MouseOnButton == TRUE || IsPressed) ? m_hIconIn : m_hIconOut, 
				         DSS_NORMAL, //(IsDisabled ? DSS_DISABLED : DSS_NORMAL), 
                 (CBrush*)NULL);
} // End of DrawTheIcon

void CButtonST::DrawTheBitmap(CDC* pDC, CString* title, RECT* rcItem, CRect* captionRect, BOOL IsPressed, BOOL IsDisabled)
{
  CRect bitmapRect = rcItem;

  pDC->DrawState(bitmapRect.TopLeft(), 
	               bitmapRect.Size(), 
				   m_bIsSelect ? m_hBitmapSel : ((m_MouseOnButton == TRUE || IsPressed) ? m_hBitmapIn : m_hBitmapOut), 
				         DSS_NORMAL, //(IsDisabled ? DSS_DISABLED : DSS_NORMAL), 
                 NULL);

  // If button is pressed then press the icon also
  if (IsPressed && m_bIsDrawDown && !m_bIsSelect) 
  {
	  bitmapRect.OffsetRect(1, 1);
	  // Ole'!
	  pDC->DrawState(bitmapRect.TopLeft(), 
					   bitmapRect.Size(), 
					   m_bIsSelect ? m_hBitmapSel : ((m_MouseOnButton == TRUE || IsPressed) ? m_hBitmapIn : m_hBitmapOut), 
							 DSS_NORMAL, //(IsDisabled ? DSS_DISABLED : DSS_NORMAL), 
					 NULL);
  }
} // End of DrawTheIcon


void CButtonST::PreSubclassWindow() 
{
	CRect rect; 
	GetClientRect(rect);
	m_ToolTip.Create(this);
	m_ToolTip.AddTool(this, m_sToolTipText, rect, 1);

	// Add BS_OWNERDRAW style
	SetButtonStyle(GetButtonStyle() | BS_OWNERDRAW );

	CButton::PreSubclassWindow();
} // End of PreSubclassWindow


void CButtonST::SetDefaultInactiveBgColor(BOOL bRepaint)
{
	m_crInactiveBg = ::GetSysColor(COLOR_BTNFACE); 
	if (bRepaint == TRUE) Invalidate();
} // End of SetDefaultInactiveBgColor


void CButtonST::SetInactiveBgColor(COLORREF crNew, BOOL bRepaint)
{
	m_crInactiveBg = crNew; 
	if (bRepaint == TRUE) Invalidate();
} // End of SetInactiveBgColor


const COLORREF CButtonST::GetInactiveBgColor()
{
	return m_crInactiveBg;
} // End of GetInactiveBgColor


void CButtonST::SetDefaultInactiveFgColor(BOOL bRepaint)
{
	m_crInactiveFg = ::GetSysColor(COLOR_BTNTEXT); 
	m_crSelectFg = m_crInactiveFg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetDefaultInactiveFgColor


void CButtonST::SetInactiveFgColor(COLORREF crNew, BOOL bRepaint)
{
	m_crInactiveFg = crNew; 
	m_crSelectFg = m_crInactiveFg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetInactiveFgColor


const COLORREF CButtonST::GetInactiveFgColor()
{
	return m_crInactiveFg;
} // End of GetInactiveFgColor


void CButtonST::SetDefaultActiveBgColor(BOOL bRepaint)
{
	m_crActiveBg = ::GetSysColor(COLOR_BTNFACE); 
	m_crSelectBg = m_crActiveBg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetDefaultActiveBgColor


void CButtonST::SetActiveBgColor(COLORREF crNew, BOOL bRepaint)
{
	m_crActiveBg = crNew; 
	m_crSelectBg = m_crActiveBg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetActiveBgColor


const COLORREF CButtonST::GetActiveBgColor()
{
	return m_crActiveBg;
} // End of GetActiveBgColor


void CButtonST::SetDefaultActiveFgColor(BOOL bRepaint)
{
	m_crActiveFg = ::GetSysColor(COLOR_BTNTEXT); 
	m_crSelectFg = m_crActiveFg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetDefaultActiveFgColor


void CButtonST::SetActiveFgColor(COLORREF crNew, BOOL bRepaint)
{
	m_crActiveFg = crNew; 
	m_crSelectFg = m_crActiveFg;
	if (bRepaint == TRUE) Invalidate();
} // End of SetActiveFgColor


const COLORREF CButtonST::GetActiveFgColor()
{
	return m_crActiveFg;
} // End of GetActiveFgColor


void CButtonST::SetFlatFocus(BOOL bDrawFlatFocus, BOOL bRepaint)
{
	m_bDrawFlatFocus = bDrawFlatFocus;
	
	// Repaint the button
	if (bRepaint == TRUE) Invalidate();
} // End of SetFlatFocus


BOOL CButtonST::GetFlatFocus()
{
	return m_bDrawFlatFocus;
} // End of GetFlatFocus


BOOL CButtonST::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// If a cursor was specified then use it!
	if (m_hCursor != NULL)
	{
		::SetCursor(m_hCursor);
		return TRUE;
	}

	return CButton::OnSetCursor(pWnd, nHitTest, message);
} // End of OnSetCursor

void CButtonST::OnSetFocus(CWnd* pOldWnd) 
{
	CButton::OnSetFocus(pOldWnd);
	
	m_bIsFocus = TRUE;

	if(m_bFocusDrawBorder)
	{
		m_MouseOnButton = TRUE;
		Invalidate();
	}
}

void CButtonST::SetFocusDrawBorder(BOOL bDrawBorder)
{
	m_bFocusDrawBorder = bDrawBorder;
}

void CButtonST::SetTransparent(BOOL bIsTransparent)
{
	m_bIsTransparent = bIsTransparent;
}

BOOL CButtonST::PreTranslateMessage(MSG* pMsg) 
{
    m_ToolTip.RelayEvent(pMsg);
    return CButton::PreTranslateMessage(pMsg);
}

void CButtonST::SetToolTipText(CString sToolTipText)
{
	m_sToolTipText = sToolTipText;

    if (::IsWindow(GetSafeHwnd())) 
        m_ToolTip.UpdateTipText(m_sToolTipText, this, 1);
}

//----------------
//
void CButtonST::SetButton(UINT nID, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);

	SetActiveBgColor(COLOR_BK_FOCUS);
	SetInactiveBgColor(COLOR_BK_NORMAL);
	SetActiveFgColor(COLOR_TEXT_FOCUS);
	SetInactiveFgColor(COLOR_TEXT_NORMAL);
	SetNormalBorder(COLOR_BORDER);
	SetSelectBgColor(COLOR_BUTTON_SELECT_BK);
	SetSelectFgColor(COLOR_TEXT_SELECT);
}

void CButtonST::SetButtonEx(UINT nID, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);

	SetActiveBgColor(COLOR_BUTTON_EX_BK_FOCUS);
	SetInactiveBgColor(COLOR_BUTTON_EX_BK_NORMAL);
	SetActiveFgColor(COLOR_BUTTON_EX_TEXT_FOCUS);
	SetInactiveFgColor(COLOR_BUTTON_EX_TEXT_NORMAL);
}


void CButtonST::SetRadioButton(UINT nID, UINT nIDNormalBitmap, UINT nIDSelectBitmap, CWnd *pParent)
{
	SubclassDlgItem(nID, pParent);
	SetBitmaps(nIDNormalBitmap, nIDNormalBitmap, nIDSelectBitmap);
	DrawBorder(FALSE);
}

void CButtonST::SetLamp(UINT nID
		, UINT nBitmapFocusId
		, UINT nBitmapNormalId
		, UINT nBitmapSelectId
		, LPCTSTR lpszToolTipText
		, CWnd *pParent
)
{
	SubclassDlgItem(nID, pParent);
   	SetBitmaps(nBitmapFocusId, nBitmapNormalId, nBitmapSelectId);
	DrawBorder(FALSE);
	SetToolTipText(lpszToolTipText);
}

#undef ST_USE_MEMDC
#undef ST_LIKE

#pragma comment( exestr, "B9D3B8FD2A64767075762B")
