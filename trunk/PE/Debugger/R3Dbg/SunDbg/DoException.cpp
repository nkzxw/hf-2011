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

list<stuCmdNode*> g_CmdList;                    //命令链表
list<stuDllInfo*> g_DllList;                    //模块信息列表
list<stuPointInfo*> g_ptList;                   //断点列表
list<stuPageInfo*>  g_PageList;                 //分页信息链表
list<stuPointPage*> g_PointPageList;            //内存断点分页对照表
list<stuCommand*> g_UserInputList;              //保存用户输入的合法命令的链表
list<stuResetMemBp*> g_ResetMemBp;              //需要恢复的内存断点

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
int              CDoException::m_nHardPtNum = 0;              //已设硬件断点数量
BOOL             CDoException::m_isStart = TRUE;
BOOL             CDoException::m_isNeedResetPoint = FALSE;
BOOL             CDoException::m_isNeedResetHardPoint = FALSE;

int              CDoException::m_nNeedResetHardPoint = -1;
BOOL             CDoException::m_isUserInputStep = FALSE;
int              CDoException::m_nCount = 0;
HANDLE           CDoException::m_hAppend = NULL;              //单步记录保存到的文件句柄
BOOL             CDoException::m_isStepRecordMode = FALSE;    //是否单步记录模式
BOOL             CDoException::m_isShowCode = FALSE;          //单步记录模式时是否需要在屏幕上显示代码
EXCEPTION_DEBUG_INFO CDoException::m_DbgInfo = {0};
stuPointInfo*    CDoException::m_pFindPoint = NULL;           //找到的断点指针
//list<stuPointInfo*>::iterator CDoException::m_itFind = NULL;  //找到的断点在链表中的迭代器位置
listStuPointInfo CDoException::m_itFind = NULL;

typedef HANDLE (__stdcall *OpenThreadFun)(
                                          DWORD dwDesiredAccess,  // access right
                                          BOOL bInheritHandle,    // handle inheritance option
                                          DWORD dwThreadId        // thread identifier
                                          );

//全局命令-函数对照表
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
                         {"", NULL}     //最后一个空项
               };

CDoException::CDoException()
{

}

CDoException::~CDoException()
{

}

