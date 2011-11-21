//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/04/02
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
//

#ifndef MEMORYACL_H
#define MEMORYACL_H

//
// 2002/05/24 add
//
BOOL RefenceAclCount();
void DerefenceAclCount();

extern PVOID m_SystemVirtualAddress;

extern void SetFilterMode(DWORD IsFilter);
extern void* GetBuffer(DWORD bIsCreateProcess);
extern void* CreateMemory(int nPageSize);
extern int FreeMemory();

//
// 2002/08/20 add
//
extern void FreeUserMemory(PVOID UserVirtualAddress);

//
// 2002/05/30 add
//
extern int CheckPacket(
	IN PNDIS_PACKET packet,
	IN BOOL IsSend
	);

extern int CheckSend(
	IN PNDIS_PACKET packet
	);
extern int CheckRecv(
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
	);

int CheckTcp(
	PIP_HEADER pIpHeader, 
	PTCP_HEADER pTcpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize,
	void* pVoid
	);
int CheckUdp(
	PIP_HEADER pIpHeader, 
	PUDP_HEADER pUdpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize,
	void* pVoid 
	);
int CheckIcmp(
	PIP_HEADER pIpHeader, 
	PICMP_HEADER pIcmpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize 
	);

int CheckApp(PPACKET_BUFFER pPacketBuffer);
int CheckNnb(PPACKET_BUFFER pPacketBuffer);
int CheckIcmpAcl(PPACKET_BUFFER pPacketBuffer);
int CheckTrojan();
int ProtectApp();
BOOL CheckGuid();

#endif // MEMORYACL_H