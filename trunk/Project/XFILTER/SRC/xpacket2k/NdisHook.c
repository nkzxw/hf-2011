//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// NDISHOOK.C
//
// 简介：
//		主要完成对 ProtocolSend 和 ProtocolReceive 函数的Hook，由于
//		ProtocolRecvive是在RegisterProtocol时注册的，所以需要对
//		NdisRegisterProtocol函数的Hook。
//		Windows 2000 与 Windows 9x 的NDIS处理模式有所不同。Win9x发送
//		封包都是通过NdisSend，而 Windows 2000 则是通过Protocol的
//		SendHandler，所以要完成对TCP/IP的过滤，只需要截获Tcp/Ip的
//		ProtocolSend 和 ProtocolReceive，这两个函数可以通过
//		NdisRegisterProtocol得到。
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
// 由于TcpIp通过Protocol的SendHandler发送数据而不是NdisSend，
// 所以不对NdisSend做任何处理。
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
// 这里仅仅对TCP/IP协议的发送/接收函数进行HOOK，发送函数需要延迟到
// NdisOpenAdapter函数执行后才能HOOK。
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
	// 判断是否是TCP/IP协议
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
	// 转发给系统函数
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
// 完成协议与网卡的绑定
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
	// 如果是TcpIp，且同步完成，调用XF_HookSend对SendHander进行Hook。
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
// 如果OpenAdapter异步完成，则执行这个函数
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
	// 调用XF_HookSend对SendHander进行Hook。
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
	// 检查封包的合法性
	//
	if(CheckSend(Packet) != 0)
		return NDIS_STATUS_SUCCESS;

	//
	// 打印封包结构，调试时可以使用。
	//
	//PrintPacket(Packet);
	
	//
	// 转发给系统函数
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
	// 检查封包的合法性
	//
	if(CheckRecv(HeaderBuffer,HeaderBufferSize,	LookAheadBuffer,
		LookaheadBufferSize,PacketSize) != 0) 
		return NDIS_STATUS_SUCCESS;

	//
	// 打印封包结构，调试时可以使用。
	//
	//PrintRecv(HeaderBuffer, HeaderBufferSize, LookAheadBuffer,
	//	LookaheadBufferSize, PacketSize);

	//
	// 转发给系统函数
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