int stuCode::operator==(const stuCode & c)
{
    //得到指令长度的较小值
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

//安全输入
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

//处理异常函数。返回值为 TRUE, 调试器处理了异常，程序的调试状态为继续。
//返回值为 FALSE，调试状态为调式器没有处理异常。
BOOL CDoException::DoException()
{
    m_DbgInfo = m_stuDbgEvent.u.Exception;

    //打开线程
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

    //处理异常的框架
    switch (m_DbgInfo.ExceptionRecord.ExceptionCode) 
    {
        //访问异常
    case EXCEPTION_ACCESS_VIOLATION:
        return DoAccessException();
        break;
        //int3异常
    case EXCEPTION_BREAKPOINT: 
        return DoInt3Exception();
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT: 
        break;
        //单步的处理
    case EXCEPTION_SINGLE_STEP:
        return DoStepException();
        break;
    case DBG_CONTROL_C: 
        break;
        // Handle other exceptions. 
    } 
    return TRUE;
}

//处理访问异常部分
BOOL CDoException::DoAccessException()
{
    BOOL                    bRet;
    DWORD                   dwAccessAddr;       //读写地址
    DWORD                   dwAccessFlag;       //读写标志
    BOOL                    isExceptionFromMemPoint = FALSE;    //异常是否由内存断点设置引起，默认为否
    stuPointInfo*           pPointInfo = NULL;                  //命中的断点
    BOOL                    isHitMemPoint = FALSE;

    dwAccessFlag = m_DbgInfo.ExceptionRecord.ExceptionInformation[0];
    dwAccessAddr = m_DbgInfo.ExceptionRecord.ExceptionInformation[1];
    //根据 访问地址 到“断点-分页表”中去查找
    //同一个内存分页可能有多个断点
    //如果没有在“断点-分页表”中查找到，则说明这个异常不是断点引起的
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    int nSize = g_PointPageList.size();

    //遍历链表中每个节点，将每个匹配的“断点-分页记录”都添加到g_ResetMemBp链表中
    for ( int i = 0; i < nSize; i++ )
    {
        stuPointPage* pPointPage = *it;
        //如果在“断点-分页表”中查找到
        //再根据断点表中信息判断是否符合用户所下断点信息
        if (pPointPage->dwPageAddr == (dwAccessAddr & 0xfffff000))
        {
            stuResetMemBp *p = new stuResetMemBp;
            p->dwAddr = pPointPage->dwPageAddr;
            p->nID = pPointPage->nPtNum;                
            g_ResetMemBp.push_back(p);
            
            //暂时恢复内存页原来的属性
            BOOL bDoOnce = FALSE;
            if (!bDoOnce)
            {
                //这些操作只需要执行一次
                bDoOnce = TRUE;
                isExceptionFromMemPoint = TRUE;
                TempResumePageProp(pPointPage->dwPageAddr);
                //设置单步，在单步中将断点设回
                UpdateContextFromThread();
                m_Context.EFlags |= TF;
                UpdateContextToThread();
            }
            
            //先找到断点序号对应的断点
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
            
            //再判断是否符合用户所下断点信息，断点类型和断点范围均相符
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
    
    //如果异常不是由内存断点设置引起，则调试器不处理
    if (isExceptionFromMemPoint == FALSE)
    {
        return FALSE;
    }
    
    //如果命中内存断点，则暂停，显示相关信息并等待用户输入
    if (isHitMemPoint)
    {
        ShowBreakPointInfo(pPointInfo);
        //显示反汇编代码
        m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
        ShowAsmCode();
        ShowRegValue(NULL);
        
        //等待用户输入
        bRet = FALSE;
        while (bRet == FALSE)
        {
            bRet = WaitForUserInput();
        }
    }
    return TRUE;
}

//处理INT3异常
BOOL CDoException::DoInt3Exception()
{
    BOOL                    bRet;
    stuPointInfo            tempPointInfo;
    stuPointInfo*           pResultPointInfo = NULL;
    char                    CodeBuf[24] = {0};

    //先过掉系统的INT3断点
    if (m_DbgInfo.ExceptionRecord.ExceptionAddress > (LPVOID)0x10000000
        && m_isStart == TRUE)
    {
        return TRUE;
    }
    
    //程序刚启动，停在OEP处
    if(m_DbgInfo.ExceptionRecord.ExceptionAddress == m_lpOepAddr
        && m_isStart == TRUE)
    {
        m_isStart = FALSE;
        //枚举目标进程的模块，并写入 g_DllList
        EnumDestMod();
    }
    
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    tempPointInfo.ptType = ORD_POINT;
    
    if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
    {
        //没有找到对应断点，则说明不是用户下的断定
        //返回FALSE，调试器不处理，交给系统继续分派异常
        return FALSE;   
    }
    else    //找到了断点
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
        //获取环境
        UpdateContextFromThread();
        m_Context.Eip--;
        m_Eip = (LPVOID)m_Context.Eip;
        
        if (m_pFindPoint->isOnlyOne == TRUE)    //是一次性断点
        {
            delete m_pFindPoint;
            g_ptList.erase(m_itFind);
        } 
        else //不是一次性断点，需要设置单步，到单步中去重设断点
        {
            //设置单步
            m_Context.EFlags |= TF;
            m_isNeedResetPoint = TRUE;
        }
    }
    
    //恢复环境
    UpdateContextToThread();
    
    //是否是单步记录模式
    if (m_isStepRecordMode == TRUE)
    {
        bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error!");
            return FALSE;
        }
        //记录指令
        RecordCode(m_Context.Eip, CodeBuf);
        return TRUE;
    }
    
    //显示反汇编代码
    m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    ShowAsmCode();
    ShowRegValue(NULL);
    
    //等待用户输入
    bRet = FALSE;
    while (bRet == FALSE)
    {
        bRet = WaitForUserInput();
    }
    return TRUE;
}

//处理单步异常
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
        
    //需要重设INT3断点
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

    //需要重设硬件断点
    if (m_isNeedResetHardPoint == TRUE)
    {
        m_Context.Dr7 |= (int)pow(4.0, m_nNeedResetHardPoint);
        UpdateContextToThread();
        m_isNeedResetHardPoint = FALSE;
    }

    dwDr6 = m_Context.Dr6;
    dwDr6Low = dwDr6 & 0xf; //取低4位

    //如果是由硬件断点触发的单步，需要用户输入才能继续
    //另外，如果是硬件执行断点，则需要先暂时取消断点，设置单步，下次再恢复断点
    if (dwDr6Low != 0)
    {
        ShowHardwareBreakpoint(dwDr6Low);
        m_nNeedResetHardPoint = log((long double)dwDr6Low)/log(2.0)+0.5;//加0.5是为了四舍五入
        //判断由 dwDr6Low 指定的DRX寄存器，是否是执行断点
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

        m_isUserInputStep = TRUE; //这个设置只是为了能够等待用户输入
    }

    if (m_isUserInputStep == FALSE)
    {
        //重设内存断点
        ResetMemBp();
        return TRUE;
    }

    //以下代码在用户输入为 "T" 命令、或硬件断点触发时执行
    //如果此处有INT3断点，则需要先暂时删除INT3断点
    //这样做是为了在用户输入“T”命令、或硬件断点触发时忽略掉INT3断点
    //以免在一个地方停下两次
    memset(&tempPointInfo, 0, sizeof(stuPointInfo));
    tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
    tempPointInfo.ptType = ORD_POINT;

    if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
    {
        //非一次性断点，才需要重设断点
        if (pResultPointInfo->isOnlyOne == FALSE)
        {
            m_Context.EFlags |= TF;
            UpdateContextToThread();
            m_isNeedResetPoint = TRUE;
        }
        else//一次性断点，从链表里面删除
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

    //更新m_Context为现在的环境值
    UpdateContextFromThread();

    if (m_isStepRecordMode == FALSE)
    {
        ShowAsmCode();
        ShowRegValue(NULL);
    }
    
    //重设内存断点
    ResetMemBp();

   //是否是单步记录模式
   if (m_isStepRecordMode == TRUE)
    {
        bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error!\r\n");
            return FALSE;
        }
        //记录指令
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


//根据g_ResetMemBp重设内存断点，在单步中调用
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

// 等待用户输入命令
// 并做相应的处理
// 用户输入的命令有两种类型
// 1. 控制程序流程的命令。如单步步入(T)、步过(P)，运行(G)等
// 2. 断点操作。如设置(bp)、查看(bl)、取消(bc)各种断点；
//    查看反汇编代码(U)、数据(D)、寄存器(R)等。
// 当用户输入的是T、P、G时，函数返回值为TRUE，表示程序需要往下运行（单步或运行）。
// 为其他输入时，函数返回值为FALSE，表示其他的操作或错误操作。
BOOL CDoException::WaitForUserInput()
{
    printf("COMMAND:");
    char chUserInputString[41] = {0};
    GetSafeStr(chUserInputString, 40);

    memset(&m_UserCmd, 0, sizeof(stuCommand));

    //用户输入的字符串转换为命令结构体
    BOOL bRet = ChangeStrToCmd(chUserInputString, &m_UserCmd);

    if ( bRet == FALSE)
    {
        return FALSE;
    }

    //根据输入的命令，查命令链表，调用相应处理函数
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
    int j = 0;  // 代表的是 stuCommand 结构体中第几个字段
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

// 从 lpDisAsmAddr 所指定的位置开始进行反汇编
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

    //查看要反汇编代码的地址所在的内存分页是否已经有内存断点
    //如果有，先修改内存属性页为可读，读完之后再改为不可访问
    //注意，所读内存可能跨分页
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

    //读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
    //还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
    //所以先恢复 SecondPage，后恢复 FirstPage
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

    //如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
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
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//调用反汇编引擎

    //对于JMP 和 CALL 指令需要修正地址， CALL 后面要换成 模块名 + 函数名
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

    // lpDisAsmAddr 地址要向后移动
    m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);
    
    bRet = FALSE;
}

//显示多行反汇编代码函数
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

//显示数据函数
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

    //调用显示数据函数
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
    
    //查看要反汇编代码的地址所在的内存分页是否已经有内存断点
    //如果有，先修改内存属性页为可读，读完之后再改为不可访问
    //注意，所读内存可能跨分页
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
    
    //读完之后重设断点，这里要注意，可能 m_lpShowDataAddr 和 m_lpShowDataAddr+80
    //还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
    //所以先恢复 SecondPage，后恢复 FirstPage
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
            //将INT3的CC还原成原字符
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

//显示寄存器函数
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

//设置一般断点
BOOL CDoException::SetOrdPoint(stuCommand* pCmd)
{
    BOOL    bRet;
    LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

    if (lpAddr == 0)
    {
        printf("Need valid parameter!\r\n");
        return FALSE;
    }

    //在断点列表中查找是否已经存在此处的一般断点
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
        if (tempPointInfo.isOnlyOne == FALSE)//设置的是非一次性断点
        {
            if (pResultPointInfo->isOnlyOne == FALSE)//查找到的是非一次性断点
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

//一般断点列表
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

//一般断点清除
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

            //如果删除的是需要恢复的INT3断点，则将恢复表示置为 FALSE
            if (m_isNeedResetPoint == TRUE && m_pFindPoint->nPtNum == nID)
            {
                m_isNeedResetPoint = FALSE;
            }

            delete [] pPointInfo;
            g_ptList.erase(it);
            //删除，提示
            printf("Clear the %d Ordinary breakpoint.\r\n", nID);
            return FALSE;
        }
        it++;
    }
    //没有找到，提示
    printf("Can not find the Ordinary breakpoint!\r\n");
    return FALSE;
}

