// DoException.cpp: implementation of the CDoException class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DoException.h"
#include "Disasm/disasm.h"
#include <math.h>
#include <Tlhelp32.h>
#include "avl_tree/c_tree.h"

#pragma warning(disable : 4800)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

list<stuCmdNode*> g_CmdList;                    //��������
list<stuDllInfo*> g_DllList;                    //ģ����Ϣ�б�
list<stuPointInfo*> g_ptList;                   //�ϵ��б�
list<stuPageInfo*>  g_PageList;                 //��ҳ��Ϣ����
list<stuPointPage*> g_PointPageList;            //�ڴ�ϵ��ҳ���ձ�
list<stuCommand*> g_UserInputList;              //�����û�����ĺϷ����������
list<stuResetMemBp*> g_ResetMemBp;              //��Ҫ�ָ����ڴ�ϵ�

c_tree<stuCode> g_Avl_Tree;

char hexVale[256][3] =
{
'0','0',0,
'0','1',0,
'0','2',0,
'0','3',0,
'0','4',0,
'0','5',0,
'0','6',0,
'0','7',0,
'0','8',0,
'0','9',0,
'0','A',0,
'0','B',0,
'0','C',0,
'0','D',0,
'0','E',0,
'0','F',0,
'1','0',0,
'1','1',0,
'1','2',0,
'1','3',0,
'1','4',0,
'1','5',0,
'1','6',0,
'1','7',0,
'1','8',0,
'1','9',0,
'1','A',0,
'1','B',0,
'1','C',0,
'1','D',0,
'1','E',0,
'1','F',0,
'2','0',0,
'2','1',0,
'2','2',0,
'2','3',0,
'2','4',0,
'2','5',0,
'2','6',0,
'2','7',0,
'2','8',0,
'2','9',0,
'2','A',0,
'2','B',0,
'2','C',0,
'2','D',0,
'2','E',0,
'2','F',0,
'3','0',0,
'3','1',0,
'3','2',0,
'3','3',0,
'3','4',0,
'3','5',0,
'3','6',0,
'3','7',0,
'3','8',0,
'3','9',0,
'3','A',0,
'3','B',0,
'3','C',0,
'3','D',0,
'3','E',0,
'3','F',0,
'4','0',0,
'4','1',0,
'4','2',0,
'4','3',0,
'4','4',0,
'4','5',0,
'4','6',0,
'4','7',0,
'4','8',0,
'4','9',0,
'4','A',0,
'4','B',0,
'4','C',0,
'4','D',0,
'4','E',0,
'4','F',0,
'5','0',0,
'5','1',0,
'5','2',0,
'5','3',0,
'5','4',0,
'5','5',0,
'5','6',0,
'5','7',0,
'5','8',0,
'5','9',0,
'5','A',0,
'5','B',0,
'5','C',0,
'5','D',0,
'5','E',0,
'5','F',0,
'6','0',0,
'6','1',0,
'6','2',0,
'6','3',0,
'6','4',0,
'6','5',0,
'6','6',0,
'6','7',0,
'6','8',0,
'6','9',0,
'6','A',0,
'6','B',0,
'6','C',0,
'6','D',0,
'6','E',0,
'6','F',0,
'7','0',0,
'7','1',0,
'7','2',0,
'7','3',0,
'7','4',0,
'7','5',0,
'7','6',0,
'7','7',0,
'7','8',0,
'7','9',0,
'7','A',0,
'7','B',0,
'7','C',0,
'7','D',0,
'7','E',0,
'7','F',0,
'8','0',0,
'8','1',0,
'8','2',0,
'8','3',0,
'8','4',0,
'8','5',0,
'8','6',0,
'8','7',0,
'8','8',0,
'8','9',0,
'8','A',0,
'8','B',0,
'8','C',0,
'8','D',0,
'8','E',0,
'8','F',0,
'9','0',0,
'9','1',0,
'9','2',0,
'9','3',0,
'9','4',0,
'9','5',0,
'9','6',0,
'9','7',0,
'9','8',0,
'9','9',0,
'9','A',0,
'9','B',0,
'9','C',0,
'9','D',0,
'9','E',0,
'9','F',0,
'A','0',0,
'A','1',0,
'A','2',0,
'A','3',0,
'A','4',0,
'A','5',0,
'A','6',0,
'A','7',0,
'A','8',0,
'A','9',0,
'A','A',0,
'A','B',0,
'A','C',0,
'A','D',0,
'A','E',0,
'A','F',0,
'B','0',0,
'B','1',0,
'B','2',0,
'B','3',0,
'B','4',0,
'B','5',0,
'B','6',0,
'B','7',0,
'B','8',0,
'B','9',0,
'B','A',0,
'B','B',0,
'B','C',0,
'B','D',0,
'B','E',0,
'B','F',0,
'C','0',0,
'C','1',0,
'C','2',0,
'C','3',0,
'C','4',0,
'C','5',0,
'C','6',0,
'C','7',0,
'C','8',0,
'C','9',0,
'C','A',0,
'C','B',0,
'C','C',0,
'C','D',0,
'C','E',0,
'C','F',0,
'D','0',0,
'D','1',0,
'D','2',0,
'D','3',0,
'D','4',0,
'D','5',0,
'D','6',0,
'D','7',0,
'D','8',0,
'D','9',0,
'D','A',0,
'D','B',0,
'D','C',0,
'D','D',0,
'D','E',0,
'D','F',0,
'E','0',0,
'E','1',0,
'E','2',0,
'E','3',0,
'E','4',0,
'E','5',0,
'E','6',0,
'E','7',0,
'E','8',0,
'E','9',0,
'E','A',0,
'E','B',0,
'E','C',0,
'E','D',0,
'E','E',0,
'E','F',0,
'F','0',0,
'F','1',0,
'F','2',0,
'F','3',0,
'F','4',0,
'F','5',0,
'F','6',0,
'F','7',0,
'F','8',0,
'F','9',0,
'F','A',0,
'F','B',0,
'F','C',0,
'F','D',0,
'F','E',0,
'F','F',0
};

DEBUG_EVENT      CDoException::m_stuDbgEvent = {0};
HANDLE           CDoException::m_hProcess = NULL; 
DWORD            CDoException::m_ProcessId = 0; 
HANDLE           CDoException::m_hThread = NULL;
LPVOID           CDoException::m_lpOepAddr = NULL;
LPVOID           CDoException::m_lpDisAsmAddr = NULL;
LPVOID           CDoException::m_lpShowDataAddr = NULL;
LPVOID           CDoException::m_Eip = NULL;
CONTEXT          CDoException::m_Context = {0};
char             CDoException::m_chOEP = 0;
stuCommand       CDoException::m_UserCmd = {0};
int              CDoException::m_nPtNum = 0;
int              CDoException::m_nOrdPtFlag = 0;
int              CDoException::m_nHardPtNum = 0;              //����Ӳ���ϵ�����
BOOL             CDoException::m_isStart = TRUE;
BOOL             CDoException::m_isNeedResetPoint = FALSE;
BOOL             CDoException::m_isNeedResetHardPoint = FALSE;

int              CDoException::m_nNeedResetHardPoint = -1;
BOOL             CDoException::m_isUserInputStep = FALSE;
int              CDoException::m_nCount = 0;
HANDLE           CDoException::m_hAppend = NULL;              //������¼���浽���ļ����
BOOL             CDoException::m_isStepRecordMode = FALSE;    //�Ƿ񵥲���¼ģʽ
BOOL             CDoException::m_isShowCode = FALSE;          //������¼ģʽʱ�Ƿ���Ҫ����Ļ����ʾ����
EXCEPTION_DEBUG_INFO CDoException::m_DbgInfo = {0};
stuPointInfo*    CDoException::m_pFindPoint = NULL;           //�ҵ��Ķϵ�ָ��
//list<stuPointInfo*>::iterator CDoException::m_itFind = NULL;  //�ҵ��Ķϵ��������еĵ�����λ��
listStuPointInfo CDoException::m_itFind = NULL;

typedef HANDLE (__stdcall *OpenThreadFun)(
                                          DWORD dwDesiredAccess,  // access right
                                          BOOL bInheritHandle,    // handle inheritance option
                                          DWORD dwThreadId        // thread identifier
                                          );

//ȫ������-�������ձ�
stuCmdNode g_aryCmd[] = {
                        ADD_COMMAND("T",    CDoException::StepInto)
                        ADD_COMMAND("P",    CDoException::StepOver)
                        ADD_COMMAND("G",    CDoException::Run)
                        ADD_COMMAND("U",    CDoException::ShowMulAsmCode)
                        ADD_COMMAND("D",    CDoException::ShowData)
                        ADD_COMMAND("R",    CDoException::ShowRegValue)
                        ADD_COMMAND("BP",   CDoException::SetOrdPoint)
                        ADD_COMMAND("BPL",  CDoException::ListOrdPoint)
                        ADD_COMMAND("BPC",  CDoException::ClearOrdPoint)
                        ADD_COMMAND("BH",   CDoException::SetHardPoint)
                        ADD_COMMAND("BHL",  CDoException::ListHardPoint)
                        ADD_COMMAND("BHC",  CDoException::ClearHardPoint)
                        ADD_COMMAND("BM",   CDoException::SetMemPoint)
                        ADD_COMMAND("BML",  CDoException::ListMemPoint)
                        ADD_COMMAND("BMC",  CDoException::ClearMemPoint)
                        ADD_COMMAND("LS",   CDoException::LoadScript)
                        ADD_COMMAND("ES",   CDoException::ExportScript)
                        ADD_COMMAND("SR",   CDoException::StepRecord)
                        ADD_COMMAND("H",    CDoException::ShowHelp)
                         {"", NULL}     //���һ������
               };

CDoException::CDoException()
{

}

CDoException::~CDoException()
{

}

