// ViewBP.cpp : implementation file
//

#include "stdafx.h"
#include "EasyDbg.h"
#include "ViewBP.h"
#include <afxtempl.h>
#include "DebugData.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DebugData.h"
/////////////////////////////////////////////////////////////////////////////
// CViewBP dialog
//ȫ��INT3����
extern CList<INT3_BP,INT3_BP&> g_Int3BpList;
//ȫ���ڴ�ϵ�����
extern CList<MEM_BP,MEM_BP&> g_MemBpList;

extern HANDLE g_hThread;

CViewBP::CViewBP(CWnd* pParent /*=NULL*/)
	: CDialog(CViewBP::IDD, pParent)
{
	//{{AFX_DATA_INIT(CViewBP)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CViewBP::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewBP)
	DDX_Control(pDX, IDC_BPLIST, m_bpList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewBP, CDialog)
	//{{AFX_MSG_MAP(CViewBP)
	ON_BN_CLICKED(BTN_INT3, OnInt3)
	ON_BN_CLICKED(BTN_HARD, OnHard)
	ON_BN_CLICKED(BTN_MEMORY, OnMemory)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewBP message handlers

void CViewBP::OnOK() 
{
	// TODO: Add extra validation here
	
	//CDialog::OnOK();
}

BOOL CViewBP::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
//     m_bpList.InsertColumn(0,TEXT("�ϵ��ַ"),LVCFMT_LEFT,100);
//     m_bpList.InsertColumn(1,TEXT("�ϵ�����"),LVCFMT_LEFT,100);
//     m_bpList.InsertColumn(2,TEXT("�ϵ�ԭ����"),LVCFMT_LEFT,100);
//     m_bpList.InsertColumn(3,TEXT("�Ƿ������öϵ�"),LVCFMT_LEFT,150);
//     
//     m_bpList.SetExtendedStyle(m_bpList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
 


	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//DEL void CViewBP::OnSize(UINT nType, int cx, int cy) 
//DEL {
//DEL 	CDialog::OnSize(nType, cx, cy);
//DEL 	
//DEL 	// TODO: Add your message handler code here
//DEL     if (!m_bpList.m_hWnd)
//DEL     {
//DEL         return;
//DEL     }
//DEL     
//DEL     CRect rc;
//DEL     GetClientRect(&rc);
//DEL     m_bpList.MoveWindow(&rc,TRUE);
//DEL 
//DEL 	
//DEL }

//ö�ٵ�ǰINT3�ϵ�
void CViewBP::ListBp()
{

    Int3UIinit();
    //ÿ�ζ�Ҫ���������
    m_bpList.DeleteAllItems();

    CString szText;
    POSITION pos=NULL;
    pos=g_Int3BpList.GetHeadPosition();
    int i=0;
    while (pos!=NULL)
    {
        
        INT3_BP bp=g_Int3BpList.GetNext(pos);
        szText.Format("%08X",bp.dwAddress);
        m_bpList.InsertItem(i,szText);
        szText.Format("INT3�ϵ�");
        m_bpList.SetItemText(i,1,szText);
        szText.Format("%02X",bp.bOriginalCode);
        m_bpList.SetItemText(i,2,szText);
        szText.Format("%d",bp.isForever);
        m_bpList.SetItemText(i,3,szText);
        i++;

    } 

}


//�鿴INT3�ϵ�
void CViewBP::OnInt3() 
{
	// TODO: Add your control notification handler code here
    ListBp();

	
}


//INT3�ϵ����ĳ�ʼ��
void CViewBP::Int3UIinit()
{
    //��ɾ�����е���
    for (int i=0;i<5;i++)
    {
        
        m_bpList.DeleteColumn(0);
    }
    m_bpList.InsertColumn(0,TEXT("�ϵ��ַ"),LVCFMT_LEFT,100);
    m_bpList.InsertColumn(1,TEXT("�ϵ�����"),LVCFMT_LEFT,100);
    m_bpList.InsertColumn(2,TEXT("�ϵ�ԭ����"),LVCFMT_LEFT,100);
    m_bpList.InsertColumn(3,TEXT("�Ƿ������öϵ�"),LVCFMT_LEFT,150);
    
    m_bpList.SetExtendedStyle(m_bpList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    //���ñ���
    SetWindowText(TEXT("��ǰINT3�ϵ�����"));
}

//Ӳ���ϵ����ĳ�ʼ��
void CViewBP::HardUIinit()
{
    //��ɾ�����е���
    for (int i=0;i<5;i++)
    {
        
        m_bpList.DeleteColumn(0);
    }
    m_bpList.InsertColumn(0,TEXT("�ϵ��ַ"),LVCFMT_LEFT,100);
    m_bpList.InsertColumn(1,TEXT("�ϵ�����"),LVCFMT_LEFT,70);
    m_bpList.InsertColumn(2,TEXT("�ϵ㳤��"),LVCFMT_LEFT,70);
    m_bpList.InsertColumn(3,TEXT("��ע"),LVCFMT_LEFT,300);
    
    m_bpList.SetExtendedStyle(m_bpList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    //���ñ���
    SetWindowText(TEXT("��ǰӲ���ϵ�����"));

}

void CViewBP::ListHardBp()
{
    HardUIinit();
    m_bpList.DeleteAllItems();
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(g_hThread,&ct))
    {
        OutputDebugString("ViewBP.cpp 183�г���");
        
        return;
    }
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;
    CString szText;
    int i=0;
    if(ct.Dr0)
    {
        //�ϵ��ַ
        szText.Format("%08X",ct.Dr0);
        m_bpList.InsertItem(i,szText);
        //�ϵ�����
        szText.Format("%d",tagDr7.DRFlag.rw0);
        m_bpList.SetItemText(i,1,szText);
        //�ϵ㳤��
        szText.Format("%d",tagDr7.DRFlag.len0+1);
        m_bpList.SetItemText(i,2,szText);
        m_bpList.SetItemText(i,3,TEXT("0:ִ�жϵ� 3:���ʶϵ� 1:д��ϵ�"));
        i++;
    }
    
    if(ct.Dr1)
    {
        
        szText.Format("%08X",ct.Dr1);
        m_bpList.InsertItem(i,szText);
        
        szText.Format("%d",tagDr7.DRFlag.rw1);
        m_bpList.SetItemText(i,1,szText);
        
        szText.Format("%d",tagDr7.DRFlag.len1+1);
        m_bpList.SetItemText(i,2,szText);
        m_bpList.SetItemText(i,3,TEXT("0:ִ�жϵ� 3:���ʶϵ� 1:д��ϵ�"));
        i++;
    }

    if(ct.Dr2)
    {
        
        szText.Format("%08X",ct.Dr2);
        m_bpList.InsertItem(i,szText);
        
        szText.Format("%d",tagDr7.DRFlag.rw2);
        m_bpList.SetItemText(i,1,szText);
        
        szText.Format("%d",tagDr7.DRFlag.len2+1);
        m_bpList.SetItemText(i,2,szText);
        m_bpList.SetItemText(i,3,TEXT("0:ִ�жϵ� 3:���ʶϵ� 1:д��ϵ�"));
        i++;
    }

    if(ct.Dr3)
    {
        
        szText.Format("%08X",ct.Dr3);
        m_bpList.InsertItem(i,szText);
        
        szText.Format("%d",tagDr7.DRFlag.rw3);
        m_bpList.SetItemText(i,1,szText);
        
        szText.Format("%d",tagDr7.DRFlag.len3+1);
        m_bpList.SetItemText(i,2,szText);
        m_bpList.SetItemText(i,3,TEXT("0:ִ�жϵ� 3:���ʶϵ� 1:д��ϵ�"));
        i++;
    }


}

//�鿴Ӳ���ϵ�
void CViewBP::OnHard() 
{
	// TODO: Add your control notification handler code here
    ListHardBp();

	
}

//�鿴�ڴ�ϵ�
void CViewBP::OnMemory() 
{
	// TODO: Add your control notification handler code here
   ListMemBp();
	
}

//�ڴ�ϵ�����ʼ��
void CViewBP::MemUIinit()
{
    //��ɾ�����е���
    for (int i=0;i<5;i++)
    {
        
        m_bpList.DeleteColumn(0);
    }
    m_bpList.InsertColumn(0,TEXT("�ϵ��ַ"),LVCFMT_LEFT,70);
    m_bpList.InsertColumn(1,TEXT("�ϵ�����"),LVCFMT_LEFT,70);
    m_bpList.InsertColumn(2,TEXT("�ϵ㳤��"),LVCFMT_LEFT,70);
    m_bpList.InsertColumn(3,TEXT("�����ڴ��ҳ"),LVCFMT_LEFT,100);
    m_bpList.InsertColumn(4,TEXT("��ע"),LVCFMT_LEFT,300);
    
    m_bpList.SetExtendedStyle(m_bpList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    //���ñ���
    SetWindowText(TEXT("��ǰ�ڴ�ϵ�����"));


}


//ö�ٵ�ǰ�ڴ�ϵ�
void CViewBP::ListMemBp()
{
    //��ʼ������
    MemUIinit();
    m_bpList.DeleteAllItems();

    CString szText;
    POSITION pos=NULL;
    pos=g_MemBpList.GetHeadPosition();
    MEM_BP mBP={0};
    int i=0;
    while (pos!=NULL)
    {
        mBP=g_MemBpList.GetNext(pos);
        szText.Format("%08X",mBP.dwBpAddress);
        m_bpList.InsertItem(i,szText);
        szText.Format("%d",mBP.dwAttribute);
        m_bpList.SetItemText(i,1,szText);
        szText.Format("%d",mBP.dwLength);
        m_bpList.SetItemText(i,2,szText);
        switch (mBP.dwNumPage)
        {
        case 1:
            szText.Format("%08X",mBP.dwMemPage[0]);
            
            break;
        case 2:
            szText.Format("%08X %08X",mBP.dwMemPage[0],mBP.dwMemPage[1]);
            break;
        case 3:
            szText.Format("%08X %08X %08X",mBP.dwMemPage[0],mBP.dwMemPage[1],mBP.dwMemPage[2]);
            break;
        case 4:
            szText.Format("%08X %08X %08X %08X",mBP.dwMemPage[0],mBP.dwMemPage[1],mBP.dwMemPage[2],mBP.dwMemPage[3]);
            break;
        case 5:
            szText.Format("%08X %08X %08X %08X %08X",mBP.dwMemPage[0],mBP.dwMemPage[1],
                mBP.dwMemPage[2],mBP.dwMemPage[3],mBP.dwMemPage[4]);
            break;
        }
        m_bpList.SetItemText(i,3,szText);
        szText.Format("1:д��ϵ� 3: ���ʶϵ�");
        m_bpList.SetItemText(i,4,szText);
        i++;

        
    }




}