//设置硬件断点
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
    int nDrNum = -1;                 // 硬件断点是在下标为nDrNum的 DRX 寄存器上
    int nPointLen = -1;

    //如果硬件断点数量>0
    if (m_nHardPtNum > 0)
    {
        if(FindPointInConext(PointInfo, &nDrNum, &nPointLen))
        {
            //找到了，需要比较一下找到的断点的字节长度是否比要设置的断点字节长度长
            if (nPointLen >= nLen)
            {
                //如果找到的断点字节长度大于需要设置的新断点，
                //则不需要重新设置，直接返回
                printf("The Hardware breakpoint is exist!\r\n");
                return FALSE;
            }
            else //否则硬件断点需要重新设置，但硬件断点数量维持不变，所以这里先将数量减1
            {
                m_nHardPtNum--;
            }
        }
    }

    //没有找到 或 需要重新设置
    if (nDrNum == -1) //没有找到，则需要先找一个空闲的调试寄存器
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

    //根据 nDrNum 设置硬件断点
    switch (nDrNum)
    {
    case 0:
        m_Context.Dr0 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 1;
        m_Context.Dr7 &= 0xfff0ffff;//清掉第16，17，18，19位（置0）
        //LEN 长度
        switch (PointInfo.dwPointLen)
        {
        case 0:
        	break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x00040000;//18位 置1
            break;
        case 4:
            m_Context.Dr7 |= 0x000c0000;//18位 置1, 19位 置1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x00010000;//16位 置1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x00030000;//16,17位 置1
            break;
        }
    	break;
    case 1:
        m_Context.Dr1 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 4;
        m_Context.Dr7 &= 0xff0fffff;//清掉第20，21，22，23位（置0）
        //LEN 长度
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x00400000;//22位 置1
            break;
        case 4:
            m_Context.Dr7 |= 0x00c00000;//22位 置1, 23位 置1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x00100000;//20位 置1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x00300000;//20,21位 置1
            break;
        }
        break;
    case 2:
        m_Context.Dr2 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 0x10;
        m_Context.Dr7 &= 0xf0ffffff;//清掉第24，25，26，27位（置0）
        //LEN 长度
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x04000000;//26位 置1
            break;
        case 4:
            m_Context.Dr7 |= 0x0c000000;//26位 置1, 27位 置1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x01000000;//24位 置1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x03000000;//24,25位 置1
            break;
        }
        break;
    case 3:
        m_Context.Dr3 = (DWORD)PointInfo.lpPointAddr;
        m_Context.Dr7 |= 0x40;
        m_Context.Dr7 &= 0x0fffffff;//清掉第28，29，30，31位（置0）
        //LEN 长度
        switch (PointInfo.dwPointLen)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            m_Context.Dr7 |= 0x40000000;//30位 置1
            break;
        case 4:
            m_Context.Dr7 |= 0xc0000000;//30位 置1, 31位 置1
            break;
        }
        switch (PointInfo.ptAccess)
        {
        case EXECUTE:
            break;
        case WRITE:
            m_Context.Dr7 |= 0x10000000;//28位 置1
            break;
        case ACCESS:
            m_Context.Dr7 |= 0x30000000;//28,29位 置1
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

//硬件断点列表
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

//硬件断点清除
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

    //如果要重设的硬件断点序号 m_nNeedResetHardPoint 等于要删除的硬件断点序号，
    //则硬件断点不需重设
    if (nID == m_nNeedResetHardPoint)
    {
        m_isNeedResetHardPoint = FALSE;
    }

    UpdateContextToThread();
    m_nHardPtNum--;
    printf("Clear the %d Hardware breakpoint.\r\n", nID+1);
    return FALSE;
}

