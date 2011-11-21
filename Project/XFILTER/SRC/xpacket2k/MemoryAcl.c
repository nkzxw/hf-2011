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
// Author & Create Date: Tony Zhu, 2002/04/02
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
//
// 简介：
//		MemoryAcl.c模块主要完成三个方便的功能：
// 
//		1. 申请保存控管规则的内存空间，并在XFILTER.EXE和XFILTER.DLL之间共享
//		2. 对NdisSend和ProtocolReceive的封包进行解析，解析的封包有 Ethernet 数据包头、
//			IP头、TCP头、UDP头、ICMP包，另外调用NetBios.c的程序对NetBios的名字进行解析
//		3. 控管规则的比对
//
//
//

#include "xprecomp.h"
#include "..\common\filt.h"
#pragma hdrstop

PVOID m_SystemVirtualAddress = NULL;
PMDL m_Mdl = NULL;
PVOID m_UserCreateAddress = NULL;
DWORD m_dwOffset = 0;

PXACL_HEADER m_pAclHeader = 0;
BOOL m_IsFilter = FALSE;
int m_PageSize = 0;

//==============================================================================
// 申请保存控管规则的内存空间，并在XFILTER.EXE和XFILTER.DLL之间共享的相关函数
//

//
// 输出MDL结构的数据
//
void PrintMdl(PMDL pMdl)
{
	if(pMdl == NULL)
		return;
	dprintf(("Next: 0x%08X, Size: %u, MdlFlags, 0x%08X, pProcess: 0x%08X, \
			MappedSystemVa: 0x%08X, StartVa: 0x%08X, ByteCount: %u, ByteOffset: 0x%08X\n"
				, pMdl->Next
				, pMdl->Size
				, pMdl->MdlFlags
				, pMdl->Process
				, pMdl->MappedSystemVa
				, pMdl->StartVa
				, pMdl->ByteCount
				, pMdl->ByteOffset
			));
}

//
// 将系统地址空间映射到用户地址空间
//
void* GetUserMemory()
{
	if(m_Mdl != NULL /*2002/08/11 add ->*/ && KeGetCurrentIrql() < DISPATCH_LEVEL)
	{
		//
		// 2002/08/19 设置 MDL_MAPPING_CAN_FAIL 标志，MmMapLockedPages 将停止进行BugCheck
		// 遇到错误时会返回 NULL，而不产生篮屏错误。
		//
		CSHORT oldfail = m_Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL;
		void* pUserAddress = NULL;
		m_Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
		pUserAddress = MmMapLockedPages(m_Mdl, UserMode);
		if(!pUserAddress)
			return NULL;
		if(!oldfail)
		  m_Mdl->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;
		return pUserAddress;
	}
	else
		return NULL;
	return NULL;
}

//
// 取消系统地址空间到用户地址空间的映射
//
void FreeUserMemory(PVOID UserVirtualAddress)
{
	if(m_Mdl != NULL
		&& UserVirtualAddress != NULL			//2002/08/20 add 
		&& KeGetCurrentIrql() < DISPATCH_LEVEL) //2002/08/11 add
		MmUnmapLockedPages(UserVirtualAddress, m_Mdl);
}

//
// 释放系统内存空间
//
void FreeShareMemory(PVOID pVoid)	
{
	if(m_Mdl != NULL)
	{
		//
		// 2002/08/11 remove，因为停止使用 MmProbeAndLockPages
		//
		// MmUnlockPages(m_Mdl);

		IoFreeMdl(m_Mdl);
		m_Mdl = NULL;
	}
	if(m_SystemVirtualAddress != NULL)
	{
		ExFreePool(m_SystemVirtualAddress);
		m_SystemVirtualAddress = NULL;
	}
}

//
// 申请一个位于系统地址空间的内存
//
void* CreateShareMemory(int NumberOfBytes)	
{
	//
	// 2002/08/11 修改，为了使空间可以在DISPATCH_LEVEL下使用
	//
	//m_SystemVirtualAddress = ExAllocatePool(PagedPool, NumberOfBytes); 
	m_SystemVirtualAddress = ExAllocatePool(NonPagedPool, NumberOfBytes); 

	if(m_SystemVirtualAddress == NULL) 
		return NULL;
	m_Mdl = IoAllocateMdl(m_SystemVirtualAddress, NumberOfBytes, FALSE, FALSE, NULL); 
	if(m_Mdl == NULL)
	{
		FreeShareMemory(NULL);
		return NULL;
	}
	PrintMdl(m_Mdl);

	//
	// 2002/08/11 修改，因为申请了NonPagedPool空间
	//
	//MmProbeAndLockPages(m_Mdl, KernelMode, IoWriteAccess); 
	MmBuildMdlForNonPagedPool(m_Mdl);

	PrintMdl(m_Mdl);
	return m_SystemVirtualAddress;
}


//
// 设置过滤模式
//		TRUE:	启动过滤
//		FALSE:	停止过滤
//
void SetFilterMode(DWORD bFilterMode)
{
	if(bFilterMode == 0)
	{
		//
		// 2002/05/24 add
		//
		if(RefenceAclCount())
		{
			m_pAclHeader->bWorkMode = XF_PASS_ALL;
			DerefenceAclCount();
		}

		m_IsFilter = FALSE;
	}
	else
	{
		m_IsFilter = TRUE;
	}
}

//
// 得到控管规则的内存地址。
//		被XFILTER.EXE和XFILTER.DLL通过DeviceIoControl调用
//		得到控管规则内存缓冲区的地址。
//
void* GetBuffer(DWORD bIsCreateProcess)
{
	PrintMdl(m_Mdl);
	if(bIsCreateProcess == 0)
		return GetUserMemory();

	m_UserCreateAddress = GetUserMemory();
	PrintMdl(m_Mdl);
	m_dwOffset = (DWORD)m_pAclHeader - (DWORD)m_UserCreateAddress;
	return m_UserCreateAddress;
}

