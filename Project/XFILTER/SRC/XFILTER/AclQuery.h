#if !defined(AFX_ACLQUERY_H__A758C292_72F8_4558_8224_F7DB1CA2EF41__INCLUDED_)
#define AFX_ACLQUERY_H__A758C292_72F8_4558_8224_F7DB1CA2EF41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclQuery.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclQuery dialog

#define ACL_QUERY_CAPTION	_T("Ñ¯ÎÊ")

#define ACL_QUERY_LABEL_INFO		0	
#define ACL_QUERY_LABEL_INFO2		1	
#define ACL_QUERY_LABEL_EXPLAIN		2
static UINT ACL_QUERY_LABEL[] = {
	IDC_LABEL_INFO,
	IDC_LABEL_INFO2,
	IDC_LABEL_EXPLAIN,
	IDC_FRAME_ACTION,
};
#define ACL_QUERY_LABEL_COUNT	sizeof(ACL_QUERY_LABEL)/sizeof(UINT)

#define ACL_QUERY_BUTTON_MIN		0
#define ACL_QUERY_BUTTON_CLOSE		1
#define ACL_QUERY_BUTTON_OK			2
#define ACL_QUERY_BUTTON_CANCEL		3
#define ACL_QUERY_BUTTON_ADVANCE	4
static UINT ACL_QUERY_BUTTON[] = {
	IDC_MIN,
	IDC_CLOSE,
	IDOK,
	IDCANCEL,
	IDC_ADVANCE,
};
#define ACL_QUERY_BUTTON_COUNT sizeof(ACL_QUERY_BUTTON)/sizeof(UINT)

static UINT ACL_QUERY_RADIO[] = {
	IDC_RADIO_DENY,
	IDC_RADIO_PASS,
	IDC_RADIO_DENY_AND_ADD_ACL,
	IDC_RADIO_PASS_AND_ADD_ACL,
};
#define ACL_QUERY_RADIO_COUNT sizeof(ACL_QUERY_RADIO)/sizeof(UINT)

class CAclQuery : public CPasseckDialog
{
// Construction
public:
	CAclQuery(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAclQuery();
	BYTE GetAction(){return m_bAction;}
	void SetSession(PSESSION pSession){m_Session = *pSession;}
	void SetPacket(PPACKET_BUFFER pPacket);

private:
	void MakeAcl();
	void MakeAction(BYTE bAction);
	void MakeList();
	CString MakeExplain();
	void AddAppList();
	void SetCaption(CString pString, CString sCaptionInfo);

private:
	CBkStatic		m_LabelBk;
	CColorStatic	m_Label[ACL_QUERY_LABEL_COUNT];
	CButtonST		m_Button[ACL_QUERY_BUTTON_COUNT];
	CButton			m_Radio[ACL_QUERY_RADIO_COUNT];
	CListCtrl		m_List;
	CButton			m_Check;
	CStatic			m_AppIcon;
	HICON			m_hIcon;

	CBrush			m_QueryBursh;
	CBitmap			m_BitmapBk;
	CDC				m_memDC;
	BOOL			m_bIsFirst;

	BYTE	m_bAction;
	SESSION m_Session;

	XACL		m_Acl;
	XACL_WEB	m_AclWeb;
	XACL_NNB	m_AclNnb;
	XACL_ICMP	m_AclIcmp;


// Dialog Data
	//{{AFX_DATA(CAclQuery)
	enum { IDD = IDD_ACL_QUERY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclQuery)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclQuery)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnRadioDeny();
	afx_msg void OnRadioPass();
	afx_msg void OnRadioDenyAndAddAcl();
	afx_msg void OnRadioPassAndAddAcl();
	afx_msg void OnAdvance();
	afx_msg void OnMin();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLQUERY_H__A758C292_72F8_4558_8224_F7DB1CA2EF41__INCLUDED_)
