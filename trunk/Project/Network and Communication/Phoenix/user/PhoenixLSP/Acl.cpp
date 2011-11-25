///////////////////////////////////////////////////
// Acl.cpp�ļ�


#define UNICODE
#define _UNICODE

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "../common/debug.h"

#include "Acl.h"


///////////////////////////////////////
// �����ڴ�

// ����ʹ��Winsock���������Ӧ�ó��򶼹���initdata��uinitdata�εı���
#pragma data_seg(".initdata")
HWND			g_hPhoenixWnd = NULL;			// �����ھ��
UCHAR			g_ucWorkMode = PF_PASS_ALL;		// ����ģʽ
#pragma data_seg()

#pragma bss_seg(".uinitdata") 
RULE_ITEM		g_Rule[MAX_RULE_COUNT];				  // Ӧ�ò����
ULONG			g_RuleCount;
QUERY_SESSION	g_QuerySession[MAX_QUERY_SESSION];	  // ���������ͻỰѯ��ʱʹ��
SESSION			g_SessionBuffer[MAX_SESSION_BUFFER];  // ���������ͻỰ��Ϣʱʹ��
TCHAR			g_szPhoenixFW[MAX_PATH];			  // ��¼������·��
#pragma bss_seg()


extern TCHAR	g_szCurrentApp[MAX_PATH];

CRITICAL_SECTION	g_csGetAccess;



CAcl::CAcl()
{
	m_nSessionCount = 0;
	// Ϊ�Ự�ṹԤ�����ڴ�ռ�
	m_nSessionMaxCount = INIT_SESSION_BUFFER;	
	m_pSession = new SESSION[m_nSessionMaxCount];

	::InitializeCriticalSection(&g_csGetAccess);
}

CAcl::~CAcl()
{
	ODS(L" CAcl::~CAcl send CODE_APP_EXIT ... ");
	// ֪ͨ��ģ�飬��ǰӦ�ó��������˳�
	int nIndex = CreateSession(0, 0);
	NotifySession(&m_pSession[nIndex], CODE_APP_EXIT);

	delete[] m_pSession;
	::DeleteCriticalSection(&g_csGetAccess);
}

/////////////////////////////////////////////////////////
// ��麯��

void CAcl::CheckSocket(SOCKET s, int af, int type, int protocol)
{	
	if (af != AF_INET) // ��֧��IPv4
		return;

	// ���жϻ���Э������

	int nProtocol = RULE_SERVICE_TYPE_ALL;

	if(protocol == 0)
	{
		if(type ==  SOCK_STREAM)
			nProtocol = RULE_SERVICE_TYPE_TCP;
		else if(type == SOCK_DGRAM)
			nProtocol = RULE_SERVICE_TYPE_UDP;
	}
	else if(protocol == IPPROTO_TCP)
		nProtocol = RULE_SERVICE_TYPE_TCP;
	else if(protocol == IPPROTO_UDP)
		nProtocol = RULE_SERVICE_TYPE_UDP;

	// Ϊ���׽��ִ����Ự��ָ��Э������
	CreateSession(s, nProtocol);
}

void CAcl::CheckCloseSocket(SOCKET s)
{
	// ɾ���Ự
	DeleteSession(s);
}

void CAcl::CheckBind(SOCKET s, const struct sockaddr *addr)
{
	int nIndex;
	if((nIndex = FindSession(s)) >= m_nSessionCount)
		return;

	// ���ûỰ
	sockaddr_in *pLocal = (sockaddr_in *)addr;
	m_pSession[nIndex].usLocalPort = ntohs(pLocal->sin_port);

	if(pLocal->sin_addr.S_un.S_addr != ADDR_ANY)
		m_pSession[nIndex].ulLocalIP = *((DWORD*)&pLocal->sin_addr);
}


