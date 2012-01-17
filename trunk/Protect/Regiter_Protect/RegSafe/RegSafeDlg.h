// RegSafeDlg.h : header file
//

#if !defined(AFX_REGSAFEDLG_H__844E4167_4651_40A8_8280_E670EB788189__INCLUDED_)
#define AFX_REGSAFEDLG_H__844E4167_4651_40A8_8280_E670EB788189__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRegSafeDlg dialog

#include"colorbutton.h"//编译按钮类
//编译要添加的页类
#include"page1.h"
#include"page2.h"
#include"page3.h"
#include"page4.h"

class CRegSafeDlg : public CDialog
{
// Construction
public:
	//设置主对话框
	CRegSafeDlg(CWnd* pParent = NULL);// standard constructor
    ~CRegSafeDlg();
	CBitmap m_bmpBackground;
  //添加对话框页
    void SetWizButton(UINT uFlag);
	void ShowPage(UINT nPos);
	void AddPage(CDialog* pDialog, UINT nID);
	//CWizard(CWnd* pParent = NULL);   // standard constructor
	CRect rectPage; //每页显示的框
	UINT nPageCount;//页的总数
	UINT nCurrentPage; //显示的当前页
 
// Dialog Data
	//{{AFX_DATA(CRegSafeDlg)
	enum { IDD = IDD_REGSAFE_DIALOG };
	
	CBitmapButton	m_Min;
    CBitmapButton	m_Close;
	//}}AFX_DATA
    HICON m_yIcon;
	CBitmapButton MyTitle;
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegSafeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
    CColorButton m_setie;//定义按钮指针
    CColorButton m_setie2;
    CColorButton m_setie3;
    CColorButton m_setie4;


	//定义显示对话框的结构
typedef struct PAGELINK{
		UINT nNum;
		CDialog* pDialog;
		struct PAGELINK* Next;
	};
	PAGELINK* pPageLink; //用来链接所有的页

	// Generated message map functions
	//{{AFX_MSG(CRegSafeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetie();
	afx_msg void OnMin();
	afx_msg void OnSetie2();
	virtual void OnOK();
	afx_msg void OnSetie3();
	afx_msg void OnSetie4();
	afx_msg void OnTitle();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGSAFEDLG_H__844E4167_4651_40A8_8280_E670EB788189__INCLUDED_)
