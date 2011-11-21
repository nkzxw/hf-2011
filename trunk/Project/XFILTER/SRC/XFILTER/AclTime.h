#if !defined(AFX_ACLTIME_H__BEF1F95B_53F0_46D6_AAF0_F76CA0526910__INCLUDED_)
#define AFX_ACLTIME_H__BEF1F95B_53F0_46D6_AAF0_F76CA0526910__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclTime.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclTime dialog
static UINT IDC_LABEL[] = {
	IDC_LABEL_WEEK,
	IDC_LABEL_0,
	IDC_LABEL_1,
	IDC_LABEL_2,
	IDC_LABEL_3,
	IDC_LABEL_4,
	IDC_LABEL_5,
	IDC_LABEL_6,
	IDC_LABEL_TIME,
	IDC_LABEL_TIME_START,
	IDC_LABEL_TIME_END
};
#define LABEL_COUNT sizeof(IDC_LABEL)/sizeof(UINT)

static UINT IDC_CHECK[] = {
	IDC_SET_TIME_CHECK_SUNDAY,
	IDC_SET_TIME_CHECK_MONDAY,
	IDC_SET_TIME_CHECK_TUESDAY,
	IDC_SET_TIME_CHECK_WEDNESDAY,
	IDC_SET_TIME_CHECK_THURSDAY,
	IDC_SET_TIME_CHECK_FRIDAY,
	IDC_SET_TIME_CHECK_SATURDAY
};
#define CHECK_COUNT sizeof(IDC_CHECK)/sizeof(UINT)

static UINT IDC_TIME[] = {
	IDC_SET_TIME_TIME_START,
	IDC_SET_TIME_TIME_END
};
#define TIME_COUNT sizeof(IDC_TIME)/sizeof(UINT)


class CAclTime : public CPasseckDialog
{
// Construction
public:
	CAclTime(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	BYTE GetButtonFlags(){return m_bButtonFlags;}

private:
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}
	BOOL SendMessageEx(BYTE bFlags);
	void AddHistory();
	void GetTime(PXACL_TIME pTime);
	int	SetAclValue(int iIndex,BOOL isTrue);
	int	SetTimeValue(int iType);
	int SetValue(void* acltime);

private:
	BYTE			m_bButtonFlags;
	CColorStatic	m_Label[LABEL_COUNT];
	CTreeCtrl		m_Tree;
	CButton			m_Check[CHECK_COUNT];
	CDateTimeCtrl	m_Time[TIME_COUNT];
	CWnd*			m_pParent;
	XACL_TIME		m_AclTime[ACL_TIME_TYPE_COUNT];
	CAclHistory		m_History;
	int				m_iTreeIndex;


// Dialog Data
	//{{AFX_DATA(CAclTime)
	enum { IDD = IDD_ACL_TIME };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclTime)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclTime)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSelchangedTimeTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetTimeCheckSunday();
	afx_msg void OnSetTimeCheckMonday();
	afx_msg void OnSetTimeCheckFriday();
	afx_msg void OnSetTimeCheckSaturday();
	afx_msg void OnSetTimeCheckThursday();
	afx_msg void OnSetTimeCheckTuesday();
	afx_msg void OnSetTimeCheckWednesday();
	afx_msg void OnDatetimechangeSetTimeTimeEnd(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeSetTimeTimeStart(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLTIME_H__BEF1F95B_53F0_46D6_AAF0_F76CA0526910__INCLUDED_)
