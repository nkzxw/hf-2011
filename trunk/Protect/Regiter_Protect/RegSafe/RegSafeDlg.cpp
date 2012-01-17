// RegSafeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RegSafe.h"
#include "RegSafeDlg.h"
#include "page1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegSafeDlg dialog
const COLORREF CLOUDBLUE = RGB(128, 184, 223);
const COLORREF WHITE = RGB(255, 255, 255);
const COLORREF BLACK = RGB(1, 1, 1);
const COLORREF DKGRAY = RGB(128, 128, 128);
const COLORREF ZDY = RGB(234, 168,156);

CRegSafeDlg::CRegSafeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRegSafeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRegSafeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_yIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	pPageLink=NULL;//��ʼ��ҳ����ָ��
	nPageCount=0;//��ʼ��ҳ������
	nCurrentPage=0;//��ʼ����ʾҳ����

}
CRegSafeDlg::~CRegSafeDlg()
{
struct PAGELINK* pTemp = pPageLink;
	while(pTemp)
	{
		struct PAGELINK* pNextTemp = pTemp->Next;
		pTemp->pDialog->DestroyWindow();
		delete pTemp->pDialog;
		delete pTemp;
		pTemp = pNextTemp;
	}
}

void CRegSafeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegSafeDlg)
	DDX_Control(pDX,IDC_TITLE,MyTitle);
	DDX_Control(pDX, IDC_MIN, m_Min);
	DDX_Control(pDX, IDC_CLOSE, m_Close);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRegSafeDlg, CDialog)
	//{{AFX_MSG_MAP(CRegSafeDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_SETIE, OnSetie)
	ON_BN_CLICKED(IDC_MIN, OnMin)
	ON_BN_CLICKED(IDC_SETIE2, OnSetie2)
	ON_BN_CLICKED(IDC_SETIE3, OnSetie3)
	ON_BN_CLICKED(IDC_SETIE4, OnSetie4)
	ON_BN_CLICKED(IDC_TITLE, OnTitle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegSafeDlg message handlers

BOOL CRegSafeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	
//ʹ���ڸ�����������֮��
const CWnd * pWndInsertAfter;
pWndInsertAfter = &wndTopMost;
SetWindowPos(pWndInsertAfter,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
//����SetWindowPosԭ��ΪBOOL SetWindowPos(const CWnd * pWndInsertAfter,int x,int y,int cx,int cy,UINT nFlags);
//pWndInsertAfterΪָ���ʶ�������͵�CWnd�����ָ�롣
//x��yΪ�������Ͻǵ����ꡣ
//cx��cyΪ���ڵĿ���ߡ�
//nFlagsȷ�����ڵĴ�С��λ�á���ΪSWP_NOSIZEʱ������cx��cy����ΪSWP_NOMOVEʱ������x��y��

	ModifyStyle( WS_CAPTION, WS_MINIMIZEBOX, SWP_DRAWFRAME );
	SetWindowText("ע���ά��");//������������ʾ�ı���
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	m_bmpBackground.LoadBitmap(IDB_BACKGROUND);//���ر���λͼ���ڴ�
	MyTitle.LoadBitmaps(IDB_MYTITLE,IDB_MYTITLEDOWN);
	m_Min.LoadBitmaps(IDB_MINNORMAL,IDB_MINDOWN);//  ��С��λͼ
    m_Close.LoadBitmaps(IDB_CLOSENORMAL,IDB_CLOSEDOWN);// �ر�λͼ
    
	
//��ȡҪ���صĶԻ���λ�ò���ʾ��һҳ
    CRect Rect1;
	GetWindowRect(&Rect1); //��������ڵ�λ��
	int nCaption = ::GetSystemMetrics(SM_CYCAPTION);
	int nXEdge = ::GetSystemMetrics(SM_CXEDGE);
	int nYEdge = ::GetSystemMetrics(SM_CYEDGE);
	CRect Rect2;
	GetDlgItem(IDC_POS)->GetWindowRect(&Rect2); //��ÿ�ܵ�λ��
	Rect1.top=Rect1.top+nCaption+nYEdge; //�������
	Rect1.left=Rect1.left+2*nXEdge;
	//�����λ��
	rectPage.top=Rect2.top-Rect1.top+25;
	rectPage.left=Rect2.left-Rect1.left;
	rectPage.bottom=Rect2.bottom-Rect1.top+30;
	rectPage.right=Rect2.right-Rect1.left;

	//���Ҫ��ʾ��ҳ
	page1* pStep1 = new page1;
	page2* pStep2 = new page2;
	page3* pStep3 = new page3;
	page4* pStep4 = new page4;

	AddPage(pStep1, IDD_PAGE1);
	AddPage(pStep2, IDD_PAGE2);
	AddPage(pStep3, IDD_PAGE3);
	AddPage(pStep4, IDD_PAGE4);

    ShowPage(1);//��ʾ��һҳ

//���ð�ť����ɫ
VERIFY(m_setie.Attach(IDC_SETIE, this, DKGRAY,BLACK,WHITE,ZDY));
VERIFY(m_setie2.Attach(IDC_SETIE2, this, DKGRAY,BLACK,WHITE,ZDY));
VERIFY(m_setie3.Attach(IDC_SETIE3, this, DKGRAY,BLACK,WHITE,ZDY));
VERIFY(m_setie4.Attach(IDC_SETIE4, this, DKGRAY,BLACK,WHITE,ZDY));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRegSafeDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);
		CRect rect;
		GetClientRect(&rect);
		CDC dcMem; 
		dcMem.CreateCompatibleDC(&dc); 
		BITMAP bitMap;
		m_bmpBackground.GetBitmap(&bitMap);
		CBitmap *pbmpOld=dcMem.SelectObject(&m_bmpBackground);
		dc.StretchBlt(0,0,rect.Width(),rect.Height(),&dcMem,0,0,bitMap.bmWidth,bitMap.bmHeight,SRCCOPY);
		//CDialog::OnPaint();
	}
}
void CRegSafeDlg::AddPage(CDialog* pDialog, UINT nID)
{
	struct PAGELINK* pTemp = pPageLink;
	//���������ɵĽ��
	struct PAGELINK* pNewPage = new PAGELINK;
	pNewPage->pDialog = pDialog;
	pNewPage->pDialog->Create(nID,this);
	
	// Is window created
	ASSERT(::IsWindow(pNewPage->pDialog->m_hWnd));
    // ���ÿҳ����ʽ
	DWORD dwStyle = pNewPage->pDialog->GetStyle();
	ASSERT((dwStyle & WS_CHILD) != 0); //�Ӵ���
	ASSERT((dwStyle & WS_BORDER) == 0); //�ޱ߽�
	//��ʾ
	pNewPage->pDialog->ShowWindow(SW_HIDE);
	pNewPage->pDialog->MoveWindow(rectPage);
	/*pNewPage->pDialog->SetWindowPos(&CWnd::wndTop,
		rectPage.left,rectPage.top,
		rectPage.Width(),rectPage.Height(),
		SWP_NOMOVE);*/
	pNewPage->Next=NULL;
	pNewPage->nNum=++nPageCount; //��������1
	if (pTemp)
	{
		while (pTemp->Next) pTemp=pTemp->Next; //�ƶ�����ĩβ
		pTemp->Next=pNewPage;

	}
	else
		pPageLink=pNewPage; //���ǵ�һ���ӵ�
}
void CRegSafeDlg::ShowPage(UINT nPos)
{
	struct PAGELINK* pTemp=pPageLink;
	while(pTemp)
	{
		if (pTemp->nNum==nPos)
		{
			pTemp->pDialog->ShowWindow(SW_SHOW);
		}
		else
		    //����ʾ
			pTemp->pDialog->ShowWindow(SW_HIDE);
		pTemp=pTemp->Next;
	}
	/*if (nPos>=nPageCount)  //���һҳ
	{
		nCurrentPage=nPageCount;
		SetWizButton(2);
		return;
	}
	if (nPos<=1) //��ҳ 
	{
		nCurrentPage=1;
		SetWizButton(0);
		return;
	}
	//�м䲽*/
	SetWizButton(nPos);
}
void CRegSafeDlg::SetWizButton(UINT uFlag)
{
	GetDlgItem(IDC_SETIE)->EnableWindow(TRUE);
	GetDlgItem(IDC_SETIE2)->EnableWindow(TRUE);
	GetDlgItem(IDC_SETIE3)->EnableWindow(TRUE);
	GetDlgItem(IDC_SETIE4)->EnableWindow(TRUE);
	
	switch(uFlag)
	{
	case 1: //��ʾ��һҳ��һ����ťʧЧ
		GetDlgItem(IDC_SETIE)->EnableWindow(FALSE);
		break;
	case 2: //
        GetDlgItem(IDC_SETIE2)->EnableWindow(FALSE);
		break;
	case 3://
		GetDlgItem(IDC_SETIE3)->EnableWindow(FALSE);
		break;
    case 4://
		GetDlgItem(IDC_SETIE4)->EnableWindow(FALSE);
		break;
	}
}
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRegSafeDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CRegSafeDlg::OnClose() 
{
OnOK();	
}

void CRegSafeDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	PostMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x, point.y));
	CDialog::OnLButtonDown(nFlags, point);
}

void CRegSafeDlg::OnSetie() 
{
ShowPage(1);
	
}

void CRegSafeDlg::OnMin() 
{
//ʹ������С��
WINDOWPLACEMENT lwndpl;
WINDOWPLACEMENT * lpwndpl;
lpwndpl=&lwndpl;
GetWindowPlacement(lpwndpl);
lpwndpl->showCmd=SW_SHOWMINIMIZED;
SetWindowPlacement(lpwndpl);
//SW_MINIMIZE��С��ָ���Ĵ��ڡ�SW_SHOWMINIMIZED;
//SW_RESTORE����󻯻���С���Ĵ��ڻָ�ԭ����С��
//SW_SHOW��ԭ���Ĵ�С��������ʾ���ڡ�
//SW_SHOWMAXIMIZED�����󻯴���
}

void CRegSafeDlg::OnSetie2() 
{
ShowPage(2);	
}

void CRegSafeDlg::OnOK() 
{

	CDialog::OnOK();
}

void CRegSafeDlg::OnSetie3() 
{
ShowPage(3);	
}

void CRegSafeDlg::OnSetie4() 
{
ShowPage(4);	
}

void CRegSafeDlg::OnTitle()
{
	
}
