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
//
// 简介：
//		演示封包解析。用Print函数将封包解析的结果作为调试信息
//		输出，利用DebugView之类的工具可以查看封包数据，判断封包
//		解析的正确与否。
//		此段程序与MemoryAcl.c的CheckSend和CheckRecv函数非常相似
//		不同的是这里将解析结果直接输出。这里就不再对每个函数做出
//		解释。
//		这段程序并没有真正的被使用，只是用来调试。
//
//

#include "xprecomp.h"
#pragma hdrstop

VOID 
PrintRecv(
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
)
{
	WORD EthernetFrameType;
	WORD LengthCount;
	PIP_HEADER pIpHeader;

	if(HeaderBufferSize < ETHERNET_FRAME_LENGTH) 
		return;
	EthernetFrameType = PrintEthernetFrame((PETHERNET_FRAME)HeaderBuffer, HeaderBufferSize);
	if(EthernetFrameType != ETHERNET_FRAME_TYPE_TCPIP)
		return;
	if(LookaheadBufferSize < IP_HEADER_LENGTH)
		return;
	pIpHeader = (PIP_HEADER)LookAheadBuffer;
	LengthCount = PrintIp(pIpHeader, LookaheadBufferSize);
	if(LengthCount == 0)
		return;
	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		{
			PTCP_HEADER pTcpHeader = (PTCP_HEADER)((char*)LookAheadBuffer + LengthCount);

			if(LookaheadBufferSize < (UINT)(LengthCount + TCP_HEADER_LENGTH))
				return;
			
			PrintTcp(pTcpHeader);

			if(ntohs(pTcpHeader->SourcePort) == 137
			   || ntohs(pTcpHeader->DestinationPort) == 137)
			   MakeNameList((char*)pTcpHeader + GET_TCP_HEADER_LENGTH(ntohs(pTcpHeader->LenAndCodeBits)));
		}
		break;
	case PROTOCOL_UDP:
		{
			PUDP_HEADER pUdpHeader = (PUDP_HEADER)((char*)LookAheadBuffer + LengthCount);

			if(LookaheadBufferSize < (UINT)(LengthCount + UDP_HEADER_LENGTH))
				return;

			PrintUdp(pUdpHeader);

			if(ntohs(pUdpHeader->SourcePort) == 137
			   || ntohs(pUdpHeader->DestinationPort) == 137)
			   MakeNameList((char*)pUdpHeader + 8);
		}
		break;
	case PROTOCOL_ICMP:
		if(LookaheadBufferSize < (UINT)(LengthCount + ICMP_HEADER_LENGTH))
			return;
		PrintIcmp((PICMP_HEADER)((char*)LookAheadBuffer + LengthCount));
		break;
	case PROTOCOL_IGMP:
		break;
	default:
		break;
	}
}

VOID PrintPacket(
	IN PNDIS_PACKET packet
)
{
	UINT i;
	UINT PhysicalBufferCount;
	UINT BufferCount;
	PNDIS_BUFFER  FirstBuffer, Buffer;
	UINT TotalPacketLength;

	NdisQueryPacket(packet
		, &PhysicalBufferCount
		, &BufferCount
		, &FirstBuffer
		, &TotalPacketLength
		);
//*
	dprintf("Ndis Packet:\n");
	dprintf("    PacketPointer: 0x%08X.\n", (VOID*)packet);
	dprintf("    PhysicalBufferCount: 0x%08X.\n", PhysicalBufferCount);
	dprintf("    TotalPacketLenth: 0x%08X.\n", TotalPacketLength);
	dprintf("    packet->Private.Flags: 0x%08X.\n", packet->Private.Flags);
	dprintf("    ProtocolReserved: 0x%08X.\n", packet->ProtocolReserved[0]);
	Buffer = FirstBuffer;

	for(i = 0; i < BufferCount; i++)
	{
		dprintf("Buffer(%u), total: %u\n", i, BufferCount);
		dprintf("    BufferPointer: 0x%08X.\n", Buffer);
		dprintf("    BufferVirtualAddress: 0x%08X.\n", Buffer->VirtualAddress);
		dprintf("    BufferLength: 0x%08X(%u).\n", Buffer->Length, Buffer->Length);
		//PrintBlock((PVOID)Buffer->VirtualAddress, Buffer->Length);
		Buffer = Buffer->Next;
	}
//*/

	if(PrintEthernetFrame((PETHERNET_FRAME)FirstBuffer->VirtualAddress
		, FirstBuffer->Length) == ETHERNET_FRAME_TYPE_TCPIP
		&& FirstBuffer->Next != NULL)
	{
		PrintTcpIpHeader(FirstBuffer->Next);
	}
}

