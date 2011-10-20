// DebugData.h: interface for the CDebugData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUGDATA_H__80BE350B_6F26_41DA_B22D_CA024071BE5A__INCLUDED_)
#define AFX_DEBUGDATA_H__80BE350B_6F26_41DA_B22D_CA024071BE5A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//����洢�뱻���Խ�����ص���Ϣ
typedef struct  TargetProcess_info
{
    //���汻���Խ��̵ľ��
    HANDLE hProcess;
    //���汻�����̵߳ľ��
    HANDLE hThread;
    //���汻���Խ��̵�ID
    DWORD dwProcessId;
    //���汻�����̵߳�ID
    DWORD dwThreadId;
    //��ڵ��ַ
    LPTHREAD_START_ROUTINE OepAddress;
    //������ڵ��׵�ַ����
    BYTE OriginalCode;
    //��������INT3�ϵ��CC
    BYTE bCC;

}TARGET_PROCESS_INFO;

//INT3�ϵ�ṹ��
typedef struct INT3BREAKPOINT
{
   //�ϵ��ַ
   DWORD dwAddress;
   //�ϵ����ֽ�����
   BYTE  bOriginalCode;
   //�Ƿ������öϵ� ���öϵ���Ҫ�ָ� һ���Զϵ��磺go address ��ʱΪһ���Զϵ� 
   //OEP���Ķϵ�Ҳ��һ���Զϵ�,����Ҫ�ڻָ�Ϊ�ϵ�
   BOOL  isForever;

}INT3_BP;

//������Ҫ���ָ�ΪINT3�ϵ�ĵ�ַ
typedef struct RECOVER_BREAKPOINT
{
    //��Ҫ���»ָ�Ϊ�ϵ�ĵ�ַ(���öϵ�)
    DWORD dwAddress;
    // �Ƿ���Ҫ���ָ�Ϊ�ϵ�
    BOOL  isNeedRecover;
    //ԭ�ֽ� //���ڻָ��ϵ�
    BYTE  bOrginalCode;
}RECOVER_BP;

//dr7���Կ��ƼĴ���
typedef union _Tag_DR7
{
    struct __DRFlag
    {
        unsigned int L0:  1 ;
        unsigned int G0:  1 ;
        unsigned int L1:  1 ;
        unsigned int G1:  1 ;
        unsigned int L2:  1 ;
        unsigned int G2:  1 ;
        unsigned int L3:  1 ;
        unsigned int G3:  1 ;
        unsigned int Le:  1 ;
        unsigned int Ge:  1 ;
        unsigned int b:   3 ;
        unsigned int gd:  1 ;
        unsigned int a:   2 ;
        unsigned int rw0: 2 ;
        unsigned int len0:2 ;
        unsigned int rw1: 2 ;
        unsigned int len1:2 ;
        unsigned int rw2: 2 ;
        unsigned int len2:2 ;
        unsigned int rw3: 2 ;
        unsigned int len3:2 ;
    } DRFlag;
    DWORD dwDr7 ;
}DR7 ;

//DR0-DR3��ʹ�����
typedef struct _DR_USE
{
    BOOL Dr0;
    BOOL Dr1;
    BOOL Dr2;
    BOOL Dr3;
 
} DR_USE;

//Ҫ�ָ���Ӳ���ϵ�ṹ��
typedef struct RECOVER_HARD_BREAKPOINT
{
    //Ҫ�ָ��ĵ��ԼĴ������ 0-3 //��Ϊ-1��ʾû��Ҫ�ָ��� 
    //������ȥ��һ����Ա,��
    DWORD dwIndex;

}RECOVER_HARDBP;

//�ڴ�ϵ�ṹ��

typedef struct MEMORYBREAKPOINT
{
    //��ַ
    DWORD dwBpAddress;
    //����
    DWORD dwLength;
    //���� �Ƿ��ʶϵ㻹��д��ϵ� 
    DWORD dwAttribute;
    //�ڴ�ҳ����ҳ���׵�ַ���� һ���ϵ�缸���ڴ�ҳ,���5����ҳ!�ڶ���Բ���
    DWORD dwMemPage[5];
    //��¼ռ�ķ�ҳ��
    DWORD dwNumPage;
   
    

}MEM_BP;

//�ڴ��ҳ�ṹ��(�����жϵ�ķ�ҳ)

typedef struct MEMORYPAGE
{
    //�ڴ�ҳ���׵�ַ
    DWORD dwBaseAddress;
    //ԭ��������
    DWORD dwProtect;


}MEM_BP_PAGE;

//Ҫ�ָ����ڴ�ҳ����

typedef struct _RECOVER_MEMPAGE
{
    //�ڴ��׵�ַ
    DWORD dwBaseAddress;
    //�ڴ�ҳ�ϵ�ı�������(����ԭ��������)
    DWORD dwProtect;
    //�Ƿ���Ҫ�ָ�
    BOOL  isNeedRecover;
}RECOVER_MEMPAGE;


//����������ַ��
typedef struct _EXPORT_FUN_INFO
{
    //������ַ
    DWORD dwAddress;
    //DLL����
    char  szDLLName[40];
    
    char  szFunName[280];


}EXPORT_FUN_INFO;


//ָ���¼�ṹ��
typedef struct _OPCODE_RECORD
{
    //ָ���ַ
    DWORD dwAddress;
    
}OPCODE_RECORD;

//ģ����Ϣ ������ʾ���⼴��ǰλ���Ǹ�ģ��
typedef struct _MODULE_INFO
{
    //ģ����
    char szModuleName[200];
    //ģ���ַ
    DWORD dwBaseAddress;
    //ģ���С
    DWORD dwSize;

}MODULE_INFO;













#endif // !defined(AFX_DEBUGDATA_H__80BE350B_6F26_41DA_B22D_CA024071BE5A__INCLUDED_)
