#if !defined(AFX_VIEWBP_H__B1FAE2B3_DB9C_4D81_8297_487170169803__INCLUDED_)
#define AFX_VIEWBP_H__B1FAE2B3_DB9C_4D81_8297_487170169803__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewBP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CViewBP dialog

class CViewBP : public CDialog
{
// Construction
public:
	CViewBP(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CViewBP)
	enum { IDD = IDD_DIALOG2 };
	CListCtrl	m_bpList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewBP)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
    public:
        //ö�ٵ�ǰINT3�ϵ�
        void ListBp();
        //ö�ٵ�ǰ����Ӳ���ϵ�
        void ListHardBp();
        //INT3�ϵ����
        void Int3UIinit();
        //Ӳ���ϵ����
        void HardUIinit();
        //�ڴ�ϵ����
        void MemUIinit();
        //ö�ٵ�ǰ�ڴ�ϵ�
        void ListMemBp();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CViewBP)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnInt3();
	afx_msg void OnHard();
	afx_msg void OnMemory();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWBP_H__B1FAE2B3_DB9C_4D81_8297_487170169803__INCLUDED_)
