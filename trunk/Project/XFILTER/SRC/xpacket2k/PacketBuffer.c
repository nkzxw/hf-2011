//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

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
// ��飺
//		PacketBuffer.c ��Ҫ����3�����ֵĹ��ܴ��룺
//		1. ά��XFILTER.DLL(SPI)�Ѿ�����Ķ˿��б���֤��SPI����Ķ˿����ﲻ�ٴ���
//		2. ά���������������ʱ��������¼��XFILTER.EXE��ʱȡ����Щ���ݡ�
//		3. ά����ǰ���߼�¼�����������������������������ɾ������Ҫɾ��ʱ��
//			����ɾ�����������Ҫɾ����¼�ı�ţ���XFILTER.EXE���ɾ����
//			XFILTER�ᶨʱ��黺�����е����ݡ�
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
// ��ϵͳ��ַ�ռ�ӳ�䵽�û���ַ�ռ�
//
void* GetBufferUserMemory()
{
	if(m_pBufferMdl != NULL /*2002/08/11 add ->*/ && KeGetCurrentIrql() < DISPATCH_LEVEL)
	{
		//
		// 2002/08/19 ���� MDL_MAPPING_CAN_FAIL ��־��MmMapLockedPages ��ֹͣ����BugCheck
		// ��������ʱ�᷵�� NULL������������������
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
// 2002/08/20 add ȡ��ϵͳ��ַ�ռ䵽�û���ַ�ռ��ӳ��
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
// �ͷ�ϵͳ�ڴ�ռ�
//
void FreeBufferShareMemory()	
{
	if(m_pBufferMdl != NULL)
	{
		//
		// 2002/08/11 remove����Ϊֹͣʹ�� MmProbeAndLockPages
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
// ����һ��λ��ϵͳ��ַ�ռ���ڴ�
//
void* CreateBufferShareMemory(int NumberOfBytes)	
{
	//
	// 2002/08/11 �޸ģ�Ϊ��ʹ�ռ������DISPATCH_LEVEL��ʹ��
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
	// 2002/08/11 �޸ģ���Ϊ������NonPagedPool�ռ�
	//
	//MmProbeAndLockPages(m_pBufferMdl, KernelMode, IoWriteAccess);
	MmBuildMdlForNonPagedPool(m_pBufferMdl);

	return m_BufferSystemVirtualAddress;
}


//
// ��ʼ��������
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
// ��������SPI�˿ڣ���XFILTER.DLLͨ��DeviceIoControl����
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
// ɾ��SPI�˿ڣ���XFILTER.DLLͨ��DeviceIoControl����
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
// ���ö��ַ�����SPI�˿�
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
// ��ID�������ӵ�ǰ���߼�¼
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
// ɾ����ǰ���߼�¼
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
// ��ID�ö��ַ��������߼�¼
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
// �õ����е����߼�¼������
//
PPACKET_DIRECTION GetDirection(int iIndex)
{
	if(iIndex < 0) return 0;
	return &m_PacketDirection[iIndex];
}

//
// ���ص�ǰ���߼�¼������ָ�롢ɾ����¼������ָ��͵�ǰ���߸���
// XFILTER.EXEͨ��DeviceIoControl���õõ���Щ������ָ�룬����ά��
// ��Щ����
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
// ���������������������������Ĵ�������ѭ�����÷���������������ջ�����
// ���������д�룬���ﵽ�������ʱ�ڷ��ص�������ͷ����������ڼ�XFILTER.EXE
// һֱû��ȡ����������ϵķ�������ǡ�
// ά��������������m_WriteIndex����������һ��д���������m_ReadIndex��¼��һ��
// ��ȡ��¼��������
//

//
// ����һ�������¼���浽pOutBuffer��������
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
// ����һ���¼���浽pOutBuffer������, *CountΪ������¼�ĸ���
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
// �õ�һ����ǰ���еĻ�����ָ�롣
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
// ���ط���������������ɷ���ĸ�������д��������ָ��ͻ������Ļ���ַ
// XFILTER.EXEͨ��DeviceIoControl������������õ���Щֵ���Ա�ά�����
// ���ݡ�
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