//设置内存断点
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
        if (pResultPointInfo->dwPointLen >= nLen)//存在同样类型且长度大于要设置断点的断点
        {
            printf("The Memory breakpoint is already exist!\r\n");
            return FALSE;
        } 
        else//查找到的断点长度小于要设置的断点长度，则删除掉找到的断点，重新设置
            //只删除断点-分页表项 和 断点表项
        {
            DeletePointInList(pResultPointInfo->nPtNum, FALSE);
        }
    }
    
    // 根据 tempPointInfo 设置内存断点
    // 添加断点链表项，添加内存断点-分页表中记录，添加分页信息表记录

    // 首先根据 tempPointInfo 中的地址和长度获得所跨越的全部分页

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
            //将内存分页信息添加到分页表中
            AddRecordInPageList(mbi.BaseAddress, 
                                mbi.RegionSize, 
                                mbi.AllocationProtect);
            //将断点-分页信息添加到断点-分页表中
            DWORD dwPageAddr = (DWORD)mbi.BaseAddress;
            while (dwPageAddr < OutAddr)
            {
                stuPointPage *pPointPage = new stuPointPage;
                pPointPage->dwPageAddr = dwPageAddr;
                pPointPage->nPtNum = tempPointInfo.nPtNum;
                g_PointPageList.push_back(pPointPage);
                //设置该内存页为不可访问
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

    //断点添加到断点信息表中
    stuPointInfo *pPoint = new stuPointInfo;
    memcpy(pPoint, &tempPointInfo, sizeof(stuPointInfo));
    g_ptList.push_back(pPoint);
    printf("***Set Memory breakpoint success!***\r\n");

    return FALSE;
}

