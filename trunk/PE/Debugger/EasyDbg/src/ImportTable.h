#if !defined(AFX_IMPORTTABLE_H__C5930AF1_9DBA_4787_9753_F07FDE714995__INCLUDED_)
#define AFX_IMPORTTABLE_H__C5930AF1_9DBA_4787_9753_F07FDE714995__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ImportTable.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportTable dialog

class CImportTable : public CDialog
{
// Construction
public:
	CImportTable(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportTable)
	enum { IDD = IDD_DIALOG4 };
	CListCtrl	m_FunList;
	CListCtrl	m_DllList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportTable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    public:
        //��ʼ������
        void UIinit();
        //��RVA�����ļ�ƫ��
        DWORD RvaToFileOffset(DWORD dwRva,DWORD dwSecNum,PIMAGE_SECTION_HEADER pSec);
        //ö�ٵ����DLL
        void ListDll();
        //�г�ĳһ��DLL�ĵ��뺯��
        void ListFun(PIMAGE_THUNK_DATA pThunK);
        //���PIMAGE_THUNK_DATA��ֵ������
         DWORD m_dwThunk[100];

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportTable)
	virtual BOOL OnInitDialog();
    virtual void OnOK();
	afx_msg void OnClickDll(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMPORTTABLE_H__C5930AF1_9DBA_4787_9753_F07FDE714995__INCLUDED_)
