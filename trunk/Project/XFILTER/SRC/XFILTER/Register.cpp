// Register.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "Register.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegister dialog

CRegister::CRegister(CHttpRequest* pInternet, CWnd* pParent /*=NULL*/)
	: CDialog(CRegister::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRegister)
	//}}AFX_DATA_INIT

	m_pInternet = pInternet;
	memset(&m_UserInfo, 0, sizeof(m_UserInfo));
}


void CRegister::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegister)
	DDX_Control(pDX, IDC_REG_EDIT_QQ, m_EditQq);
	DDX_Control(pDX, IDC_REG_EDIT_INC, m_EditInc);
	DDX_Control(pDX, IDC_REG_TIME_BIRTHDAY, m_TimeBirthday);
	DDX_Control(pDX, IDC_REG_EDIT_ZIP, m_EditZip);
	DDX_Control(pDX, IDC_REG_EDIT_NAME, m_EditName);
	DDX_Control(pDX, IDC_REG_EDIT_EMAIL, m_EditEmail);
	DDX_Control(pDX, IDC_REG_EDIT_ADDRESS, m_EditAddress);
	DDX_Control(pDX, IDC_REG_EDIT_DUTY, m_EditDuty);
	DDX_Control(pDX, IDC_REG_COMBO_GENDER, m_ComboGender);
	DDX_Control(pDX, IDC_REG_COMBO_SALARY, m_ComboSalary);
	DDX_Control(pDX, IDC_REG_COMBO_METIER, m_ComboMetier);
	DDX_Control(pDX, IDC_REG_COMBO_DEGREE, m_ComboDegree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegister, CDialog)
	//{{AFX_MSG_MAP(CRegister)
	ON_BN_CLICKED(IDC_CHECK_GET_WAY, OnCheckGetWay)
	ON_BN_CLICKED(IDC_CHECK_PAYFOR_ANTIVIRUS, OnCheckPayforAntivirus)
	ON_BN_CLICKED(IDC_CHECK_PAYFOR_DOCUMENTS, OnCheckPayforDocuments)
	ON_BN_CLICKED(IDC_CHECK_PAYFOR_FIREWALL, OnCheckPayforFirewall)
	ON_BN_CLICKED(IDC_CHECK_PAYFOR_SORUCECODE, OnCheckPayforSorucecode)
	ON_BN_CLICKED(IDC_CHECK_PAYFOR_WAY, OnCheckPayforWay)
	ON_BN_CLICKED(IDC_CHECK_SOFTWARE_REQUEST, OnCheckSoftwareRequest)
	ON_BN_CLICKED(IDC_CHECK_OFFEN_TO_WEB, OnCheckOffenToWeb)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegister message handlers

BOOL CRegister::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	InitDlgResource();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//---------------------------------------------------------------------------------------------
//initialize dialog's resource

void CRegister::InitDlgResource()
{
	ModifyStyleEx(WS_EX_TOOLWINDOW, WS_EX_APPWINDOW); 

	int i = 0;
	for(i; i < REGISTER_COMBO_ARRAY_COUNT; i++)
	{
		m_Combo[i].SubclassDlgItem(REGISTER_COMBO_ARRAY[i], this);
		InitCombo(&m_Combo[i], (char**)REGISTER_STRING_ARRAY[i], REGISTER_STRING_COUNT_ARRAY[i], 0);
	}
	for(i = 0; i < REGISTER_CHECK_ARRAY_COUNT; i++)
		m_Check[i].SubclassDlgItem(REGISTER_CHECK_ARRAY[i], this);
	m_EditOffenToWeb.SubclassDlgItem(IDC_EDIT_OFFEN_TO_WEB, this);
	m_EditOffenToWeb.SetLimitText(50);

	m_EditCity.SubclassDlgItem(IDC_REG_EDIT_CITY, this);
	m_EditCity.SetLimitText(20);

	m_EditRecommender.SubclassDlgItem(IDC_REG_EDIT_EMAIL2, this);
	m_EditRecommender.SetLimitText(50);

	m_StaticAd.SubclassDlgItem(IDC_AD, this);
	m_StaticAd.SetColor(COLOR_BLUE);

	m_HyperLink.SubclassDlgItem(IDC_WEB, this);

	InitCombo(&m_ComboGender, GUI_GENDER,  (sizeof(GUI_GENDER)  / sizeof(TCHAR*)), 0);
	InitCombo(&m_ComboDegree, GUI_DEGREE,  (sizeof(GUI_DEGREE)  / sizeof(TCHAR*)), 0);
	InitCombo(&m_ComboMetier, GUI_METIER,  (sizeof(GUI_METIER)  / sizeof(TCHAR*)), 0);
	InitCombo(&m_ComboSalary, GUI_SALARY,  (sizeof(GUI_SALARY)  / sizeof(TCHAR*)), 0);

	m_EditEmail.SetLimitText(50);
	m_EditName.SetLimitText(20);
	m_EditDuty.SetLimitText(20);
	m_EditZip.SetLimitText(6);
	m_EditAddress.SetLimitText(50);
	m_EditQq.SetLimitText(12);
	m_EditInc.SetLimitText(50);

	if(m_pInternet != NULL)
		m_UserInfo = m_pInternet->m_UserInfo;
	InitView(&m_UserInfo);
}

