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
// NDISHOOK.C
//
// ��飺
//		��Ҫ��ɶ� NdisSend �� ProtocolReceive ������Hook������
//		ProtocolRecvive����RegisterProtocolʱע��ģ�������Ҫ��
//		NdisRegisterProtocol������Hook��
//		���⣬Windows 95/98 �� Windows ME ��Hook��ʽ��ͬ��
//		Windows 95/98 ����ֱ��ͨ��Hook_Device_Service Hook��
//		NdisSend �� NdisRegisterProtocol����Windows Me����Ҫ����
//		Hook һ������PELDR_AddExportTable�ĺ�����Ȼ��ͨ���������
//		Hook NdisSend �� NdisRegisterProtocol��
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
// Hook ��Ҫ�� Hook �ĺ���
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
// Hook���躯��For Windows ME
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
	// ת����ϵͳ��Ӧ����
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
	// ������Ϸ���
	//
	if(CheckSend(Packet) != 0)
	{
		*Status = NDIS_STATUS_SUCCESS;
		return;
	}

	//
	// ת����ϵͳ NdisSend
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
	// ������ĺϷ���
	//
	if(CheckRecv(HeaderBuffer,HeaderBufferSize,	LookAheadBuffer,
		LookaheadBufferSize,PacketSize) != 0) 
		return NDIS_STATUS_SUCCESS;

	//
	// ת����ϵͳ����
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
// ���������TCP/IPЭ��Ľ��պ�������HOOK
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
	// �ж�Protocol�İ汾�Լ������Ƿ����
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
	// ת����ϵͳ����
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
// ����û��������ֱ��ת��
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
	// ������ݷ���ĺϷ���
	//
	if(CheckSend(Packet) != 0)
	{
		*Status = NDIS_STATUS_SUCCESS;
		return;
	}

	//
	// ת����ϵͳ����
	//
	m_pSysNdisSend(Status,	NdisBindingHandle, Packet);
}

//
// Our NdisRegisterProtocol For Windows Me
// ���������TCP/IPЭ��Ľ��պ�������HOOK
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
	// �ж�Protocol�İ汾�Լ������Ƿ����
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
	// ת����ϵͳ����
	//
	m_pSysNdisRegisterProtocol(
		Status,
		NdisProtocolHandle,
		(PNDIS_PROTOCOL_CHARACTERISTICS )ProtocolCharacteristics,
		CharacteristicsLength
		);
}

#pragma comment( exestr, "B9D3B8FD2A70666B756A71716D2B")
