/*
 *
 * $Id: ntddpack.h,v 1.3 2003/03/15 01:47:15 yjchen Exp $
 *
 *  Kernel Driver Communication with Application.
 *
 */

#include <ntddndis.h>

#ifndef __NTDDPACKET_H__
#define __NTDDPACKET_H__ 

#include <devioctl.h>

#define MAX_LINK_NAME_LENGTH   124

typedef struct _PACKET_OID_DATA {

    ULONG           Oid;
    ULONG           Length;
    UCHAR           Data[1];

} PACKET_OID_DATA, *PPACKET_OID_DATA;

#define FILE_DEVICE_PROTOCOL        0x8000

#define IOCTL_PROTOCOL_SET_OID      CTL_CODE(FILE_DEVICE_PROTOCOL, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTOCOL_QUERY_OID    CTL_CODE(FILE_DEVICE_PROTOCOL, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PROTOCOL_RESET        CTL_CODE(FILE_DEVICE_PROTOCOL, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENUM_ADAPTERS         CTL_CODE(FILE_DEVICE_PROTOCOL, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif //__NTDDPACKET_H__


