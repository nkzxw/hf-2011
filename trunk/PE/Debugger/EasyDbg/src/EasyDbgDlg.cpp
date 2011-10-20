// EasyDbgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EasyDbg.h"
#include "EasyDbgDlg.h"
#include <stdio.h>

#include "Psapi.h"

#include<Tlhelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Decode2Asm.h"
#include "resource.h"

#define BUFSIZE 512
#pragma comment(lib,"Psapi.Lib")


extern CList<INT3_BP,INT3_BP&> g_Int3BpList(100);

extern CList<MEM_BP,MEM_BP&> g_MemBpList(100);
//����ӳ��Ļ�ַ
extern char* pFile=NULL;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


extern HANDLE g_hProcess=NULL;

extern HANDLE g_hThread=NULL;


HANDLE hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
//�̻߳ص���������
DWORD WINAPI DebugThreadProc(
                        LPVOID lpParameter   // thread data
                        );

//OpenThreadָ������
typedef HANDLE (__stdcall *LPFUN_OPENTHREAD)(
	   DWORD dwDesiredAccess,  // access right
	   BOOL bInheritHandle,    // handle inheritance option
	   DWORD dwThreadId        // thread identifier
	   );

//��ʼ��
LPFUN_OPENTHREAD pfnOpenThread = NULL;


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEasyDbgDlg dialog

CEasyDbgDlg::CEasyDbgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEasyDbgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEasyDbgDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
    
}

void CEasyDbgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEasyDbgDlg)
	DDX_Control(pDX, IDC_LIST5, m_Stack);
	DDX_Control(pDX, IDC_LIST3, m_AsmList);
	DDX_Control(pDX, IDC_LIST2, m_DataList);
	DDX_Control(pDX, IDC_LIST4, m_Result);
	DDX_Control(pDX, IDC_EDIT1, m_command);
	DDX_Control(pDX, IDC_LIST1, m_reglist);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEasyDbgDlg, CDialog)
	//{{AFX_MSG_MAP(CEasyDbgDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(IDM_OPEN, OnOpen)
    ON_WM_DROPFILES()
	ON_COMMAND(IDM_STEPINTO, OnStepinto)
	ON_COMMAND(IDM_STEPOVER, OnStepover)
	ON_BN_CLICKED(BTN_SHOWDATA, OnShowdata)
	ON_COMMAND(IDM_MEMORY, OnMemory)
	ON_COMMAND(IDM_MODULE, OnModule)
	ON_COMMAND(IDM_RUN, OnRun)
	ON_COMMAND(IDM_QUIT, OnQuit)
	ON_COMMAND(IDM_SETBP, OnSetbp)
	ON_COMMAND(IDM_BREAKPOINT, OnBreakpoint)
	ON_COMMAND(IDM_VIEWPE, OnViewpe)
	ON_COMMAND(IDM_AUTOSTEPOUT, OnAutostepout)
	ON_COMMAND(IDM_AUTOSTEPINTO, OnAutostepinto)
	ON_COMMAND(IDM_OUTFUN, OnOutfun)
	ON_COMMAND(IDM_DUMP, OnDump)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST3, OnDblclkList3)
	ON_NOTIFY(NM_RCLICK, IDC_LIST3, OnRclickList3)
	ON_COMMAND(IDM_INT3, OnInt3)
	ON_COMMAND(IDM_EXECUTE, OnExecute)
	ON_COMMAND(IDM_HARD, OnHard)
	ON_BN_CLICKED(BTN_API, OnApi)
	ON_COMMAND(IDM_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEasyDbgDlg message handlers

BOOL CEasyDbgDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
    //��ʼ������ֵ
    UIinit();
    //��ʼ���������
    memset(m_SzFilePath,0,sizeof(m_SzFilePath));
    memset(&m_tpInfo,0,sizeof(m_tpInfo));
    memset(&m_Recover_BP,0,sizeof(m_Recover_BP));
    memset(&m_Dr_Use,0,sizeof(m_Dr_Use));
    m_tpInfo.bCC=0xCC;
    m_isDebuging=FALSE;
    m_IsOepBP=TRUE;
    m_IsF8=FALSE;
    m_IsGo=FALSE;
    m_isDelete=FALSE;
    m_Uaddress=0;
    m_Recover_HBP.dwIndex=-1;

    m_Attribute[0]=0;//��ռλ�� ʵ�����õ��� 1 3
    m_Attribute[1]=PAGE_EXECUTE_READ;
    m_Attribute[2]=0;
    m_Attribute[3]=PAGE_NOACCESS;

    memset(&m_Recover_Mpage,0,sizeof(m_Recover_Mpage));
    m_isMoreMem=FALSE;

    m_IsAutoF8=FALSE;

    m_IsAutoF7=FALSE;
    m_hFile=INVALID_HANDLE_VALUE;
    memset(&m_AsmAddress,0,sizeof(m_AsmAddress));
    m_GetModule=FALSE;    
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEasyDbgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEasyDbgDlg::OnPaint() 
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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEasyDbgDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CEasyDbgDlg::OnOK()
{
    
}

//�õ���ִ���ļ���·��
void CEasyDbgDlg::GetExeFilePath(char* szFilePath)
{
    OPENFILENAME file = {0} ;
    file.lpstrFile = szFilePath ;
    file.lStructSize = sizeof(OPENFILENAME) ;
    file.nMaxFile = 256 ;
    file.lpstrFilter = "Executables\0*.exe\0All Files\0*.*\0\0" ;
    file.nFilterIndex = 1 ;
    
    if(!::GetOpenFileName(&file))
    {
        //����ȡ����ť���˳�����
        return;
    }
}




void CEasyDbgDlg::OnOpen() 
{
	// TODO: Add your command handler code here
   
    if (m_isDebuging==TRUE)
    {
        AfxMessageBox("���������ڵ�����!�����ڵ�����һ������");
        return;
    }
    GetExeFilePath(m_SzFilePath);
   //����û�����˹رհ�ť m_SzFilePathû��ֵ
    if (m_SzFilePath[0]==0x00)
    {
       
        return;

    }
     if (!MapPEFile())
     {
         return;
        

     }
     m_isDebuging=TRUE;

   
    CreateThread(NULL,0,DebugThreadProc,this,NULL,NULL);

}

//�����̺߳���
DWORD WINAPI DebugThreadProc(
                         LPVOID lpParameter   // thread data
                        )
{
    STARTUPINFO si={0};
    //Ҫ��ʼ���˳�Ա
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi={0};
    char szFilePath[256]={0};
    CEasyDbgDlg* pDebug=(CEasyDbgDlg*)lpParameter;
    //Ҫ�ù����߳� �������Խ���
    if (!CreateProcess(pDebug->m_SzFilePath,NULL,NULL,NULL,FALSE,DEBUG_ONLY_THIS_PROCESS,NULL,NULL,&si,&pi))
   {
        OutputDebugString("EasyDbgDlg.cpp�е�337�д������");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //��ó�����Ϣ�����
        pDebug->GetErrorMessage(dwErrorCode);
        return FALSE;
   }
    
    BOOL isExit=FALSE;//�����Խ����Ƿ��˳��ı�־
    //�����¼�
    DEBUG_EVENT de={0};
    //��Ϊϵͳ��һ�ζϵ�ı�־
    BOOL bFirstBp=FALSE;
    //��־ �������߳��������ķ�ʽ�ָ�
    DWORD  dwContinueStatus=DBG_CONTINUE;
    //����ѭ��
  while (!isExit&&WaitForDebugEvent(&de,INFINITE))//���������isExit�򱻵��Խ����˳�ʱ,����������һֱ�ȴ���
  {  
      switch (de.dwDebugEventCode)
      {
      case EXCEPTION_DEBUG_EVENT:
          switch (de.u.Exception.ExceptionRecord.ExceptionCode)
          {
          case EXCEPTION_ACCESS_VIOLATION:
              {
              DWORD dwAccessAddress=0;
              //�쳣���ʵĵ�ַ
              dwAccessAddress=de.u.Exception.ExceptionRecord.ExceptionInformation[1];
              dwContinueStatus=pDebug->ON_EXCEPTION_ACCESS_VIOLATION(
                  (DWORD)de.u.Exception.ExceptionRecord.ExceptionAddress,
                  dwAccessAddress
                                                             );
              break;
              }
          case EXCEPTION_BREAKPOINT:
              if (bFirstBp)
              {
                 dwContinueStatus = pDebug->ON_EXCEPTION_BREAKPOINT(
                     (DWORD)de.u.Exception.ExceptionRecord.ExceptionAddress
                                                                   );
                
              }
              else
              {
                  //����ϵͳ��һ�ζϵ�

                  bFirstBp=TRUE;
              }

              break;
          case EXCEPTION_SINGLE_STEP:

              dwContinueStatus = pDebug->ON_EXCEPTION_SINGLE_STEP(
                  (DWORD)de.u.Exception.ExceptionRecord.ExceptionAddress
                                                                 );
              
              break;


          }
          
          break;
      case CREATE_THREAD_DEBUG_EVENT:
        
          
          //���̴߳��������д��¼�
         // AfxMessageBox("�̴߳���");
          break;
      case CREATE_PROCESS_DEBUG_EVENT:

          //���̴߳���
          //AfxMessageBox("���̴���");
          dwContinueStatus=pDebug->ON_CREATE_PROCESS_DEBUG_EVENT(de.dwProcessId,
                                                                 de.dwThreadId,
                                                                 de.u.CreateProcessInfo.lpStartAddress);
          break;  
          
      case EXIT_THREAD_DEBUG_EVENT:
          //���߳��˳�����������¼�
          //AfxMessageBox("�߳��˳�");
          break;
      case EXIT_PROCESS_DEBUG_EVENT:
          //���߳��˳�
          //AfxMessageBox("�����˳�");

          isExit=TRUE;
        
          AfxMessageBox("�����Խ����˳�");
          
          break;
      case LOAD_DLL_DEBUG_EVENT:
          pDebug->ON_LOAD_DLL_DEBUG_EVENT(de.u.LoadDll.hFile,de.u.LoadDll.lpBaseOfDll);
          
          
          break;
      case UNLOAD_DLL_DEBUG_EVENT:
          
          break;
      case OUTPUT_DEBUG_STRING_EVENT:
          break;
      }

      //�ָ��������̵߳�����
      if (!ContinueDebugEvent(de.dwProcessId,de.dwThreadId,dwContinueStatus ))
      {
          OutputDebugString("EasyDbgDlg.cpp 442�г���");
          DWORD dwErrorCode=0;
          dwErrorCode=GetLastError();
          pDebug->GetErrorMessage(dwErrorCode);
          
          return DBG_EXCEPTION_NOT_HANDLED ;
          
          
      }
      //���ô˱�־
      dwContinueStatus=DBG_CONTINUE;
  
      
  }
    

    
    return 0;
}

//��ʾ�Ĵ���
void CEasyDbgDlg::ShowReg(HANDLE hThread)
{
    CONTEXT lpContext={0};
    lpContext.ContextFlags=CONTEXT_FULL;
    GetThreadContext(hThread,&lpContext);
    CString szText;
    szText.Format("%08X",lpContext.Eax);
    m_reglist.SetItemText(0,1,szText);
    szText.Format("%08X",lpContext.Ebx);
    m_reglist.SetItemText(1,1,szText);
    szText.Format("%08X",lpContext.Ecx);
    m_reglist.SetItemText(2,1,szText);
    szText.Format("%08X",lpContext.Edx);
    m_reglist.SetItemText(3,1,szText);
    szText.Format("%08X",lpContext.Ebp);
    m_reglist.SetItemText(4,1,szText);
    szText.Format("%08X",lpContext.Esp);
    m_reglist.SetItemText(5,1,szText);
    szText.Format("%08X",lpContext.Esi);
    m_reglist.SetItemText(6,1,szText);
    szText.Format("%08X",lpContext.Edi);
    m_reglist.SetItemText(7,1,szText);
    szText.Format("%08X",lpContext.Eip);
    
    m_reglist.SetItemText(8,1,szText);
    szText.Format("%04X",lpContext.SegCs);
    m_reglist.SetItemText(9,1,szText);
    szText.Format("%04X",lpContext.SegSs);
    m_reglist.SetItemText(10,1,szText);
    szText.Format("%04X",lpContext.SegDs);
    m_reglist.SetItemText(11,1,szText);
    szText.Format("%04X",lpContext.SegEs);
    m_reglist.SetItemText(12,1,szText);
    szText.Format("%04X",lpContext.SegGs);
    m_reglist.SetItemText(13,1,szText);
    szText.Format("%04X",lpContext.SegFs);
    m_reglist.SetItemText(14,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0001));
    m_reglist.SetItemText(15,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0004));
    m_reglist.SetItemText(16,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0010));
    m_reglist.SetItemText(17,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0040));
    m_reglist.SetItemText(18,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0080));
    m_reglist.SetItemText(19,1,szText);
    szText.Format("%d",(bool)(lpContext.EFlags & 0x0800));
    m_reglist.SetItemText(20,1,szText);

    

    

}

//�ػ���Ϣ
BOOL CEasyDbgDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
      
        //���� �ֹ������������Ϣ
        if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
        {
             
            if (m_command.GetFocus()->GetDlgCtrlID()==IDC_EDIT1)
            {
                
                char buffer[100]={0};
                m_command.GetWindowText(buffer,200);

                //��������
                Handle_User_Command(buffer);


            }
           
           
        }
         //����F7��ݼ�
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F7)
        {
            
             ON_VK_F7();
        }
        //����F8��ݼ�
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F8)
        {
            ON_VK_F8();
        }
        //����F9��ݼ�
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F9)
        {
            OnRun();
        }
        //����F6��ݼ�  �Զ�����
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F6)
        {
            OnAutostepout();
        }
        //����F5��ݼ� �Զ�����
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F5)
        {
            OnAutostepinto();
        }
    
        return CDialog::PreTranslateMessage(pMsg);



}

//��ʾ�������� �����ϵ��ڻ������ﻹԭ
void CEasyDbgDlg::ShowAsm(DWORD dwStartAddress)
{

    ShowAsmInWindow(dwStartAddress);

    //��ʾ��ջ Ĭ���Զ�����ģʽ�²���ʾ��ջ�������ٶ�
    if (!m_IsAutoF7 && !m_IsAutoF8)
    {
            ShowStack();
    }

   BYTE pCode[40]={0};

   DWORD dwOldProtect=0;
   DWORD dwRet=0;
   VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
   if (!ReadProcessMemory(m_tpInfo.hProcess,(LPCVOID)dwStartAddress,pCode,sizeof(pCode),NULL))
   {
       OutputDebugString("EasyDbgDlg.cpp 594�г���");
       DWORD dwErrorCode=0;
       dwErrorCode=GetLastError();
       //���û����������Ϣ
       GetErrorMessage(dwErrorCode);
       VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);
       return;
   }
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);
   //�ж��Ƿ��жϵ������ڶϵ������� ����������ڻ������лָ� 
   for (int i=0;i<16;i++)
   {

       POSITION pos=NULL;
       pos = m_Int3BpList.GetHeadPosition();
       while(pos!=NULL)
       {
           INT3_BP bp=m_Int3BpList.GetNext(pos);
           //�ж϶ϵ��ַ�Ƿ���������λ�������
           //��ԭ���öϵ�Ķ���
         
               if (bp.dwAddress==dwStartAddress+i)
               {
                   //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
                   pCode[i]=bp.bOriginalCode;
               }
           
          

        }


   }
   
   char szAsm[120]={0};
   char szOpCode[120]={0};
   UINT CodeSize=0;

    Decode2AsmOpcode(pCode,szAsm,szOpCode,&CodeSize,dwStartAddress);
    EXPORT_FUN_INFO expFun={0};
    //����ҵ��ı���ʾ��ʽ
    if (IsExportFun(szAsm,expFun))
    {
        //��ʾ���б��ؼ���
        char szResult[200]={0};
        sprintf(szResult,"%08X    %s        %s <%s.%s>",dwStartAddress,szOpCode,szAsm,expFun.szDLLName,expFun.szFunName);
        m_Result.AddString(szResult);
 
        m_Result.SetTopIndex(m_Result.GetCount()-1);

        
        //������Զ�F8ģʽ
        if (m_IsAutoF8)
        {
            OPCODE_RECORD op={0};
            //�����ָ����ӳ������Ѵ��� �Ͳ���д�ļ� (�жϵ�ַ)
            if (m_Opcode.Lookup(dwStartAddress,op))
            {
                return;
            }
            //���û�оͼ���ӳ���д�ļ�
            op.dwAddress=dwStartAddress;
            m_Opcode.SetAt(dwStartAddress,op);
            //��ʱҲҪ�ı���ʾ��ʽ
            char szNowShow[100]={0};
            sprintf(szNowShow,"%s <%s.%s>",szAsm,expFun.szDLLName,expFun.szFunName);
            WriteOpcodeToFile(dwStartAddress,szNowShow);
        }
            
        return;
    }
    //��ʾ���б��ؼ���
    char szResult[200]={0};
    sprintf(szResult,"%08X    %s        %s",dwStartAddress,szOpCode,szAsm);
    m_Result.AddString(szResult);

    m_Result.SetTopIndex(m_Result.GetCount()-1);
   
    //������Զ�F8ģʽ
    if (m_IsAutoF8)
    {
        OPCODE_RECORD op={0};
        //�����ָ����ӳ������Ѵ��� �Ͳ���д�ļ� (�жϵ�ַ)
        if (m_Opcode.Lookup(dwStartAddress,op))
        {
            return;
        }
        //���û�оͼ���ӳ���д�ļ�
        op.dwAddress=dwStartAddress;
        m_Opcode.SetAt(dwStartAddress,op);
        WriteOpcodeToFile(dwStartAddress,szAsm);

    }

    
    
  
}

