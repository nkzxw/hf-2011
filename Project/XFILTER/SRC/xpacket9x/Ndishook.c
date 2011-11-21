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
// NDISHOOK.C
//
// 简介：
//		主要完成对 NdisSend 和 ProtocolReceive 函数的Hook，由于
//		ProtocolRecvive是在RegisterProtocol时注册的，所以需要对
//		NdisRegisterProtocol函数的Hook。
//		另外，Windows 95/98 与 Windows ME 的Hook方式不同。
//		Windows 95/98 可以直接通过Hook_Device_Service Hook到
//		NdisSend 和 NdisRegisterProtocol。而Windows Me则需要首先
//		Hook 一个叫做PELDR_AddExportTable的函数，然后通过这个函数
//		Hook NdisSend 和 NdisRegisterProtocol。
//
//

#include "xprecomp.h"
#pragma hdrstop

PADD_EXPORT_TABLE m_pSystemAddExportTable = NULL;

PNDIS_SEND					m_pSysNdisSend				  = NULL;
NDIS_REGISTER_PROTOCOL		m_pSysNdisRegisterProtocol	  = NULL;

PNDIS_SEND					m_pNdisSend				  = NULL;
NDIS_REGISTER_PROTOCOL		m_pNdisRegisterProtocol	  = NULL;
NDIS_DEREGISTER_PROTOCOL	m_pNdisDeregisterProtocol = NULL;
RECEIVE_HANDLER				m_pNdisReceive			  = NULL;

HDSC_Thunk m_AddExportTableThunk;
HDSC_Thunk m_NdisSendThunk;
HDSC_Thunk m_NdisRegisterProtocolThunk;
HDSC_Thunk m_NdisDeregisterProtocolThunk;

BOOL IsWindowsMe()
{
	DWORD DebugInfo, Version, Major, Minor;
	Version = Get_VMM_Version(&DebugInfo);
	Major = Version & 0x0000FF00;
	Minor = Version & 0x000000FF;
	dprintf("DebugInfo: 0x%08X, Version: 0x%08X, Major: %u, Minor: %u\n"
		, DebugInfo
		, Version
		, Major
		, Minor
		);
	if(Minor >= 0x5A)
		return TRUE;
	return FALSE;
}

//
// Hook 需要的 Hook 的函数
//
NTSTATUS Hook_Ndis_Function()
{
	DWORD nResult = VXDLDR_GetVersion();
	
	dprintf( "-->Hook_Ndis_Function\n" );

	if(nResult && IsWindowsMe())
	{
		//
		// Hook _PELDR_AddExportTable For WinMe
		//
		m_pSystemAddExportTable = (PADD_EXPORT_TABLE)Hook_Device_Service_C(
			___PELDR_AddExportTable
			, XF_PELDR_AddExportTable
			, &m_AddExportTableThunk);
		if(m_pSystemAddExportTable == NULL)
		{
			dprintf( "Hook _PELDR_AddExportTable Failed\n" );
			return NDIS_STATUS_FAILURE;
		}
	}
	else
	{
		dprintf("VXDLDR NOT LOADED\n");
	}

	//
	// Hook NdisSend For Win95/98
	//
	m_pNdisSend = (PNDIS_SEND)Hook_Device_Service_C(
		__NdisSend
		, XF_NdisSend
		, &m_NdisSendThunk);
	if(m_pNdisSend == NULL)
	{
		dprintf( "Hook NdisSend Failed\n" );
		return NDIS_STATUS_FAILURE;
	}

	//
	// Hook NdisRegisterProtocol For Win95/98
	//
	m_pNdisRegisterProtocol = (NDIS_REGISTER_PROTOCOL)Hook_Device_Service_C(
		__NdisRegisterProtocol
		, XF_NdisRegisterProtocol
		, &m_NdisRegisterProtocolThunk);
	if(m_pNdisRegisterProtocol == NULL)
	{
		dprintf( "Hook NdisRegisterProtocol Failed\n" );
		return NDIS_STATUS_FAILURE;
	}

	//
	// Hook NdisRegisterProtocol For Win95/98
	//
	m_pNdisDeregisterProtocol = (NDIS_DEREGISTER_PROTOCOL)Hook_Device_Service_C(
		__NdisDeregisterProtocol
		, XF_NdisDeregisterProtocol
		, &m_NdisDeregisterProtocolThunk);
	if(m_pNdisDeregisterProtocol == NULL)
	{
		dprintf( "Hook NdisDeregisterProtocol Failed\n" );
		return NDIS_STATUS_FAILURE;
	}

	return NDIS_STATUS_SUCCESS;
}

//
// Hook所需函数For Windows ME
//
LRESULT CDECL XF_PELDR_AddExportTable(
	PHPEEXPORTTABLE     pht,
	PSTR                pszModuleName,
	ULONG               cExportedFunctions,
	ULONG               cExportedNames,
	ULONG               ulOrdinalBase,
	PVOID               *pExportNameList,
	PUSHORT             pExportOrdinals,
	PVOID               *pExportAddrs,
	PHLIST              phetl
)
{
	dprintf("XF_PELDR_AddExportTable\n");

	if(strcmp(pszModuleName, "NDIS.SYS") == 0)
	{
		ULONG i;
		for(i = 0; i < cExportedNames; ++i)
		{
			//
			// Hook NdisRegisterProtocol For Windows Me
			//
			if(strcmp(pExportNameList[i], "NdisRegisterProtocol") == 0)
			{
				dprintf( "  Hooking NdisRegisterProtocol\n" );
				m_pSysNdisRegisterProtocol = pExportAddrs[i];
				pExportAddrs[i] = &XFME_NdisRegisterProtocol;
			}

			//
			// Hook NdisSend For Windows Me
			//
			if(strcmp(pExportNameList[i], "NdisSend") == 0)
			{
				dprintf( "  Hooking NdisSend\n" );
				m_pSysNdisSend = pExportAddrs[i];
				pExportAddrs[i] = &XFME_NdisSend;
			}
		}
	}

	//
	// 转发给系统相应函数
	//
	return m_pSystemAddExportTable(
				pht,
				pszModuleName,
				cExportedFunctions,
				cExportedNames,
				ulOrdinalBase,
				pExportNameList,
				pExportOrdinals,
				pExportAddrs,
				phetl
				);
}

