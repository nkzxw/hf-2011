//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/31
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#ifndef __PACKETBUFFER_H__
#define __PACKETBUFFER_H__

extern int AddPort(USHORT Port);
extern int DeletePort(USHORT Port);
extern int FindPort(USHORT Port, int *Index);

extern int AddDirection(
	IN PPACKET_DIRECTION pPacketDirection
	);
extern int DeleteDirection(ULONG Id, BOOL DelIn, BOOL DelOut);
extern int FindDirection(ULONG Id, int *Index);
extern PPACKET_DIRECTION GetDirection(int iIndex);
extern BOOL GetDirectionPoint(PDIRECTION_POINT pDirectionPoint);

extern int ReadPacket(PPACKET_BUFFER pOutBuffer, int* Count);
extern PPACKET_BUFFER GetFreePacket();
extern BOOL GetPacketPoint(PPACKET_BUFFER_POINT pBufferPoint);

void InitBuffer();


#endif // __PACKETBUFFER_H__