void CEasyDbgDlg::UIinit()
{
    //������Ĵ��ڳ�ʼ��
    m_AsmList.InsertColumn(0,"��ַ",LVCFMT_LEFT,90);
    m_AsmList.InsertColumn(1,"HEX����",LVCFMT_LEFT,140);
    m_AsmList.InsertColumn(2,"�����",LVCFMT_LEFT,400);
    
    m_AsmList.SetExtendedStyle(m_AsmList.GetExtendedStyle()|LVS_EX_FULLROWSELECT);

    COLORREF crBkColor=::GetSysColor(COLOR_3DHILIGHT);
    m_AsmList.SetBkColor(crBkColor);

    //��ջ���ڳ�ʼ��
    m_Stack.InsertColumn(0,"��ַ",LVCFMT_LEFT,100);
    m_Stack.InsertColumn(1,"ֵ",LVCFMT_LEFT,100);
    m_Stack.SetExtendedStyle(m_Stack.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

    //�Ĵ����б�ؼ��ĳ�ʼ��
     
    m_reglist.InsertColumn(0,"�Ĵ���",LVCFMT_LEFT,50);
    m_reglist.InsertColumn(1,"�Ĵ���ֵ",LVCFMT_LEFT,200);
    m_reglist.InsertItem(0,"EAX");
    m_reglist.InsertItem(1,"EBX");
    m_reglist.InsertItem(2,"ECX");
    m_reglist.InsertItem(3,"EDX");
    m_reglist.InsertItem(4,"EBP");
    m_reglist.InsertItem(5,"ESP");
    m_reglist.InsertItem(6,"ESI");
    m_reglist.InsertItem(7,"EDI");
    m_reglist.InsertItem(8,"EIP");
    m_reglist.InsertItem(9,"CS");
    m_reglist.InsertItem(10,"SS");
    m_reglist.InsertItem(11,"DS");
    m_reglist.InsertItem(12,"ES");
    m_reglist.InsertItem(13,"GS");
    m_reglist.InsertItem(14,"FS");
    m_reglist.InsertItem(15,"CF");
    m_reglist.InsertItem(16,"PF");
    m_reglist.InsertItem(17,"AF");
    m_reglist.InsertItem(18,"ZF");
    m_reglist.InsertItem(19,"SF");
    m_reglist.InsertItem(20,"OF");
    m_reglist.SetExtendedStyle(m_reglist.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    //���ݴ����б�ĳ�ʼ��
    m_DataList.InsertColumn(0,"��ַ",LVCFMT_LEFT,70);
    m_DataList.InsertColumn(1,"HEX����",LVCFMT_LEFT,170);
    m_DataList.InsertColumn(2,"ASCII",LVCFMT_LEFT,100);
    m_DataList.SetExtendedStyle(m_DataList.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);


}

//���������Ϣ 
void CEasyDbgDlg::GetErrorMessage(DWORD dwErrorCode)
{
    LPVOID lpMsgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
        );
    
    ::MessageBox( NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK | MB_ICONINFORMATION );
    //�ͷŶѿռ�
    LocalFree( lpMsgBuf );
}

//�õ�����DLLʱ��·��
void CEasyDbgDlg::GetFileNameFromHandle(HANDLE hFile,LPVOID pBase)
{
    //��DLL���� ��ʾģ����Ϣ��Ҫ����
    m_GetModule=TRUE;

    //�����������Ч���ж�
    if (hFile==NULL)
    {
        AfxMessageBox("�����Ч");
        return;
    }
    
    TCHAR pszFilename[MAX_PATH+1];
    HANDLE hFileMap;
    
    // Get the file size.
    DWORD dwFileSizeHi = 0;
    DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 
    
    if( dwFileSizeLo == 0 && dwFileSizeHi == 0 )
    {
        _tprintf(TEXT("Cannot map a file with a length of zero.\n"));
        return ;
    }
    
    // Create a file mapping object.

    hFileMap = CreateFileMapping(hFile, 
        NULL, 
        PAGE_READONLY,
        0, 
        0,
        NULL);
    
    if (hFileMap) 
    {
        // Create a file mapping to get the file name.
        void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
        
        if (pMem) 
        {
            //��õ���������Ϣ
            GetExportFunAddress(hFile,(char*)pMem,pBase);
            if (GetMappedFileName (GetCurrentProcess(), 
                pMem, 
                pszFilename,
                MAX_PATH)) 
            {
                
                // Translate path with device name to drive letters.
                TCHAR szTemp[BUFSIZE];
                szTemp[0] = '\0';
                
                if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
                {
                    TCHAR szName[MAX_PATH];
                    TCHAR szDrive[3] = TEXT(" :");
                    BOOL bFound = FALSE;
                    TCHAR* p = szTemp;
                    
                    do 
                    {
                        // Copy the drive letter to the template string
                        *szDrive = *p;
                        
                        // Look up each device name
                        if (QueryDosDevice(szDrive, szName, MAX_PATH))
                        {
                            size_t uNameLen = _tcslen(szName);
                            
                            if (uNameLen < MAX_PATH) 
                            {
                                bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0;
                                
                                if (bFound && *(pszFilename + uNameLen) == _T('\\')) 
                                {
                                    // Reconstruct pszFilename using szTempFile
                                    // Replace device path with DOS path
                                     TCHAR szTempFile[MAX_PATH];
                                     _stprintf(szTempFile,
                                        
                                         TEXT("%s%s"),
                                         szDrive,
                                         pszFilename+uNameLen);
                                     _tcsncpy(pszFilename, szTempFile, MAX_PATH);
                                }
                            }
                        }
                        
                        // Go to the next NULL character.
                        while (*p++);
                    } while (!bFound && *p); // end of string
                }
            }
            
            UnmapViewOfFile(pMem);
            pMem=NULL;
        } 
        
        CloseHandle(hFileMap);
        hFileMap=NULL;
    }
    

    m_Result.AddString(pszFilename);

    m_Result.SetTopIndex(m_Result.GetCount()-1); 



}

//���� CREATE_PROCESS_DEBUG_EVENT �¼��ĺ��� 
DWORD CEasyDbgDlg::ON_CREATE_PROCESS_DEBUG_EVENT(DWORD dwProcessId,DWORD dwThreadId,LPTHREAD_START_ROUTINE lpOepAddress)
{
   //�����¼���Ϣ�еľ������Ϊ��,�����������ֱ��OpenProcess��OpenThread����ñ����Խ��̺����̵߳ľ��

    //��ʼ��  m_tpInfo�ṹ��
    HMODULE hDll=GetModuleHandle("Kernel32.dll");
    if (hDll==NULL)
    {
        
        hDll=LoadLibrary("Kernel32.dll");
        if (hDll==NULL)
        {
            OutputDebugString("EasyDbgDlg.cpp 897�г���");
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���û����������Ϣ
            GetErrorMessage(dwErrorCode);
            return DBG_EXCEPTION_NOT_HANDLED ;
        }
    }
    pfnOpenThread = (LPFUN_OPENTHREAD)(GetProcAddress(hDll,"OpenThread"));
    if (pfnOpenThread==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 908�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return DBG_EXCEPTION_NOT_HANDLED ;
    }
    
    m_tpInfo.hThread=pfnOpenThread(THREAD_ALL_ACCESS,FALSE,dwThreadId);
    if (m_tpInfo.hThread==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 918�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return DBG_EXCEPTION_NOT_HANDLED ;

    }

    m_tpInfo.hProcess=OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwProcessId);
    if (m_tpInfo.hProcess==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 929�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return DBG_EXCEPTION_NOT_HANDLED ;
    }
    //ȫ�־����ֵ
    g_hProcess=m_tpInfo.hProcess;
    g_hThread=m_tpInfo.hThread;

    m_tpInfo.dwProcessId=dwProcessId;
    m_tpInfo.dwThreadId=dwThreadId;
    m_tpInfo.OepAddress=lpOepAddress;
    //��OEP���¶ϵ�
    if (!ReadProcessMemory(m_tpInfo.hProcess,m_tpInfo.OepAddress,&m_tpInfo.OriginalCode,1,NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 946�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return DBG_EXCEPTION_NOT_HANDLED ;
    }
    if (!WriteProcessMemory(m_tpInfo.hProcess,m_tpInfo.OepAddress,&m_tpInfo.bCC,1,NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 954�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return DBG_EXCEPTION_NOT_HANDLED ;

    }
    return DBG_CONTINUE;

    
}

//�ļ���ק
void CEasyDbgDlg::OnDropFiles( HDROP hDropInfo )
{
    
    
    if (m_isDebuging==TRUE)
    {
        AfxMessageBox("���������ڵ�����!�����ڵ�����һ������");
        return;
    }

    ::DragQueryFile 
        (hDropInfo,0,m_SzFilePath,sizeof(m_SzFilePath)); //0��ʾȡ��һ������ק���ļ���  
    ::DragFinish (hDropInfo); //�ͷ��ڴ�   
    

    if (!MapPEFile())
    {
        return;
     }
    m_isDebuging=TRUE;
    CreateThread(NULL,0,DebugThreadProc,this,NULL,NULL);

    
    
    
    
    
}

//��ʾ�����Խ����ڴ�����,Ĭ�ϴ�OEP��ʼ,��ʾ800�ֽ�,ÿ��8�ֽ�
void CEasyDbgDlg::ShowProcessMemory(DWORD dwStartAddress)
{

   


    //����������
    if (dwStartAddress==NULL)
    {
        AfxMessageBox("��Ч��ַ");
        return;
    }
    MEMORY_BASIC_INFORMATION mbi={0};
    //�ж��ǲ�����Ч��ҳ�ĵ�ַ
    if (!IsAddressValid(dwStartAddress,mbi))
    {
        AfxMessageBox("��ַ��Ч");
        return;
    }
    MEMORY_BASIC_INFORMATION mbiend={0};
    //�ж���ֹ��ַ�Ƿ�����Ч��ַ
    if (!IsAddressValid(dwStartAddress+800,mbiend))
    {
        //������Ǿ���ʾ��mbi���ڵ���Ч��ҳ�ĵ�ַ������

        //ÿ����ʾ������б��е�����
        m_DataList.DeleteAllItems();
        
        BYTE bBuffer[800]={0};
        DWORD dwOldProtect=0;
        DWORD dwRet=0;
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
        if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwStartAddress,bBuffer,((DWORD)mbi.BaseAddress+(DWORD)mbi.RegionSize-dwStartAddress),NULL))
        {
            OutputDebugString("EasyDbgDlg.cpp 1031�г���");
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���û����������Ϣ
            GetErrorMessage(dwErrorCode);
            return;
        }
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);
        
        //Ҫ�ж��Ƕϵ����� ����Ƕϵ�������ʾ��CC��Ҫ�ڻ������л�ԭ��
        for (DWORD j=0;j<((DWORD)mbi.BaseAddress+(DWORD)mbi.RegionSize-dwStartAddress);j++)
        {
            POSITION pos=NULL;
            pos = m_Int3BpList.GetHeadPosition();
            while(pos!=NULL)
            {
                INT3_BP bp=m_Int3BpList.GetNext(pos);
                //�ж϶ϵ��ַ�Ƿ���������λ�������
                if (bp.dwAddress==dwStartAddress+j)
                {
                    //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
                    bBuffer[j]=bp.bOriginalCode;
                }
            }
            
        }
        
        CString szText;
        
        for (DWORD i=0;i<(((DWORD)mbi.BaseAddress+(DWORD)mbi.RegionSize-dwStartAddress)/8);i++)
        {
            szText.Format("%08X",dwStartAddress+i*8);
            m_DataList.InsertItem(i,szText);
            szText.Format("%02X %02X %02X %02X %02X %02X %02X %02X" ,bBuffer[0+i*8],bBuffer[1+i*8],bBuffer[2+i*8],
                bBuffer[3+i*8],bBuffer[4+i*8],bBuffer[5+i*8],bBuffer[6+i*8],bBuffer[7+i*8]);
            
            m_DataList.SetItemText(i,1,szText);
            char szBffer[9]={0};
            for (int j=0;j<8;j++)
            {
                szBffer[j]=bBuffer[j+i*8];
                //���������ַ� ��س� TAB��
                if (szBffer[j]==NULL||szBffer[j]==0xFF||szBffer[j]==0x0A ||szBffer[j]==0x0D||szBffer[j]==0x09||szBffer[j]==0x07)
                    
                {
                    //�����ַ���ʾ��
                    szBffer[j]=0x2e;
                }
            }
            m_DataList.SetItemText(i,2,szBffer);
        }



        return;
    }
    //ÿ����ʾ������б��е�����
    m_DataList.DeleteAllItems();
 
    BYTE bBuffer[800]={0};
    DWORD dwOldProtect=0;
    DWORD dwRet=0;
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
    if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwStartAddress,bBuffer,sizeof(bBuffer),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1098�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);

    //Ҫ�ж��Ƕϵ����� ����Ƕϵ�������ʾ��CC��Ҫ�ڻ������л�ԭ��
    for (int j=0;j<800;j++)
    {
        POSITION pos=NULL;
        pos = m_Int3BpList.GetHeadPosition();
        while(pos!=NULL)
        {
            INT3_BP bp=m_Int3BpList.GetNext(pos);
            //�ж϶ϵ��ַ�Ƿ���������λ�������
            if (bp.dwAddress==dwStartAddress+j)
            {
                //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
                bBuffer[j]=bp.bOriginalCode;
            }
        }
        
   }

    CString szText;

    
    for (int i=0;i<100;i++)
    {
        szText.Format("%08X",dwStartAddress+i*8);
        m_DataList.InsertItem(i,szText);
        szText.Format("%02X %02X %02X %02X %02X %02X %02X %02X" ,bBuffer[0+i*8],bBuffer[1+i*8],bBuffer[2+i*8],
            bBuffer[3+i*8],bBuffer[4+i*8],bBuffer[5+i*8],bBuffer[6+i*8],bBuffer[7+i*8]);

        m_DataList.SetItemText(i,1,szText);
        char szBffer[9]={0};
        for (int j=0;j<8;j++)
        {
            szBffer[j]=bBuffer[j+i*8];
            //���������ַ� ��س� TAB��
            if (szBffer[j]==NULL||szBffer[j]==0xFF||szBffer[j]==0x0A ||szBffer[j]==0x0D||szBffer[j]==0x09||szBffer[j]==0x07)
                
            {
                //�����ַ���ʾ��
                szBffer[j]=0x2e;
            }
        }
        m_DataList.SetItemText(i,2,szBffer);
    }

}


//F7���Ĵ����� ��������
void CEasyDbgDlg::ON_VK_F7()
{
    //�õ���
    SetDlgItemText(IDC_STATE,"");
    CONTEXT ct;
    ct.ContextFlags=CONTEXT_FULL;
    GetThreadContext(m_tpInfo.hThread,&ct);
    ct.EFlags|=0x100;
    SetThreadContext(m_tpInfo.hThread,&ct);

    SetEvent(hEvent);
}

