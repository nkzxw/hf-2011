/////////////////////////////////////////
// GroupTalk.h


#include "initsock.h"


#define WM_GROUPTALK	WM_USER + 105    

/*

  uMsg:		WM_GROUPTALK
  wParam:	������룬0��ʾû�д���
  lParam:	��Ϣ��GT_HDRͷָ��

*/


#define BUFFER_SIZE 4096
#define GROUP_PORT  4567


const enum
{
	MT_JION = 1,	// һ���û�����
	MT_LEAVE,		// һ���û��뿪
	MT_MESG,		// һ���û�������Ϣ

	// �ڲ�ʹ�ã������¼�����û��Լ����û���Ϣ
	MT_MINE
};

typedef struct gt_hdr
{
	u_char gt_type;			// ��Ϣ����
	DWORD dwAddr;			// ���ʹ���Ϣ���û���IP��ַ
	char szUser[15];		// ���ʹ���Ϣ���û����û���

	int nDataLength;		// �������ݵĳ���
	char *data() { return (char*)(this + 1); }
} GT_HDR;



class CGroupTalk
{
public:
	// ���캯�������������̣߳����������
	CGroupTalk(HWND hNotifyWnd, DWORD dwMultiAddr, DWORD dwLocalAddr = INADDR_ANY, int nTTL = 64);
	// ����������������Դ���뿪������
	~CGroupTalk();

	// ��������Ա������Ϣ��dwRemoteAddrΪĿ���Ա�ĵ�ַ�����Ϊ0�������г�Ա����
	BOOL SendText(char *szText, int nLen, DWORD dwRemoteAddr = 0);

protected:
		// ��������
	// ����һ���ಥ��
	BOOL JoinGroup();
	// �뿪һ���ಥ��
	BOOL LeaveGroup();
	// ��ָ����ַ����UDP���
	BOOL Send(char *szText, int nLen, DWORD dwRemoteAddr);

protected:
		// ����ʵ��
	// �������ķ��
	void DispatchMsg(GT_HDR *pHeader, int nLen);
	// �����߳�
	friend DWORD WINAPI _GroupTalkEntry(LPVOID lpParam); 	CInitSock theSock;

	HWND m_hNotifyWnd;		// �����ھ��
	DWORD m_dwMultiAddr;	// ����ʹ�õĶಥ��ַ
	DWORD m_dwLocalAddr;	// �û�Ҫʹ�õı��ؽӿ�
	int m_nTTL;				// �ಥ�����TTLֵ
	HANDLE m_hThread;		// �����߳̾��
	HANDLE m_hEvent;		// �¼����������ʹ���ص�I/O��������

	SOCKET m_sRead;			// �������ݵ��׽��֣����������ಥ��
	SOCKET m_sSend;			// �������ݵ��׽��֣����ؼ���ಥ��

	BOOL m_bQuit;			// ����֪ͨ�����߳��˳�
	
	char m_szUser[256];		// �û���
};




/*

  ����ʹ������Ľṹ
#define MAX_EXPIRED 1000*60*5

struct USER_INFO
{
	DWORD dwAddr;
	char szUser[15];
	DWORD dwLastActiveTime;
};

*/