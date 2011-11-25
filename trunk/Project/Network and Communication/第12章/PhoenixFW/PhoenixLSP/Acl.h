///////////////////////////////////////////////
// Acl.h�ļ�





#include "../common/PMacRes.h"

#include "../common/TypeStruct.h"



// Ϊÿ���׽��ִ���һ��Session		check access list  CPCheckAccess
 
class CAcl
{
public:
	CAcl();
	~CAcl();

	// �׽��ֵĴ�����رգ���Ӧ��Session�Ĵ�����ر�
	void CheckSocket(SOCKET s, int af, int type, int protocol);
	void CheckCloseSocket(SOCKET s);
	void CheckBind(SOCKET s, const struct sockaddr *addr);

	// �鿴�Ự�Ƿ������Ự��Զ�̷�����Ϣ��
	int CheckAccept(SOCKET s, SOCKET sNew, sockaddr FAR *addr);
	int CheckConnect(SOCKET s, const struct sockaddr FAR *addr);

	int	CheckSendTo(SOCKET s, const SOCKADDR *pTo);
	int	CheckRecvFrom(SOCKET s, SOCKADDR *pFrom);

private:
	// ���ûỰ������
	void SetSession(SESSION *pSession, 
		USHORT usRemotePort, ULONG ulRemoteIP, UCHAR ucDirection);
	// ��Ӧ�ó���֪ͨһ���Ự
	void NotifySession(SESSION *pSession, int nCode);

	int GetAccessInfo(SESSION *pSession);
	int GetAccessFromWorkMode(SESSION *pSession);


	// �ڹ����ļ��в���ָ������Ĺ��˹���
	int FindRule(TCHAR *szAppName, int nStart);


	// ��������
	int CreateSession(SOCKET s, int nProtocol);

	void DeleteSession(SOCKET s);

	void InitializeSession(SESSION *pSession);

	int FindSession(SOCKET s);






	SESSION *m_pSession;
	int m_nSessionCount;
	int m_nSessionMaxCount;

	static BOOL QueryAccess();
	static BOOL IsLocalIP(DWORD dwIP);
};