//F8���Ĵ����� ��������
void CEasyDbgDlg::ON_VK_F8()
{
    
    SetDlgItemText(IDC_STATE,"");
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 1178�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        return;
    }
    BYTE szCodeBuffer[40]={0};

    DWORD dwOldProtect=0;
    DWORD dwRet=0;

    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)ct.Eip,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
    //��õ�ǰEIP����ָ��
    if (!ReadProcessMemory(m_tpInfo.hProcess,(LPCVOID)ct.Eip,szCodeBuffer,sizeof(szCodeBuffer),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1193�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)ct.Eip,4,dwOldProtect,&dwRet);
        return;
    }
     VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)ct.Eip,4,dwOldProtect,&dwRet);

   char szAsm[120]={0};
   char szOpCode[120]={0};
   UINT CodeSize=0;
   //����ಢ�жϵ�ǰָ���ǲ���callָ��
   Decode2AsmOpcode(szCodeBuffer,szAsm,szOpCode,&CodeSize,ct.Eip);

   if (szAsm[0]=='c' && szAsm[1]=='a' && szAsm[2]=='l' && szAsm[3]=='l')
   {
        //�����ǰָ����callָ��,��ô������һ��ָ��������ʱ�ϵ�

       //�ж������һ��ָ���Ѿ��жϵ���,����Ҫ����
       POSITION pos=NULL;
       pos=m_Int3BpList.GetHeadPosition();
       INT3_BP bp={0};
       
       while(pos!=NULL)
       {
           bp=m_Int3BpList.GetNext(pos);
           //����ҵ��ϵ��� ����Ҫ���¶ϵ�
           if (bp.dwAddress==ct.Eip+CodeSize)
           {
               //���ñ�־λ
               m_IsF8=TRUE;

               SetEvent(hEvent);
               return;

           }

       }
       //�����öϵ�
       bp.dwAddress=ct.Eip+CodeSize;
       bp.isForever=FALSE;

       VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)bp.dwAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
       if (!ReadProcessMemory(m_tpInfo.hProcess,(LPCVOID)bp.dwAddress,&bp.bOriginalCode,sizeof(BYTE),NULL))
       {
           OutputDebugString("EasyDbgDlg.cpp 1239�г���");
           DWORD dwErrorCode=0;
           dwErrorCode=GetLastError();
           GetErrorMessage(dwErrorCode);
           VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)bp.dwAddress,4,dwOldProtect,&dwRet);
           return;
       }
       if (!WriteProcessMemory(m_tpInfo.hProcess,(LPVOID)bp.dwAddress,&m_tpInfo.bCC,sizeof(BYTE),NULL))
       {
           OutputDebugString("EasyDbgDlg.cpp 1248�г���");
           DWORD dwErrorCode=0;
           dwErrorCode=GetLastError();
           GetErrorMessage(dwErrorCode);
           VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)bp.dwAddress,4,dwOldProtect,&dwRet);
           return;
       }
       VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)bp.dwAddress,4,dwOldProtect,&dwRet);
       FlushInstructionCache(m_tpInfo.hProcess,(LPCVOID)bp.dwAddress,sizeof(BYTE));
       //�Ѷϵ��������
       m_Int3BpList.AddTail(bp);
       //���ñ�־λ
       m_IsF8=TRUE;
       

       SetEvent(hEvent);
       
   }
   else
   {
       //�����ǰָ���CALLָ��,��ô���õ���
       ON_VK_F7();
   }
    
    
}


//��Ӧ�˵���Ϣ
void CEasyDbgDlg::OnStepinto() 
{
	// TODO: Add your command handler code here
    ON_VK_F7();
    
	
}
//��Ӧ�˵���Ϣ
void CEasyDbgDlg::OnStepover() 
{
	// TODO: Add your command handler code here
    ON_VK_F8();
    
    	
}

//ȥ���������ߺ��ұߵĿո��ַ�
BOOL CEasyDbgDlg::Kill_SPACE(char* szCommand)
{
    //�����ַ��� ������������ֹ��
    int nSize=strlen(szCommand);
    //û��������Ͱ��س���
    if (*szCommand==0)
    {
        AfxMessageBox("û����������");
        return FALSE;
    }
    //ȥ��ǰ��Ŀո�
	int i = 0;
    for (i;i<nSize;i++)
    {
        if (szCommand[i]!=0x20)
        {
            //ȥ��ǰ��Ŀո�֮����ַ�����С
            int  nNowSize=nSize-i;
            for (int j=0;j<nNowSize;j++)
            {
                //��ǰ�ƶ�
                szCommand[j]=szCommand[i];
                i++;
            }
            szCommand[nNowSize]=0;
            
        }
    }
    //֮����ȥ������Ŀո�
    for (i=strlen(szCommand)-1;i>0;i--)
    {
        //�Ӻ���ǰ����,������һ�����ǿո���ַ�����
        if (szCommand[i]!=0x20)
        {
            //������Ϊ��ֹ��
            szCommand[i+1]=0;
            break;
        }
   }

    return TRUE;

}

//�û�����Ĵ�����
void CEasyDbgDlg::Handle_User_Command(char* szCommand)
{
    //ȥ��ǰ��Ŀո�
    if (!Kill_SPACE(szCommand))
    {
        AfxMessageBox("�����������");
        return;
    }
    //���������
    switch (szCommand[0])
    {
    case 't':
    case 'T':
        ON_VK_F7();
        break;
    case 'p':
    case 'P':
        ON_VK_F8();
        break;
    case 'u':
    case 'U':
        {
            //uf  ���� �Ժ��������
            if (szCommand[1]=='F'||szCommand[1]=='f')
            {
                char szName[100]={0};
                sscanf(szCommand, "%s%s", stderr, &szName);
                DisassemblerExcFun(szName);
                
                
            }
            else
            {
                unsigned int dwAddress = 0 ;
                //��ȡU����ĵ�ַ
                sscanf(szCommand, "%s%x", stderr, &dwAddress);
                ON_U_COMMAND(dwAddress);
            }
        break;
        }
    case 'b':
    case 'B':
        Handle_B_Command(szCommand);
        
        break;
    case 'g':
    case 'G':
        {
        unsigned int dwAddress = 0 ;
            //��ȡU����ĵ�ַ
        sscanf(szCommand, "%s%x", stderr, &dwAddress);
        ON_G_COMMAND(dwAddress);
        break;

        }
    case 's':
    case 'S':
        
            //�Զ�����
            OnAutostepout();
            break;
    case 'o':
    case 'O':
        //��������
        StepOutFromFun();
        break;
    default:
        AfxMessageBox(TEXT("�������"));
    }
    

    
}

//��ʾ�ڴ�����
void CEasyDbgDlg::OnShowdata() 
{
	// TODO: Add your control notification handler code here

    //����������ݵ���16���ƴ���,Ҳ����ֻ����16�������� 
    char szBuffer[40];
    GetDlgItemText(EDIT_DATA,szBuffer,sizeof(szBuffer));
    DWORD dwAddress = 0 ;
    //��ȡ16��������
    sscanf(szBuffer, "%x", &dwAddress) ; 
    ShowProcessMemory(dwAddress);
    
	
}


//�鿴�����Խ��̵�ַ�ռ�
void CEasyDbgDlg::OnMemory() 
{
	// TODO: Add your command handler code here
    CVirtualMemory dlg;
    dlg.DoModal();


	
}
//�鿴�����Խ��̵�ģ��
void CEasyDbgDlg::OnModule() 
{
	// TODO: Add your command handler code here
    CProcessModule dlg;
    dlg.DoModal();
	
}

//�ϵ��쳣������

//����dwExpAddress:�쳣��ַ

DWORD CEasyDbgDlg::ON_EXCEPTION_BREAKPOINT(DWORD dwExpAddress)
{

    //�ж��Ƿ���OEP�ϵ�
    if (m_IsOepBP)
    {
        //�ָ��ϵ�
        RecoverBP(m_tpInfo.hProcess,(DWORD)m_tpInfo.OepAddress,m_tpInfo.OriginalCode);
        //EIP--
        ReduceEIP();

        ShowReg(m_tpInfo.hThread);

        ShowAsm(dwExpAddress); 
        //����U�����Ĭ�ϵ�ַ
        m_Uaddress=dwExpAddress;
        
        //����ΪFALSE
        m_IsOepBP=FALSE;
        ShowProcessMemory(dwExpAddress);
        WaitForSingleObject(hEvent,INFINITE);
        return DBG_CONTINUE;
      
    }

    //���Ϊ�����ϵ��ֱ��ִ�й�ȥ����OD WINDBGһ��
    //�ж����û����õĶϵ㻹�Ǳ����Գ������ʹ��ڵĶϵ�ָ��
    if (isUserBP(dwExpAddress))
    {
        SetDlgItemText(IDC_STATE,TEXT("INT3�ϵ㵽��"));
       
        RecoverBP(m_tpInfo.hProcess,dwExpAddress,m_Recover_BP.bOrginalCode);
        //EIP--
        ReduceEIP();
        //������Զ�����ģʽ��INT3
        if (m_IsAutoF8)
        {

            
            ShowReg(m_tpInfo.hThread);
           
           ShowAsm(dwExpAddress);
           m_Uaddress=dwExpAddress;
           //ɾ��������ϵ� �������Զϵ�
            DeleteUserBP(m_tpInfo.hProcess,dwExpAddress);

            ON_VK_F8();

            return DBG_CONTINUE;
            
        }


        if (m_IsF8 || m_IsGo)
        {
            if(m_IsGo)
            {
                //����б��
                m_Result.ResetContent();

            }
            //ɾ��������ϵ� �������Զϵ�
            DeleteUserBP(m_tpInfo.hProcess,dwExpAddress);


           ShowReg(m_tpInfo.hThread);

           ShowAsm(dwExpAddress);
           m_Uaddress=dwExpAddress;

           m_IsF8=FALSE;
           m_IsGo=FALSE;

           
           WaitForSingleObject(hEvent,INFINITE);
        }
        

        return DBG_CONTINUE;
    }

    //�����û��ϵ� �Ͳ�����
  return DBG_EXCEPTION_NOT_HANDLED;
  
}




//�����쳣������  �����쳣��ַ
DWORD CEasyDbgDlg::ON_EXCEPTION_SINGLE_STEP(DWORD dwExpAddress)
{


    //�Ƿ����Զ�����ģʽ
    if (m_IsAutoF8)
    {
        //�õ���

        ShowAsm(dwExpAddress);
        ShowReg(m_tpInfo.hThread);
        ON_VK_F8();


        return DBG_CONTINUE;

    }
    //�Ƿ�Ϊ�Զ�����ģʽ
    if (m_IsAutoF7)
    {
        ShowAsm(dwExpAddress);
        ShowReg(m_tpInfo.hThread);
        ON_VK_F7();
        
        return DBG_CONTINUE;

    }
    //���ж���û��Ҫ���»ָ���INT3�ϵ�
    if (m_Recover_BP.isNeedRecover)
    {

        DebugSetBp(m_tpInfo.hProcess,m_Recover_BP.dwAddress,m_tpInfo.bCC);

        //������ΪFALSE
        m_Recover_BP.isNeedRecover=FALSE;
    }
    //�����Ӳ���ϵ�Ҫ�ָ�
    if (m_Recover_HBP.dwIndex!=-1)
    {
        //�ָ�Ӳ���ϵ�
        RecoverHBP(m_Recover_HBP.dwIndex);


        m_Recover_HBP.dwIndex=-1;
    }
    //������ڴ�ϵ�Ҫ�ָ�
    if (m_Recover_Mpage.isNeedRecover)
    {
        DWORD dwOldProtect=0;
        if (!VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)m_Recover_Mpage.dwBaseAddress,4,
                                  m_Recover_Mpage.dwProtect,&dwOldProtect)
            )
        {
            
            OutputDebugString("EasyDbgDlg.cpp 1595�г���");
            DWORD dwErrorCode=0;
            
            dwErrorCode=GetLastError();
            //���������Ϣ
            GetErrorMessage(dwErrorCode);
        }
        //������ΪFALSE
        m_Recover_Mpage.isNeedRecover=FALSE;
    }


    //�жϵ����쳣�ǲ�����ΪӲ���ϵ� ��DR6�ĵ���λ��û����λ
    DWORD dwBpAddress=0;
    if (IfStepHard(dwBpAddress))
    {
        SetDlgItemText(IDC_STATE,TEXT("Ӳ���ϵ㵽��"));
        //��Ӳ���ϵ���Ч
        InvalidHardBP(dwBpAddress);

    }
    //�������Ϊһ��Gģʽ���ڴ�ϵ�Ͳ�Ҫ�ȴ�ͬ���¼�
    if (m_isMoreMem)
    {
        
        if (m_IsF8)
        {
            m_IsGo=FALSE;
            m_isMoreMem=FALSE;
            //ShowAsm(dwExpAddress);
           // ShowReg(m_tpInfo.hThread);
            //����U�������ʼ��ַ
            m_Uaddress=dwExpAddress;
            
            //WaitForSingleObject(hEvent,INFINITE);
           //m_IsF8=FALSE;
           return DBG_CONTINUE;
 

        }
        //���G������N�ڴ�ϵ�ֱ�����е��ϵ�
        if (m_IsGo)
        {
            m_isMoreMem=FALSE;

            return DBG_CONTINUE;


        }
       
        m_isMoreMem=FALSE;
        ShowAsm(dwExpAddress);
        ShowReg(m_tpInfo.hThread);

        m_Uaddress=dwExpAddress;
        
        WaitForSingleObject(hEvent,INFINITE);
        
        return DBG_CONTINUE;

        
    }


    ShowAsm(dwExpAddress);
    ShowReg(m_tpInfo.hThread);
    //����U�������ʼ��ַ
    m_Uaddress=dwExpAddress;

    WaitForSingleObject(hEvent,INFINITE);

    return DBG_CONTINUE;
}

//���ڶϵ��쳣���� EIP--
void CEasyDbgDlg::ReduceEIP()
{

    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL;
    GetThreadContext(m_tpInfo.hThread,&ct);
    ct.Eip--;
    SetThreadContext(m_tpInfo.hThread,&ct);
    
}


//�ָ��ϵ�Ϊԭ���� ���ڶϵ��쳣
void CEasyDbgDlg::RecoverBP(HANDLE hProcess ,DWORD dwBpAddress,BYTE bOrignalCode)
{
    DWORD dwOldProtect=0;
    DWORD dwRet=0;
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);

    if (!WriteProcessMemory(hProcess,(LPVOID)dwBpAddress,&bOrignalCode,sizeof(bOrignalCode),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1694�г���");
        DWORD dwErrcode=0;
        dwErrcode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrcode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
        return ;
    }




    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
    //ˢ��
    FlushInstructionCache(hProcess,(LPCVOID)dwBpAddress,sizeof(BYTE));
    

    return;

}

