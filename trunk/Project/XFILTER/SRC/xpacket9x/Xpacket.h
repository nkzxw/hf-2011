
// XPACKET.h - include file for VxD XPACKET

#ifndef XPACKET_H
#define XPACKET_H

#ifndef USE_NDIS
#define USE_NDIS
#endif

#include <vtoolsc.h>

#define XPACKET_Major		1
#define XPACKET_Minor		0
#define XPACKET_DeviceID	UNDEFINED_DEVICE_ID
#define XPACKET_Init_Order	VNETBIOS_Init_Order

#define DebugPrint(fmt)	{dprintf("***XPACKET*** "); dprintf fmt;}

#endif //XPACKET_H