//内存断点列表
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

//内存断点清除
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
        //如果要删除的内存断点正好是要恢复的内存断点，则不需要再恢复内存断点
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

//显示帮助
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
   * 序号 命令名      命令码 英文说明        参数1    参数2    参数3        *\r\n\
   * 1    单步步入      T    step into                                      *\r\n\
   * 2    单步步过      P    step over                                      *\r\n\
   * 3    运行          G    run             地址或无                       *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 4    反汇编        U    assemble        地址或无                       *\r\n\
   * 5    数据          D    data            地址或无                       *\r\n\
   * 6    寄存器        R    register                                       *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 7    一般断点      bp   breakpoint      地址    [once](一次性)         *\r\n\
   * 8    一般断点列表  bpl  bp list                                        *\r\n\
   * 9    删除一般断点  bpc  clear bp        序号                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 10   硬件断点      bh 　hard bp         地址 execute/access/write 长度 *\r\n\
   * 11   硬件断点列表  bhl  hard bp list                                   *\r\n\
   * 12   删除硬件断点  bhc  clear hard bp   序号                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 13   内存断点      bm   memory bp       起始地址 access/write 长度     *\r\n\
   * 14   内存断点列表  bml  memory bp list                                 *\r\n\
   * 15   删除内存断点  bmc  clear memory bp 序号                           *\r\n\
   *------------------------------------------------------------------------*\r\n\
   * 16   导入脚本      ls   load script                                    *\r\n\
   * 17   导出脚本      es   export script                                  *\r\n\
   * 18   单步记录      sr   step record                                    *\r\n\
   * 19   帮助          h    help                                           *\r\n");
    printf("   **************************************************************************\r\n");
    return FALSE;
}

//单步步入
BOOL CDoException::StepInto(stuCommand* pCmd)
{
    UpdateContextFromThread();
    
    //设置单步
    m_Context.EFlags |= TF;
    m_isUserInputStep = TRUE;
        
    UpdateContextToThread();    
    return TRUE;
}

//单步步过
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
    
    //查看要反汇编代码的地址所在的内存分页是否已经有内存断点
    //如果有，先修改内存属性页为可读，读完之后再改为原先属性
    //注意，所读内存可能跨分页
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
    
    //读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
    //还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
    //所以先恢复 SecondPage，后恢复 FirstPage
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
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//调用反汇编引擎
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

//运行
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

//16进制字符串转换为数值
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


//在断点列表中查找是否有匹配PointInfo断点信息的断点。
//只匹配 断点起始地址，断点类型，断点访问类型。
//参数isNeedSave，表示是否需要保存
//返回值为TRUE表示找到；FALSE表示没有找到。
//找到的断点指针，放入 ppResultPointInfo 参数中
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

