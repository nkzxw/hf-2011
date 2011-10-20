#pragma once
/*

  DrvFltIp.H

  

*/
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
//Include This file for the definitions of the CTL_CODE and extra
#include <winioctl.h>

#define FILE_DEVICE_DRVFLTIP	0x00654322

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//
#define DRVFLTIP_IOCTL_INDEX	0x830

//
//The MONO device driver IOCTLs
//
#define START_IP_HOOK	CTL_CODE(FILE_DEVICE_DRVFLTIP, DRVFLTIP_IOCTL_INDEX, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define STOP_IP_HOOK	CTL_CODE(FILE_DEVICE_DRVFLTIP, DRVFLTIP_IOCTL_INDEX + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define ADD_FILTER		CTL_CODE(FILE_DEVICE_DRVFLTIP, DRVFLTIP_IOCTL_INDEX + 2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
#define CLEAR_FILTER	CTL_CODE(FILE_DEVICE_DRVFLTIP, DRVFLTIP_IOCTL_INDEX + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define READ_FILTER		CTL_CODE(FILE_DEVICE_DRVFLTIP, DRVFLTIP_IOCTL_INDEX + 4, METHOD_BUFFERED, FILE_READ_ACCESS)

//struct to define filter rules

/*********************************************
 *struct to define filter rules
 *
 * @author 
 ********************************************/
typedef struct	filter
{
	/*********************************************
	 * protocol used
	 ********************************************/
	USHORT	protocol;			//protocol used

	/*********************************************
	 *source ip address
	 ********************************************/
	ULONG	sourceIp;			//source ip address

	/*********************************************
	 * destination ip address
	 ********************************************/
	ULONG	destinationIp;		//destination ip address

	/*********************************************
	 *source mask
	 ********************************************/
	ULONG	sourceMask;			//source mask

	/*********************************************
	 * destination mask
	 ********************************************/
	ULONG	destinationMask;	//destination mask

	/*********************************************
	 * source port number
	 ********************************************/
	USHORT	sourcePort;			//source port

	/*********************************************
	 * destination port number
	 ********************************************/
	USHORT	destinationPort;	//destination port

	/*********************************************
	 * if true, the packet will be drop, otherwise the packet pass
	 ********************************************/
	BOOLEAN drop;				//if true, the packet will be drop, otherwise the packet pass
} IPFilter;

//struct to build a linked list

/*********************************************
 * struct to build a linked list 
 *
 * @author 
 ********************************************/
struct filterList
{
	IPFilter			ipf;

	struct filterList	*next;
};

//Ip Header

/*********************************************
 * Ip Header
 *
 * @author 
 ********************************************/
typedef struct	IPHeader
{
	/*********************************************
     * Version and length 
     ********************************************/
	UCHAR	iphVerLen;		// Version and length

	/*********************************************
     * Type of service 
     ********************************************/
	UCHAR	ipTOS;			// Type of service

	/*********************************************
     * Total datagram length 
     ********************************************/
	USHORT	ipLength;		// Total datagram length

	/*********************************************
     *Identification 
     ********************************************/
	USHORT	ipID;			// Identification

	/*********************************************
     *  Flags
     ********************************************/
	USHORT	ipFlags;		// Flags

	/*********************************************
     * Time to live 
     ********************************************/
	UCHAR	ipTTL;			// Time to live

	/*********************************************
     * Protocol 
     ********************************************/
	UCHAR	ipProtocol;		// Protocol

	/*********************************************
     *  Header checksum 
     ********************************************/
	USHORT	ipChecksum;		// Header checksum

	/*********************************************
     *  Source address 
     ********************************************/
	ULONG	ipSource;		// Source address

	/*********************************************
     * Destination address 
     ********************************************/
	ULONG	ipDestination;	// Destination address
} IPPacket;

//TCP Header

/*********************************************
 * TCP Header
 *
 * @author 
 ********************************************/
typedef struct	_TCPHeader
{
	/*********************************************
	 * Source Port
	 ********************************************/
	USHORT	sourcePort;			// Source Port

	/*********************************************
	 * Destination Port
	 ********************************************/
	USHORT	destinationPort;	// Destination Port

	/*********************************************
	 * Number of Sequence
	 ********************************************/
	ULONG	sequenceNumber;		// Number of Sequence

	/*********************************************
	 * Number of aknowledge
	 ********************************************/
	ULONG	acknowledgeNumber;	// Number of aknowledge

	/*********************************************
	 * Pointer to data
	 ********************************************/
	UCHAR	dataoffset;			// Pointer to data

	/*********************************************
	 * Flags
	 ********************************************/
	UCHAR	flags;				// Flags

	/*********************************************
	 *Size of window
	 ********************************************/
	USHORT	windows;			// Size of window

	/*********************************************
	 *Total checksum
	 ********************************************/
	USHORT	checksum;			// Total checksum

	/*********************************************
	 *  Urgent pointer
	 ********************************************/
	USHORT	urgentPointer;		// Urgent pointer
} TCPHeader;

//UDP Header

/*********************************************
 * UDP Header
 *
 * @author 
 ********************************************/
typedef struct	_UDPHeader
{
	/*********************************************
	 *Source Port
	 ********************************************/
	USHORT	sourcePort;			// Source Port

	/*********************************************
	 * Destination Port
	 ********************************************/
	USHORT	destinationPort;	// Destination Port

	/*********************************************
	 * Total length
	 ********************************************/
	USHORT	len;				// Total length

	/*********************************************
	 * Total checksum
	 ********************************************/
	USHORT	checksum;			// Total checksum
} UDPHeader;
