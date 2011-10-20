#if !defined(AFX_PESCAN_H__E0332456_BF90_4B9A_9BBB_341E4DC41FF4__INCLUDED_)
#define AFX_PESCAN_H__E0332456_BF90_4B9A_9BBB_341E4DC41FF4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PeScan.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPeScan dialog

#include "ImportTable.h"
#include "ExportTable.h"
class CPeScan : public CDialog
{
// Construction
public:
	CPeScan(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPeScan)
	enum { IDD = IDD_DIALOG3 };
	CListCtrl	m_SecList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPeScan)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
    public:
        //�����ļ�·��
        char m_FilePath[200];

        //�����ļ�ӳ���ַ
        char* m_pFile;
        //����PE���ṹ
        PIMAGE_DOS_HEADER m_pDos;
        PIMAGE_FILE_HEADER m_pFileHeader;
        PIMAGE_OPTIONAL_HEADER m_pOption;
        PIMAGE_SECTION_HEADER m_Section;

    public:
        //���PE���������Ϣ ���ж��Ƿ�����ЧPE
        BOOL GetBasicPEInfo();
        //ӳ��PE�ļ�
        BOOL MapPEFile();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPeScan)
	virtual void OnOK();
	afx_msg void OnSelect();
	virtual BOOL OnInitDialog();
	afx_msg void OnImport();
	afx_msg void OnExport();
    afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PESCAN_H__E0332456_BF90_4B9A_9BBB_341E4DC41FF4__INCLUDED_)
