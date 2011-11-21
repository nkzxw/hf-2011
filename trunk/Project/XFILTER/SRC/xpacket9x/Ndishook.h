// NDISHOOK.H

#ifndef __NDISHOOK_H__
#define __NDISHOOK_H__

typedef
INT
(NDIS_API *RECEIVE_PACKET_HANDLER)(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet
	);

typedef
NDIS_STATUS
(NDIS_API *PNP_EVENT_HANDLER)(
	IN	NDIS_HANDLE       ProtocolBindingContext,
	IN	PVOID             NetPnPEvent
	);

typedef struct _NDIS30_PROTOCOL_CHARACTERISTICS
{
	UCHAR							MajorNdisVersion;
	UCHAR							MinorNdisVersion;
	USHORT							Filler;
	union
	{
		UINT						Reserved;
		UINT						Flags;
	};
	OPEN_ADAPTER_COMPLETE_HANDLER	OpenAdapterCompleteHandler;
	CLOSE_ADAPTER_COMPLETE_HANDLER	CloseAdapterCompleteHandler;
	SEND_COMPLETE_HANDLER			SendCompleteHandler;
	TRANSFER_DATA_COMPLETE_HANDLER	TransferDataCompleteHandler;

	RESET_COMPLETE_HANDLER			ResetCompleteHandler;
	REQUEST_COMPLETE_HANDLER		RequestCompleteHandler;
	RECEIVE_HANDLER					ReceiveHandler;
	RECEIVE_COMPLETE_HANDLER		ReceiveCompleteHandler;
	STATUS_HANDLER					StatusHandler;
	STATUS_COMPLETE_HANDLER			StatusCompleteHandler;

	UNICODE_STRING					Name;
} NDIS30_PROTOCOL_CHARACTERISTICS, *PNDIS30_PROTOCOL_CHARACTERISTICS;


typedef struct _NDIS40_PROTOCOL_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS30_PROTOCOL_CHARACTERISTICS	Ndis30Chars;
#else
	NDIS30_PROTOCOL_CHARACTERISTICS;
#endif

	RECEIVE_PACKET_HANDLER     ReceivePacketHandler;

	BIND_ADAPTER_HANDLER       BindAdapterHandler;
	UNBIND_ADAPTER_HANDLER     UnbindAdapterHandler;
	PNP_EVENT_HANDLER          PnPEventHandler;
	UNLOAD_PROTOCOL_HANDLER    UnloadProtocolHandler;

} NDIS40_PROTOCOL_CHARACTERISTICS, *PNDIS40_PROTOCOL_CHARACTERISTICS;


typedef struct _NDIS50_PROTOCOL_CHARACTERISTICS
{
#ifdef __cplusplus
	NDIS40_PROTOCOL_CHARACTERISTICS	Ndis40Chars;
#else
	NDIS40_PROTOCOL_CHARACTERISTICS;
#endif
	
	PVOID						ReservedHandlers[4];

	PVOID                      CoSendCompleteHandler;
	PVOID                      CoStatusHandler;
	PVOID                      CoReceivePacketHandler;
	PVOID                      CoAfRegisterNotifyHandler;

} NDIS50_PROTOCOL_CHARACTERISTICS, *PNDIS50_PROTOCOL_CHARACTERISTICS;


#ifndef __NdisGetVersion

enum
{
   __NdisGetVersion = (NDIS_DEVICE_ID<<16),
   __NdisAllocateSpinLock,
   __NdisFreeSpinLock,
   __NdisAcquireSpinLock,
   __NdisReleaseSpinLock,

   __NdisOpenConfiguration,
   __NdisReadConfiguration,
   __NdisCloseConfiguration,
   __NdisReadEisaSlotInformation, 
   __NdisReadMcaPosInformation,

   __NdisAllocateMemory,
   __NdisFreeMemory,
   __NdisSetTimer,
   __NdisCancelTimer,
   __NdisStallExecution,
   __NdisInitializeInterrupt,
   __NdisRemoveInterrupt,
   __NdisSynchronizeWithInterrupt,
   __NdisOpenFile,
   __NdisMapFile,
   __NdisUnmapFile,
   __NdisCloseFile,

   __NdisAllocatePacketPool,
   __NdisFreePacketPool,
   __NdisAllocatePacket,
   __NdisReinitializePacket,
   __NdisFreePacket,
   __NdisQueryPacket,

   __NdisAllocateBufferPool,
   __NdisFreeBufferPool,
   __NdisAllocateBuffer,
   __NdisCopyBuffer,
   __NdisFreeBuffer,
   __NdisQueryBuffer,
   __NdisGetBufferPhysicalAddress,
   __NdisChainBufferAtFront,
   __NdisChainBufferAtBack,
   __NdisUnchainBufferAtFront,
   __NdisUnchainBufferAtBack,
   __NdisGetNextBuffer,
   __NdisCopyFromPacketToPacket,