int stuCode::operator==(const stuCode & c)
{
    //�õ�ָ��ȵĽ�Сֵ
    int nMinCodeLen = min(m_nCodeLen, c.m_nCodeLen);
    if(c.m_nEip == m_nEip && 
        0 == memcmp(c.m_OpCode, m_OpCode, nMinCodeLen))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int stuCode::operator<(const stuCode & c)
{
    if(c.m_nEip < m_nEip)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int stuCode::operator>(const stuCode & c)
{
    if(c.m_nEip > m_nEip)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//��ȫ����
void __stdcall GetSafeStr(char *p, int n)
{
    for(int i = 0; i < n; i++)
    {
        *p = getchar();
        if(*p == 0x0a)
        {
            *p = 0;
            break;
        }
        p++;
    }
    _flushall();
}

//�����쳣����������ֵΪ TRUE, �������������쳣������ĵ���״̬Ϊ������
//����ֵΪ FALSE������״̬Ϊ��ʽ��û�д����쳣��
BOOL CDoException::DoException()
{
    m_DbgInfo = m_stuDbgEvent.u.Exception;

    //���߳�
    OpenThreadFun MyOpenThread;
    MyOpenThread = (OpenThreadFun)GetProcAddress(LoadLibrary("Kernel32.dll"), 
                                                 "OpenThread");
    CDoException::m_hThread = MyOpenThread(THREAD_ALL_ACCESS, FALSE,
                                           m_stuDbgEvent.dwThreadId);
    if (CDoException::m_hThread == NULL)
    {
        printf("OpenThread failed!");
        return 1;
    }

    //�����쳣�Ŀ��
    switch (m_DbgInfo.ExceptionRecord.ExceptionCode) 
    {
        //�����쳣
    case EXCEPTION_ACCESS_VIOLATION:
        return DoAccessException();
        break;
        //int3�쳣
    case EXCEPTION_BREAKPOINT: 
        return DoInt3Exception();
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT: 
        break;
        //�����Ĵ���
    case EXCEPTION_SINGLE_STEP:
        return DoStepException();
        break;
    case DBG_CONTROL_C: 
        break;
        // Handle other exceptions. 
    } 
    return TRUE;
}

//��������쳣����
BOOL CDoException::DoAccessException()
{
    BOOL                    bRet;
    DWORD                   dwAccessAddr;       //��д��ַ
    DWORD                   dwAccessFlag;       //��д��־
    BOOL                    isExceptionFromMemPoint = FALSE;    //�쳣�Ƿ����ڴ�ϵ���������Ĭ��Ϊ��
    stuPointInfo*           pPointInfo = NULL;                  //���еĶϵ�
    BOOL                    isHitMemPoint = FALSE;

    dwAccessFlag = m_DbgInfo.ExceptionRecord.ExceptionInformation[0];
    dwAccessAddr = m_DbgInfo.ExceptionRecord.ExceptionInformation[1];
    //���� ���ʵ�ַ �����ϵ�-��ҳ����ȥ����
    //ͬһ���ڴ��ҳ�����ж���ϵ�
    //���û���ڡ��ϵ�-��ҳ���в��ҵ�����˵������쳣���Ƕϵ������
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    int nSize = g_PointPageList.size();

    //����������ÿ���ڵ㣬��ÿ��ƥ��ġ��ϵ�-��ҳ��¼������ӵ�g_ResetMemBp������
    for ( int i = 0; i < nSize; i++ )
    {
        stuPointPage* pPointPage = *it;
        //����ڡ��ϵ�-��ҳ���в��ҵ�
        //�ٸ��ݶϵ������Ϣ�ж��Ƿ�����û����¶ϵ���Ϣ
        if (pPointPage->dwPageAddr == (dwAccessAddr & 0xfffff000))
        {
            stuResetMemBp *p = new stuResetMemBp;
            p->dwAddr = pPointPage->dwPageAddr;
            p->nID = pPointPage->nPtNum;                
            g_ResetMemBp.push_back(p);
            
            //��ʱ�ָ��ڴ�ҳԭ��������
            BOOL bDoOnce = FALSE;
            if (!bDoOnce)
            {
                //��Щ����ֻ��Ҫִ��һ��
                bDoOnce = TRUE;
                isExceptionFromMemPoint = TRUE;
                TempResumePageProp(pPointPage->dwPageAddr);
                //���õ������ڵ����н��ϵ����
                UpdateContextFromThread();
                m_Context.EFlags |= TF;
                UpdateContextToThread();
            }
            
            //���ҵ��ϵ���Ŷ�Ӧ�Ķϵ�
            list<stuPointInfo*>::iterator it2 = g_ptList.begin();
            for ( int j = 0; j < g_ptList.size(); j++ )
            {
                pPointInfo = *it2;
                if (pPointInfo->nPtNum == pPointPage->nPtNum)
                {
                    break;
                }
                it2++;
            }
            
            //���ж��Ƿ�����û����¶ϵ���Ϣ���ϵ����ͺͶϵ㷶Χ�����
            if (isHitMemPoint == FALSE)
            {
                if (dwAccessAddr >= (DWORD)pPointInfo->lpPointAddr && 
                    dwAccessAddr < (DWORD)pPointInfo->lpPointAddr +
                    pPointInfo->dwPointLen)
                {
                    if ( pPointInfo->ptAccess == ACCESS || 
                        (pPointInfo->ptAccess == WRITE && dwAccessFlag == 1) )
                    {
                        isHitMemPoint = TRUE;
                        //                             break;
                    }
                }
            }
        }
        it++;
    }
    
    //����쳣�������ڴ�ϵ����������������������
    if (isExceptionFromMemPoint == FALSE)
    {
        return FALSE;
    }
    
    //��������ڴ�ϵ㣬����ͣ����ʾ�����Ϣ���ȴ��û�����
    if (isHitMemPoint)
    {
        ShowBreakPointInfo(pPointInfo);
        //��ʾ��������
        m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
        ShowAsmCode();
        ShowRegValue(NULL);
        
        //�ȴ��û�����
        bRet = FALSE;
        while (bRet == FALSE)
        {
            bRet = WaitForUserInput();
        }
    }
    return TRUE;
}

//����INT3�쳣
BOOL CDoException::DoInt3Exception()
{
    BOOL                    bRet;
    stuPointInfo            tempPointInfo;
    stuPointInfo*           pResultPointInfo = NULL;
    char                    CodeBuf[24] = {0};

    //�ȹ���ϵͳ��INT3�ϵ�
    if (m_DbgInfo.ExceptionRecord.ExceptionAddress > (LPVOID)0x10000000
        && m_isStart == TRUE)
    {
        return TRUE;
    }
    
    //�����������ͣ��OEP��
    if(m_DbgInfo.ExceptionRecord.ExceptionAddress == m_lpOepAddr
        && m_isStart == TRUE)
    {
        m_isStart = FALSE;
        //ö��Ŀ����̵�ģ�飬��д�� g_DllList
        EnumDestMod();
    }
    
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    tempPointInfo.ptType = ORD_POINT;
    
    if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
    {
        //û���ҵ���Ӧ�ϵ㣬��˵�������û��µĶ϶�
        //����FALSE������������������ϵͳ���������쳣
        return FALSE;   
    }
    else    //�ҵ��˶ϵ�
    {
        ShowBreakPointInfo(pResultPointInfo);
        BOOL bRet = WriteProcessMemory(m_hProcess, 
            m_pFindPoint->lpPointAddr, 
            &(m_pFindPoint->u.chOldByte), 1, NULL);
        
        if (bRet == FALSE)
        {
            printf("WriteProcessMemory error!\r\n");
            return FALSE;
        }
        //��ȡ����
        UpdateContextFromThread();
        m_Context.Eip--;
        m_Eip = (LPVOID)m_Context.Eip;
        
        if (m_pFindPoint->isOnlyOne == TRUE)    //��һ���Զϵ�
        {
            delete m_pFindPoint;
            g_ptList.erase(m_itFind);
        } 
        else //����һ���Զϵ㣬��Ҫ���õ�������������ȥ����ϵ�
        {
            //���õ���
            m_Context.EFlags |= TF;
            m_isNeedResetPoint = TRUE;
        }
    }
    
    //�ָ�����
    UpdateContextToThread();
    
    //�Ƿ��ǵ�����¼ģʽ
    if (m_isStepRecordMode == TRUE)
    {
        bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error!");
            return FALSE;
        }
        //��¼ָ��
        RecordCode(m_Context.Eip, CodeBuf);
        return TRUE;
    }
    
    //��ʾ��������
    m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    ShowAsmCode();
    ShowRegValue(NULL);
    
    //�ȴ��û�����
    bRet = FALSE;
    while (bRet == FALSE)
    {
        bRet = WaitForUserInput();
    }
    return TRUE;
}

//�������쳣
BOOL CDoException::DoStepException()
{
    BOOL                    bRet;
    DWORD                   dwDr6 = 0;
    DWORD                   dwDr6Low = 0;
    stuPointInfo            tempPointInfo;
    stuPointInfo*           pResultPointInfo = NULL;
    char                    CodeBuf[24] = {0};
    DWORD                   dwOldProtect;
    DWORD                   dwNoUseProtect;

    UpdateContextFromThread();
        
    //��Ҫ����INT3�ϵ�
    if (m_isNeedResetPoint == TRUE)
    {
        m_isNeedResetPoint = FALSE;
        char chCC = (char)0xcc;
        VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = WriteProcessMemory(m_hProcess, m_pFindPoint->lpPointAddr, 
            &chCC, 1, NULL);
        VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
            1, dwOldProtect, &dwNoUseProtect);
        if (bRet == FALSE)
        {
            printf("WriteProcessMemory error!\r\n");
            return FALSE;
        }
    }

    //��Ҫ����Ӳ���ϵ�
    if (m_isNeedResetHardPoint == TRUE)
    {
        m_Context.Dr7 |= (int)pow(4.0, m_nNeedResetHardPoint);
        UpdateContextToThread();
        m_isNeedResetHardPoint = FALSE;
    }

    dwDr6 = m_Context.Dr6;
    dwDr6Low = dwDr6 & 0xf; //ȡ��4λ

    //�������Ӳ���ϵ㴥���ĵ�������Ҫ�û�������ܼ���
    //���⣬�����Ӳ��ִ�жϵ㣬����Ҫ����ʱȡ���ϵ㣬���õ������´��ٻָ��ϵ�
    if (dwDr6Low != 0)
    {
        ShowHardwareBreakpoint(dwDr6Low);
        m_nNeedResetHardPoint = log((long double)dwDr6Low)/log(2.0)+0.5;//��0.5��Ϊ����������
        //�ж��� dwDr6Low ָ����DRX�Ĵ������Ƿ���ִ�жϵ�
        if((m_Context.Dr7 << (14 - (m_nNeedResetHardPoint * 2))) >> 30 == 0)
        {
            switch (m_nNeedResetHardPoint)
            {
            case 0:
                m_Context.Dr7 &= 0xfffffffe;
                break;
            case 1:
                m_Context.Dr7 &= 0xfffffffb;
                break;
            case 2:
                m_Context.Dr7 &= 0xffffffef;
                break;
            case 3:
                m_Context.Dr7 &= 0xffffffbf;
                break;
            default:
                printf("Error!\r\n");
            }
            m_Context.EFlags |= TF;
            UpdateContextToThread();
            m_isNeedResetHardPoint = TRUE;
        }

        m_isUserInputStep = TRUE; //�������ֻ��Ϊ���ܹ��ȴ��û�����
    }

    if (m_isUserInputStep == FALSE)
    {
        //�����ڴ�ϵ�
        ResetMemBp();
        return TRUE;
    }

    //���´������û�����Ϊ "T" �����Ӳ���ϵ㴥��ʱִ��
    //����˴���INT3�ϵ㣬����Ҫ����ʱɾ��INT3�ϵ�
    //��������Ϊ�����û����롰T�������Ӳ���ϵ㴥��ʱ���Ե�INT3�ϵ�
    //������һ���ط�ͣ������
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    tempPointInfo.ptType = ORD_POINT;

    if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
    {
        //��һ���Զϵ㣬����Ҫ����ϵ�
        if (pResultPointInfo->isOnlyOne == FALSE)
        {
            m_Context.EFlags |= TF;
            UpdateContextToThread();
            m_isNeedResetPoint = TRUE;
        }
        else//һ���Զϵ㣬����������ɾ��
        {
            delete[] m_pFindPoint;
            g_ptList.erase(m_itFind);
        }
        VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = WriteProcessMemory(m_hProcess, m_pFindPoint->lpPointAddr, 
            &(m_pFindPoint->u.chOldByte), 1, NULL);
        if (bRet == FALSE)
        {
            printf("WriteProcessMemory error!\r\n");
            return FALSE;
        }
        VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
            1, dwOldProtect, &dwNoUseProtect);
    }

    m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    m_isUserInputStep = FALSE;

    //����m_ContextΪ���ڵĻ���ֵ
    UpdateContextFromThread();

    if (m_isStepRecordMode == FALSE)
    {
        ShowAsmCode();
        ShowRegValue(NULL);
    }
    
    //�����ڴ�ϵ�
    ResetMemBp();

   //�Ƿ��ǵ�����¼ģʽ
   if (m_isStepRecordMode == TRUE)
    {
        bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error!\r\n");
            return FALSE;
        }
        //��¼ָ��
        RecordCode(m_Context.Eip, CodeBuf);
        return TRUE;
    }

    bRet = FALSE;
    while (bRet == FALSE)
    {
        bRet = WaitForUserInput();
    }
    return TRUE;
}


//����g_ResetMemBp�����ڴ�ϵ㣬�ڵ����е���
void CDoException::ResetMemBp()
{
    list<stuResetMemBp*>::iterator itDw = g_ResetMemBp.begin();
    while (g_ResetMemBp.size())
    {
        stuResetMemBp* p = *itDw;
        itDw++;
        DWORD dwTempProtect;
        VirtualProtectEx(m_hProcess, (LPVOID)p->dwAddr,
                         1, PAGE_NOACCESS, &dwTempProtect);
        delete p;
        g_ResetMemBp.remove(p);
    }
}

// �ȴ��û���������
// ������Ӧ�Ĵ���
// �û��������������������
// 1. ���Ƴ������̵�����絥������(T)������(P)������(G)��
// 2. �ϵ������������(bp)���鿴(bl)��ȡ��(bc)���ֶϵ㣻
//    �鿴��������(U)������(D)���Ĵ���(R)�ȡ�
// ���û��������T��P��Gʱ����������ֵΪTRUE����ʾ������Ҫ�������У����������У���
// Ϊ��������ʱ����������ֵΪFALSE����ʾ�����Ĳ�������������
BOOL CDoException::WaitForUserInput()
{
    printf("COMMAND:");
    char chUserInputString[41] = {0};
    GetSafeStr(chUserInputString, 40);

    memset(&m_UserCmd, 0, sizeof(stuCommand));

    //�û�������ַ���ת��Ϊ����ṹ��
    BOOL bRet = ChangeStrToCmd(chUserInputString, &m_UserCmd);

    if ( bRet == FALSE)
    {
        return FALSE;
    }

    //����������������������������Ӧ������
    CmdProcessFun pFun = GetFunFromAryCmd(m_UserCmd);
    if (pFun)
    {
        stuCommand *pCmd = new stuCommand;
        memcpy(pCmd, &m_UserCmd, sizeof(stuCommand));
        g_UserInputList.push_back(pCmd);
        return (*pFun)(pCmd);
    }
    else
    {
        printf("Error input!\r\n");
        return FALSE;
    }
}

BOOL CDoException::ChangeStrToCmd(
                      IN char* chUserInputString, 
                      OUT stuCommand* pUserCmd)
{
    char * chStr = DelFrontSpace(chUserInputString);
    int nLen = strlen(chStr);

    int i = 0;
    int j = 0;  // ������� stuCommand �ṹ���еڼ����ֶ�
//     int k = FIELD_LEN;
    while (chStr[0] != '\0' && chStr[0] != '\r')
    {
        for (i = 0; i < nLen; i++)
        {
            if (chStr[i] == ' ' || chStr[i] == '\0' || chStr[i] == '\r')
            {
                break;
            }
            
            if (i >= FIELD_LEN)
            {
                printf("Error command! One of the param's length greater than 20!\r\n");
                return FALSE;
            }
        }
        
        memcpy((char*)((int)pUserCmd + FIELD_LEN*j), chStr, i);
        j++;

        chStr = DelFrontSpace((char*)((int)chStr + i));
        nLen = strlen(chStr);
    }
    return TRUE;
}


char* CDoException::DelFrontSpace(char * pStr)
{
    int nLen = strlen(pStr);
    for (int i = 0; i < nLen; i++)
    {
        if (pStr[i] != ' ')
        {
            return &pStr[i];
        }
    }
    return &pStr[nLen];
}

// �� lpDisAsmAddr ��ָ����λ�ÿ�ʼ���з����
void CDoException::ShowAsmCode()
{
    char            CodeBuf[20];
    t_disasm        da;
    int             nCodelen;
    BOOL            bRet;
    BOOL            isNeedResetFirstPage = FALSE;
    BOOL            isNeedResetSecondPage = FALSE;
    DWORD           dwTempProtect1;
    DWORD           dwTempProtect2;
    DWORD           dwOldProtect;

    //�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
    //����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊ���ɷ���
    //ע�⣬�����ڴ���ܿ��ҳ
    if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
            1, PAGE_READONLY, &dwTempProtect1);
        isNeedResetFirstPage = TRUE;
    }

    if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
            1, PAGE_READONLY, &dwTempProtect2);
        isNeedResetSecondPage = TRUE;
    }

    bRet = ReadProcessMemory(m_hProcess, m_lpDisAsmAddr, CodeBuf, 20, NULL);

    //����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
    //������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
    //�����Ȼָ� SecondPage����ָ� FirstPage
    if (isNeedResetSecondPage)
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
            1, dwTempProtect2, &dwOldProtect);
        isNeedResetSecondPage = FALSE;
    }
    
    if (isNeedResetFirstPage)
    {
        VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
            1, dwTempProtect1, &dwOldProtect);
        isNeedResetFirstPage = FALSE;
    }
    
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return;
    }

    //����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
    if (CodeBuf[0] == 0xCC)
    {
        stuPointInfo tempPointInfo;
        stuPointInfo* pResultPointInfo = NULL;
        memset(&tempPointInfo, 0, sizeof(stuPointInfo));
        tempPointInfo.lpPointAddr = m_lpDisAsmAddr;
        tempPointInfo.ptType = ORD_POINT;

        if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
        {
            CodeBuf[0] = pResultPointInfo->u.chOldByte;
        }
    }
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//���÷��������

    //����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
    printf("%p    %s %s %s", m_lpDisAsmAddr, da.dump, 
           "                        " + strlen(da.dump), da.result);

    char chCall[5] = {0};
    char chJmp[2] = {0};
    memcpy(chCall, da.result, 4);
    memcpy(chJmp, da.result, 1);

    if (stricmp(chCall, "CALL") == 0 || 
        stricmp(chJmp, "J") == 0 || 
        da.result[strlen(da.result)-1] == ']')
    {
        ShowFunctionName(da.result);
    }
    printf("\r\n");

    // lpDisAsmAddr ��ַҪ����ƶ�
    m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);
    
    bRet = FALSE;
}

