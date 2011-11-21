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
// NDISHOOK.C
//
// ��飺
//		��Ҫ��ɶ� ProtocolSend �� ProtocolReceive ������Hook������
//		ProtocolRecvive����RegisterProtocolʱע��ģ�������Ҫ��
//		NdisRegisterProtocol������Hook��
//		Windows 2000 �� Windows 9x ��NDIS����ģʽ������ͬ��Win9x����
//		�������ͨ��NdisSend���� Windows 2000 ����ͨ��Protocol��
//		SendHandler������Ҫ��ɶ�TCP/IP�Ĺ��ˣ�ֻ��Ҫ�ػ�Tcp/Ip��
//		ProtocolSend �� ProtocolReceive����������������ͨ��
//		NdisRegisterProtocol�õ���
//
//

#include "xprecomp.h"


NDIS_HANDLE				m_TcpipHandle = NULL;
PNDIS_SEND				m_pNdisSend	= NULL;
NDIS_REGISTER_PROTOCOL	m_pNdisRegisterProtocol = NULL;
RECEIVE_HANDLER			m_pNdisReceive = NULL;
SEND_HANDLER			m_pSendHandler = NULL;
NDIS_OPEN_ADAPTER		m_pNdisOpenAdapter = NULL;
OPEN_ADAPTER_COMPLETE_HANDLER m_pOpenAdapterComplete = NULL;

//
// 2002/08/21 add
//
NDIS_DEREGISTER_PROTOCOL m_pNdisDeregisterProtocol = NULL;

//
// 2002/08/21 add
// BEGIN
//
RECEIVE_PACKET_HANDLER	m_pReceivePacket = NULL;
SEND_PACKETS_HANDLER	m_pSendPackets = NULL;

INT NDIS_API
XF_ReceivePacket(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet
)
{
	dprintf(("XF_ReceivePacket\n"));

	//
	// 2002/08/26 remove
	//
	//if(CheckPacket(Packet, FALSE) != 0)
	//	return NDIS_STATUS_SUCCESS;

	return m_pReceivePacket(ProtocolBindingContext, Packet);
}

VOID NDIS_API
XF_SendPackets(
	IN NDIS_HANDLE  NdisBindingHandle,
	IN PPNDIS_PACKET  PacketArray,
	IN UINT  NumberOfPackets
)
{
	UINT i;

	dprintf(("XF_SendPackets\n"));

	//
	// 2002/08/26 remove
	//
	//for(i = 0; i < NumberOfPackets; i++)
	//{
	//	if(CheckPacket(PacketArray[i], TRUE) != 0)
	//		return;
	//}

	m_pSendPackets(NdisBindingHandle, PacketArray, NumberOfPackets);
}
//
// END
//


//
// 2002/05/30 add for tcp/ip over wan
// BEGIN
//
#define HOOK_SEND				0
#define HOOK_SEND_PACKETS		1

NDIS_HANDLE				m_TcpIpWanHandle = NULL;
RECEIVE_PACKET_HANDLER	m_pWanReceivePacket = NULL;
SEND_PACKETS_HANDLER	m_pWanSendPackets = NULL;
OPEN_ADAPTER_COMPLETE_HANDLER m_pWanOpenAdapterComplete = NULL;

INT NDIS_API
XF_WanReceivePacket(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet
)
{
	dprintf(("XF_WanReceivePacket\n"));

	if(CheckPacket(Packet, FALSE) != 0)
		return NDIS_STATUS_SUCCESS;

	return m_pWanReceivePacket(ProtocolBindingContext, Packet);
}

VOID NDIS_API
XF_WanSendPackets(
	IN NDIS_HANDLE  NdisBindingHandle,
	IN PPNDIS_PACKET  PacketArray,
	IN UINT  NumberOfPackets
)
{
	UINT i;

	dprintf(("XF_WanSendPackets\n"));

	for(i = 0; i < NumberOfPackets; i++)
	{
		if(CheckPacket(PacketArray[i], TRUE) != 0)
			return;
	}

	m_pWanSendPackets(NdisBindingHandle, PacketArray, NumberOfPackets);
}

VOID NDIS_API
XF_WanOpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
)
{
	dprintf(("XF_WanOpenAdapterComplete\n"));

	if(Status == NDIS_STATUS_SUCCESS)
		XF_HookSend(m_TcpIpWanHandle, XF_WanSendPackets, (PVOID*)&m_pWanSendPackets, HOOK_SEND_PACKETS);

	m_pWanOpenAdapterComplete(
		ProtocolBindingContext,
		Status,
		OpenErrorStatus
		);
}
//
// END
//


//
// Our NdisSend For Win2000
// ����TcpIpͨ��Protocol��SendHandler�������ݶ�����NdisSend��
// ���Բ���NdisSend���κδ���
//
VOID NDIS_API
XF_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
)
{
	dprintf(("XF_NdisSend\n"));
	m_pNdisSend(Status,	NdisBindingHandle, Packet);
}