WORD
PrintEthernetFrame(
	IN PETHERNET_FRAME pEthernetFrame,
	IN UINT Length
)
{
	WORD EthernetFrameType;

	dprintf("Ethernet Frame:\n");

	if(pEthernetFrame == NULL || Length < ETHERNET_FRAME_LENGTH)
	{
		dprintf("    Invalid Ethernet Frame.\n");
		return ETHERNET_FRAME_TYPE_INVALID;
	}

	dprintf("    Destination Address: %02X-%02X-%02X-%02X-%02X-%02X\n"
		, pEthernetFrame->DestinationAddress[0]
		, pEthernetFrame->DestinationAddress[1]
		, pEthernetFrame->DestinationAddress[2]
		, pEthernetFrame->DestinationAddress[3]
		, pEthernetFrame->DestinationAddress[4]
		, pEthernetFrame->DestinationAddress[5]
		);

	dprintf("    Source Address: %02X-%02X-%02X-%02X-%02X-%02X\n"
		, pEthernetFrame->SourceAddress[0]
		, pEthernetFrame->SourceAddress[1]
		, pEthernetFrame->SourceAddress[2]
		, pEthernetFrame->SourceAddress[3]
		, pEthernetFrame->SourceAddress[4]
		, pEthernetFrame->SourceAddress[5]
		);

	EthernetFrameType = ntohs(pEthernetFrame->FrameType);

	switch(EthernetFrameType)
	{
	case ETHERNET_FRAME_TYPE_TCPIP:
		dprintf("    Ethernet Frame Type: TCP/IP(0x%04X)\n", EthernetFrameType);
		break;
	case ETHERNET_FRAME_TYPE_PUP:
		dprintf("    Ethernet Frame Type: PUP(0x%04X)\n", EthernetFrameType);
		break;
	case ETHERNET_FRAME_TYPE_ARP:
		dprintf("    Ethernet Frame Type: ARP(0x%04X)\n", EthernetFrameType);
		break;
	case ETHERNET_FRAME_TYPE_RARP:
		dprintf("    Ethernet Frame Type: RARP(0x%04X)\n", EthernetFrameType);
		break;
	default:
		dprintf("    Ethernet Frame Type: OTHER(0x%04X)\n", EthernetFrameType);
		break;
	}

	return EthernetFrameType;
}

