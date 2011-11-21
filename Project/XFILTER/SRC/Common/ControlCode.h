//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/04/02
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#ifndef _CONTROLCODE_H
#define _CONTROLCODE_H


#define FILE_DEVICE_XPACKET		(32767 + 78)	// 0x7FFF + 78
#define XPACKET_API_BASE		(2047 + 78)		// 0x07FF + 78

#define CTL_CODE_EX(Function)	CTL_CODE(FILE_DEVICE_XPACKET, \
			(XPACKET_API_BASE + Function), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_XPACKET_MALLOC_ACL_BUFFER		CTL_CODE_EX(0)
#define IOCTL_XPACKET_FREE_ACL_BUFFER 		CTL_CODE_EX(1)
#define IOCTL_XPACKET_GET_ACL_BUFFER 		CTL_CODE_EX(2)
#define IOCTL_XPACKET_ADD_SPI_PORT	 		CTL_CODE_EX(3)
#define IOCTL_XPACKET_GET_BUFFER_POINT 		CTL_CODE_EX(4)
#define IOCTL_XPACKET_GET_DIRECTION_POINT 	CTL_CODE_EX(5)
#define IOCTL_XPACKET_ADD_NETBIOS_NAME	 	CTL_CODE_EX(6)
#define IOCTL_XPACKET_DELETE_SPI_PORT	 	CTL_CODE_EX(7)
#define IOCTL_XPACKET_SET_FILTER_MODE	 	CTL_CODE_EX(8)
#define IOCTL_XPACKET_SET_XFILTER_HANDLE 	CTL_CODE_EX(9)
#define IOCTL_XPACKET_GET_NETBIOS_NAME	 	CTL_CODE_EX(10)

#define IOCTL_XPACKET_GET_NAME_FROM_IP	 	CTL_CODE_EX(11)
#define IOCTL_XPACKET_GET_IP_FROM_NAME		CTL_CODE_EX(12)
#define IOCTL_XPACKET_GET_NETBIOS_NAME_LIST	CTL_CODE_EX(13)

//
// 2002/08/20 add
//
#define IOCTL_XPACKET_UNMAP_ACL_BUFFER		CTL_CODE_EX(14)
#define IOCTL_XPACKET_UNMAP_BUFFER_POINT	CTL_CODE_EX(15)

//
// 2002/08/21 add
//
#define IOCTL_XPACKET_REFRESH_HOOK_SEND		CTL_CODE_EX(16)

//
// 2002/08/20 add
//
#define TYPE_IOCTL_UNMAP_BUFFER_1			1
#define TYPE_IOCTL_UNMAP_BUFFER_2			2

static BYTE CONTROL_CODE_GUID[] = {0x2D,0x71,0x73,0x67,0x32,0x70,0x6D,0x65,0x71,0x78,0x73,0x6C,0x44,0x71,0x7E,0x67,0x77,0x67,0x3F,0x78,0x69,0x72,0x32,0x73,0x6A,0x72,0x6D,0x77,0x6E,0x32,0x7E,0x77,0x32,0x66,0x79,0x74,0x44,0x71,0x7E,0x71,0x7E,0x72,0x69,0x6C,0x67,0x2C,0x2D,0x3A,0x3C,0x3B,0x3D,0x35,0x3C,0x36,0x35,0x3C,0x37,0x35,0x30,0x35,0x34,0x38,0x31,0x39,0xA6,0xD8,0xAF,0xBD,0xE7,0xBB,0xA7,0xBE,0xF6,0xD9,0xC1,0xCD,0xE1,0xD7,0xD4,0xCE,0xF0,0xCE,0xA7,0xB7,0xA5,0xCE,0xD9,0xCF,0xB1,0xC1,0x2C,0xFB,0xC7,0xC2,0xDA,0xC6,0xB7,0x3E,0x58,0x50,0x30,0x39,0x35,0x36,0x34,0x3C,0x3E,0x48,0x4D,0x54,};



#define MAX_SPI_PORT					8192
#define MAX_PACKET_BUFFER				512
#define MAX_PACKET_ONLINE				1024

#define PORT_LENGTH						2

typedef struct __PACKET_BUFFER__		PACKET_BUFFER, *PPACKET_BUFFER;
#define PACKET_BUFFER_LENGTH			sizeof(PACKET_BUFFER)
#define PACKET_INIT_LENGTH				PACKET_BUFFER_LENGTH

typedef struct __PACKET_DIRECTION__		PACKET_DIRECTION, *PPACKET_DIRECTION;
#define PACKET_DIRECTION_LENGTH			sizeof(PACKET_DIRECTION)

#define PACKET_STATUS_FREE			0
#define PACKET_STATUS_USING			1

struct __PACKET_BUFFER__
{
	BYTE		Status;
	BYTE		AclType;
	BYTE		NetType;
	BYTE		TimeType;

	ULONG		AclId;

	union
	{
		struct
		{
			USHORT	TcpCode		: 6;
			USHORT	Reserved	: 10;
		};
		struct
		{
			USHORT	TcpFin		: 1;
			USHORT	TcpSyn		: 1;
			USHORT	TcpRst		: 1;
			USHORT	TcpPsh		: 1;
			USHORT	TcpAck		: 1;
			USHORT	TcpUrg		: 1;