//���öϵ�  �ϵ��ַ 0xCC �������öϵ����»ָ�Ϊ�ϵ�
void CEasyDbgDlg::DebugSetBp(HANDLE hProcess,DWORD dwBpAddress,BYTE bCCode)
{
    DWORD dwOldProtect=0;
    DWORD dwRet=0;
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
    if (!WriteProcessMemory(hProcess,(LPVOID)dwBpAddress,&bCCode,sizeof(bCCode),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1724�г���");
        DWORD dwErrcode=0;
        dwErrcode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrcode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
        return ;
    }

    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
    //ˢ��
    FlushInstructionCache(hProcess,(LPCVOID)dwBpAddress,sizeof(BYTE));

}

//�ж��Ƿ����û����õ�INT3�ϵ� ͨ����ѯINT3���� 
BOOL CEasyDbgDlg::isUserBP(DWORD dwBpAddress)
{
    POSITION pos=NULL;
    //��־�Ƿ��ҵ�
    BOOL isYes=FALSE;
    pos=m_Int3BpList.GetHeadPosition();
    while(pos!=NULL)
    {
       INT3_BP bp=m_Int3BpList.GetNext(pos);
       //�жϸöϵ��ַ�Ƿ��ڵ�ַ�б���
       if (bp.dwAddress==dwBpAddress)
       {
           //����ҵ�,�ж��Ƿ������öϵ� ������Ҫ�ڵ����쳣��������Ϊ�ϵ�
           //�ڵ����쳣������ϵ��������m_Recover_BP.isNeedRecoverΪFALSE
           m_Recover_BP.isNeedRecover=bp.isForever;
           m_Recover_BP.dwAddress=bp.dwAddress;
           m_Recover_BP.bOrginalCode=bp.bOriginalCode;
           

           isYes=TRUE;

           break;
       }
    }

    return isYes;

}


//���б����Գ���
void CEasyDbgDlg::OnRun() 
{
	// TODO: Add your command handler code here
    ON_VK_F9();
	
}

//�˳�����
void CEasyDbgDlg::OnQuit() 
{
	// TODO: Add your command handler code here
    ExitProcess(0);
	
}


//����U���� ���û�е�ַ�ʹ���ǰ�ĵ�ַ����U �ڵ������߶ϵ��쳣���ٰ������ַ��Ϊ��ǰEIP��ֵ
void CEasyDbgDlg::ON_U_COMMAND(DWORD dwAddress)
{
    //Ĭ����ʾ8��ָ��
    //���ָ���˵�ַ��ֵm_Uaddress
    if (dwAddress)
    {
        m_Uaddress=dwAddress;      
    }

    

   BYTE pCode[120]={0};
   DWORD dwOldProtect=0;
   DWORD dwRet=0;
   VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)m_Uaddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);

   if (!ReadProcessMemory(m_tpInfo.hProcess,(LPCVOID)m_Uaddress,pCode,sizeof(pCode),NULL))
   {
       OutputDebugString("EasyDbgDlg.cpp 1804�г���");
       DWORD dwErrorCode=0;
       dwErrorCode=GetLastError();
       GetErrorMessage(dwErrorCode);

       VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)m_Uaddress,4,dwOldProtect,&dwRet);
       return;

   }
  
   VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)m_Uaddress,4,dwOldProtect,&dwRet);


   //Ҫ�ж��Ƕϵ����� ����Ƕϵ�������ʾ��CC��Ҫ�ڻ������л�ԭ��
   for (int j=0;j<120;j++)
   {
       POSITION pos=NULL;
       pos = m_Int3BpList.GetHeadPosition();
       while(pos!=NULL)
       {
           INT3_BP bp=m_Int3BpList.GetNext(pos);
           //�ж϶ϵ��ַ�Ƿ���������λ�������
           if (bp.dwAddress==m_Uaddress+j)
           {
               //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
               pCode[j]=bp.bOriginalCode;
           }
       }
       
   }


   char szAsm[120]={0};
   char szOpCode[120]={0};
   UINT CodeSize=0;
   int nIndex=0;
   //��ʼ�����
   for (int i=0;i<8;i++)
   {
       
       Decode2AsmOpcode(&pCode[nIndex],szAsm,szOpCode,&CodeSize,m_Uaddress);
       //��ʾ���б��ؼ���
       char szResult[200]={0};
       EXPORT_FUN_INFO expFun={0};
       //����ǵ����������������
       if (IsExportFun(szAsm,expFun))
       {
           sprintf(szResult,"%08X    %s       %s <%s.%s>",m_Uaddress,szOpCode,szAsm,expFun.szDLLName,expFun.szFunName);
           m_Result.AddString(szResult);
          
           m_Result.SetTopIndex(m_Result.GetCount()-1);
           m_Uaddress+=CodeSize;
           nIndex+=CodeSize;
           continue;
           

       }
       sprintf(szResult,"%08X    %s        %s",m_Uaddress,szOpCode,szAsm);
       m_Result.AddString(szResult);
      
       m_Result.SetTopIndex(m_Result.GetCount()-1);
       m_Uaddress+=CodeSize;
       nIndex+=CodeSize;

   }



}

//���öϵ�
void CEasyDbgDlg::OnSetbp() 
{
	// TODO: Add your command handler code here
    CSetBP dlg;
    if (dlg.DoModal()==IDOK)
    {
        switch (dlg.m_Select)
        {
        case 1:
            //����INT3�ϵ�
            UserSetBP(m_tpInfo.hProcess,dlg.m_dwBpAddress,m_tpInfo.bCC);
            break;
        case 2:
            //����Ӳ���ϵ�
            SetHardBP(dlg.m_dwBpAddress,dlg.m_dwAttribute,dlg.m_dwLength);
            break;
        case 3:
            SetMemBP(dlg.m_dwBpAddress,dlg.m_dwAttribute,dlg.m_dwLength);
            break;
        }

    }

	
}

//�û����öϵ�
void CEasyDbgDlg::UserSetBP(HANDLE hProcess,DWORD dwBpAddress,BYTE bCCode)
{
    //�жϸõ�ַ�Ƿ��Ѿ��Ƕϵ�
    POSITION pos=NULL;
    INT3_BP bp={0};
    while (pos!=NULL)
    {
        bp=m_Int3BpList.GetNext(pos);
        if (bp.dwAddress==dwBpAddress)
        {
            AfxMessageBox("�˵�ַ�Ѿ����öϵ�,������Ч");
            return;
        }
    }
    memset(&bp,0,sizeof(INT3_BP));
    bp.dwAddress=dwBpAddress;
    bp.isForever=TRUE;

    DWORD dwOldProtect=0;
    DWORD dwRet=0;
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);

    //��ȡ���ֽ�
    if (!ReadProcessMemory(hProcess,(LPVOID)dwBpAddress,&bp.bOriginalCode,sizeof(BYTE),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1927�г���");
        DWORD dwErrcode=0;
        dwErrcode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrcode);

        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
        return ;

    }

    
     //д��0xCC
    if (!WriteProcessMemory(hProcess,(LPVOID)dwBpAddress,&bCCode,sizeof(bCCode),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 1942�г���");
        DWORD dwErrcode=0;
        dwErrcode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrcode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
        return ;
    }
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBpAddress,4,dwOldProtect,&dwRet);
    //ˢ��
    SetDlgItemText(IDC_STATE,"���öϵ�ɹ�");
    FlushInstructionCache(hProcess,(LPCVOID)dwBpAddress,sizeof(BYTE));
    m_Int3BpList.AddTail(bp);
    

}

//F9���Ĵ����� ����
void CEasyDbgDlg::ON_VK_F9()
{
    SetDlgItemText(IDC_STATE,"");
    m_IsGo=TRUE;
    SetEvent(hEvent);
    SetDlgItemText(IDC_STATE,"�����Գ������гɹ�");
}


//ɾ���û��ϵ�
void CEasyDbgDlg::DeleteUserBP(HANDLE hProcess,DWORD dwBpAddress)
{
    //�ж�Ҫɾ���ϵ��ַ�ڲ��ڶϵ�������
    POSITION pos=NULL;
    INT3_BP bp={0};
    BOOL isFind=FALSE;
    pos=m_Int3BpList.GetHeadPosition();
    while (pos!=NULL)
    {
        bp=m_Int3BpList.GetNext(pos);
        if (bp.dwAddress==dwBpAddress)
        {
            //���ǵ���ͬһ��ַ�������ϵ� ����ʱ�ϵ�����öϵ���G����ȫ����continue
            if (bp.isForever)
            {
               isFind=TRUE;
               //�ָ�Ϊԭ�����ֽ�
               RecoverBP(hProcess,dwBpAddress,bp.bOriginalCode);
               
               
               if (m_isDelete)
               {
                   
                   if (m_Int3BpList.GetCount()==1)
                   {
                       m_Int3BpList.RemoveHead();
                       m_isDelete=FALSE;
                       SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");
                       return;
                   }
                   
                   if (pos==NULL)
                   {
                       m_Int3BpList.RemoveTail();
                       m_isDelete=FALSE;
                       SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");
                       return;
                       
                       
                   }
                   
                   m_Int3BpList.GetPrev(pos);
                   
                   m_Int3BpList.RemoveAt(pos);
                   SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");
               }
              m_isDelete=FALSE;
                
               continue;;
            }
            else
            {
                //������ɾ�������öϵ�

                if (m_Int3BpList.GetCount()==1)
                {
                    m_Int3BpList.RemoveHead();
                    m_isDelete=FALSE;
                    SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");
                    return;
                }
                
                if (pos==NULL)
                {
                    m_Int3BpList.RemoveTail();
                    m_isDelete=FALSE;
                    SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");
                    return;
                    
                    
                }
                
                m_Int3BpList.GetPrev(pos);
                
                m_Int3BpList.RemoveAt(pos);
                SetDlgItemText(IDC_STATE,"�ϵ�ɾ���ɹ�");

                continue;
            }
            //�ҵ�

        }
    }
    //���û���ڶϵ��������ҵ��˵�ַ
    if (!isFind)
    {
         AfxMessageBox("Ҫɾ���Ķϵ�����Ч�ϵ�");
         return;
       
    }



    

}

//�鿴��ǰ�ϵ�
void CEasyDbgDlg::OnBreakpoint() 
{
	// TODO: Add your command handler code here
    //�Ȱ�ȫ�ֵ���� �����β鿴�����ظ�

    g_Int3BpList.RemoveAll();
    g_MemBpList.RemoveAll();
    POSITION pos=NULL;
    //��m_Int3BpList�Ķϵ㿽����g_Int3BpList��ȥ
    pos=m_Int3BpList.GetHeadPosition();
    while(pos!=NULL)
    {
        INT3_BP bp=m_Int3BpList.GetNext(pos);
        g_Int3BpList.AddTail(bp);

    }
    //�����ڴ�ϵ�
    pos=NULL;
    pos=m_MemBpList.GetHeadPosition();
     MEM_BP mBP={0};
    while (pos!=NULL)
    {
        mBP=m_MemBpList.GetNext(pos);
        g_MemBpList.AddTail(mBP);
       
    }

    CViewBP dlg;  
    
    dlg.DoModal();
 
	
}

//����B����
void CEasyDbgDlg::Handle_B_Command(char* szCommand)
{
    switch (szCommand[1])
    {
    case 'p':
    case 'P':
        {
        unsigned int dwAddress = 0 ;
        
        //��ȡP����ĵ�ַ
        sscanf(szCommand, "%s%x", stderr, &dwAddress);
        if (dwAddress==0)
        {
            AfxMessageBox("�������,������ϵ��ַ");
            break;
        }
        //���öϵ�
       UserSetBP(m_tpInfo.hProcess,dwAddress,m_tpInfo.bCC);
          
        break;
        }
    case 'l':
    case 'L':
        //�г���ǰ�ϵ�
        ListBP();
      
        break;
    case 'c':
    case 'C':
        {
            unsigned int dwAddress = 0 ;
            //��ȡC����ĵ�ַ
            sscanf(szCommand, "%s%x", stderr, &dwAddress);
            if (dwAddress==0)
            {
                AfxMessageBox("�������,������ϵ��ַ");
                return;
            }
            //ɾ�������Զϵ� ��־��ΪTRUE
            m_isDelete=TRUE;
            DeleteUserBP(m_tpInfo.hProcess,dwAddress);
        
        break;
        }
    case 'h':
        //����Ӳ���ϵ��ɾ��
    case 'H':
        {
            //���û������ָ��ֻ���򵥵��ж�
            if (szCommand[2]=='C'||szCommand[2]=='c')
            {
                unsigned int dwAddress = 0 ;
                //��ȡC����ĵ�ַ
                sscanf(szCommand, "%s%x", stderr, &dwAddress);
                if (dwAddress==0)
                {
                    AfxMessageBox("�������,������ϵ��ַ");
                    return;
                }
                DeleteHardBP(dwAddress);
               
            }
            else
            {

            DWORD dwAddress = 0 ;
            DWORD dwAttribute=0;
            DWORD dwLength=0;
            //��ȡ����ֵ
            sscanf(szCommand, "%s%x%x%x", stderr, &dwAddress, &dwAttribute, &dwLength);
            //����Ӳ���ϵ�
            SetHardBP(dwAddress,dwAttribute,dwLength);
            }

          break;
        }
    case 'm':
    case 'M':
        {
            //����ڴ�ϵ�
            if (szCommand[2]=='C'||szCommand[2]=='c')
            {
                unsigned int dwAddress = 0 ;
                //��ȡC����ĵ�ַ
                sscanf(szCommand, "%s%x", stderr, &dwAddress);
                if (dwAddress==0)
                {
                    AfxMessageBox("�������,������ϵ��ַ");
                    return;
                }
                DeleteMemBP(dwAddress);
                
            }
            else
            {

            DWORD dwAddress = 0 ;
            DWORD dwAttribute=0;
            DWORD dwLength=0;
            //��ȡ����ֵ
            sscanf(szCommand, "%s%x%x%x", stderr, &dwAddress, &dwAttribute, &dwLength);
            SetMemBP(dwAddress,dwAttribute,dwLength);
            }

            break;
        }
    }


}

//ö�ٶϵ�
void CEasyDbgDlg::ListBP()
{

    POSITION pos=NULL;
    pos=m_Int3BpList.GetHeadPosition();
    INT3_BP bp={0};
    CString szText;
    if (m_Int3BpList.GetCount()!=0)
    {
        //�о�INT3�ϵ�
        while(pos!=NULL)
        {
            bp=m_Int3BpList.GetNext(pos);
            szText.Format("INT3�ϵ� �ϵ��ַ:%08X   �ϵ㴦ԭ����:%2X �Ƿ������öϵ�: %d",bp.dwAddress,bp.bOriginalCode,bp.isForever);
            m_Result.AddString(szText);
            m_Result.SetTopIndex(m_Result.GetCount()-1);
            
        }
    }
    else
    {
        szText.Format("��ǰ��INT3�ϵ�");
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);
        

    }


    //�о�Ӳ���ϵ�
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2251�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;
    
    
    if (m_Dr_Use.Dr0)
    {
        szText.Format("Ӳ���ϵ� �ϵ��ַ:%08X �ϵ�����:%d  �ϵ㳤��:%d",ct.Dr0,tagDr7.DRFlag.rw0,tagDr7.DRFlag.len0+1);
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);
        
    }
    if (m_Dr_Use.Dr1)
    {
        szText.Format("Ӳ���ϵ� �ϵ��ַ:%08X �ϵ�����:%d  �ϵ㳤��:%d",ct.Dr1,tagDr7.DRFlag.rw1,tagDr7.DRFlag.len1+1);
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);
        
    }
    if (m_Dr_Use.Dr2)
    {
        szText.Format("Ӳ���ϵ� �ϵ��ַ:%08X �ϵ�����:%d  �ϵ㳤��:%d",ct.Dr2,tagDr7.DRFlag.rw2,tagDr7.DRFlag.len2+1);
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);
        
    }
    if (m_Dr_Use.Dr3)
    {
        szText.Format("Ӳ���ϵ� �ϵ��ַ:%08X �ϵ�����:%d  �ϵ㳤��:%d",ct.Dr3,tagDr7.DRFlag.rw3,tagDr7.DRFlag.len3+1);
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);
        
    }

    //�о��ڴ�ϵ�
    pos=NULL;
    pos=m_MemBpList.GetHeadPosition();
    MEM_BP mBP={0};
    if (m_MemBpList.GetCount()!=0)
    {
        while (pos!=NULL)
        {
            mBP=m_MemBpList.GetNext(pos);
            switch (mBP.dwNumPage)
            {
            case 1:
                {
                    szText.Format("�ڴ�ϵ� �ϵ��ַ:%08X �ϵ����� %d �ϵ㳤��:%d  �ϵ������ҳ:%08X",
                        mBP.dwBpAddress,mBP.dwAttribute,mBP.dwLength,mBP.dwMemPage[0]);
                    m_Result.AddString(szText);
                    m_Result.SetTopIndex(m_Result.GetCount()-1);
                    break;
                }
            case 2:
                {
                    szText.Format("�ڴ�ϵ� �ϵ��ַ:%08X �ϵ����� %d �ϵ㳤��:%d  �ϵ������ҳ:%08X %08X",
                        mBP.dwBpAddress,mBP.dwAttribute,mBP.dwLength,mBP.dwMemPage[0],mBP.dwMemPage[1]);
                    m_Result.AddString(szText);
                    m_Result.SetTopIndex(m_Result.GetCount()-1);
                    break;
                }
            case 3:
                {
                    szText.Format("�ڴ�ϵ� �ϵ��ַ:%08X �ϵ����� %d �ϵ㳤��:%d  �ϵ������ҳ:%08X %08X %08X",
                        mBP.dwBpAddress,mBP.dwAttribute,mBP.dwLength,mBP.dwMemPage[0],mBP.dwMemPage[1],
                        mBP.dwMemPage[2]);
                    m_Result.AddString(szText);
                    m_Result.SetTopIndex(m_Result.GetCount()-1);
                    
                    
                    break;
                }
            case 4:
                {
                    szText.Format("�ڴ�ϵ� �ϵ��ַ:%08X �ϵ����� %d �ϵ㳤��:%d  �ϵ������ҳ:%08X %08X %08X %08X",
                        mBP.dwBpAddress,mBP.dwAttribute,mBP.dwLength,mBP.dwMemPage[0],mBP.dwMemPage[1],
                        mBP.dwMemPage[2],mBP.dwMemPage[3]);
                    m_Result.AddString(szText);
                    m_Result.SetTopIndex(m_Result.GetCount()-1);
                    break;
                }
            case 5:
                {
                    szText.Format("�ڴ�ϵ� �ϵ��ַ:%08X �ϵ����� %d �ϵ㳤��:%d  �ϵ������ҳ:%08X %08X %08X %08X %08X",
                        mBP.dwBpAddress,mBP.dwAttribute,mBP.dwLength,mBP.dwMemPage[0],mBP.dwMemPage[1],
                        mBP.dwMemPage[2],mBP.dwMemPage[3],mBP.dwMemPage[4]);
                    m_Result.AddString(szText);
                    m_Result.SetTopIndex(m_Result.GetCount()-1);
                    
                    break;
                }
            }
            


        }

    }
    else
    {
        szText.Format("��ǰ���ڴ�ϵ�");
        m_Result.AddString(szText);
        m_Result.SetTopIndex(m_Result.GetCount()-1);

    }






}

//G�����
void CEasyDbgDlg::ON_G_COMMAND(DWORD dwAddress)
{
    //�����ָ����ַĬ�Ϻ�F9����һ��
    if(dwAddress==0)
    {
        m_Result.ResetContent();
       ON_VK_F9();
       return;
    }
    INT3_BP bp={0};
    bp.dwAddress=dwAddress;

    DWORD dwOldProtect=0;
    DWORD dwRet=0;

    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);


    if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwAddress,&bp.bOriginalCode,sizeof(BYTE),NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 2392�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrorCode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
        return;
    }
    //�����öϵ�
    bp.isForever=FALSE;
    //д��0XCC
    if (!WriteProcessMemory(m_tpInfo.hProcess,(LPVOID)dwAddress,&m_tpInfo.bCC,sizeof(BYTE),NULL))
    {

        OutputDebugString("EasyDbgDlg.cpp 2405�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���û����������Ϣ
        GetErrorMessage(dwErrorCode);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
        return;

    }

    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
    //����ϵ�����
    m_Int3BpList.AddTail(bp);

    m_IsGo=TRUE;
    //����
    ON_VK_F9();



}


//����Ӳ���ϵ� ���� ��ַ ���� ����
//dwAttribute 0��ʾִ�жϵ� 3��ʾ���ʶϵ� 1 ��ʾд��ϵ�
//dwLength ȡֵ 1 2 4
void CEasyDbgDlg::SetHardBP(DWORD dwBpAddress,DWORD dwAttribute,DWORD dwLength)
{
    

    if (dwLength!=1 && dwLength!=2 &&dwLength!=4)
    {
        AfxMessageBox("�ϵ㳤�����ô���");
        return;
    }
    //ǿ�ư�ִ�жϵ�ĳ��ȸ�Ϊ1
    if (dwAttribute==0)
    {
        dwLength=1;
    }
    
    int nIndex=0;
    //��õ�ǰ���е��Ĵ������
    nIndex=FindFreeDebugRegister();

    if (nIndex==-1)
    {
        AfxMessageBox("��ǰӲ���ϵ�����,��ɾ��������");
        return;
    }
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2460�г���");

        DWORD dwErrorCode=0;

        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;

    }
    //��ֵ���Ƕ����DR7�ṹ��,����ʡȥ��λ�Ʋ����ķ���
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;

    switch (nIndex)
    {
    case 0:
        //�жϵ�ַ
        ct.Dr0=dwBpAddress;
        //�ϵ㳤��
        tagDr7.DRFlag.len0=dwLength-1;
        //����
        tagDr7.DRFlag.rw0=dwAttribute;
        //�ֲ��ϵ�
        tagDr7.DRFlag.L0=1;
        //���ñ�־λ��¼���ԼĴ�����ʹ�����
        m_Dr_Use.Dr0=TRUE;

        break;
    case 1:
        ct.Dr1=dwBpAddress;

        tagDr7.DRFlag.len1=dwLength-1;

        tagDr7.DRFlag.rw1=dwAttribute;

        tagDr7.DRFlag.L1=1;

        m_Dr_Use.Dr1=TRUE;
        

        break;
    case 2:
        ct.Dr2=dwBpAddress;

        tagDr7.DRFlag.len2=dwLength-1;
        
        tagDr7.DRFlag.rw2=dwAttribute;

        tagDr7.DRFlag.L2=1;

        m_Dr_Use.Dr2=TRUE;

        break;
    case 3:
        ct.Dr3=dwBpAddress;

        tagDr7.DRFlag.len3=dwLength-1;

        tagDr7.DRFlag.rw3=dwAttribute;

        tagDr7.DRFlag.L3=1;

        m_Dr_Use.Dr3=TRUE;
        break;
    }

    //��ֵ��ȥ
    ct.Dr7=tagDr7.dwDr7;
    if (!SetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2531�г���");
        DWORD dwErrorCode=0;

        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    SetDlgItemText(IDC_STATE,"����Ӳ���ϵ�ɹ�");

   
}


//���ص�ǰ���õĵ��ԼĴ���
int CEasyDbgDlg::FindFreeDebugRegister()
{
     if (!m_Dr_Use.Dr0)
     {
         return 0;

     }
     if (!m_Dr_Use.Dr1)
     {
         return 1;
     }
     if (!m_Dr_Use.Dr2)
     {
         return 2;

     }
     if (!m_Dr_Use.Dr3)
     {
         return 3;
     }
     //���Dr0-Dr3����ʹ���򷵻�-1
     return -1;
}

//ɾ��Ӳ���ϵ�
void CEasyDbgDlg::DeleteHardBP(DWORD dwAddress)
{

    if (dwAddress==0)
    {
        AfxMessageBox("û����ϵ��ַ");
        return;
    }
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2583�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;
    //�ҵ���Ӧ�ϵ�ĵ��ԼĴ���
    int nIndex=GetDeletedDrIndex(dwAddress,ct);
    if (nIndex==-1)
    {
        AfxMessageBox("�ϵ���Ч");
        return;
    }
   //��0�����ö�Ӧ��־λΪFALSE
    switch (nIndex)
    {
    case 0:
        //��ַ
        ct.Dr0=0;
        //����
        tagDr7.DRFlag.rw0=0;
        //�ֲ��ϵ�
        tagDr7.DRFlag.L0=0;
        //����
        tagDr7.DRFlag.len0=0;

        m_Dr_Use.Dr0=FALSE;
        break;
    case 1:
        
        ct.Dr1=0;
        
        tagDr7.DRFlag.rw1=0;
        
        tagDr7.DRFlag.L1=0;
        
        tagDr7.DRFlag.len1=0;

        m_Dr_Use.Dr1=FALSE;
        break;
    case 2:
        
        ct.Dr2=0;
        
        tagDr7.DRFlag.rw2=0;
        
        tagDr7.DRFlag.L2=0;
        
        tagDr7.DRFlag.len2=0;

        m_Dr_Use.Dr2=FALSE;
        break;
    case 3:
        
        ct.Dr3=0;
        
        tagDr7.DRFlag.rw3=0;
        
        tagDr7.DRFlag.L3=0;
        
        tagDr7.DRFlag.len3=0;

        m_Dr_Use.Dr3=FALSE;
        break;
    }
    //��ֵ
    ct.Dr7=tagDr7.dwDr7;

    if (!SetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2656�г���");

        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);

        return;

    }
    SetDlgItemText(IDC_STATE,TEXT("ɾ��Ӳ���ϵ�ɹ�"));


}


//���Ҫ��ɾ����Ӳ���ϵ�ĵ��ԼĴ������ ����-1��ʾû�ҵ�
int CEasyDbgDlg::GetDeletedDrIndex(DWORD dwAddress,CONTEXT ct)
{
    if (dwAddress == ct.Dr0)
    {
        return 0;
    }
    if (dwAddress == ct.Dr1)
    {
        return 1;
    }
    if (dwAddress == ct.Dr2)
    {
        return 2;
    }
    if (dwAddress == ct.Dr3)
    {
        return 3;
    }

    return -1;

}


//�жϵ����쳣�Ƿ���Ӳ���ϵ������ �������� �ϵ��ַ
BOOL CEasyDbgDlg::IfStepHard(DWORD& dwBPAddress)
{

    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2705�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return FALSE;
    }
    //�ж�Dr6�ĵ�4λ�Ƿ�Ϊ0
    int nIndex=ct.Dr6 &0xf;
    if (nIndex==0)
    {
        return FALSE;
    }

    switch (nIndex)
    {
    case 0x1:
        //�����ҵ��Ķϵ��ַ
        dwBPAddress=ct.Dr0;
        break;
    case 0x2:
        dwBPAddress=ct.Dr1;
        break;
    case 0x4:
        dwBPAddress=ct.Dr2;
        break;
    case 0x8:
       dwBPAddress=ct.Dr3;
        break;
    }
    return TRUE;


}

//ʹӲ���ϵ���ʱ��Ч
void CEasyDbgDlg::InvalidHardBP(DWORD dwBpAddress)
{
    //����������ж�
    if (dwBpAddress==0)
    {
        AfxMessageBox("�ϵ�Ϊ0��Чֵ");
        return;
    }

    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2754�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    //�ж��жϵ�ַ���Ǹ����ԼĴ�����
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;
//     DWORD dwDr0=ct.Dr0;
//     DWORD dwDr1=ct.Dr1;
//     DWORD dwDr2=ct.Dr2;
//     DWORD dwDr3=ct.Dr3;
//     switch (dwBpAddress)
//     {
// 
//     case dwDr0:
//         //��Lλ�öϵ���Ч �����ַ��,��û��ñ����ַ
//         tagDr7.DRFlag.L0=0;
//         //����Ҫ�ָ��ĵ��ԼĴ������
//         m_Recover_HBP.dwIndex=0;
// 
//         break;
//     case dwDr1:
//         tagDr7.DRFlag.L1=0;
//         m_Recover_HBP.dwIndex=1;
// 
//         break;
//     case dwDr2:
//         tagDr7.DRFlag.L2=0;
//         m_Recover_HBP.dwIndex=2;
// 
//         break;
//     case dwDr3:
//         tagDr7.DRFlag.L3=0;
//         m_Recover_HBP.dwIndex=3;
//         break;
// 
//     }
    if (ct.Dr0==dwBpAddress)
    {
        //��Lλ�öϵ���Ч �����ַ��,��û��ñ����ַ
        tagDr7.DRFlag.L0=0;
        //����Ҫ�ָ��ĵ��ԼĴ������
        m_Recover_HBP.dwIndex=0;

    }

    if (ct.Dr1==dwBpAddress)
    {
        tagDr7.DRFlag.L1=0;
        m_Recover_HBP.dwIndex=1;

    }

    if (ct.Dr2==dwBpAddress)
    {
        tagDr7.DRFlag.L2=0;
        m_Recover_HBP.dwIndex=2;

    }

    if (ct.Dr3==dwBpAddress)
    {
        tagDr7.DRFlag.L3=0;
        m_Recover_HBP.dwIndex=3;     
    }

    //��ֵ��ȥ
    ct.Dr7=tagDr7.dwDr7;
    //�����߳�������
    if (!SetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2828�г���");

        DWORD dwErrorCode=0;

        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }


}



//�ָ�Ӳ���ϵ� ����Ϊ ���ԼĴ����ı��
void CEasyDbgDlg::RecoverHBP(DWORD dwIndex)
{
    //����������ж�
    if (dwIndex==-1)
    {
        AfxMessageBox("�ָ�Ӳ���ϵ����");
        return;
    }

    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2857�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    
    DR7 tagDr7={0};
    tagDr7.dwDr7=ct.Dr7;

    switch (dwIndex)
    {

    case 0:
        //����Lλ
        tagDr7.DRFlag.L0=1;
        m_Recover_HBP.dwIndex=-1;
        break;
    case 1:
        tagDr7.DRFlag.L1=1;
        m_Recover_HBP.dwIndex=-1;
        break;
    case 2:
        tagDr7.DRFlag.L2=1;
        m_Recover_HBP.dwIndex=-1;
        break;
    case 3:
        tagDr7.DRFlag.L3=1;
        m_Recover_HBP.dwIndex=-1;
        break;

    }
    //д ��CONTEXT
    ct.Dr7=tagDr7.dwDr7;

    if (!SetThreadContext(m_tpInfo.hThread,&ct))
    {
        OutputDebugString("EasyDbgDlg.cpp 2895�г���");
        
        DWORD dwErrorCode=0;
        
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }



    

}




//�����ڴ�ϵ�  dwAttribute 1��ʾд��ϵ� 3��ʾ���ʶϵ�
void CEasyDbgDlg::SetMemBP(DWORD dwBpAddress,DWORD dwAttribute,DWORD dwLength)
{
    if (dwAttribute!=1 && dwAttribute!=3)
    {
        AfxMessageBox("�ڴ�ϵ�����Ū��");
        return;
    }
    
    MEMORY_BASIC_INFORMATION mbi={0};
    MEM_BP mbp={0};

    if (!IsAddressValid(dwBpAddress,mbi))
    {
        AfxMessageBox("�ϵ��ַ��Ч");
        return;
    }
    //�жϵ�ַ�ͳ���ռ�˼�����ҳ�������ڴ��ҳ�� Ҳ�Ѷϵ����ϵ��
    if (!AddMemBpPage(dwBpAddress,dwLength,mbi,dwAttribute,mbp))
    {
        AfxMessageBox("�ϵ����ʧ��");
        return;
    }
    //�ñ�������
    for (DWORD i=0;i<mbp.dwNumPage;i++)
    {
        DWORD dwOldProtect=0;
        
        if (!VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)mbp.dwMemPage[i],4,m_Attribute[dwAttribute],&dwOldProtect))
        {
            OutputDebugString("EasyDbgDlg.cpp 2944�г���");
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���������Ϣ
            GetErrorMessage(dwErrorCode);
            AfxMessageBox("�ϵ����ʧ��");
            return;

        }
        
        
    }

   SetDlgItemText(IDC_STATE,"�ڴ�ϵ����óɹ�");  
    
}


//�жϵ�ַ�Ƿ���Ч
BOOL CEasyDbgDlg::IsAddressValid(DWORD dwAddress,MEMORY_BASIC_INFORMATION& mbi)
{
    

    DWORD dwRet=0;

    dwRet=VirtualQueryEx(m_tpInfo.hProcess,(LPVOID)dwAddress,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
    //����ֵ���仺�������Ȳ���ͬ���ʾ��ַ��Ч
    if (dwRet!=sizeof(MEMORY_BASIC_INFORMATION))
    {
        return FALSE;
    }
    //MEM_FREE �Ĳ��ܷ��� MEM_RESERVE�ı�������δ֪
    if (mbi.State==MEM_COMMIT)
    {
        return TRUE;
    }

    return FALSE;
}


//�жϵ�ַ�ͳ���ռ�˼�����ҳ�������ڴ��ҳ�� ���Ѷϵ�Ҳ����ϵ�����
BOOL CEasyDbgDlg::AddMemBpPage(DWORD dwBpAddress,DWORD dwLength,MEMORY_BASIC_INFORMATION mbi,DWORD dwAttribute,MEM_BP& mbp)
{
    //�����һ����ҳ��(�����ҳ)
    MEM_BP_PAGE mBPage={0};
    
    
    if (dwBpAddress>=(DWORD)mbi.BaseAddress && (DWORD)mbi.BaseAddress+mbi.RegionSize>=dwBpAddress+dwLength)
    {
        mBPage.dwBaseAddress=(DWORD)mbi.BaseAddress;
        mBPage.dwProtect=mbi.Protect;
        //���ڴ�������û�ҵ������
        if (!FindMemPage((DWORD)mbi.BaseAddress))
        {
              m_MemPageList.AddTail(mBPage);
        }
        //����ڴ�����
        mbp.dwAttribute=dwAttribute;
        mbp.dwBpAddress=dwBpAddress;
        mbp.dwLength=dwLength;
        mbp.dwMemPage[0]=mBPage.dwBaseAddress;
        mbp.dwNumPage=1;
        //�鿴�õ�ַ���Ƿ��Ѿ����ڴ�ϵ�����в������¶ϵ�
        if (FindMemBP(dwBpAddress))
        {
             AfxMessageBox("�õ�ַ�Ѿ����ڴ�ϵ�,�������¶ϵ�");
             return FALSE;
        }
        else
        {   
            //��ӵ��ϵ�����
            m_MemBpList.AddTail(mbp);
        }
        return TRUE;


    }
    //������ҳ����� ��Ϊ��̫��ҳ�����Բ���Ϊ,��Ϊ�Ͳ��������е��ڴ�ҳ������
    //ֱ�ӱȽ� ��ʵ��3��ҳ�ľ������Բ���Ϊ....
    int i=0;

    mbp.dwAttribute=dwAttribute;
    mbp.dwBpAddress=dwBpAddress;
    mbp.dwLength=dwLength;

    while ((DWORD)mbi.BaseAddress+mbi.RegionSize<dwBpAddress+dwLength)
    {
        if (i>4)
        {
            AfxMessageBox("�Ҷ�����������ô���ҳ���ڴ�ϵ�");
            return FALSE;
        }
        mBPage.dwBaseAddress=(DWORD)mbi.BaseAddress;
        mBPage.dwProtect=mbi.Protect;
        //���ڴ�������û�ҵ������
        if (!FindMemPage((DWORD)mbi.BaseAddress))
        {
            m_MemPageList.AddTail(mBPage);
        }
        mbp.dwMemPage[i]=mBPage.dwBaseAddress;

        DWORD dwRet=0;

        //����һ����ҳ
        dwRet=VirtualQueryEx(m_tpInfo.hProcess,(LPVOID)((DWORD)mbi.BaseAddress+mbi.RegionSize),&mbi,sizeof(MEMORY_BASIC_INFORMATION));
        //����ֵ���仺�������Ȳ���ͬ���ʾ��ַ��Ч
        if (dwRet!=sizeof(MEMORY_BASIC_INFORMATION))
        {
            return FALSE;
        }

        i++;

    
    }

    if (i>4)
    {
        AfxMessageBox("�Ҷ�����������ô���ҳ���ڴ�ϵ�");
        return FALSE;
    }

    mBPage.dwBaseAddress=(DWORD)mbi.BaseAddress;
    mBPage.dwProtect=mbi.Protect;
    //���ڴ�������û�ҵ������
    if (!FindMemPage((DWORD)mbi.BaseAddress))
    {
        m_MemPageList.AddTail(mBPage);
    }
    mbp.dwMemPage[i]=mBPage.dwBaseAddress;

    if (FindMemBP(dwBpAddress))
    {
        AfxMessageBox("�õ�ַ�Ѿ����ڴ�ϵ�,�������¶ϵ�");
        return FALSE;
    }
    else
    {   
        //��ӵ��ϵ�����
        m_MemBpList.AddTail(mbp);
    }
    return TRUE;

}

//�ж�ĳһҳ�׵�ַ�Ƿ������ҳ������
BOOL CEasyDbgDlg::FindMemPage(DWORD dwBaseAddress)
{

    POSITION pos;
    pos=m_MemPageList.GetHeadPosition();
    while (pos!=NULL)
    {
        MEM_BP_PAGE mBPage={0};
        mBPage=m_MemPageList.GetNext(pos);
        //����ҵ�����TRUE
        if (mBPage.dwBaseAddress==dwBaseAddress)
        {
            return TRUE;
          
        }
    }
    return FALSE;
}


//�жϵ�ַ�Ƿ��ظ����ڴ�ϵ�
BOOL CEasyDbgDlg::FindMemBP(DWORD dwBpAddress)
{
     POSITION pos=NULL;
     pos=m_MemBpList.GetHeadPosition();
     while (pos!=NULL)
     {
         MEM_BP memBp={0};
         memBp=m_MemBpList.GetNext(pos);
         //����ҵ�����TRUE
         if (memBp.dwBpAddress==dwBpAddress)
         {
             return TRUE;
         }

     }
     return FALSE;
}

//����EXCEPTION_ACCESS_VIOLATION�쳣 ���� �쳣��ַ
DWORD CEasyDbgDlg::ON_EXCEPTION_ACCESS_VIOLATION(DWORD dwExpAddress,DWORD dwAccessAddress)
{
    
    if (dwAccessAddress==0)
    {
        return DBG_EXCEPTION_NOT_HANDLED;
    }
    //�ó���ǰ��ַ���ڵķ�ҳ
    DWORD dwRet=0;
    MEMORY_BASIC_INFORMATION mbi={0};
    dwRet=VirtualQueryEx(m_tpInfo.hProcess,(LPVOID)dwAccessAddress,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
    //����ֵ���仺�������Ȳ���ͬ���ʾ��ַ��Ч
    if (dwRet!=sizeof(MEMORY_BASIC_INFORMATION))
    {
        OutputDebugString("EasyDbgDlg.cpp 3143�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();

        //���������Ϣ
        GetErrorMessage(dwErrorCode);

        return DBG_EXCEPTION_NOT_HANDLED;
    }
    //������Ƕϵ� �����ڴ��ҳ�����ҵ�����׵�ַ
    if (!FindMemBP(dwAccessAddress))
    {
        if (FindMemOriginalProtect(mbi))
        {
            //�ָ��ϵ�
            RecoverMemBP((DWORD)mbi.BaseAddress,mbi.Protect);

            
            return DBG_CONTINUE;
        }
        //���û�ҵ� �Ǿ���Ӧ�ó����Լ��ķ����쳣
        else
        {

            return DBG_EXCEPTION_NOT_HANDLED;

        }
    
    }
    //����Ƕϵ�

    if (FindMemOriginalProtect(mbi))
    {
        SetDlgItemText(IDC_STATE,TEXT("�ڴ�ϵ㵽��"));
        //�ָ��ϵ�
        RecoverMemBP((DWORD)mbi.BaseAddress,mbi.Protect);
        //�����GOģʽ,����ͣ������ʾ�����Ĵ����� ����ģʽֱ���߹�ȥ
        if (m_IsGo)
        {
            ShowAsm(dwExpAddress);
            ShowReg(m_tpInfo.hThread);
            m_IsGo=FALSE;
            CONTEXT ct={0};
            ct.ContextFlags=CONTEXT_FULL;
            GetThreadContext(m_tpInfo.hThread,&ct);
            ct.EFlags|=0x100;
            SetThreadContext(m_tpInfo.hThread,&ct);
            //������־ ���ڵ����в���ʾ
            m_isMoreMem=TRUE;
            WaitForSingleObject(hEvent,INFINITE);
            
        }
        
        
        return DBG_CONTINUE;
    }
    //���û�ҵ� �Ǿ���Ӧ�ó����Լ��ķ����쳣
    else
    {
        
        return DBG_EXCEPTION_NOT_HANDLED;
        
    }



   

    return DBG_CONTINUE;
    
}


//��ʱ�ָ��ڴ�ϵ�(ʹ�ڴ�ϵ���Ч) ���� �ڴ�ҳ��ַ ԭ��������
void CEasyDbgDlg::RecoverMemBP(DWORD dwBaseAddress,DWORD dwProtect)
{
    if (dwBaseAddress==0)
    {
        AfxMessageBox("�ڴ�ϵ�ָ�ʧ��");
        return;
    }
    m_Recover_Mpage.dwBaseAddress=dwBaseAddress;
    m_Recover_Mpage.isNeedRecover=TRUE;

    DWORD dwOldProtect=0;
    if (!VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBaseAddress,4,dwProtect,&dwOldProtect))
    {
        
        OutputDebugString("EasyDbgDlg.cpp 3232�г���");
        DWORD dwErrorCode=0;

        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        return;
    }
    //����ϵ�ı�������
    m_Recover_Mpage.dwProtect=dwOldProtect;
    

}


//�ҵ�ԭʼ�ڴ�ҳ����Ķ�Ӧ���Բ����� ��������
BOOL CEasyDbgDlg::FindMemOriginalProtect(MEMORY_BASIC_INFORMATION& mbi)
{

    POSITION pos;
    pos=m_MemPageList.GetHeadPosition();
    while (pos!=NULL)
    {
        MEM_BP_PAGE mBPage={0};
        mBPage=m_MemPageList.GetNext(pos);
        //����ҵ�����TRUE
        if (mBPage.dwBaseAddress==(DWORD)mbi.BaseAddress)
        {
            //��ֵԭʼ����
            mbi.Protect=mBPage.dwProtect;
            return TRUE;
          
        }
    }
    return FALSE;


}

//ɾ���ڴ�ϵ�
void CEasyDbgDlg::DeleteMemBP(DWORD dwBpAddress)
{
    MEM_BP mBP={0};
    //�ҵ��ڴ�ϵ㲢���������Ƴ�
    if (!FindMemBPInformation(mBP,dwBpAddress))
    {
        AfxMessageBox("�˵�ַ���Ƕϵ�");
        return;
    }

    for (DWORD i=0;i<mBP.dwNumPage;i++)
    {
        //���ж���û����һ���ڴ�ϵ��������ҳ��,������ھ��޸�Ϊ��һ���ϵ���Ҫ�������
        if (!ModifyPageProtect(mBP.dwMemPage[i]))
        {
            //���û�������ڴ�ϵ��ڴ��ڴ�ҳ�Ͼ�ֱ�ӱ����ڴ�ҳ���޸�Ϊԭ��������
            MEMORY_BASIC_INFORMATION mbi={0};
            mbi.BaseAddress=(PVOID)mBP.dwMemPage[i];
            //����ɹ�����ԭ����
            if (FindMemOriginalProtect(mbi))
            {
                DWORD dwOldProtect=0;
                //�޸�
                if (!VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)mBP.dwMemPage[i],4,mbi.Protect,&dwOldProtect))
                {
                    OutputDebugString("EasyDbgDlg.cpp 3299�г���");
                    DWORD dwErrorCode=0;
                    dwErrorCode=GetLastError();
                    //���������Ϣ
                    GetErrorMessage(dwErrorCode);
                }
                

            }

        }


    }


}

//�ҵ����ϵ��ڴ�ϵ���Ϣ������ ��������Ϊ���� ����������ɾ����Ԫ��
BOOL CEasyDbgDlg::FindMemBPInformation(MEM_BP& mBP,DWORD dwBpAddress)
{

    POSITION pos=NULL;
    pos=m_MemBpList.GetHeadPosition();
    while (pos!=NULL)
    {
        
        mBP=m_MemBpList.GetNext(pos);
        //����ҵ�����TRUE
        if (mBP.dwBpAddress==dwBpAddress)
        {
            
            if (m_MemBpList.GetCount()==1)
            {
                m_MemBpList.RemoveHead();
                
                SetDlgItemText(IDC_STATE,"�ڴ�ϵ�ɾ���ɹ�");
                return TRUE;
                
            }
            
            if (pos==NULL)
            {
                m_MemBpList.RemoveTail();
                
                SetDlgItemText(IDC_STATE,"�ڴ�ϵ�ɾ���ɹ�");
                return TRUE;
                           
            }
            
            m_MemBpList.GetPrev(pos);
            
            m_MemBpList.RemoveAt(pos);
            SetDlgItemText(IDC_STATE,"�ڴ�ϵ�ɾ���ɹ�");

            return TRUE;
        }
        
    }
     return FALSE;

}



//���ж���û����һ���ڴ�ϵ��������ҳ��,������ھ��޸�Ϊ��һ���ϵ���Ҫ�������
//���� �ڴ�ҳ���׵�ַ
BOOL CEasyDbgDlg::ModifyPageProtect(DWORD dwBaseAddress)
{
    POSITION pos=NULL;
    pos=m_MemBpList.GetHeadPosition();
    
    MEM_BP mBP={0};
    BOOL isFind=FALSE;
     //����
    while(pos!=NULL)
    {
        mBP=m_MemBpList.GetNext(pos);
        for (DWORD i=0;i<mBP.dwNumPage;i++)
        {
            //����ҵ��ڴ�ϵ㻹�ڴ��ڴ�ҳ����
            if (mBP.dwMemPage[i]==dwBaseAddress)
            {
                isFind=TRUE;
                DWORD dwOldProtect=0;
                //�Ͱ��ڴ汣�����Իָ�Ϊ��������Ҫ��
                if (!VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwBaseAddress,4,
                    m_Attribute[mBP.dwAttribute],&dwOldProtect))
                {
                    OutputDebugString("EasyDbgDlg.cpp 3387�г���");
                    DWORD dwErrorCode=0;
                    dwErrorCode=GetLastError();
                    
                    GetErrorMessage(dwErrorCode);
                }
            }
        }


    }
    
    return isFind;
}




//PE�鿴����
void CEasyDbgDlg::OnViewpe() 
{
	// TODO: Add your command handler code here

    //Ĭ�ϲ鿴���Ǳ����Գ����PE��Ϣ
    CPeScan dlg;
    dlg.DoModal();
	
}

//ӳ���ļ� �����PE��Ч���Լ��ǲ���EXE�ļ�
BOOL CEasyDbgDlg::MapPEFile()
{
    HANDLE hFile=NULL;
    //���ļ�����ļ����
    hFile=CreateFile(m_SzFilePath,GENERIC_ALL,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        OutputDebugString("EasyDbgDlg.cpp 3424�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        
        return FALSE;
    }
    HANDLE hFileMap=NULL;
    //�����ļ�ӳ��
    hFileMap=CreateFileMapping(hFile,NULL,PAGE_READWRITE,0,0,NULL);
    if (hFileMap==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 3437�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        CloseHandle(hFile);
        return FALSE;
    }
    //ӳ���ļ�
    pFile=(char*)MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,0,0,0);
    if (pFile==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 3448�г���");
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        GetErrorMessage(dwErrorCode);
        
        CloseHandle(hFile);
        CloseHandle(hFileMap);
        return FALSE;
    }

     //�ж�PE��Ч��
    PIMAGE_DOS_HEADER pDos=NULL;
    pDos=(PIMAGE_DOS_HEADER)pFile;
    PIMAGE_NT_HEADERS pNt=(PIMAGE_NT_HEADERS)(pFile+pDos->e_lfanew);
    
    //���MZ PE ������־
    if (pDos->e_magic!=IMAGE_DOS_SIGNATURE || pNt->Signature!=IMAGE_NT_SIGNATURE)
    {
        AfxMessageBox("������Ч��PE�ļ�");
        CloseHandle(hFile);
        CloseHandle(hFileMap);
        return FALSE;
    }
    if (pNt->FileHeader.Characteristics&IMAGE_FILE_DLL)
    {
        AfxMessageBox("���ļ���DLL,EXE�ļ�");
        CloseHandle(hFile);
        CloseHandle(hFileMap);
        return FALSE;
    }
    
   
    CloseHandle(hFile);
    CloseHandle(hFileMap);

    return TRUE;
    
}




// ��õ��������ַ
BOOL CEasyDbgDlg::GetExportFunAddress(HANDLE hFile,char* pDll,LPVOID pBase)
{
   

    PIMAGE_DOS_HEADER pDos=NULL;
    PIMAGE_FILE_HEADER pFileHeader=NULL;
    PIMAGE_OPTIONAL_HEADER pOption=NULL;
    PIMAGE_SECTION_HEADER pSec=NULL;

    //��ȡ���ṹ��ָ��
    pDos=(PIMAGE_DOS_HEADER)pDll;
 
    pFileHeader=(PIMAGE_FILE_HEADER)(pDll+pDos->e_lfanew+4);
    pOption=(PIMAGE_OPTIONAL_HEADER)((char*)pFileHeader+sizeof(IMAGE_FILE_HEADER));
    pSec=(PIMAGE_SECTION_HEADER)((char*)pOption+pFileHeader->SizeOfOptionalHeader);
    //�ڱ���Ŀ
    DWORD dwSecNum=0;
    dwSecNum=pFileHeader->NumberOfSections;
    //������ƫ��
    DWORD dwExportRva=0;
       
    dwExportRva=pOption->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;


    DWORD dwExportOffset=0;
    //��õ������ļ�ƫ��
    dwExportOffset=RvaToFileOffset(dwExportRva,dwSecNum,pSec);
    PIMAGE_EXPORT_DIRECTORY pExp=NULL;
    pExp=(PIMAGE_EXPORT_DIRECTORY)(pDll+dwExportOffset);

    EXPORT_FUN_INFO ExFun={0};
   
    
    DWORD dwNameOffset=0;
    dwNameOffset=RvaToFileOffset(pExp->Name,dwSecNum,pSec);
    char*pName=NULL;
    //DLL��
    pName=(char*)(pDll+dwNameOffset);
    strcpy(ExFun.szDLLName,pName);
    
    DWORD dwBase=0;
    dwBase=pExp->Base;
    DWORD dwFunNum=0;
    dwFunNum=pExp->NumberOfFunctions;
    for (DWORD j=0;j<dwFunNum;j++)
    {
        //�ȱ���������ַ����
        PDWORD pAddr=(PDWORD)(pDll+RvaToFileOffset(pExp->AddressOfFunctions,dwSecNum,pSec));
        //��ַ��Ч
        if (pAddr[j]!=0)
        {
            //ͨ����ŵõ���Ӧ�����������±�
            //�������
            PWORD pNum=(PWORD)(pDll+RvaToFileOffset(pExp->AddressOfNameOrdinals,dwSecNum,pSec));
            for (WORD k=0;k<pExp->NumberOfNames;k++ )
            {
                //������������������ͬ�� �ҵ��±�Ȼ���������
                if (j==pNum[k])
                {
                    //����������(�����������) �õ�����RVA
                    PDWORD pName=(PDWORD)(pDll+RvaToFileOffset(pExp->AddressOfNames,dwSecNum,pSec));
                    
                    char *pszName=(char*)(pDll+RvaToFileOffset(pName[k],dwSecNum,pSec));
                    
                    memcpy(&ExFun.szFunName,pszName,strlen(pszName)+1);
                    
                    
                    if (pBase)
                    {
                        ExFun.dwAddress=(DWORD)pBase+pAddr[j];
                        //����CMAP��
                        m_ExFunList.SetAt(ExFun.dwAddress,ExFun);
                        //���뺯�������ַ��Ӧ��
                        m_Fun_Address.SetAt(pszName,ExFun.dwAddress);
                    }
                    
                    
                    break;
                }
            }
            
            
        }
       
        
    }


    return TRUE;
  
}


//����һ ������RVA ����2��������Ŀ ����3�������׵�ַ
DWORD CEasyDbgDlg::RvaToFileOffset(DWORD dwRva,DWORD dwSecNum,PIMAGE_SECTION_HEADER pSec)
{
    if (dwSecNum==0)
    {
        return 0;
    }
    
    for (DWORD i=0;i<dwSecNum;i++)
    {
        
        if (dwRva>=pSec[i].VirtualAddress&&dwRva<pSec[i].VirtualAddress+pSec[i].SizeOfRawData)
        {
            
            return dwRva-pSec[i].VirtualAddress+pSec[i].PointerToRawData;
            
        }
    }
    return 0;
    
}


//�������DLL�¼�
void CEasyDbgDlg::ON_LOAD_DLL_DEBUG_EVENT(HANDLE hFile,LPVOID pBase)
{
    if (hFile==NULL || pBase==NULL)
    {
        return;
    }
    GetFileNameFromHandle(hFile,pBase);

    
}

//�жϽ���ָ���еĺ�������
BOOL CEasyDbgDlg::IsExportFun(char* szBuffer, EXPORT_FUN_INFO& expFun)
{
    if (szBuffer==NULL)
    {
        return FALSE;
    }
    //ָ���
    int nLength=0;
    nLength=strlen(szBuffer);
    char szCall[5]={0};
    char szJmp[4]={0};
    //���ǲ���CALL JMP֮��� //��CALL �Ĵ���Ҫ����
    //�� call [00400000] call dword ptr[00400000]  jmp [00400000]���н��� ע��һ��Ҫ��[]�������ڵ���������
    memcpy(szCall,szBuffer,4);
    memcpy(szJmp,szBuffer,3);
     

    //��ʱ������CALL reg�����
    if (szBuffer[5]=='e')
    {
        return FALSE;
    }



    if (strcmp(szCall,"call")==0||strcmp(szJmp,"jmp")==0)
    {
       //���ֱ����[]��ֱ�ӽ���
       if (nLength!=0 && szBuffer[nLength-1]==']')
       {
           //�ҵ�[]�ڵĵ�ֵַ ������������
           char Address[10]={0};
           for (int i=0;i<8;i++)
           {
               Address[i]=szBuffer[nLength-9+i];

           }
           DWORD dwAddress = 0 ;
         
          sscanf(Address, "%x", &dwAddress);
          //��ȡ��ֵַ��������
          DWORD dwActualAddress=0;
          //�޸ı������� 
          DWORD dwOldProtect=0;
          DWORD dwRet=0;
          VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,PAGE_READONLY,&dwOldProtect);
          if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwAddress,&dwActualAddress,sizeof(DWORD),NULL))
          {
              OutputDebugString("EasyDbgDlg.cpp 3669�г���");
              DWORD dwErrorCode=0;
              dwErrorCode=GetLastError();
            
              
              VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
              return FALSE;
          }
         
        
          VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
          //��ѯ��û�з��ϵĺ�����ַ
          if (m_ExFunList.Lookup(dwActualAddress,expFun))
          {
              return TRUE;
          }



       }


       //�������[]�Ϳ�����һ���ǲ���[]
       if (nLength!=0 && szBuffer[nLength-1]!=']')
       {
           //�������༶���� ֱ����������

           //�ҵ���ֵַ �������һ��ָ��
           char Address[10]={0};
           for (int j=0;j<8;j++)
           {
               Address[j]=szBuffer[nLength-8+j];
               
           }
           DWORD dwAddress = 0 ;
           
           sscanf(Address, "%x", &dwAddress);
           //��ȡ��ֵַ��������
           DWORD dwActualAddress=0;
           //�޸ı������� 
           DWORD dwOldProtect=0;
           DWORD dwRet=0;
           //����Ͳ��ж����ж�Read��һ����Ч��
           VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
           BYTE pCode[40]={0};
           if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwAddress,&pCode,sizeof(pCode),NULL))
           {
               OutputDebugString("EasyDbgDlg.cpp 3717�г���");
               DWORD dwErrorCode=0;
               dwErrorCode=GetLastError();

               //GetErrorMessage(dwErrorCode);
               return FALSE;
           }

           VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);

           
           for (int i=0;i<16;i++)
           {
               
               POSITION pos=NULL;
               pos = m_Int3BpList.GetHeadPosition();
               while(pos!=NULL)
               {
                   INT3_BP bp=m_Int3BpList.GetNext(pos);
                   //�ж϶ϵ��ַ�Ƿ���������λ�������
                   if (bp.dwAddress==dwAddress+i)
                   {
                       //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
                       pCode[i]=bp.bOriginalCode;
                   }
               }
           }

           char szAsm[120]={0};
           char szOpCode[120]={0};
           UINT CodeSize=0;
           Decode2AsmOpcode(pCode,szAsm,szOpCode,&CodeSize,dwAddress);
           //�жϱ���ָ��

           //ָ���
           int nLength=0;
           nLength=strlen(szAsm);
           char szCall[5]={0};
           char szJmp[4]={0};
           //���ǲ���CALL JMP֮���
           //�� call [00400000] call dword ptr[00400000]  jmp [00400000]���н��� ע��һ��Ҫ��[]�������ڵ���������
           memcpy(szCall,szBuffer,4);
           memcpy(szJmp,szBuffer,3);
           if (strcmp(szCall,"call")==0||strcmp(szJmp,"jmp")==0)
           {
               //���ֱ����[]��ֱ�ӽ���
               if (nLength!=0 && szAsm[nLength-1]==']')
               {
                   //�ҵ�[]�ڵĵ�ֵַ ������������
                   char Address[10]={0};
                   for (int i=0;i<8;i++)
                   {
                       Address[i]=szAsm[nLength-9+i];
                       
                   }
                   DWORD dwAddress = 0 ;
                   
                   sscanf(Address, "%x", &dwAddress);
                   //��ȡ��ֵַ��������
                   DWORD dwActualAddress=0;
                   //�޸ı������� 
                   DWORD dwOldProtect=0;
                   DWORD dwRet=0;
                   VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,PAGE_READONLY,&dwOldProtect);
                   if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)dwAddress,&dwActualAddress,sizeof(DWORD),NULL))
                   {
                       OutputDebugString("EasyDbgDlg.cpp 3783�г���");
                       DWORD dwErrorCode=0;
                       dwErrorCode=GetLastError();
                      // GetErrorMessage(dwErrorCode);
                       
                       VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
                       return FALSE;
                   }
                   
                  
                   VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwAddress,4,dwOldProtect,&dwRet);
                   //��ѯ��û�з��ϵĺ�����ַ
                   if (m_ExFunList.Lookup(dwActualAddress,expFun))
                   {
                       return TRUE;
                   }
                   
                   
                   
               }
           }
           
       }
    }
return FALSE;

}




//ɾ�����жϵ� ���ڼ�¼ָ��
void CEasyDbgDlg::DeleteAllBreakPoint()
{


    POSITION pos=NULL;

    //ɾ�����е��ڴ�ϵ�
    pos=m_MemBpList.GetHeadPosition();
    MEM_BP memBP={0};
    while (pos!=NULL)
    {
        memBP=m_MemBpList.GetNext(pos);
        DeleteMemBP(memBP.dwBpAddress);
        
        
    }
    //ɾ�����е�INT3�ϵ�
    INT3_BP bp={0};
    pos=NULL;
    pos=m_Int3BpList.GetHeadPosition();
    while (pos!=NULL)
    {
        bp=m_Int3BpList.GetNext(pos);
        //�ָ�Ϊԭ�����ֽ�
        RecoverBP(m_tpInfo.hProcess,bp.dwAddress,bp.bOriginalCode);
            
        
    }

    m_Int3BpList.RemoveAll();

    //ɾ�����е�Ӳ���ϵ�

    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(m_tpInfo.hThread,&ct);
    if (ct.Dr0!=0)
    {
        DeleteHardBP(ct.Dr0);
        
    }
    if (ct.Dr1!=0)
    {
        DeleteHardBP(ct.Dr1);
        
    }
    if (ct.Dr2!=0)
    {
        DeleteHardBP(ct.Dr2);
        
    }
    if (ct.Dr3!=0)
    {
        DeleteHardBP(ct.Dr3);
        
    }

}


//�Զ����� 
void CEasyDbgDlg::OnAutostepout() 
{
	// TODO: Add your command handler code here
    m_hFile=INVALID_HANDLE_VALUE;
   
    char            szFileName[MAX_PATH] = "Record";	
    OPENFILENAME    ofn={0};
    char            CodeBuf[24] = {0};
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.lpstrFile	 = szFileName;
    ofn.lpstrDefExt="txt";
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = "����Ҫ�����ָ���¼�ļ���(*.txt)\0*.txt\0";
    ofn.nFilterIndex = 1;
    if( GetSaveFileName(&ofn) == FALSE)
    {
        return ;
    }
     //�����ļ�
    m_hFile=CreateFile(szFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    //�Զ����� ɾ�����жϵ�
    DeleteAllBreakPoint();
    m_IsAutoF8=TRUE;
    ON_VK_F8();

	
}

//�Զ�����
void CEasyDbgDlg::OnAutostepinto() 
{
    DeleteAllBreakPoint();
    m_IsAutoF7=TRUE;
    ON_VK_F7();
	// TODO: Add your command handler code here
	
}

//�Ѽ�¼д���ļ�  ���� ָ���ַ ָ��� ����ʾ������ ��,
//��ʾ�����ı��ļ��в��ö��� 
void CEasyDbgDlg::WriteOpcodeToFile(DWORD dwAddress,char* szAsm)
{
    if (m_hFile==INVALID_HANDLE_VALUE||szAsm==NULL || dwAddress ==0)
    {
        return;
    }

    DWORD dwLength=0;
    dwLength=strlen(szAsm);
    //�س�����
    szAsm[dwLength]='\r';
    szAsm[dwLength+1]='\n';
    char szBuffer[16]={0};

    sprintf(szBuffer,"%08X",dwAddress);

    WriteFile(m_hFile,(LPVOID)szBuffer,sizeof(szBuffer),&dwLength,NULL);
  
    WriteFile(m_hFile,(LPVOID)szAsm,strlen(szAsm),&dwLength,NULL);


    

    

}


//��������  ��������MOV EBP ,ESPָ��֮�� POP EBP֮ǰ ���ö�ջԭ���ȡ���ص�ַ
void CEasyDbgDlg::StepOutFromFun()
{
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL;
    if (GetThreadContext(m_tpInfo.hThread,&ct)==0)
    {
        return;
    }
    
    DWORD dwBpAddress=0;
    DWORD dwOldProtect=0;
    DWORD dwRet=0;
    //��ȡ�����ķ��ص�ַ
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Ebp+4),4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
    if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)(ct.Ebp+4),&dwBpAddress,4,NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 3961�г���");
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Ebp+4),4,dwOldProtect,&dwRet);
        return;

    }
    VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Ebp+4),4,dwOldProtect,&dwRet);
    ON_G_COMMAND(dwBpAddress);
    



}


//����������
void CEasyDbgDlg::OnOutfun() 
{
	// TODO: Add your command handler code here
    StepOutFromFun();
	
}

//��ʾ��ջ
void CEasyDbgDlg::ShowStack()
{
    m_Stack.DeleteAllItems();
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_FULL;
    if (GetThreadContext(m_tpInfo.hThread,&ct)==0)
    {
        return;
    }
    CString szText;
    for (int i=0;i<20;i++)
    {
        szText.Format("%08X",(ct.Esp+i*4));
        m_Stack.InsertItem(i,szText);
        DWORD dwRet=0;
        DWORD dwOldProtect=0;
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Esp+i*4),4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
        if (!ReadProcessMemory(m_tpInfo.hProcess,(LPVOID)(ct.Esp+i*4),&dwRet,4,NULL))
        {
            VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Esp+i*4),4,dwOldProtect,&dwRet);
            if (i<20)
            {
                continue;
            }
            return;
        }
        
        szText.Format("%08X",dwRet);
        m_Stack.SetItemText(i,1,szText);
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)(ct.Esp+i*4),4,dwOldProtect,&dwRet);
        
    }

    

}

//DUMP
void CEasyDbgDlg::OnDump() 
{
	// TODO: Add your command handler code here
	DumpTargetProcess();
}


