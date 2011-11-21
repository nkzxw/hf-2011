// AclNnbSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclNnbSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclNnbSet dialog


CAclNnbSet::CAclNnbSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclNnbSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclNnbSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bIsChange = FALSE;
	memset(&m_Acl, 0, sizeof(XACL_NNB));
}


void CAclNnbSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclNnbSet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclNnbSet, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclNnbSet)
	ON_WM_PAINT()
	ON_LBN_SELCHANGE(IDC_LIST_NET, OnSelchangeListNet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclNnbSet message handlers

BOOL CAclNnbSet::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = 0;
	for(i; i < ACL_NNB_SET_LABEL_COUNT; i++)
		m_Label[i].SetLabelEx(ACL_NNB_SET_LABEL[i], this);
	for(i = 0; i < ACL_NNB_SET_EDIT_COUNT; i++)
		m_Edit[i].SubclassDlgItem(ACL_NNB_SET_EDIT[i], this);
	for(i = 0; i < ACL_NNB_SET_LIST_COUNT; i++)
		m_List[i].SubclassDlgItem(ACL_NNB_SET_LIST[i], this);
	for(i = 0; i < ACL_NNB_SET_BUTTON_COUNT; i++)
		m_Button[i].SetButton(ACL_NNB_SET_BUTTON[i], this);

	InitDialog();
	ShowAcl();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclNnbSet::InitDialog()
{
	EnumNnb();
	AddListStrings(&m_List[ACL_NNB_SET_LIST_TIME], ACL_TIME_TYPE, ACL_TIME_TYPE_COUNT);
	AddListStrings(&m_List[ACL_NNB_SET_LIST_DIR], GUI_DIRECTION, GUI_DIRECTION_COUNT);
	AddListStrings(&m_List[ACL_NNB_SET_LIST_ACTION], GUI_ACTION, GUI_ACTION_COUNT);

	m_Edit[ACL_NNB_SET_EDIT_COSTOM].SetLimitText(63);
	m_Edit[ACL_NNB_SET_EDIT_MEMO].SetLimitText(50);
}

void CAclNnbSet::ShowAcl()
{
	int nIndex = m_Acl.sNnb[0];
	if(nIndex != 0)
	{
		nIndex = m_List[ACL_NNB_SET_LIST_NNB].FindString(-1, m_Acl.sNnb);
		if(nIndex == LB_ERR)
		{
			nIndex = m_List[ACL_NNB_SET_LIST_NNB].GetCount() - 1;
		}
	}
	m_List[ACL_NNB_SET_LIST_NNB].SetCurSel(nIndex);
	OnSelchangeListNet();
	if(m_List[ACL_NNB_SET_LIST_NET].GetCurSel() == m_List[ACL_NNB_SET_LIST_NET].GetCount() - 1)	
	{
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].SetWindowText(m_Acl.sNnb);
	}

	m_List[ACL_NNB_SET_LIST_TIME].SetCurSel(m_Acl.bTimeType);
	m_List[ACL_NNB_SET_LIST_ACTION].SetCurSel(m_Acl.bAction);
	m_List[ACL_NNB_SET_LIST_DIR].SetCurSel(m_Acl.bDirection);

	m_Edit[ACL_NNB_SET_EDIT_MEMO].SetWindowText(m_Acl.sMemo);
}

void CAclNnbSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclNnbSet::OnOK() 
{
	CString tmpStrMemo, tmpStrNnb;
	
	DWORD dwTempIp = m_Acl.dwIp;
	if(m_Edit[ACL_NNB_SET_EDIT_COSTOM].IsWindowEnabled())
	{
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].GetWindowText(tmpStrNnb);
	}
	else
	{
		int iIndex = m_List[ACL_NNB_SET_LIST_NET].GetCurSel();
		m_List[ACL_NNB_SET_LIST_NET].GetText(iIndex, tmpStrNnb);
		if(tmpStrNnb.Compare("*") == 0)
			dwTempIp = 0xFFFFFFFF;	// 255.255.255.255
		else
			dwTempIp = GetIpFromName(tmpStrNnb);
	}
	if(tmpStrNnb.IsEmpty())
	{
		AfxMessageBox(GUI_ACL_MESSAGE_NNB_ERROR);
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].SetFocus();
		return;
	}

	m_Edit[ACL_NNB_SET_EDIT_MEMO].GetWindowText(tmpStrMemo);

	if(tmpStrMemo.Compare(m_Acl.sMemo) != 0 
		|| tmpStrNnb.Compare(m_Acl.sNnb) != 0
		|| m_Acl.bTimeType != m_List[ACL_NNB_SET_LIST_TIME].GetCurSel() 
		|| m_Acl.bAction != m_List[ACL_NNB_SET_LIST_ACTION].GetCurSel()
		|| m_Acl.bDirection != m_List[ACL_NNB_SET_LIST_DIR].GetCurSel()
		|| m_Acl.dwIp != dwTempIp
		)
	{
		m_Acl.bTimeType  = m_List[ACL_NNB_SET_LIST_TIME].GetCurSel();
		m_Acl.bAction	 = m_List[ACL_NNB_SET_LIST_ACTION].GetCurSel();
		m_Acl.bDirection = m_List[ACL_NNB_SET_LIST_DIR].GetCurSel();

		if(tmpStrNnb.GetLength() >= 64) tmpStrNnb.SetAt(63, 0);
		_tcscpy(m_Acl.sNnb, tmpStrNnb);
		_tcscpy(m_Acl.sMemo, tmpStrMemo);

		m_Acl.dwIp = dwTempIp;

		m_bIsChange = TRUE;
	}

	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDOK);
}

void CAclNnbSet::OnCancel() 
{
	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDCANCEL);
}

void CAclNnbSet::EnumNnb()
{
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_List[ACL_NNB_SET_LIST_NET].AddString(GUI_NET_IP_ARIA_ALL);
	if(IsWin9x())
	{
		PNAME_LIST pNameList = XF_GetNameList();
		while(pNameList != NULL)
		{
			m_List[ACL_NNB_SET_LIST_NET].AddString(pNameList->Name);
			pNameList = pNameList->pNext;
		}
	}
	else
	{
		//EnumerateFunc(NULL, &m_List[ACL_NNB_SET_LIST_NET], RESOURCEDISPLAYTYPE_SERVER);
		static char pBuffer[16384];
		DWORD nLength = XF_GetNameListEx(pBuffer, sizeof(pBuffer));
		if(nLength > sizeof(WORD))
		{
			DWORD nTemp = 0;
			WORD* pStrLen = NULL;
			while(nTemp <= (nLength - sizeof(WORD)))
			{
				pStrLen = (WORD*)(pBuffer + nTemp);
				nTemp += sizeof(WORD);
				if((nTemp + (*pStrLen)) > nLength)
					break;
				m_List[ACL_NNB_SET_LIST_NET].AddString(pBuffer + nTemp);
				nTemp += (*pStrLen);
			}
		}
	}
	m_List[ACL_NNB_SET_LIST_NET].AddString(ACL_NNB_COMSTOM);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}

DWORD CAclNnbSet::GetIpFromName(LPCTSTR sName)
{
	if(IsWin9x())
	{
		PNAME_LIST pNameList = XF_GetNameList();
		while(pNameList != NULL)
		{
			if(strcmp(pNameList->Name, sName) == 0)
				return pNameList->Address;
			pNameList = pNameList->pNext;
		}
	}
	else
	{
		return XF_GetIpFromName((char*)sName);
	}
	return 0;
}

void CAclNnbSet::OnSelchangeListNet() 
{
	if(m_List[ACL_NNB_SET_LIST_NET].GetCurSel() == m_List[ACL_NNB_SET_LIST_NET].GetCount() - 1)	
	{
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].EnableWindow(TRUE);
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].SetFocus();
	}
	else
	{
		m_Edit[ACL_NNB_SET_EDIT_COSTOM].EnableWindow(FALSE);
	}
}

#pragma comment( exestr, "B9D3B8FD2A63656E7070647567762B")