//��ʾ���з������뺯��
BOOL CDoException::ShowMulAsmCode(stuCommand* pCmd)
{
    LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
    if (lpAddr != 0)
    {
        m_lpDisAsmAddr = lpAddr;
    }

    for (int i = 0; i < 8; i++)
    {
        ShowAsmCode();
    }
    return FALSE;
}

//��ʾ���ݺ���
BOOL CDoException::ShowData(stuCommand* pCmd)
{
    char            CodeBuf[0x80] = {0};
    BOOL            bRet;
    DWORD           dwReadBytes = 0;
    BOOL            isNeedResetFirstPage = FALSE;
    BOOL            isNeedResetSecondPage = FALSE;
    DWORD           dwTempProtect1;
    DWORD           dwTempProtect2;
    DWORD           dwOldProtect;

    //������ʾ���ݺ���
    LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
    if (m_lpShowDataAddr == 0)
    {
        m_lpShowDataAddr = m_Eip;
    }
    if (lpAddr != 0)
    {
        m_lpShowDataAddr = lpAddr;
    }

    printf("-----------------------------------------------------------------------------\r\n");
    
    //�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
    //����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊ���ɷ���
    //ע�⣬�����ڴ���ܿ��ҳ
    if (FindRecordInPointPageList((DWORD)m_lpShowDataAddr & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, m_lpShowDataAddr,
            1, PAGE_READONLY, &dwTempProtect1);
        isNeedResetFirstPage = TRUE;
    }
    
    if (FindRecordInPointPageList(((DWORD)m_lpShowDataAddr + 0x80) & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpShowDataAddr+0x80),
            1, PAGE_READONLY, &dwTempProtect2);
        isNeedResetSecondPage = TRUE;
    }
    
    bRet = ReadProcessMemory(m_hProcess, m_lpShowDataAddr, CodeBuf, 0x80, &dwReadBytes);
    
    //����֮������ϵ㣬����Ҫע�⣬���� m_lpShowDataAddr �� m_lpShowDataAddr+80
    //������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
    //�����Ȼָ� SecondPage����ָ� FirstPage
    if (isNeedResetSecondPage)
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpShowDataAddr+0x80),
            1, dwTempProtect2, &dwOldProtect);
        isNeedResetSecondPage = FALSE;
    }
    
    if (isNeedResetFirstPage)
    {
        VirtualProtectEx(m_hProcess, m_lpShowDataAddr,
            1, dwTempProtect1, &dwOldProtect);
        isNeedResetFirstPage = FALSE;
    }

    

    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return FALSE;
    }

    for(int i = 0; i < 8; i++)
    {
        printf("%p:  ", (int)m_lpShowDataAddr + 0x10 * i);
        for (int j = 0; j < 16; j++)
        {
            //��INT3��CC��ԭ��ԭ�ַ�
            if (CodeBuf[i*8 + j] == 0xcc)
            {
                stuPointInfo tempPointInfo;
                stuPointInfo* pResultPointInfo = NULL;
                memset(&tempPointInfo, 0, sizeof(stuPointInfo));
                tempPointInfo.lpPointAddr = (LPVOID)((DWORD)m_lpShowDataAddr + i*8 + j);
                tempPointInfo.ptType = ORD_POINT;
                
                if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
                {
                    CodeBuf[i*8 + j] = pResultPointInfo->u.chOldByte;
                }
            }

            printf("%0.2X ", CodeBuf[i*8 + j]);
            if (i*8 + j >= dwReadBytes)
            {
                break;
            }
        }
		int j = 0;
        for (j; j < 16; j++)
        {
            if (CodeBuf[i*8 + j] < ' ')
            {
                printf(".");
            } 
            else
            {
                printf("%C", CodeBuf[i*8 + j]);
            }
            if (i*8 + j >= dwReadBytes)
            {
                break;
            }
        }
        printf("\r\n");
        if (i*8 + j >= dwReadBytes)
        {
            break;
        }       
    }
    printf("-----------------------------------------------------------------------------\r\n");
    m_lpShowDataAddr = (LPVOID)((int)m_lpShowDataAddr + 0x80);
    return FALSE;
}

//��ʾ�Ĵ�������
BOOL CDoException::ShowRegValue(stuCommand* pCmd)
{
    printf("-----------------------------------------------------------------------------\r\n");
    printf("EAX=%p EBX=%p ECX=%p EDX=%p ESI=%p EDI=%p\r\n", 
           m_Context.Eax, m_Context.Ebx, m_Context.Ecx, m_Context.Edx,
           m_Context.Esi, m_Context.Edi);
    printf("EIP=%p ESP=%p EBP=%p                OF DF IF SF ZF AF PF CF\r\n", 
           m_Context.Eip, m_Context.Esp, m_Context.Ebp);
    printf("CS=%0.4X SS=%0.4X DS=%0.4X ES=%0.4X FS=%0.4X GS=%0.4X",
            m_Context.SegCs, m_Context.SegSs, m_Context.SegDs, m_Context.SegEs,
            m_Context.SegFs, m_Context.SegGs);
    printf("       %d  %d  %d  %d  %d  %d  %d  %d\r\n", 
            (bool)(m_Context.EFlags & 0x0800),
            (bool)(m_Context.EFlags & 0x0400),
            (bool)(m_Context.EFlags & 0x0200),
            (bool)(m_Context.EFlags & 0x0080),
            (bool)(m_Context.EFlags & 0x0040),
            (bool)(m_Context.EFlags & 0x0010),
            (bool)(m_Context.EFlags & 0x0004),
            (bool)(m_Context.EFlags & 0x0001)
        );
    printf("-----------------------------------------------------------------------------\r\n");
    return FALSE;
}

//����һ��ϵ�
BOOL CDoException::SetOrdPoint(stuCommand* pCmd)
{
    BOOL    bRet;
    LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

    if (lpAddr == 0)
    {
        printf("Need valid parameter!\r\n");
        return FALSE;
    }

    //�ڶϵ��б��в����Ƿ��Ѿ����ڴ˴���һ��ϵ�
    stuPointInfo tempPointInfo;
    stuPointInfo* pResultPointInfo = NULL;
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = lpAddr;
    tempPointInfo.ptType = ORD_POINT;
    if (stricmp(pCmd->chParam2, "once") == 0)
    {
        tempPointInfo.isOnlyOne = TRUE;
    } 
    else
    {
        tempPointInfo.isOnlyOne = FALSE;
    }

    if (FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
    {
        if (tempPointInfo.isOnlyOne == FALSE)//���õ��Ƿ�һ���Զϵ�
        {
            if (pResultPointInfo->isOnlyOne == FALSE)//���ҵ����Ƿ�һ���Զϵ�
            {
                printf("This Ordinary BreakPoint is already exist!\r\n");
            } 
            else
            {
                pResultPointInfo->isOnlyOne = FALSE;
            }
        }
        return FALSE;
    }    

    char chOld;
    char chCC = 0xcc;
    DWORD dwOldProtect;
    VirtualProtectEx(m_hProcess, lpAddr, 1, PAGE_READWRITE, &dwOldProtect);
    bRet = ReadProcessMemory(m_hProcess, lpAddr, &chOld, 1, NULL);
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error! may be is not a valid memory address!\r\n");
        return FALSE;
    }

    bRet = WriteProcessMemory(m_hProcess, lpAddr, &chCC, 1, NULL);
    if (bRet == FALSE)
    {
        printf("WriteProcessMemory error!\r\n");
        return FALSE;
    }

    VirtualProtectEx(m_hProcess, lpAddr, 1, dwOldProtect, &dwOldProtect);

    stuPointInfo* NewPointInfo = new stuPointInfo;
    memset(NewPointInfo, 0, sizeof(stuPointInfo));
    NewPointInfo->nPtNum = m_nOrdPtFlag;
    m_nOrdPtFlag++;
    NewPointInfo->ptType = ORD_POINT;
    NewPointInfo->lpPointAddr = lpAddr;
    NewPointInfo->u.chOldByte = chOld;
    NewPointInfo->isOnlyOne = tempPointInfo.isOnlyOne;
    g_ptList.push_back(NewPointInfo);
    if (m_isStepRecordMode == FALSE && m_nOrdPtFlag != 1)
    {
        printf("***Set Ordinary breakpoint(INT3) success!***\r\n");
    }

    return FALSE;
}

//һ��ϵ��б�
BOOL CDoException::ListOrdPoint(stuCommand* pCmd)
{
    printf("------------------------------------\r\n");
    printf("ID    Breakpoint type    Address\r\n");
    list<stuPointInfo*>::iterator it = g_ptList.begin();
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->ptType == ORD_POINT)
        {
            printf("%03d   INT3 breakpoint    0x%p\r\n", 
                pPointInfo->nPtNum, pPointInfo->lpPointAddr);
        }
        it++;
    }
    printf("------------------------------------\r\n");
    return FALSE;
}

//һ��ϵ����
BOOL CDoException::ClearOrdPoint(stuCommand* pCmd)
{
    BOOL bRet;

    int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);
    
    if (nID == 0)
    {
        printf("Need valid ID!\r\n");
        return FALSE;
    }

    list<stuPointInfo*>::iterator it = g_ptList.begin();
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->nPtNum == nID)
        {
            DWORD dwOldProtect;
            DWORD dwNoUseProtect;
            VirtualProtectEx(m_hProcess, pPointInfo->lpPointAddr,
                1, PAGE_READWRITE, &dwOldProtect);
            bRet = WriteProcessMemory(m_hProcess, pPointInfo->lpPointAddr, 
                   &(pPointInfo->u.chOldByte), 1, NULL);
            if (bRet == FALSE)
            {
                printf("WriteProcessMemory error!\r\n");
                return FALSE;
            }           
            VirtualProtectEx(m_hProcess, pPointInfo->lpPointAddr,
                1, dwOldProtect, &dwNoUseProtect);

            //���ɾ��������Ҫ�ָ���INT3�ϵ㣬�򽫻ָ���ʾ��Ϊ FALSE
            if (m_isNeedResetPoint == TRUE && m_pFindPoint->nPtNum == nID)
            {
                m_isNeedResetPoint = FALSE;
            }

            delete [] pPointInfo;
            g_ptList.erase(it);
            //ɾ������ʾ
            printf("Clear the %d Ordinary breakpoint.\r\n", nID);
            return FALSE;
        }
        it++;
    }
    //û���ҵ�����ʾ
    printf("Can not find the Ordinary breakpoint!\r\n");
    return FALSE;
}

