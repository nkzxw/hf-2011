//=============================================================================================
/*
	HyperLink.h

	Project	: XFILTER 1.0
	Author	: Tony Zhu
	Create Date	: 2001/08/06
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================

#if !defined(HYPERLINK_H)
#define HYPERLINK_H

class CHyperLink : public CStatic
{
public:
	CHyperLink();
	virtual ~CHyperLink();

public:
	COLORREF	m_LinkColor;
	COLORREF	m_HoverColor;
	COLORREF	m_VisitedColor;
	BOOL		m_bIsHover;
	BOOL		m_bIsVisited;
	CString		m_sUrl;
    HCURSOR		m_hLinkCursor;                        
    CFont		m_Font; 
	BOOL		m_bUnderline;
    CToolTipCtrl m_ToolTip;                        

private:
	void SetCursor();

public:
	void SetURL(CString sUrl);

protected:
    //{{AFX_VIRTUAL(CHyperLink)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    protected:
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CHyperLink)
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    //}}AFX_MSG
    afx_msg void OnClicked();
    DECLARE_MESSAGE_MAP()
};

#endif