//在CONTEXT中查找是否已经存在 PointInfo 指定的硬件断点
//返回TRUE表示找到，FALSE表示未找到
//参数nDrNum返回找到的DRX寄存器的下标，参数nPointLen返回找到的断点长度
BOOL CDoException::FindPointInConext(stuPointInfo PointInfo, 
                                     int *nDrNum, int *nPointLen)
{
    if((m_Context.Dr7 & 1) && m_Context.Dr0 == (DWORD)PointInfo.lpPointAddr)
    {
        int nAccess = (m_Context.Dr7 << 14) >> 30;  //断点触发条件
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

//将线程信息更新到 M_Context 变量中
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

//将 M_Context 中的环境更新到线程中
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

// 根据内存断点的序号，删除断点-分页表项 和 断点表
// 参数 isNeedResetProtect，说明是否要恢复内存页原来的属性
BOOL CDoException::DeletePointInList(int nPtNum, BOOL isNeedResetProtect)
{
    //先根据序号找到对应断点
    list<stuPointInfo*>::iterator it = g_ptList.begin();
    
    for ( int i = 0; i < g_ptList.size(); i++ )
    {
        stuPointInfo* pPointInfo = *it;
        if (pPointInfo->nPtNum == nPtNum)//找到了
        {
            //先判断是不是内存断点，如果不是就是错误的
            if (pPointInfo->ptType != MEM_POINT)
            {
                printf("Only allow memony breakpoint reach here!\r\n");
                return FALSE;
            }

            //是内存断点，要删除断点表项 和 断点-分页表项
            delete [] pPointInfo;
            g_ptList.erase(it);

            //删除断点-分页表项
            DeleteRecordInPointPageList(nPtNum, isNeedResetProtect);

            return TRUE;
        }
        it++;
    }
    return FALSE;
}

//将内存分页信息添加到分页表中，如果和已有的记录重复，则不添加
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


//查找分页信息表中的记录
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

//删除分页信息表中的记录（这个函数用不上）
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

//从断点-分页表中删除断点序号为 nPtNum 的记录。
//参数 isNeedResetProtect ，指定是否需要恢复分页原来的属性。
void CDoException::DeleteRecordInPointPageList(int nPtNum, BOOL isNeedResumeProtect)
{
    list<stuPointPage*>::iterator it = g_PointPageList.begin();
    
    //一个断点可能对应多个分页，所以要遍历链表
    while ( it != g_PointPageList.end() )
    {
        stuPointPage* pPointPage = *it;
        it++;
        if (pPointPage->nPtNum == nPtNum)
        {
            //恢复内存页原来属性
            if (isNeedResumeProtect)
            {
                //如果找到另外一个内存断点也在这个分页上，则不需要恢复内存页属性
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

//临时恢复内存属性页的属性
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

//在断点-分页表中查找记录
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
            printf("error!");//不会有这种情况
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

    //从后往前读到有 空格 或 逗号 的地方
    while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
    {
        nLen--;
    }
    pAddr = &pResult[nLen];

    //是否有中括号[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        //去除中括号之后是否是合法的地址
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);

        if (dwAddr == 0)
        {
            return FALSE;
        }
        
        //到地址中去取内容
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
        //直接判断地址，是否是API
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        if (dwAddr == 0)
        {
            return FALSE;
        }
        dwFunAddr = (DWORD)dwAddr;
    }

    //判断 dwFunAddr 是否是API地址
    //首先判断 dwFunAddr 是哪个模块的地址
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

    //如果没有找到对应的模块，则删除掉模块链表中的模块记录
    //重新枚举模块，重新查找对应模块
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

    //读导出表看是否命中某个函数
    char chFuncName[MAXBYTE] = {0};
    isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);

    if (isHit == TRUE)
    {
        printf("(%s)", chFuncName);
        return TRUE;
    }

    //如果CALL到的地址是一个跳转表JMP或CALL，再解析这个JMP、CALL的地址
    char            CodeBuf[20];
    t_disasm        da;
    int             nCodelen;
    BOOL            isNeedResetFirstPage = FALSE;
    BOOL            isNeedResetSecondPage = FALSE;
    DWORD           dwTempProtect1;
    DWORD           dwTempProtect2;
    DWORD           dwOldProtect;
    
    //查看要反汇编代码的地址所在的内存分页是否已经有内存断点
    //如果有，先修改内存属性页为可读，读完之后再改为不可访问
    //注意，所读内存可能跨分页
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
    
    //读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
    //还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
    //所以先恢复 SecondPage，后恢复 FirstPage
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
    
    //如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
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
    
    nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, dwFunAddr);//调用反汇编引擎
    
    //对于JMP 和 CALL 指令需要修正地址， CALL 后面要换成 模块名 + 函数名
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

//枚举模块
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

//查找函数名称
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

    //根据序号 i 找到函数序号对应的下标j   找到函数名
    int j = 0;
    WORD dwNameOrd = 0;
    BOOL isHaveName = FALSE;    //是否有函数名
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
    else //序号方式的函数
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

//释放链表资源
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

    list<stuCmdNode*>::iterator itCmd = g_CmdList.begin();                    //命令链表
    while ( g_CmdList.size() )
    {
        stuCmdNode* pCmd = *itCmd;
        itCmd++;
        delete pCmd;
        g_CmdList.remove(pCmd);
    }
  
    list<stuPageInfo*>::iterator itPage = g_PageList.begin();                 //分页信息链表
    while ( g_PageList.size() )
    {
        stuPageInfo* pPage = *itPage;
        itPage++;
        delete pPage;
        g_PageList.remove(pPage);
    }

    list<stuCommand*>::iterator itUser = g_UserInputList.begin();             //保存用户输入的合法命令的链表
    while ( g_UserInputList.size() )
    {
        stuCommand* pUser = *itUser;
        itUser++;
        delete pUser;
        g_UserInputList.remove(pUser);
    }
}

//查命令链表，找对应的处理函数指针
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

//单步记录功能
BOOL CDoException::StepRecord(stuCommand* pCmd)
{
    //使用单步记录功能，提示用户取消内存断点
    if ((MessageBox(NULL, "You should clear all Memory breakpoint, \
otherwise may be bring access mistake!\r\n\r\n      \
                            Do you want to continue?",
         "", MB_YESNO) == IDNO)
         )
    {
        return FALSE;
    }
    
    printf("please wait for a moment......\r\n");
    //选择要保存的文件
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

    //是否需要在屏幕上显示代码
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
    //记录指令
    RecordCode(m_Context.Eip, CodeBuf);
  
    return TRUE;
}

//载入脚本
BOOL CDoException::LoadScript(stuCommand *pCmd)
{
    //选择要载入的文件
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
        //找到“\r\n”结束位置
        for (; pFileBuf[j] != '\n'; j++)
        {
            if (j > dwFileLen)
            {
                break;
            }
        }

        stuCommand UserCmd = {0};
        if (pFileBuf[i] != ';')//非注释才处理，注释语句不处理
        {
            bRet = ChangeStrToCmd(&pFileBuf[i], &UserCmd);
            if ( bRet == FALSE)
            {
                return FALSE;
            }

            //空行不处理
            if (stricmp(UserCmd.chCmd, "") == 0)
            {
                j++;
                i = j;
                continue;
            }

            //根据输入的命令，查命令链表，调用相应处理函数
            CmdProcessFun pFun = GetFunFromAryCmd(UserCmd);
            if (pFun)
            {
                stuCommand *pCmd = new stuCommand;
                memcpy(pCmd, &UserCmd, sizeof(stuCommand));
                g_UserInputList.push_back(pCmd);
                bRet = (*pFun)(pCmd);          //函数调用
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

//导出脚本
BOOL CDoException::ExportScript(stuCommand *pCmd)
{
    //选择要保存的文件
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
//以下是代码记录功能相关代码
//*************************************************************************
//*************************************************************************
stuCode* CDoException::AddInAvlTree(int nEip, char *pCodeBuf)
{
    stuCode stCode;
    stCode.m_nEip = nEip;
    memcpy(stCode.m_OpCode, pCodeBuf, 24);
    node<stuCode>* pNode = g_Avl_Tree.find_data(stCode);
    t_disasm da;
    
    //没有找到
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
    
    //找到
    pNode->data.m_nCount++;
    return &pNode->data;
}


void CDoException::ContinueRun(stuCode* pstuCode)
{
    UpdateContextFromThread();
    
    //如果是已经解析得到API函数名称的指令则必然是CALL到API地址的指令
    if (pstuCode->m_nCount > 1 && pstuCode->m_chApiName[0] != 0)
    {
        //单步步过
        char buf[20] = {0};
        sprintf(buf, "%p", (int)m_Context.Eip + pstuCode->m_nCodeLen);
        strcpy(m_UserCmd.chParam1, buf);
        Run(&m_UserCmd);
        return;
    }
    
    //如果是已经执行过的代码且没有对应的API函数名称，则应单步步入
    if (pstuCode->m_nCount > 1 && pstuCode->m_chApiName[0] == 0)
    {
        //单步步入
        StepInto(NULL);
        return;
    }
    
    //以下为 pstuCode->m_nCount == 1 的情况
    //即执行到的指令是一条第一次执行到的新指令
    char chTemp[100] = {0};
    memcpy(chTemp, pstuCode->m_AsmCode, 4);
    
    if (stricmp(chTemp, "CALL") == 0)
    {
        strcpy(chTemp, pstuCode->m_AsmCode);
        char chApiName[100] = {0};
        
        //找到对应的API
        if (GetFunctionName(chTemp, chApiName))
        {
            strcpy(pstuCode->m_chApiName, chApiName);
            //单步步过
            char buf[20] = {0};
            sprintf(buf, "%p", (int)m_Context.Eip + pstuCode->m_nCodeLen);
            strcpy(m_UserCmd.chParam1, buf);
            Run(&m_UserCmd);
        } 
        else
        {
            //单步步入
            StepInto(NULL);
        }
    }
    else
    {
        //单步步入
        StepInto(NULL);
    }
}

//进行CALL语句解释，对形如 call    dword ptr [eax+ebx*2+3]
//带寄存器的表达进行解析，转化为CALL [数值]
void CDoException::ParseCallCode(char *pResult)
{
    char* pAddr;
    BOOL isBackstair = FALSE;   //是否间接CALL，先置为 FALSE，表示不是间接CALL
    //  char chSave[100] = {0};
    //  strcpy(chSave, pResult);
    int nLen = strlen(pResult);
    
    //从后往前读到有 空格 的地方
    while (pResult[nLen-1] != ' ')
    {
        nLen--;
    }
    
    pAddr = &pResult[nLen];
    
    //是否有中括号[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        isBackstair = TRUE;
        //去除中括号
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
    } 
    
    LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
    
    //dwAddr != 0，是有效地址，不需处理
    if (dwAddr != 0)
    {
        //还原尾部中括号“]”
        if (isBackstair)
        {
            pAddr[strlen(pAddr)] = ']';
        }
        return ;
    }
    
    //无效地址，内部有寄存器，需要处理
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
    
    //进行CALL语句解释，对形如 call    dword ptr [eax+ebx*2+3], call eax
    //带寄存器的表达进行解析，转化为CALL [数值], call 数值
    ParseCallCode(pResult);
    
    //从后往前读到有 空格 或 逗号 的地方
    while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
    {
        nLen--;
    }
    pAddr = &pResult[nLen];
    
    //是否有中括号[]
    if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
    {
        //去除中括号之后是否是合法的地址
        pAddr[strlen(pAddr)-1] = '\0';
        pAddr = &pAddr[1];
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        
        if (dwAddr == 0)
        {
            return FALSE;
        }
        
        //到地址中去取内容
        bRet = ReadProcessMemory(m_hProcess, dwAddr, &dwFunAddr, 4, NULL);
        if (bRet == FALSE)
        {
            printf("ReadProcessMemory error");
            return FALSE;
        }
    } 
    else
    {
        //直接判断地址，是否是API
        LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
        if (dwAddr == 0)
        {
            return FALSE;
        }
        dwFunAddr = (DWORD)dwAddr;
    }
    
    //判断 dwFunAddr 是否是API地址
    //首先判断 dwFunAddr 是哪个模块的地址
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
    
    //如果没有找到对应的模块，则删除掉模块链表中的模块记录
    //重新枚举模块，重新查找对应模块
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
    
    //读导出表看是否命中某个函数
    char chFuncName[MAXBYTE] = {0};
    isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);
    
    if (isHit == TRUE)
    {
        strcpy(pApiName, chFuncName);
        //   printf("(%s)", chFuncName);
        return TRUE;
    }
    
    //如果CALL到的地址是一个跳转表JMP或CALL，再解析这个JMP、CALL的地址
    char   CodeBuf[20];
    t_disasm     da;
    int    nCodelen;
    
    bRet = ReadProcessMemory(m_hProcess, (LPVOID)dwFunAddr, CodeBuf, 24, NULL);
    
    if (bRet == FALSE)
    {
        printf("ReadProcessMemory error!\r\n");
        return FALSE;
    }
    
    //如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
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
    
    //对于JMP 和 CALL 指令需要修正地址， CALL 后面要换成 模块名 + 函数名
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

// 有寄存器参与的CALL指令，将寄存器表达式转化为数值
// 参数 pAddr 可能为以下情况的字符串：
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
    
    //先找有没有 * 号
    BOOL isFindMultiplicationSign = FALSE;  //是否找到乘号
    BOOL isFindPlusSign = FALSE;   //是否找到加号
    int nLen = strlen(pAddr);
    
    int nMultiplicationPos;  //找到的乘号位置下标
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
        //从乘号向前找，直到遇到加号或找到头
        int nTemp = nMultiplicationPos;
        while (nTemp > 0 && pAddr[nTemp] != '+')
        {
            nTemp--;
        }
        //获得乘法的操作数1，必定是一个寄存器
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
        
        //从乘号向后找
        //获得乘法的操作数2，必定是2，4，8
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
        
        //对 pAddr 字符串进行重组
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
    
    //乘法处理完后，表达式中将只有“+”号，或没有符号，或是空字符串
    if (nLen == 0)
    {
        return nRetValue;
    }
    
    //找加号
    int nPlusPos;  //从前往后找到的加号位置下标
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
        //加法之前必定是一个寄存器
        char chPlusOpNum1[5] = {0};
        memcpy(chPlusOpNum1, &pAddr[0], 3);
        int nPlusOp1 = RegStringToInt(chPlusOpNum1);
        
        //加法之后可能是一个寄存器或立即数，判断一下是否是Eax等寄存器
        if (pAddr[nPlusPos+3] == 'x' || pAddr[nPlusPos+3] == 'X' ||
            pAddr[nPlusPos+3] == 'i' || pAddr[nPlusPos+3] == 'I' ||
            pAddr[nPlusPos+3] == 'p' || pAddr[nPlusPos+3] == 'P')
        {
            //是寄存器
            char chPlusOpNum2[5] = {0};
            memcpy(chPlusOpNum2, &pAddr[nPlusPos+1], 3);
            int nPlusOp2 = RegStringToInt(chPlusOpNum2);
            nRetValue += nPlusOp1 + nPlusOp2;
            //对 pAddr 字符串进行重组
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
            //是立即数，说明是最后一个操作数
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

//寄存器字符串转为地址
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
    
    //让程序继续运行
    //但是需要根据 pstuCode 结构体判断是要单步步入还是单步步过
    ContinueRun(pstuCode);
    
    //显示出来
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