void CRegister::InitCombo(CComboBox *combo, TCHAR *string[], int count, int iSelected)
{
	for(int iIndex = 0; iIndex < count; iIndex ++)
		combo->InsertString(iIndex, string[iIndex]);
	combo->SetCurSel(iSelected);
}

void CRegister::InitView(PXUSER_INFO pUserInfo)
{
	if(pUserInfo == NULL)
		return;
	int i = 0;
	for(i; i < REGISTER_CHECK_ARRAY_COUNT; i++)
		m_Check[i].SetCheck(pUserInfo->bCheck[i]);
	for(i = 0; i < REGISTER_COMBO_ARRAY_COUNT; i++)
	{
		m_Combo[i].SetCurSel(pUserInfo->bCombo[i]);
		if(i < IDC_OS_INDEX)
			m_Combo[i].ShowWindow(pUserInfo->bCheck[i]);
	}
	m_EditOffenToWeb.SetWindowText(pUserInfo->sOffenToWeb);
	m_EditOffenToWeb.ShowWindow(pUserInfo->bCheck[IDC_CHECK_OFFEN_TO_WEB_INDEX]);

	m_EditCity.SetWindowText(pUserInfo->sCity);

	m_TimeBirthday.SetTime(&CTime(pUserInfo->tBirthday));

	m_ComboGender.SetCurSel(pUserInfo->bGender);
	m_ComboDegree.SetCurSel(pUserInfo->bDegree);
	m_ComboMetier.SetCurSel(pUserInfo->bMetier);
	m_ComboSalary.SetCurSel(pUserInfo->bSalary);

	m_EditName.SetWindowText(pUserInfo->sName);
	m_EditDuty.SetWindowText(pUserInfo->sDuty);
	m_EditZip.SetWindowText(pUserInfo->sZip);
	m_EditAddress.SetWindowText(pUserInfo->sAddress);
	m_EditQq.SetWindowText(pUserInfo->sQQ);
	m_EditInc.SetWindowText(pUserInfo->sInc);
	m_EditEmail.SetWindowText(pUserInfo->sEmail);

	m_EditRecommender.SetWindowText(pUserInfo->Recommender);
}

