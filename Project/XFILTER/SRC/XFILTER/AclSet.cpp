// AclSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclSet dialog


CAclSet::CAclSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	memset(&m_Acl, 0, sizeof(XACL));
	m_bIsChange = FALSE;
	m_bIsAutoPort = TRUE;
}

CAclSet::~CAclSet()
{
	if(m_hIcon != NULL)
		DestroyIcon(m_hIcon);
}


void CAclSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclSet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclSet, CDialog)
	//{{AFX_MSG_MAP(CAclSet)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON_APPLICATION, OnButtonApplication)
	ON_CBN_SELCHANGE(IDC_COMBO_SERVICE_TYPE, OnSelchangeComboServiceType)
	ON_CBN_SELCHANGE(IDC_COMBO_ACCESS_TIME, OnSelchangeComboAccessTime)
	ON_CBN_EDITCHANGE(IDC_COMBO_APPLICATION, OnEditchangeComboApplication)
	ON_CBN_SELCHANGE(IDC_COMBO_APPLICATION, OnSelchangeComboApplication)
	ON_CBN_SELCHANGE(IDC_COMBO_ACTION, OnSelchangeComboAction)
	ON_CBN_SELCHANGE(IDC_COMBO_DIRECTION, OnSelchangeComboDirection)
	ON_CBN_SELCHANGE(IDC_COMBO_REMOTE_NET, OnSelchangeComboRemoteNet)
	ON_EN_CHANGE(IDC_EDIT_LOCAL_PORT, OnChangeEditLocalPort)
	ON_EN_CHANGE(IDC_EDIT_MEMO, OnChangeEditMemo)
	ON_EN_CHANGE(IDC_EDIT_SERVICE_PORT, OnChangeEditServicePort)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclSet message handlers

BOOL CAclSet::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = 0;
	for(i; i < ACL_SET_LABEL_COUNT; i++)
		m_Label[i].SetLabelEx(ACL_SET_LABEL[i], this);
	for(i = 0; i < ACL_SET_EDIT_COUNT; i++)
		m_Edit[i].SubclassDlgItem(ACL_SET_EDIT[i], this);
	for(i = 0; i < ACL_SET_COMBO_COUNT; i++)
		m_Combo[i].SubclassDlgItem(ACL_SET_COMBO[i], this);
	for(i = 0; i < ACL_SET_BUTTON_COUNT; i++)
		m_Button[i].SetButton(ACL_SET_BUTTON[i], this);
	m_Label[ACL_SET_LABEL_EXPAIN].SetColor(COLOR_RED);

	m_hIcon = ExtractIcon(theApp.m_hInstance, m_Acl.sApplication, 0);
	if(m_hIcon == NULL)
		m_hIcon = theApp.LoadIcon(IDR_NULLAPP);
	m_AppIcon.SubclassDlgItem(IDC_APP_ICON, this);
	m_AppIcon.SetIcon(m_hIcon);

	InitDialog();
	ShowAcl();
	m_sExplain = MakeExplain();
	OnSelchangeComboServiceType();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclSet::InitDialog()
{
	m_Combo[ACL_SET_COMBO_APP].AddString("*");
	PXACL pAcl = theApp.m_AclFile.GetHeader()->pAcl;
	while(pAcl != NULL)
	{
		if(m_Combo[ACL_SET_COMBO_APP].FindString(0, pAcl->sApplication) == CB_ERR)
		{
			m_Combo[ACL_SET_COMBO_APP].InsertString(m_Combo[ACL_SET_COMBO_APP].GetCount(), pAcl->sApplication);
		}
		pAcl = pAcl->pNext;
	}

	AddComboStrings(&m_Combo[ACL_SET_COMBO_NET], ACL_NET_TYPE, ACL_NET_TYPE_COUNT);
	AddComboStrings(&m_Combo[ACL_SET_COMBO_TIME], ACL_TIME_TYPE, ACL_TIME_TYPE_COUNT);
	AddComboStrings(&m_Combo[ACL_SET_COMBO_DIR], GUI_DIRECTION, GUI_DIRECTION_COUNT);
	AddComboStrings(&m_Combo[ACL_SET_COMBO_ACTION], GUI_ACTION, GUI_ACTION_COUNT);
	AddComboStrings(&m_Combo[ACL_SET_COMBO_PROTOCOL], GUI_SERVICE_TYPE, GUI_SERVICE_TYPE_COUNT);

	m_Edit[ACL_SET_EDIT_LOCAL_PORT].SetLimitText(5);
	m_Edit[ACL_SET_EDIT_SERVICE_PORT].SetLimitText(5);
	m_Edit[ACL_SET_EDIT_MEMO].SetLimitText(50);
}

