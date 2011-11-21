#if !defined(AFX_REGISTER_H__27EA90DB_7B57_430B_83E2_03DA6D66B3E3__INCLUDED_)
#define AFX_REGISTER_H__27EA90DB_7B57_430B_83E2_03DA6D66B3E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Register.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegister dialog

#include "Internet.h"

#define MESSAGE_REGISTER_ERROR		_T("注册没有成功，请稍后再试。")
#define MESSAGE_REGISTER_SUCCESS	_T("注册成功。")
#define MESSAGE_REGISTER_NOINPUT	_T("请配合做一下用户调查。")

static TCHAR* REGISTER_OS[] = {
	_T(""),
	_T("其它"),
	_T("Windows 95"),
	_T("Windows 98"),
	_T("Windows Me"),
	_T("Windows NT Workstation"),
	_T("Windows NT Server"),
	_T("Windows 2000 Professional"),
	_T("Windows 2000 Server"),
	_T("Windows 2000 Advance Server"),
	_T("Windows XP Professional"),
	_T("Linux"),
	_T("Unix"),
};
#define REGISTER_OS_COUNT		sizeof(REGISTER_OS)/sizeof(TCHAR*)

static TCHAR* REGISTER_USED_SOURCE_CODE[] = {
	_T(""),
	_T("其它"),
	_T("个人爱好"),
	_T("学习"),
	_T("完成公司任务"),
	_T("参考"),
	_T("解决某个技术问题"),
	_T("提高技术水平"),
};
#define REGISTER_USED_SOURCE_CODE_COUNT		sizeof(REGISTER_USED_SOURCE_CODE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_SOFTWARE[] = {
	_T(""),
	_T("< 10 元"),
	_T("10 - 30 元"),
	_T("30 - 50 元"),
	_T("50 - 80 元"),
	_T("80 - 100 元"),
	_T("> 100 元"),
};
#define REGISTER_PAYFOR_SOFTWARE_COUNT		sizeof(REGISTER_PAYFOR_SOFTWARE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_SOURCECODE[] = {
	_T(""),
	_T("< 50 元"),
	_T("50 - 100 元"),
	_T("100 - 150 元"),
	_T("150 - 200 元"),
	_T("200 - 300 元"),
	_T("300 - 500 元"),
	_T("500 - 1000 元"),
	_T("> 1000 元"),
};
#define REGISTER_PAYFOR_SOURCECODE_COUNT	sizeof(REGISTER_PAYFOR_SOURCECODE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_WAY[] = {
	_T(""),
	_T("其他"),
	_T("市场直接购买"),
	_T("网上在线支付"),
	_T("网上转账"),
	_T("银行电汇/转账"),
	_T("  招商银行"),
	_T("  建设银行"),
	_T("  工商银行"),
	_T("  中国银行"),
	_T("  交通银行"),
	_T("  其他银行"),
	_T("邮局汇款"),
};
#define REGISTER_PAYFOR_WAY_COUNT	sizeof(REGISTER_PAYFOR_WAY)/sizeof(TCHAR*)

static TCHAR* REGISTER_SOFTWARE_REQUEST[] = {
	_T(""),
	_T("其他"),
	_T("防病毒"),
	_T("防木马"),
	_T("防黑客攻击"),
	_T("过滤垃圾邮件"),
	_T("防色情"),
	_T("计费"),
	_T("封包监视"),
};
#define REGISTER_SOFTWARE_REQUEST_COUNT	sizeof(REGISTER_SOFTWARE_REQUEST)/sizeof(TCHAR*)

static TCHAR* REGISTER_GET_WAY[] = {
	_T(""),
	_T("其他"),
	_T("网络广告"),
	_T("论坛"),
	_T("新闻组"),
	_T("聊天室"),
	_T("电子邮件"),
	_T("其他网络方式"),
	_T("报刊/杂志"),
	_T("朋友介绍"),
};
#define REGISTER_GET_WAY_COUNT	sizeof(REGISTER_GET_WAY)/sizeof(TCHAR*)

static PVOID REGISTER_STRING_ARRAY[] = {
	(PVOID)REGISTER_PAYFOR_SOFTWARE,
	(PVOID)REGISTER_PAYFOR_SOFTWARE,
	(PVOID)REGISTER_PAYFOR_WAY,
	(PVOID)REGISTER_SOFTWARE_REQUEST,
	(PVOID)REGISTER_PAYFOR_SOURCECODE,
	(PVOID)REGISTER_PAYFOR_SOURCECODE,
	(PVOID)REGISTER_GET_WAY,
	(PVOID)REGISTER_OS,
	(PVOID)REGISTER_USED_SOURCE_CODE,
};

static UINT REGISTER_STRING_COUNT_ARRAY[] = {
	REGISTER_PAYFOR_SOFTWARE_COUNT,
	REGISTER_PAYFOR_SOFTWARE_COUNT,
	REGISTER_PAYFOR_WAY_COUNT,
	REGISTER_SOFTWARE_REQUEST_COUNT,
	REGISTER_PAYFOR_SOURCECODE_COUNT,
	REGISTER_PAYFOR_SOURCECODE_COUNT,
	REGISTER_GET_WAY_COUNT,
	REGISTER_OS_COUNT,
	REGISTER_USED_SOURCE_CODE_COUNT, 
};

