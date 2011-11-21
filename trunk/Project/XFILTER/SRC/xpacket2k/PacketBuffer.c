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
// Author & Create Date: Tony Zhu, 2002/03/31
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
// 简介：
//		PacketBuffer.c 主要包含3个部分的功能代码：
//		1. 维护XFILTER.DLL(SPI)已经处理的端口列表，保证被SPI处理的端口这里不再处理。
//		2. 维护封包缓冲区，临时保存封包记录，XFILTER.EXE定时取走这些数据。
//		3. 维护当前连线记录缓冲区，驱动程序产生并但不负责删除，需要删除时，
//			会在删除表中添加需要删除记录的编号，由XFILTER.EXE完成删除。
//			XFILTER会定时检查缓冲区中的数据。
//
//
//
//

#include "xprecomp.h"
#pragma hdrstop

int				m_SpiPortCount = 0;
USHORT			m_SpiPort[MAX_SPI_PORT];

int*				m_WriteIndex = 0;
int*				m_ReadIndex = 0;
PPACKET_BUFFER		m_PacketBuffer;

int*				m_DirectionCount = NULL;
PPACKET_DIRECTION	m_PacketDirection = NULL;
PPACKET_DIRECTION	m_DeleteDirection = NULL;

PVOID m_BufferSystemVirtualAddress = NULL;
PMDL  m_pBufferMdl = NULL;

//
// 2002/08/20 add
//
PVOID m_UserAddress1 = NULL;
PVOID m_UserAddress2 = NULL;

//
// 将系统地址空间映射到用户地址空间
//
void* GetBufferUserMemory()
{
	if(m_pBufferMdl != NULL /*2002/08/11 add ->*/ && KeGetCurrentIrql() < DISPATCH_LEVEL)
	{
		//
		// 2002/08/19 设置 MDL_MAPPING_CAN_FAIL 标志，MmMapLockedPages 将停止进行BugCheck
		// 遇到错误时会返回 NULL，而不产生篮屏错误。
		//
		CSHORT oldfail = m_pBufferMdl->MdlFlags & MDL_MAPPING_CAN_FAIL;
		void* pUserAddress = NULL;
		m_pBufferMdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
		pUserAddress = MmMapLockedPages(m_pBufferMdl, UserMode);
		if(!pUserAddress)
			return NULL;
		if(!oldfail)
		  m_pBufferMdl->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;
		return pUserAddress;
	}
	else
		return NULL;
	return NULL;
}

//
// 2002/08/20 add 取消系统地址空间到用户地址空间的映射
//
void UnmapMemory(DWORD Type)
{
	if(m_pBufferMdl != NULL && KeGetCurrentIrql() < DISPATCH_LEVEL) //2002/08/11 add
	{
		if(Type == TYPE_IOCTL_UNMAP_BUFFER_1 && m_UserAddress1 != NULL)
			MmUnmapLockedPages(m_UserAddress1, m_pBufferMdl);
		if(Type == TYPE_IOCTL_UNMAP_BUFFER_2 && m_UserAddress2 != NULL)
			MmUnmapLockedPages(m_UserAddress2, m_pBufferMdl);
	}
}


//
// 释放系统内存空间
//
void FreeBufferShareMemory()	
{
	if(m_pBufferMdl != NULL)
	{
		//
		// 2002/08/11 remove，因为停止使用 MmProbeAndLockPages
		//
		//MmUnlockPages(m_pBufferMdl);

		IoFreeMdl(m_pBufferMdl);
		m_pBufferMdl = NULL;
	}
	if(m_BufferSystemVirtualAddress != NULL)
	{
		ExFreePool(m_BufferSystemVirtualAddress);
		m_BufferSystemVirtualAddress = NULL;
	}
}

//
// 申请一个位于系统地址空间的内存
//
void* CreateBufferShareMemory(int NumberOfBytes)	
{
	//
	// 2002/08/11 修改，为了使空间可以在DISPATCH_LEVEL下使用
	//
	//m_BufferSystemVirtualAddress = ExAllocatePool(PagedPool, NumberOfBytes); 
	m_BufferSystemVirtualAddress = ExAllocatePool(NonPagedPool, NumberOfBytes);
	
	dprintf(("m_BufferSystemVirtualAddress: 0x%08X\n", m_BufferSystemVirtualAddress));
	if(m_BufferSystemVirtualAddress == NULL) 
		return NULL;
	m_pBufferMdl = IoAllocateMdl(m_BufferSystemVirtualAddress, NumberOfBytes, FALSE, FALSE, NULL); 
	dprintf(("m_pBufferMdl: 0x%08X\n", m_pBufferMdl));
	if(m_pBufferMdl == NULL)
	{
		FreeBufferShareMemory();
		return NULL;
	}

	//
	// 2002/08/11 修改，因为申请了NonPagedPool空间
	//
	//MmProbeAndLockPages(m_pBufferMdl, KernelMode, IoWriteAccess);
	MmBuildMdlForNonPagedPool(m_pBufferMdl);

	return m_BufferSystemVirtualAddress;
}


