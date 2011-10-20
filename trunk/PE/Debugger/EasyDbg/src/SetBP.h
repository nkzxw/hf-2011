#if !defined(AFX_SETBP_H__A1B86FA5_4107_428B_965B_7376DBE59AB3__INCLUDED_)
#define AFX_SETBP_H__A1B86FA5_4107_428B_965B_7376DBE59AB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetBP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetBP dialog

class CSetBP : public CDialog
{
// Construction
public:
	CSetBP(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetBP)
	enum { IDD = IDD_BPSETING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetBP)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
    public:
        //�����û����õ�INT3��Ӳ���ϵ���ڴ�ϵ��ַ
        DWORD m_dwBpAddress;
        //�ڴ��Ӳ���ϵ�ĳ���
        DWORD m_dwLength;
        //��ʶ�ϵ�����
        DWORD m_Select;
        //Ӳ���ϵ������
        DWORD m_dwAttribute;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetBP)
	afx_msg void OnInt3();
	afx_msg void OnHard();
	afx_msg void OnMemory();
	afx_msg void OnOk();
	afx_msg void OnCancle();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETBP_H__A1B86FA5_4107_428B_965B_7376DBE59AB3__INCLUDED_)