static TCHAR *GUI_GENDER[]		= {
	_T("男"),
	_T("女")
};

static TCHAR *GUI_ID_TYPE[]		= {
	_T(""),
	_T("其他"),
	_T("身份证"),
	_T("学生证"),
	_T("军官证")
};

static TCHAR *GUI_DEGREE[]		= {
	_T(""),
	_T("其他"),
	_T("小学"),
	_T("初中"),
	_T("高中"),
	_T("中专"),
	_T("大专"),
	_T("大学"),
	_T("硕士"),
	_T("博士")
};

static TCHAR *GUI_METIER[]		= {
	_T(""),
	_T("其他"),
	_T("计算机业"),
	_T("学生"),
	_T("教师"),
	_T("军人"),
	_T("娱乐业"),
	_T("商业"),
	_T("农业"),
	_T("服务业"),
	_T("制造业"),
	_T("金融业"),
	_T("广告业"),
	_T("销售"),
	_T("法律"),
	_T("政府部门"),
	_T("医疗")
};

static TCHAR *GUI_SALARY[]		= {
	_T(""),
	_T("无"),
	_T("1000 以下"),
	_T("1000 - 2500"),
	_T("2500 - 4000"),
	_T("4000 - 6000"),
	_T("6000 - 8000"),
	_T("8000 - 10000"),
	_T("10000 以上")
};

#define IDC_OS_INDEX		7
static UINT REGISTER_COMBO_ARRAY[] = {
	IDC_PAYFOR_FIREWALL, 
	IDC_PAYFOR_ANTIVIRUS, 
	IDC_PAYFOR_WAY,
	IDC_SOFTWARE_REQUEST,
	IDC_PAYFOR_SORUCECODE,
	IDC_PAYFOR_DOCUMENTS,
	IDC_GET_WAY,
	IDC_OS,
	IDC_USED_SOURCE,
};
#define REGISTER_COMBO_ARRAY_COUNT sizeof(REGISTER_COMBO_ARRAY)/sizeof(TCHAR*)

#define IDC_CHECK_OFFEN_TO_WEB_INDEX	7
static UINT REGISTER_CHECK_ARRAY[] = {
	IDC_CHECK_PAYFOR_FIREWALL,
	IDC_CHECK_PAYFOR_ANTIVIRUS, 
	IDC_CHECK_PAYFOR_WAY,
	IDC_CHECK_SOFTWARE_REQUEST,
	IDC_CHECK_PAYFOR_SORUCECODE,
	IDC_CHECK_PAYFOR_DOCUMENTS,
	IDC_CHECK_GET_WAY,
	IDC_CHECK_OFFEN_TO_WEB,
	IDC_USE_FIREWALL,
	IDC_USE_ANTIVIRUS,
	IDC_HACK,
	IDC_VIRUS,
	IDC_LIKE_NET_TEC,
	IDC_CAN_PAYFOR_BOOK,
};
#define REGISTER_CHECK_ARRAY_COUNT sizeof(REGISTER_CHECK_ARRAY)/sizeof(TCHAR*)

class CRegister : public CDialog
{
// Construction
public:
	CRegister(CHttpRequest* pInternet, CWnd* pParent = NULL);   // standard constructor
	void InitView(PXUSER_INFO pUserInfo);

private:
	XUSER_INFO	m_UserInfo;
	CHttpRequest* m_pInternet;
	CComboBox	m_Combo[REGISTER_COMBO_ARRAY_COUNT];
	CButton		m_Check[REGISTER_CHECK_ARRAY_COUNT];
	CEdit		m_EditOffenToWeb;
	CEdit		m_EditCity;
	CEdit		m_EditRecommender;
	CColorStatic m_StaticAd;
	CHyperLink	m_HyperLink;

// Dialog Data
	//{{AFX_DATA(CRegister)
	enum { IDD = IDD_DIALOG_REGISTER };
	CEdit	m_EditQq;
	CEdit	m_EditInc;
	CDateTimeCtrl	m_TimeBirthday;
	CEdit	m_EditZip;
	CEdit	m_EditName;
	CEdit	m_EditEmail;
	CEdit	m_EditAddress;
	CEdit	m_EditDuty;
	CComboBox	m_ComboGender;
	CComboBox	m_ComboSalary;
	CComboBox	m_ComboMetier;
	CComboBox	m_ComboDegree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegister)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRegister)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCheckGetWay();
	afx_msg void OnCheckPayforAntivirus();
	afx_msg void OnCheckPayforDocuments();
	afx_msg void OnCheckPayforFirewall();
	afx_msg void OnCheckPayforSorucecode();
	afx_msg void OnCheckPayforWay();
	afx_msg void OnCheckSoftwareRequest();
	afx_msg void OnCheckOffenToWeb();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//---------------------------------------------------------------------------------------------
//owner declare

public:
	void		InitDlgResource();
	void		InitCombo(CComboBox *combo, TCHAR *string[], int count, int iSelected);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTER_H__27EA90DB_7B57_430B_83E2_03DA6D66B3E3__INCLUDED_)
