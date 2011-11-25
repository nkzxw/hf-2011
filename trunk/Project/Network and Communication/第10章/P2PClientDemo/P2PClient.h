////////////////////////////////////////////////////////
// p2pclient.h�ļ�

#ifndef __P2PCLIENT_H__
#define __P2PCLIENT_H__

#include <windows.h>
#include "../comm.h"

class CP2PClient
{
public:
	CP2PClient();
	~CP2PClient();
	// ��ʼ������ĳ�Ա
	BOOL Init(USHORT usLocalPort = 0);

	// ��½���������ǳ�������
	BOOL Login(char *pszUserName, char *pszServerIp);
	void Logout();

	// ������������û��б������û��б��¼
	BOOL GetUserList();

	// ��һ���û�������Ϣ
	BOOL SendText(char *pszUserName, char *pszText, int nTextLen);

	// ���յ�����Ϣ���麯��
	virtual void OnRecv(char *pszUserName, char *pszData, int nDataLen) { }

	// �û��б�
	CPeerList m_PeerList;

protected:
	void HandleIO(char *pBuf, int nBufLen, sockaddr *addr, int nAddrLen);
	static DWORD WINAPI RecvThreadProc(LPVOID lpParam);

	CRITICAL_SECTION m_PeerListLock;	// ͬ�����û��б�ķ���

	SOCKET m_s;				// ����P2Pͨ�ŵ��׽��־��
	HANDLE m_hThread;		// �߳̾��	
	WSAOVERLAPPED m_ol;		// ���ڵȴ������¼����ص��ṹ

	PEER_INFO m_LocalPeer;	// ���û���Ϣ

	DWORD m_dwServerIp;		// ������IP��ַ

	BOOL m_bThreadExit;		// ����ָʾ�����߳��˳�

	BOOL m_bLogin;			// �Ƿ��½
	BOOL m_bUserlistCmp;	// �û��б��Ƿ������
	BOOL m_bMessageACK;		// �Ƿ���յ���ϢӦ��
};


#endif // __P2PCLIENT_H__