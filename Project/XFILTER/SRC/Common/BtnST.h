#ifndef _BTNST_H
#define _BTNST_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// CBtnST.h : header file
//

// Comment this if you don't want that CButtonST hilights itself
// also when the window is inactive (like happens in Internet Explorer)
#define ST_LIKEIE

// Comment this if you don't want to use CMemDC class
#define ST_USE_MEMDC

/////////////////////////////////////////////////////////////////////////////
// CButtonST window

#define HEIGHTTYPE_DEFAULT		0
#define HEIGHTTYPE_2_3			1
#define HEIGHTTYPE_3_4			2

class CButtonST : public CButton
{
// Construction
public:
    CButtonST();
	~CButtonST();
	enum {ST_ALIGN_HORIZ, ST_ALIGN_VERT};

// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CButtonST)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF GetDisableFgColor(){return m_crDisableFg;}
	void SetDisableFgColor(COLORREF crDisableFg){m_crDisableFg = crDisableFg;}
	COLORREF GetSelectBgColor(){return m_crSelectBg;}
	void SetSelectBgColor(COLORREF crSelectBg){m_crSelectBg = crSelectBg;}
	COLORREF GetSelectFgColor(){return m_crSelectFg;}
	void SetSelectFgColor(COLORREF crSelectFg){m_crSelectFg = crSelectFg;}

	void SetNormalBorder(COLORREF nNormalBorderColor){m_nNormalBorderColor = nNormalBorderColor; m_bIsDrawNormalBorder = TRUE;}
	void SetButton(UINT nID, CWnd *pParent);
	void SetButtonEx(UINT nID, CWnd *pParent);
	void SetRadioButton(UINT nID, UINT nIDNormalBitmap, UINT nIDSelectBitmap, CWnd *pParent);
	void SetLamp(UINT nID
		, UINT nBitmapFocusId
		, UINT nBitmapNormalId
		, UINT nBitmapSelectId
		, LPCTSTR lpszToolTipText
		, CWnd *pParent
		);

	void SetDrawDown(BOOL bIsDrawDown){m_bIsDrawDown = bIsDrawDown;}
	void SetSelect(BOOL bIsSelect){m_bIsSelect = bIsSelect; EnableWindow(!bIsSelect); Invalidate();}
	void SetToolTipText(CString sToolTipText);
	void SetHeightType(int nHeightType){m_nHeightType = nHeightType;}
	void SetTransparent(BOOL bIsTransparent);
	void SetFocusDrawBorder(BOOL bDrawBorder);
	void SetBitmaps(int nBitmapInId, int nBitmapOutId = NULL, int nBitmapSelId = NULL, BYTE cx = 0, BYTE cy = 0);

	BOOL SetBtnCursor(int nCursorId = -1);

	void SetFlatFocus(BOOL bDrawFlatFocus, BOOL bRepaint = FALSE);
	BOOL GetFlatFocus();

	void SetDefaultActiveFgColor(BOOL bRepaint = FALSE);
	void SetActiveFgColor(COLORREF crNew, BOOL bRepaint = FALSE);
	const COLORREF GetActiveFgColor();
	
	void SetDefaultActiveBgColor(BOOL bRepaint = FALSE);
	void SetActiveBgColor(COLORREF crNew, BOOL bRepaint = FALSE);
	const COLORREF GetActiveBgColor();
	
	void SetDefaultInactiveFgColor(BOOL bRepaint = FALSE);
	void SetInactiveFgColor(COLORREF crNew, BOOL bRepaint = FALSE);
	const COLORREF GetInactiveFgColor();

	void SetDefaultInactiveBgColor(BOOL bRepaint = FALSE);
	void SetInactiveBgColor(COLORREF crNew, BOOL bRepaint = FALSE);
	const COLORREF GetInactiveBgColor();

	void SetShowText(BOOL bShow = TRUE);
	BOOL GetShowText();

	void SetAlign(int nAlign);
	int GetAlign();

	void SetFlat(BOOL bState = TRUE);
	BOOL GetFlat();

	void DrawBorder(BOOL bEnable = TRUE);
	void SetIcon(int nIconInId, int nIconOutId = NULL, BYTE cx = 32, BYTE cy = 32);


	static const short GetVersionI();
	static const char* GetVersionC();

protected:
    //{{AFX_MSG(CButtonST)
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void DrawTheIcon(CDC* pDC, CString* title, RECT* rcItem, CRect* captionRect, BOOL IsPressed, BOOL IsDisabled);

	void DrawTheBitmap(CDC* pDC, CString* title, RECT* rcItem, CRect* captionRect, BOOL IsPressed, BOOL IsDisabled);

    CToolTipCtrl m_ToolTip;
	CString		 m_sToolTipText;
	
	BOOL m_bIsDrawNormalBorder;
	COLORREF m_nNormalBorderColor;

	BOOL m_bIsDrawDown;
	BOOL m_bIsSelect;
	int m_nAlign;
	BOOL m_bShowText;
	BOOL m_bDrawBorder;
	BOOL m_bIsFlat;
	BOOL m_MouseOnButton;
	BOOL m_bDrawFlatFocus;

	BOOL m_bFocusDrawBorder;
	BOOL m_bIsFocus;

	BOOL m_bIsTransparent;
	int m_nHeightType;

	HCURSOR m_hCursor;

	HICON m_hIconIn;
	HICON m_hIconOut;
	BYTE m_cyIcon;
	BYTE m_cxIcon;

	HBITMAP m_hBitmapIn;
	HBITMAP m_hBitmapOut;
	HBITMAP m_hBitmapSel;
	BYTE m_cyBitmap;
	BYTE m_cxBitmap;

	COLORREF  m_crInactiveBg;
    COLORREF  m_crInactiveFg;
    COLORREF  m_crActiveBg;
    COLORREF  m_crActiveFg;

	COLORREF  m_crSelectBg;
	COLORREF  m_crSelectFg;

	COLORREF  m_crDisableFg;
};

/////////////////////////////////////////////////////////////////////////////


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif
