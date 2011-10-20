// EasyDbgDlg.h : header file
//

#if !defined(AFX_EASYDBGDLG_H__23A333A4_86FB_42E9_9EA7_CE145304AC6E__INCLUDED_)
#define AFX_EASYDBGDLG_H__23A333A4_86FB_42E9_9EA7_CE145304AC6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DebugData.h"
#include <afxtempl.h>
#include "VirtualMemory.h"
#include "ProcessModule.h"
#include "SetBP.h"
#include "ViewBP.h"
#include "PeScan.h"
#include "Help.h"



#pragma   warning(disable:4800)

/////////////////////////////////////////////////////////////////////////////
// CEasyDbgDlg dialog

class CEasyDbgDlg : public CDialog
{
// Construction
public:
	CEasyDbgDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEasyDbgDlg)
	enum { IDD = IDD_EASYDBG_DIALOG };
	CListCtrl	m_Stack;
	CListCtrl	m_AsmList;  
	CListCtrl	m_DataList;
	CListBox	m_Result;
	CEdit	m_command;
	CListCtrl	m_reglist;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEasyDbgDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL
    public:
        //�õ���ִ���ļ���·��
        void GetExeFilePath(char* szFilePath);
        void ShowReg(HANDLE hThread);
        //�����
        void ShowAsm(DWORD dwStartAddress);
        char m_SzFilePath[200];
    public:
        //�����Խ�����Ϣ�ṹ��
        TARGET_PROCESS_INFO m_tpInfo;
        //�������Ƿ����ڵ���
        BOOL m_isDebuging;
        //INT3�ϵ�����
        CList<INT3_BP,INT3_BP&> m_Int3BpList;
        //��Ҫ�����»ָ�ΪINT3�ϵ�Ľṹ��
        RECOVER_BP m_Recover_BP;
        //�Ƿ���OEP���ϵ�,����� ��ͣ����,���������ִ�й�ȥ,��OD,WINDBGһ�� ��ֵ��ΪTRUE
        BOOL m_IsOepBP;
        //�ж��Ƿ��ǵ�������ģʽ �������Ҫ�ڶϵ��쳣������Ĵ�����ָ��
        BOOL m_IsF8;
        //�ж��Ƿ�Ϊf9ģʽ
        BOOL m_IsGo;
        //��¼U����ĵ�ǰ��ַ
        DWORD m_Uaddress;
        //ɾ�����öϵ�
        BOOL m_isDelete;
        //���ԼĴ���Dr0-Dr3��ʹ�����
        DR_USE m_Dr_Use;
        //Ҫ�ָ���Ӳ���ϵ�
        RECOVER_HARDBP m_Recover_HBP;
        //�ڴ�ϵ�����
        CList<MEM_BP,MEM_BP&> m_MemBpList;
        //�ڴ�ҳ����
        CList<MEM_BP_PAGE,MEM_BP_PAGE&> m_MemPageList;
        //����Ҫ�ı����Ե����� ȡ 1 3��Ԫ��ֵ,����������
        DWORD m_Attribute[4];
        //Ҫ�ָ����ڴ�ҳ
        RECOVER_MEMPAGE m_Recover_Mpage;
        BOOL m_isMoreMem;
        //��������ӳ���  �ú�����ַ������ ������ѯ��
        CMap<DWORD,DWORD&,EXPORT_FUN_INFO,EXPORT_FUN_INFO&> m_ExFunList;
        //��ӳ����¼�Ѿ������˵�ָ���ַ �������Թ����ظ�ָ��(��ѭ����)
        CMap<DWORD,DWORD&,OPCODE_RECORD,OPCODE_RECORD&> m_Opcode;
         //���������ַ��Ӧ �����ں��������API�ϵ�
        CMap<CString,LPCTSTR,DWORD,DWORD&> m_Fun_Address;
        //�Ƿ����Զ�F8ģʽ �Զ���������
        BOOL m_IsAutoF8;
        //�Ƿ����Զ�F7ģʽ �Զ�����
        BOOL m_IsAutoF7;
        //����������ָ���¼���ļ����
        HANDLE m_hFile;
        //����ര����ʾ��ָ���ַ
        DWORD m_AsmAddress[22];
        //ģ����Ϣ�� ������ʾ������ ����ǰָ�����ĸ�ģ��
        CList<MODULE_INFO,MODULE_INFO&> m_Module;
        //��־ �ж��Ƿ�Ҫ����ö��ģ�� ����DLL����ʱ,��ΪTRUE ��ʱ��Ҫ����ģ����Ϣ��
        BOOL m_GetModule;


        
        
        

    public:
        //��ʼ������
        void UIinit();
        //��ȡ������Ϣ,����ʾ���û�
        void GetErrorMessage(DWORD dwErrorCode);
        //�õ�����DLLʱ��·��
        void GetFileNameFromHandle(HANDLE hFile,LPVOID pBase);
        //���� CREATE_PROCESS_DEBUG_INFO  ����ֵ���� ContinueDebugEvent�ĵ���������
        DWORD ON_CREATE_PROCESS_DEBUG_EVENT(DWORD dwProcessId,DWORD dwThreadId,LPTHREAD_START_ROUTINE lpOepAddress);
        //���� EXCEPTION_BREAKPOINT �쳣  ����Ϊ�쳣��ַ
        DWORD ON_EXCEPTION_BREAKPOINT(DWORD dwExpAddress);
        //����EXCEPTION_SINGLE_STEP�쳣 �����쳣��ַ
        DWORD ON_EXCEPTION_SINGLE_STEP(DWORD dwExpAddress);
        //����EXCEPTION_ACCESS_VIOLATION�쳣 ���� �쳣��ַ���쳣���ʵ�ַ
        DWORD ON_EXCEPTION_ACCESS_VIOLATION(DWORD dwExpAddress,DWORD dwAccessAddress);
        //�������DLL�¼�
        void ON_LOAD_DLL_DEBUG_EVENT(HANDLE hFile,LPVOID pBase);

        //��ʾ�����Խ����ڴ�����,Ĭ�ϴ�OEP��ʼ,��ʾ800�ֽ�,ÿ��8�ֽ�
        void ShowProcessMemory(DWORD dwStartAddress);
        //F7���Ĵ����� ��������
        void ON_VK_F7();
        //F8���Ĵ����� ��������
        void ON_VK_F8();
        //F9���Ĵ����� ����
        void ON_VK_F9();
        //ȥ���������ߺ��ұߵĿո��ַ�
        BOOL Kill_SPACE(char* szCommand);
        //�û�����Ĵ�����
        void Handle_User_Command(char* szCommand);
       //�Ǳ����Խ��̵�EIP��1,���ڶϵ��쳣����
        void ReduceEIP();
        //�ָ��ϵ�Ϊԭ���� ���������Խ��̾�� �ϵ��ַ ԭ�����ֽ�
        void RecoverBP(HANDLE hProcess,DWORD dwBpAddress,BYTE bOrignalCode);
        //���öϵ� ���� �����Խ��̾��  �ϵ��ַ 0xCC �������öϵ����»ָ�Ϊ�ϵ�
        void DebugSetBp(HANDLE hProcess,DWORD dwBpAddress,BYTE bCCode);
        //�ж��Ƿ����û����õ�INT3�ϵ� ͨ����ѯINT3���� ����Ϊ �ϵ��ַ
        BOOL isUserBP(DWORD dwBpAddress);
        //����U����
        void ON_U_COMMAND(DWORD dwAddress=0);
        //�û����öϵ�
        void UserSetBP(HANDLE hProcess,DWORD dwBpAddress,BYTE bCCode);
        //ɾ���û��ϵ�
        void DeleteUserBP(HANDLE hProcess,DWORD dwBpAddress);
        //����B����
        void Handle_B_Command(char* szCommand);
        //ö�ٶϵ�
        void ListBP();
        //G�����
        void ON_G_COMMAND(DWORD dwAddress=0);
        //����Ӳ���ϵ�
        void SetHardBP(DWORD dwBpAddress,DWORD dwAttribute,DWORD dwLength);
        //���ص�ǰ���õĵ��ԼĴ���
        int FindFreeDebugRegister();
        //ɾ��Ӳ���ϵ�
        void DeleteHardBP(DWORD dwAddress);
        //���Ҫ��ɾ����Ӳ���ϵ�ĵ��ԼĴ������ �����ϵ��ַ�� CONTEXT
        int GetDeletedDrIndex(DWORD dwAddress,CONTEXT ct);
        //�жϵ����쳣�Ƿ���Ӳ���ϵ������  �������÷��򴫽�ȥ��ֻ�Ǹı�Ĳ����ĸ���
        BOOL IfStepHard(DWORD& dwBPAddress);
        //ʹӲ���ϵ���ʱ��Ч
        void InvalidHardBP(DWORD dwBpAddress);
        //�ָ�Ӳ���ϵ� ����Ϊ ���ԼĴ����ı��
        void RecoverHBP(DWORD dwIndex);
        //�����ڴ�ϵ�  dwAttribute 1��ʾд��ϵ� 3��ʾ���ʶϵ�
        void SetMemBP(DWORD dwBpAddress,DWORD dwAttribute,DWORD dwLength);
        //�жϵ�ַ�Ƿ���Ч ע�����һ������������
        BOOL IsAddressValid(DWORD dwAddress,MEMORY_BASIC_INFORMATION& mbi);
        //�жϵ�ַ�ͳ���ռ�˼�����ҳ�������ڴ��ҳ�� ����mbi���Ѿ���úõ� Ҳ�Ѷϵ����ϵ��
        BOOL AddMemBpPage(DWORD dwBpAddress,DWORD dwLength,MEMORY_BASIC_INFORMATION mbi,DWORD dwAttribute,MEM_BP& mbp);
        //�ж�ĳһҳ�׵�ַ�Ƿ������ҳ������
        BOOL FindMemPage(DWORD dwBaseAddress);
        //�жϵ�ַ�Ƿ��ظ����ڴ�ϵ�
        BOOL FindMemBP(DWORD dwBpAddress);
        //��ʱ�ָ��ڴ�ϵ� ���� �ڴ�ҳ��ַ ԭ��������
        void RecoverMemBP(DWORD dwBaseAddress,DWORD dwProtect);
        //�ҵ�ԭʼ�ڴ�ҳ����Ķ�Ӧ���Բ����� ��������
        BOOL FindMemOriginalProtect(MEMORY_BASIC_INFORMATION& mbi);
        //ɾ���ڴ�ϵ�
        void DeleteMemBP(DWORD dwBpAddress);
        //�ҵ����ϵ��ڴ�ϵ���Ϣ������ ��������Ϊ����
        BOOL FindMemBPInformation(MEM_BP& mBP,DWORD dwBpAddress);
        //���ж���û����һ���ڴ�ϵ��������ҳ��,������ھ��޸�Ϊ��һ���ϵ���Ҫ�������
        //���� �ڴ�ҳ���׵�ַ
        BOOL ModifyPageProtect(DWORD dwBaseAddress);
         //ӳ���ļ�
        BOOL MapPEFile();

        // ��õ���������ַ �����ȫ�� ��Ϊ�漰����̬���ص�DLL
        BOOL GetExportFunAddress(HANDLE hFile,char* pDll,LPVOID pBase);
        //RVAת�ļ�ƫ��
        DWORD RvaToFileOffset(DWORD dwRva,DWORD dwSecNum,PIMAGE_SECTION_HEADER pSec);
        //�жϽ���ָ���еĺ������� ����ָ��� ����������Ϣ�ṹ���ڴ���
        BOOL IsExportFun(char* szBuffer, EXPORT_FUN_INFO& expFun);

        //ɾ�����жϵ� ���ڼ�¼ָ��

        void DeleteAllBreakPoint();

        //��ָ��д���ļ�  ����ָ���ַ ָ��� 
        void WriteOpcodeToFile(DWORD dwAddress , char* szAsm);
        //��������
        void StepOutFromFun();
        //��ʾ��ջ
        void ShowStack();
        //DUMP�����Խ���
        void DumpTargetProcess();
        //�ڷ���ര����ʾ������
        void ShowAsmInWindow(DWORD dwStartAddress);

        //����Ҫ��ʾ��ָ���ַ �жϵ�ǰָ���ַ�������Ƿ��� ���оͷ������±�
        DWORD IsFindAsmAddress(DWORD dwStartAddress);
        //��õ�ǰ����ģ����Ϣ
        BOOL GetCurrentModuleList(HANDLE hProcess);
        //���õ���������(�ڵ���ʲô����,�Լ���ǰָ�����ĸ�ģ��)
        //����Ϊ��ǰָ���ַ
        void SetDebuggerTitle(DWORD dwAddress);
        //��DLL�����������з����
        void DisassemblerExcFun(char* szFunName);


        



// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CEasyDbgDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    virtual void OnOK();
	afx_msg void OnOpen();
    afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnStepinto();   
	afx_msg void OnStepover();  
	afx_msg void OnShowdata();
	afx_msg void OnMemory();
	afx_msg void OnModule();
	afx_msg void OnRun();
	afx_msg void OnQuit();
	afx_msg void OnSetbp();
	afx_msg void OnBreakpoint();
	afx_msg void OnViewpe();
	afx_msg void OnAutostepout();
	afx_msg void OnAutostepinto();
	afx_msg void OnOutfun();
	afx_msg void OnDump();
	afx_msg void OnDblclkList3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickList3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnInt3();
	afx_msg void OnExecute();
	afx_msg void OnHard();
	afx_msg void OnApi();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EASYDBGDLG_H__23A333A4_86FB_42E9_9EA7_CE145304AC6E__INCLUDED_)