//DUMP�����Խ���
void CEasyDbgDlg::DumpTargetProcess()
{


    char            szFileName[MAX_PATH] = "Dump";	
    OPENFILENAME    ofn={0};
    char            CodeBuf[24] = {0};
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.lpstrFile	 = szFileName;
    
    ofn.lpstrDefExt="exe";
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = "����Ҫ�����ת���ļ���(*.exe)\0*.exe\0";
    ofn.nFilterIndex = 1;
    if( GetSaveFileName(&ofn) == FALSE)
    {
        return ;
    }

    //��Ҫ�����е��ڴ�ϵ��INT3�ϵ�Ӳ���ϵ�ȫ�����
    DeleteAllBreakPoint();


    HANDLE hSnap=NULL;
    HANDLE hFile=NULL;
    //���������Խ��̵�ģ�����
    hSnap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,m_tpInfo.dwProcessId);
    if (hSnap==INVALID_HANDLE_VALUE)
    {
        OutputDebugString("EasyDbgDlg.cpp 4059�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);

        return;
    }

    MODULEENTRY32 me32={0};
    //ö�ٵ��ĵ�һ��ģ����Ӧ�ó������ӳ�� ������ӳ���ַ�����С
    me32.dwSize=sizeof(MODULEENTRY32);
    if (!Module32First(hSnap,&me32))
    {
        OutputDebugString("EasyDbgDlg.cpp 4073�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);
        
        CloseHandle(hSnap);
        return;
    }

    HGLOBAL hBase=NULL;
    //�ڱ���������ͬ����С�Ŀռ� ������Ŀ����̵�����
    hBase=GlobalAlloc(0,me32.modBaseSize);
    if (hBase==NULL)
    {
        OutputDebugString("EasyDbgDlg.cpp 4086�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);
        CloseHandle(hSnap);
        return;
    }
  
    //��ȡĿ�����
    if ( !ReadProcessMemory(m_tpInfo.hProcess,me32.modBaseAddr,hBase,me32.modBaseSize,NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 4098�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);

        CloseHandle(hSnap);
        return;
    }
    //�����ļ�
    hFile=CreateFile(szFileName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        OutputDebugString("EasyDbgDlg.cpp 4108�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);
        
        CloseHandle(hSnap);
        return;

    }
    DWORD dwRet=0;
    //д�ļ�
    if (!WriteFile(hFile,hBase,me32.modBaseSize,&dwRet,NULL))
    {
        OutputDebugString("EasyDbgDlg.cpp 4123�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);
        
        CloseHandle(hSnap);
        return;

    }
    CloseHandle(hFile);
    //�ͷŶѿռ�
    GlobalFree(hBase);
   
    hFile=NULL;
    hFile=CreateFile(szFileName,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        OutputDebugString("EasyDbgDlg.cpp 4139�г���");
        DWORD dwErorCode=0;
        dwErorCode=GetLastError();
        GetErrorMessage(dwErorCode);
        CloseHandle(hSnap);
       
        return;
        
    }

    //�޸��������ļ�ƫ����RVA��ֵ��ֵ���ļ�ƫ�� 
     //�������û�ѡ���޸�OEPʹOEP���ڵ�ǰEIP

    PIMAGE_DOS_HEADER pDosHeader=new IMAGE_DOS_HEADER;
    PIMAGE_FILE_HEADER pFileHeader=new IMAGE_FILE_HEADER;

    if (!ReadFile(hFile,pDosHeader,sizeof(IMAGE_DOS_HEADER),&dwRet,NULL))
    {
        dwRet=GetLastError();
        AfxMessageBox("��ȡʧ��");
        CloseHandle(hSnap);
        CloseHandle(hFile);
        return;
    }
    //�ƶ��ļ�ָ�뵽IMAGE_FILE_HEADER��
    SetFilePointer(hFile,pDosHeader->e_lfanew+4,NULL,FILE_BEGIN);
    //��ȡIMAGE_FILE_HEADER
    if (!ReadFile(hFile,pFileHeader,sizeof(IMAGE_FILE_HEADER),&dwRet,NULL))
    {
        AfxMessageBox("��ȡʧ��");
        CloseHandle(hSnap);
     
        CloseHandle(hFile);
        return;
    }
    if (::MessageBox(NULL,"�Ƿ񽫵�ǰEIP��ΪOEP","��ʾ",MB_OKCANCEL)==IDOK)
    {
        SetFilePointer(hFile,pDosHeader->e_lfanew+4+sizeof(IMAGE_FILE_HEADER)+16,NULL,FILE_BEGIN);
 
        DWORD dwNewOEP=0;
        ReadFile(hFile,&dwNewOEP,4,&dwRet,NULL);
        CONTEXT ct={0};
        ct.ContextFlags=CONTEXT_FULL;
        GetThreadContext(m_tpInfo.hThread,&ct);
        //�µ�OEP��RVA
        dwNewOEP=ct.Eip-(DWORD)me32.modBaseAddr;
       
        SetFilePointer(hFile,pDosHeader->e_lfanew+4+sizeof(IMAGE_FILE_HEADER)+16,NULL,FILE_BEGIN);
        
        WriteFile(hFile,&dwNewOEP,sizeof(DWORD),&dwRet,NULL);
   
    }

    DWORD dwRva=0;
    DWORD dwVirtualOffset=0;
    //ѭ���ñ������� PointerToRawData,ʹ����RVA���
    for (int i=0;i<pFileHeader->NumberOfSections;i++)
    {
        dwRva=0;
        dwVirtualOffset=0;
        //�ƶ�ָ�뵽�������е�VirtualAddress 
        dwVirtualOffset=pDosHeader->e_lfanew+4+sizeof(IMAGE_FILE_HEADER)+pFileHeader->SizeOfOptionalHeader+12+i*sizeof(IMAGE_SECTION_HEADER);
        SetFilePointer(hFile,dwVirtualOffset,NULL,FILE_BEGIN);
        //��ȡVirtualAddress
        if (!ReadFile(hFile,&dwRva,sizeof(DWORD),&dwRet,NULL))
        {
            AfxMessageBox("��ȡʧ��");
            CloseHandle(hSnap);
         
            CloseHandle(hFile);
            return;
            
        }
        //�ƶ�ָ�뵽PointerToRawData
        dwVirtualOffset+=8;
        SetFilePointer(hFile,dwVirtualOffset,NULL,FILE_BEGIN);

        //�ı�PointerToRawData
        if (!WriteFile(hFile,&dwRva,sizeof(DWORD),&dwRet,NULL))
        {
            AfxMessageBox("д��ʧ��");
            CloseHandle(hSnap);
           
            CloseHandle(hFile);
            return;
       }
 
        dwVirtualOffset-=12;
        SetFilePointer(hFile,dwVirtualOffset,NULL,FILE_BEGIN);
        //�ƶ�ָ�벢��ȡVirtualSize
        if (!ReadFile(hFile,&dwRva,sizeof(DWORD),&dwRet,NULL))
        {
            AfxMessageBox("��ȡʧ��");
            CloseHandle(hSnap);
            
            CloseHandle(hFile);
            return;
            
        }

        dwVirtualOffset+=8;
        SetFilePointer(hFile,dwVirtualOffset,NULL,FILE_BEGIN);
        
        //�ı�SizeOfRawData ʹ�����VirtualSize
        if (!WriteFile(hFile,&dwRva,sizeof(DWORD),&dwRet,NULL))
        {
            AfxMessageBox("д��ʧ��");
            CloseHandle(hSnap);
            
            CloseHandle(hFile);
            return;
       }





    }
    delete pFileHeader;
    delete pDosHeader;

    CloseHandle(hSnap);
    CloseHandle(hFile);


}

//�ڷ���ര����ʾ������  ���� Ҫ������ָ���ַ
void CEasyDbgDlg::ShowAsmInWindow(DWORD dwStartAddress)
{
    //���ñ���
    SetDebuggerTitle(dwStartAddress);

    //�жϸõ�ַ�Ƿ��ڵ�ǰ��ʾ��ָ���ַ������
    DWORD dwRet=0;
    dwRet=IsFindAsmAddress(dwStartAddress);
    if (dwRet!=-1)
    {
        
        m_AsmList.SetItemState(dwRet, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
        return;

    }
    //���������������¶�
    m_AsmList.DeleteAllItems();
    CString szText;
    for (int k=0;k<22;k++)
    {
        BYTE pCode[40]={0};
        
        DWORD dwOldProtect=0;
        DWORD dwRet=0;
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,PAGE_EXECUTE_READWRITE,&dwOldProtect);
        if (!ReadProcessMemory(m_tpInfo.hProcess,(LPCVOID)dwStartAddress,pCode,sizeof(pCode),NULL))
        {
            OutputDebugString("EasyDbgDlg.cpp 4296�г���");
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���û����������Ϣ
            GetErrorMessage(dwErrorCode);
           VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);
            return;
        }
        VirtualProtectEx(m_tpInfo.hProcess,(LPVOID)dwStartAddress,4,dwOldProtect,&dwRet);
        
        for (int i=0;i<16;i++)
        {
            
            POSITION pos=NULL;
            pos = m_Int3BpList.GetHeadPosition();
            while(pos!=NULL)
            {
                INT3_BP bp=m_Int3BpList.GetNext(pos);
                //�ж϶ϵ��ַ�Ƿ���������λ�������
                if (bp.dwAddress==dwStartAddress+i)
                {
                    //������� ��˵����Ϊ�û��ϵ����ԭ�ֽڻ�ԭ
                    pCode[i]=bp.bOriginalCode;
                }
            }
            
            
        }
        
        char szAsm[120]={0};
        char szOpCode[120]={0};
        UINT CodeSize=0;
        Decode2AsmOpcode(pCode,szAsm,szOpCode,&CodeSize,dwStartAddress);
        EXPORT_FUN_INFO expFun={0};
        //����ҵ��ı���ʾ��ʽ
        if (IsExportFun(szAsm,expFun))
        {
            //��ʾ���б��ؼ���
            szText.Format("%08X",dwStartAddress);
            m_AsmList.InsertItem(k,szText);
            m_AsmList.SetItemText(k,1,szOpCode);
            szText.Format("%s <%s.%s>",szAsm,expFun.szDLLName,expFun.szFunName); 
            m_AsmList.SetItemText(k,2,szText);
             
            m_AsmAddress[k]=dwStartAddress;
            dwStartAddress=CodeSize+dwStartAddress;
            continue;
        }
        //��ʾ���б��ؼ���
        szText.Format("%08X",dwStartAddress);
        m_AsmList.InsertItem(k,szText);
        m_AsmList.SetItemText(k,1,szOpCode);
        m_AsmList.SetItemText(k,2,szAsm);
        
        m_AsmAddress[k]=dwStartAddress;
        dwStartAddress=CodeSize+dwStartAddress;

    }

     
     m_AsmList.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);







}


//����Ҫ��ʾ��ָ���ַ �жϵ�ǰָ���ַ�������Ƿ��� ���оͷ������±� ���򷵻�-1
DWORD CEasyDbgDlg::IsFindAsmAddress(DWORD dwStartAddress)
{
    for (DWORD i=0;i<25;i++)
    {
        if (dwStartAddress==m_AsmAddress[i])
        {
            return i;
        }
    }
    return -1;
}


//���˫������ �¶ϵ�
void CEasyDbgDlg::OnDblclkList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here

    POSITION pos=NULL;
    pos=m_AsmList.GetFirstSelectedItemPosition();
   
    if (pos==NULL)
    {
        return;
    }
  
    int nIndex=m_AsmList.GetNextSelectedItem(pos);
 
    UserSetBP(m_tpInfo.hProcess,m_AsmAddress[nIndex],m_tpInfo.bCC);



	
	*pResult = 0;
}





//�Ҽ������˵�
void CEasyDbgDlg::OnRclickList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here

    POINT pt;
    
    GetCursorPos(&pt);
    
    CMenu menu;
    
    menu.LoadMenu(CG_IDR_POPUP_EASY_DBG_DLG);
    
	menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN,pt.x,pt.y,this);
	
	*pResult = 0;
}

//��INT3�ϵ�
void CEasyDbgDlg::OnInt3() 
{
	// TODO: Add your command handler code here

    POSITION pos=NULL;
    pos=m_AsmList.GetFirstSelectedItemPosition();
   
    if (pos==NULL)
    {
        return;
    }
    
    int nIndex=m_AsmList.GetNextSelectedItem(pos);
    
    UserSetBP(m_tpInfo.hProcess,m_AsmAddress[nIndex],m_tpInfo.bCC);
	
}



//ִ�е���ǰ��ַ
void CEasyDbgDlg::OnExecute() 
{
	// TODO: Add your command handler code here
    POSITION pos=NULL;
    pos=m_AsmList.GetFirstSelectedItemPosition();
    
    if (pos==NULL)
    {
        return;
    }
    
    int nIndex=m_AsmList.GetNextSelectedItem(pos);
    
    ON_G_COMMAND(m_AsmAddress[nIndex]);

	
}

//��Ӳ��ִ�жϵ�
void CEasyDbgDlg::OnHard() 
{
	// TODO: Add your command handler code here

    POSITION pos=NULL;
    pos=m_AsmList.GetFirstSelectedItemPosition();
    
    if (pos==NULL)
    {
        return;
    }
    
    int nIndex=m_AsmList.GetNextSelectedItem(pos);
    
    SetHardBP(m_AsmAddress[nIndex],0,1);


	
}



//�ں�����ڴ���API�ϵ�
void CEasyDbgDlg::OnApi() 
{
	// TODO: Add your control notification handler code here
    char szBuffer[40];
    GetDlgItemText(IDC_API,szBuffer,sizeof(szBuffer));
    DWORD dwAddress=0;
    if (!m_Fun_Address.Lookup(szBuffer,dwAddress))
    {
        AfxMessageBox("�޴˺���");
        return ;
    }
    //��API��ڵ�ַ�¶ϵ�  �������CALL���¶ϵ� ��Ҫ���ȱ���ȫ����ָ��,����Ͳ�����
    UserSetBP(m_tpInfo.hProcess,dwAddress,m_tpInfo.bCC);
   

	
}






void CEasyDbgDlg::OnHelp() 
{
	// TODO: Add your command handler code here
    CHelp dlg;
    dlg.DoModal();
	
}

//��õ�ǰ����ģ����Ϣ
BOOL CEasyDbgDlg::GetCurrentModuleList(HANDLE hProcess)
{
    if (hProcess==NULL)
    {
        return FALSE;
    }
    //ɾ������Ԫ��
    m_Module.RemoveAll();

    
    HMODULE  hModule[500];
    //���շ��ص��ֽ���
    DWORD nRetrun=0;
    //ö��
    BOOL isSuccess=EnumProcessModules(hProcess,hModule,sizeof(hModule),&nRetrun);
    if (isSuccess==0)
    {
        OutputDebugString(TEXT("EasyDbgDlg.cpp 4538�г���"));
        DWORD dwErrorCode=0;
        dwErrorCode=GetLastError();
        //���������Ϣ
        GetErrorMessage(dwErrorCode);
        
        return FALSE;
        
    }
    
    TCHAR ModuleName[500];
    //ģ����Ϣ�ṹ��
    MODULEINFO minfo;
    //��ʼ���
    for (DWORD i=0;i<(nRetrun/sizeof(HMODULE));i++)
    {
        MODULE_INFO mem={0};
        //��ȡģ����
        DWORD nLength=GetModuleBaseName(hProcess,hModule[i],ModuleName,sizeof(ModuleName));
        if (nLength==0)
        {
            OutputDebugString(TEXT("EasyDbgDlg.cpp 4559�г���"));
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���������Ϣ
            GetErrorMessage(dwErrorCode);
            
            return FALSE;
        }
        
        strncpy(mem.szModuleName,ModuleName,strlen(ModuleName)+1);
        //��ʽ��ģ���ַ
        mem.dwBaseAddress=(DWORD)hModule[i];
        //��ȡģ����Ϣ
        nLength=GetModuleInformation(g_hProcess,hModule[i],&minfo,sizeof(minfo));
        if (nLength==0)
        {
            OutputDebugString(TEXT("EasyDbgDlg.cpp 4575�г���"));
            DWORD dwErrorCode=0;
            dwErrorCode=GetLastError();
            //���������Ϣ
            GetErrorMessage(dwErrorCode);
            
            return FALSE;

        }
       
        mem.dwSize=minfo.SizeOfImage;
       //��ӵ�����
        m_Module.AddTail(mem);
        
        
    }
    //�ѱ�־��ΪFALSE
    m_GetModule=FALSE;
    return TRUE;


}


//���õ���������(�ڵ���ʲô����,�Լ���ǰָ�����ĸ�ģ��)
//����Ϊ��ǰָ���ַ
void CEasyDbgDlg::SetDebuggerTitle(DWORD dwAddress)
{

    //���ģ����Ҫ����  ֻҪ��DLL���ؾ���Ҫ����
    if (m_GetModule)
    {

        if (!GetCurrentModuleList(m_tpInfo.hProcess))
        {
            return;

        }

    }

    //�жϵ�ǰ��ַ���ĸ�ģ��

    POSITION pos=NULL;
    pos=m_Module.GetHeadPosition();
    CString szText;
    while (pos!=NULL)
    {
         MODULE_INFO mem={0};
         mem=m_Module.GetNext(pos);
         if (dwAddress>=mem.dwBaseAddress && dwAddress<=(mem.dwSize+mem.dwBaseAddress))
         {
             
             MODULE_INFO mFirst={0};
             mFirst=m_Module.GetHead();
             //���ñ���
             szText.Format("EasyDbg -%s- [CPU - ���߳�,ģ�� - %s]",mFirst.szModuleName,mem.szModuleName);
             SetWindowText(szText);
             break;

         }


    }



}


//��DLL�����������з����
void CEasyDbgDlg::DisassemblerExcFun(char* szFunName)
{

    DWORD dwAddress=0;
    if (!m_Fun_Address.Lookup(szFunName,dwAddress))
    {
        AfxMessageBox("�޴˺���");
        return ;
    }
    
    m_Result.ResetContent();

    //���ģ����Ҫ����  ֻҪ��DLL���ؾ���Ҫ����
    if (m_GetModule)
    {
        
        if (!GetCurrentModuleList(m_tpInfo.hProcess))
        {
            return;
            
        }
        
    }
    
    //��õ�ǰ����ຯ�����ĸ�ģ��
    
    POSITION pos=NULL;
    pos=m_Module.GetHeadPosition();
    CString szText;
    while (pos!=NULL)
    {
        MODULE_INFO mem={0};
        mem=m_Module.GetNext(pos);
        if (dwAddress>=mem.dwBaseAddress && dwAddress<=(mem.dwSize+mem.dwBaseAddress))
        {
            
            //��ʾҪ�����ĺ������Լ�������ģ��
            szText.Format("%s!%s:",mem.szModuleName,szFunName);
            m_Result.AddString(szText);
            m_Result.SetTopIndex(m_Result.GetCount()-1);
            break;
            
        }
        
        
    }

    //�����

    ON_U_COMMAND(dwAddress);

}




