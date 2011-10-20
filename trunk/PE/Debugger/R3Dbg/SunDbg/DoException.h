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

//�ϵ�����
enum PointType
{
    ORD_POINT = 1,          //һ��ϵ�
    HARD_POINT = 2,         //Ӳ���ϵ�
    MEM_POINT = 3           //�ڴ�ϵ�
};

enum PointAccess
{
    ACCESS = 1,             //����
    WRITE = 2,              //д��
    EXECUTE = 3             //ִ��
};

class stuCode
{
public:
    int     m_nID;                  //ָ��ִ��ʱ���±�ֵ
    int		m_nEip;				    //ָ���Ӧ��EIP
    int		m_nCount;				//ָ��ִ�еĴ���
    char    m_OpCode[24];           //ָ�������
    char    m_nCodeLen;             //ָ���
    char	m_AsmCode[100];		    //ָ������
    char    m_chApiName[100];       //�����CALLָ���CALL��API�������¼API����
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

//�������
typedef BOOL (__cdecl *CmdProcessFun)(stuCommand* cmd);

//���������еĽڵ�ṹ��
struct stuCmdNode
{
    char            chCmd[20];
    CmdProcessFun   pFun;
};

//�ϵ���Ϣ�ṹ��
struct stuPointInfo
{
    PointType   ptType;                 //�ϵ�����
    int         nPtNum;                 //�ϵ����
    LPVOID      lpPointAddr;            //�ϵ��ַ
    PointAccess ptAccess;               //����д��ִ������
    DWORD       dwPointLen;             //�ϵ㳤�ȣ����Ӳ���ϵ㡢�ڴ�ϵ㣩
    BOOL        isOnlyOne;              //�Ƿ�һ���Զϵ㣨���INT3�ϵ㣩
    int         nNewProtect;            //�µ��ڴ�ҳ����(����ڴ�ϵ�)
    union{
        char chOldByte;                 //ԭ�ȵ��ֽڣ����INT3�ϵ㣩
        int nOldProtect;                //ԭ�ȵ��ڴ�ҳ����(����ڴ�ϵ�)
        }u;
};

typedef list<stuPointInfo*>::iterator listStuPointInfo;

//�ڴ�ϵ��Ӧ�ڴ��ҳ�ṹ��
struct stuPointPage
{
    int         nPtNum;                 //�ϵ����
    DWORD       dwPageAddr;             //�ڴ��ҳ�׵�ַ
};

//��Ҫ�ָ������裩���ڴ�ϵ�ṹ��
struct stuResetMemBp
{
    DWORD dwAddr;
    int nID;
};

//��ҳ��Ϣ�ṹ��
struct stuPageInfo
{
    DWORD       dwPageAddr;             //�ڴ��ҳ�׵�ַ
    DWORD       dwOldProtect;           //��ҳԭ������
};

class CDoException  
{
public:
    static DEBUG_EVENT      m_stuDbgEvent;
    static HANDLE           m_hProcess; 
    static DWORD            m_ProcessId; 
    static HANDLE           m_hThread;
    static LPVOID           m_lpOepAddr;
    static LPVOID           m_lpDisAsmAddr;         //��������ʼ��ַ
    static LPVOID           m_lpShowDataAddr;       //��ʾ���ݵ���ʼ��ַ
    static LPVOID           m_Eip;                  //���Գ����EIPֵ
    static CONTEXT          m_Context;              //���Գ���Ļ���
    static int              m_nPtNum;               //�ϵ�����
    static int              m_nOrdPtFlag;           //�ϵ�����ֵ
    static int              m_nHardPtNum;           //Ӳ���ϵ�����������
    static BOOL             m_isStart;              //�����Ƿ�մ������̣�ͣ��OEP��
    static BOOL             m_isNeedResetPoint;     //�ڵ������Ƿ���Ҫ������ʱ��ȡ���Ķϵ�
    static BOOL             m_isNeedResetHardPoint; //�ڵ������Ƿ���Ҫ������ʱ��ȡ����Ӳ���ϵ�

//     static BOOL             m_isNeedResetPageProp1;  //�Ƿ���Ҫ�����ڴ�ҳ�����ԣ������ڴ�ϵ㣩֮һ
//     static int              m_nNeedResetMemPointID1; //��Ҫ������ڴ�ϵ�ID֮һ
//     static DWORD            m_NeedResetPageAddr1;    //��Ҫ�����ڴ�ϵ���ڴ�ҳ�׵�ַ֮һ
//     static BOOL             m_isNeedResetPageProp2;  //�Ƿ���Ҫ�����ڴ�ҳ�����ԣ������ڴ�ϵ㣩֮��
//     static int              m_nNeedResetMemPointID2; //��Ҫ������ڴ�ϵ�ID֮��
//     static DWORD            m_NeedResetPageAddr2;    //��Ҫ�����ڴ�ϵ���ڴ�ҳ�׵�ַ֮��
    