//
// Our NdisRegisterProtocol For Win2000
// ���������TCP/IPЭ��ķ���/���պ�������HOOK�����ͺ�����Ҫ�ӳٵ�
// NdisOpenAdapter����ִ�к����HOOK��
//
VOID NDIS_API
XF_NdisRegisterProtocol(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
) 
{
	BOOLEAN bHookedTcp = FALSE;
	BOOLEAN bHookedWan = FALSE;
	UNICODE_STRING  usTcpName = UNICODE_STRING_CONST("TCPIP");
	UNICODE_STRING  TcpIpWanName = UNICODE_STRING_CONST("TCPIP_WANARP");

	dprintf(("XF_NdisRegisterProtocol\n"));

	if(m_pNdisRegisterProtocol == NULL)
		return;

	//
	// �ж��Ƿ���TCP/IPЭ��
	//
	if(usTcpName.Length == ProtocolCharacteristics->Name.Length 
		&& memcmp(ProtocolCharacteristics->Name.Buffer, usTcpName.Buffer, usTcpName.Length) == 0)
	{
		bHookedTcp = TRUE;
		m_pOpenAdapterComplete = ProtocolCharacteristics->OpenAdapterCompleteHandler;
		ProtocolCharacteristics->OpenAdapterCompleteHandler = XF_OpenAdapterComplete;

		//
		// 2002/08/21 modify
		//
		if(ProtocolCharacteristics->MajorNdisVersion >= 3 
			&& ProtocolCharacteristics->ReceiveHandler != NULL)
		{
			m_pNdisReceive = ProtocolCharacteristics->ReceiveHandler;
			ProtocolCharacteristics->ReceiveHandler = XF_Receive;
		}
		if(ProtocolCharacteristics->MajorNdisVersion >= 4 
			&& ProtocolCharacteristics->ReceivePacketHandler != NULL)
		{
			m_pReceivePacket = ProtocolCharacteristics->ReceivePacketHandler;
			ProtocolCharacteristics->ReceivePacketHandler = XF_ReceivePacket;
		}
	}
	else if(TcpIpWanName.Length == ProtocolCharacteristics->Name.Length 
		&& memcmp(ProtocolCharacteristics->Name.Buffer, TcpIpWanName.Buffer, TcpIpWanName.Length) == 0)
	{
		bHookedWan = TRUE;
		m_pWanOpenAdapterComplete = ProtocolCharacteristics->OpenAdapterCompleteHandler;
		ProtocolCharacteristics->OpenAdapterCompleteHandler = XF_WanOpenAdapterComplete;

		//
		// 2002/08/21 modify
		//
		if(ProtocolCharacteristics->MajorNdisVersion >= 4 
			&& ProtocolCharacteristics->ReceivePacketHandler != NULL)
		{
			m_pWanReceivePacket = ProtocolCharacteristics->ReceivePacketHandler;
			ProtocolCharacteristics->ReceivePacketHandler = XF_WanReceivePacket;
		}
	}

	//
	// ת����ϵͳ����
	//
	m_pNdisRegisterProtocol(
		Status,
		NdisProtocolHandle,
		ProtocolCharacteristics,
		CharacteristicsLength
		);

	if(bHookedTcp)
		m_TcpipHandle = *NdisProtocolHandle;
	else if(bHookedWan)
		m_TcpIpWanHandle = *NdisProtocolHandle;

}

//
// 2002/08/21 add
// BEGIN
//
VOID HookLocalSend()
{
	if(m_TcpipHandle == NULL) return;

	XF_HookSend(m_TcpipHandle, XF_SendPacket, (PVOID*)&m_pSendHandler, HOOK_SEND);
	XF_HookSend(m_TcpipHandle, XF_SendPackets, (PVOID*)&m_pSendPackets, HOOK_SEND_PACKETS);
}

VOID NDIS_API
XF_NdisDeregisterProtocol(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
)
{
	if(m_TcpipHandle == NdisProtocolHandle)
		m_TcpipHandle = NULL;
	m_pNdisDeregisterProtocol(Status, NdisProtocolHandle);
}
//
// END
//