void CAclSet::ShowAcl()
{
	m_Combo[ACL_SET_COMBO_APP].SetWindowText(m_Acl.sApplication);
	m_Combo[ACL_SET_COMBO_NET].SetCurSel(m_Acl.bRemoteNetType);
	m_Combo[ACL_SET_COMBO_TIME].SetCurSel(m_Acl.bAccessTimeType);
	m_Combo[ACL_SET_COMBO_ACTION].SetCurSel(m_Acl.bAction);
	m_Combo[ACL_SET_COMBO_DIR].SetCurSel(m_Acl.bDirection);
	m_Combo[ACL_SET_COMBO_PROTOCOL].SetCurSel(m_Acl.bServiceType);

	SetPort(GUI_SERVICE_PORT_NUM[m_Acl.bServiceType]
		, GUI_SERVICE_ENABLE[m_Acl.bServiceType]
		, GUI_SERVICE_PORT_NUM[m_Acl.wLocalPort]
		, TRUE
		);

	m_Edit[ACL_SET_EDIT_MEMO].SetWindowText(m_Acl.sMemo);
}

void CAclSet::SetPort(WORD wLocalPort, BOOL bLocalIsEnable, WORD wRemotePort, BOOL bRemoteIsEnable)
{
	CString sLocalPort, sRemotePort;
	if(!m_bIsAutoPort)
	{
		wLocalPort = m_Acl.wLocalPort;
		wRemotePort = m_Acl.uiServicePort;
	}
	sLocalPort.Format("%u", wLocalPort);
	sRemotePort.Format("%u", wRemotePort);
	m_Edit[ACL_SET_EDIT_LOCAL_PORT].SetWindowText(sLocalPort);
	m_Edit[ACL_SET_EDIT_SERVICE_PORT].SetWindowText(sRemotePort);
	m_Edit[ACL_SET_EDIT_LOCAL_PORT].EnableWindow(bLocalIsEnable);
	m_Edit[ACL_SET_EDIT_SERVICE_PORT].EnableWindow(bRemoteIsEnable);
}

