//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/04/02
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
// ��飺
//		MemoryAcl.cģ����Ҫ�����������Ĺ��ܣ�
// 
//		1. ���뱣��عܹ�����ڴ�ռ䣬����XFILTER.EXE��XFILTER.DLL֮�乲��
//		2. ��NdisSend��ProtocolReceive�ķ�����н����������ķ���� Ethernet ���ݰ�ͷ��
//			IPͷ��TCPͷ��UDPͷ��ICMP�����������NetBios.c�ĳ����NetBios�����ֽ��н���
//		3. �عܹ���ıȶ�
//
//

#include "xprecomp.h"
#include "..\common\filt.h"
#pragma hdrstop

PXACL_HEADER m_pAclHeader = 0;
BOOL m_IsFilter = FALSE;
int m_PageSize = 0;

//==============================================================================
// ���뱣��عܹ�����ڴ�ռ䣬����XFILTER.EXE��XFILTER.DLL֮�乲�����غ���
//

//
// ���ù���ģʽ
//		TRUE:	��������
//		FALSE:	ֹͣ����
//
void SetFilterMode(BOOL IsFilter)
{
	//
	// 2002/05/24 add
	//
	if(!IsFilter)
	{
		if(RefenceAclCount())
		{
			m_pAclHeader->bWorkMode = XF_PASS_ALL;
			DerefenceAclCount();
		}
	}

	m_IsFilter = IsFilter;
}

//
// �õ��عܹ�����ڴ��ַ��
//		��XFILTER.EXE��XFILTER.DLLͨ��DeviceIoControl����
//		�õ��عܹ����ڴ滺�����ĵ�ַ��
//
void* GetBuffer()
{
	return m_pAclHeader;
}

//
// Ϊ�عܹ��������ڴ�ռ䡣
// ������
//		nPageSize: �����ڴ�ռ�Ĵ�С
//
void* CreateMemory(int nPageSize)
{
	static void* pBuffer;
	if(m_pAclHeader != 0 && nPageSize == m_PageSize) 
		return m_pAclHeader;
	m_pAclHeader = 0;
	pBuffer = malloc(nPageSize);
	m_PageSize = nPageSize;
	m_pAclHeader = (PXACL_HEADER)pBuffer;
	return pBuffer;
}

//
// �ͷſعܹ�����ڴ�ռ�
//
int FreeMemory()
{
	if(m_pAclHeader == 0) return 0;
	free(m_pAclHeader);
	m_pAclHeader = 0;
	m_PageSize = 0;
	return 1;
}

//
// ʹ�عܹ�������ʹ�õļ����� + 1
//
BOOL RefenceAclCount()
{
	if(!m_IsFilter || m_pAclHeader == NULL) 
		return FALSE;
	if(m_pAclHeader->wPv == PV_LOCKED)
		return FALSE;
	m_pAclHeader->wRefenceCount++;
 	dprintf("RefenceAclCount: %d\n", m_pAclHeader->wRefenceCount);
	return TRUE;
}

//
// ʹ�عܹ�������ʹ�õļ����� - 1
//
void DerefenceAclCount()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wRefenceCount--;
	dprintf("DerefenceAclCount: %d\n", m_pAclHeader->wRefenceCount);
}


//==============================================================================
// �عܹ���Ƚ���غ���
//

//
// ���ܹ���ģʽ�õ��ع�״̬
//
int GetAccessFromWorkMode()
{
	if(m_pAclHeader->bWorkMode == XF_PASS_ALL)
		return XF_PASS;
	if(m_pAclHeader->bWorkMode == XF_DENY_ALL)
		return XF_DENY;
	if(m_pAclHeader->bWorkMode != XF_QUERY_ALL)
		return XF_UNKNOWN;

	return XF_FILTER;
}

//
// �ж��Ƿ��Ǳ���IP��ַ
// ����ֵ��
//		TRUE:	�Ǳ���IP
//		FALSE:	���Ǳ���IP
//
BOOL IsLocalIp(DWORD *ip)
{
	static BYTE IsLocalIP[4];
	memcpy(IsLocalIP, ip, sizeof(DWORD));

	if(*ip == 0 || IsLocalIP[3] == 127)
		return TRUE;
	return FALSE;
}

//
// �ж��Ƿ������й��µķ��������㲥����
//	�˺�����ͣ�ж���Զ����FALSE��
//
BOOL IsRoutine(PPACKET_BUFFER pPacketBuffer)
{
//	if(pPacketBuffer->DestinationIp == 0xFFFFFFFF || pPacketBuffer->SourceIp == 0xFFFFFFFF)
//		return TRUE;
//	if( (pPacketBuffer->DestinationIp & 0x00FFFFFF) == (pPacketBuffer->SourceIp - 0x00FFFFFF)
//		&& ((pPacketBuffer->DestinationIp & 0xFF000000) == 0xFF 
//			|| (pPacketBuffer->SourceIp & 0xFF000000) == 0xFF)
//		)
//		return TRUE;
	return FALSE;
}