//����Ӳ���ϵ�
BOOL CDoException::SetHardPoint(stuCommand* pCmd)
{
    if(!UpdateContextFromThread())
    {
        return FALSE;
    }

    if (m_nHardPtNum == 4)
    {
        printf("Warning:Current Hardware breakpoint is full!\r\n");
        printf("        You can delete some Hardware breakpoint!\r\n");
        return FALSE; 
    }
    LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

    if (lpAddr == 0)
    {
        printf("Need valid parameter!\r\n");
        return FALSE;
    }

    stuPointInfo PointInfo;
    memset(&PointInfo, 0, sizeof(stuPointInfo));
    PointInfo.lpPointAddr = lpAddr;
    PointInfo.ptType = HARD_POINT;
    
    if (stricmp("access", pCmd->chParam2) == 0)
    {
        PointInfo.ptAccess = ACCESS;
    } 
    else if (stricmp("write", pCmd->chParam2) == 0)
    {
        PointInfo.ptAccess = WRITE;
    }
    else if (stricmp("execute", pCmd->chParam2) == 0)
    {
        PointInfo.ptAccess = EXECUTE;
    }
    else
    {
        printf("Void access!\r\n");
        return FALSE;
    }
    
    int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);
    if (nLen != 0 && PointInfo.ptAccess == EXECUTE)
    {
        printf("Point length error!\r\n");
        return FALSE;
    }
    if (nLen != 0 && nLen != 1 && nLen != 2 && nLen != 4)
    {
        printf("Point length error!\r\n");
        return FALSE;
    }

    PointInfo.dwPointLen = nLen;
    int nDrNum = -1;                 // Ӳ���ϵ������±�ΪnDrNum�� DRX �Ĵ�����
    int nPointLen = -1;

    //���Ӳ���ϵ�����>0
    if (m_nHardPtNum > 0)
    {
        if(FindPointInConext(PointInfo, &nDrNum, &nPointLen))
        {
            //�ҵ��ˣ���Ҫ�Ƚ�һ���ҵ��Ķϵ���ֽڳ����Ƿ��Ҫ���õĶϵ��ֽڳ��ȳ�
            if (nPointLen >= nLen)
            {
                //����ҵ��Ķϵ��ֽڳ��ȴ�����Ҫ���õ��¶ϵ㣬
                //����Ҫ�������ã�ֱ�ӷ���
                printf("The Hardware breakpoint is exist!\r\n");
                return FALSE;
            }
            else //����Ӳ���ϵ���Ҫ�������ã���Ӳ���ϵ�����ά�ֲ��䣬���������Ƚ�������1
            {
                m_nHardPtNum--;
            }
        }
    }

    //û���ҵ� �� ��Ҫ��������
    if (nDrNum == -1) //û���ҵ�������Ҫ����һ�����еĵ��ԼĴ���
    {
        if((m_Context.Dr7 & 1) == 0)
        {
            nDrNum = 0;
        }
        else if ((m_Context.Dr7 & 4) == 0)
        {
            nDrNum = 1;
        }
        else if ((m_Context.Dr7 & 0x10) == 0)
        {
            nDrNum = 2;
        }
        else
        {
            nDrNum = 3;
        }
    }

    //���� nDrNum ����Ӳ���ϵ�
    switch (nDrNum)
    {
    case 0:
        m_Context.Dr0 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 1;
        m_Context.Dr7 &= 0xfff0ffff;//�����16��17��18��19λ����0��
        //LEN ����
        switch (PointInfo.dwPointLen)
        {
        case 0:
        	break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x00040000;//18λ ��1
            break;
        case 4:
            m_Context.Dr7 |= 0x000c0000;//18λ ��1, 19λ ��1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x00010000;//16λ ��1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x00030000;//16,17λ ��1
            break;
        }
    	break;
    case 1:
        m_Context.Dr1 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 4;
        m_Context.Dr7 &= 0xff0fffff;//�����20��21��22��23λ����0��
        //LEN ����
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x00400000;//22λ ��1
            break;
        case 4:
            m_Context.Dr7 |= 0x00c00000;//22λ ��1, 23λ ��1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x00100000;//20λ ��1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x00300000;//20,21λ ��1
            break;
        }
        break;
    case 2:
        m_Context.Dr2 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 0x10;
        m_Context.Dr7 &= 0xf0ffffff;//�����24��25��26��27λ����0��
        //LEN ����
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x04000000;//26λ ��1
            break;
        case 4:
            m_Context.Dr7 |= 0x0c000000;//26λ ��1, 27λ ��1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x01000000;//24λ ��1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x03000000;//24,25λ ��1
            break;
        }
        break;
    case 3:
        m_Context.Dr3 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 0x40;
        m_Context.Dr7 &= 0x0fffffff;//�����28��29��30��31λ����0��
        //LEN ����
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x40000000;//30λ ��1
            break;
        case 4:
            m_Context.Dr7 |= 0xc0000000;//30λ ��1, 31λ ��1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x10000000;//28λ ��1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x30000000;//28,29λ ��1
            break;
        }
        break;
    }

    if(!UpdateContextToThread())
    {
        return FALSE;
    }

    m_nHardPtNum++;
    printf("***Set Hard breakpoint success!***\r\n");
    return FALSE;
}

//Ӳ���ϵ��б�
BOOL CDoException::ListHardPoint(stuCommand* pCmd)
{
    printf("------------------------------------------------------------\r\n");
    printf("ID    Breakpoint type       Address     Access type   Length\r\n");
    if (m_nHardPtNum == 0)
    {
        return FALSE;
    }
    for (int i = 0; i < 4; i++)
    {
        if((m_Context.Dr7 & (int)pow(4.0, i)) != 0 || 
            (m_isNeedResetHardPoint == TRUE && m_nNeedResetHardPoint == i) )
        {
            printf("%d     Hardware breakpoint   ", i+1);
            switch (i)
            {
            case 0:
                printf("0x%p", m_Context.Dr0);
                break;
            case 1:
                printf("0x%p", m_Context.Dr1);
                break;
            case 2:
                printf("0x%p", m_Context.Dr2);
                break;
            case 3:
                printf("0x%p", m_Context.Dr3);
                break;
            }
            switch ((m_Context.Dr7 << (14 - 4*i)) >> 30)
            {
            case 0:
                printf("  EXECUTE");
                break;
            case 1:
                printf("  WRITE  ");
                break;
            case 3:
                printf("  ACCESS ");
                break;
            }
            printf("       ");
            switch ((m_Context.Dr7 << (12 - 4*i)) >> 30)
            {
            case 0:
                printf("1");
                break;
            case 1:
                printf("2");
                break;
            case 3:
                printf("4");
                break;
            }
            printf("\r\n");
          
        }
    }
    printf("------------------------------------------------------------\r\n");
    return FALSE;
}

//Ӳ���ϵ����
BOOL CDoException::ClearHardPoint(stuCommand* pCmd)
{
    int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);
    if (nID < 1 || nID > 4)
    {
        printf("Need valid ID!\r\n");
        return FALSE;
    }

    nID--;
    if ((m_Context.Dr7 & (int)pow(4.0, nID)) == 0 && 
        (nID != m_nNeedResetHardPoint || m_isNeedResetHardPoint == FALSE))
    {
        printf("Can not find the Hardware breakpoint!\r\n");
        return FALSE;
    }

    m_Context.Dr7 &= ~(int)pow(4.0, nID);

    //���Ҫ�����Ӳ���ϵ���� m_nNeedResetHardPoint ����Ҫɾ����Ӳ���ϵ���ţ�
    //��Ӳ���ϵ㲻������
    if (nID == m_nNeedResetHardPoint)
    {
        m_isNeedResetHardPoint = FALSE;
    }

    UpdateContextToThread();
    m_nHardPtNum--;
    printf("Clear the %d Hardware breakpoint.\r\n", nID+1);
    return FALSE;
}

//�����ڴ�ϵ�
BOOL CDoException::SetMemPoint(stuCommand* pCmd)
{
    LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

    if (lpAddr == 0)
    {
        printf("Need valid parameter!\r\n");
        return FALSE;
    }

    stuPointInfo tempPointInfo;
    stuPointInfo* pResultPointInfo = NULL;
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = lpAddr;
    tempPointInfo.ptType = MEM_POINT;
    tempPointInfo.isOnlyOne = FALSE;

    if (stricmp("access", pCmd->chParam2) == 0)
    {
        tempPointInfo.ptAccess = ACCESS;
    } 
    else if (stricmp("write", pCmd->chParam2) == 0)
    {
        tempPointInfo.ptAccess = WRITE;
    }
    else
    {
        printf("Void access!\r\n");
        return FALSE;
    }
    
    int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);

    if (nLen == 0 )
    {
        printf("Point length can not set Zero!\r\n");
        return FALSE;
    }

    tempPointInfo.dwPointLen = nLen;
    tempPointInfo.nPtNum = m_nOrdPtFlag;
    m_nOrdPtFlag++;

    if (FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
    {
        if (pResultPointInfo->dwPointLen >= nLen)//����ͬ�������ҳ��ȴ���Ҫ���öϵ�Ķϵ�
        {
            printf("The Memory breakpoint is already exist!\r\n");
            return FALSE;
        } 
        else//���ҵ��Ķϵ㳤��С��Ҫ���õĶϵ㳤�ȣ���ɾ�����ҵ��Ķϵ㣬��������
            //ֻɾ���ϵ�-��ҳ���� �� �ϵ����
        {
            DeletePointInList(pResultPointInfo->nPtNum, FALSE);
        }
    }
    
    // ���� tempPointInfo �����ڴ�ϵ�
    // ��Ӷϵ����������ڴ�ϵ�-��ҳ���м�¼����ӷ�ҳ��Ϣ���¼

    // ���ȸ��� tempPointInfo �еĵ�ַ�ͳ��Ȼ������Խ��ȫ����ҳ

    LPVOID lpAddress = (LPVOID)((int)tempPointInfo.lpPointAddr & 0xfffff000);
    DWORD OutAddr = (DWORD)tempPointInfo.lpPointAddr + 
            tempPointInfo.dwPointLen;

    MEMORY_BASIC_INFORMATION mbi = {0};

    while ( TRUE )
    {
        if ( sizeof(mbi) != VirtualQueryEx(m_hProcess, lpAddress, &mbi, sizeof(mbi)) )
        {
            break;
        }

        if ((DWORD)mbi.BaseAddress >= OutAddr)
        {
            break;            
        }
    
        if ( mbi.State == MEM_COMMIT )
        {
            //���ڴ��ҳ��Ϣ��ӵ���ҳ����
            AddRecordInPageList(mbi.BaseAddress, 
                                mbi.RegionSize, 
                                mbi.AllocationProtect);
            //���ϵ�-��ҳ��Ϣ��ӵ��ϵ�-��ҳ����
            DWORD dwPageAddr = (DWORD)mbi.BaseAddress;
            while (dwPageAddr < OutAddr)
            {
                stuPointPage *pPointPage = new stuPointPage;
                pPointPage->dwPageAddr = dwPageAddr;
                pPointPage->nPtNum = tempPointInfo.nPtNum;
                g_PointPageList.push_back(pPointPage);
                //���ø��ڴ�ҳΪ���ɷ���
                DWORD dwTempProtect;
                VirtualProtectEx(m_hProcess, (LPVOID)dwPageAddr,
                    1, PAGE_NOACCESS, &dwTempProtect);

                dwPageAddr += 0x1000;
            }

//             TRACE2("0x%p  0x%p \r\n",mbi.BaseAddress,mbi.RegionSize);       
        }
        lpAddress = (LPVOID)((DWORD)mbi.BaseAddress + mbi.RegionSize);
        if ((DWORD)lpAddress >= OutAddr)
        {
            break;
        }
    }

    //�ϵ���ӵ��ϵ���Ϣ����
    stuPointInfo *pPoint = new stuPointInfo;
    memcpy(pPoint, &tempPointInfo, sizeof(stuPointInfo));
    g_ptList.push_back(pPoint);
    printf("***Set Memory breakpoint success!***\r\n");

    return FALSE;
}

//�ڴ�ϵ��б�
BOOL CDoException::ListMemPoint(stuCommand* pCmd)
{
    printf("------------------------------------------------------------\r\n");
    printf("ID    Breakpoint type       Address     Access type   Length\r\n");
    list<stuPointInfo*>::iterator it = g_ptList.begin();
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->ptType == MEM_POINT)
        {
            printf("%03d   Memory breakpoint     0x%p", 
                pPointInfo->nPtNum, pPointInfo->lpPointAddr);
            if (pPointInfo->ptAccess == WRITE)
            {
                printf("  WRITE ");
            } 
            else
            {
                printf("  ACCESS");
            }
            printf("        0x%x\r\n", pPointInfo->dwPointLen);
        }
        it++;
    }
    printf("------------------------------------------------------------\r\n");
    return FALSE;
}

//�ڴ�ϵ����
BOOL CDoException::ClearMemPoint(stuCommand* pCmd)
{
    int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);
    
    if (nID == 0)
    {
        printf("Need valid ID!\r\n");
        return FALSE;
    }
    
    if (DeletePointInList(nID, TRUE))
    {
        //���Ҫɾ�����ڴ�ϵ�������Ҫ�ָ����ڴ�ϵ㣬����Ҫ�ٻָ��ڴ�ϵ�
        list<stuResetMemBp*>::iterator itDw = g_ResetMemBp.begin();
        while (itDw != g_ResetMemBp.end())
        {
            stuResetMemBp* p = *itDw;
            itDw++;
            if (p->nID == nID)
            {
                delete p;
                g_ResetMemBp.remove(p);
                break;
            }
        }

//         if (nID == m_nNeedResetMemPointID1)
//         {
//             m_isNeedResetPageProp1 = FALSE;
//         }
//         if (nID == m_nNeedResetMemPointID2)
//         {
//             m_isNeedResetPageProp2 = FALSE;
//         }
        printf("Clear the %d Memory breakpoint.\r\n", nID);
    }
    else
    {
        printf("Can not find the Memory breakpoint!\r\n");
    }
    return FALSE;
}

//��ʾ����
BOOL CDoException::ShowHelp(stuCommand* pCmd)
{
//     printf("******************************************************************\r\n");
//     printf("Description         Command  Parameter1    Parameter2   Parameter3\r\n");
//     printf("******************************************************************\r\n");
//     printf("Step into           T\r\n");
//     printf("Step over           P\r\n");
//     printf("Run                 G        [Address]\r\n");
//     printf("------------------------------------------------------------------\r\n");
//     printf("Disassembler        U        [Address]\r\n");
//     printf("Data                D        [Address]\r\n");
//     printf("Registers           R\r\n");
//     printf("------------------------------------------------------------------\r\n");
//     printf("Int3 breakpoint     BP       Address\r\n");
//     printf("Int3 list           BPL\r\n");
//     printf("Delete a int3       BPC      Breakpoint ID\r\n");
//     printf("------------------------------------------------------------------\r\n");
//     printf("Hardware breakpoint BH       Address       Access type  Length\r\n");
//     printf("Hardware bp list    BHL\r\n");
//     printf("Delete a HD bp      BHC      Breakpoint ID\r\n");
//     printf("------------------------------------------------------------------\r\n");
//     printf("Memory breakpoint   BM       Address       Access type  Length\r\n");
//     printf("Memory bp list      BML\r\n");
//     printf("Delete a memory bp  BMC      Breakpoint ID\r\n");
//     printf("------------------------------------------------------------------\r\n");
//     printf("Help                H\r\n");
//     printf("******************************************************************\r\n");
    printf("   ============================ help menu ===========================---=====\r\n");
    printf("   **************************************************************************\r\n");
    printf("\
   * ��� ������      ������ Ӣ��˵��        ����1    ����2    ����3        *\r\n\
   * 1    ��������      T    step into                                      *\r\n\
   * 2    ��������      P    step over                                      *\r\n\
   * 3    ����          G    run             ��ַ����                       *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 4    �����        U    assemble        ��ַ����                       *\r\n\
   * 5    ����          D    data            ��ַ����                       *\r\n\
   * 6    �Ĵ���        R    register                                       *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 7    һ��ϵ�      bp   breakpoint      ��ַ    [once](һ����)         *\r\n\
   * 8    һ��ϵ��б�  bpl  bp list                                        *\r\n\
   * 9    ɾ��һ��ϵ�  bpc  clear bp        ���                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 10   Ӳ���ϵ�      bh ��hard bp         ��ַ execute/access/write ���� *\r\n\
   * 11   Ӳ���ϵ��б�  bhl  hard bp list                                   *\r\n\
   * 12   ɾ��Ӳ���ϵ�  bhc  clear hard bp   ���                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 13   �ڴ�ϵ�      bm   memory bp       ��ʼ��ַ access/write ����     *\r\n\
   * 14   �ڴ�ϵ��б�  bml  memory bp list                                 *\r\n\
   * 15   ɾ���ڴ�ϵ�  bmc  clear memory bp ���                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 16   ����ű�      ls   load script                                    *\r\n\
   * 17   �����ű�      es   export script                                  *\r\n\
   * 18   ������¼      sr   step record                                    *\r\n\
   * 19   ����          h    help                                           *\r\n");
    printf("   **************************************************************************\r\n");
    return FALSE;
}

