////////////////////////////////////////////
// ProtoPacket.cpp�ļ�

#include <Ws2spi.h>
#include <ntddndis.h>
#include <windows.h>
#include "protoutils.h"
#include "ProtoPacket.h"

CArpPacket::CArpPacket(CAdapter *pAdapter):m_pAdapter(pAdapter) 
{
	// ���ù�������
	m_pAdapter->SetFilter(	// NDIS_PACKET_TYPE_PROMISCUOUS|
		NDIS_PACKET_TYPE_DIRECTED | 
		NDIS_PACKET_TYPE_MULTICAST | NDIS_PACKET_TYPE_BROADCAST);
	// ��ʼ�������첽���ͺͽ������ݵ��ص��ṹ
	memset(&m_olRead, 0, sizeof(m_olRead));
	m_olRead.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);	
	memset(&m_olWrite, 0, sizeof(m_olWrite));
	m_olWrite.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

CArpPacket::~CArpPacket()
{
	::CloseHandle(m_olRead.hEvent);
	::CloseHandle(m_olWrite.hEvent);
}

BOOL CArpPacket::SendPacket(u_char *pdEtherAddr, u_char *psEtherAddr, 
			int nOpcode, u_char *pdMac, DWORD dIPAddr, u_char *psMac, DWORD sIPAddr)
{
	// �����껺����
	u_char ucFrame[ARPFRAME_SIZE];

	// ����Ethernetͷ

	ETHeader eh = { 0 };
	memcpy(eh.dhost, pdEtherAddr, 6);
	memcpy(eh.shost, psEtherAddr, 6);
	eh.type = ::htons(ETHERTYPE_ARP);
	memcpy(ucFrame, &eh, sizeof(eh));

	// ����Arpͷ
	ARPHeader ah = { 0 };

	ah.hrd = htons(ARPHRD_ETHER);
	ah.eth_type = htons(ETHERTYPE_IP);
	ah.maclen = 6;
	ah.iplen = 4;
	ah.opcode = htons(nOpcode);

	memcpy(ah.smac, psMac, 6);
	memcpy(ah.saddr, &sIPAddr, 4);
	memcpy(ah.dmac, pdMac, 6);
	memcpy(ah.daddr, &dIPAddr, 4);


	memcpy(&ucFrame[sizeof(ETHeader)], &ah, sizeof(ah));

	// ����
	if(m_pAdapter->SendData(ucFrame, sizeof(ETHeader)+ sizeof(ARPHeader), &m_olWrite) == -1)
	{
		if(::GetLastError() == ERROR_IO_PENDING)
		{
			int nRet = ::WaitForSingleObject(m_olWrite.hEvent, 1000*60);
			if(nRet == WAIT_FAILED || nRet == WAIT_TIMEOUT)
				return FALSE;
		}
	}
	return TRUE;
}

PARPHeader CArpPacket::WaitReply(DWORD dwMillionSec)
{
	PETHeader pEtherh = (PETHeader)m_ucFrame;
	PARPHeader pArph = NULL;
	int nRecvLen = sizeof(ETHeader)+ sizeof(ARPHeader);

	// �ȴ�����ARP��Ӧ
	DWORD dwTick = ::GetTickCount();
	DWORD dwOldTick = dwTick;
	while(TRUE)
	{
		if(m_pAdapter->RecieveData(m_ucFrame, nRecvLen, &m_olRead) == -1)
		{
			if(::GetLastError() == ERROR_IO_PENDING)
			{
				int nRet = ::WaitForSingleObject(m_olRead.hEvent, dwMillionSec);
				if(nRet == WAIT_FAILED || nRet == WAIT_TIMEOUT)
					break;
			}
			else
			{
				break;
			}
		}
		if(pEtherh->type == ::htons(ETHERTYPE_ARP))
		{	
			PARPHeader pTmpHdr = (PARPHeader)(pEtherh + 1);
			if(pTmpHdr->opcode == ::htons(ARPOP_REPLY))
			{	
				// ���յ�ARP��Ӧ������
				pArph = pTmpHdr;
				break;
			}
		}

	    dwOldTick = dwTick;
		dwTick = ::GetTickCount();
		if(dwTick - dwOldTick >= dwMillionSec)	// ��ʱ������
			break;
		else
			dwMillionSec = dwMillionSec - (dwTick - dwOldTick); 
	}

	return pArph;
}