//
// 初始化缓冲区
//
BOOL InitBuffer()
{
	int CreateBufferLength = sizeof(int) * 3
		+ MAX_PACKET_BUFFER * PACKET_BUFFER_LENGTH 
		+ (MAX_PACKET_ONLINE + 100) * PACKET_DIRECTION_LENGTH;
	dprintf(("CreateBufferLength: %d\n", CreateBufferLength));
	CreateBufferShareMemory(CreateBufferLength);
	if(m_BufferSystemVirtualAddress == NULL)
		return FALSE;
	dprintf(("<== m_BufferSystemVirtualAddress: 0x%08X\n", m_BufferSystemVirtualAddress));

	m_WriteIndex = (int*)m_BufferSystemVirtualAddress;
	m_ReadIndex = m_WriteIndex + 1;
	m_DirectionCount = m_WriteIndex + 2;
	m_PacketBuffer = (PPACKET_BUFFER)(m_WriteIndex + 3);
	m_PacketDirection = (PPACKET_DIRECTION)
		((char*)m_PacketBuffer + (MAX_PACKET_BUFFER * PACKET_BUFFER_LENGTH));
	m_DeleteDirection = (PPACKET_DIRECTION)
		((char*)m_PacketDirection + (MAX_PACKET_ONLINE * PACKET_DIRECTION_LENGTH));

	m_SpiPortCount		= 0;
	(*m_WriteIndex)		= 0;
	(*m_ReadIndex)		= 0;
	(*m_DirectionCount) = 0;

	memset(m_SpiPort, 0, MAX_SPI_PORT * PORT_LENGTH);
	memset(m_PacketBuffer, 0, MAX_PACKET_BUFFER * PACKET_BUFFER_LENGTH);
	memset(m_PacketDirection, 0, MAX_PACKET_ONLINE * PACKET_DIRECTION_LENGTH);
	memset(m_DeleteDirection, 0, 100 * PACKET_DIRECTION_LENGTH);

	return TRUE;
}

//===========================================================================

//
// 排序增加SPI端口，由XFILTER.DLL通过DeviceIoControl调用
//
int AddPort(USHORT Port)
{
	int Index, MoveCount;
	if(Port == 0 || FindPort(Port, &Index) || Index >= MAX_SPI_PORT)
		return -1;
	MoveCount = m_SpiPortCount - Index;
	if(MoveCount > 0)
		memcpy(m_SpiPort + Index + 1, m_SpiPort + Index, MoveCount * PORT_LENGTH);
	m_SpiPort[Index] = Port;
	m_SpiPortCount++;
	return Index;
}

//
// 删除SPI端口，由XFILTER.DLL通过DeviceIoControl调用
//
int DeletePort(USHORT Port)
{
	int Index, MoveCount;
	if(Port == 0 || !FindPort(Port, &Index))
		return -1;
	MoveCount = m_SpiPortCount - Index - 1;
	if(MoveCount > 0)
		memcpy(m_SpiPort + Index, m_SpiPort + Index + 1, MoveCount * PORT_LENGTH);
	else
		m_SpiPort[Index] = 0;
	m_SpiPortCount--;
	return 0;
}

//
// 利用二分法查找SPI端口
//
int FindPort(USHORT Port, int *Index)
{
	int low, hig, mid, Find;

	*Index = 0;
	low = 0;
	hig = m_SpiPortCount == 0 ? 0 : m_SpiPortCount - 1;
	mid = 0;
	Find = 0;

	while(low < hig)
	{
		mid = (low + hig) / 2;
		if(Port == m_SpiPort[mid])
		{
			Find = 1;
			*Index = mid;
			return Find;
		}
		if(Port < m_SpiPort[mid])
			hig = mid - 1;
		else if(Port > m_SpiPort[mid])
			low = mid + 1;
	}

	if(Port == m_SpiPort[low])
	{
		Find = 1;
		*Index = low;
	}
	else
	{
		if(m_SpiPort[hig] == 0)
			*Index = 0;
		else if(Port > m_SpiPort[hig])
			*Index = hig + 1;
		else
			*Index = hig;
	}
	return Find;
}



//===========================================================================