//��������
BOOL CDoException::StepInto(stuCommand* pCmd)
{
    UpdateContextFromThread();
    
    //���õ���
    m_Context.EFlags |= TF;
    m_isUserInputStep = TRUE;
        
    UpdateContextToThread();    
    return TRUE;
}

//��������
BOOL CDoException::StepOver(stuCommand* pCmd)
{
    char            CodeBuf[20] = {0};
    t_disasm        da;
    int             nCodelen;
    BOOL            bRet;
    BOOL            isNeedResetFirstPage = FALSE;
    BOOL            isNeedResetSecondPage = FALSE;
    DWORD           dwTempProtect1;
    DWORD           dwTempProtect2;
    DWORD           dwOldProtect;
    
    //�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
    //����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊԭ������
    //ע�⣬�����ڴ���ܿ��ҳ
    if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
            1, PAGE_READONLY, &dwTempProtect1);
        isNeedResetFirstPage = TRUE;
    }
    
    if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
            1, PAGE_READONLY, &dwTempProtect2);
        isNeedResetSecondPage = TRUE;
    }
    
    bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 20, NULL);
    
    //����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
    //������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
    //�����Ȼָ� SecondPage����ָ� FirstPage
    if (isNeedResetSecondPage)
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
            1, dwTempProtect2, &dwOldProtect);
        isNeedResetSecondPage = FALSE;
    }
    
    if (isNeedResetFirstPage)
    {
        VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
            1, dwTempProtect1, &dwOldProtect);
        isNeedResetFirstPage = FALSE;
    }
    
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return FALSE;
    }
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//���÷��������
    (da.result)[4] = '\0';
    if (strcmp(da.result, "CALL") == 0)
    {
        char buf[20] = {0};
        sprintf(buf, "%p", (int)m_Context.Eip + nCodelen);
        strcpy(m_UserCmd.chParam1, buf);
        Run(&m_UserCmd);
    }
    else
    {
        StepInto(NULL);
    }

    return TRUE;
}

//����
BOOL CDoException::Run(stuCommand* pCmd)
{
    LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
    if (lpAddr != (LPVOID)0)
    {
        wsprintf(pCmd->chParam2, "%s", "once");
        SetOrdPoint(pCmd);
    }
    return TRUE;
}

//16�����ַ���ת��Ϊ��ֵ
LPVOID CDoException::HexStringToHex(char* pHexString, BOOL isShowError)
{
    DWORD dwAddr = 0;
    int nLen = strlen(pHexString);
    int j = 1;
    for (int i = 0; i < nLen; i++)
    {
        char ch = pHexString[nLen-i-1];

        if (ch >= 'A' && ch <= 'F')
        {
            dwAddr += j*(ch - 'A' + 10);
        } 
        else if (ch >= 'a' && ch <= 'f')
        {
            dwAddr += j*(ch - 'a' + 10);
        }
        else if (ch >= '0' && ch <= '9')
        {
            dwAddr += j*(ch - '0' + 0);
        }
        else
        {
            if (isShowError)
            {
                printf("Invoid hex value!\r\n");
            }
            return 0;
        }
        j *= 16;
    }
    return (LPVOID)dwAddr;
}


//�ڶϵ��б��в����Ƿ���ƥ��PointInfo�ϵ���Ϣ�Ķϵ㡣
//ֻƥ�� �ϵ���ʼ��ַ���ϵ����ͣ��ϵ�������͡�
//����isNeedSave����ʾ�Ƿ���Ҫ����
//����ֵΪTRUE��ʾ�ҵ���FALSE��ʾû���ҵ���
//�ҵ��Ķϵ�ָ�룬���� ppResultPointInfo ������
BOOL CDoException::FindPointInList(IN stuPointInfo PointInfo, 
                                   OUT stuPointInfo** ppResultPointInfo,
                                   BOOL isNeedSave)
{
    list<stuPointInfo*>::iterator it = g_ptList.begin();
    
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* tempPointInfo = *it;
        if (tempPointInfo->lpPointAddr == PointInfo.lpPointAddr && 
            tempPointInfo->ptType == PointInfo.ptType &&
            tempPointInfo->ptAccess == PointInfo.ptAccess)
        {
            *ppResultPointInfo = tempPointInfo;
            if (isNeedSave == TRUE)
            {
                m_itFind = it;
                m_pFindPoint = tempPointInfo;
            }
            return TRUE;
        }
        it++;
    }
    
    return FALSE;

}

BOOL CDoException::FindPointInList(LPVOID lpAddr, PointType ptType, BOOL isNeedSave)
{
    list<stuPointInfo*>::iterator it = g_ptList.begin();

    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->lpPointAddr == lpAddr && 
            pPointInfo->ptType == ptType)
        {
            if (isNeedSave == TRUE)
            {
                m_itFind = it;
                m_pFindPoint = pPointInfo;
            }
            return TRUE;
        }
        it++;
    }

    return FALSE;
}

