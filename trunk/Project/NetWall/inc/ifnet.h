/*
 *
 * $Id: ifnet.h,v 1.0.0 2003/02/27 09:10:04 yjchen Exp $
 *
 *
 *  12-Mar-2003    1.0.4       Change Htonl/Htons to Macro HTONL/HTONS
 */

#ifndef __IFNET_H__
#define __IFNET_H__


#define	ETHER_ADDR_LENGTH		6
#define	ETHER_TYPE_LENGTH		2
#define ETHER_HEADER_LENGTH     14
#define	MAX_802_3_LENGTH		1500	// Maximum Value For 802.3 Length Field
#define	MAX_ETHER_SIZE			1514	// Maximum Ethernet Packet Length
#define	MIN_ETHER_SIZE	  		60		// Minimum Ethernet Packet Length

#define ARPHRD_ETHER 		    1	    /* ethernet hardware format */
#define ARPHRD_IEEE802		    6	    /* token-ring hardware format */
#define ARPHRD_FRELAY 		    15	    /* frame relay hardware format */

#define	ARPOP_REQUEST		    1	    /* request to resolve address */
#define	ARPOP_REPLY			    2	    /* response to previous request */
#define	ARPOP_REVREQUEST	    3	    /* request protocol address given hardware */
#define	ARPOP_REVREPLY		    4	    /* response giving protocol address */
#define ARPOP_INVREQUEST	    8 	    /* request to identify peer */
#define ARPOP_INVREPLY		    9	    /* response identifying peer */

#define PROTOCOL_IP			    0x0008
#define PROTOCOL_ARP		    0x0608

/* These definitions are copy from winsock.h   */
#define IPPROTO_ICMP            1       /* control message protocol */
#define IPPROTO_IGMP            2       /* group management protocol */
#define IPPROTO_GGP             3       /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6       /* tcp */
#define IPPROTO_PUP             12      /* pup */
#define IPPROTO_UDP             17      /* user datagram protocol */
#define IPPROTO_IDP             22      /* xns idp */
#define IPPROTO_ND              77      /* UNOFFICIAL net disk proto */

#define IPPROTO_RAW             255     /* raw IP packet */
#define IPPROTO_MAX             256

#pragma pack(1)
typedef struct _EthAddr 
{
	unsigned char       AddrByte[6]; 

}EthAddr, * PEthAddr; 

typedef struct _EthHead 
{
	EthAddr             DestAddr; 
	EthAddr             SourAddr; 
	unsigned short      SrvType; 

} EthHead, * PEthHead;  

typedef struct _IpHead 
{
	unsigned char       HeaderLengthVersion;// Version and length 
	unsigned char       TypeOfService;		// Type of service
	unsigned short      TotalLength;		// total length of the packet
	unsigned short      Identification;		// unique identifier
	unsigned short      FragmentationFlags; // flags
	unsigned char       TTL;				// Time To Live
	unsigned char       Protocol;           // protocol (TCP, UDP etc)
	unsigned short      CheckSum;			// IP Header checksum
	unsigned int        sourceIPAddress;	// Source address
	unsigned int        destIPAddress;		// Destination Address

} IpHead, *PIpHead;

typedef struct _TcpHead 
{
	unsigned short		usSrcPort;          // Source Port
	unsigned short		usDestPort;         // Destination Port
	unsigned long		ulSerialNo;	        // Number of Sequence
	unsigned long		ulAckNo;            // Number of aknowledge
    unsigned char       dataOffset;			// Pointer to data
    unsigned char       flags;				// Flags
    unsigned short      windows;			// Size of window
    unsigned short      checksum;			// Total checksum
    unsigned short      urgentPointer;		// Urgent pointer

} TcpHead, *PTcpHead;

typedef struct _UdpHead
{
	unsigned short      SourcePort;         // Source Port
	unsigned short      DestinationPort;    // Destination Port
	unsigned short      Length;	            // Total length
	unsigned short      Checksum;           // Total checksum
} UdpHead, *PUdpHead;

typedef struct	_EtherArp {
	EthHead			    ar_hdr;		/* header of ethernet frame	  */
	unsigned short	    ar_hrd;		/* format of hardware address */
	unsigned short	    ar_pro;		/* format of protocol address */
	unsigned char	    ar_hln;		/* length of hardware address */
	unsigned char	    ar_pln;		/* length of protocol address */
	unsigned short	    ar_op;		/* one of: */
	unsigned char	    ar_sha[6];	/* sender hardware address */
	unsigned long	    ar_spa;		/* sender protocol address */
	unsigned char	    ar_tha[6];	/* target hardware address */
	unsigned long	    ar_tpa;		/* target protocol address */
}EtherArp, * PEtherArp;

#pragma pack()

 
unsigned long  Htonl(unsigned long  iNum);
unsigned short Htons(unsigned short port);
 
unsigned short CheckSum(unsigned short  *addr, 
                        unsigned short  len
                       );
 
unsigned short PseudoCheckSum(unsigned short * HeaderAddr,
                              unsigned long    Saddr,
                              unsigned long    Daddr,
                              unsigned char    PType,
                              unsigned short   Lengthgth
                            );


#endif	/* __IFNET_H__ */