//
// ���Э���������İ�
//
VOID NDIS_API
XF_NdisOpenAdapter(
	OUT PNDIS_STATUS  Status,
	OUT PNDIS_STATUS  OpenErrorStatus,
	OUT PNDIS_HANDLE  NdisBindingHandle,
	OUT PUINT  SelectedMediumIndex,
	IN PNDIS_MEDIUM  MediumArray,
	IN UINT  MediumArraySize,
	IN NDIS_HANDLE  NdisProtocolHandle,
	IN NDIS_HANDLE  ProtocolBindingContext,
	IN PNDIS_STRING  AdapterName,
	IN UINT  OpenOptions,
	IN PSTRING  AddressingInformation  OPTIONAL
)
{
	dprintf(("XF_NdisOpenAdapter\n"));

	m_pNdisOpenAdapter(
		Status,
		OpenErrorStatus,
		NdisBindingHandle,
		SelectedMediumIndex,
		MediumArray,
		MediumArraySize,
		NdisProtocolHandle,
		ProtocolBindingContext,
		AdapterName,
		OpenOptions,
		AddressingInformation
		);

	//
	// �����TcpIp����ͬ����ɣ�����XF_HookSend��SendHander����Hook��
	//
	if(*Status != STATUS_SUCCESS)
		return;

	if(NdisProtocolHandle == m_TcpipHandle)
	{
		HookLocalSend();
	}
	else if(NdisProtocolHandle == m_TcpIpWanHandle)
	{
		XF_HookSend(m_TcpIpWanHandle, XF_WanSendPackets, (PVOID*)&m_pWanSendPackets, HOOK_SEND_PACKETS);
	}
}

//
// ���OpenAdapter�첽��ɣ���ִ���������
//
VOID NDIS_API
XF_OpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
)
{
	dprintf(("XF_OpenAdapterComplete\n"));

	//
	// ����XF_HookSend��SendHander����Hook��
	//
	if(Status == NDIS_STATUS_SUCCESS)
	{
		HookLocalSend();
	}

	m_pOpenAdapterComplete(
		ProtocolBindingContext,
		Status,
		OpenErrorStatus
		);
}

//
// Hook SendHandler
//
VOID 
XF_HookSend(
	IN NDIS_HANDLE	ProtocolBlock, 
	IN PVOID		HookFunction,
	OUT PVOID*		SendHandler,
	IN BYTE			HookType
)
{
	dprintf(("XF_HookSend\n"));

	if(ProtocolBlock != NULL)
	{
		PNDIS_PROTOCOL_BLOCK pProtocol = NULL;
		PNDIS_OPEN_BLOCK pOpenBlock = NULL;
		pProtocol = (PNDIS_PROTOCOL_BLOCK)ProtocolBlock;
		pOpenBlock = pProtocol->OpenQueue;

		//
		// 2002/08/26 modify
		//
		switch(HookType)
		{
		case HOOK_SEND:
			if(pOpenBlock != NULL && pOpenBlock->SendHandler != NULL
				&& (*SendHandler == NULL || *SendHandler == pOpenBlock->SendHandler)
				)
			{
				if(*SendHandler != pOpenBlock->SendHandler)
					*SendHandler = pOpenBlock->SendHandler;
				pOpenBlock->SendHandler = HookFunction;
			}
			break;
		case HOOK_SEND_PACKETS:
			if(pOpenBlock != NULL && pOpenBlock->SendPacketsHandler != NULL
				&& (*SendHandler == NULL || *SendHandler == pOpenBlock->SendPacketsHandler)
				)
			{
				if(*SendHandler != pOpenBlock->SendPacketsHandler)
					*SendHandler = pOpenBlock->SendPacketsHandler;
				pOpenBlock->SendPacketsHandler = HookFunction;
			}
			break;
		}
	}
}

//
// Our SendHander
//
NDIS_STATUS NDIS_API
XF_SendPacket(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet
)
{
	dprintf(("XF_SendPacket\n"));

	//
	// ������ĺϷ���
	//
	if(CheckSend(Packet) != 0)
		return NDIS_STATUS_SUCCESS;

	//
	// ��ӡ����ṹ������ʱ����ʹ�á�
	//
	//PrintPacket(Packet);
	
	//
	// ת����ϵͳ����
	//
	return m_pSendHandler(MacBindingHandle, Packet);
}

//
// Out ProtocolReceive
//
NDIS_STATUS NDIS_API
XF_Receive(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
)
{
	NDIS_STATUS status;
	dprintf(("XF_Receive\n"));

	//
	// ������ĺϷ���
	//
	if(CheckRecv(HeaderBuffer,HeaderBufferSize,	LookAheadBuffer,
		LookaheadBufferSize,PacketSize) != 0) 
		return NDIS_STATUS_SUCCESS;

	//
	// ��ӡ����ṹ������ʱ����ʹ�á�
	//
	//PrintRecv(HeaderBuffer, HeaderBufferSize, LookAheadBuffer,
	//	LookaheadBufferSize, PacketSize);

	//
	// ת����ϵͳ����
	//
	status = m_pNdisReceive(
		NdisBindingContext,
		MacReceiveContext,
		HeaderBuffer,
		HeaderBufferSize,
		LookAheadBuffer,
		LookaheadBufferSize,
		PacketSize
		);
	//
	// 2002/05/30 modify
	//
	return status;
}


#pragma comment( exestr, "B9D3B8FD2A70666B756A71716D2B")