//
// 为控管规则申请内存空间。
// 参数：
//		nPageSize: 申请内存空间的大小
//
void* CreateMemory(int nPageSize)
{
	void* pBuffer;
	if(m_pAclHeader != 0 && nPageSize == m_PageSize) 
		return GetUserMemory();
	if(m_SystemVirtualAddress != NULL)
		FreeShareMemory(NULL);
	m_pAclHeader = 0;
	m_UserCreateAddress = 0;
	pBuffer = CreateShareMemory(nPageSize);
	m_PageSize = nPageSize;
	m_pAclHeader = (PXACL_HEADER)pBuffer;
	return m_pAclHeader;
}

//
// 由于映射的每一个用户地址空间的各不相同，所以需要通过偏移量
// 计算出正确的地址
//
DWORD Nat(PVOID pAddress)
{
	if(pAddress == NULL) return 0;
	return ((DWORD)pAddress + m_dwOffset);
}

//
// 释放控管规则的内存空间
//
int FreeMemory()
{
	if(m_pAclHeader == 0) return 0;
	FreeShareMemory(m_pAclHeader);
	m_pAclHeader = 0;
	m_PageSize = 0;
	m_UserCreateAddress = 0;
	return 1;
}


//
// 使控管规则正在使用的计数器 + 1
//
BOOL RefenceAclCount()
{
	if(!m_IsFilter || m_pAclHeader == NULL) 
		return FALSE;
	if(m_pAclHeader->wPv == PV_LOCKED)
		return FALSE;
	m_pAclHeader->wRefenceCount++;
 	dprintf(("RefenceAclCount: %d\n", m_pAclHeader->wRefenceCount));
	return TRUE;
}

//
// 使控管规则正在使用的计数器 - 1
//
void DerefenceAclCount()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wRefenceCount--;
	dprintf(("DerefenceAclCount: %d\n", m_pAclHeader->wRefenceCount));
}



//==============================================================================
// 控管规则比较相关函数
//

//
// 从总工作模式得到控管状态
//
int GetAccessFromWorkMode()
{
	if(m_pAclHeader->bWorkMode == XF_PASS_ALL)
		return XF_PASS;
	if(m_pAclHeader->bWorkMode == XF_DENY_ALL)
		return XF_DENY;
	if(m_pAclHeader->bWorkMode != XF_QUERY_ALL)
		return XF_UNKNOWN;

	return XF_FILTER;
}

//
// 判断是否是本地IP地址
// 返回值：
//		TRUE:	是本地IP
//		FALSE:	不是本地IP
//
BOOL IsLocalIp(DWORD *ip)
{
	BYTE IsLocalIP[4];
	memcpy(IsLocalIP, ip, sizeof(DWORD));

	if(*ip == 0 || IsLocalIP[3] == 127)
		return TRUE;
	return FALSE;
}

//
// 判断是否是例行公事的封包，比如广播包。
//	此函数暂停判断永远返回FALSE。
//
BOOL IsRoutine(PPACKET_BUFFER pPacketBuffer)
{
//	if(pPacketBuffer->DestinationIp == 0xFFFFFFFF || pPacketBuffer->SourceIp == 0xFFFFFFFF)
//		return TRUE;
//	if( (pPacketBuffer->DestinationIp & 0x00FFFFFF) == (pPacketBuffer->SourceIp - 0x00FFFFFF)
//		&& ((pPacketBuffer->DestinationIp & 0xFF000000) == 0xFF 
//			|| (pPacketBuffer->SourceIp & 0xFF000000) == 0xFF)
//		)
//		return TRUE;

	return FALSE;
}

//
// 不使用控管规则判断封包是否允许放行，在使用控管规则判断之前一般
// 首先调用此函数进行判断，如果返回XF_FILTER则需要进一步使用控管规则
// 进行判断，如果是XF_PASS或者XF_DENY则直接进行放行或者拒绝。
// 
// 参数：
//		pPacketBuffer: 指向截获的数据封包指针。
//
int	GetAccessWithoutAcl(PPACKET_BUFFER pPacketBuffer)
{
	int Action;
	Action = GetAccessFromWorkMode();
	if(Action != XF_FILTER)
		return Action;

	if(IsLocalIp(&pPacketBuffer->DestinationIp) || IsRoutine(pPacketBuffer)) 
		return XF_PASS;

	if(pPacketBuffer->Direction == ACL_DIRECTION_NOT_SET)
		return XF_PASS;

	return XF_FILTER;
}