//
// 按ID排序增加当前连线记录
//
int AddDirection(
	IN PPACKET_DIRECTION pPacketDirection
)
{
	int Index, MoveCount;
	dprintf(("-->AddDirection, CurrentCount: %d, ID:%u\n", *m_DirectionCount, pPacketDirection->Id));
	if(pPacketDirection == NULL || pPacketDirection->Id == 0)
		return -1;
	if(FindDirection(pPacketDirection->Id, &Index))
	{
		m_PacketDirection[Index] = *pPacketDirection;
		return Index;
	}
	if(Index >= MAX_PACKET_ONLINE)
		return -1;
	MoveCount = *m_DirectionCount - Index;
	dprintf(("-->AddDirection, Index:%d, MoveCount:%d\n", Index, MoveCount));
	if(MoveCount > 0)
	{
		memcpy(m_PacketDirection + Index + 1
			, m_PacketDirection + Index
			, MoveCount * PACKET_DIRECTION_LENGTH);
	}
	m_PacketDirection[Index] = *pPacketDirection;

	(*m_DirectionCount)++;
	dprintf(("<--AddDirection, %d\n", (*m_DirectionCount)));
	return Index;
}

//
// 删除当前连线记录
//
int DeleteDirection(ULONG Id, BOOL DelIn, BOOL DelOut)
{
	int Index, MoveCount, i;
	dprintf(("-->DeleteDirection, DelIn(%d), DelOut(%d), Id(%u)\n", DelIn, DelOut, Id));
	if(Id == 0 || !FindDirection(Id, &Index))
		return -1;

	dprintf(("-->DeleteDirection, Index:%d\n", Index));

	if(DelIn)
		m_PacketDirection[Index].DeleteIn = DelIn;
	if(DelOut)
		m_PacketDirection[Index].DeleteOut = DelOut;

	if(!m_PacketDirection[Index].DeleteIn
		|| !m_PacketDirection[Index].DeleteOut)
		return -1;

	for(i = 0; i < MAX_PACKET_ONLINE; i++)
	{
		if(m_DeleteDirection[i].Id == 0)
		{
			m_DeleteDirection[i] = m_PacketDirection[Index];
			break;
		}
	}

	MoveCount = (*m_DirectionCount) - Index - 1;
	dprintf(("-->DeleteDirection, MoveCount:%d, Id:%u\n", MoveCount, m_PacketDirection[Index].Id));
	if(MoveCount > 0)
	{
		memcpy(m_PacketDirection + Index
			, m_PacketDirection + Index + 1
			, MoveCount * PACKET_DIRECTION_LENGTH);
	}
	else
	{
		m_PacketDirection[Index].Id = 0;
	}
	(*m_DirectionCount)--;
	dprintf(("<--DeleteDirection, %d\n", (*m_DirectionCount)));
	return 0;
}

//
// 按ID用二分法查找连线记录
//
int FindDirection(ULONG Id, int *Index)
{
	int low, hig, mid, Find;

	*Index = 0;
	low = 0;
	hig = (*m_DirectionCount) == 0 ? 0 : (*m_DirectionCount) - 1;
	mid = 0;
	Find = 0;

	while(low < hig)
	{
		mid = (low + hig) / 2;
		if(Id == m_PacketDirection[mid].Id)
		{
			Find = 1;
			*Index = mid;
			return Find;
		}
		if(Id < m_PacketDirection[mid].Id)
			hig = mid - 1;
		else if(Id > m_PacketDirection[mid].Id)
			low = mid + 1;
	}
	if(Id == m_PacketDirection[low].Id)
	{
		Find = 1;
		*Index = low;
	}
	else
	{
		if(m_PacketDirection[hig].Id == 0)
			*Index = 0;
		else if(Id > m_PacketDirection[hig].Id)
			*Index = hig + 1;
		else
			*Index = hig;
	}
	return Find;
}

//
// 得到空闲的连线记录缓冲区
//
PPACKET_DIRECTION GetDirection(int iIndex)
{
	if(iIndex < 0) return 0;
	return &m_PacketDirection[iIndex];
}

//
// 返回当前连线记录缓冲区指针、删除记录缓冲区指针和当前连线个数
// XFILTER.EXE通过DeviceIoControl调用得到这些缓冲区指针，进而维护
// 这些数据
//
BOOL GetDirectionPoint(PDIRECTION_POINT pDirectionPoint)
{
	PVOID pUserAddress = GetBufferUserMemory();
	if(pUserAddress == NULL)
		return FALSE;

	//
	// 2002/08/20 add
	//
	m_UserAddress2 = pUserAddress;

	pDirectionPoint->DirectionCount = (int*)(
		(DWORD)pUserAddress + ((DWORD)m_DirectionCount - (DWORD)m_BufferSystemVirtualAddress)
		);
	pDirectionPoint->pDirection = (PPACKET_DIRECTION)(
		(DWORD)pUserAddress + ((DWORD)m_PacketDirection - (DWORD)m_BufferSystemVirtualAddress)
		);
	pDirectionPoint->pDelete = (PPACKET_DIRECTION)(
		(DWORD)pUserAddress + ((DWORD)m_DeleteDirection - (DWORD)m_BufferSystemVirtualAddress)
		);
	return TRUE;
}



