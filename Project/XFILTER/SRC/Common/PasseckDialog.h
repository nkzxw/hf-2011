// CPasseckDialog.h

#ifndef CPASSECKDIALOG_H
#define CPASSECKDIALOG_H

class CPasseckDialog : public CDialog
{
public:
	CPasseckDialog(UINT nID, CWnd* pParent = NULL, LPCTSTR lpszWindowName = NULL);   // standard constructor

	void SetMoveParent(BOOL bMoveParent){m_bMoveParent = bMoveParent;}

	void CreateCaption(LPCTSTR lpszWindowName);

	void CreateCaptionEx(
		LPCTSTR lpszWindowName,
		LPCTSTR lpszFont = DEFAULT_FONT,
		COLORREF nFkColor = COLOR_LABLE_FG,
		int nHeight = DEFAULT_HEIGHT,
		int nWeight = FW_BOLD,
		BYTE bItalic = DEFAULT_ITALIC,
		BYTE bUnderline = false
		);
	void SetWindowCaption(LPCTSTR lpszWindowName);

	void SetHaveHead(BOOL bHaveHead){m_bHaveHead = bHaveHead;}

private:
	CColorStatic m_StaticCaption;
	CColorStatic m_StaticCaption2;
	CColorStatic m_StaticCaptionEx;
	CColorStatic m_StaticCaptionEx2;
	BOOL m_bHaveHead;
	CString m_sCaption;
	BOOL m_bMoveParent;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMainDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CPasseckFrame : public CWnd
{
public:
	void AddDialog(CPasseckDialog* pDialog, UINT nID, LPCTSTR lpszWindowName);
};
#endif // CPASSECKDIALOG_H