//
// 根据子工作模式判断是否放行。对于应用程序、网上邻居、ICMP分别都有
// 一个子工作模式，这个子工作模式优先与控管规则进行判断。
//
int CheckSubWorkMode(BYTE bWorkMode, PPACKET_BUFFER pPacketBuffer)
{
	if(strcmp(m_pAclHeader->sSignature, ACL_HEADER_SIGNATURE) != 0)
		return XF_PASS;

	switch(bWorkMode)
	{
	case ACL_PASS_ALL:
		return XF_PASS;
	case ACL_DENY_ALL:
		return XF_DENY;
	case ACL_DENY_IN:
		if(pPacketBuffer->Protocol == ACL_SERVICE_TYPE_UDP
			&& (pPacketBuffer->SourcePort == 137 
				|| pPacketBuffer->SourcePort == 138
				|| pPacketBuffer->SourcePort == 53
				)
			)
			return XF_PASS;
		if(pPacketBuffer->Direction == ACL_DIRECTION_IN)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_DENY_OUT:
		if(pPacketBuffer->Protocol == ACL_SERVICE_TYPE_UDP
			&& (pPacketBuffer->DestinationPort == 137 
				|| pPacketBuffer->DestinationPort == 138
				|| pPacketBuffer->DestinationPort == 53
				)
			)
			return XF_PASS;
		if(pPacketBuffer->Direction == ACL_DIRECTION_OUT)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}

//
// 当没有找到符合的控管规则时按照控管规则的默认设置决定是放行、拒绝还是询问
//
int CheckQueryEx(BYTE bQueryEx)
{
	switch(bQueryEx)
	{
	case ACL_QUERY_PASS:
		return XF_PASS;
	case ACL_QUERY_DENY:
		return XF_DENY;
	case ACL_QUERY_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}


//
// 根据安全等级判断是否需要放行。当需要询问而安全等级为低时默认放行
// 或者当需要询问安全等级为中时，连出默认放行。
//
int GetAccessFromSecurity(PPACKET_BUFFER pPacket)
{
	if(pPacket->Action == XF_QUERY && 
		(m_pAclHeader->bSecurity == ACL_SECURITY_LOWER
		|| (m_pAclHeader->bSecurity == ACL_SECURITY_NORMAL
			&& pPacket->Direction == ACL_DIRECTION_OUT
		    && (pPacket->AclType == ACL_TYPE_NNB || pPacket->AclType == ACL_TYPE_ICMP)
			)
		)
	  )
		return XF_PASS;
	return pPacket->Action;
}

//
// 应用程序规则比较函数
//
int CheckApp(PPACKET_BUFFER pPacketBuffer)
{
	int Action;
	PXACL pAcl;
	char pProcessName[260];

	dprintf(("CheckApp\n"));

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	GetProcessFileName(pProcessName, 260, FALSE);

	Action = CheckSubWorkMode(m_pAclHeader->bAppSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	pAcl = (PXACL)Nat(m_pAclHeader->pAcl);

	if(pAcl != NULL)
	{
		//
		// 循环比较应用程序规则
		//
		do
		{
			if(pAcl->bDirection != ACL_DIRECTION_IN_OUT 
				&& pAcl->bDirection != pPacketBuffer->Direction) continue;
			if(pAcl->uiServicePort != ACL_SERVICE_PORT_ALL 
				&& pAcl->uiServicePort != pPacketBuffer->DestinationPort) continue;
			if(pAcl->bRemoteNetType != ACL_NET_TYPE_ALL 
				&& pAcl->bRemoteNetType != pPacketBuffer->NetType) continue;
			if(pAcl->wLocalPort != ACL_SERVICE_PORT_ALL 
				&& pAcl->wLocalPort != pPacketBuffer->SourcePort) continue;
			if(pAcl->bAccessTimeType != ACL_SERVICE_PORT_ALL 
				&& pAcl->bAccessTimeType != pPacketBuffer->TimeType) continue;
			if(pAcl->bServiceType != ACL_SERVICE_TYPE_ALL 
				&& pAcl->bServiceType != pPacketBuffer->Protocol) continue;
			if(strcmp(pAcl->sApplication, _T("*")) != 0 
				&& (strlen(pAcl->sApplication) != strlen(pProcessName) 
				|| strnicmp(pAcl->sApplication, pProcessName
					, strlen(pProcessName)) != 0)
				) continue;

			Action = pAcl->bAction;
			pPacketBuffer->AclId = pAcl->ulAclID;
			goto XF_EXIT;

		}while((pAcl = (PXACL)Nat(pAcl->pNext)) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bAppQueryEx);
	goto XF_EXIT;


XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// 网上邻居控管规则比对函数
//
int CheckNnb(PPACKET_BUFFER pPacketBuffer)
{
	int Action;
	PXACL_NNB pNnb;
	char sNnb[NETBIOS_NAME_MAX_LENTH];

	dprintf(("CheckNnb\n"));

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	Action = CheckSubWorkMode(m_pAclHeader->bNnbSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	//
	//2002/06/10 remove, bug's here, this only used to debug.
	//
	//Action = XF_PASS;
	//goto XF_EXIT;

	GetNameFromIp(pPacketBuffer->DestinationIp, sNnb);
	if(sNnb[0] == 0)
		return XF_PASS;

	pNnb = (PXACL_NNB)Nat(m_pAclHeader->pNnb);

	if(pNnb != NULL)
	{
		//
		// 循环比较网上邻居规则
		//
		do
		{
			if(pNnb->bDirection != ACL_DIRECTION_IN_OUT 
				&& pNnb->bDirection != pPacketBuffer->Direction) continue;
			if(pNnb->bTimeType != ACL_SERVICE_PORT_ALL 
				&& pNnb->bTimeType != pPacketBuffer->TimeType) continue;
			if(strcmp(pNnb->sNnb, _T("*")) != 0 
				&& (strlen(pNnb->sNnb) != strlen(sNnb) || strnicmp(pNnb->sNnb, sNnb, strlen(sNnb)) != 0)
				&& pNnb->dwIp != pPacketBuffer->DestinationIp
				) continue;

			if(strnicmp(pNnb->sNnb, sNnb, strlen(sNnb)) == 0 && pNnb->dwIp != pPacketBuffer->DestinationIp)
				pNnb->dwIp = pPacketBuffer->DestinationIp;

			Action = pNnb->bAction;
			pPacketBuffer->AclId = pNnb->dwId;
			goto XF_EXIT;

		}while((pNnb = (PXACL_NNB)Nat(pNnb->pNext)) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bNnbQueryEx);
	goto XF_EXIT;

XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// ICMP控管规则比较函数
//
int CheckIcmpAcl(PPACKET_BUFFER pPacketBuffer)
{
	int Action;
	PXACL_ICMP pIcmp;

	dprintf(("CheckIcmpAcl\n"));

	Action = XF_PASS;
	if(!RefenceAclCount())
		return Action;

	if((Action = GetAccessWithoutAcl(pPacketBuffer)) != XF_FILTER)
		goto XF_EXIT;

	Action = CheckSubWorkMode(m_pAclHeader->bIcmpSet, pPacketBuffer);
	if(Action != XF_QUERY) 
		goto XF_EXIT;

	pIcmp = (PXACL_ICMP)Nat(m_pAclHeader->pIcmp);

	if(pIcmp != NULL)
	{
		//
		// 循环比较ICMP规则
		//
		do
		{
			if(pIcmp->bDirection != ACL_DIRECTION_IN_OUT 
				&& pIcmp->bDirection != pPacketBuffer->Direction) continue;
			if(pIcmp->bTimeType != ACL_SERVICE_PORT_ALL 
				&& pIcmp->bTimeType != pPacketBuffer->TimeType) continue;
			if(pIcmp->bNetType != ACL_NET_TYPE_ALL 
				&& pIcmp->bNetType != pPacketBuffer->NetType) continue;

			Action = pIcmp->bAction;
			pPacketBuffer->AclId = pIcmp->dwId;
			goto XF_EXIT;

		}while((pIcmp = (PXACL_ICMP)Nat(pIcmp->pNext)) != NULL);
	}

	Action = CheckQueryEx(m_pAclHeader->bIcmpQueryEx);
	goto XF_EXIT;


XF_EXIT:
	DerefenceAclCount();
	return Action;
}

//
// 保留函数
//
int CheckTrojan()
{
	return XF_PASS;
}

//
// 保留函数
//
int ProtectApp()
{
	return XF_PASS;
}



//==============================================================================
// 封包解析相关函数
//

//
// 2002/05/20 add this function and move CheckSend code to here
//
// 对 NDIS_PACKET 封包进行解析
//
int CheckPacket(
	IN PNDIS_PACKET packet,
	IN BOOL IsSend
)
{
 	PNDIS_BUFFER  FirstBuffer, Buffer;
	UINT TotalPacketLength;
	WORD EthernetFrameType;
	int HeaderLength;
	PIP_HEADER pIpHeader;
	PETHERNET_FRAME pEthernetFrame;
	void* pBiosBuffer;
	PICMP_HEADER pIcmpHeader;
	PTCP_HEADER pTcpHeader;
	PUDP_HEADER pUdpHeader;

	UINT PhysicalBufferCount;
	UINT BufferCount;
	PVOID VirtualAddress;
	int Length = 0;

	dprintf(("CheckSend\n"));

	//
	// 得到第一个NDIS_BUFFER
	//
	TotalPacketLength = 0;
	NdisQueryPacket(packet
		, &PhysicalBufferCount
		, &BufferCount
		, &FirstBuffer
		, &TotalPacketLength
		);

	if(FirstBuffer == NULL)
		return XF_PASS;

	//
	// 2002/05/04 Add. Because Buffer is not initialized, it can bring a bug check
	// 0x1e in source code following. 
	// 
	Buffer = FirstBuffer;

	//
	// 解析Ethernet Frame
	//
	NdisQueryBufferSafe(FirstBuffer, &VirtualAddress, &Length, HighPagePriority);
	pEthernetFrame = (PETHERNET_FRAME)VirtualAddress;
	EthernetFrameType = ntohs(pEthernetFrame->FrameType);

	if(EthernetFrameType != ETHERNET_FRAME_TYPE_TCPIP)
		return XF_PASS;

	//
	// 解析Ip Header
	//
	if((Length - ETHERNET_FRAME_LENGTH) >= IP_HEADER_LENGTH)
	{
		pIpHeader = (PIP_HEADER)((char*)pEthernetFrame + ETHERNET_FRAME_LENGTH);
		Length = Length - ETHERNET_FRAME_LENGTH;
	}
	else
	{
		NdisGetNextBuffer(FirstBuffer, &Buffer);

		if(Buffer == NULL)
			return XF_PASS;

		NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);

		if(VirtualAddress == NULL || Length < IP_HEADER_LENGTH)
			return XF_PASS;

		pIpHeader = (PIP_HEADER)VirtualAddress;
	}

	HeaderLength = pIpHeader->HeaderLength * HEADER_LENGTH_MULTIPLE;

	//dprintf(("HeaderLength: %u\n", HeaderLength));
	PrintIp(pIpHeader, Length);

	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		//
		// 解析Tcp Header
		//
		if((Length - HeaderLength) < TCP_HEADER_LENGTH)
		{
			//
			// if Buffer is NULL or Invalid Address, It can bring a bug check
			// 0x1e.
			//
			NdisGetNextBuffer(Buffer, &Buffer);
			if(Buffer == NULL) return XF_PASS;
			NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);
			if(VirtualAddress != NULL && Length >= TCP_HEADER_LENGTH)
			{
				pTcpHeader = (PTCP_HEADER)(VirtualAddress);
			}
			else
			{
				return XF_PASS;
			}
		}
		else
		{
			pTcpHeader = (PTCP_HEADER)((DWORD)pIpHeader + HeaderLength);
		}

		//PrintTcp(pTcpHeader);

		pBiosBuffer = NULL;
		if(Buffer != NULL)
		{
			NdisGetNextBuffer(Buffer, &Buffer);
			if(Buffer != NULL)
			{
				NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);
				if(VirtualAddress != NULL && Length >= NETBIOS_MIN_PACKET_SIZE)
					pBiosBuffer = (void*)VirtualAddress;
			} 
		}
		//
		// 调用CheckTcp对封包的合法性进行审查
		//
		return CheckTcp(pIpHeader, pTcpHeader, IsSend, TotalPacketLength, pBiosBuffer);

	case PROTOCOL_UDP:
		//
		// 解析UDP Header
		//
		if((Length - HeaderLength) < UDP_HEADER_LENGTH)
		{
			//
			// if Buffer is NULL or Invalid Address, It can bring a bug check
			// 0x1e.
			//
			NdisGetNextBuffer(Buffer, &Buffer);
			if(Buffer == NULL) return XF_PASS;
			NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);
			if(VirtualAddress != NULL && Length >= UDP_HEADER_LENGTH)
			{
				pUdpHeader = (PUDP_HEADER)(VirtualAddress);
			}
			else
			{
				return XF_PASS;
			}
		}
		else
		{
			pUdpHeader = (PUDP_HEADER)((DWORD)pIpHeader + HeaderLength);
		}

		pBiosBuffer = NULL;
		if(Buffer != NULL)
		{
			NdisGetNextBuffer(Buffer, &Buffer);
			if(Buffer != NULL)
			{
				NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);
				if(VirtualAddress != NULL && Length >= NETBIOS_MIN_PACKET_SIZE)
					pBiosBuffer = (void*)VirtualAddress;
			}
		}

		//PrintUdp(pUdpHeader);

		//
		// 调用 CheckUdp 对封包的合法性进行审查
		//
		return CheckUdp(pIpHeader, pUdpHeader, IsSend, TotalPacketLength, pBiosBuffer);

	case PROTOCOL_ICMP:
		//
		// 解析 ICMP
		//
		if((Length - HeaderLength) < ICMP_HEADER_LENGTH)
		{
			//
			// if Buffer is NULL or Invalid Address, It can bring a bug check
			// 0x1e.
			//
			NdisGetNextBuffer(Buffer, &Buffer);
			if(Buffer == NULL) return XF_PASS;
			NdisQueryBufferSafe(Buffer, &VirtualAddress, &Length, HighPagePriority);
			if(VirtualAddress != NULL && Length >= ICMP_HEADER_LENGTH)
				pIcmpHeader = (PICMP_HEADER)(VirtualAddress);
			else
				return XF_PASS;
		}
		else
		{
			pIcmpHeader = (PICMP_HEADER)((DWORD)pIpHeader + HeaderLength);
		}

		//
		// 调用 CheckIcmp 对封包的合法性进行审查
		//
		return CheckIcmp(pIpHeader, pIcmpHeader, IsSend, TotalPacketLength);

	case PROTOCOL_IGMP:
	default:
		break;
	}

	return XF_PASS;
}

//
// 对NdisSend发送的NDIS_PACKET封包进行解析
//
int CheckSend(
	IN PNDIS_PACKET packet
)
{
	return CheckPacket(packet, TRUE);
}

//
// 对ProtocolReceive接收的HeaderBuffer和LookAheadBuffer进行解析
//
int CheckRecv(
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
)
{
 	WORD EthernetFrameType;
	WORD LengthCount;
	PIP_HEADER pIpHeader;
	PETHERNET_FRAME pEthernetFrame;

	if(HeaderBufferSize < ETHERNET_FRAME_LENGTH) 
		return XF_PASS;

	dprintf(("CheckRecv\n"));

	//
	// 解析Ethernet Frame
	//
	pEthernetFrame = (PETHERNET_FRAME)HeaderBuffer;
	EthernetFrameType = ntohs(pEthernetFrame->FrameType);
	if(EthernetFrameType != ETHERNET_FRAME_TYPE_TCPIP
		|| LookaheadBufferSize < IP_HEADER_LENGTH)
		return XF_PASS;

	//
	// 解析Ip Header
	//
	pIpHeader = (PIP_HEADER)LookAheadBuffer;
	LengthCount = pIpHeader->HeaderLength * HEADER_LENGTH_MULTIPLE;
	if(LengthCount == 0)
		return XF_PASS;

	switch(pIpHeader->Protocol)
	{
	case PROTOCOL_TCP:
		//
		// 解析Tcp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + TCP_HEADER_LENGTH))
			return XF_PASS;
		return CheckTcp(pIpHeader
			, (PTCP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, PacketSize + HeaderBufferSize
			, (PVOID)LookaheadBufferSize
			);

	case PROTOCOL_UDP:
		//
		// 解析 Udp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + UDP_HEADER_LENGTH))
			return XF_PASS;
		return CheckUdp(pIpHeader
			, (PUDP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, PacketSize + HeaderBufferSize
			, (PVOID)LookaheadBufferSize
			);

	case PROTOCOL_ICMP:
		//
		// 解析Icmp Header
		//
		if(LookaheadBufferSize < (UINT)(LengthCount + ICMP_HEADER_LENGTH))
			return XF_PASS;
		return CheckIcmp(pIpHeader
			, (PICMP_HEADER)((char*)LookAheadBuffer + LengthCount)
			, FALSE
			, PacketSize + HeaderBufferSize
			);

	case PROTOCOL_IGMP:
	default:
		break;
	}
	return XF_PASS;
}

//
// 从协议和端口得到自定义的协议类型
//
int GetProtocol(BYTE bProtocol, WORD wPort)
{
	if(bProtocol == PROTOCOL_TCP)
	{
		switch(wPort)
		{
		case ACL_SERVICE_PORT_FTP:
			return ACL_SERVICE_TYPE_FTP;
		case ACL_SERVICE_PORT_TELNET:
			return ACL_SERVICE_TYPE_TELNET;
		case ACL_SERVICE_PORT_NNTP:
			return ACL_SERVICE_TYPE_NNTP;
		case ACL_SERVICE_PORT_POP3:
			return ACL_SERVICE_TYPE_POP3;
		case ACL_SERVICE_PORT_SMTP:
			return ACL_SERVICE_TYPE_SMTP;
		case ACL_SERVICE_PORT_HTTP:
			return ACL_SERVICE_TYPE_HTTP;
		}
		return ACL_SERVICE_TYPE_TCP;
	}
	else if(bProtocol == PROTOCOL_UDP)
		return ACL_SERVICE_TYPE_UDP;
	else if(bProtocol == PROTOCOL_ICMP)
		return ACL_SERVICE_TYPE_ICMP;

	return ACL_SERVICE_TYPE_ALL;
}

//
// 得到字节中某一位
//
int	GetBit(BYTE bit, int index)
{
	bit <<= index;
	bit >>= (8 - 1);

	return bit;
}

//
// 根据时间和星期得到自定义的时间类型
//
int FindTime(DWORD t, BYTE Week)
{
	PXACL_TIME pTime;
	int i;

	if(!RefenceAclCount()) 
		return ACL_TIME_TYPE_ALL;

	i = 0; pTime = (PXACL_TIME)Nat(m_pAclHeader->pTime);
	if(pTime != NULL) pTime = (PXACL_TIME)Nat(pTime->pNext);
	do
	{
		i++;
		if(GetBit(pTime->bWeekDay, Week - 1) != 1)
			continue;
		if(pTime->tStartTime == pTime->tEndTime)
			break;
		if(pTime->tStartTime < pTime->tEndTime)
		{
			if(t >= pTime->tStartTime && t <= pTime->tEndTime)
				break;
		}
		else
		{
			if(t >= pTime->tStartTime || t <= pTime->tEndTime)
				break;
		}

	} while((pTime = (PXACL_TIME)Nat(pTime->pNext)) != NULL); 

	DerefenceAclCount();

	if(pTime != NULL)
		return i;

	return ACL_TIME_TYPE_ALL;
}

//
// Ip 地址段比较函数
//
BOOL FindIpEx(PXACL_IP pIp, DWORD dwIp)
{
	while(pIp != NULL)
	{
		if(dwIp >= pIp->ulStartIP && dwIp <= pIp->ulEndIP) 
			return TRUE;
		pIp = (PXACL_IP)Nat(pIp->pNext);
	}
	return FALSE;
}

//
// 根据Ip地址得到自定义的网络类型
//
int FindIp(DWORD Ip)
{
	int iRet;

	iRet = ACL_NET_TYPE_ALL;
	if(!RefenceAclCount())
		return iRet;
	if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pIntranetIp), Ip))
		iRet = ACL_NET_TYPE_INTRANET;
	else if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pDistrustIp), Ip))
		iRet = ACL_NET_TYPE_DISTRUST;
	else if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pTrustIp), Ip))
		iRet = ACL_NET_TYPE_TRUST;
	else if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pCustomIp), Ip))
		iRet = ACL_NET_TYPE_CUSTOM;
	DerefenceAclCount();
	return iRet;
}