BYTE
PrintTcpIpHeader(
	IN PNDIS_BUFFER Buffer
)
{
	PIP_HEADER	pIpHeader;
	UINT		HeaderLength;
	

	if(Buffer == NULL || Buffer->Length < IP_HEADER_LENGTH)
	{
		dprintf("    Invalid IP Header.\n");
		return PROTOCOL_INVALID_IP;
	}

	pIpHeader = (PIP_HEADER)Buffer->VirtualAddress;

	HeaderLength = PrintIp(pIpHeader, Buffer->Length);

	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		dprintf("    Protocol: %s(0x%02X)\n", "TCP", pIpHeader->Protocol);
		{
			PTCP_HEADER pTcpHeader;

			if((Buffer->Length - HeaderLength) < TCP_HEADER_LENGTH)
			{
				Buffer = Buffer->Next;
				if(Buffer != NULL && Buffer->Length >= TCP_HEADER_LENGTH)
				{
					pTcpHeader = (PTCP_HEADER)(Buffer->VirtualAddress);
				}
				else
				{
					dprintf("    Invalid Tcp Header.\n");
					return PROTOCOL_INVALID_TCP;
				}
			}
			else
			{
				pTcpHeader = (PTCP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
			}

			PrintTcp(pTcpHeader);

			if(ntohs(pTcpHeader->SourcePort) == 137
			   || ntohs(pTcpHeader->DestinationPort) == 137)
			{
				Buffer = Buffer->Next;
				if(Buffer != NULL && Buffer->Length >= NETBIOS_MIN_PACKET_SIZE)
					MakeNameList((char*)Buffer->VirtualAddress);
			}
		}

		break;
	case PROTOCOL_UDP:
		dprintf("    Protocol: %s(0x%02X)\n", "UDP", pIpHeader->Protocol);
		{
			PUDP_HEADER pUdpHeader;

			if((Buffer->Length - HeaderLength) < UDP_HEADER_LENGTH)
			{
				if(Buffer->Next != NULL && Buffer->Next->Length >= UDP_HEADER_LENGTH)
				{
					pUdpHeader = (PUDP_HEADER)(Buffer->Next->VirtualAddress);
				}
				else
				{
					dprintf("    Invalid Udp Header.\n");
					return PROTOCOL_INVALID_UDP;
				}
			}
			else
			{
				pUdpHeader = (PUDP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
			}
			
			PrintUdp(pUdpHeader);

			if(ntohs(pUdpHeader->SourcePort) == 137
			   || ntohs(pUdpHeader->DestinationPort) == 137)
			{
				Buffer = Buffer->Next;
				if(Buffer != NULL && Buffer->Length >= NETBIOS_MIN_PACKET_SIZE)
					MakeNameList((char*)Buffer->VirtualAddress);
			}
		}

		break;
	case PROTOCOL_ICMP:
		dprintf("    Protocol: %s(0x%02X)\n", "ICMP", pIpHeader->Protocol);
		{
			PICMP_HEADER pIcmpHeader;

			if((Buffer->Length - HeaderLength) < ICMP_HEADER_LENGTH)
			{
				if(Buffer->Next != NULL && Buffer->Next->Length >= ICMP_HEADER_LENGTH)
				{
					pIcmpHeader = (PICMP_HEADER)(Buffer->Next->VirtualAddress);
				}
				else
				{
					dprintf("    Invalid Icmp Header.\n");
					return PROTOCOL_INVALID_ICMP;
				}
			}
			else
			{
				pIcmpHeader = (PICMP_HEADER)((DWORD)Buffer->VirtualAddress + HeaderLength);
			}

			PrintIcmp(pIcmpHeader);
		}

		break;
	case PROTOCOL_IGMP:
		dprintf("    Protocol: %s(0x%02X)\n", "IGMP", pIpHeader->Protocol);
		break;
	default:
		dprintf("    Protocol: %s(0x%02X)\n", "OTHER", pIpHeader->Protocol);
		break;
	}

	return pIpHeader->Protocol;
}

UINT
PrintIp(
	IN PIP_HEADER pIpHeader,
	IN UINT TotalLength
)
{
	UINT HeaderLength = GET_IP_HEADER_LENGTH(pIpHeader->VersionAndHeaderLength);
	if(HeaderLength > TotalLength) return 0;

	dprintf("Ip Header, Header Total Length(%u), 0x%08X:\n", TotalLength, TotalLength);
	dprintf("    Ip Version: %u\n", GET_IP_VERSION(pIpHeader->VersionAndHeaderLength));
	dprintf("    Header Length: %u\n", HeaderLength);
	dprintf("    Type Of Service: %u\n", pIpHeader->TypeOfService);
	dprintf("    Datagram Length: %u\n", ntohs(pIpHeader->DatagramLength));
	dprintf("    Id: %u\n", ntohs(pIpHeader->Id));
	dprintf("    Flags: %u\n", GET_IP_FLAGS(ntohs(pIpHeader->FlagsAndFragmentOffset)));
	dprintf("    Fragment Offset: %u\n",  GET_IP_FRAGMENT_OFFSET(ntohs(pIpHeader->FlagsAndFragmentOffset)));
	dprintf("    TimeToLive: %u\n", pIpHeader->TimeToLive);
	dprintf("    CheckSum: %u\n", ntohs(pIpHeader->CheckSum));

	dprintf("    SourceIp: %u.%u.%u.%u\n"
		, pIpHeader->SourceIp[0]
		, pIpHeader->SourceIp[1]
		, pIpHeader->SourceIp[2]
		, pIpHeader->SourceIp[3]
		);

	dprintf("    DestinationIp: %u.%u.%u.%u\n"
		, pIpHeader->DestinationIp[0]
		, pIpHeader->DestinationIp[1]
		, pIpHeader->DestinationIp[2]
		, pIpHeader->DestinationIp[3]
		);
	return HeaderLength;
}

VOID
PrintTcp(
	IN PTCP_HEADER pTcpHeader
)
{
	dprintf("Tcp Header:\n");
	dprintf("    Source Port: %u\n", ntohs(pTcpHeader->SourcePort));
	dprintf("    Destination Port: %u\n", ntohs(pTcpHeader->DestinationPort));
	dprintf("    SeqNumber: %u\n", ntohl(pTcpHeader->SeqNumber));
	dprintf("    AckNumber: %u\n", ntohl(pTcpHeader->AckNumber));
	dprintf("    Header Length: %u\n", GET_TCP_HEADER_LENGTH(ntohs(pTcpHeader->LenAndCodeBits)));
	dprintf("    Code Bits: %u\n", GET_TCP_CODE_BITS(ntohs(pTcpHeader->LenAndCodeBits)));
	dprintf("    Window: %u\n", ntohs(pTcpHeader->Window));
	dprintf("    CheckSum: %u\n", ntohs(pTcpHeader->CheckSum));
	dprintf("    UrgentPointer: %u\n", ntohs(pTcpHeader->UrgentPointer));
}

VOID
PrintUdp(
	IN PUDP_HEADER pUdpHeader
)
{
	dprintf("Udp Header:\n");
	dprintf("    Source Port: %u\n", ntohs(pUdpHeader->SourcePort));
	dprintf("    Destination Port: %u\n", ntohs(pUdpHeader->DestinationPort));
	dprintf("    Length(include this header): %u\n", ntohs(pUdpHeader->Length));
	dprintf("    CheckSum: %u\n", ntohs(pUdpHeader->CheckSum));
}

VOID
PrintIcmp(
	IN PICMP_HEADER pIcmpHeader
)
{
	CHAR sIcmpType[101];

	IcmpTypeToString(pIcmpHeader->Type, pIcmpHeader->Code, sIcmpType);

	dprintf("Icmp Header:\n");
	dprintf("    Type: %s(%u)\n", sIcmpType, pIcmpHeader->Type);
	dprintf("    Code: %u\n", pIcmpHeader->Code);
	dprintf("    CheckSum: %u\n", ntohs(pIcmpHeader->CheckSum));
	dprintf("    ID: %u\n", ntohs(pIcmpHeader->ID));
	dprintf("    Seq: %u\n", ntohs(pIcmpHeader->Seq));
}

VOID
IcmpTypeToString(
	IN	BYTE		bIcmpType,
	IN	BYTE		bSubCode,
	OUT	PCHAR		sIcmpType
)
{
	sIcmpType[0] = 0;
	switch(bIcmpType)
	{
	case ICMP_ECHOREPLY:
		strcat(sIcmpType, "ICMP_ECHOREPLY(echo reply)");
		break;
	case ICMP_UNREACH:
		strcat(sIcmpType, "ICMP_UNREACH");
		switch(bSubCode)
		{
		case ICMP_UNREACH_NET:
			strcat(sIcmpType, "[ICMP_UNREACH_NET](bad net)");
			break;
		case ICMP_UNREACH_HOST:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST](bad host)");
			break;
		case ICMP_UNREACH_PROTOCOL:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST](bad protocol)");
		case ICMP_UNREACH_PORT:
			strcat(sIcmpType, "[ICMP_UNREACH_PORT](bad port)");
			break;
		case ICMP_UNREACH_NEEDFRAG:
			strcat(sIcmpType, "[ICMP_UNREACH_NEEDFRAG](IP_DF caused drop)");
			break;
		case ICMP_UNREACH_SRCFAIL:
			strcat(sIcmpType, "[ICMP_UNREACH_SRCFAIL](src route failed)");
			break;
		case ICMP_UNREACH_NET_UNKNOWN:
			strcat(sIcmpType, "[ICMP_UNREACH_NET_UNKNOWN](unknown net)");
			break;
		case ICMP_UNREACH_HOST_UNKNOWN:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST_UNKNOWN](unknown host)");
			break;
		case ICMP_UNREACH_ISOLATED:
			strcat(sIcmpType, "[ICMP_UNREACH_ISOLATED](src host isolated)");
			break;
		case ICMP_UNREACH_NET_PROHIB:
			strcat(sIcmpType, "[ICMP_UNREACH_NET_PROHIB](prohibited access)");
			break;
		case ICMP_UNREACH_HOST_PROHIB:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST_PROHIB](ditto)");
			break;
		case ICMP_UNREACH_TOSNET:
			strcat(sIcmpType, "[ICMP_UNREACH_TOSNET](bad tos for net)");
			break;
		case ICMP_UNREACH_TOSHOST:
			strcat(sIcmpType, "[ICMP_UNREACH_TOSHOST](bad tos for host)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_SOURCEQUENCH:
		strcat(sIcmpType, "ICMP_SOURCEQUENCH(packet lost, slow down)");
		break;
	case ICMP_REDIRECT:
		strcat(sIcmpType, "ICMP_REDIRECT(shorter route)");
		switch(bSubCode)
		{
		case ICMP_REDIRECT_NET:
			strcat(sIcmpType, "[ICMP_REDIRECT_NET](for network)");
			break;
		case ICMP_REDIRECT_HOST:
			strcat(sIcmpType, "[ICMP_REDIRECT_HOST](for host)");
			break;
		case ICMP_REDIRECT_TOSNET:
			strcat(sIcmpType, "[ICMP_REDIRECT_TOSNET](for tos and net)");
			break;
		case ICMP_REDIRECT_TOSHOST:
			strcat(sIcmpType, "[ICMP_REDIRECT_TOSHOST](for tos and host)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_ECHO:
		strcat(sIcmpType, "ICMP_ECHO(echo service)");
		break;
	case ICMP_ROUTERADVERT:
		strcat(sIcmpType, "ICMP_ROUTERADVERT(router advertisement)");
		break;
	case ICMP_ROUTERSOLICIT:
		strcat(sIcmpType, "ICMP_ROUTERSOLICIT(router solicitation)");
		break;
	case ICMP_TIMXCEED:
		strcat(sIcmpType, "ICMP_TIMXCEED(time exceeded)");
		switch(bSubCode)
		{
		case ICMP_TIMXCEED_INTRANS:
			strcat(sIcmpType, "[ICMP_TIMXCEED_INTRANS](ttl==0 in transit)");
			break;
		case ICMP_TIMXCEED_REASS:
			strcat(sIcmpType, "[ICMP_TIMXCEED_REASS](ttl==0 in reass)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_PARAMPROB:
		strcat(sIcmpType, "ICMP_PARAMPROB(ip header bad)");
		switch(bSubCode)
		{
		case ICMP_PARAMPROB_OPTABSENT:
			strcat(sIcmpType, "[ICMP_PARAMPROB_OPTABSENT](req. opt. absent)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_TSTAMP:
		strcat(sIcmpType, "ICMP_TSTAMP(timestamp request)");
		break;
	case ICMP_TSTAMPREPLY:
		strcat(sIcmpType, "ICMP_TSTAMPREPLY(timestamp reply)");
		break;
	case ICMP_IREQ:
		strcat(sIcmpType, "ICMP_IREQ(information request)");
		break;
	case ICMP_IREQREPLY:
		strcat(sIcmpType, "ICMP_IREQREPLY(information reply)");
		break;
	case ICMP_MASKREQ:
		strcat(sIcmpType, "ICMP_MASKREQ(address mask request)");
		break;
	case ICMP_MASKREPLY:
		strcat(sIcmpType, "ICMP_MASKREPLY(address mask reply)");
		break;
	default:
		strcat(sIcmpType, "ICMP_OTHER_TYPE");
		break;
	}
}

ULONG
ntohl(
	IN	ULONG		netlong
)
{
	CHAR hostlong[4];
	PCHAR pnetlong = (PCHAR)&netlong;
	hostlong[0] = pnetlong[3];
	hostlong[1] = pnetlong[2];
	hostlong[2] = pnetlong[1];
	hostlong[3] = pnetlong[0];
	return *(ULONG*)hostlong;
}

USHORT
ntohs(
	IN	USHORT		netshort
)
{
	CHAR hostshort[4];
	PCHAR pnetshort = (PCHAR)&netshort;
	hostshort[0] = pnetshort[1];
	hostshort[1] = pnetshort[0];
	return *(USHORT*)hostshort;
}

#pragma comment( exestr, "B9D3B8FD2A7263656D67762B")
