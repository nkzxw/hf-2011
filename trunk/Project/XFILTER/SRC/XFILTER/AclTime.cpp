// AclTime.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclTime.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclTime dialog

DWORD  m_ConstTime = CTime(1990,1,1,0,0,0).GetTime();

CAclTime::CAclTime(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclTime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclTime)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
}


void CAclTime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclTime)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclTime, CDialog)
	//{{AFX_MSG_MAP(CAclTime)
	ON_WM_PAINT()
	ON_NOTIFY(TVN_SELCHANGED, IDC_TIME_TREE, OnSelchangedTimeTree)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_SUNDAY, OnSetTimeCheckSunday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_MONDAY, OnSetTimeCheckMonday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_FRIDAY, OnSetTimeCheckFriday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_SATURDAY, OnSetTimeCheckSaturday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_THURSDAY, OnSetTimeCheckThursday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_TUESDAY, OnSetTimeCheckTuesday)
	ON_BN_CLICKED(IDC_SET_TIME_CHECK_WEDNESDAY, OnSetTimeCheckWednesday)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_SET_TIME_TIME_END, OnDatetimechangeSetTimeTimeEnd)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_SET_TIME_TIME_START, OnDatetimechangeSetTimeTimeStart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclTime message handlers

BOOL CAclTime::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pParent = GetParent()->GetParent();
	
	int i = 0;
	for(i; i < LABEL_COUNT; i++)
	{
		m_Label[i].SubclassDlgItem(IDC_LABEL[i], this);
		m_Label[i].SetColor(COLOR_TREE_TEXT);
	}
	m_Label[1].SetColor(COLOR_GLOD_BLOD_Ex);
	m_Label[7].SetColor(COLOR_GLOD_BLOD_Ex);

	for(i = 0; i < CHECK_COUNT; i++)
	{
		m_Check[i].SubclassDlgItem(IDC_CHECK[i], this);
	}

	for(i = 0; i < TIME_COUNT; i++)
	{
		m_Time[i].SubclassDlgItem(IDC_TIME[i], this);
	}

	m_Tree.SubclassDlgItem(IDC_TIME_TREE, this);
	AddTreeList(&m_Tree, ACL_TIME_TYPE, sizeof(ACL_TIME_TYPE)/sizeof(TCHAR*));

	GetTime(theApp.m_AclFile.GetHeader()->pTime);
	m_History.InitHistory(ACL_TYPE_TIME, &theApp.m_AclFile);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclTime::GetTime(PXACL_TIME pTime)
{
	PXACL_TIME pCurrent = pTime;
	int nIndex = 0;
	while(pCurrent != NULL)
	{
		m_AclTime[nIndex] = *pCurrent;
		pCurrent = pCurrent->pNext;
		nIndex++;
	}
}

void CAclTime::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TREE_BK);
	dc.FillRect(&rect, &brush);
}

int CAclTime::SetValue(void* acltime)
{
	if(acltime == NULL) return 0;

	XACL_TIME* pAclTime = (XACL_TIME*)acltime;

	int i = 0;
	for(i; i < CHECK_COUNT; i++)
	{
		m_Check[i].SetCheck(GetBit(pAclTime->bWeekDay, i));
		m_Check[i].EnableWindow(!pAclTime->bNotAllowEdit);
	}

	m_Time[0].SetTime(&CTime(m_ConstTime + pAclTime->tStartTime.GetTime()));
	m_Time[1].SetTime(&CTime(m_ConstTime + pAclTime->tEndTime.GetTime()));

	for(i = 0; i < TIME_COUNT; i++)
	{
		m_Time[i].EnableWindow(!pAclTime->bNotAllowEdit);
	}

	return 0;
}

void CAclTime::OnSelchangedTimeTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	CString sText = m_Tree.GetItemText(m_Tree.GetSelectedItem());
	int nCount = sizeof(ACL_TIME_TYPE)/sizeof(TCHAR*);
	int iIndex = TextToIndex(sText , ACL_TIME_TYPE, nCount);

	m_iTreeIndex = iIndex;

	SetValue((void*)&m_AclTime[iIndex]);
	//SetValue(theApp.m_AclFile.FindAcl(iIndex + 1, ACL_TYPE_TIME));
	
	*pResult = 0;
}

void CAclTime::AddHistory()
{
	if(memcmp(m_AclTime
		, theApp.m_AclFile.GetHeader()->pTime
		, ACL_TIME_LENTH * ACL_TIME_TYPE_COUNT) == 0)
	{
		m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;
		m_History.Cancel();
	}
	else
	{
		m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
		m_History.AddHistory(OPT_TYPE_EDIT, m_bButtonFlags, (char*)&m_AclTime[m_iTreeIndex]);
	}
	SendMessageEx(m_bButtonFlags);
}

BOOL CAclTime::SendMessageEx(BYTE bFlags)
{
	m_bButtonFlags = bFlags;
	return GetParent()->GetParent()->SendMessage(ACL_WM_SUB_NOTIFY, ACL_BUTTON_TIME, bFlags);
}

void CAclTime::Apply()
{
	if((m_bButtonFlags & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK) return;
	m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;
	m_History.Apply();
	SendMessageEx(m_bButtonFlags);
}
void CAclTime::Cancel()
{
	if((m_bButtonFlags & ACL_BUTTON_CANCEL_MASK) != ACL_BUTTON_CANCEL_MASK) return;
	GetTime(theApp.m_AclFile.GetHeader()->pTime);
	SetValue((void*)&m_AclTime[m_iTreeIndex]);

	m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;
	m_History.Cancel();
	SendMessageEx(m_bButtonFlags);
}

void CAclTime::OnSetTimeCheckSunday() 
{
	SetAclValue(0, m_Check[0].GetCheck());
}

void CAclTime::OnSetTimeCheckMonday() 
{
	SetAclValue(1, m_Check[1].GetCheck());
}

void CAclTime::OnSetTimeCheckTuesday() 
{
	SetAclValue(2, m_Check[2].GetCheck());
}

void CAclTime::OnSetTimeCheckWednesday() 
{
	SetAclValue(3, m_Check[3].GetCheck());
}

void CAclTime::OnSetTimeCheckThursday() 
{
	SetAclValue(4, m_Check[4].GetCheck());
}

void CAclTime::OnSetTimeCheckFriday() 
{
	SetAclValue(5, m_Check[5].GetCheck());
}

void CAclTime::OnSetTimeCheckSaturday() 
{
	SetAclValue(6, m_Check[6].GetCheck());
}

void CAclTime::OnDatetimechangeSetTimeTimeStart(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetTimeValue(0);
	*pResult = 0;
}

void CAclTime::OnDatetimechangeSetTimeTimeEnd(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetTimeValue(1);
	*pResult = 0;
}

int CAclTime::SetAclValue(int iIndex,BOOL isTrue)
{
	SetBit(&m_AclTime[m_iTreeIndex].bWeekDay,iIndex,isTrue);
	AddHistory();
	return 0;
}

int CAclTime::SetTimeValue(int iType)
{
	CTime t;

	switch(iType)
	{
	case 0:			//start time
		m_Time[iType].GetTime(t);
		m_AclTime[m_iTreeIndex].tStartTime =  t.GetTime() - m_ConstTime;
		break;

	case 1:			//end time
		m_Time[iType].GetTime(t);
		m_AclTime[m_iTreeIndex].tEndTime =  t.GetTime() - m_ConstTime;
		break;

	default:
		return -1;
	}

	AddHistory();

	return 0;
}

#pragma comment( exestr, "B9D3B8FD2A63656E766B6F672B")
