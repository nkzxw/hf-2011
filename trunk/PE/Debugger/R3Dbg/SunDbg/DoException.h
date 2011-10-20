// DoException.h: interface for the CDoException class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOEXCEPTION_H__778EC16D_4E47_4FB9_A88D_DDC21D2C47C7__INCLUDED_)
#define AFX_DOEXCEPTION_H__778EC16D_4E47_4FB9_A88D_DDC21D2C47C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define TF 0x100
#define FIELD_LEN 20
#define CMD_COUNT 20
#define ADD_COMMAND(str, memberFxn) \
    {str, memberFxn},

//断点类型
enum PointType
{
    ORD_POINT = 1,          //一般断点
    HARD_POINT = 2,         //硬件断点
    MEM_POINT = 3           //内存断点
};

enum PointAccess
{
    ACCESS = 1,             //访问
    WRITE = 2,              //写入
    EXECUTE = 3             //执行
};

class stuCode
{
public:
    int     m_nID;                  //指令执行时的下标值
    int		m_nEip;				    //指令对应的EIP
    int		m_nCount;				//指令执行的次数
    char    m_OpCode[24];           //指令机器码
    char    m_nCodeLen;             //指令长度
    char	m_AsmCode[100];		    //指令内容
    char    m_chApiName[100];       //如果是CALL指令，且CALL到API，这里记录API名称
public:
    stuCode()
    {
        m_nID = -1;
        m_nEip = -1;
        m_nCount = 1;
        m_nCount = 24;
        memset(m_AsmCode, 0, 100);
        memset(m_chApiName, 0, 100);
    }
    
    int operator==(const stuCode & c);
    int operator>(const stuCode & c);
    int operator<(const stuCode & c);
};

struct stuDllInfo
{
    DWORD dwDllAddr;
    DWORD dwModSize;
    char  szDllName[255];
};

struct stuCommand
{
    char chCmd[FIELD_LEN];
    char chParam1[FIELD_LEN];
    char chParam2[FIELD_LEN];
    char chParam3[FIELD_LEN];
    char chParam4[FIELD_LEN];
    char chParam5[FIELD_LEN];
    char chParam6[FIELD_LEN];
};

//命令处理函数
typedef BOOL (__cdecl *CmdProcessFun)(stuCommand* cmd);

//命令链表中的节点结构体
struct stuCmdNode
{
    char            chCmd[20];
    CmdProcessFun   pFun;
};

//断点信息结构体
struct stuPointInfo
{
    PointType   ptType;                 //断点类型
    int         nPtNum;                 //断点序号
    LPVOID      lpPointAddr;            //断点地址
    PointAccess ptAccess;               //读、写、执行属性
    DWORD       dwPointLen;             //断点长度（针对硬件断点、内存断点）
    BOOL        isOnlyOne;              //是否一次性断点（针对INT3断点）
    int         nNewProtect;            //新的内存页属性(针对内存断点)
    union{
        char chOldByte;                 //原先的字节（针对INT3断点）
        int nOldProtect;                //原先的内存页属性(针对内存断点)
        }u;
};

typedef list<stuPointInfo*>::iterator listStuPointInfo;

//内存断点对应内存分页结构体
struct stuPointPage
{
    int         nPtNum;                 //断点序号
    DWORD       dwPageAddr;             //内存分页首地址
};

//需要恢复（重设）的内存断点结构体
struct stuResetMemBp
{
    DWORD dwAddr;
    int nID;
};

//分页信息结构体
struct stuPageInfo
{
    DWORD       dwPageAddr;             //内存分页首地址
    DWORD       dwOldProtect;           //分页原有属性
};

class CDoException  
{
public:
    static DEBUG_EVENT      m_stuDbgEvent;
    static HANDLE           m_hProcess; 
    static DWORD            m_ProcessId; 
    static HANDLE           m_hThread;
    static LPVOID           m_lpOepAddr;
    static LPVOID           m_lpDisAsmAddr;         //反汇编的起始地址
    static LPVOID           m_lpShowDataAddr;       //显示数据的起始地址
    static LPVOID           m_Eip;                  //调试程序的EIP值
    static CONTEXT          m_Context;              //调试程序的环境
    static int              m_nPtNum;               //断点数量
    static int              m_nOrdPtFlag;           //断点的序号值
    static int              m_nHardPtNum;           //硬件断点已设置数量
    static BOOL             m_isStart;              //程序是否刚创建进程，停在OEP处
    static BOOL             m_isNeedResetPoint;     //在单步中是否需要重设临时被取消的断点
    static BOOL             m_isNeedResetHardPoint; //在单步中是否需要重设临时被取消的硬件断点

//     static BOOL             m_isNeedResetPageProp1;  //是否需要重设内存页的属性（重设内存断点）之一
//     static int              m_nNeedResetMemPointID1; //需要重设的内存断点ID之一
//     static DWORD            m_NeedResetPageAddr1;    //需要重设内存断点的内存页首地址之一
//     static BOOL             m_isNeedResetPageProp2;  //是否需要重设内存页的属性（重设内存断点）之二
//     static int              m_nNeedResetMemPointID2; //需要重设的内存断点ID之二
//     static DWORD            m_NeedResetPageAddr2;    //需要重设内存断点的内存页首地址之二
    
    static int              m_nNeedResetHardPoint;  //在单步中需要重设的硬件断点寄存器
    static BOOL             m_isUserInputStep;      //是否是用户输入T，进入的单步
    static stuPointInfo*    m_pFindPoint;           //找到的断点指针
	//static list<stuPointInfo*>::iterator m_itFind;  //找到的断点在链表中的迭代器位置
	static listStuPointInfo        m_itFind;
    static char             m_chOEP;
    static stuCommand       m_UserCmd;              //用户输入的字符串转换成的命令结构体