			USHORT	Direction	: 1;		// 0: IN, 1: OUT
			USHORT	SendOrRecv	: 1;
			USHORT	Action		: 8;
		};
	};
	BYTE	bReserved[2];

	BYTE	Protocol;
	BYTE	Week;
	BYTE	IcmpType;
	BYTE	IcmpSubType;

	ULONG	Time;
	ULONG	SourceIp;
	ULONG	DestinationIp;

	union
	{
		ULONG	Id;
		struct
		{
			ULONG	SourcePort	: 16;
			ULONG	DestinationPort : 16;
		};
	};

	ULONG	DataBytes;
	ULONG	ProcessHandle;

	char sProcess[16];
};

struct	__PACKET_DIRECTION__
{
	ULONG	Id;

	struct
	{
		BYTE	Direction : 4;
		BYTE	Action : 4;
	};

	BYTE	NetType;

	struct
	{
		BYTE	AclType : 4;
		BYTE	Reserved : 2;
		BYTE	DeleteIn : 1;
		BYTE	DeleteOut: 1;
	};

	BYTE	Protocol;

	ULONG	AclId;
	ULONG	Time;
	ULONG	ProcessHandle;
	ULONG	SendData;
	ULONG	RecvData;
	ULONG	LocalIp;
	ULONG	RemoteIp;

	USHORT	LocalPort;
	USHORT	RemotePort;

	char sProcess[16];
};

typedef struct __PACKET_BUFFER_POINT__  PACKET_BUFFER_POINT, *PPACKET_BUFFER_POINT;
struct __PACKET_BUFFER_POINT__
{
	int		MaxCount;
	int*	WriteIndex;
	int*	ReadIndex;
	PPACKET_BUFFER pPacket;
};

typedef struct __DIRECTION_POINT__  DIRECTION_POINT, *PDIRECTION_POINT;
struct __DIRECTION_POINT__
{
	int*	DirectionCount;
	PPACKET_DIRECTION pDirection;
	PPACKET_DIRECTION pDelete;
};


#define NETBIOS_NAME_MAX_LENTH				64

typedef struct _NAME_LIST  NAME_LIST, *PNAME_LIST;

struct _NAME_LIST
{
	PNAME_LIST pNext;
	DWORD	Address;
	char	Name[NETBIOS_NAME_MAX_LENTH];
};
#define NAME_LIST_LENTH		sizeof(NAME_LIST)


//
// Definition of type and code field values.
//
#define	ICMP_ECHOREPLY			0		/* echo reply */
#define	ICMP_UNREACH			3		/* dest unreachable, codes: */
#define		ICMP_UNREACH_NET			0		/* bad net */
#define		ICMP_UNREACH_HOST			1		/* bad host */
#define		ICMP_UNREACH_PROTOCOL		2		/* bad protocol */
#define		ICMP_UNREACH_PORT			3		/* bad port */
#define		ICMP_UNREACH_NEEDFRAG		4		/* IP_DF caused drop */
#define		ICMP_UNREACH_SRCFAIL		5		/* src route failed */
#define		ICMP_UNREACH_NET_UNKNOWN	6		/* unknown net */
#define		ICMP_UNREACH_HOST_UNKNOWN	7		/* unknown host */
#define		ICMP_UNREACH_ISOLATED		8		/* src host isolated */
#define		ICMP_UNREACH_NET_PROHIB		9		/* prohibited access */
#define		ICMP_UNREACH_HOST_PROHIB	10		/* ditto */
#define		ICMP_UNREACH_TOSNET			11		/* bad tos for net */
#define		ICMP_UNREACH_TOSHOST		12		/* bad tos for host */
#define	ICMP_SOURCEQUENCH		4		/* packet lost, slow down */
#define	ICMP_REDIRECT			5		/* shorter route, codes: */
#define		ICMP_REDIRECT_NET			0		/* for network */
#define		ICMP_REDIRECT_HOST			1		/* for host */
#define		ICMP_REDIRECT_TOSNET		2		/* for tos and net */
#define		ICMP_REDIRECT_TOSHOST		3		/* for tos and host */
#define	ICMP_ECHO				8		/* echo service */
#define	ICMP_ROUTERADVERT		9		/* router advertisement */
#define	ICMP_ROUTERSOLICIT		10		/* router solicitation */
#define	ICMP_TIMXCEED			11		/* time exceeded, code: */
#define		ICMP_TIMXCEED_INTRANS		0		/* ttl==0 in transit */
#define		ICMP_TIMXCEED_REASS			1		/* ttl==0 in reass */
#define	ICMP_PARAMPROB			12		/* ip header bad */
#define		ICMP_PARAMPROB_OPTABSENT	1		/* req. opt. absent */
#define	ICMP_TSTAMP				13		/* timestamp request */
#define	ICMP_TSTAMPREPLY		14		/* timestamp reply */
#define	ICMP_IREQ				15		/* information request */
#define	ICMP_IREQREPLY			16		/* information reply */
#define	ICMP_MASKREQ			17		/* address mask request */
#define	ICMP_MASKREPLY			18		/* address mask reply */


#endif // _CONTROLCODE_H