   __NdisRegisterProtocol,
   __NdisDeregisterProtocol,
   __NdisOpenAdapter,
   __NdisCloseAdapter,
   __NdisSend,
   __NdisTransferData,
   __NdisReset,
   __NdisRequest,

   __NdisInitializeWrapper,
   __NdisTerminateWrapper,
   __NdisRegisterMac,
   __NdisDeregisterMac,
   __NdisRegisterAdapter,
   __NdisDeregisterAdapter,
   __NdisCompleteOpenAdapter,
   __NdisCompleteCloseAdapter,
   __NdisCompleteSend,
   __NdisCompleteTransferData,
   __NdisCompleteReset,
   __NdisCompleteRequest,
   __NdisIndicateReceive,
   __NdisIndicateReceiveComplete,
   __NdisIndicateStatus,
   __NdisIndicateStatusComplete,
   __NdisCompleteQueryStatistics,

   __NdisEqualString,
   __NdisRegAdaptShutdown,
   __NdisReadNetworkAddress,

   __NdisWriteErrorLogEntry,

   __NdisMapIoSpace,
   __NdisDeregAdaptShutdown,

   __NdisAllocateSharedMemory,
   __NdisFreeSharedMemory, 

   __NdisAllocateDmaChannel, 
   __NdisSetupDmaTransfer, 
   __NdisCompleteDmaTransfer, 
   __NdisReadDmaCounter, 
   __NdisFreeDmaChannel, 
   __NdisReleaseAdapterResources, 
   __NdisQueryGlobalStatistics, 

   __NdisOpenProtocolConfiguration, 
   __NdisCompleteBindAdapter, 
   __NdisCompleteUnbindAdapter, 
   __WrapperStartNet, 
   __WrapperGetComponentList, 
   __WrapperQueryAdapterResources,
   __WrapperDelayBinding,
   __WrapperResumeBinding,
   __WrapperRemoveChildren,
   __NdisImmediateReadPciSlotInformation,
   __NdisImmediateWritePciSlotInformation,
   __NdisReadPciSlotInformation,
   __NdisWritePciSlotInformation,
   __NdisPciAssignResources
};

#endif // __NdisGetVersion


#define UNICODE_STRING_CONST(x)	{sizeof(L##x)-2, sizeof(L##x), L##x}
extern UINT m_MajorNdisVersion;
extern UINT m_MinorNdisVersion;


//typedef LRESULT __cdecl ADD_EXPORT_TABLE(
typedef LRESULT CDECL ADD_EXPORT_TABLE(
	PHPEEXPORTTABLE     pht,
	PSTR                pszModuleName,
	ULONG               cExportedFunctions,
	ULONG               cExportedNames,
	ULONG               ulOrdinalBase,
	PVOID               *pExportNameList,
	PUSHORT             pExportOrdinals,
	PVOID               *pExportAddrs,
	PHLIST              phetl
);
typedef ADD_EXPORT_TABLE *PADD_EXPORT_TABLE;


typedef VOID NDIS_API NDIS_SEND(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	NDIS_HANDLE RequestHandle
);
typedef NDIS_SEND *PNDIS_SEND;



NTSTATUS Hook_Ndis_Function();

LRESULT __cdecl XF_PELDR_AddExportTable(
	PHPEEXPORTTABLE     pht,
	PSTR                pszModuleName,
	ULONG               cExportedFunctions,
	ULONG               cExportedNames,
	ULONG               ulOrdinalBase,
	PVOID               *pExportNameList,
	PUSHORT             pExportOrdinals,
	PVOID               *pExportAddrs,
	PHLIST              phetl
);

VOID NDIS_API
XF_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
);




typedef VOID  
(NDIS_API *NDIS_REGISTER_PROTOCOL)(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
    );

typedef VOID  
(NDIS_API *NDIS_DEREGISTER_PROTOCOL)(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
    );

extern NDIS_REGISTER_PROTOCOL	m_pNdisRegisterProtocol;
extern NDIS_DEREGISTER_PROTOCOL m_pNdisDeregisterProtocol;
extern RECEIVE_HANDLER			m_pNdisReceive;

VOID NDIS_API
XF_NdisRegisterProtocol(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
    );

VOID NDIS_API
XF_NdisDeregisterProtocol(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
    );

NDIS_STATUS NDIS_API
XF_NdisReceive(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
);

VOID NDIS_API
XFME_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
);

VOID NDIS_API
XFME_NdisRegisterProtocol(
   PNDIS_STATUS Status,
   PNDIS_HANDLE NdisProtocolHandle,
   PNDIS50_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
   UINT CharacteristicsLength
);


#endif //__NDISHOOK_H__