CString CAclSet::MakeExplain(BOOL bIsAppSelChange)
{
	CString sExplain;
	CString sCombo[ACL_SET_COMBO_COUNT];
	CString sEdit[ACL_SET_EDIT_COUNT];
	int i = 0;
	for(i; i < ACL_SET_COMBO_COUNT; i++)
	{
		m_Combo[i].GetWindowText(sCombo[i]);
	}
	for(i = 0; i < ACL_SET_EDIT_COUNT; i++)
	{
		m_Edit[i].GetWindowText(sEdit[i]);
	}

	if(bIsAppSelChange)
	{
		int nIndex = m_Combo[ACL_SET_COMBO_APP].GetCurSel();
		if(nIndex >= 0)
		{
			int nLen = m_Combo[ACL_SET_COMBO_APP].GetLBTextLen(nIndex);
			m_Combo[ACL_SET_COMBO_APP].GetLBText(
				nIndex, sCombo[ACL_SET_COMBO_APP].GetBuffer(nLen));
		}
	}
	
	if(m_Combo[ACL_SET_COMBO_DIR].GetCurSel() == ACL_DIRECTION_IN)
	{
		// _T("%s%s通过%s协议的%s端口向本机%s%s端口发出的连接请求将被%s。%s")
		sExplain.Format(GUI_ACL_EXPLAIN_OUT_FORMAT
			, sCombo[ACL_SET_COMBO_NET]
			, sCombo[ACL_SET_COMBO_TIME]
			, sCombo[ACL_SET_COMBO_PROTOCOL]
			, sEdit[ACL_SET_EDIT_SERVICE_PORT].Compare("0") == 0 ? GUI_ACL_EXPLAIN_PORT_ALL : sEdit[ACL_SET_EDIT_SERVICE_PORT] 
			, sCombo[ACL_SET_COMBO_APP].Compare("*") == 0 ? _T("") : sCombo[ACL_SET_COMBO_APP]
			, sEdit[ACL_SET_EDIT_LOCAL_PORT].Compare("0") == 0 ? GUI_ACL_EXPLAIN_PORT_ALL : sEdit[ACL_SET_EDIT_LOCAL_PORT] 
			, sCombo[ACL_SET_COMBO_ACTION]
			, GUI_ACL_EXPLAIN_CONST
			);
	}
	else
	{
		// _T("%s%s通过%s协议的%s端口访问%s的%s端口将被%s。%s")
		sExplain.Format(GUI_ACL_EXPLAIN_OUT_FORMAT
			, sCombo[ACL_SET_COMBO_APP].Compare("*") == 0 ? _T("") : sCombo[ACL_SET_COMBO_APP]
			, sCombo[ACL_SET_COMBO_TIME]
			, sCombo[ACL_SET_COMBO_PROTOCOL]
			, sEdit[ACL_SET_EDIT_LOCAL_PORT].Compare("0") == 0 ? GUI_ACL_EXPLAIN_PORT_ALL : sEdit[ACL_SET_EDIT_LOCAL_PORT] 
			, sCombo[ACL_SET_COMBO_NET]
			, sEdit[ACL_SET_EDIT_SERVICE_PORT].Compare("0") == 0 ? GUI_ACL_EXPLAIN_PORT_ALL : sEdit[ACL_SET_EDIT_SERVICE_PORT] 
			, sCombo[ACL_SET_COMBO_ACTION]
			, GUI_ACL_EXPLAIN_CONST
			);
		if(m_Combo[ACL_SET_COMBO_DIR].GetCurSel() == ACL_DIRECTION_IN_OUT)
			sExplain += '.';
	}

	char sSpace[201]; memset(sSpace, ' ', 200); sSpace[200] = 0;
	m_Label[ACL_SET_LABEL_EXPAIN].SetWindowText(sSpace);
	m_sLastExplain = sExplain;
	m_Label[ACL_SET_LABEL_EXPAIN].SetWindowText(sExplain);
	return sExplain;
}

void CAclSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclSet::OnOK() 
{
	CString tmpStrApp, tmpStrMemo, tmpStrPort, tmpStrLocalPort;

	m_Combo[ACL_SET_COMBO_APP].GetWindowText(tmpStrApp);
	m_Edit[ACL_SET_EDIT_MEMO].GetWindowText(tmpStrMemo);
	m_Edit[ACL_SET_EDIT_SERVICE_PORT].GetWindowText(tmpStrPort);
	m_Edit[ACL_SET_EDIT_LOCAL_PORT].GetWindowText(tmpStrLocalPort);

	UINT tmpPort = _ttoi(tmpStrPort);
	if(tmpPort > 65535 || tmpPort < 0)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_INAVALID_PORT);
		m_Edit[ACL_SET_EDIT_SERVICE_PORT].SetFocus();
		return;
	}

	UINT tmpLocalPort = _ttoi(tmpStrLocalPort);
	if(tmpLocalPort > 65535 || tmpLocalPort < 0)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_INAVALID_PORT);
		m_Edit[ACL_SET_EDIT_LOCAL_PORT].SetFocus();
		return;
	}

	if(tmpStrApp == "")
	{
		AfxMessageBox(GUI_ACL_MESSAGE_APP_PATH_ERROR);
		m_Combo[ACL_SET_COMBO_APP].SetFocus();
		return;
	}

	if((tmpStrApp.Right(1) == "\\") || (tmpStrApp.Right(1) == ":"))
	{
		AfxMessageBox(GUI_ACL_MESSAGE_ONLY_PATH);
		m_Combo[ACL_SET_COMBO_APP].SetFocus();
		return;
	}

	if(tmpStrApp.Compare("*") != 0 && _taccess(tmpStrApp,0) == -1)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_APP_NOT_EXSITS);
		m_Combo[ACL_SET_COMBO_APP].SetFocus();
		return;
	}

	if(tmpStrMemo.Compare(m_Acl.sMemo) != 0 || m_sExplain.Compare(m_sLastExplain) != 0)
	{
		m_Acl.bAccessTimeType	= m_Combo[ACL_SET_COMBO_TIME].GetCurSel();	
		m_Acl.bAction			= m_Combo[ACL_SET_COMBO_ACTION].GetCurSel();
		m_Acl.bDirection		= m_Combo[ACL_SET_COMBO_DIR].GetCurSel();
		m_Acl.bRemoteNetType	= m_Combo[ACL_SET_COMBO_NET].GetCurSel();
		m_Acl.bServiceType		= m_Combo[ACL_SET_COMBO_PROTOCOL].GetCurSel();
		m_Acl.uiServicePort		= tmpPort;	
		m_Acl.wLocalPort		= tmpLocalPort;		

		_tcscpy(m_Acl.sApplication, tmpStrApp);
		_tcscpy(m_Acl.sMemo, tmpStrMemo);

		m_bIsChange = TRUE;
	}

	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDOK);
}