//
// 判断是否是XFILTER.DLL(SPI)层控管的端口端口，如果属于SPI控管
// 则没有必要再次控管。
//
BOOL IsSpiPort(PPACKET_BUFFER pPacketBuffer)
{
	int iPort;
	if(pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_FTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_TELNET
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_NNTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_POP3
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_SMTP
		|| pPacketBuffer->DestinationPort == ACL_SERVICE_PORT_HTTP
		)
		return TRUE;
	if(FindPort((USHORT)pPacketBuffer->SourcePort, &iPort))
		return TRUE;
	return FALSE;
}

//
// Tcp Header 解析函数
//
int CheckTcp(
	PIP_HEADER pIpHeader, 
	PTCP_HEADER pTcpHeader, 
	BOOL IsSend, 
	UINT LookaheadBufferSize,
	PVOID pVoid
)
{
	PPACKET_BUFFER pPacketBuffer;
	PACKET_BUFFER PacketBuffer;
	PPACKET_DIRECTION pPacketDirection;
	ULONG Time;

	dprintf(("CheckTcp\n"));

	//
	// 分析TCP封包并保存到PacketBuffer，已备进行合法性检查和记录日志
	// 关于TCP封包的具体结构请参阅RFC793
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);
	if(IsSend)
	{
		PacketBuffer.SourcePort		 = ntohs(pTcpHeader->SourcePort);
		PacketBuffer.DestinationPort = ntohs(pTcpHeader->DestinationPort);
	}
	else
	{
		PacketBuffer.SourcePort		 = ntohs(pTcpHeader->DestinationPort);
		PacketBuffer.DestinationPort = ntohs(pTcpHeader->SourcePort);
	}

	//
	// 2002/05/30 remove netbios name parse over tcp
	//
	// 端口为137表示是NetBios的名字服务
	//
	//if(PacketBuffer.SourcePort == 137 || PacketBuffer.DestinationPort == 137)
	//{
		//
		// NetBios名字解析
		//
	//	if(IsSend)
	//	{
	//		if(pVoid != NULL)
 	//			MakeNameList((char*)pVoid);
	//	}
	//	else
	//	{
	//		int HeaderLength;
	//		HeaderLength = GET_TCP_HEADER_LENGTH(ntohs(pTcpHeader->LenAndCodeBits));
	//		if((DWORD)pVoid >= (UINT)(HeaderLength + NETBIOS_MIN_PACKET_SIZE))
 	//			MakeNameList((char*)pTcpHeader + HeaderLength);
			//return XF_PASS;
	//	}
	//}

	if(IsSpiPort(&PacketBuffer))
		return XF_PASS;

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_TCP;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.TcpCode		 = pTcpHeader->TcpCode;
	PacketBuffer.AclType		 = ACL_TYPE_APP;
	PacketBuffer.ProcessHandle   = (DWORD)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = (BYTE)FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = ACL_NET_TYPE_ALL;
	PacketBuffer.Direction		 = ACL_DIRECTION_NOT_SET;

	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);
	
	if(pTcpHeader->TcpSyn)
	{
		PACKET_DIRECTION PacketDirection;

		//
		// 建立连线
		//

		if(IsSend)
		{
			PacketBuffer.Direction = pTcpHeader->TcpAck 
				? ACL_DIRECTION_IN : ACL_DIRECTION_OUT;
		}
		else
		{
			PacketBuffer.Direction =  pTcpHeader->TcpAck 
				? ACL_DIRECTION_OUT : ACL_DIRECTION_IN;
		}

		if(PacketBuffer.SourcePort == 139 || PacketBuffer.DestinationPort == 139)
		{
			//
			// 端口139表示是NetBios的资源共享服务，用网上邻居控管规则进行控管
			//
			PacketBuffer.AclType = ACL_TYPE_NNB;
			PacketBuffer.Action = (USHORT)CheckNnb(&PacketBuffer);
		}
		else
		{
			//
			// 按应用程序规则进行过滤
			//
			WORD wPort;
			wPort = (PacketBuffer.Direction == ACL_DIRECTION_IN) 
				? (WORD)PacketBuffer.SourcePort 
				: (WORD)PacketBuffer.DestinationPort;
			PacketBuffer.NetType = (BYTE)FindIp(PacketBuffer.DestinationIp);
			PacketBuffer.Protocol = (BYTE)GetProtocol(PROTOCOL_TCP, wPort);
			PacketBuffer.AclType = ACL_TYPE_DRIVER_APP;

			PacketBuffer.Action = (USHORT)CheckApp(&PacketBuffer);
		}

		PacketBuffer.Action = (USHORT)GetAccessFromSecurity(&PacketBuffer);

		if(PacketBuffer.Action != XF_PASS || !pTcpHeader->TcpAck)
			goto XF_EXIT;

		//
		// 增加TCP连线记录
		//
		PacketDirection.DeleteIn	= 0;
		PacketDirection.DeleteOut	= 0;
		PacketDirection.Id			= PacketBuffer.Id;
		PacketDirection.Direction	= (BYTE)PacketBuffer.Direction;
		PacketDirection.Action		= (BYTE)PacketBuffer.Action;
		PacketDirection.AclType		= PacketBuffer.AclType;
		PacketDirection.AclId		= PacketBuffer.AclId;
		PacketDirection.Protocol	= PacketBuffer.Protocol;
		PacketDirection.NetType		= PacketBuffer.NetType;
		PacketDirection.ProcessHandle = PacketBuffer.ProcessHandle;
		strcpy(PacketDirection.sProcess, PacketBuffer.sProcess);
		if(IsSend)
		{
			PacketDirection.RecvData = 0;
			PacketDirection.SendData = PacketBuffer.DataBytes;
		}
		else
		{
			PacketDirection.SendData = 0;
			PacketDirection.RecvData = PacketBuffer.DataBytes;
		}
		PacketDirection.LocalIp		= PacketBuffer.SourceIp;
		PacketDirection.RemoteIp	= PacketBuffer.DestinationIp;
		PacketDirection.LocalPort	= (USHORT)PacketBuffer.SourcePort;
		PacketDirection.RemotePort	= (USHORT)PacketBuffer.DestinationPort;
		PacketDirection.Time		= PacketBuffer.Time;

		AddDirection(&PacketDirection);

		goto XF_EXIT;
	}
	else
	{
		int iPort;
		int iIndex;

		//
		// 连线活动，比如发送数据、断开连线、连线复位等命令
		//

		//
		// 从已存在的连线记录中查询
		//
		if(FindDirection(PacketBuffer.Id, &iIndex))
		{
			pPacketDirection = GetDirection(iIndex);

			if(IsSend)
				pPacketDirection->SendData += PacketBuffer.DataBytes;
			else
				pPacketDirection->RecvData += PacketBuffer.DataBytes;

			PacketBuffer.Direction = pPacketDirection->Direction;
			PacketBuffer.Protocol = pPacketDirection->Protocol;
			PacketBuffer.AclType = pPacketDirection->AclType;
			PacketBuffer.AclId = pPacketDirection->AclId;
			PacketBuffer.NetType = pPacketDirection->NetType;

			//
			// 连线复位
			//
			if(pTcpHeader->TcpRst)
			{
				DeleteDirection(PacketBuffer.Id, TRUE, TRUE);
				PacketBuffer.Action = XF_PASS;
				goto XF_EXIT;
			}
			//
			// 中断连接请求
			//
			else if(pTcpHeader->TcpFin)
			{
				if(IsSend)
					DeleteDirection(PacketBuffer.Id, FALSE, TRUE);
				else
					DeleteDirection(PacketBuffer.Id, TRUE, FALSE);
				PacketBuffer.Action = XF_PASS;
				goto XF_EXIT;
			}
			//
			// 数据传输
			//
			else //if(pTcpHeader->TcpPsh)
			{
				if(PacketBuffer.AclType == ACL_TYPE_NNB)
					PacketBuffer.Action = (USHORT)CheckNnb(&PacketBuffer);
				else
					PacketBuffer.Action = (USHORT)CheckApp(&PacketBuffer);

				PacketBuffer.Action = (USHORT)GetAccessFromSecurity(&PacketBuffer);


				pPacketDirection->Action = (BYTE)PacketBuffer.Action;
				if(PacketBuffer.Action == XF_QUERY)
					goto XF_EXIT;
				//if(PacketBuffer.Action == XF_QUERY)
				//	pPacketDirection->Action = XF_DENY;
				//else
				//	pPacketDirection->Action = (BYTE)PacketBuffer.Action;

				return PacketBuffer.Action;
			}

			goto XF_EXIT;
		}
		PacketBuffer.Action = XF_PASS;
		return PacketBuffer.Action;
	}

	PacketBuffer.Action = XF_PASS; 
	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// Udp Header 解析函数