//
// Our NdisSend For Win95/98
//
VOID NDIS_API
XF_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
)
{
	dprintf("XF_NdisSend\n");

	//
	// 检查封包合法性
	//
	if(CheckSend(Packet) != 0)
	{
		*Status = NDIS_STATUS_SUCCESS;
		return;
	}

	//
	// 转发给系统 NdisSend
	//
	m_pNdisSend(Status,	NdisBindingHandle, Packet);
}

//
// Our ProtocolRecvive For Win95/98/ME
//
NDIS_STATUS NDIS_API
XF_Receive(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
)
{
	NDIS_STATUS status;
	dprintf("XF_Receive\n");

	//
	// 检查封包的合法性
	//
	if(CheckRecv(HeaderBuffer,HeaderBufferSize,	LookAheadBuffer,
		LookaheadBufferSize,PacketSize) != 0) 
		return NDIS_STATUS_SUCCESS;

	//
	// 转发给系统函数
	//
	status = m_pNdisReceive(
		NdisBindingContext,
		MacReceiveContext,
		HeaderBuffer,
		HeaderBufferSize,
		LookAheadBuffer,
		LookaheadBufferSize,
		PacketSize
		);
	return NDIS_STATUS_SUCCESS;
}


UINT m_MajorNdisVersion = 0;
UINT m_MinorNdisVersion = 0;

//
// Our NdisRegisterProtocol For Win95/98
// 这里仅仅对TCP/IP协议的接收函数进行HOOK
//
VOID NDIS_API
XF_NdisRegisterProtocol(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
)
{
	NDIS_STRING sTcpName = NDIS_STRING_CONST("MSTCP");

	if(m_pNdisRegisterProtocol == NULL)
		return;

	//
	// 判断Protocol的版本以及名称是否符合
	//
	if(ProtocolCharacteristics->MajorNdisVersion == 3
		&& ProtocolCharacteristics->MinorNdisVersion == 0x0A  
		&& NdisEqualString(&ProtocolCharacteristics->Name, &sTcpName, FALSE) 
		)
	{
		m_pNdisReceive = ProtocolCharacteristics->ReceiveHandler;
		ProtocolCharacteristics->ReceiveHandler = XF_Receive;
	}

	//
	// 转发给系统函数
	//
	m_pNdisRegisterProtocol(
		Status,
		NdisProtocolHandle,
		ProtocolCharacteristics,
		CharacteristicsLength
		);
}

//
// Our NdisDeregisterProtocol For Win95/98
// 这里没有做处理，直接转发
//
VOID NDIS_API
XF_NdisDeregisterProtocol(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
    )
{
	if(m_pNdisDeregisterProtocol == NULL)
		return;

	m_pNdisDeregisterProtocol(
		Status,
		NdisProtocolHandle
		);
}

//
// Out NdisSend For Windows Me
// 
VOID NDIS_API
XFME_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
)
{
	dprintf("XFME_NdisSend\n");

	//
	// 检查数据封包的合法性
	//
	if(CheckSend(Packet) != 0)
	{
		*Status = NDIS_STATUS_SUCCESS;
		return;
	}

	//
	// 转发给系统函数
	//
	m_pSysNdisSend(Status,	NdisBindingHandle, Packet);
}

//
// Our NdisRegisterProtocol For Windows Me
// 这里仅仅对TCP/IP协议的接收函数进行HOOK
//
VOID NDIS_API
XFME_NdisRegisterProtocol(
   PNDIS_STATUS Status,
   PNDIS_HANDLE NdisProtocolHandle,
   PNDIS50_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
   UINT CharacteristicsLength
)
{
	UNICODE_STRING  usTcpName = UNICODE_STRING_CONST("MSTCP");

	dprintf("XFME_NdisRegisterProtocol\n");

	//
	// 判断Protocol的版本以及名称是否符合
	//
	if(ProtocolCharacteristics->MajorNdisVersion != 3
		&& ProtocolCharacteristics->MinorNdisVersion != 0x0A
		&& usTcpName.Length == ProtocolCharacteristics->Name.Length 
		&& memcmp(ProtocolCharacteristics->Name.Buffer, usTcpName.Buffer, usTcpName.Length) == 0)
	{
		m_MajorNdisVersion = ProtocolCharacteristics->MajorNdisVersion;
		m_MinorNdisVersion = ProtocolCharacteristics->MinorNdisVersion;
		dprintf("HookTcp, MajorNdisVersion: %u, MinorNdisVersion: 0x%08X\n"
			, ProtocolCharacteristics->MajorNdisVersion
			, ProtocolCharacteristics->MinorNdisVersion
			);
		m_pNdisReceive = ProtocolCharacteristics->ReceiveHandler;
		ProtocolCharacteristics->ReceiveHandler = XF_Receive;
	}
 
	//
	// 转发给系统函数
	//
	m_pSysNdisRegisterProtocol(
		Status,
		NdisProtocolHandle,
		(PNDIS_PROTOCOL_CHARACTERISTICS )ProtocolCharacteristics,
		CharacteristicsLength
		);
}

#pragma comment( exestr, "B9D3B8FD2A70666B756A71716D2B")
