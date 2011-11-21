/*
	packet.h 
	Create date: 2001-12-25
*/
#ifndef PACKET_H
#define PACKET_H

//
// Ethernet header
//
typedef struct _ETHERNET_FRAME
{
	BYTE	DestinationAddress[6];					// 0xFFFFFF is Broadcast
	BYTE	SourceAddress[6];
	WORD	FrameType;								// in host-order

} EHTERNET_FRAME, *PETHERNET_FRAME;

#define ETHERNET_FRAME_LENGTH			14

#define ETHERNET_FRAME_TYPE_INVALID		0xFFFF		// Invalid Ethernet Frame
#define ETHERNET_FRAME_TYPE_TCPIP		0x0800		// TCP/IP Protocol
#define ETHERNET_FRAME_TYPE_PUP			0x0200		// PUP Protocol
#define ETHERNET_FRAME_TYPE_ARP			0x0806		// ARP protocol
#define ETHERNET_FRAME_TYPE_RARP		0x8035		// RAPR Protocol

//
// Ip header
//
typedef struct _IP_HEADER
{
	union
	{
		BYTE	VersionAndHeaderLength;				// Version 4 bit, Header Length 4 bit * 4
		struct
		{
			BYTE	HeaderLength : 4;
			BYTE	Version : 4;
		};
	};
	BYTE	TypeOfService;
	WORD	DatagramLength;
	WORD	Id;
	WORD	FlagsAndFragmentOffset;					// Flags 3 bit, Fragment Offset 13 bit
	BYTE	TimeToLive;
	BYTE	Protocol;
	WORD	CheckSum;
	BYTE	SourceIp[4];
	BYTE	DestinationIp[4];

} IP_HEADER, *PIP_HEADER;

#define IP_HEADER_LENGTH		20

#define	PROTOCOL_INVALID_IP		0xFF
#define	PROTOCOL_INVALID_TCP	0xFE
#define	PROTOCOL_INVALID_UDP	0xFD
#define	PROTOCOL_INVALID_ICMP	0xFC
#define	PROTOCOL_TCP			0x06
#define PROTOCOL_UDP			0x11
#define PROTOCOL_ICMP			0x01
#define PROTOCOL_IGMP			0x02

#define HEADER_LENGTH_MULTIPLE	4
#define GET_IP_VERSION(verlen)			((verlen & 0xF0) >> 4)
#define GET_IP_HEADER_LENGTH(verlen)	((verlen & 0x0F) * HEADER_LENGTH_MULTIPLE)
#define GET_IP_FLAGS(ffo)				((ffo & 0xE000) >> 13)
#define GET_IP_FRAGMENT_OFFSET(ffo)		(ffo & 0x1FFF)

//
// Tcp Header
//
typedef struct _TCP_HEADER
{
	WORD	SourcePort;
	WORD	DestinationPort;
	DWORD	SeqNumber;
	DWORD	AckNumber;
	union
	{
		WORD	LenAndCodeBits;		// Header length 4 bit, Reserved 6 bit, Code Bits 6 bit
		struct
		{
			WORD	Reserved1 : 8;
			WORD	TcpCode : 6;
			WORD	Reserved2 : 2;
		};
		struct
		{
			WORD	Reserved3 : 4;
			WORD	HeaderLength : 4;
			WORD	TcpFin : 1;
			WORD	TcpSyn : 1;
			WORD	TcpRst : 1;
			WORD	TcpPsh : 1;
			WORD	TcpAck : 1;
			WORD	TcpUrg : 1;
			WORD	Reserved4 : 2;
		};
	};
	WORD	Window;
	WORD	CheckSum;
	WORD	UrgentPointer;

} TCP_HEADER, *PTCP_HEADER;

#define TCP_HEADER_LENGTH			20

#define GET_TCP_HEADER_LENGTH(lcb)	(((lcb & 0xF000) >> 12) * HEADER_LENGTH_MULTIPLE)
#define GET_TCP_CODE_BITS(lcb)		(lcb & 0x003F)

//
// Udp Header
//
typedef struct _UDP_HEADER
{
	WORD	SourcePort;
	WORD	DestinationPort;
	WORD	Length;				// including this header 
	WORD	CheckSum;

} UDP_HEADER, *PUDP_HEADER;

#define UDP_HEADER_LENGTH		8

//
// Icmp Header
//
typedef struct _ICMP_HEADER
{
	BYTE	Type;
	BYTE	Code;				// type sub code
	WORD	CheckSum;
	WORD	ID;
	WORD	Seq;

} ICMP_HEADER, *PICMP_HEADER;

#define ICMP_HEADER_LENGTH	8

#define ICMP_NORMAL			0
#define ICMP_REQUEST		1
#define ICMP_RESPONSE		2


VOID 
PrintRecv(
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
);

VOID
PrintPacket(
	IN PNDIS_PACKET packet
);

WORD
PrintEthernetFrame(
	IN PETHERNET_FRAME pEthernetFrame,
	IN UINT Length
);

BYTE
PrintTcpIpHeader(
	IN PNDIS_BUFFER Buffer
);

INT
PrintIp(
	IN PIP_HEADER pIpHeader,
	IN UINT TotalLength
);

VOID
PrintTcp(
	IN PTCP_HEADER pTcpHeader
);

VOID
PrintUdp(
	IN PUDP_HEADER pUdpHeader
);

VOID
PrintIcmp(
	IN PICMP_HEADER pIcmpHeader
);




VOID
IcmpTypeToString(
	IN	BYTE		bIcmpType,
	IN	BYTE		bSubCode,
	OUT	PCHAR		sIcmpType
);

ULONG
ntohl(
	IN	ULONG		netlong
);

USHORT
ntohs(
	IN	USHORT		netshort
);


#endif //PACKET_H