//��CONTEXT�в����Ƿ��Ѿ����� PointInfo ָ����Ӳ���ϵ�
//����TRUE��ʾ�ҵ���FALSE��ʾδ�ҵ�
//����nDrNum�����ҵ���DRX�Ĵ������±꣬����nPointLen�����ҵ��Ķϵ㳤��
BOOL CDoException::FindPointInConext(stuPointInfo PointInfo, 
                                     int *nDrNum, int *nPointLen)
{
    if((m_Context.Dr7 & 1) && m_Context.Dr0 == (DWORD)PointInfo.lpPointAddr)
    {
        int nAccess = (m_Context.Dr7 << 14) >> 30;  //�ϵ㴥������
        if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
            ((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
            ((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
            )
        {
            *nDrNum = 0;
            *nPointLen = ((m_Context.Dr7 << 12) >> 30);
            switch (*nPointLen)
            {
            case 0:
                *nPointLen = 1;
            	break;
            case 1:
                *nPointLen = 2;
                break;
            case 3:
                *nPointLen = 4;
                break;
            default:
                printf("error!\r\n");
            }
            return TRUE;
        }
    }

    if((m_Context.Dr7 & 4) && m_Context.Dr1 == (DWORD)PointInfo.lpPointAddr)
    {
        int nAccess = (m_Context.Dr7 << 10) >> 30;
        if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
            ((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
            ((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
            )
        {
            *nDrNum = 1;
            *nPointLen = ((m_Context.Dr7 << 8) >> 30);
            switch (*nPointLen)
            {
            case 0:
                *nPointLen = 1;
                break;
            case 1:
                *nPointLen = 2;
                break;
            case 3:
                *nPointLen = 4;
                break;
            default:
                printf("error!\r\n");
            }
            return TRUE;
        }
    }

    if((m_Context.Dr7 & 0x10) && m_Context.Dr2 == (DWORD)PointInfo.lpPointAddr)
    {
        int nAccess = (m_Context.Dr7 << 6) >> 30;
        if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
            ((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
            ((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
            )
        {
            *nDrNum = 2;
            *nPointLen = ((m_Context.Dr7 << 4) >> 30);
            switch (*nPointLen)
            {
            case 0:
                *nPointLen = 1;
                break;
            case 1:
                *nPointLen = 2;
                break;
            case 3:
                *nPointLen = 4;
                break;
            default:
                printf("error!\r\n");
            }
            return TRUE;
        }
    }

    if((m_Context.Dr7 & 0x40) && m_Context.Dr3 == (DWORD)PointInfo.lpPointAddr)
    {
        int nAccess = (m_Context.Dr7 << 2) >> 30;
        if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
            ((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
            ((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
            )
        {
            *nDrNum = 3;
            *nPointLen = (m_Context.Dr7 >> 30);
            switch (*nPointLen)
            {
            case 0:
                *nPointLen = 1;
                break;
            case 1:
                *nPointLen = 2;
                break;
            case 3:
                *nPointLen = 4;
                break;
            default:
                printf("error!\r\n");
            }
            return TRUE;
        }
    }

    return FALSE;
}

//���߳���Ϣ���µ� M_Context ������
BOOL CDoException::UpdateContextFromThread()
{
    BOOL bRet = TRUE;
    m_Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    bRet = GetThreadContext(m_hThread, &m_Context);
    if (bRet == FALSE)
    {
        printf("GetThreadContext error!\r\n");
    }
    return bRet;   
}

//�� M_Context �еĻ������µ��߳���
BOOL CDoException::UpdateContextToThread()
{
    BOOL bRet = TRUE;
    bRet = SetThreadContext(m_hThread, &m_Context);
    if (bRet == FALSE)
    {
        printf("SetThreadContext error!\r\n");
    }
    return bRet;   
}

// �����ڴ�ϵ����ţ�ɾ���ϵ�-��ҳ���� �� �ϵ��
// ���� isNeedResetProtect��˵���Ƿ�Ҫ�ָ��ڴ�ҳԭ��������
BOOL CDoException::DeletePointInList(int nPtNum, BOOL isNeedResetProtect)
{
    //�ȸ�������ҵ���Ӧ�ϵ�
    list<stuPointInfo*>::iterator it = g_ptList.begin();
    
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->nPtNum == nPtNum)//�ҵ���
        {
            //���ж��ǲ����ڴ�ϵ㣬������Ǿ��Ǵ����
            if (pPointInfo->ptType != MEM_POINT)
            {
                printf("Only allow memony breakpoint reach here!\r\n");
                return FALSE;
            }

            //���ڴ�ϵ㣬Ҫɾ���ϵ���� �� �ϵ�-��ҳ����
            delete [] pPointInfo;
            g_ptList.erase(it);

            //ɾ���ϵ�-��ҳ����
            DeleteRecordInPointPageList(nPtNum, isNeedResetProtect);

            return TRUE;
        }
        it++;
    }
    return FALSE;
}

//���ڴ��ҳ��Ϣ��ӵ���ҳ���У���������еļ�¼�ظ��������
void CDoException::AddRecordInPageList(LPVOID BaseAddr, 
                                       DWORD dwRegionSize, 
                                       DWORD dwProtect)
{
    DWORD dwBaseAddr = (DWORD)BaseAddr;
    int nPageNum = dwRegionSize / 0x1000;
    for (int i = 0; i < nPageNum; i++)
    {
        stuPageInfo* pFind = NULL;
        if (!FindRecordInPageList(dwBaseAddr, &pFind))
        {
            stuPageInfo* pPageInfo = new stuPageInfo;
            pPageInfo->dwPageAddr = dwBaseAddr;
            pPageInfo->dwOldProtect = dwProtect;
            g_PageList.push_back(pPageInfo);
        }

        dwBaseAddr += 0x1000;
    }
}


//���ҷ�ҳ��Ϣ���еļ�¼
BOOL CDoException::FindRecordInPageList(DWORD dwBaseAddr, 
                                        stuPageInfo** ppFind)
{
    list<stuPageInfo*>::iterator it = g_PageList.begin();
    
    for ( int i = 0; i < g_PageList.size(); i++ )
    {
        stuPageInfo* pPageInfo = *it;
        if (pPageInfo->dwPageAddr == dwBaseAddr)
        {
            *ppFind = pPageInfo;
            return TRUE;
        }
        it++;
    }
    return FALSE;
}

//ɾ����ҳ��Ϣ���еļ�¼����������ò��ϣ�
void CDoException::DeleteRecordInPageList(DWORD dwBaseAddr)
{
    list<stuPageInfo*>::iterator it = g_PageList.begin();
    
    for ( int i = 0; i < g_PageList.size(); i++ )
    {
        stuPageInfo* pPageInfo = *it;
        if (pPageInfo->dwPageAddr == dwBaseAddr)
        {
            delete [] pPageInfo;
            g_PageList.erase(it);
            return;
        }
        it++;
    }

}

//�Ӷϵ�-��ҳ����ɾ���ϵ����Ϊ nPtNum �ļ�¼��
//���� isNeedResetProtect ��ָ���Ƿ���Ҫ�ָ���ҳԭ�������ԡ�
void CDoException::DeleteRecordInPointPageList(int nPtNum, BOOL isNeedResumeProtect)
{
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    
    //һ���ϵ���ܶ�Ӧ�����ҳ������Ҫ��������
    while ( it != g_PointPageList.end() )
    {
        stuPointPage* pPointPage = *it;
        it++;
        if (pPointPage->nPtNum == nPtNum)
        {
            //�ָ��ڴ�ҳԭ������
            if (isNeedResumeProtect)
            {
                //����ҵ�����һ���ڴ�ϵ�Ҳ�������ҳ�ϣ�����Ҫ�ָ��ڴ�ҳ����
                if(FindAnotherRecordInPointPageList(pPointPage) == FALSE)
                {
                    stuPageInfo* pPageInfo = NULL;
                    FindRecordInPageList(pPointPage->dwPageAddr, &pPageInfo);
                    if (pPageInfo)
                    {
                        DWORD dwTempProtect;
                        VirtualProtectEx(m_hProcess, (LPVOID)pPageInfo->dwPageAddr,
                            1, pPageInfo->dwOldProtect, &dwTempProtect);
                    }
                }
            }
            delete pPointPage;
            g_PointPageList.remove(pPointPage);
        }
    }
}

//��ʱ�ָ��ڴ�����ҳ������
void CDoException::TempResumePageProp(DWORD dwPageAddr)
{
    list<stuPageInfo*>::iterator it = g_PageList.begin();
    
    for ( int i = 0; i < g_PageList.size(); i++ )
    {
        stuPageInfo* pPageInfo = *it;
        if (pPageInfo->dwPageAddr == dwPageAddr)
        {
            DWORD dwTempProtect;
            VirtualProtectEx(m_hProcess, (LPVOID)pPageInfo->dwPageAddr,
                1, pPageInfo->dwOldProtect, &dwTempProtect);
            return;
        }
        it++;
    }
}

//�ڶϵ�-��ҳ���в��Ҽ�¼
BOOL CDoException::FindRecordInPointPageList(DWORD dwPageAddr)
{
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    
    for ( int i = 0; i < g_PointPageList.size(); i++ )
    {
        stuPointPage* pPointPage = *it;
        if (pPointPage->dwPageAddr == dwPageAddr)
        {
            return TRUE;
        }
        it++;
    }
    return FALSE;
}

void CDoException::ShowBreakPointInfo(stuPointInfo *pPoint)
{
    if (pPoint->isOnlyOne == TRUE && pPoint->ptType == ORD_POINT)
    {
        return;
    }

    printf("***********************************************************\r\n");
    printf("Hit the breakpoint: \r\n");
    printf("breakpoint's ID: %d. ", pPoint->nPtNum);
    if (pPoint->ptType == ORD_POINT)
    {
        printf("Ordinary breakpoint(INT3) at 0x%p.\r\n", pPoint->lpPointAddr);
    } 
    else if(pPoint->ptType == MEM_POINT)
    {
        printf("       Memory breakpoint at 0x%p.\r\n", pPoint->lpPointAddr);
        if (pPoint->ptAccess == ACCESS)
        {
            printf("Breakpoint type is ACCESS. ");
        } 
        else if (pPoint->ptAccess == WRITE)
        {
            printf("Breakpoint type is WRITE. ");
        }
        else
        {
            printf("error!");//�������������
        }
        printf("Breakpoint length is %d.\r\n", pPoint->dwPointLen);
    }
    printf("***********************************************************\r\n");
}

void CDoException::ShowHardwareBreakpoint(DWORD dwDr6Low)
{
    printf("***********************************************************\r\n");
    printf("Hit the breakpoint: \r\n");
    int nIdx;
    switch (dwDr6Low)
    {
    case 1:
        nIdx = 0;
        printf("Breakpoint address: %p", m_Context.Dr0);
    	break;
    case 2:
        nIdx = 1;
        printf("Breakpoint address: %p", m_Context.Dr1);
        break;
    case 4:
        nIdx = 2;
        printf("Breakpoint address: %p", m_Context.Dr2);
        break;
    case 8:
        nIdx = 3;
        printf("Breakpoint address: %p", m_Context.Dr3);
        break;
    }
    printf(".   Hardware breakpoint at Dr%d.\r\n", nIdx);
    int nType = (m_Context.Dr7 << (14 - (nIdx * 2))) >> 30;
    int nLen = (m_Context.Dr7 << (12 - (nIdx * 2))) >> 30;

    printf("Breakpoint type is");
    switch (nType)
    {
    case 0:
        printf(" EXECUTE.");
        break;
    case 1:
        printf(" WRITE.  ");
        break;
    case 3:
        printf(" ACCESS. ");
        break;
    }
    printf("     Breakpoint length is ");
    switch (nLen)
    {
    case 0:
        printf("1");
        break;
    case 1:
        printf("2");
        break;
    case 3:
        printf("4");
        break;
    }
    printf(".\r\n");
    printf("***********************************************************\r\n");
}

BOOL CDoException::ShowFunctionName(char *pResult)
{
    int nLen = strlen(pResult);
    char* pAddr;
    BOOL bRet;
    DWORD dwFunAddr = 0;

    //�Ӻ���ǰ������ �ո� �� ���� �ĵط�
    while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
    {
        nLen--;
    }
    pAddr = &pResult[nLen];

    //�Ƿ���������[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        //ȥ��������֮���Ƿ��ǺϷ��ĵ�ַ
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);

        if (dwAddr == 0)
        {
            return FALSE;
        }
        
        //����ַ��ȥȡ����
        DWORD dwOldProtect;
        DWORD dwNoUseProtect;
        VirtualProtectEx(m_hProcess, dwAddr, 1, PAGE_READWRITE, &dwOldProtect);
        bRet = ReadProcessMemory(m_hProcess, dwAddr, &dwFunAddr, 4, NULL);
        VirtualProtectEx(m_hProcess, dwAddr, 1, dwOldProtect, &dwNoUseProtect);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error");
            return FALSE;
        }
    } 
    else
    {
        //ֱ���жϵ�ַ���Ƿ���API
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        if (dwAddr == 0)
        {
            return FALSE;
        }
        dwFunAddr = (DWORD)dwAddr;
    }

    //�ж� dwFunAddr �Ƿ���API��ַ
    //�����ж� dwFunAddr ���ĸ�ģ��ĵ�ַ
    BOOL isHit = FALSE;
    stuDllInfo* pDllInfo = NULL;
    list<stuDllInfo*>::iterator it = g_DllList.begin();
    for (int i = 0; i < g_DllList.size(); i++)
    {
        pDllInfo = *it;
        if (dwFunAddr > pDllInfo->dwDllAddr && 
            dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
        {
            isHit = TRUE;
            break;
        }
        it++;
    }

    //���û���ҵ���Ӧ��ģ�飬��ɾ����ģ�������е�ģ���¼
    //����ö��ģ�飬���²��Ҷ�Ӧģ��
    if (isHit == FALSE)
    {
        list<stuDllInfo*>::iterator itDll = g_DllList.begin();
        while (itDll != g_DllList.end())
        {
            stuDllInfo* pDll = *itDll;
            itDll++;
            delete pDll;
            g_DllList.remove(pDll);
        }

        EnumDestMod();

        it = g_DllList.begin();
        for (int i = 0; i < g_DllList.size(); i++)
        {
            pDllInfo = *it;
            if (dwFunAddr > pDllInfo->dwDllAddr && 
                dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
            {
                isHit = TRUE;
                break;
            }
            it++;
        }

    }

    if (isHit == FALSE)
    {
        return FALSE;
    }

    //���������Ƿ�����ĳ������
    char chFuncName[MAXBYTE] = {0};
    isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);

    if (isHit == TRUE)
    {
        printf("(%s)", chFuncName);
        return TRUE;
    }

    //���CALL���ĵ�ַ��һ����ת��JMP��CALL���ٽ������JMP��CALL�ĵ�ַ
    char            CodeBuf[20];
    t_disasm        da;
    int             nCodelen;
    BOOL            isNeedResetFirstPage = FALSE;
    BOOL            isNeedResetSecondPage = FALSE;
    DWORD           dwTempProtect1;
    DWORD           dwTempProtect2;
    DWORD           dwOldProtect;
    
    //�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
    //����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊ���ɷ���
    //ע�⣬�����ڴ���ܿ��ҳ
    if (FindRecordInPointPageList((DWORD)dwFunAddr & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, (LPVOID)dwFunAddr,
            1, PAGE_READONLY, &dwTempProtect1);
        isNeedResetFirstPage = TRUE;
    }
    
    if (FindRecordInPointPageList(((DWORD)dwFunAddr + 20) & 0xfffff000))
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)dwFunAddr+20),
            1, PAGE_READONLY, &dwTempProtect2);
        isNeedResetSecondPage = TRUE;
    }
    
    bRet = ReadProcessMemory(m_hProcess, (LPVOID)dwFunAddr, CodeBuf, 20, NULL);
    
    //����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
    //������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
    //�����Ȼָ� SecondPage����ָ� FirstPage
    if (isNeedResetSecondPage)
    {
        VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)dwFunAddr+20),
            1, dwTempProtect2, &dwOldProtect);
        isNeedResetSecondPage = FALSE;
    }
    
    if (isNeedResetFirstPage)
    {
        VirtualProtectEx(m_hProcess, (LPVOID)dwFunAddr,
            1, dwTempProtect1, &dwOldProtect);
        isNeedResetFirstPage = FALSE;
    }
    
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return FALSE;
    }
    
    //����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
    if (CodeBuf[0] == 0xCC)
    {
        stuPointInfo tempPointInfo;
        stuPointInfo* pResultPointInfo = NULL;
        memset(&tempPointInfo, 0, sizeof(stuPointInfo));
        tempPointInfo.lpPointAddr = m_lpDisAsmAddr;
        tempPointInfo.ptType = ORD_POINT;
        
        if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
        {
            CodeBuf[0] = pResultPointInfo->u.chOldByte;
        }
    }
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, dwFunAddr);//���÷��������
    
    //����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
    char chCall[5] = {0};
    char chJmp[2] = {0};
    memcpy(chCall, da.result, 4);
    memcpy(chJmp, da.result, 1);
    if (stricmp(chCall, "CALL") == 0 || stricmp(chJmp, "J") == 0)
    {
        printf("(%s)", da.result);
        return ShowFunctionName(da.result);
    }
    return FALSE;
}

//ö��ģ��
void CDoException::EnumDestMod()
{
    HANDLE hmodule = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_ProcessId);
    
    if (hmodule == INVALID_HANDLE_VALUE)
    {
        printf("CreateToolhelp32Snapshot error!\r\n");
    }
    
    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);
    
    if (::Module32First(hmodule, &me))
    {
        do 
        {
            stuDllInfo* pDllInfo = new stuDllInfo;
            pDllInfo->dwDllAddr = (DWORD)me.modBaseAddr;
            pDllInfo->dwModSize = me.modBaseSize;
            strcpy(pDllInfo->szDllName, me.szModule);
            g_DllList.push_back(pDllInfo);
        } while (::Module32Next(hmodule, &me));
    }
    
    CloseHandle(hmodule);

}