    static int              m_nNeedResetHardPoint;  //�ڵ�������Ҫ�����Ӳ���ϵ�Ĵ���
    static BOOL             m_isUserInputStep;      //�Ƿ����û�����T������ĵ���
    static stuPointInfo*    m_pFindPoint;           //�ҵ��Ķϵ�ָ��
	//static list<stuPointInfo*>::iterator m_itFind;  //�ҵ��Ķϵ��������еĵ�����λ��
	static listStuPointInfo        m_itFind;
    static char             m_chOEP;
    static stuCommand       m_UserCmd;              //�û�������ַ���ת���ɵ�����ṹ��

    static int              m_nCount;               //ָ��ִ��ʱ���±�ֵ������
    static HANDLE           m_hAppend;              //������¼���浽���ļ����
    static BOOL             m_isStepRecordMode;     //�Ƿ񵥲���¼ģʽ
    static BOOL             m_isShowCode;           //������¼ģʽʱ�Ƿ���Ҫ����Ļ����ʾ����

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
    //�����쳣����
    static BOOL DoException();
    //�ȴ��û�����ʹ����û����뺯��
    static BOOL WaitForUserInput();
    //���û��ַ���ת��Ϊ����ṹ�庯��
    static BOOL ChangeStrToCmd(
                        IN char* chUserInputString, 
                        OUT stuCommand* pUserCmd);
    //16�����ַ���ת��Ϊ��ֵ
    static LPVOID HexStringToHex(char*, BOOL isShowError);
    //ɾ���ַ���ǰ�ÿո�
    static char* DelFrontSpace(char *);
    //��ʾһ�з�������
    static void ShowAsmCode();

    //����Ϊ��Ӧ�û�����Ĵ�����
    //��ʾ���з������뺯��
    static BOOL ShowMulAsmCode(stuCommand* pCmd);
    //��ʾ���ݺ���
    static BOOL ShowData(stuCommand* pCmd);
    //��ʾ�Ĵ�������
    static BOOL ShowRegValue(stuCommand* pCmd);
    //����һ��ϵ㣬����isOnceOrdPoint��ʾ�Ƿ���һ���Զϵ�
    static BOOL SetOrdPoint(stuCommand* pCmd);
    //һ��ϵ��б�
    static BOOL ListOrdPoint(stuCommand* pCmd);
    //һ��ϵ����
    static BOOL ClearOrdPoint(stuCommand* pCmd);
    //����Ӳ���ϵ�
    static BOOL SetHardPoint(stuCommand* pCmd);
    //Ӳ���ϵ��б�
    static BOOL ListHardPoint(stuCommand* pCmd);
    //Ӳ���ϵ����
    static BOOL ClearHardPoint(stuCommand* pCmd);
    //�����ڴ�ϵ�
    static BOOL SetMemPoint(stuCommand* pCmd);
    //�ڴ�ϵ��б�
    static BOOL ListMemPoint(stuCommand* pCmd);
    //�ڴ�ϵ����
    static BOOL ClearMemPoint(stuCommand* pCmd);
    //��ʾ����
    static BOOL ShowHelp(stuCommand* pCmd);
    //��������
    static BOOL StepInto(stuCommand* pCmd);
    //��������
    static BOOL StepOver(stuCommand* pCmd);
    //����
    static BOOL Run(stuCommand* pCmd);

    //�ڶϵ��б��в���ĳ��ַ�Ƿ��Ѿ���ĳ���͵Ķϵ�,����isNeedSave����ʾ�Ƿ���Ҫ����
    static BOOL FindPointInList(LPVOID lpAddr, PointType ptType, BOOL isNeedSave);

    //�ڶϵ��б��в����� PointInfo ָ���Ķϵ�,����isNeedSave����ʾ�Ƿ���Ҫ����
    static BOOL FindPointInList(IN stuPointInfo PointInfo, 
                                OUT stuPointInfo** pResultPointInfo,
                                BOOL isNeedSave);
    //��CONTEXT�в����Ƿ��Ѿ����� PointInfo ָ����Ӳ���ϵ�
    //����TRUE��ʾ�ҵ���FALSE��ʾδ�ҵ�
    //����nDrNum�����ҵ���DRX�Ĵ������±꣬����nPointLen�����ҵ��Ķϵ㳤��
    static BOOL FindPointInConext(stuPointInfo PointInfo, int* nDrNum, int* nPointLen);

    //������ţ�ɾ���ϵ�
    static BOOL DeletePointInList(int nPtNum, BOOL isNeedResetProtect);
};

#endif // !defined(AFX_DOEXCEPTION_H__778EC16D_4E47_4FB9_A88D_DDC21D2C47C7__INCLUDED_)


