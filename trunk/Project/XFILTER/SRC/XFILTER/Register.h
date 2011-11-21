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

#define MESSAGE_REGISTER_ERROR		_T("ע��û�гɹ������Ժ����ԡ�")
#define MESSAGE_REGISTER_SUCCESS	_T("ע��ɹ���")
#define MESSAGE_REGISTER_NOINPUT	_T("�������һ���û����顣")

static TCHAR* REGISTER_OS[] = {
	_T(""),
	_T("����"),
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
	_T("����"),
	_T("���˰���"),
	_T("ѧϰ"),
	_T("��ɹ�˾����"),
	_T("�ο�"),
	_T("���ĳ����������"),
	_T("��߼���ˮƽ"),
};
#define REGISTER_USED_SOURCE_CODE_COUNT		sizeof(REGISTER_USED_SOURCE_CODE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_SOFTWARE[] = {
	_T(""),
	_T("< 10 Ԫ"),
	_T("10 - 30 Ԫ"),
	_T("30 - 50 Ԫ"),
	_T("50 - 80 Ԫ"),
	_T("80 - 100 Ԫ"),
	_T("> 100 Ԫ"),
};
#define REGISTER_PAYFOR_SOFTWARE_COUNT		sizeof(REGISTER_PAYFOR_SOFTWARE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_SOURCECODE[] = {
	_T(""),
	_T("< 50 Ԫ"),
	_T("50 - 100 Ԫ"),
	_T("100 - 150 Ԫ"),
	_T("150 - 200 Ԫ"),
	_T("200 - 300 Ԫ"),
	_T("300 - 500 Ԫ"),
	_T("500 - 1000 Ԫ"),
	_T("> 1000 Ԫ"),
};
#define REGISTER_PAYFOR_SOURCECODE_COUNT	sizeof(REGISTER_PAYFOR_SOURCECODE)/sizeof(TCHAR*)

static TCHAR* REGISTER_PAYFOR_WAY[] = {
	_T(""),
	_T("����"),
	_T("�г�ֱ�ӹ���"),
	_T("��������֧��"),
	_T("����ת��"),
	_T("���е��/ת��"),
	_T("  ��������"),
	_T("  ��������"),
	_T("  ��������"),
	_T("  �й�����"),
	_T("  ��ͨ����"),
	_T("  ��������"),
	_T("�ʾֻ��"),
};
#define REGISTER_PAYFOR_WAY_COUNT	sizeof(REGISTER_PAYFOR_WAY)/sizeof(TCHAR*)

static TCHAR* REGISTER_SOFTWARE_REQUEST[] = {
	_T(""),
	_T("����"),
	_T("������"),
	_T("��ľ��"),
	_T("���ڿ͹���"),
	_T("���������ʼ�"),
	_T("��ɫ��"),
	_T("�Ʒ�"),
	_T("�������"),
};
#define REGISTER_SOFTWARE_REQUEST_COUNT	sizeof(REGISTER_SOFTWARE_REQUEST)/sizeof(TCHAR*)

static TCHAR* REGISTER_GET_WAY[] = {
	_T(""),
	_T("����"),
	_T("������"),
	_T("��̳"),
	_T("������"),
	_T("������"),
	_T("�����ʼ�"),
	_T("�������緽ʽ"),
	_T("����/��־"),
	_T("���ѽ���"),
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
	_T("��"),
	_T("Ů")
};

static TCHAR *GUI_ID_TYPE[]		= {
	_T(""),
	_T("����"),
	_T("���֤"),
	_T("ѧ��֤"),
	_T("����֤")
};

static TCHAR *GUI_DEGREE[]		= {
	_T(""),
	_T("����"),
	_T("Сѧ"),
	_T("����"),
	_T("����"),
	_T("��ר"),
	_T("��ר"),
	_T("��ѧ"),
	_T("˶ʿ"),
	_T("��ʿ")
};

static TCHAR *GUI_METIER[]		= {
	_T(""),
	_T("����"),
	_T("�����ҵ"),
	_T("ѧ��"),
	_T("��ʦ"),
	_T("����"),
	_T("����ҵ"),
	_T("��ҵ"),
	_T("ũҵ"),
	_T("����ҵ"),
	_T("����ҵ"),
	_T("����ҵ"),
	_T("���ҵ"),
	_T("����"),
	_T("����"),
	_T("��������"),
	_T("ҽ��")
};

static TCHAR *GUI_SALARY[]		= {
	_T(""),
	_T("��"),
	_T("1000 ����"),
	_T("1000 - 2500"),
	_T("2500 - 4000"),
	_T("4000 - 6000"),
	_T("6000 - 8000"),
	_T("8000 - 10000"),
	_T("10000 ����")
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