int CAcl::CheckAccept(SOCKET s, SOCKET sNew, sockaddr FAR *addr)
{
	int nIndex;
	if((nIndex = FindSession(s)) >= m_nSessionCount)
		return PF_PASS;

	nIndex = CreateSession(sNew, RULE_SERVICE_TYPE_TCP);

	// ���ûỰ
	if(addr != NULL)
	{
		sockaddr_in *pRemote = (sockaddr_in *)addr;
		USHORT usPort = ntohs(pRemote->sin_port);
		DWORD dwIP = *((DWORD*)&pRemote->sin_addr);
		SetSession(&m_pSession[nIndex], usPort, dwIP, RULE_DIRECTION_IN_OUT);
	}

	return GetAccessInfo(&m_pSession[nIndex]);
}

int CAcl::CheckConnect(SOCKET s, const struct sockaddr FAR *addr)
{
	int nIndex;
	if((nIndex = FindSession(s)) >= m_nSessionCount)
		return PF_PASS;

	// ���ûỰԶ�̵�ַ
	sockaddr_in *pRemote = (sockaddr_in *)addr;
	USHORT usPort = ntohs(pRemote->sin_port);
	DWORD dwIP = *((DWORD*)&pRemote->sin_addr);
	SetSession(&m_pSession[nIndex], usPort, dwIP, RULE_DIRECTION_IN_OUT);

	return GetAccessInfo(&m_pSession[nIndex]);
}

int CAcl::CheckSendTo(SOCKET s, const SOCKADDR *pTo)
{
	int nIndex;
	if((nIndex = FindSession(s)) >= m_nSessionCount)
		return PF_PASS;

	if(pTo != NULL)
	{
		// ���ûỰԶ�̵�ַ
		sockaddr_in *pRemote = (sockaddr_in *)pTo;
		USHORT usPort = ntohs(pRemote->sin_port);
		DWORD dwIP = *((DWORD*)&pRemote->sin_addr);
		SetSession(&m_pSession[nIndex], usPort, dwIP, RULE_DIRECTION_OUT);
	}

	return GetAccessInfo(&m_pSession[nIndex]);
}

int CAcl::CheckRecvFrom(SOCKET s, SOCKADDR *pFrom)
{
	int nIndex;
	if((nIndex = FindSession(s)) >= m_nSessionCount)
		return PF_PASS;

	if(pFrom != NULL)
	{
		// ���ûỰԶ�̵�ַ
		sockaddr_in *pRemote = (sockaddr_in *)pFrom;
		USHORT usPort = ntohs(pRemote->sin_port);
		DWORD dwIP = *((DWORD*)&pRemote->sin_addr);
		SetSession(&m_pSession[nIndex], usPort, dwIP, RULE_DIRECTION_IN);
	}
	return GetAccessInfo(&m_pSession[nIndex]);
}


/////////////////////////////////////////////////////////////
// �鿴����Ȩ��


int CAcl::GetAccessInfo(SESSION *pSession)
{
	// �������ģ��������磬����
	if(wcsicmp(g_szCurrentApp, g_szPhoenixFW) == 0)
	{
		return PF_PASS;
	}
	
	// �Ȳ鿴����ģʽ
	int nRet;
	if((nRet = GetAccessFromWorkMode(pSession)) != PF_FILTER)
	{
		ODS(L" GetAccessInfo return from WorkMode \n");
		return nRet;
	}

	// ����ģʽΪ���ˣ������ļ��м�¼�Ĺ������֮
	::EnterCriticalSection(&g_csGetAccess);

	RULE_ITEM *pItem = NULL;
	int nIndex = 0;
	nRet = PF_PASS;
	while(TRUE)
	{
		// ������ǵ�һ�β�ѯ�����1�����������ͬ�Ĺ���
		if(nIndex > 0) 
			nIndex++;

		nIndex = FindRule(g_szCurrentApp, nIndex);
		if(nIndex >= (int)g_RuleCount)
		{
			if(pItem == NULL)	// һ����¼��Ҳû�У����ѯ
			{
				// ѯ����ģ����ô��
				if(!QueryAccess())
				{
					nRet = PF_DENY;
				}
				break;
			}
			else				// ������һ����¼���
			{
				if(pItem->ucAction != RULE_ACTION_PASS)
				{	
					nRet = PF_DENY;
				}
				break;
			}
		}
		
		ODS(L" Find a rule in GetAccessInfo ");
		// �鿴����ͻỰ�������Ƿ�һ��
		pItem = &g_Rule[nIndex];
		// ����
		if(pItem->ucDirection != RULE_DIRECTION_IN_OUT &&
			pItem->ucDirection != pSession->ucDirection)
			continue;
		// ��������
		if(pItem->ucServiceType != RULE_SERVICE_TYPE_ALL &&
			pItem->ucServiceType != pSession->nProtocol)
			continue;
		// ����˿�
		if(pItem->usServicePort != RULE_SERVICE_PORT_ALL &&
			 pItem->usServicePort != pSession->usRemotePort)
			 continue;
		// �������е����˵���ҵ���һ���ͻỰ������ȫ��ͬ�Ĺ���
		if(pItem->ucAction != RULE_ACTION_PASS)
		{	
			nRet = PF_DENY;
		}	
		break;
	}
	::LeaveCriticalSection(&g_csGetAccess);

	if(nRet == PF_PASS)
		pSession->ucAction = RULE_ACTION_PASS;
	else
		pSession->ucAction =  RULE_ACTION_DENY;
	return nRet;
}

