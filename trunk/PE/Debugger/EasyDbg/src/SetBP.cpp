// SetBP.cpp : implementation file
//

#include "stdafx.h"
#include "EasyDbg.h"
#include "SetBP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSetBP dialog


CSetBP::CSetBP(CWnd* pParent /*=NULL*/)
	: CDialog(CSetBP::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetBP)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_dwLength=0;
    m_dwBpAddress=0;
    m_Select=0;
    m_dwAttribute=0;

}


void CSetBP::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetBP)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetBP, CDialog)
	//{{AFX_MSG_MAP(CSetBP)
	ON_BN_CLICKED(IDC_INT3, OnInt3)
	ON_BN_CLICKED(IDC_HARD, OnHard)
	ON_BN_CLICKED(IDC_MEMORY, OnMemory)
	ON_BN_CLICKED(BTN_OK, OnOk)
	ON_BN_CLICKED(BTN_CANCLE, OnCancle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetBP message handlers



//����INT3�ϵ�
void CSetBP::OnInt3() 
{
	// TODO: Add your control notification handler code here
    //ʹ���Ⱥ����ͱ༭�����Ч
    GetDlgItem(IDC_LENGTH)->EnableWindow(FALSE);
    GetDlgItem(IDC_ATTRIBUTE)->EnableWindow(FALSE);
    m_Select=1;
	
}

//����Ӳ���ϵ�
void CSetBP::OnHard() 
{
	// TODO: Add your control notification handler code here
    GetDlgItem(IDC_LENGTH)->EnableWindow(TRUE);
    GetDlgItem(IDC_ATTRIBUTE)->EnableWindow(TRUE);
    m_Select=2;
	
}
//�����ڴ�ϵ�
void CSetBP::OnMemory() 
{
	// TODO: Add your control notification handler code here
    GetDlgItem(IDC_LENGTH)->EnableWindow(TRUE);
    GetDlgItem(IDC_ATTRIBUTE)->EnableWindow(TRUE);
    m_Select=3;
	
}

void CSetBP::OnOk() 
{
	// TODO: Add your control notification handler code here

    //����������ݵ���16���ƴ���,Ҳ����ֻ����16�������� 
    char szBuffer[40];
    GetDlgItemText(IDC_ADDRESS,szBuffer,sizeof(szBuffer));
    
    //��ȡ16��������  ��öϵ��ַ
    sscanf(szBuffer, "%x", &m_dwBpAddress) ; //��Ϊ��%x���Ի�����������ת����16����
    
    GetDlgItemText(IDC_LENGTH,szBuffer,sizeof(szBuffer));
    
    //��ȡ16��������  ��öϵ㳤��
    sscanf(szBuffer, "%x", &m_dwLength) ; 
    //��öϵ�����
    GetDlgItemText(IDC_ATTRIBUTE,szBuffer,sizeof(szBuffer));
    sscanf(szBuffer,"%x",&m_dwAttribute);

    if (m_dwBpAddress==0)
    {
        AfxMessageBox("û������ϵ��ַ");
        return;
    }
    if (m_Select==3||m_Select==2)
    {
        if (m_dwLength==0)
        {
            AfxMessageBox("û�����볤��");
            return;
        }
    }
    
    CDialog::OnOK();
  
	
}

void CSetBP::OnCancle() 
{
	// TODO: Add your control notification handler code here
      CDialog::OnCancel();
	
}