//
// ��ʹ�ÿعܹ����жϷ���Ƿ�������У���ʹ�ÿعܹ����ж�֮ǰһ��
// ���ȵ��ô˺��������жϣ��������XF_FILTER����Ҫ��һ��ʹ�ÿعܹ���
// �����жϣ������XF_PASS����XF_DENY��ֱ�ӽ��з��л��߾ܾ���
// 
// ������
//		pPacketBuffer: ָ��ػ�����ݷ��ָ�롣
//
int	GetAccessWithoutAcl(PPACKET_BUFFER pPacketBuffer)
{
	static int Action;
	Action = GetAccessFromWorkMode();
	if(Action != XF_FILTER)
		return Action;

	if(IsLocalIp(&pPacketBuffer->DestinationIp) || IsRoutine(pPacketBuffer)) 
		return XF_PASS;

	if(pPacketBuffer->Direction == ACL_DIRECTION_NOT_SET)
		return XF_PASS;

	return XF_FILTER;
}

//
// �����ӹ���ģʽ�ж��Ƿ���С�����Ӧ�ó��������ھӡ�ICMP�ֱ���
// һ���ӹ���ģʽ������ӹ���ģʽ������عܹ�������жϡ�
//
int CheckSubWorkMode(BYTE bWorkMode, PPACKET_BUFFER pPacketBuffer)
{
	if(strcmp(m_pAclHeader->sSignature, ACL_HEADER_SIGNATURE) != 0)
		return XF_PASS;

	switch(bWorkMode)
	{
	case ACL_PASS_ALL:
		return XF_PASS;
	case ACL_DENY_ALL:
		return XF_DENY;
	case ACL_DENY_IN:
		if(pPacketBuffer->Protocol == ACL_SERVICE_TYPE_UDP
			&& (pPacketBuffer->SourcePort == 137 
				|| pPacketBuffer->SourcePort == 138
				|| pPacketBuffer->SourcePort == 53
				)
			)
			return XF_PASS;
		if(pPacketBuffer->Direction == ACL_DIRECTION_IN)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_DENY_OUT:
		if(pPacketBuffer->Protocol == ACL_SERVICE_TYPE_UDP
			&& (pPacketBuffer->DestinationPort == 137 
				|| pPacketBuffer->DestinationPort == 138
				|| pPacketBuffer->DestinationPort == 53
				)
			)
			return XF_PASS;
		if(pPacketBuffer->Direction == ACL_DIRECTION_OUT)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}

//
// ��û���ҵ����ϵĿعܹ���ʱ���տعܹ����Ĭ�����þ����Ƿ��С��ܾ�����ѯ��
//
int CheckQueryEx(BYTE bQueryEx)
{
	switch(bQueryEx)
	{
	case ACL_QUERY_PASS:
		return XF_PASS;
	case ACL_QUERY_DENY:
		return XF_DENY;
	case ACL_QUERY_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}

//
// ���ݰ�ȫ�ȼ��ж��Ƿ���Ҫ���С�����Ҫѯ�ʶ���ȫ�ȼ�Ϊ��ʱĬ�Ϸ���
// ���ߵ���Ҫѯ�ʰ�ȫ�ȼ�Ϊ��ʱ������Ĭ�Ϸ��С�
//
int GetAccessFromSecurity(PPACKET_BUFFER pPacket)
{
	if(pPacket->Action == XF_QUERY && 
		(m_pAclHeader->bSecurity == ACL_SECURITY_LOWER
		|| (m_pAclHeader->bSecurity == ACL_SECURITY_NORMAL
			&& pPacket->Direction == ACL_DIRECTION_OUT
		    && (pPacket->AclType == ACL_TYPE_NNB || pPacket->AclType == ACL_TYPE_ICMP)
			)
		)
	  )
		return XF_PASS;
	return pPacket->Action;
}

//
// Ӧ�ó������ȽϺ���
//
int CheckApp(PPACKET_BUFFER pPacketBuffer)
{
	static int Action;
	static PXACL pAcl;
	static char pProcessName[260];

	dprintf("CheckApp\n");

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	GetProcessFileName(pProcessName, 260, FALSE);

	Action = CheckSubWorkMode(m_pAclHeader->bAppSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	pAcl = m_pAclHeader->pAcl;

	if(pAcl != NULL)
	{
		//
		// ѭ���Ƚ�Ӧ�ó������
		//
		do
		{
			if(pAcl->bDirection != ACL_DIRECTION_IN_OUT 
				&& pAcl->bDirection != pPacketBuffer->Direction) continue;
			if(pAcl->uiServicePort != ACL_SERVICE_PORT_ALL 
				&& pAcl->uiServicePort != pPacketBuffer->DestinationPort) continue;
			if(pAcl->bRemoteNetType != ACL_NET_TYPE_ALL 
				&& pAcl->bRemoteNetType != pPacketBuffer->NetType) continue;
			if(pAcl->wLocalPort != ACL_SERVICE_PORT_ALL 
				&& pAcl->wLocalPort != pPacketBuffer->SourcePort) continue;
			if(pAcl->bAccessTimeType != ACL_SERVICE_PORT_ALL 
				&& pAcl->bAccessTimeType != pPacketBuffer->TimeType) continue;
			if(pAcl->bServiceType != ACL_SERVICE_TYPE_ALL 
				&& pAcl->bServiceType != pPacketBuffer->Protocol) continue;
			if(strcmp(pAcl->sApplication, _T("*")) != 0 
				&& (strlen(pAcl->sApplication) != strlen(pProcessName) 
				|| strnicmp(pAcl->sApplication, pProcessName
					, strlen(pProcessName)) != 0)
				) continue;

			Action = pAcl->bAction;
			pPacketBuffer->AclId = pAcl->ulAclID;
			goto XF_EXIT;

		}while((pAcl = pAcl->pNext) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bAppQueryEx);
	goto XF_EXIT;


XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// �����ھӿعܹ���ȶԺ���
//
int CheckNnb(PPACKET_BUFFER pPacketBuffer)
{
	static int Action;
	static PXACL_NNB pNnb;
	static char sNnb[NETBIOS_NAME_MAX_LENTH];

	dprintf("CheckNnb\n");

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	Action = CheckSubWorkMode(m_pAclHeader->bNnbSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	GetNameFromIp(pPacketBuffer->DestinationIp, sNnb);

	pNnb = m_pAclHeader->pNnb;
	if(pNnb != NULL)
	{
		//
		// ѭ���Ƚ������ھӹ���
		//
		do
		{
			if(pNnb->bDirection != ACL_DIRECTION_IN_OUT 
				&& pNnb->bDirection != pPacketBuffer->Direction) continue;
			if(pNnb->bTimeType != ACL_SERVICE_PORT_ALL 
				&& pNnb->bTimeType != pPacketBuffer->TimeType) continue;
			if(strcmp(pNnb->sNnb, _T("*")) != 0 
				&& (strlen(pNnb->sNnb) != strlen(sNnb) || strnicmp(pNnb->sNnb, sNnb, strlen(sNnb)) != 0)
				&& pNnb->dwIp != pPacketBuffer->DestinationIp
				) continue;

			if(strnicmp(pNnb->sNnb, sNnb, strlen(sNnb)) == 0 && pNnb->dwIp != pPacketBuffer->DestinationIp)
				pNnb->dwIp = pPacketBuffer->DestinationIp;

			Action = pNnb->bAction;
			pPacketBuffer->AclId = pNnb->dwId;
			goto XF_EXIT;

		}while((pNnb = pNnb->pNext) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bNnbQueryEx);
	goto XF_EXIT;

XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// ICMP�عܹ���ȽϺ���
//
int CheckIcmpAcl(PPACKET_BUFFER pPacketBuffer)
{
	static int Action;
	static PXACL_ICMP pIcmp;

	dprintf("CheckIcmpAcl\n");

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	Action = CheckSubWorkMode(m_pAclHeader->bIcmpSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	pIcmp = m_pAclHeader->pIcmp;
	if(pIcmp != NULL)
	{
		//
		// ѭ���Ƚ�ICMP����
		//
		do
		{
			if(pIcmp->bDirection != ACL_DIRECTION_IN_OUT 
				&& pIcmp->bDirection != pPacketBuffer->Direction) continue;
			if(pIcmp->bTimeType != ACL_SERVICE_PORT_ALL 
				&& pIcmp->bTimeType != pPacketBuffer->TimeType) continue;
			if(pIcmp->bNetType != ACL_NET_TYPE_ALL 
				&& pIcmp->bNetType != pPacketBuffer->NetType) continue;

			Action = pIcmp->bAction;
			pPacketBuffer->AclId = pIcmp->dwId;
			goto XF_EXIT;

		}while((pIcmp = pIcmp->pNext) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bIcmpQueryEx);
	goto XF_EXIT;


XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// ��������
//
int CheckTorjan()
{
	return XF_PASS;
}

//
// ��������
//
int ProtectApp()
{
	return XF_PASS;
}

//==============================================================================
// ���������غ���
//

//
// ��NdisSend���͵�NDIS_PACKET������н���
//
int CheckSend(
	IN PNDIS_PACKET packet
)
{
 	static PNDIS_BUFFER  FirstBuffer, Buffer;
	static UINT TotalPacketLength;
	static WORD EthernetFrameType;
	static WORD HeaderLength;
	static PIP_HEADER pIpHeader;
	static PETHERNET_FRAME pEthernetFrame;
	static void* pBiosBuffer;
	static PICMP_HEADER pIcmpHeader;
	static PTCP_HEADER pTcpHeader;
	static PUDP_HEADER pUdpHeader;

	dprintf("CheckSend\n");

	//
	// �õ���һ��NDIS_BUFFER����ѭ��������NDIS_PACKET�����з����С
	//
	FirstBuffer = packet->Private.Head;
	Buffer = FirstBuffer;
	TotalPacketLength = 0;
	while(Buffer != NULL)
	{
		TotalPacketLength += Buffer->Length;
		Buffer = Buffer->Next;
	}
	Buffer = FirstBuffer->Next;

	//
	// ����Ethernet Frame
	//
	pEthernetFrame = (PETHERNET_FRAME)FirstBuffer->VirtualAddress;
	EthernetFrameType = ntohs(pEthernetFrame->FrameType);
	if(EthernetFrameType != ETHERNET_FRAME_TYPE_TCPIP
		|| Buffer == NULL || Buffer->Length < IP_HEADER_LENGTH)
		return XF_PASS;

	//
	// ����Ip Header
	//
	pIpHeader = (PIP_HEADER)Buffer->VirtualAddress;
	HeaderLength = pIpHeader->HeaderLength * HEADER_LENGTH_MULTIPLE;

	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		//
		// ����Tcp Header
		//
		if((Buffer->Length - HeaderLength) < TCP_HEADER_LENGTH)
		{
			Buffer = Buffer->Next;
			if(Buffer != NULL && Buffer->Length >= TCP_HEADER_LENGTH)
				pTcpHeader = (PTCP_HEADER)(Buffer->VirtualAddress);
			else
				return XF_PASS;
		}
		else
		{
			pTcpHeader = (PTCP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
		}
		pBiosBuffer = NULL;
		Buffer = Buffer->Next;
		if(Buffer != NULL && Buffer->Length >= NETBIOS_MIN_PACKET_SIZE)
			pBiosBuffer = (void*)Buffer->VirtualAddress;
		//
		// ����CheckTcp�Է���ĺϷ��Խ������
		//
		return CheckTcp(pIpHeader, pTcpHeader, TRUE, TotalPacketLength, pBiosBuffer);

	case PROTOCOL_UDP:
		//
		// ����UDP Header
		//
		if((Buffer->Length - HeaderLength) < UDP_HEADER_LENGTH)
		{
			if(Buffer->Next != NULL && Buffer->Next->Length >= UDP_HEADER_LENGTH)
				pUdpHeader = (PUDP_HEADER)(Buffer->Next->VirtualAddress);
			else
				return XF_PASS;
		}
		else
		{
			pUdpHeader = (PUDP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
		}
		pBiosBuffer = NULL;
		Buffer = Buffer->Next;
		if(Buffer != NULL && Buffer->Length >= NETBIOS_MIN_PACKET_SIZE)
			pBiosBuffer = (void*)Buffer->VirtualAddress;
		//
		// ���� CheckUdp �Է���ĺϷ��Խ������
		//
		return CheckUdp(pIpHeader, pUdpHeader, TRUE, TotalPacketLength, pBiosBuffer);

	case PROTOCOL_ICMP:
		//
		// ���� ICMP
		//
		if((Buffer->Length - HeaderLength) < ICMP_HEADER_LENGTH)
		{
			if(Buffer->Next != NULL && Buffer->Next->Length >= ICMP_HEADER_LENGTH)
				pIcmpHeader = (PICMP_HEADER)(Buffer->Next->VirtualAddress);
			else
				return XF_PASS;
		}
		else
		{
			pIcmpHeader = (PICMP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
		}
		//
		// ���� CheckIcmp �Է���ĺϷ��Խ������
		//
		return CheckIcmp(pIpHeader, pIcmpHeader, TRUE, TotalPacketLength);

	case PROTOCOL_IGMP:
	default:
		break;
	}

	return XF_PASS;
}

//
// ��ProtocolReceive���յ�HeaderBuffer��LookAheadBuffer���н���
//
int CheckRecv(
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
)
{
 	static WORD EthernetFrameType;
	static WORD LengthCount;
	static PIP_HEADER pIpHeader;
	static PETHERNET_FRAME pEthernetFrame;

	if(HeaderBufferSize < ETHERNET_FRAME_LENGTH) 
		return XF_PASS;

	dprintf("CheckRecv\n");

	//
	// ����Ethernet Frame
	//
	pEthernetFrame = (PETHERNET_FRAME)HeaderBuffer;
	EthernetFrameType = ntohs(pEthernetFrame->FrameType);
	if(EthernetFrameType != ETHERNET_FRAME_TYPE_TCPIP
		|| LookaheadBufferSize < IP_HEADER_LENGTH)
		return XF_PASS;

	//
	// ����Ip Header
	//
	pIpHeader = (PIP_HEADER)LookAheadBuffer;
	LengthCount = pIpHeader->HeaderLength * HEADER_LENGTH_MULTIPLE;
	if(LengthCount == 0)
		return XF_PASS;

	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		//
		// ����Tcp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + TCP_HEADER_LENGTH))
			return XF_PASS;
		return CheckTcp(pIpHeader
			, (PTCP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, PacketSize + HeaderBufferSize
			, (PVOID)LookaheadBufferSize
			);

	case PROTOCOL_UDP:
		//
		// ���� Udp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + UDP_HEADER_LENGTH))
			return XF_PASS;
		return CheckUdp(pIpHeader
			, (PUDP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, PacketSize + HeaderBufferSize
			, (PVOID)LookaheadBufferSize
			);

	case PROTOCOL_ICMP:
		//
		// ����Icmp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + ICMP_HEADER_LENGTH))
			return XF_PASS;
		return CheckIcmp(pIpHeader
			, (PICMP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, LookaheadBufferSize//PacketSize + HeaderBufferSize
			);

	case PROTOCOL_IGMP:
	default:
		break;
	}
	return XF_PASS;
}

//
// ��Э��Ͷ˿ڵõ��Զ����Э������
//
int GetProtocol(BYTE bProtocol, WORD wPort)
{
	if(bProtocol == PROTOCOL_TCP)
	{
		switch(wPort)
		{
		case ACL_SERVICE_PORT_FTP:
			return ACL_SERVICE_TYPE_FTP;
		case ACL_SERVICE_PORT_TELNET:
			return ACL_SERVICE_TYPE_TELNET;
		case ACL_SERVICE_PORT_NNTP:
			return ACL_SERVICE_TYPE_NNTP;
		case ACL_SERVICE_PORT_POP3:
			return ACL_SERVICE_TYPE_POP3;
		case ACL_SERVICE_PORT_SMTP:
			return ACL_SERVICE_TYPE_SMTP;
		case ACL_SERVICE_PORT_HTTP:
			return ACL_SERVICE_TYPE_HTTP;
		}
		return ACL_SERVICE_TYPE_TCP;
	}
	else if(bProtocol == PROTOCOL_UDP)
		return ACL_SERVICE_TYPE_UDP;
	else if(bProtocol == PROTOCOL_ICMP)
		return ACL_SERVICE_TYPE_ICMP;

	return ACL_SERVICE_TYPE_ALL;
}

//
// �õ��ֽ���ĳһλ
//
int	GetBit(BYTE bit, int index)
{
	bit <<= index;
	bit >>= (8 - 1);

	return bit;
}

//
// ����ʱ������ڵõ��Զ����ʱ������
//
int FindTime(DWORD t, BYTE Week)
{
	static PXACL_TIME pTime;
	static int i;

	if(!RefenceAclCount()) 
		return ACL_TIME_TYPE_ALL;

	i = 0; pTime = m_pAclHeader->pTime;
	if(pTime != NULL) pTime = pTime->pNext;
	do
	{
		i++;
		if(GetBit(pTime->bWeekDay, Week - 1) != 1)
			continue;
		if(pTime->tStartTime == pTime->tEndTime)
			break;
		if(pTime->tStartTime < pTime->tEndTime)
		{
			if(t >= pTime->tStartTime && t <= pTime->tEndTime)
				break;
		}
		else
		{
			if(t >= pTime->tStartTime || t <= pTime->tEndTime)
				break;
		}

	} while((pTime = pTime->pNext) != NULL); 

	DerefenceAclCount();

	if(pTime != NULL)
		return i;

	return ACL_TIME_TYPE_ALL;
}

//
// Ip ��ַ�αȽϺ���
//
BOOL FindIpEx(PXACL_IP pIp, DWORD dwIp)
{
	while(pIp != NULL)
	{
		if(dwIp >= pIp->ulStartIP && dwIp <= pIp->ulEndIP) 
			return TRUE;
		pIp = pIp->pNext;
	}
	return FALSE;
}

//
// ����Ip��ַ�õ��Զ������������
//
int FindIp(DWORD Ip)
{
	static int iRet;

	iRet = ACL_NET_TYPE_ALL;
	if(!RefenceAclCount())
		return iRet;
	if(FindIpEx(m_pAclHeader->pIntranetIp, Ip))
		iRet = ACL_NET_TYPE_INTRANET;
	else if(FindIpEx(m_pAclHeader->pDistrustIp, Ip))
		iRet = ACL_NET_TYPE_DISTRUST;
	else if(FindIpEx(m_pAclHeader->pTrustIp, Ip))
		iRet = ACL_NET_TYPE_TRUST;
	else if(FindIpEx(m_pAclHeader->pCustomIp, Ip))
		iRet = ACL_NET_TYPE_CUSTOM;
	DerefenceAclCount();
	return iRet;
}

//
// �ж��Ƿ���XFILTER.DLL(SPI)��عܵĶ˿ڶ˿ڣ��������SPI�ع�
// ��û�б�Ҫ�ٴοعܡ�
//
BOOL IsSpiPort(PPACKET_BUFFER pPacketBuffer)
{
	static int iPort;
	if(pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_FTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_TELNET
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_NNTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_POP3
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_SMTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_HTTP
		)
		return TRUE;
	if(FindPort((USHORT)pPacketBuffer->SourcePort, &iPort))
		return TRUE;
	return FALSE;
}

//
// Tcp Header ��������
//
int CheckTcp(
	PIP_HEADER pIpHeader, 
	PTCP_HEADER pTcpHeader, 
	BOOL IsSend, 
	UINT LookaheadBufferSize,
	PVOID pVoid
)
{
	static PPACKET_BUFFER pPacketBuffer;
	static PACKET_BUFFER PacketBuffer;
	static PPACKET_DIRECTION pPacketDirection;
	static ULONG Time;

	dprintf("CheckTcp\n");

	//
	// ����TCP��������浽PacketBuffer���ѱ����кϷ��Լ��ͼ�¼��־
	// ����TCP����ľ���ṹ�����RFC793
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);
	if(IsSend)
	{
		PacketBuffer.SourcePort		 = ntohs(pTcpHeader->SourcePort);
		PacketBuffer.DestinationPort = ntohs(pTcpHeader->DestinationPort);
	}
	else
	{
		PacketBuffer.SourcePort		 = ntohs(pTcpHeader->DestinationPort);
		PacketBuffer.DestinationPort = ntohs(pTcpHeader->SourcePort);
	}

	//
	// �˿�Ϊ137��ʾ��NetBios�����ַ���
	//
	if(PacketBuffer.SourcePort == 137 || PacketBuffer.DestinationPort == 137)
	{
		//
		// NetBios���ֽ���
		//
		if(IsSend && pVoid != NULL)
		{
 			MakeNameList((char*)pVoid);
		}
		else
		{
			static int HeaderLength;
			HeaderLength = GET_TCP_HEADER_LENGTH(ntohs(pTcpHeader->LenAndCodeBits));
			if((DWORD)pVoid >= (UINT)(HeaderLength + NETBIOS_MIN_PACKET_SIZE))
 				MakeNameList((char*)pTcpHeader + HeaderLength);
			//return XF_PASS;
		}
	}
	//if(PacketBuffer.SourcePort == 138 || PacketBuffer.DestinationPort == 138)
	//	return XF_PASS;

	if(IsSpiPort(&PacketBuffer))
		return XF_PASS;

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_TCP;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.TcpCode		 = pTcpHeader->TcpCode;
	PacketBuffer.AclType		 = ACL_TYPE_APP;
	PacketBuffer.ProcessHandle   = (DWORD)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = ACL_NET_TYPE_ALL;
	PacketBuffer.Direction		 = ACL_DIRECTION_NOT_SET;

	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);
	
	if(pTcpHeader->TcpSyn)
	{
		static PACKET_DIRECTION PacketDirection;

		//
		// ��������
		//

		if(IsSend)
		{
			PacketBuffer.Direction = pTcpHeader->TcpAck 
				? ACL_DIRECTION_IN : ACL_DIRECTION_OUT;
		}
		else
		{
			PacketBuffer.Direction =  pTcpHeader->TcpAck 
				? ACL_DIRECTION_OUT : ACL_DIRECTION_IN;
		}

		if(PacketBuffer.SourcePort == 139 || PacketBuffer.DestinationPort == 139)
		{
			//
			// �˿�139��ʾ��NetBios����Դ��������������ھӿعܹ�����пع�
			//
			PacketBuffer.AclType = ACL_TYPE_NNB;
			PacketBuffer.Action = CheckNnb(&PacketBuffer);
		}
		else
		{
			//
			// ��Ӧ�ó��������й���
			//
			static WORD wPort;
			wPort = (PacketBuffer.Direction == ACL_DIRECTION_IN) 
				? (WORD)PacketBuffer.SourcePort 
				: (WORD)PacketBuffer.DestinationPort;
			PacketBuffer.NetType = FindIp(PacketBuffer.DestinationIp);
			PacketBuffer.Protocol = GetProtocol(PROTOCOL_TCP, wPort);
			PacketBuffer.AclType = ACL_TYPE_DRIVER_APP;

			PacketBuffer.Action = CheckApp(&PacketBuffer);
		}

		PacketBuffer.Action = GetAccessFromSecurity(&PacketBuffer);

		if(PacketBuffer.Action != XF_PASS || !pTcpHeader->TcpAck)
			goto XF_EXIT;

		//
		// ����TCP���߼�¼
		//
		PacketDirection.DeleteIn	= 0;
		PacketDirection.DeleteOut	= 0;
		PacketDirection.Id			= PacketBuffer.Id;
		PacketDirection.Direction	= (BYTE)PacketBuffer.Direction;
		PacketDirection.Action		= (BYTE)PacketBuffer.Action;
		PacketDirection.AclType		= PacketBuffer.AclType;
		PacketDirection.AclId		= PacketBuffer.AclId;
		PacketDirection.Protocol	= PacketBuffer.Protocol;
		PacketDirection.NetType		= PacketBuffer.NetType;
		PacketDirection.ProcessHandle = PacketBuffer.ProcessHandle;
		strcpy(PacketDirection.sProcess, PacketBuffer.sProcess);
		if(IsSend)
		{
			PacketDirection.RecvData = 0;
			PacketDirection.SendData = PacketBuffer.DataBytes;
		}
		else
		{
			PacketDirection.SendData = 0;
			PacketDirection.RecvData = PacketBuffer.DataBytes;
		}
		PacketDirection.LocalIp		= PacketBuffer.SourceIp;
		PacketDirection.RemoteIp	= PacketBuffer.DestinationIp;
		PacketDirection.LocalPort	= (USHORT)PacketBuffer.SourcePort;
		PacketDirection.RemotePort	= (USHORT)PacketBuffer.DestinationPort;
		PacketDirection.Time		= PacketBuffer.Time;

		AddDirection(&PacketDirection);

		goto XF_EXIT;
	}
	else
	{
		static int iPort;
		static int iIndex;

		//
		// ���߻�����緢�����ݡ��Ͽ����ߡ����߸�λ������
		//

		//
		// ���Ѵ��ڵ����߼�¼�в�ѯ
		//
		if(FindDirection(PacketBuffer.Id, &iIndex))
		{
			pPacketDirection = GetDirection(iIndex);

			if(IsSend)
				pPacketDirection->SendData += PacketBuffer.DataBytes;
			else
				pPacketDirection->RecvData += PacketBuffer.DataBytes;

			PacketBuffer.Direction = pPacketDirection->Direction;
			PacketBuffer.Protocol = pPacketDirection->Protocol;
			PacketBuffer.AclType = pPacketDirection->AclType;
			PacketBuffer.AclId = pPacketDirection->AclId;
			PacketBuffer.NetType = pPacketDirection->NetType;

			//
			// ���߸�λ
			//
			if(pTcpHeader->TcpRst)
			{
				DeleteDirection(PacketBuffer.Id, TRUE, TRUE);
				PacketBuffer.Action = XF_PASS;
				goto XF_EXIT;
			}
			//
			// �ж���������
			//
			else if(pTcpHeader->TcpFin)
			{
				DeleteDirection(PacketBuffer.Id, !IsSend, IsSend);
				PacketBuffer.Action = XF_PASS;
				goto XF_EXIT;
			}
			//
			// ���ݴ���
			//
			else //if(pTcpHeader->TcpPsh)
			{
				if(PacketBuffer.AclType == ACL_TYPE_NNB)
					PacketBuffer.Action = CheckNnb(&PacketBuffer);
				else
					PacketBuffer.Action = CheckApp(&PacketBuffer);

				PacketBuffer.Action = GetAccessFromSecurity(&PacketBuffer);

				pPacketDirection->Action = (BYTE)PacketBuffer.Action;
				if(PacketBuffer.Action == XF_QUERY)
					goto XF_EXIT;
				//if(PacketBuffer.Action == XF_QUERY)
				//	pPacketDirection->Action = XF_DENY;
				//else
				//	pPacketDirection->Action = (BYTE)PacketBuffer.Action;

				return PacketBuffer.Action;
			}

			goto XF_EXIT;
		}
		PacketBuffer.Action = XF_PASS;
		return PacketBuffer.Action;
	}

	PacketBuffer.Action = XF_PASS; 
	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// Udp Header ��������
//
int CheckUdp(
	PIP_HEADER pIpHeader, 
	PUDP_HEADER pUdpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize,
	void *pVoid
)
{
	static PPACKET_BUFFER pPacketBuffer;
	static PACKET_BUFFER PacketBuffer;
	static PPACKET_DIRECTION pPacketDirection;
	static ULONG Time;

	dprintf("CheckUdp\n");

	//
	// ����UDP��������浽PacketBuffer���ѱ����кϷ��Լ��ͼ�¼��־
	// ����UDP����ľ���ṹ�����RFC768
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);
	if(IsSend)
	{
		PacketBuffer.SourcePort		 = ntohs(pUdpHeader->SourcePort);
		PacketBuffer.DestinationPort = ntohs(pUdpHeader->DestinationPort);
	}
	else
	{
		PacketBuffer.SourcePort		 = ntohs(pUdpHeader->DestinationPort);
		PacketBuffer.DestinationPort = ntohs(pUdpHeader->SourcePort);
	}

	//
	// �˿�Ϊ137��ʾ��NetBios�����ַ���
	//
	if(PacketBuffer.SourcePort == 137 || PacketBuffer.DestinationPort == 137)
	{
		//
		// NetBios���ֽ���
		//
		if(IsSend && pVoid != NULL)
 			MakeNameList((char*)pVoid);
		else
		{
			if((DWORD)pVoid >= 8 + NETBIOS_MIN_PACKET_SIZE)
				MakeNameList((char*)pUdpHeader + 8);
		}
		//return XF_PASS;
	}
	//if(PacketBuffer.SourcePort == 138 || PacketBuffer.DestinationPort == 138)
	//	return XF_PASS;

	if(IsSpiPort(&PacketBuffer))
		return XF_PASS;

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.AclType		 = ACL_TYPE_APP;
	PacketBuffer.ProcessHandle   = (ULONG)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = ACL_NET_TYPE_ALL;

	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_UDP;
	
	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);

	if(IsSend)
		PacketBuffer.Direction = ACL_DIRECTION_OUT;
	else
		PacketBuffer.Direction = ACL_DIRECTION_IN;

	if(PacketBuffer.SourcePort == 139 || PacketBuffer.DestinationPort == 139)
	{
		PacketBuffer.AclType = ACL_TYPE_NNB;
		PacketBuffer.Action = CheckNnb(&PacketBuffer);
	}
	else
	{
		PacketBuffer.NetType = FindIp(PacketBuffer.DestinationIp);
		PacketBuffer.AclType = ACL_TYPE_DRIVER_APP;
		PacketBuffer.Action = CheckApp(&PacketBuffer);
	}

	PacketBuffer.Action = GetAccessFromSecurity(&PacketBuffer);

	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// ���� Icmp �������ж����߷���
//
int GetIcmpDirection(BYTE bType)
{
	switch(bType)
	{
	case ICMP_ECHO:
	case ICMP_TSTAMP:
	case ICMP_IREQ:
	case ICMP_MASKREQ:
		return ICMP_REQUEST;

	case ICMP_ECHOREPLY:
	case ICMP_TSTAMPREPLY:
	case ICMP_IREQREPLY:
	case ICMP_MASKREPLY:
		return ICMP_RESPONSE;

	case ICMP_UNREACH:
	case ICMP_SOURCEQUENCH:
	case ICMP_REDIRECT:
	case ICMP_ROUTERADVERT:
	case ICMP_ROUTERSOLICIT:
	case ICMP_TIMXCEED:
	case ICMP_PARAMPROB:
		break;
	}
	return ICMP_NORMAL;
}

//
// ICMP ��������
//
int CheckIcmp(
	PIP_HEADER pIpHeader, 
	PICMP_HEADER pIcmpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize 
)
{
	static BYTE bIcmpType;
	static PPACKET_BUFFER pPacketBuffer;
	static PACKET_BUFFER PacketBuffer;
	static ULONG Time;

	dprintf("CheckIcmp\n");

	//
	// ����ICMP��������浽PacketBuffer���ѱ����кϷ��Լ��ͼ�¼��־
	// ����ICMP����ľ���ṹ�����RFC792
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.AclType		 = ACL_TYPE_ICMP;
	PacketBuffer.ProcessHandle   = (ULONG)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = FindIp(PacketBuffer.DestinationIp);
	PacketBuffer.Id				 = PacketBuffer.SourceIp ^ PacketBuffer.DestinationIp;

	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);

	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_ICMP;
	PacketBuffer.IcmpType		 = pIcmpHeader->Type;
	PacketBuffer.IcmpSubType	 = pIcmpHeader->Code;
	
	bIcmpType = GetIcmpDirection(pIcmpHeader->Type);

	if(bIcmpType == ICMP_NORMAL)
	{
		PacketBuffer.Direction = ACL_DIRECTION_NOT_SET;
	}
	else if(IsSend)
	{
		PacketBuffer.Direction = 
			(bIcmpType == ICMP_REQUEST) ?
			ACL_DIRECTION_OUT
			: ACL_DIRECTION_IN;
	}
	else
	{
		PacketBuffer.Direction = 
			(bIcmpType == ICMP_REQUEST) ?
			ACL_DIRECTION_IN
			: ACL_DIRECTION_OUT;
	}

	PacketBuffer.Action = CheckIcmpAcl(&PacketBuffer);

	PacketBuffer.Action = GetAccessFromSecurity(&PacketBuffer);

	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// ���Ӧ�ó���DLL����������֮��ı�������Ƿ���ͬ
// �˴������������޹�
//
BOOL CheckGuid()
{
	BOOLEAN bReturn;
	CHECK_GUID(bReturn);
	return bReturn;
}

#pragma comment( exestr, "B9D3B8FD2A6F676F71747B63656E2B")
