////////////////////////////////////
// comm.h�ļ�


#ifndef __COMM_H__
#define __COMM_H__



#define MAX_USERNAME 15
#define MAX_TRY_NUMBER 5
#define MAX_ADDR_NUMBER 5
#define MAX_PACKET_SIZE 1024
#define SERVER_PORT 4567

// һ���ն���Ϣ
struct ADDR_INFO
{
	DWORD dwIp;
	u_short nPort;
};

// һ���ڵ���Ϣ
struct PEER_INFO
{
	char szUserName[MAX_USERNAME];	 // �û���
	ADDR_INFO addr[MAX_ADDR_NUMBER]; // �ɽڵ��˽���ն˺͹����ն���ɵ�����
	u_char AddrNum;					 // addr����Ԫ������
	ADDR_INFO p2pAddr;				 // P2Pͨ��ʱӦʹ�õĵ�ַ���ͻ���ʹ�ã�
	DWORD dwLastActiveTime;			 // ��¼���û��Ļʱ�䣨������ʹ�ã�
};

// ͨ����Ϣ��ʽ
struct CP2PMessage
{
	int nMessageType;	// ��Ϣ����
	PEER_INFO peer;		// �ڵ���Ϣ
};


// �û�ֱ���������֮�䷢�͵���Ϣ
#define USERLOGIN	101		// �û���½������
#define USERLOGOUT	102		// �û��ǳ�������
#define USERLOGACK  103

#define GETUSERLIST	104		// �����û��б�
#define USERLISTCMP	105		// �б������

#define USERACTIVEQUERY	106			// ������ѯ��һ���û��Ƿ���Ȼ���
#define USERACTIVEQUERYACK	107		// ������ѯ��Ӧ��

// ͨ����������ת���û����û�֮�䷢�͵���Ϣ
#define P2PCONNECT	108			// ������һ���û���������
#define P2PCONNECTACK	109		// ����Ӧ�𣬴���Ϣ���ڴ�

// �û�ֱ�����û�֮�䷢�͵���Ϣ
#define P2PMESSAGE		110			// ������Ϣ
#define P2PMESSAGEACK	111			// �յ���Ϣ��Ӧ��


class CPeerList
{
public:
	CPeerList();
	~CPeerList();
	
	// ���б������һ���ڵ�
	BOOL AddAPeer(PEER_INFO *pPeer);
	// ����ָ���û�����Ӧ�Ľڵ�
	PEER_INFO *GetAPeer(char *pszUserName);
	// ���б���ɾ��һ���ڵ�
	void DeleteAPeer(char *pszUserName);
	// ɾ�����нڵ�
	void DeleteAllPeers();

	// ��ͷָ��ͱ�Ĵ�С
	PEER_INFO *m_pPeer;	
	int m_nCurrentSize;

protected:
	int m_nTatolSize;	
};


#endif // __COMM_H__





