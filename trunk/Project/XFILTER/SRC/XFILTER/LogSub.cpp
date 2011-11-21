// LogSub.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "LogSub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogSub dialog


CLogSub::CLogSub(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CLogSub::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLogSub)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bSelectedButton = 255;
}


void CLogSub::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogSub)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLogSub, CDialog)
	//{{AFX_MSG_MAP(CLogSub)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_LOG_APP, OnLogApp)
	ON_BN_CLICKED(IDC_LOG_ICMP, OnLogIcmp)
	ON_BN_CLICKED(IDC_LOG_NNB, OnLogNnb)
	ON_BN_CLICKED(IDC_BUTTON_QUERY, OnButtonQuery)
	ON_BN_CLICKED(IDC_BUTTON_CLS, OnButtonCls)
	ON_BN_CLICKED(IDC_BUTTON_NEXT, OnButtonNext)
	ON_BN_CLICKED(IDC_BUTTON_BACK, OnButtonBack)
	//}}AFX_MSG_MAP
	ON_MESSAGE(LOG_WM_ADD_LOG, OnAddLog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogSub message handlers

LRESULT CLogSub::OnAddLog(UINT wParam, LONG lParam)
{
	if(theApp.m_AclFile.GetHeader()->bWriteLog)
		m_LogFile[wParam].AddLog((PPACKET_LOG)lParam, theApp.m_AclFile.GetHeader()->uiLogSize);

	return 0;
}

void CLogSub::InitLogClass()
{
	GUI_UNIT GuiUnit;
	WORD wMaxSize = theApp.m_AclFile.GetHeader()->uiLogSize;
	
	GuiUnit.m_pButtonClear = &m_ButtonEx[LOG_BUTTON_CLS];
	GuiUnit.m_pButtonQuery = &m_ButtonEx[LOG_BUTTON_QUERY];
	GuiUnit.m_pButtonPrevPage = &m_ButtonEx[LOG_BUTTON_BACK];
	GuiUnit.m_pButtonNextPage = &m_ButtonEx[LOG_BUTTON_NEXT];
	GuiUnit.m_pEndDate = &m_Time[LOG_END_DATE];
	GuiUnit.m_pEndTime = &m_Time[LOG_END_TIME];
	GuiUnit.m_pLabelQuery = &m_Label[2];
	GuiUnit.m_pStartDate = &m_Time[LOG_START_DATE];
	GuiUnit.m_pStartTime = &m_Time[LOG_START_TIME];

	for(int i = 0; i < LOG_LIST_COUNT; i++)
	{
		GuiUnit.m_pListLog = &m_List[i];
		m_LogFile[i].InitDlgResource(&GuiUnit, i, LOG_FILE_NAME[i], wMaxSize);
	}
}

BOOL CLogSub::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CreateCaptionEx(LOG_CAPTION[LOG_BUTTON_APP]);
	m_LableTitle.SubclassDlgItem(IDC_LABLE_TITLE, this);
	m_LableTitle.SetBkColor(COLOR_LABLE_BK);

	int i = 0;
	for(i; i < LOG_LIST_COUNT; i++)
		m_List[i].SubclassDlgItem(LOG_LIST[i], this);
	for(i = 0; i < LOG_TIME_COUNT; i++)
		m_Time[i].SubclassDlgItem(LOG_TIME[i], this);

	m_Button[LOG_BUTTON_APP].SetButton(IDC_LOG_APP, this);
	m_Button[LOG_BUTTON_NNB].SetButton(IDC_LOG_NNB, this);
	m_Button[LOG_BUTTON_ICMP].SetButton(IDC_LOG_ICMP, this);

	m_ButtonEx[LOG_BUTTON_QUERY].SetButton(IDC_BUTTON_QUERY, this);
	m_ButtonEx[LOG_BUTTON_CLS].SetButton(IDC_BUTTON_CLS, this);
	m_ButtonEx[LOG_BUTTON_BACK].SetButton(IDC_BUTTON_BACK, this);
	m_ButtonEx[LOG_BUTTON_NEXT].SetButton(IDC_BUTTON_NEXT, this);

	m_Label[0].SubclassDlgItem(IDC_LABEL_FROM, this);
	m_Label[1].SubclassDlgItem(IDC_LABEL_TO, this);
	m_Label[2].SetLabelQuery(IDC_LABEL_RESULT_LIST, this);

	InitLogClass();

	OnLogApp();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogSub::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
	
	// Do not call CDialog::OnPaint() for painting messages
}

void CLogSub::SetSelectButton(BYTE bSelectButton)
{
	if(m_bSelectedButton == bSelectButton) return;
	if(m_bSelectedButton != 255)
	{
		m_Button[m_bSelectedButton].SetSelect(FALSE);
	}
	m_Button[bSelectButton].SetSelect(TRUE);
	SetWindowCaption(LOG_CAPTION[bSelectButton]);
	m_bSelectedButton = bSelectButton;

	m_List[LOG_TYPE_APP].ShowWindow(bSelectButton == LOG_TYPE_APP);
	m_List[LOG_TYPE_NNB].ShowWindow(bSelectButton == LOG_TYPE_NNB);
	m_List[LOG_TYPE_ICMP].ShowWindow(bSelectButton == LOG_TYPE_ICMP);

	m_LogFile[bSelectButton].RefreshGuiStatus();
}

void CLogSub::OnLogApp() 
{
	SetSelectButton(LOG_BUTTON_APP);
}

void CLogSub::OnLogIcmp() 
{
	SetSelectButton(LOG_BUTTON_ICMP);
}

void CLogSub::OnLogNnb() 
{
	SetSelectButton(LOG_BUTTON_NNB);
}

void CLogSub::OnButtonQuery() 
{
	m_LogFile[m_bSelectedButton].OnLogQueryButtonQuery();
}

void CLogSub::OnButtonCls() 
{
	m_LogFile[m_bSelectedButton].OnLogQueryButtonDelete();
}

void CLogSub::OnButtonNext() 
{
	m_LogFile[m_bSelectedButton].OnLogQueryButtonNext();
}

void CLogSub::OnButtonBack() 
{
	m_LogFile[m_bSelectedButton].OnLogQueryButtonBack();
}

#pragma comment( exestr, "B9D3B8FD2A6E71697577642B")