int CAcl::GetAccessFromWorkMode(SESSION *pSession)
{
	if(g_ucWorkMode == PF_PASS_ALL)
		return PF_PASS;

	if(g_ucWorkMode == PF_DENY_ALL)
		return PF_DENY;

	if(g_ucWorkMode == PF_QUERY_ALL)
		return PF_FILTER;

	return PF_UNKNOWN;
}

int CAcl::FindRule(TCHAR *szAppName, int nStart)
{
	// ��ָ��λ�ÿ�ʼ���ң����ع��������
	int nIndex;
	for(nIndex = nStart; nIndex < (int)g_RuleCount; nIndex++)
	{
		if(wcsicmp(szAppName, g_Rule[nIndex].szApplication) == 0)
			break;
	}
	return nIndex;
}

BOOL CAcl::QueryAccess()
{
	ODS(L" QueryAccess ... ");

	// ������Ϣ
	for(int i=0; i<MAX_QUERY_SESSION; i++)
	{
		if(!g_QuerySession[i].bUsed) // �ҵ�һ��û��ʹ�õ�QuerySession������ѯ��
		{
			g_QuerySession[i].bUsed = TRUE;
			wcscpy(g_QuerySession[i].szPathName, g_szCurrentApp);

			if(!::PostMessage(g_hPhoenixWnd, PM_QUERY_ACL_NOTIFY, i, 0))
			{
				g_QuerySession[i].bUsed = FALSE;
				return TRUE;
			}
			// ѯ�ʷ��ͳɹ����ȴ�
			ODS(L"ѯ�ʷ��ͳɹ����ȴ�... ");

			int n=0;
			while(g_QuerySession[i].bUsed)
			{
				if(n++ > 3000)		// ��5���ӣ�����û������������ͽ�ֹ
					return FALSE;
				::Sleep(100);
			}

			if(g_QuerySession[i].nReturnValue == 0)
				return FALSE;
			return TRUE;
		}
	}
	// ������
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////
// �Ự����

int CAcl::CreateSession(SOCKET s, int nProtocol)
{
	for(int i=0; i<m_nSessionCount; i++)
	{
		if(m_pSession[i].s == s)
			return i;
	}

	// ȷ�����㹻���ڴ�ռ�
	if(m_nSessionCount >= m_nSessionMaxCount)	// �Ѿ��ﵽ�������
	{
		SESSION *pTmp = new SESSION[m_nSessionMaxCount];
		memcpy(pTmp, m_pSession, m_nSessionMaxCount);

		delete[] m_pSession;
		m_pSession = new SESSION[m_nSessionMaxCount*2];

		memcpy(m_pSession, pTmp,  m_nSessionMaxCount);
		delete[] pTmp;
		m_nSessionMaxCount = m_nSessionMaxCount*2;
	}
	// ��ʼ���µĻỰ
	InitializeSession(&m_pSession[m_nSessionCount]);
	// ���ûỰ����
	m_pSession[m_nSessionCount].s = s;
	m_pSession[m_nSessionCount].nProtocol = nProtocol;
	wcscpy(m_pSession[m_nSessionCount].szPathName, g_szCurrentApp);
	m_nSessionCount++;
	
	ODS1(L" CreateSession m_nSessionCount = %d \n", m_nSessionCount);
	// ���ػỰ����
	return m_nSessionCount - 1;
}

void CAcl::InitializeSession(SESSION *pSession)
{
	memset(pSession, 0, sizeof(SESSION));
	pSession->ucDirection = RULE_DIRECTION_NOT_SET;
	pSession->ucAction = RULE_ACTION_NOT_SET;
}

void CAcl::DeleteSession(SOCKET s)
{
	for(int i=0; i<m_nSessionCount; i++)
	{
		if(m_pSession[i].s == s)
		{
			// ֪ͨӦ�ó�����һ���Ự������
			NotifySession(&m_pSession[i], CODE_DELETE_SESSION);
			memcpy(&m_pSession[i], &m_pSession[i+1], m_nSessionCount - i - 1);
			m_nSessionCount --;
			break;
		}
	}
}

void CAcl::SetSession(SESSION *pSession, USHORT usRemotePort, ULONG ulRemoteIP, UCHAR ucDirection)
{
	pSession->ucDirection = ucDirection;
	if((pSession->usRemotePort != usRemotePort) || (pSession->ulRemoteIP != ulRemoteIP))
	{
		// ����Զ�̶˿ں�����Զ�̷�������
		if(pSession->nProtocol == RULE_SERVICE_TYPE_TCP)
		{
			if(usRemotePort == RULE_SERVICE_PORT_FTP)
				pSession->nProtocol = RULE_SERVICE_TYPE_FTP;
			else if(usRemotePort == RULE_SERVICE_PORT_TELNET)
				pSession->nProtocol = RULE_SERVICE_TYPE_TELNET;
			else if(usRemotePort == RULE_SERVICE_PORT_POP3)
				pSession->nProtocol = RULE_SERVICE_TYPE_POP3;
			else if(usRemotePort == RULE_SERVICE_PORT_SMTP)
				pSession->nProtocol = RULE_SERVICE_TYPE_SMTP;
			else if(usRemotePort  == RULE_SERVICE_PORT_NNTP)
				pSession->nProtocol = RULE_SERVICE_TYPE_NNTP;
			else if(usRemotePort  == RULE_SERVICE_PORT_HTTP)
				pSession->nProtocol = RULE_SERVICE_TYPE_HTTP;
		}

		// ��������
		pSession->usRemotePort = usRemotePort;
		pSession->ulRemoteIP = ulRemoteIP;

		// ֪ͨ������
		NotifySession(pSession, CODE_CHANGE_SESSION);
	}
}

int CAcl::FindSession(SOCKET s)
{
	int i=0;
	for(i; i<m_nSessionCount; i++)
	{
		if(m_pSession[i].s == s)
		{
			break;
		}
	}
	return i;
}

void CAcl::NotifySession(SESSION *pSession, int nCode)
{	
	ODS(L" NotifySession... ");
	if(g_hPhoenixWnd != NULL)
	{
		// ��g_SessionBuffer�����в���һ��δʹ�õĳ�Ա
		int i=0;
		for(i; i<MAX_SESSION_BUFFER; i++)
		{
			if(g_SessionBuffer[i].s == 0)
			{
				g_SessionBuffer[i] = *pSession;
				break;
			}
		}
		// ���Ự���͸���ģ��
		if(i<MAX_SESSION_BUFFER &&
				!::PostMessage(g_hPhoenixWnd, PM_SESSION_NOTIFY, i, nCode))
		{
			// �������ʧ�ܣ��ָ���Ա��ʶ
			g_SessionBuffer[i].s = 0;
		}
	}
}





////////////////////////////////////////////////////////////



int __stdcall PLSPIoControl(LSP_IO_CONTROL *pIoControl, int nType)
{
	switch(nType)
	{
	case IO_CONTROL_SET_RULE_FILE:			// ����Ӧ�ò����
		{
			if(pIoControl->pRuleFile->header.ulLspRuleCount <= MAX_RULE_COUNT)
			{
				g_RuleCount = pIoControl->pRuleFile->header.ulLspRuleCount;
				memcpy(g_Rule, pIoControl->pRuleFile->LspRules, g_RuleCount * sizeof(RULE_ITEM));
			}
		}
		break; 
	case IO_CONTROL_SET_WORK_MODE:			// ���ù���ģʽ
		{
			g_ucWorkMode = pIoControl->ucWorkMode;
		}
		break;
	case IO_CONTROL_GET_WORK_MODE:			// ��ȡ����ģʽ
		{
			return g_ucWorkMode;
		}
		break;
	case IO_CONTROL_SET_PHOENIX_INSTANCE:	// ������ģ����Ϣ
		{
			g_hPhoenixWnd = pIoControl->hPhoenixWnd;
			wcscpy(g_szPhoenixFW, pIoControl->szPath);
		}
		break;
	case IO_CONTROL_GET_SESSION:			// ��ȡһ���Ự
		{
			*pIoControl->pSession = g_SessionBuffer[pIoControl->nSessionIndex];
			// ��ʶ�Ѿ�����ʹ�������Ա��
			g_SessionBuffer[pIoControl->nSessionIndex].s = 0;
		}
		break; 
	case IO_CONTROL_SET_QUERY_SESSION:		// ����DLLѯ�ʵĽ��
		{
			g_QuerySession[pIoControl->nSessionIndex].nReturnValue = pIoControl->ucWorkMode;
			// ��ʶ�Ѿ�����ʹ�������Ա��
			g_QuerySession[pIoControl->nSessionIndex].bUsed = FALSE;
		}
		break;
	case IO_CONTROL_GET_QUERY_SESSION:		// ��ȡ����ѯ�ʵĻỰ
		{	
			wcscpy(pIoControl->szPath, g_QuerySession[pIoControl->nSessionIndex].szPathName);
		}
		break;
	}
	return 0;
}












/*




int CAcl::CheckSend(SOCKET s, const char *buf, DWORD dwTransed)
{
	int nIndex;
	if(nIndex = FindSession(s) >= m_nSessionCount)
		return PF_PASS;


	return GetAccessInfo(&m_pSession[nIndex]);
}
	int	CheckSend(SOCKET s,  const char *buf, DWORD dwTransed);
	int	CheckRecv(SOCKET s,  const char *buf, DWORD dwTransed);
int CAcl::CheckRecv(SOCKET s, const char *buf, DWORD dwTransed)
{
	int nIndex;
	if(nIndex = FindSession(s) >= m_nSessionCount)
		return PF_PASS;

	return GetAccessInfo(&m_pSession[nIndex]);
}


BOOL CAcl::IsLocalIP(DWORD dwIP)
{
	if(dwIP == 0 || ((BYTE*)&dwIP)[3] == 127)
		return TRUE;
	return FALSE;
}

	// �����߳�ͬ�������ȼ����û�������߳����ڷ������ѯ��
	for(int i=0; i<MAX_QUERY_SESSION; i++)
	{
		if(g_QuerySession[i].bUsed && 
				(wcscmp(g_QuerySession[i].szPathName, g_szCurrentApp) == 0)) // �Ѿ��лỰ����ѯ�ʣ��ȴ�
		{
			// �ȴ�
			ODS(L"�Ѿ��лỰ����ѯ�ʣ��ȴ�");
			int n=0;
			while(g_QuerySession[i].bUsed)
			{
				if(n++ > 3000)		// ��5���ӣ�����û������������ͽ�ֹ
					return FALSE;
				::Sleep(100);
			}

			if(g_QuerySession[i].nReturnValue == 0)
				return FALSE;
			return TRUE;
		}
	}
  */
