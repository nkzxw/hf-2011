// ProcessModule.cpp : implementation file
//

#include "stdafx.h"
#include "EasyDbg.h"
#include "ProcessModule.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "Psapi.h"
#pragma comment(lib,"Psapi.Lib")

/////////////////////////////////////////////////////////////////////////////
// CProcessModule dialog


extern HANDLE g_hProcess;

CProcessModule::CProcessModule(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessModule::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProcessModule)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CProcessModule::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcessModule)
	DDX_Control(pDX, IDC_LIST1, m_ModuleList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProcessModule, CDialog)
	//{{AFX_MSG_MAP(CProcessModule)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcessModule message handlers

void CProcessModule::OnOK() 
{
	// TODO: Add extra validation here
	
	
}

BOOL CProcessModule::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   
    m_ModuleList.InsertColumn(0,TEXT("ģ����"));
    m_ModuleList.InsertColumn(1,TEXT("ģ���ַ"));
    m_ModuleList.InsertColumn(2,TEXT("ģ���С"));
    m_ModuleList.InsertColumn(3,TEXT("ģ����ڵ�"));
    m_ModuleList.InsertColumn(4,TEXT("ģ��·��"));
    
    
    m_ModuleList.SetExtendedStyle(m_ModuleList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    
    ListModules();

    m_ModuleList.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
    m_ModuleList.SetColumnWidth(1,LVSCW_AUTOSIZE_USEHEADER);
    m_ModuleList.SetColumnWidth(2,LVSCW_AUTOSIZE_USEHEADER);
    m_ModuleList.SetColumnWidth(3,LVSCW_AUTOSIZE_USEHEADER);
    m_ModuleList.SetColumnWidth(4,LVSCW_AUTOSIZE_USEHEADER);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//�оٱ����Խ��̵�ģ��
void  CProcessModule::ListModules()
{
    m_ModuleList.DeleteAllItems();
   
    HMODULE  hModule[500];
    //���շ��ص��ֽ���
    DWORD nRetrun=0;
    //ö��
    BOOL isSuccess=EnumProcessModules(g_hProcess,hModule,sizeof(hModule),&nRetrun);
    if (isSuccess==0)
    {
        OutputDebugString(TEXT("ö��ģ��ʧ��"));
        return;
        
    }
    
    TCHAR ModuleName[500];
    TCHAR ModuleFilePath[MAX_PATH];
    CString szText;
   
    MODULEINFO minfo;
    //��ʼ���
    for (DWORD i=0;i<(nRetrun/sizeof(HMODULE));i++)
    {
        //��ȡģ����
        DWORD nLength=GetModuleBaseName(g_hProcess,hModule[i],ModuleName,sizeof(ModuleName));
        if (nLength==0)
        {
            OutputDebugString(TEXT("��ȡģ����ʧ��"));
        }
        //��ӵ��б�
        m_ModuleList.InsertItem(i,ModuleName);
        //��ʽ��ģ���ַ
        szText.Format("0x%08X",hModule[i]);
        m_ModuleList.SetItemText(i,1,szText);
        //��ȡģ����Ϣ
        nLength=GetModuleInformation(g_hProcess,hModule[i],&minfo,sizeof(minfo));
        if (nLength==0)
        {
            OutputDebugString(TEXT("��ȡģ����Ϣʧ��"));
            return;
        }
        //��ʽ��ģ���С
        szText.Format("0x%08X",minfo.SizeOfImage);
        m_ModuleList.SetItemText(i,2,szText);
        //��ʽ��ģ����ڵ�
        szText.Format("0x%08X",minfo.EntryPoint);
        m_ModuleList.SetItemText(i,3,szText);
        //��ȡģ����·��
        nLength=GetModuleFileNameEx(g_hProcess,hModule[i],ModuleFilePath,MAX_PATH);
        if (nLength==0)
        {
            OutputDebugString(TEXT("��ȡģ��·��ʧ��"));
        }
        m_ModuleList.SetItemText(i,4,ModuleFilePath);
        
    }
    


}

void CProcessModule::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
   
    if (!m_ModuleList.m_hWnd)
    {
        return;
    }
    
    CRect rc;
    GetClientRect(&rc);
    m_ModuleList.MoveWindow(&rc,TRUE);
	
}