//���Һ�������
BOOL CDoException::FindFunction(DWORD dwFunAddr, DWORD dwDllAddr, char* pFunName)
{
    DWORD dwNtHeaderRva = 0;
    DWORD dwOldProtect;
    DWORD dwNoUseProtect;
    BOOL bRet;
    VirtualProtectEx(m_hProcess, (LPVOID)dwDllAddr, 1, PAGE_READWRITE, &dwOldProtect);
    bRet = ReadProcessMemory(CDoException::m_hProcess, 
                (LPVOID)((DWORD)dwDllAddr + 0x3c), &dwNtHeaderRva, 4, NULL);

    if (bRet == FALSE)
    {
        printf("FindFunction ReadProcessMemory error!\r\n");
        return FALSE;
    }

    DWORD pNtHeader = (DWORD)dwDllAddr + dwNtHeaderRva;
    
    DWORD dwRva = 0;
    bRet = ReadProcessMemory(CDoException::m_hProcess, 
        (LPVOID)(pNtHeader + 0x78), &dwRva, 4, NULL);
    if (bRet == FALSE)
    {
        printf("FindFunction ReadProcessMemory error!\r\n");
        return FALSE;
    }

    VirtualProtectEx(m_hProcess, (LPVOID)dwDllAddr, 1, dwOldProtect, &dwNoUseProtect);
    
    if (dwRva == 0)
    {
        return FALSE;
    }

    IMAGE_EXPORT_DIRECTORY ExportDir = {0};
    VirtualProtectEx(m_hProcess, (LPVOID)(dwRva + (DWORD)dwDllAddr),
                     1, PAGE_READWRITE, &dwOldProtect);
    bRet = ReadProcessMemory(CDoException::m_hProcess, 
                      (LPVOID)(dwRva + (DWORD)dwDllAddr), 
                      &ExportDir, sizeof(IMAGE_EXPORT_DIRECTORY), NULL);
    if (bRet == FALSE)
    {
        printf("FindFunction ReadProcessMemory error!\r\n");
        return FALSE;
    }
    VirtualProtectEx(m_hProcess, (LPVOID)(dwRva + (DWORD)dwDllAddr),
        1, dwOldProtect, &dwNoUseProtect);

    DWORD dwRvaFunAddr = ExportDir.AddressOfFunctions;

    int i;
    BOOL isHit = FALSE;
    for (i = 0; i < ExportDir.NumberOfFunctions; i++)
    {
        DWORD dwRvaReadFunAddr = 0;
        VirtualProtectEx(m_hProcess, (LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i),
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = ReadProcessMemory(CDoException::m_hProcess, 
            (LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i), 
            &dwRvaReadFunAddr, 4, NULL);
        if (bRet == FALSE)
        {
            printf("FindFunction ReadProcessMemory error!\r\n");
            return FALSE;
        }
        VirtualProtectEx(m_hProcess, (LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i),
            1, dwOldProtect, &dwNoUseProtect);

        if (dwFunAddr == dwRvaReadFunAddr + (DWORD)dwDllAddr)
        {
            isHit = TRUE;
            break;
        }
    }

    if (isHit == FALSE)
    {
        return FALSE;
    }

    //������� i �ҵ�������Ŷ�Ӧ���±�j   �ҵ�������
    int j = 0;
    WORD dwNameOrd = 0;
    BOOL isHaveName = FALSE;    //�Ƿ��к�����
    for(; j < ExportDir.NumberOfNames; j++)
    {
        VirtualProtectEx(m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = ReadProcessMemory(CDoException::m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
            &dwNameOrd, 4, NULL);
        if (bRet == FALSE)
        {
            printf("FindFunction ReadProcessMemory error!\r\n");
            return FALSE;
        }
        VirtualProtectEx(m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
            1, dwOldProtect, &dwNoUseProtect);

        if ( (dwNameOrd == i))
        {
            isHaveName = TRUE;
            break;
        }
    }

    if (isHaveName)
    {
        DWORD dwRvaFunNameAddr = 0;
        VirtualProtectEx(m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = ReadProcessMemory(CDoException::m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
            &dwRvaFunNameAddr, 4, NULL);
        if (bRet == FALSE)
        {
            printf("FindFunction ReadProcessMemory error!\r\n");
            return FALSE;
        }
        VirtualProtectEx(m_hProcess, 
            (LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
            1, dwOldProtect, &dwNoUseProtect);

        VirtualProtectEx(m_hProcess, 
            (LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
            1, PAGE_READWRITE, &dwOldProtect);
        bRet = ReadProcessMemory(CDoException::m_hProcess, 
            (LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
            pFunName, MAXBYTE, NULL);
        if (bRet == FALSE)
        {
            printf("FindFunction ReadProcessMemory error!\r\n");
            return FALSE;
        }
        VirtualProtectEx(m_hProcess, 
            (LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
            1, dwOldProtect, &dwNoUseProtect);
    }
    else //��ŷ�ʽ�ĺ���
    {
        wsprintf(pFunName, "#%d", i + ExportDir.Base);
    }
    
    char DllName[MAXBYTE] = {0};
    VirtualProtectEx(m_hProcess, 
        (LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
        1, PAGE_READWRITE, &dwOldProtect);
    bRet = ReadProcessMemory(CDoException::m_hProcess, 
        (LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
        DllName, MAXBYTE, NULL);
    if (bRet == FALSE)
    {
        printf("FindFunction ReadProcessMemory error!\r\n");
        return FALSE;
    }
    VirtualProtectEx(m_hProcess, 
        (LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
        1, dwOldProtect, &dwNoUseProtect);

    i = 0;
    while (DllName[i] != '.')
    {
        i++;
    }
    DllName[++i] = '\0';
    strcat(DllName, pFunName);
    memcpy(pFunName, DllName, MAXBYTE);
    return TRUE;
}

BOOL CDoException::FindAnotherRecordInPointPageList(stuPointPage *p)
{
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    
    for ( int i = 0; i < g_PointPageList.size(); i++ )
    {
        stuPointPage* pPointPage = *it;
        if (pPointPage->dwPageAddr == p->dwPageAddr && 
            pPointPage->nPtNum != p->nPtNum)
        {
            return TRUE;
        }
        it++;
    }
    return FALSE;
}

//�ͷ�������Դ
void CDoException::ReleaseResource()
{
    if (m_hAppend)
    {
        CloseHandle(m_hAppend);
        m_hAppend = NULL;
    }

    list<stuDllInfo*>::iterator itDll = g_DllList.begin();
    while (itDll != g_DllList.end())
    {
        stuDllInfo* pDll = *itDll;
        itDll++;
        delete pDll;
        g_DllList.remove(pDll);
    }

    list<stuPointInfo*>::iterator itPoint = g_ptList.begin();
    while ( g_ptList.size() )
    {
        stuPointInfo* pPoint = *itPoint;
        itPoint++;
        delete pPoint;
        g_ptList.remove(pPoint);
    }

    list<stuPointPage*>::iterator itPointPage = g_PointPageList.begin();
    while ( g_PointPageList.size() )
    {
        stuPointPage* pPointPage = *itPointPage;
        itPointPage++;
        delete pPointPage;
        g_PointPageList.remove(pPointPage);
    }

    list<stuCmdNode*>::iterator itCmd = g_CmdList.begin();                    //��������
    while ( g_CmdList.size() )
    {
        stuCmdNode* pCmd = *itCmd;
        itCmd++;
        delete pCmd;
        g_CmdList.remove(pCmd);
    }
  
    list<stuPageInfo*>::iterator itPage = g_PageList.begin();                 //��ҳ��Ϣ����
    while ( g_PageList.size() )
    {
        stuPageInfo* pPage = *itPage;
        itPage++;
        delete pPage;
        g_PageList.remove(pPage);
    }

    list<stuCommand*>::iterator itUser = g_UserInputList.begin();             //�����û�����ĺϷ����������
    while ( g_UserInputList.size() )
    {
        stuCommand* pUser = *itUser;
        itUser++;
        delete pUser;
        g_UserInputList.remove(pUser);
    }
}

//�����������Ҷ�Ӧ�Ĵ�����ָ��
CmdProcessFun CDoException::GetFunFromAryCmd(stuCommand m_UserCmd)
{
    int i = 0;
    while (g_aryCmd[i].pFun != NULL)
    {
        if (stricmp(g_aryCmd[i].chCmd, m_UserCmd.chCmd) == 0)
        {
            return g_aryCmd[i].pFun;
        }
        i++;
    }
    return NULL;
}

//������¼����
BOOL CDoException::StepRecord(stuCommand* pCmd)
{
    //ʹ�õ�����¼���ܣ���ʾ�û�ȡ���ڴ�ϵ�
    if ((MessageBox(NULL, "You should clear all Memory breakpoint, \
otherwise may be bring access mistake!\r\n\r\n      \
                            Do you want to continue?",
         "", MB_YESNO) == IDNO)
         )
    {
        return FALSE;
    }
    
    printf("please wait for a moment......\r\n");
    //ѡ��Ҫ������ļ�
    char            szFileName[MAX_PATH] = "";	
    OPENFILENAME    ofn;
    char            CodeBuf[24] = {0};
    
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.lpstrFile	 = szFileName;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = "code record files(*.txt)\0*.txt\0";
    ofn.nFilterIndex = 1;
    if( GetSaveFileName(&ofn) == FALSE)
    {
        return FALSE;
    }
    
    m_hAppend = CreateFile( szFileName,                   // open
                            GENERIC_WRITE,                // open for writing 
                            0,                            // do not share 
                            NULL,                         // no security 
                            OPEN_ALWAYS,                  // open or create 
                            FILE_ATTRIBUTE_NORMAL,        // normal file 
                            NULL);                        // no attr. template 
    
    if (m_hAppend == INVALID_HANDLE_VALUE) 
    { 
        printf("Could not create file.\r\n");    // process error 
        return FALSE;
    }

    m_isStepRecordMode = TRUE;

    //�Ƿ���Ҫ����Ļ����ʾ����
    if ((MessageBox(NULL, "Need show code on user interface?",
        "", MB_YESNO) == IDYES)
        )
    {
        m_isShowCode = TRUE;
    }

    BOOL bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!");
        return FALSE;
    }
    //��¼ָ��
    RecordCode(m_Context.Eip, CodeBuf);
  
    return TRUE;
}

//����ű�
BOOL CDoException::LoadScript(stuCommand *pCmd)
{
    //ѡ��Ҫ������ļ�
    char            szFileName[MAX_PATH] = "";	
    OPENFILENAME    ofn;
    BOOL            bRet;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.lpstrFile	 = szFileName;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = "script Files(*.scp)\0*.scp\0";
    ofn.nFilterIndex = 1;

    if(GetOpenFileName(&ofn) == FALSE)
    {
        return FALSE;
    }

    HANDLE hAppend = CreateFile(szFileName,                   // open
                                GENERIC_READ,                 // open for writing 
                                0,                            // do not share 
                                NULL,                         // no security 
                                OPEN_EXISTING,                // open or create 
                                FILE_ATTRIBUTE_NORMAL,        // normal file 
                                NULL);                        // no attr. template 
    
    if (hAppend == INVALID_HANDLE_VALUE) 
    { 
        printf("Could not open file.\r\n");    // process error 
        return FALSE;
    } 
    
    DWORD dwFileLen = SetFilePointer(hAppend, 0, NULL, FILE_END);
    if (dwFileLen == 0)
    {
        return FALSE;
    }

    char * pFileBuf = new char[dwFileLen+1];    
    DWORD dwBytesWritten;
    SetFilePointer(hAppend, 0, NULL, FILE_BEGIN);
    ReadFile(hAppend, pFileBuf, dwFileLen, &dwBytesWritten, NULL);
    pFileBuf[dwFileLen] = '\0';
    int i = 0;
    int j = 0;
    do 
    {
        //�ҵ���\r\n������λ��
        for (; pFileBuf[j] != '\n'; j++)
        {
            if (j > dwFileLen)
            {
                break;
            }
        }

        stuCommand UserCmd = {0};
        if (pFileBuf[i] != ';')//��ע�ͲŴ���ע����䲻����
        {
            bRet = ChangeStrToCmd(&pFileBuf[i], &UserCmd);
            if ( bRet == FALSE)
            {
                return FALSE;
            }

            //���в�����
            if (stricmp(UserCmd.chCmd, "") == 0)
            {
                j++;
                i = j;
                continue;
            }

            //����������������������������Ӧ������
            CmdProcessFun pFun = GetFunFromAryCmd(UserCmd);
            if (pFun)
            {
                stuCommand *pCmd = new stuCommand;
                memcpy(pCmd, &UserCmd, sizeof(stuCommand));
                g_UserInputList.push_back(pCmd);
                bRet = (*pFun)(pCmd);          //��������
                if (bRet == TRUE)
                {
                    if (hAppend)
                    {
                        CloseHandle(hAppend);
                    }
                    return TRUE;
                }
            }
            else
            {
                printf("Error input!\r\n");
            }
        }
        j++;
        i = j;
    } while (i < dwFileLen);

    if (pFileBuf)
    {
        delete []pFileBuf;
        pFileBuf = NULL;
    }
    if (hAppend)
    {
        CloseHandle(hAppend);
    }
    return FALSE;
}

//�����ű�
BOOL CDoException::ExportScript(stuCommand *pCmd)
{
    //ѡ��Ҫ������ļ�
    char            szFileName[MAX_PATH] = "";	
    OPENFILENAME    ofn;
    
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize  = sizeof(OPENFILENAME);
    ofn.lpstrFile	 = szFileName;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = "script Files(*.scp)\0*.scp\0";
    ofn.nFilterIndex = 1;
    if( GetSaveFileName(&ofn) == FALSE)
    {
        return FALSE;
    }

    if (tolower(szFileName[strlen(szFileName)-1]) != 'p' &&
        tolower(szFileName[strlen(szFileName)-2]) != 'c' &&
        tolower(szFileName[strlen(szFileName)-3]) != 's' &&
        tolower(szFileName[strlen(szFileName)-4]) != '.')
    {
        strcat(szFileName, ".scp");
    }

    HANDLE hAppend = CreateFile(szFileName,                   // open
                                GENERIC_WRITE,                // open for writing 
                                0,                            // do not share 
                                NULL,                         // no security 
                                OPEN_ALWAYS,                  // open or create 
                                FILE_ATTRIBUTE_NORMAL,        // normal file 
                                NULL);                        // no attr. template 
    
    if (hAppend == INVALID_HANDLE_VALUE) 
    { 
        printf("Could not create file.\r\n");    // process error 
        return FALSE;
    } 

    list<stuCommand*>::iterator it = g_UserInputList.begin();
    for ( int i = 0; i < g_UserInputList.size(); i++ )
    {
        stuCommand *p = *it;

        DWORD dwPos = SetFilePointer(hAppend, 0, NULL, FILE_END);
        DWORD dwBytesWritten;
        char buff[50] = {0};
        wsprintf(buff, "%s %s %s %s\r\n", 
                 p->chCmd, p->chParam1, p->chParam2, p->chParam3);
        WriteFile(hAppend, buff, strlen(buff), &dwBytesWritten, NULL); 
        it++;
    }

    CloseHandle(hAppend);

    return FALSE;
}

//*************************************************************************
//*************************************************************************
//�����Ǵ����¼������ش���
//*************************************************************************
//*************************************************************************
stuCode* CDoException::AddInAvlTree(int nEip, char *pCodeBuf)
{
    stuCode stCode;
    stCode.m_nEip = nEip;
    memcpy(stCode.m_OpCode, pCodeBuf, 24);
    node<stuCode>* pNode = g_Avl_Tree.find_data(stCode);
    t_disasm da;
    
    //û���ҵ�
    if (pNode == NULL)
    {
        int nLen = Disasm(pCodeBuf, 24, 0, &da, DISASM_CODE, nEip);
        stCode.m_nCodeLen = nLen;
        strcpy(stCode.m_AsmCode, da.result);
        stCode.m_nID = ++m_nCount;
        stCode.m_nCount = 1;
        pNode = g_Avl_Tree.balance_sort_insert(stCode);
        return &pNode->data;
    }
    
    //�ҵ�
    pNode->data.m_nCount++;
    return &pNode->data;
}


void CDoException::ContinueRun(stuCode* pstuCode)
{
    UpdateContextFromThread();
    
    //������Ѿ������õ�API�������Ƶ�ָ�����Ȼ��CALL��API��ַ��ָ��
    if (pstuCode->m_nCount > 1 && pstuCode->m_chApiName[0] != 0)
    {
        //��������
        char buf[20] = {0};
        sprintf(buf, "%p", (int)m_Context.Eip + pstuCode->m_nCodeLen);
        strcpy(m_UserCmd.chParam1, buf);
        Run(&m_UserCmd);
        return;
    }
    
    //������Ѿ�ִ�й��Ĵ�����û�ж�Ӧ��API�������ƣ���Ӧ��������
    if (pstuCode->m_nCount > 1 && pstuCode->m_chApiName[0] == 0)
    {
        //��������
        StepInto(NULL);
        return;
    }
    
    //����Ϊ pstuCode->m_nCount == 1 �����
    //��ִ�е���ָ����һ����һ��ִ�е�����ָ��
    char chTemp[100] = {0};
    memcpy(chTemp, pstuCode->m_AsmCode, 4);
    
    if (stricmp(chTemp, "CALL") == 0)
    {
        strcpy(chTemp, pstuCode->m_AsmCode);
        char chApiName[100] = {0};
        
        //�ҵ���Ӧ��API
        if (GetFunctionName(chTemp, chApiName))
        {
            strcpy(pstuCode->m_chApiName, chApiName);
            //��������
            char buf[20] = {0};
            sprintf(buf, "%p", (int)m_Context.Eip + pstuCode->m_nCodeLen);
            strcpy(m_UserCmd.chParam1, buf);
            Run(&m_UserCmd);
        } 
        else
        {
            //��������
            StepInto(NULL);
        }
    }
    else
    {
        //��������
        StepInto(NULL);
    }
}

//����CALL�����ͣ������� call    dword ptr [eax+ebx*2+3]
//���Ĵ����ı����н�����ת��ΪCALL [��ֵ]
void CDoException::ParseCallCode(char *pResult)
{
    char* pAddr;
    BOOL isBackstair = FALSE;   //�Ƿ���CALL������Ϊ FALSE����ʾ���Ǽ��CALL
    //  char chSave[100] = {0};
    //  strcpy(chSave, pResult);
    int nLen = strlen(pResult);
    
    //�Ӻ���ǰ������ �ո� �ĵط�
    while (pResult[nLen-1] != ' ')
    {
        nLen--;
    }
    
    pAddr = &pResult[nLen];
    
    //�Ƿ���������[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        isBackstair = TRUE;
        //ȥ��������
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
    } 
    
    LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
    
    //dwAddr != 0������Ч��ַ�����账��
    if (dwAddr != 0)
    {
        //��ԭβ�������š�]��
        if (isBackstair)
        {
            pAddr[strlen(pAddr)] = ']';
        }
        return ;
    }
    
    //��Ч��ַ���ڲ��мĴ�������Ҫ����
    int nCallAddr = ExpressionToInt(pAddr);
    
    if (isBackstair == TRUE)
    {
        wsprintf(pResult, "[%p]", nCallAddr);
    } 
    else
    {
        wsprintf(pResult, "%p", nCallAddr);
    }
    
}

BOOL CDoException::GetFunctionName(char *pResult, char *pApiName)
{
    int nLen = strlen(pResult);
    char* pAddr;
    BOOL bRet;
    DWORD dwFunAddr = 0;
    
    //����CALL�����ͣ������� call    dword ptr [eax+ebx*2+3], call eax
    //���Ĵ����ı����н�����ת��ΪCALL [��ֵ], call ��ֵ
    ParseCallCode(pResult);
    
    //�Ӻ���ǰ������ �ո� �� ���� �ĵط�
    while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
    {
        nLen--;
    }
    pAddr = &pResult[nLen];
    
    //�Ƿ���������[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        //ȥ��������֮���Ƿ��ǺϷ��ĵ�ַ
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        
        if (dwAddr == 0)
        {
            return FALSE;
        }
        
        //����ַ��ȥȡ����
        bRet = ReadProcessMemory(m_hProcess, dwAddr, &dwFunAddr, 4, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error");
            return FALSE;
        }
    } 
    else
    {
        //ֱ���жϵ�ַ���Ƿ���API
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        if (dwAddr == 0)
        {
            return FALSE;
        }
        dwFunAddr = (DWORD)dwAddr;
    }
    
    //�ж� dwFunAddr �Ƿ���API��ַ
    //�����ж� dwFunAddr ���ĸ�ģ��ĵ�ַ
    BOOL isHit = FALSE;
    stuDllInfo* pDllInfo = NULL;
    list<stuDllInfo*>::iterator it = g_DllList.begin();
    for (int i = 0; i < g_DllList.size(); i++)
    {
        pDllInfo = *it;
        if (dwFunAddr > pDllInfo->dwDllAddr && 
            dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
        {
            isHit = TRUE;
            break;
        }
        it++;
    }
    
    //���û���ҵ���Ӧ��ģ�飬��ɾ����ģ�������е�ģ���¼
    //����ö��ģ�飬���²��Ҷ�Ӧģ��
    if (isHit == FALSE)
    {
        list<stuDllInfo*>::iterator itDll = g_DllList.begin();
        while (itDll != g_DllList.end())
        {
            stuDllInfo* pDll = *itDll;
            itDll++;
            delete pDll;
            g_DllList.remove(pDll);
        }
        
        EnumDestMod();
        
        it = g_DllList.begin();
        for (int i = 0; i < g_DllList.size(); i++)
        {
            pDllInfo = *it;
            if (dwFunAddr > pDllInfo->dwDllAddr && 
                dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
            {
                isHit = TRUE;
                break;
            }
            it++;
        }
        
    }
    
    if (isHit == FALSE)
    {
        return FALSE;
    }
    
    //���������Ƿ�����ĳ������
    char chFuncName[MAXBYTE] = {0};
    isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);
    
    if (isHit == TRUE)
    {
        strcpy(pApiName, chFuncName);
        //   printf("(%s)", chFuncName);
        return TRUE;
    }
    
    //���CALL���ĵ�ַ��һ����ת��JMP��CALL���ٽ������JMP��CALL�ĵ�ַ
    char   CodeBuf[20];
    t_disasm     da;
    int    nCodelen;
    
    bRet = ReadProcessMemory(m_hProcess, (LPVOID)dwFunAddr, CodeBuf, 24, NULL);
    
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return FALSE;
    }
    
    //����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
    if (CodeBuf[0] == 0xCC)
    {
        stuPointInfo tempPointInfo;
        stuPointInfo* pResultPointInfo = NULL;
        memset(&tempPointInfo, 0, sizeof(stuPointInfo));
        tempPointInfo.lpPointAddr = m_lpDisAsmAddr;
        tempPointInfo.ptType = ORD_POINT;
        
        if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
        {
            CodeBuf[0] = pResultPointInfo->u.chOldByte;
        }
    }
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, dwFunAddr);
    
    //����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
    char chCall[5] = {0};
    char chJmp[2] = {0};
    memcpy(chCall, da.result, 4);
    memcpy(chJmp, da.result, 1);
    if (stricmp(chCall, "CALL") == 0 || stricmp(chJmp, "J") == 0)
    {
        //   printf("(%s)", da.result);
        return GetFunctionName(da.result, pApiName);
    }
    return FALSE;
}

// �мĴ��������CALLָ����Ĵ������ʽת��Ϊ��ֵ
// ���� pAddr ����Ϊ����������ַ�����
// eax
// eax+3
// eax*4
// eax*4+ebx
// eax*8+1000
// eax+ebx+3000
// ebx+eax*2+F10000
int CDoException::ExpressionToInt(char *pAddr)
{
    char chNewBuf[30] = {0};
    int nRetValue = 0;
    
    //������û�� * ��
    BOOL isFindMultiplicationSign = FALSE;  //�Ƿ��ҵ��˺�
    BOOL isFindPlusSign = FALSE;   //�Ƿ��ҵ��Ӻ�
    int nLen = strlen(pAddr);
    
    int nMultiplicationPos;  //�ҵ��ĳ˺�λ���±�
    for ( nMultiplicationPos = 0; nMultiplicationPos < nLen; nMultiplicationPos++)
    {
        if (pAddr[nMultiplicationPos] == '*')
        {
            isFindMultiplicationSign = TRUE;
            break;
        }
    }
    
    if (isFindMultiplicationSign == TRUE)
    {
        //�ӳ˺���ǰ�ң�ֱ�������ӺŻ��ҵ�ͷ
        int nTemp = nMultiplicationPos;
        while (nTemp > 0 && pAddr[nTemp] != '+')
        {
            nTemp--;
        }
        //��ó˷��Ĳ�����1���ض���һ���Ĵ���
        char chOpNum1[5] = {0};
        if (nTemp != 0)
        {
            memcpy(chOpNum1, &pAddr[nTemp+1], nMultiplicationPos - nTemp -1);
        } 
        else
        {
            memcpy(chOpNum1, &pAddr[0], nMultiplicationPos);
        }
        int nOpNum1 = RegStringToInt(chOpNum1);
        
        //�ӳ˺������
        //��ó˷��Ĳ�����2���ض���2��4��8
        if (pAddr[nMultiplicationPos+1] == '2')
        {
            nRetValue += nOpNum1*2;
        } 
        else if(pAddr[nMultiplicationPos+1] == '4')
        {
            nRetValue += nOpNum1*4;
        } 
        else if(pAddr[nMultiplicationPos+1] == '8')
        {
            nRetValue += nOpNum1*8;
        }
        else
        {
            printf("invalid scale!\r\n");
            return 0;
        }
        
        //�� pAddr �ַ�����������
        if (nTemp != 0)
        {
            memcpy(&pAddr[nTemp], &pAddr[nMultiplicationPos+2], 20);
        } 
        else
        {
            memcpy(&pAddr[0], &pAddr[nMultiplicationPos+2], 20);
        }
        nLen = strlen(pAddr);
    }
    
    //�˷�������󣬱��ʽ�н�ֻ�С�+���ţ���û�з��ţ����ǿ��ַ���
    if (nLen == 0)
    {
        return nRetValue;
    }
    
    //�ҼӺ�
    int nPlusPos;  //��ǰ�����ҵ��ļӺ�λ���±�
    for ( nPlusPos = 0; nPlusPos < nLen; nPlusPos++)
    {
        if (pAddr[nPlusPos] == '+')
        {
            isFindPlusSign = TRUE;
            break;
        }
    }
    
    if (isFindPlusSign == TRUE)
    {
        //�ӷ�֮ǰ�ض���һ���Ĵ���
        char chPlusOpNum1[5] = {0};
        memcpy(chPlusOpNum1, &pAddr[0], 3);
        int nPlusOp1 = RegStringToInt(chPlusOpNum1);
        
        //�ӷ�֮�������һ���Ĵ��������������ж�һ���Ƿ���Eax�ȼĴ���
        if (pAddr[nPlusPos+3] == 'x' || pAddr[nPlusPos+3] == 'X' ||
            pAddr[nPlusPos+3] == 'i' || pAddr[nPlusPos+3] == 'I' ||
            pAddr[nPlusPos+3] == 'p' || pAddr[nPlusPos+3] == 'P')
        {
            //�ǼĴ���
            char chPlusOpNum2[5] = {0};
            memcpy(chPlusOpNum2, &pAddr[nPlusPos+1], 3);
            int nPlusOp2 = RegStringToInt(chPlusOpNum2);
            nRetValue += nPlusOp1 + nPlusOp2;
            //�� pAddr �ַ�����������
            if (nLen == 7)
            {
                return nRetValue;
            } 
            else
            {
                memcpy(&pAddr[0], &pAddr[8], 20);
                nLen = strlen(pAddr);
            }
        } 
        else
        {
            //����������˵�������һ��������
            int nPlusOp2 = (int)HexStringToHex(&pAddr[nPlusPos+1], FALSE);
            nRetValue += nPlusOp1 + nPlusOp2;
            return nRetValue;
        }
    }
    
    int nLast = (int)HexStringToHex(pAddr, FALSE);
    if (nLast == 0)
    {
        nLast = RegStringToInt(pAddr);
    }
    nRetValue += nLast;
    
    return nRetValue;
}

//�Ĵ����ַ���תΪ��ַ
int CDoException::RegStringToInt(char *chOpNum)
{
    if (stricmp(chOpNum, "eax") == 0)
    {
        //  return 0x1;
        return m_Context.Eax;
    } 
    else if (stricmp(chOpNum, "ebx") == 0)
    {
        //  return 0x10;
        return m_Context.Ebx;
    } 
    else if (stricmp(chOpNum, "ecx") == 0)
    {
        //  return 0x100;
        return m_Context.Ecx;
    } 
    else if (stricmp(chOpNum, "edx") == 0)
    {
        //  return 0x1000;
        return m_Context.Edx;
    } 
    else if (stricmp(chOpNum, "esi") == 0)
    {
        //  return 0x10000;
        return m_Context.Esi;
    } 
    else if (stricmp(chOpNum, "edi") == 0)
    {
        //  return 0x100000;
        return m_Context.Edi;
    } 
    else if (stricmp(chOpNum, "esp") == 0)
    {
        //  return 0x1000000;
        return m_Context.Esp;
    } 
    else if (stricmp(chOpNum, "ebp") == 0)
    {
        //  return 0x10000000;
        return m_Context.Ebp;
    }
    else
    {
        printf("Register error!\r\n");
        return 0;
    }
}

void CDoException::RecordCode(int nEip, char *pCodeBuf)
{
    stuCode* pstuCode = AddInAvlTree(nEip, pCodeBuf);
    DWORD  dwBytesWritten, dwPos; 
    char   buff[400]; 
    
    //�ó����������
    //������Ҫ���� pstuCode �ṹ���ж���Ҫ�������뻹�ǵ�������
    ContinueRun(pstuCode);
    
    //��ʾ����
    if (m_isShowCode == TRUE)
    {
        printf("%06d %06d %p    ", pstuCode->m_nID, pstuCode->m_nCount, 
            pstuCode->m_nEip);
        for (int i = 0; i < pstuCode->m_nCodeLen; i++)
        {
            printf("%s", &hexVale[pstuCode->m_OpCode[i]]);
        }
        printf("%s%s", "                          "+pstuCode->m_nCodeLen*2,
            pstuCode->m_AsmCode);
        if (pstuCode->m_chApiName[0] != 0)
        {
            printf("(%s)", pstuCode->m_chApiName);
        }
        printf("\r\n");
    }
    
    if (pstuCode->m_nCount == 1)
    {
        wsprintf(buff, "%06d %p    ", pstuCode->m_nID, pstuCode->m_nEip);
        
        for (int i = 0; i < pstuCode->m_nCodeLen; i++)
        {
            strcat(buff, hexVale[pstuCode->m_OpCode[i]]);
        }
        
        strcat(buff, "                          "+pstuCode->m_nCodeLen*2);
        strcat(buff, pstuCode->m_AsmCode);
        
        if (pstuCode->m_chApiName[0] != 0)
        {
            strcat(buff, "(");
            strcat(buff, pstuCode->m_chApiName);
            strcat(buff, ")\r\n");
        }
        else
        {
            strcat(buff, "\r\n");
        }
        
        dwPos = SetFilePointer(m_hAppend, 0, NULL, FILE_END); 
        WriteFile(m_hAppend, buff, strlen(buff), &dwBytesWritten, NULL); 
    }
}