    static int              m_nCount;               //指令执行时的下标值计数器
    static HANDLE           m_hAppend;              //单步记录保存到的文件句柄
    static BOOL             m_isStepRecordMode;     //是否单步记录模式
    static BOOL             m_isShowCode;           //单步记录模式时是否需要在屏幕上显示代码

    static EXCEPTION_DEBUG_INFO m_DbgInfo;
public:
	static BOOL DoStepException();
	static BOOL DoInt3Exception();
	static BOOL DoAccessException();
	static void ResetMemBp();
    static void RecordCode(int nEip, char *pCodeBuf);
    static int RegStringToInt(char* chOpNum1);
    static int  ExpressionToInt(char* pAddr);
    static void ParseCallCode(char* chTemp);
    static void ContinueRun(stuCode* pstuCode);
    static stuCode* AddInAvlTree(IN int nEip, IN char* pCodeBuf);

    static BOOL ExportScript(stuCommand* pCmd);
	static BOOL LoadScript(stuCommand* pCmd);
	static BOOL StepRecord(stuCommand* pCmd);
	static CmdProcessFun GetFunFromAryCmd(stuCommand UserCmd);
	static void ReleaseResource();
	static BOOL FindFunction(DWORD dwFunAddr, DWORD dwDllAddr, char* pFunName);
	static void EnumDestMod();
	static BOOL ShowFunctionName(char *);
    static BOOL GetFunctionName(char *pResult, char *pApiName);
	static void ShowHardwareBreakpoint(DWORD dwDr6Low);
	static void ShowBreakPointInfo(stuPointInfo *pPoint);
	static BOOL FindRecordInPointPageList(DWORD dwPageAddr);
    static BOOL FindAnotherRecordInPointPageList(stuPointPage * pPointPage);
	static void TempResumePageProp(DWORD dwPageAddr);
    static BOOL FindRecordInPageList(DWORD dwBaseAddr, stuPageInfo** ppFind);
	static void DeleteRecordInPointPageList(int nPtNum, BOOL isNeedResumeProtect);
	static void DeleteRecordInPageList(DWORD dwBaseAddr);
	static void AddRecordInPageList(LPVOID BaseAddr, DWORD dwRegionSize, DWORD dwProtect);
	static BOOL UpdateContextToThread();
	static BOOL UpdateContextFromThread();
	CDoException();
	virtual ~CDoException();
    //处理异常函数
    static BOOL DoException();
    //等待用户输入和处理用户输入函数
    static BOOL WaitForUserInput();
    //将用户字符串转换为命令结构体函数
    static BOOL ChangeStrToCmd(
                        IN char* chUserInputString, 
                        OUT stuCommand* pUserCmd);
    //16进制字符串转换为数值
    static LPVOID HexStringToHex(char*, BOOL isShowError);
    //删除字符串前置空格
    static char* DelFrontSpace(char *);
    //显示一行反汇编代码
    static void ShowAsmCode();

    //以下为对应用户输入的处理函数
    //显示多行反汇编代码函数
    static BOOL ShowMulAsmCode(stuCommand* pCmd);
    //显示数据函数
    static BOOL ShowData(stuCommand* pCmd);
    //显示寄存器函数
    static BOOL ShowRegValue(stuCommand* pCmd);
    //设置一般断点，参数isOnceOrdPoint表示是否是一次性断点
    static BOOL SetOrdPoint(stuCommand* pCmd);
    //一般断点列表
    static BOOL ListOrdPoint(stuCommand* pCmd);
    //一般断点清除
    static BOOL ClearOrdPoint(stuCommand* pCmd);
    //设置硬件断点
    static BOOL SetHardPoint(stuCommand* pCmd);
    //硬件断点列表
    static BOOL ListHardPoint(stuCommand* pCmd);
    //硬件断点清除
    static BOOL ClearHardPoint(stuCommand* pCmd);
    //设置内存断点
    static BOOL SetMemPoint(stuCommand* pCmd);
    //内存断点列表
    static BOOL ListMemPoint(stuCommand* pCmd);
    //内存断点清除
    static BOOL ClearMemPoint(stuCommand* pCmd);
    //显示帮助
    static BOOL ShowHelp(stuCommand* pCmd);
    //单步步入
    static BOOL StepInto(stuCommand* pCmd);
    //单步步过
    static BOOL StepOver(stuCommand* pCmd);
    //运行
    static BOOL Run(stuCommand* pCmd);

    //在断点列表中查找某地址是否已经有某类型的断点,参数isNeedSave，表示是否需要保存
    static BOOL FindPointInList(LPVOID lpAddr, PointType ptType, BOOL isNeedSave);

    //在断点列表中查找由 PointInfo 指定的断点,参数isNeedSave，表示是否需要保存
    static BOOL FindPointInList(IN stuPointInfo PointInfo, 
                                OUT stuPointInfo** pResultPointInfo,
                                BOOL isNeedSave);
    //在CONTEXT中查找是否已经存在 PointInfo 指定的硬件断点
    //返回TRUE表示找到，FALSE表示未找到
    //参数nDrNum返回找到的DRX寄存器的下标，参数nPointLen返回找到的断点长度
    static BOOL FindPointInConext(stuPointInfo PointInfo, int* nDrNum, int* nPointLen);

    //根据序号，删除断点
    static BOOL DeletePointInList(int nPtNum, BOOL isNeedResetProtect);
};

#endif // !defined(AFX_DOEXCEPTION_H__778EC16D_4E47_4FB9_A88D_DDC21D2C47C7__INCLUDED_)