void CAclSet::OnCancel() 
{
	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDCANCEL);
}

void CAclSet::OnButtonApplication() 
{
	static TCHAR BASED_CODE szFilter[] = _T("(*.exe)|*.exe||");

	CFileDialog *dlgFile;
	dlgFile =  new CFileDialog(TRUE,NULL,NULL,OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,szFilter);

	int iRet = dlgFile->DoModal();

	if(iRet == IDCANCEL)
	{
		delete dlgFile;
		dlgFile = NULL;
		return;
	}

	m_Combo[ACL_SET_COMBO_APP].SetWindowText(dlgFile->m_ofn.lpstrFile);
	MakeExplain();	

	delete  dlgFile;
	dlgFile = NULL;
}

void CAclSet::OnSelchangeComboServiceType() 
{
	int nIndex = m_Combo[ACL_SET_COMBO_PROTOCOL].GetCurSel();
	int nDir = m_Combo[ACL_SET_COMBO_DIR].GetCurSel();
	if(nDir == ACL_DIRECTION_IN)
	{
		SetPort(GUI_SERVICE_PORT_NUM[nIndex]
			, GUI_SERVICE_ENABLE[nIndex]
			, GUI_SERVICE_PORT_NUM[0]
			, TRUE
			);
	}
	else if(nDir == ACL_DIRECTION_OUT)
	{
		SetPort(GUI_SERVICE_PORT_NUM[0]
			, TRUE
			, GUI_SERVICE_PORT_NUM[nIndex]
			, GUI_SERVICE_ENABLE[nIndex]
			);
	}
	else
	{
		SetPort(GUI_SERVICE_PORT_NUM[0]
			, GUI_SERVICE_ENABLE[nIndex]
			, GUI_SERVICE_PORT_NUM[0]
			, GUI_SERVICE_ENABLE[nIndex]
			);
	}
}

void CAclSet::OnSelchangeComboAccessTime() 
{
	MakeExplain();	
}
void CAclSet::OnEditchangeComboApplication() 
{
	MakeExplain();	
}
void CAclSet::OnSelchangeComboApplication() 
{
	MakeExplain(TRUE);	
}
void CAclSet::OnSelchangeComboAction() 
{
	MakeExplain();	
}
void CAclSet::OnSelchangeComboDirection() 
{
	OnSelchangeComboServiceType();
	MakeExplain();	
}
void CAclSet::OnSelchangeComboRemoteNet() 
{
	MakeExplain();	
}
void CAclSet::OnChangeEditLocalPort() 
{
	MakeExplain();		
}
void CAclSet::OnChangeEditMemo() 
{
}
void CAclSet::OnChangeEditServicePort() 
{
	MakeExplain();		
}

CBrush m_DlgBrush(COLOR_TORJAN_BK);
HBRUSH CAclSet::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	int nID = pWnd->GetDlgCtrlID();
	if(nID == IDC_APP_ICON)
	{
		pDC->SetBkMode(TRANSPARENT);
		return m_DlgBrush;
	}

	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	return hbr;
}

#pragma comment( exestr, "B9D3B8FD2A63656E7567762B")