void CRegister::OnOK() 
{
	CString tmpStr = "";

	m_EditRecommender.GetWindowText(tmpStr);
	if(tmpStr != "")
	{
		if(tmpStr.GetLength() == 0 || tmpStr.Find('@') == -1
			|| tmpStr.Left(1) == '@' || tmpStr.Right(1) == '@')
		{
			AfxMessageBox(GUI_REG_MESSAGE_MUST_INPUT_EMAIL2);
			m_EditRecommender.SetFocus();
			return;
		}
	}
	strcpy(m_UserInfo.Recommender, tmpStr);

	m_EditEmail.GetWindowText(tmpStr);
	if(tmpStr.GetLength() == 0 || tmpStr.Find('@') == -1
		|| tmpStr.Left(1) == '@' || tmpStr.Right(1) == '@')
	{
		AfxMessageBox(GUI_REG_MESSAGE_MUST_INPUT_EMAIL);
		m_EditEmail.SetFocus();
		return;
	}
	_tcscpy(m_UserInfo.sEmail, tmpStr);

	m_EditCity.GetWindowText(tmpStr);
	if(tmpStr.GetLength() == 0)
	{
		AfxMessageBox(GUI_REG_MESSAGE_MUST_INPUT_CITY);
		m_EditCity.SetFocus();
		return;
	}
	_tcscpy(m_UserInfo.sCity, tmpStr);

	int i = 0;
	for(i; i < REGISTER_CHECK_ARRAY_COUNT; i++)
		m_UserInfo.bCheck[i] = m_Check[i].GetCheck();
	for(i = 0; i < REGISTER_COMBO_ARRAY_COUNT; i++)
		m_UserInfo.bCombo[i] = m_Combo[i].GetCurSel();
	m_EditOffenToWeb.GetWindowText(tmpStr);
	_tcscpy(m_UserInfo.sOffenToWeb, tmpStr);

	CString sName, sQQ, sInc, sDuty, sZip, sAddress;

	m_EditName.GetWindowText(sName);
	m_EditDuty.GetWindowText(sDuty);
	m_EditZip.GetWindowText(sZip);
	m_EditAddress.GetWindowText(sAddress);
	m_EditQq.GetWindowText(sQQ);
	m_EditInc.GetWindowText(sInc);

	_tcscpy(m_UserInfo.sName, sName);
	_tcscpy(m_UserInfo.sDuty, sDuty);
	_tcscpy(m_UserInfo.sZip, sZip);
	_tcscpy(m_UserInfo.sAddress, sAddress);
	_tcscpy(m_UserInfo.sQQ, sQQ);
	_tcscpy(m_UserInfo.sInc, sInc);

	m_UserInfo.bGender = m_ComboGender.GetCurSel();
	m_UserInfo.bDegree = m_ComboDegree.GetCurSel();
	m_UserInfo.bMetier = m_ComboMetier.GetCurSel();
	m_UserInfo.bSalary = m_ComboSalary.GetCurSel();

	m_TimeBirthday.GetTime(m_UserInfo.tBirthday);

	if(m_pInternet->GetIsEdit() && memcmp(&m_UserInfo, &m_pInternet->m_UserInfo, sizeof(m_UserInfo)) == 0)
	{
		m_pInternet->SetIsEdit(FALSE);
		CDialog::OnOK();
		return;
	}

	BYTE bCheck[14];
	memset(&bCheck, 0, sizeof(bCheck));
	if(memcmp(&bCheck, &m_UserInfo.bCheck, sizeof(bCheck)) == 0)
	{
		AfxMessageBox(MESSAGE_REGISTER_NOINPUT);
		m_Check[0].SetFocus();
		return;
	}

	if(m_Combo[IDC_OS_INDEX].GetCurSel() == 0)
	{
		AfxMessageBox(GUI_REG_MESSAGE_MUST_INPUT_OS);
		m_Combo[IDC_OS_INDEX].SetFocus();
		return;
	}

	m_pInternet->m_UserInfo = m_UserInfo;
	if(!m_pInternet->GetIsEdit())
		m_pInternet->m_UserInfo.iStatus = REG_STATUS_REGISTERING;
	m_pInternet->SetIsEdit(FALSE);
	m_pInternet->m_Install.SaveReg(REG_INFO_ITEM
		, (BYTE*)&m_pInternet->m_UserInfo, sizeof(XUSER_INFO));

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_pInternet->m_IsUploaded = m_pInternet->PreUpload(m_pInternet->m_IsSyn);
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	if(!m_pInternet->m_IsUploaded)
		AfxMessageBox(MESSAGE_REGISTER_ERROR);
	else if(m_pInternet->m_IsSyn)
		AfxMessageBox(MESSAGE_REGISTER_SUCCESS);

	CDialog::OnOK();
}

void CRegister::OnCheckPayforFirewall() 
{
	m_Combo[0].ShowWindow(m_Check[0].GetCheck());
}
void CRegister::OnCheckPayforAntivirus() 
{
	m_Combo[1].ShowWindow(m_Check[1].GetCheck());
}
void CRegister::OnCheckPayforWay() 
{
	m_Combo[2].ShowWindow(m_Check[2].GetCheck());
}
void CRegister::OnCheckSoftwareRequest() 
{
	m_Combo[3].ShowWindow(m_Check[3].GetCheck());
}
void CRegister::OnCheckPayforSorucecode() 
{
	m_Combo[4].ShowWindow(m_Check[4].GetCheck());
}
void CRegister::OnCheckPayforDocuments() 
{
	m_Combo[5].ShowWindow(m_Check[5].GetCheck());
}
void CRegister::OnCheckGetWay() 
{
	m_Combo[6].ShowWindow(m_Check[6].GetCheck());
}
void CRegister::OnCheckOffenToWeb() 
{
	m_EditOffenToWeb.ShowWindow(m_Check[IDC_CHECK_OFFEN_TO_WEB_INDEX].GetCheck());
}

#pragma comment( exestr, "B9D3B8FD2A7467696B757667742B")
