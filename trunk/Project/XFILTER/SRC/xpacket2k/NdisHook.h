// NdisHook.h

//#include <ndis.h>
//#include "NtApi.h"

#define NDIS_API __stdcall

struct _NDIS_PROTOCOL_BLOCK
{
	PNDIS_OPEN_BLOCK				OpenQueue;				// queue of opens for this protocol
	REFERENCE						Ref;					// contains spinlock for OpenQueue
	UINT							Length;					// of this NDIS_PROTOCOL_BLOCK struct
	NDIS50_PROTOCOL_CHARACTERISTICS	ProtocolCharacteristics;// handler addresses

	struct _NDIS_PROTOCOL_BLOCK *	NextProtocol;			// Link to next
	ULONG							MaxPatternSize;
#if defined(NDIS_WRAPPER)
	//
	// Protocol filters
	//
	struct _NDIS_PROTOCOL_FILTER *	ProtocolFilter[NdisMediumMax+1];
	WORK_QUEUE_ITEM					WorkItem;				// Used during NdisRegisterProtocol to
															// notify protocols of existing drivers.
	KMUTEX							Mutex;					// For serialization of Bind/Unbind requests
	PKEVENT							DeregEvent;				// Used by NdisDeregisterProtocol
#endif
};


#define UNICODE_STRING_CONST(x)	{sizeof(L##x)-2, sizeof(L##x), L##x}



typedef VOID NDIS_API NDIS_SEND(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	NDIS_HANDLE RequestHandle
);
typedef NDIS_SEND *PNDIS_SEND;

typedef VOID  
(NDIS_API *NDIS_REGISTER_PROTOCOL)(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
    );

//
// 2002/08/21 add
//
typedef VOID
(NDIS_API *NDIS_DEREGISTER_PROTOCOL)(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
    );

typedef
NDIS_STATUS
(NDIS_API *SEND_HANDLER)(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet
	);

typedef
VOID 
(NDIS_API *NDIS_OPEN_ADAPTER)(
	OUT PNDIS_STATUS  Status,
	OUT PNDIS_STATUS  OpenErrorStatus,
	OUT PNDIS_HANDLE  NdisBindingHandle,
	OUT PUINT  SelectedMediumIndex,
	IN PNDIS_MEDIUM  MediumArray,
	IN UINT  MediumArraySize,
	IN NDIS_HANDLE  NdisProtocolHandle,
	IN NDIS_HANDLE  ProtocolBindingContext,
	IN PNDIS_STRING  AdapterName,
	IN UINT  OpenOptions,
	IN PSTRING  AddressingInformation  OPTIONAL
	);


VOID NDIS_API
XF_NdisSend(
	PNDIS_STATUS Status,
	NDIS_HANDLE NdisBindingHandle,
	PNDIS_PACKET Packet
);

VOID NDIS_API
XF_NdisRegisterProtocol(
    OUT PNDIS_STATUS  Status,
    OUT PNDIS_HANDLE  NdisProtocolHandle,
    IN PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN UINT  CharacteristicsLength
    );

//
// 2002/08/21 add
//
VOID NDIS_API
XF_NdisDeregisterProtocol(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle
    );


NDIS_STATUS NDIS_API
XF_Receive(
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookAheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
);

VOID NDIS_API
XF_NdisOpenAdapter(
	OUT PNDIS_STATUS  Status,
	OUT PNDIS_STATUS  OpenErrorStatus,
	OUT PNDIS_HANDLE  NdisBindingHandle,
	OUT PUINT  SelectedMediumIndex,
	IN PNDIS_MEDIUM  MediumArray,
	IN UINT  MediumArraySize,
	IN NDIS_HANDLE  NdisProtocolHandle,
	IN NDIS_HANDLE  ProtocolBindingContext,
	IN PNDIS_STRING  AdapterName,
	IN UINT  OpenOptions,
	IN PSTRING  AddressingInformation  OPTIONAL
);

VOID NDIS_API
XF_OpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
);

NDIS_STATUS NDIS_API
XF_SendPacket(
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet
);

VOID 
XF_HookSend(
	IN NDIS_HANDLE	ProtocolBlock, 
	IN PVOID		HookFunction,
	OUT PVOID*		SendHandler,
	IN BYTE			HookType
);

extern NDIS_HANDLE				m_TcpipHandle;

extern PNDIS_SEND				m_pNdisSend;
extern NDIS_REGISTER_PROTOCOL	m_pNdisRegisterProtocol;
extern RECEIVE_HANDLER			m_pNdisReceive;
extern SEND_HANDLER				m_pSendHandler;
extern NDIS_OPEN_ADAPTER		m_pNdisOpenAdapter;
extern OPEN_ADAPTER_COMPLETE_HANDLER m_pOpenAdapterComplete;

//
// 2002/08/21 add
//
extern VOID HookLocalSend();

//
// 2002/08/21 add
//
extern NDIS_DEREGISTER_PROTOCOL m_pNdisDeregisterProtocol;