//
int CheckUdp(
	PIP_HEADER pIpHeader, 
	PUDP_HEADER pUdpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize,
	void *pVoid
)
{
	PPACKET_BUFFER pPacketBuffer;
	PACKET_BUFFER PacketBuffer;
	PPACKET_DIRECTION pPacketDirection;
	ULONG Time;

	dprintf(("CheckUdp\n"));

	//
	// 分析UDP封包并保存到PacketBuffer，已备进行合法性检查和记录日志
	// 关于UDP封包的具体结构请参阅RFC768
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);
	if(IsSend)
	{
		PacketBuffer.SourcePort		 = ntohs(pUdpHeader->SourcePort);
		PacketBuffer.DestinationPort = ntohs(pUdpHeader->DestinationPort);
	}
	else
	{
		PacketBuffer.SourcePort		 = ntohs(pUdpHeader->DestinationPort);
		PacketBuffer.DestinationPort = ntohs(pUdpHeader->SourcePort);
	}

	//
	// 端口为137表示是NetBios的名字服务
	//
	if(PacketBuffer.SourcePort == 137 || PacketBuffer.DestinationPort == 137)
	{
		if(IsSend && pVoid != NULL)
		{
			if(pVoid != NULL)
 				MakeNameList((char*)pVoid);
		}
		else
		{
			if((DWORD)pVoid >= 8 + NETBIOS_MIN_PACKET_SIZE)
				MakeNameList((char*)pUdpHeader + 8);
		}
		//return XF_PASS;
	}
	//if(PacketBuffer.SourcePort == 138 || PacketBuffer.DestinationPort == 138)
	//	return XF_PASS;

	if(IsSpiPort(&PacketBuffer))
		return XF_PASS;

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.AclType		 = ACL_TYPE_APP;
	PacketBuffer.ProcessHandle   = (ULONG)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = (BYTE)FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = ACL_NET_TYPE_ALL;

	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_UDP;
	
	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);

	if(IsSend)
		PacketBuffer.Direction = ACL_DIRECTION_OUT;
	else
		PacketBuffer.Direction = ACL_DIRECTION_IN;

	if(PacketBuffer.SourcePort == 139 || PacketBuffer.DestinationPort == 139)
	{
		PacketBuffer.AclType = ACL_TYPE_NNB;
		PacketBuffer.Action = (USHORT)CheckNnb(&PacketBuffer);
	}
	else
	{
		PacketBuffer.NetType = (BYTE)FindIp(PacketBuffer.DestinationIp);
		PacketBuffer.AclType = ACL_TYPE_DRIVER_APP;
		PacketBuffer.Action = (USHORT)CheckApp(&PacketBuffer);
	}

	PacketBuffer.Action = (USHORT)GetAccessFromSecurity(&PacketBuffer);

	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// 根据 Icmp 的类型判断连线方向