//===========================================================================
// 封包缓冲区处理函数。封包缓冲区的处理方法是循环利用封包缓冲区，即按照缓冲区
// 的索引逐个写入，当达到最大索引时在返回到缓冲区头。如果在这期间XFILTER.EXE
// 一直没有取出封包，则老的封包被覆盖。
// 维护两个计数器，m_WriteIndex用来保存下一条写入的索引，m_ReadIndex记录下一条
// 读取记录的索引。
//

//
// 读出一条封包记录保存到pOutBuffer缓冲区。
//
int ReadOnePacket(PPACKET_BUFFER pOutBuffer)
{
	if(pOutBuffer == 0 || (*m_ReadIndex) == (*m_WriteIndex)) 
		return 0;
	*pOutBuffer = m_PacketBuffer[(*m_ReadIndex)];
	(*m_ReadIndex)++;
	if((*m_ReadIndex) >= MAX_PACKET_BUFFER)
		(*m_ReadIndex) = 0;
	return 1;
}

//
// 读出一组记录保存到pOutBuffer缓冲区, *Count为读出记录的个数
//
int ReadPacket(PPACKET_BUFFER pOutBuffer, int* Count)
{
	int i;
	if(pOutBuffer == NULL || Count == NULL) 
		return -1;
	for(i = 0; i < *Count; i++)
	{
		if(!ReadOnePacket(pOutBuffer + i)) 
			break;
	}
	return i;
}

//
// 得到一个当前空闲的缓冲区指针。
//
PPACKET_BUFFER GetFreePacket()
{
	int iFree;
	dprintf(("GetFreePacket: %d\n", (*m_WriteIndex)));
	iFree = (*m_WriteIndex);
	(*m_WriteIndex)++;
	if((*m_WriteIndex) >= MAX_PACKET_BUFFER)
		(*m_WriteIndex) = 0;
	if((*m_WriteIndex) == (*m_ReadIndex))
	{
		int iPool;
		iPool = (*m_ReadIndex) + 100;
		if(iPool >= MAX_PACKET_BUFFER)
			iPool = iPool - MAX_PACKET_BUFFER;
		(*m_ReadIndex) = iPool;
	}
	memset(&m_PacketBuffer[iFree], 0, PACKET_INIT_LENGTH);
	return &m_PacketBuffer[iFree];
}

//
// 返回封包缓冲区可以容纳封包的个数、读写计数器的指针和缓冲区的基地址
// XFILTER.EXE通过DeviceIoControl调用这个函数得到这些值，以便维护封包
// 数据。
//
BOOL GetPacketPoint(PPACKET_BUFFER_POINT pBufferPoint)
{
	PVOID pUserAddress = GetBufferUserMemory();
	if(pUserAddress == NULL)
		return FALSE;

	//
	// 2002/08/20 add
	//
	m_UserAddress1 = pUserAddress;

	dprintf(("pUserAddress: 0x%08X, System: 0x%08X, m_ReadIndex: 0x%08X, \
			m_WriteIndex: 0x%08X, m_PacketBuffer: 0x%08X\n"
		, pUserAddress
		, m_BufferSystemVirtualAddress
		, m_ReadIndex
		, m_WriteIndex
		, m_PacketBuffer
		));

	pBufferPoint->MaxCount = MAX_PACKET_BUFFER;
	pBufferPoint->ReadIndex = (int*)(
		(DWORD)pUserAddress + ((DWORD)m_ReadIndex - (DWORD)m_BufferSystemVirtualAddress)
		);
	pBufferPoint->WriteIndex = (int*)(
		(DWORD)pUserAddress + ((DWORD)m_WriteIndex - (DWORD)m_BufferSystemVirtualAddress)
		);
	pBufferPoint->pPacket = (PPACKET_BUFFER)(
		(DWORD)pUserAddress + ((DWORD)m_PacketBuffer - (DWORD)m_BufferSystemVirtualAddress)
		);

	dprintf(("pUserAddress: 0x%08X, System: 0x%08X, m_ReadIndex: 0x%08X, \
			m_WriteIndex: 0x%08X, m_PacketBuffer: 0x%08X\n"
		, pUserAddress
		, m_BufferSystemVirtualAddress
		, pBufferPoint->ReadIndex
		, pBufferPoint->WriteIndex
		, pBufferPoint->pPacket
		));

	return TRUE;
}

#pragma comment( exestr, "B9D3B8FD2A7263656D67766477686867742B")
