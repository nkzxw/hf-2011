#if !defined(AFX_PARAMETERSUB_H__96F27F52_9DD8_4525_87EF_6E0D4E6F4566__INCLUDED_)
#define AFX_PARAMETERSUB_H__96F27F52_9DD8_4525_87EF_6E0D4E6F4566__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ParameterSub.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CParameterSub dialog

#define PARAMETER_CAPTION	_T("״ָ̬ʾ")

static TCHAR* PARAMETER_SECURITY[] = {
	_T("��(�����ϸ�Ŀعܹ���)"),
	_T("��(����һ��Ŀعܹ���)"),
	_T("��(��Ҫѯ��ʱĬ�Ϸ���)"),
};

static TCHAR* PARAMETER_FORMAT[] = {
	_T("����ʱ�䣺%s"),
	_T("����ʱ�䣺%s"),
	_T("��������%u"),
	_T("��������%u"),
	_T("�ܾ������%u"),
	_T("���з����%u"),
	_T("�ܹ���ģʽ��%s"),
	_T("Ӧ�ó���%s"),
	_T("��վ��%s"),
	_T("�����ھӣ�%s"),
	_T("ICMP(PING)��%s"),
	_T("��ȫ�ȼ���%s"),
};

static TCHAR* PARAMETER_WORK_MODE[] = {
	_T("����"),
	_T("������Ĭ��%s"),
	_T("˫���ֹ"),
	_T("��ֹ����"),
	_T("��ֹ����"),
};

#define PARAMETER_TYPE_IN_DATA			2
#define PARAMETER_TYPE_OUT_DATA			3
#define PARAMETER_TYPE_DENY_PACKETS		4
#define PARAMETER_TYPE_PASS_PACKETS		5

static UINT	PARAMETER_LABEL[] = {
	IDC_START_TIME, 
	IDC_REMAIN_TIME,
	IDC_IN_DATA,
	IDC_OUT_DATA,
	IDC_DENY_PACKETS,
	IDC_PASS_PACKETS,
	IDC_WORK_MODE,
	IDC_WORK_MODE_APP,
	IDC_WORK_MODE_WEB,
	IDC_WORK_MODE_NNB,
	IDC_WORK_MODE_ICMP,
	IDC_CURRENT_SECURITY,
};
#define PARAMETER_LABEL_COUNT	sizeof(PARAMETER_LABEL)/sizeof(UINT)


class CParameterSub : public CPasseckDialog
{
// Construction
public:
	CParameterSub(CWnd* pParent = NULL);   // standard constructor
	virtual ~CParameterSub();
	void Refresh();

private:
	CStatic m_Label[PARAMETER_LABEL_COUNT];
	DWORD	m_dwInData;
	DWORD	m_dwOutData;
	DWORD	m_dwDenyPackets;
	DWORD	m_dwPassPackets;
	CTime	m_tStartTime;
	TCHAR	m_sSpace[100];
	CBrush	m_hbr;

	CSliderCtrl m_Slider;
	COsilloGraph m_OsilloGraph;

// Dialog Data
	//{{AFX_DATA(CParameterSub)
	enum { IDD = IDD_PARAMETER_SUB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParameterSub)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CParameterSub)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	afx_msg LRESULT UpdateData(WPARAM bType, LPARAM dwData);
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMETERSUB_H__96F27F52_9DD8_4525_87EF_6E0D4E6F4566__INCLUDED_)