//
int GetIcmpDirection(BYTE bType)
{
	switch(bType)
	{
	case ICMP_ECHO:
	case ICMP_TSTAMP:
	case ICMP_IREQ:
	case ICMP_MASKREQ:
		return ICMP_REQUEST;

	case ICMP_ECHOREPLY:
	case ICMP_TSTAMPREPLY:
	case ICMP_IREQREPLY:
	case ICMP_MASKREPLY:
		return ICMP_RESPONSE;

	case ICMP_UNREACH:
	case ICMP_SOURCEQUENCH:
	case ICMP_REDIRECT:
	case ICMP_ROUTERADVERT:
	case ICMP_ROUTERSOLICIT:
	case ICMP_TIMXCEED:
	case ICMP_PARAMPROB:
		break;
	}
	return ICMP_NORMAL;
}

//
// ICMP 解析函数
//
int CheckIcmp(
	PIP_HEADER pIpHeader, 
	PICMP_HEADER pIcmpHeader, 
	BOOL IsSend,
	UINT LookaheadBufferSize 
)
{
	BYTE bIcmpType;
	PPACKET_BUFFER pPacketBuffer;
	PACKET_BUFFER PacketBuffer;
	ULONG Time;

	dprintf(("CheckIcmp\n"));

	//
	// 分析ICMP封包并保存到PacketBuffer，已备进行合法性检查和记录日志
	// 关于ICMP封包的具体结构请参阅RFC792
	//

	memset(&PacketBuffer, 0, PACKET_INIT_LENGTH);

	if(IsSend)
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->SourceIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
	}
	else
	{
		PacketBuffer.SourceIp		 = ntohl(*(ULONG*)pIpHeader->DestinationIp);
		PacketBuffer.DestinationIp	 = ntohl(*(ULONG*)pIpHeader->SourceIp);
	}
	PacketBuffer.Time			 = GetCurrentTime(&PacketBuffer.Week, &Time);
	PacketBuffer.DataBytes		 = LookaheadBufferSize;
	PacketBuffer.SendOrRecv		 = IsSend;
	PacketBuffer.Status			 = 1;
	PacketBuffer.AclType		 = ACL_TYPE_ICMP;
	PacketBuffer.ProcessHandle   = (ULONG)VWIN32_GetCurrentProcessHandle();
	PacketBuffer.TimeType		 = (BYTE)FindTime(Time, PacketBuffer.Week);
	PacketBuffer.NetType		 = (BYTE)FindIp(PacketBuffer.DestinationIp);
	PacketBuffer.Id				 = PacketBuffer.SourceIp ^ PacketBuffer.DestinationIp;

	GetProcessFileName(PacketBuffer.sProcess, 16, TRUE);

	PacketBuffer.Protocol		 = ACL_SERVICE_TYPE_ICMP;
	PacketBuffer.IcmpType		 = pIcmpHeader->Type;
	PacketBuffer.IcmpSubType	 = pIcmpHeader->Code;
	
	bIcmpType = (BYTE)GetIcmpDirection(pIcmpHeader->Type);

	if(bIcmpType == ICMP_NORMAL)
	{
		PacketBuffer.Direction = ACL_DIRECTION_NOT_SET;
	}
	else if(IsSend)
	{
		PacketBuffer.Direction = 
			(bIcmpType == ICMP_REQUEST) ?
			ACL_DIRECTION_OUT
			: ACL_DIRECTION_IN;
	}
	else
	{
		PacketBuffer.Direction = 
			(bIcmpType == ICMP_REQUEST) ?
			ACL_DIRECTION_IN
			: ACL_DIRECTION_OUT;
	}

	PacketBuffer.Action = (USHORT)CheckIcmpAcl(&PacketBuffer);

	PacketBuffer.Action = (USHORT)GetAccessFromSecurity(&PacketBuffer);

	goto XF_EXIT;

XF_EXIT:

	pPacketBuffer = GetFreePacket();
	if(pPacketBuffer != NULL)
		*pPacketBuffer = PacketBuffer;
	return PacketBuffer.Action;
}

//
// 检查应用程序、DLL、驱动程序之间的编译参数是否相同
// 此代码与封包解析无关
//
BOOL CheckGuid()
{
	BOOLEAN bReturn;
	CHECK_GUID(bReturn);
	return bReturn;
}

#pragma comment( exestr, "B9D3B8FD2A6F676F71747B63656E2B")
