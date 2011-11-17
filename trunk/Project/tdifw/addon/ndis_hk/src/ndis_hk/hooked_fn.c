// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: hooked_fn.c,v 1.10 2003/07/07 11:41:55 dev Exp $

/** @addtogroup hook_driver
 *@{
 */

/**
 * @file hooked_fn.c
 * Hooked NDIS functions
 */

#include <ntddk.h>
#include <tdikrnl.h>			// for TdiCopyBufferToMdl

#include "adapters.h"
#include "av.h"
#include "memtrack.h"
#include "ndis_hk.h"
#include "filter.h"
#include "except.h"

/* prototypes */

static VOID		new_OpenAdapterCompleteHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
	IN NDIS_HANDLE				ProtocolBindingContext,
    IN NDIS_STATUS				Status,
    IN NDIS_STATUS				OpenErrorStatus);

static NDIS_STATUS	new_ReceiveHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
    IN NDIS_HANDLE				ProtocolBindingContext,
    IN NDIS_HANDLE				MacReceiveContext,
    IN PVOID					HeaderBuffer,
    IN UINT						HeaderBufferSize,
    IN PVOID					LookaheadBuffer,
    IN UINT						LookaheadBufferSize,
    IN UINT						PacketSize);

static VOID		new_TransferDataCompleteHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
    IN NDIS_HANDLE				ProtocolBindingContext,
    IN PNDIS_PACKET				Packet,
    IN NDIS_STATUS				Status,
    IN UINT						BytesTransferred);

static INT		new_ReceivePacketHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet);

static VOID			new_SendCompleteHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
    IN NDIS_HANDLE				ProtocolBindingContext,
    IN PNDIS_PACKET				Packet,
    IN NDIS_STATUS				Status);

static VOID			new_RequestCompleteHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status);

static NDIS_STATUS	new_PnPEventHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
    IN NDIS_HANDLE				ProtocolBindingContext,
    IN PNET_PNP_EVENT			NetPnPEvent
    );

static NDIS_STATUS	new_SendHandler(
	struct ADAPTER_PROTOCOL		*adapter,		/* added by ASM stub */
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet);

static VOID			new_SendPacketsHandler(
	struct ADAPTER_PROTOCOL		*adapter,		/* added by ASM stub */
	IN NDIS_HANDLE				NdisBindingHandle,
	IN PPNDIS_PACKET			PacketArray,
	IN UINT						NumberOfPackets);

static NDIS_STATUS	new_TransferDataHandler(
	struct ADAPTER_PROTOCOL		*adapter,		/* added by ASM stub */
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer,
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred);

/* typedefs */

#define ASM_STUB_SIZE		12		/**< pop eax; push context; push eax; jmp new_fn */

/**
 * generate ASM code: pop eax; push pchars; push eax; jmp new_fn .
 * eax is ret address; we insert new first parameter for function - pchars
 */
#define GENERATE_ASM_STUB(pchars, fn_name)																	\
	do {																									\
		(pchars)->asm_##fn_name[0] = 0x58;	/* pop eax */													\
		(pchars)->asm_##fn_name[1] = 0x68;	/* push pchars */												\
		*(ULONG *)&(pchars)->asm_##fn_name[2] = (ULONG)(pchars);											\
		(pchars)->asm_##fn_name[6] = 0x50;	/* push eax */													\
		(pchars)->asm_##fn_name[7] = 0xe9;	/* jmp new_fn */												\
		*(ULONG *)&(pchars)->asm_##fn_name[8] = (UCHAR *)new_##fn_name - &(pchars)->asm_##fn_name[12];		\
	} while (0)

#define PCHARS_MAGIC		'PcHa'		/**< magic prefix for struct PROTOCOL_CHARS */

/** information about registered protocol */
struct PROTOCOL_CHARS {

	ULONG		magic;					/**< PCHARS_MAGIC */

	/* ASM stubs for NDIS_PROTOCOL_CHARACTERISTICS */

	UCHAR		asm_OpenAdapterCompleteHandler[ASM_STUB_SIZE];	/**< asm stub for OpenAdapterCompleteHandler */
	
	UCHAR		asm_ReceiveHandler[ASM_STUB_SIZE];				/**< asm stub for ReceiveHandler */
	UCHAR		asm_TransferDataCompleteHandler[ASM_STUB_SIZE];	/**< asm stub for TransferDataCompleteHandler */

	UCHAR		asm_ReceivePacketHandler[ASM_STUB_SIZE];		/**< asm stub for ReceivePacketHandler */

	UCHAR		asm_SendCompleteHandler[ASM_STUB_SIZE];			/**< asm stub for SendCompleteHandler */
	UCHAR		asm_RequestCompleteHandler[ASM_STUB_SIZE];		/**< asm stub for RequestCompleteHandler */
	UCHAR		asm_PnPEventHandler[ASM_STUB_SIZE];				/**< asm stub for PnPEventHandler */

	struct		ADAPTER_PROTOCOL *adapter;		/**< single-linked list of open adapters */

	UINT		chars_len;						/**< size of old NDIS_PROTOCOL_CHARACTERISTICS */
	char		chars[0];	/**< old NDIS_PROTOCOL_CHARACTERISTICS and new NDIS_PROTOCOL_CHARACTERISTICS */
};

/** macro to get old NDIS_PROTOCOL_CHARACTERISTICS from struct PROTOCOL_CHARS */
#define PCHARS_OLD_CHARS(pchars)	((PNDIS_PROTOCOL_CHARACTERISTICS)((pchars)->chars))
/** macro to new old NDIS_PROTOCOL_CHARACTERISTICS from struct PROTOCOL_CHARS */
#define PCHARS_NEW_CHARS(pchars)	((PNDIS_PROTOCOL_CHARACTERISTICS)((pchars)->chars + (pchars)->chars_len))

/** @def _CHECK_PCHARS macro to check magic of struct PROTOCOL_CHARS */

#if DBG
#	define _CHECK_PCHARS(pchars)			\
	if ((pchars)->magic != PCHARS_MAGIC) {	\
		__asm int 3							\
	}
#else
#	define _CHECK_PCHARS(pchars)
#endif

/** number of pchars of TCPIP protocol */
#define TCPIP_PCHARS_N		2

/** global pchars for TCPIP protocol [0] & TCPIP/WAN [1] */
static struct PROTOCOL_CHARS *g_tcpip_pchars[TCPIP_PCHARS_N];

/** information about opened adapter by protocol */
struct ADAPTER_PROTOCOL {

	struct		ADAPTER_PROTOCOL *next;		/**< next adapter in single-linked list of open adapters */
	struct		PROTOCOL_CHARS *pchars;		/**< related pchars structure */
	
	int			adapter_index;				/**< index of adapter */
	NDIS_MEDIUM	medium;						/**< medium (ethernet for example) of adapter */

	NDIS_HANDLE NdisBindingHandle;			/**< NdisBindingHandle for this adapter */
	NDIS_HANDLE	ProtocolBindingContext;		/**< ProtocolBindingContext for this adapter */

	/* temporary storage */
	PUINT			pSelectedMediumIndex;	/**< pSelectedMediumIndex pointer for NdisOpenAdapter completion */
	PNDIS_MEDIUM	pMediumArray;			/**< pMediumArray pointer for NdisOpenAdapter completion */
	PNDIS_HANDLE	pNdisBindingHandle;		/**< pNdisBindingHandle pointer for NdisOpenAdapter completion */

	/* hooked SendHandler & SendPacketsHandler */

	/** ASM stub to call new SendHandler */
	UCHAR			asm_SendHandler[ASM_STUB_SIZE];
	/** original SendHandler address */
	SEND_HANDLER	old_SendHandler;
	
	/** ASM stub to call new SendPacketsHandler */
	UCHAR					asm_SendPacketsHandler[ASM_STUB_SIZE];
	/** original SendPacketsHandler address */
	SEND_PACKETS_HANDLER	old_SendPacketsHandler;

	/* hooked TransferDataHandler */

	/** ASM stub to call new TransferDataHandler */
	UCHAR					asm_TransferDataHandler[ASM_STUB_SIZE];
	/** original old_TransferDataHandler address */
	TRANSFER_DATA_HANDLER	old_TransferDataHandler;

	/** adapter name */
	wchar_t		adapter_name[0];
};

/** variables for NdisRequest */
static struct {
	KMUTEX			guard;		/**< guard mutex */
	NDIS_EVENT		event;		/**< event for completion */
	NDIS_STATUS		status;		/**< status of completion */
	NDIS_REQUEST	*pend_req;	/**< pending request */
} g_request;

/*
 * --- hooked NDIS functions ---
 */

/**
 * Hooked NdisRegisterProtocol.
 * If protocol is TCP/IP related, function makes copy of
 * NDIS_PROTOCOL_CHARACTERISTICS, changed some functions in it, creates struct PROTOCOL_CHARS
 * and calls original NdisRegisterProtocol function.
 * PROTOCOL_CHARS stored in "av" with NdisProtocolHandle as key.
 */
VOID
new_NdisRegisterProtocol(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_HANDLE			NdisProtocolHandle,
	IN	PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
	IN	UINT					CharacteristicsLength)
{
	struct PROTOCOL_CHARS *pchars = NULL;
	PNDIS_PROTOCOL_CHARACTERISTICS new_ProtocolCharacteristics;
	NTSTATUS status;

	// working at PASSIVE_LEVEL - can use UNICODE %S to output (see DbgPrint documentation)
	KdPrint(("[ndis_hk] new_NdisRegisterProtocol: %S\n", ProtocolCharacteristics->Name.Buffer));
	
	__try {
		
		// working only with "TCPIP" (and "TCPIP_WANARP" (2k) or "RASARP" protocol (NT))
		if (wcscmp(ProtocolCharacteristics->Name.Buffer, L"TCPIP") != 0 &&
			wcscmp(ProtocolCharacteristics->Name.Buffer, L"TCPIP_WANARP") != 0 &&
			wcscmp(ProtocolCharacteristics->Name.Buffer, L"RASARP") != 0) {
			
			HOOKED_OLD_FN(NdisRegisterProtocol)(Status, NdisProtocolHandle,
				ProtocolCharacteristics, CharacteristicsLength);
			
			KdPrint(("[ndis_hk] new_NdisRegisterProtocol(!TCPIP): 0x%x\n", *Status));
			__leave;
		}
		
		// create PROTOCOL_TO_CHARS
		pchars = (struct PROTOCOL_CHARS *)malloc_np(sizeof(*pchars) + CharacteristicsLength * 2);
		if (pchars == NULL) {
			
			KdPrint(("[ndis_hk] new_NdisRegisterProtocol: malloc_np!\n"));
			
			// simulate error
			*Status = NDIS_STATUS_RESOURCES;
			__leave;
		}
		memset(pchars, 0, sizeof(*pchars) + CharacteristicsLength);
		
		pchars->magic = PCHARS_MAGIC;
		
		// save copy of ProtocolCharacteristics
		pchars->chars_len = CharacteristicsLength;
		memcpy(PCHARS_OLD_CHARS(pchars), ProtocolCharacteristics, CharacteristicsLength);
		
		// generate ASM code for handlers
		
		GENERATE_ASM_STUB(pchars, OpenAdapterCompleteHandler);
		GENERATE_ASM_STUB(pchars, ReceiveHandler);
		GENERATE_ASM_STUB(pchars, TransferDataCompleteHandler);
		GENERATE_ASM_STUB(pchars, ReceivePacketHandler);
		GENERATE_ASM_STUB(pchars, SendCompleteHandler);
		GENERATE_ASM_STUB(pchars, RequestCompleteHandler);
		GENERATE_ASM_STUB(pchars, PnPEventHandler);

		// prepare new ProtocolCharacteristics
		new_ProtocolCharacteristics = PCHARS_NEW_CHARS(pchars);
		
		memcpy(new_ProtocolCharacteristics, ProtocolCharacteristics, CharacteristicsLength);
		
		// patch new ProtocolCharacteristics
		
		KdPrint(("[ndis_hk] new_NdisRegisterProtocol: MajorNdisVersion %u (len: %u)\n",
			new_ProtocolCharacteristics->MajorNdisVersion, CharacteristicsLength));
		
		new_ProtocolCharacteristics->OpenAdapterCompleteHandler = (OPEN_ADAPTER_COMPLETE_HANDLER)(pchars->asm_OpenAdapterCompleteHandler);
		
		if (new_ProtocolCharacteristics->ReceiveHandler != NULL)
			new_ProtocolCharacteristics->ReceiveHandler = (RECEIVE_HANDLER)(pchars->asm_ReceiveHandler);
		
		if (new_ProtocolCharacteristics->TransferDataCompleteHandler != NULL)
			new_ProtocolCharacteristics->TransferDataCompleteHandler =
			(TRANSFER_DATA_COMPLETE_HANDLER)(pchars->asm_TransferDataCompleteHandler);
		
		if (new_ProtocolCharacteristics->MajorNdisVersion >= 4 &&
			new_ProtocolCharacteristics->ReceivePacketHandler != NULL)
			new_ProtocolCharacteristics->ReceivePacketHandler = (RECEIVE_PACKET_HANDLER)(pchars->asm_ReceivePacketHandler);
		
		// not checking for NULL! this function must be defined see new_SendCompleteHandler
		new_ProtocolCharacteristics->SendCompleteHandler = (SEND_COMPLETE_HANDLER)(pchars->asm_SendCompleteHandler);

		new_ProtocolCharacteristics->RequestCompleteHandler = (REQUEST_COMPLETE_HANDLER)(pchars->asm_RequestCompleteHandler);
		
		if (new_ProtocolCharacteristics->MajorNdisVersion >= 4)
			new_ProtocolCharacteristics->PnPEventHandler = (PNP_EVENT_HANDLER)(pchars->asm_PnPEventHandler);

		// call original function with new ProtocolCharacteristics
		HOOKED_OLD_FN(NdisRegisterProtocol)(Status, NdisProtocolHandle,
			new_ProtocolCharacteristics, CharacteristicsLength);
		
		KdPrint(("[ndis_hk] new_NdisRegisterProtocol: 0x%x\n", *Status));
		
		if (*Status != NDIS_STATUS_SUCCESS)
			__leave;
		
		// for "TCPIP" protocol save pchars
		if (wcscmp(ProtocolCharacteristics->Name.Buffer, L"TCPIP") == 0) {
			g_tcpip_pchars[0] = pchars;

			// and initialize some related globals
			KeInitializeMutex(&g_request.guard, 0);
			NdisInitializeEvent(&g_request.event);
			g_request.pend_req = NULL;
		
		} else {
			// working with TCPIP/WAN protocol
			g_tcpip_pchars[1] = pchars;
		}
		
		// save NdisProtocolHandle -> pchars
		status = add_av(*NdisProtocolHandle, pchars, PROTOCOL_TO_PCHARS, FALSE);
		if (status != STATUS_SUCCESS) {
			
			// deregister protocol
			NdisDeregisterProtocol(Status, *NdisProtocolHandle);
			
			// simulate error
			*Status = NDIS_STATUS_RESOURCES;
			__leave;
		}
		
		// don't free pchars
		pchars = NULL;
	
	} __except((*Status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)) {
		KdPrint(("[ndis_hk] new_NdisRegisterProtocol: exception 0x%x!\n", *Status));
	}

	if (pchars != NULL)
		free(pchars);
}

/**
 * Hooked NdisDeregisterProtocol.
 * Function deletes NDIS_PROTOCOL_CHARACTERISTICS by NdisProtocolHandle
 * and calls original NdisDeregisterProtocol function
 */
VOID
new_NdisDeregisterProtocol(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  NdisProtocolHandle)
{
	// delete NdisProtocolHandle -> pchars
	del_av(NdisProtocolHandle, PROTOCOL_TO_PCHARS, FALSE);

	// call original handler
	HOOKED_OLD_FN(NdisDeregisterProtocol)(Status, NdisProtocolHandle);
}

/**
 * Hooked NdisOpenAdapter.
 * Function creates struct ADAPTER_PROTOCOL, calls original NdisOpenAdapter and on success addes
 * ADAPTER_PROTOCOL to related PROTOCOL_CHARS. ADAPTER_PROTOCOL is added in "av" with NdisBindingHandle
 * as key in open adapter completion. ADAPTER_PROTOCOL is added into PROTOCOL_CHARS to help completion
 * find ADAPTER_PROTOCOL by PROTOCOL_CHARS and ProtocolBindingContext.
 */
VOID
new_NdisOpenAdapter(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_STATUS			OpenErrorStatus,
	OUT	PNDIS_HANDLE			NdisBindingHandle,
	OUT	PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				NdisProtocolHandle,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_STRING			AdapterName,
	IN	UINT					OpenOptions,
	IN	PSTRING					AddressingInformation OPTIONAL)
{
	struct PROTOCOL_CHARS *pchars = NULL;
	KIRQL irql;
	struct ADAPTER_PROTOCOL *adapter = NULL;
	ULONG size;
	UINT i;
	NTSTATUS status;
	
	// working at PASSIVE_LEVEL - can use UNICODE %S to output (see DbgPrint documentation)
	KdPrint(("[ndis_hk] new_NdisOpenAdapter: %S (context = 0x%x)\n", AdapterName->Buffer,
		ProtocolBindingContext));

	__try {
		
		/*
		 * search MeduimArray for NdisMedium802_3 or NdisMediumWan
		 */
		
		for (i = 0; i < MediumArraySize; i++) {
			if (MediumArray[i] == NdisMedium802_3 || MediumArray[i] == NdisMediumWan)
				break;
		}
		
		if (i >= MediumArraySize) {
			
			// not found
			KdPrint(("[ndis_hk] new_NdisOpenAdapter: unsupported medium for this adapter\n"));
			
			// anyway call original handler
			*Status = NDIS_STATUS_SUCCESS;
			__leave;
		}
		
		// get pchars
		pchars = (struct PROTOCOL_CHARS *)get_av(NdisProtocolHandle, PROTOCOL_TO_PCHARS, &irql);
		if (pchars == NULL) {
			
			KdPrint(("[ndis_hk] new_NdisOpenAdapter: get_av(PROTOCOL_TO_PCHARS)!\n"));
			
			// This protocol is not for us. Call original handler but don't call our function.
			*Status = NDIS_STATUS_SUCCESS;
			__leave;
		}
		
		// allocate ADAPTER_PROTOCOL
		size = sizeof(*adapter) + (wcslen(AdapterName->Buffer) + 1) * sizeof(wchar_t);
		adapter = (struct ADAPTER_PROTOCOL *)malloc_np(size);
		if (adapter == NULL) {
			
			KdPrint(("[ndis_hk] new_NdisOpenAdapter: get_av(PROTOCOL_TO_PCHARS)!\n"));
			
			*Status = NDIS_STATUS_RESOURCES;
			__leave;
		}
		memset(adapter, 0, size);
		
		// save copy of AdapterName
		wcscpy(adapter->adapter_name, AdapterName->Buffer);
		
		// save ProtocolBindingContext
		adapter->ProtocolBindingContext = ProtocolBindingContext;
		
		// link adapter with pchars
		adapter->next = pchars->adapter;
		pchars->adapter = adapter;
		
		adapter->pchars = pchars;
		
		if (MediumArraySize > 1) {
			// save temporary pointers
			adapter->pMediumArray = MediumArray;
			adapter->pSelectedMediumIndex = SelectedMediumIndex;
		} else {
			// we have only one index and one chance to choose. do it now.
			adapter->medium = MediumArray[0];
		}
		
		adapter->pNdisBindingHandle = NdisBindingHandle;	// in completion we'll have NdisBindingHandler here
		
		// that's all
		*Status = NDIS_STATUS_SUCCESS;
		
	} __except((*Status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)) {
		KdPrint(("[ndis_hk] new_NdisOpenAdapter: exception 0x%x!\n", *Status));
	}
	
	// cleanup
	if (pchars != NULL)
		KeReleaseSpinLock(&g_av_hash_guard, irql);

	if (*Status != NDIS_STATUS_SUCCESS)
		return;			// no need to call original handler - our errors

	// call original handler
	HOOKED_OLD_FN(NdisOpenAdapter)(Status, OpenErrorStatus, NdisBindingHandle,
		SelectedMediumIndex, MediumArray, MediumArraySize, NdisProtocolHandle,
		ProtocolBindingContext, AdapterName, OpenOptions, AddressingInformation);

	KdPrint(("[ndis_hk] new_NdisOpenAdapter: 0x%x\n", *Status));

	if (*Status == NDIS_STATUS_SUCCESS) {

		/*
		 * support only 802.3 and Wan adapters
		 */
		if (MediumArray[*SelectedMediumIndex] == NdisMedium802_3 ||
			MediumArray[*SelectedMediumIndex] == NdisMediumWan) {

			/*
			 * a little magic: call completion with NDIS_STATUS_PENDING
			 * it means don't call original completion and return status to us
			 */

			if (pchars != NULL)
				new_OpenAdapterCompleteHandler(pchars, ProtocolBindingContext, NDIS_STATUS_PENDING, 0);

			// don't delete adapter
			adapter = NULL;
		}

	} else if (*Status == NDIS_STATUS_PENDING) {

		// don't delete adapter
		adapter = NULL;

	}

	if (adapter != NULL) {

		/* destroy created ADAPTER_PROTOCOL */

		// unlink it from pchars
		pchars = (struct PROTOCOL_CHARS *)get_av(NdisProtocolHandle, PROTOCOL_TO_PCHARS, &irql);
		if (pchars != NULL) {
			// find adapter by pointer
			struct ADAPTER_PROTOCOL *a, *prev_a;
			for (prev_a = NULL, a = pchars->adapter; a != NULL; a = a->next) {
				if (a == adapter) {
					if (prev_a == NULL)
						pchars->adapter = adapter->next;
					else
						prev_a->next = adapter->next;
				}
				prev_a = a;
			}
			KeReleaseSpinLock(&g_av_hash_guard, irql);
		}
	
		// and free
		free(adapter);
	}
}

/**
 * Hooked NdisCloseAdapter.
 * Finds and frees ADAPTER_PROTOCOL by NdisBindingHandle
 */
VOID
new_NdisCloseAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle)
{
	struct ADAPTER_PROTOCOL *adapter, *a, *prev_a;
	KIRQL irql;
	struct PROTOCOL_CHARS *pchars;

	__try {
		
		// get adapter by NdisBindingHandle
		adapter = get_av(NdisBindingHandle, BINDING_TO_ADAPTER, &irql);
		if (adapter == NULL) {
			KdPrint(("[ndis_hk] new_NdisCloseAdapter: get_av(BINDING_TO_ADAPTER)!\n"));
			__leave;
		}
		
		// unlink it from pchars
		pchars = adapter->pchars;
		for (prev_a = NULL, a = pchars->adapter; a != NULL; a = a->next) {
			if (a == adapter) {
				if (prev_a == NULL)
					pchars->adapter = adapter->next;
				else
					prev_a->next = adapter->next;
			}
			prev_a = a;
		}
		
		// delete adapter
		del_av(NdisBindingHandle, BINDING_TO_ADAPTER, TRUE);
		
	} __finally {
		if (adapter != NULL)
			KeReleaseSpinLock(&g_av_hash_guard, irql);
	}

	// call original handler
	HOOKED_OLD_FN(NdisCloseAdapter)(Status, NdisBindingHandle);
}

/*
 * --- NDIS functions from NDIS_PROTOCOL_CHARACTERISTICS ---
 */


/**
 * Hooked OpenAdapterCompleteHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * Function can be called using ASM stub in case of pending of NdisOpenAdapter or
 * function can be called by hooked NdisOpenAdapter when NdisOpenAdapter returns NDIS_STATUS_SUCCESS.
 * In last case Status == NDIS_STATUS_PENDING
 */
VOID
new_OpenAdapterCompleteHandler(
	struct PROTOCOL_CHARS *pchars,				/* added by ASM stub */
	IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus)
{
	struct ADAPTER_PROTOCOL *adapter;

	_CHECK_PCHARS(pchars);

	KdPrint(("[ndis_hk] new_OpenAdapterComplete: 0x%x (context = 0x%x)\n", Status,
		ProtocolBindingContext));
	
	__try {

		if (Status != NDIS_STATUS_SUCCESS &&
			Status != NDIS_STATUS_PENDING)		// PENGING is a _magic_ value see above
			__leave;
		
		// get adapter
		for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next) {
			if (adapter->ProtocolBindingContext == ProtocolBindingContext)
				break;
		}
		
		if (adapter == NULL) {
			
			KdPrint(("[ndis_hk] new_OpenAdapterComplete: adapter not found\n"));
			
			// This adapter is not for us.
			__leave;
		}
		
		// save stuff from temporary storage & set temporary storage to zero
		
		adapter->NdisBindingHandle = *(adapter->pNdisBindingHandle);
		adapter->pNdisBindingHandle = NULL;
		
		if (adapter->pMediumArray != NULL &&
			adapter->pSelectedMediumIndex != NULL) {
			
			adapter->medium = adapter->pMediumArray[*(adapter->pSelectedMediumIndex)];
			
			adapter->pMediumArray = NULL;
			adapter->pSelectedMediumIndex = NULL;
		}
		
		if (adapter->medium == NdisMedium802_3 || adapter->medium == NdisMediumWan) {
			PNDIS_OPEN_BLOCK nob;
			
			// assign adapter index
			adapter->adapter_index = add_adapter(adapter->adapter_name);
			if (adapter->adapter_index == 0) {
				KdPrint(("[ndis_hk] new_OpenAdapterComplete: add_adapter!\n"));
				// panic()?
			}
			
			// save mapping NdisBindingHandle -> struct ADAPTER_PROTOCOL
			if (add_av(adapter->NdisBindingHandle, adapter, BINDING_TO_ADAPTER, FALSE) != STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_OpenAdapterComplete: add_av!\n"));
				// panic()?
			}
			
			// can't use UNICODE %S to output (see DbgPrint documentation)
			KdPrint(("[ndis_hk] new_OpenAdapterComplete: (index = %d)\n",
				adapter->adapter_index));
			
			// and now hook SendHandler & SendPacketsHandler in (PNDIS_OPEN_BLOCK)NdisBindingHandle
			nob = (PNDIS_OPEN_BLOCK)adapter->NdisBindingHandle;
			
			adapter->old_SendHandler = nob->SendHandler;
			GENERATE_ASM_STUB(adapter, SendHandler);
			nob->SendHandler = (SEND_HANDLER)adapter->asm_SendHandler;
			
			KdPrint(("[ndis_hk] new_OpenAdapterCompleteHandler: SendHandler: old 0x%x new 0x%x\n",
				adapter->old_SendHandler, adapter->asm_SendHandler));
			
			if (PCHARS_OLD_CHARS(pchars)->MajorNdisVersion >= 4) {
				
				adapter->old_SendPacketsHandler = nob->SendPacketsHandler;
				GENERATE_ASM_STUB(adapter, SendPacketsHandler);
				nob->SendPacketsHandler = (SEND_PACKETS_HANDLER)adapter->asm_SendPacketsHandler;
				
				KdPrint(("[ndis_hk] new_OpenAdapterCompleteHandler: SendPacketsHandler: old 0x%x new 0x%x\n",
					adapter->old_SendPacketsHandler, adapter->asm_SendPacketsHandler));
			}
			
			// and NdisTransferData too
			adapter->old_TransferDataHandler = nob->TransferDataHandler;
			GENERATE_ASM_STUB(adapter, TransferDataHandler);
			nob->TransferDataHandler = (TRANSFER_DATA_HANDLER)adapter->asm_TransferDataHandler;
		}
		
	} __finally {

		if (Status != NDIS_STATUS_PENDING) {
			// call original handler anyway
			PCHARS_OLD_CHARS(pchars)->OpenAdapterCompleteHandler(ProtocolBindingContext,
				Status, OpenErrorStatus);
		}
	
	}
}

/**
 * Hooked ReceiveHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * Function is called when NDIS miniport adapter indicated incoming data using old scheme.
 * If we get LookaheadBuffer smaller than PacketSize (for old PIO based network cards) we
 * call original NdisTransferData manually to get the whole packet.
 * We call original ReceiveHandler with our buffer as MacReceiveContext. If protocol driver want to
 * call hooked NdisTransferData we extract data for him from this buffer.
 */
NDIS_STATUS
new_ReceiveHandler(
	struct PROTOCOL_CHARS *pchars,				/* added by ASM stub */
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  MacReceiveContext,
    IN PVOID  HeaderBuffer,
    IN UINT  HeaderBufferSize,
    IN PVOID  LookaheadBuffer,
    IN UINT  LookaheadBufferSize,
    IN UINT  PacketSize)
{
	struct ADAPTER_PROTOCOL *adapter;
	BOOLEAN result = FALSE;
	NDIS_STATUS status;
	PNDIS_PACKET packet = NULL;
	PNDIS_BUFFER hdr_buffer = NULL, data_buffer = NULL;
	void *buf = NULL;
	ULONG bytes;

	_CHECK_PCHARS(pchars);
	
	__try {
		
		// get adapter
		for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next) {
			if (adapter->ProtocolBindingContext == ProtocolBindingContext)
				break;
		}
			
		if (adapter == NULL) {
			KdPrint(("[ndis_hk] new_ReceiveHandler: adapter not found!\n"));
			__leave;
		}
		
		// can't use UNICODE %S to output (see DbgPrint documentation)
		KdPrint(("[ndis_hk] new_ReceiveHandler: (%d) hdr %u; look %u; pkt %u\n",
			adapter->adapter_index,
			HeaderBufferSize, LookaheadBufferSize, PacketSize));
		
		if (LookaheadBufferSize == PacketSize) {
			// already got the whole frame!
			
			// prepare packet for filtering
			
			NdisAllocateBuffer(&status, &hdr_buffer, g_buffer_pool, HeaderBuffer, HeaderBufferSize);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocateBuffer: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisAllocateBuffer(&status, &data_buffer, g_buffer_pool, LookaheadBuffer, LookaheadBufferSize);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocateBuffer: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisAllocatePacket(&status, &packet, g_packet_pool);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocatePacket: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisChainBufferAtFront(packet, data_buffer);
			NdisChainBufferAtFront(packet, hdr_buffer);
			
			// filter it!
			
			result = filter_packet(DIRECTION_IN, adapter->adapter_index, packet);
			
		} else {
			// get the whole frame! (use NdisTransferData)
			
			// NOTE: NdisTransferData CAN return NDIS_STATUS_PENDING but pointers are only valid
			// in context of current call. That's because we're allocating memory (slow way).
			buf = malloc_np(HeaderBufferSize + PacketSize);
			if (buf == NULL) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: malloc_np!\n"));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			// copy header
			memcpy(buf, HeaderBuffer, HeaderBufferSize);
			
			// make buffer
			
			NdisAllocateBuffer(&status, &hdr_buffer, g_buffer_pool, buf, HeaderBufferSize);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocateBuffer: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisAllocateBuffer(&status, &data_buffer, g_buffer_pool, (char *)buf + HeaderBufferSize, PacketSize);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocateBuffer: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisAllocatePacket(&status, &packet, g_packet_pool);
			if (status != NDIS_STATUS_SUCCESS) {
				KdPrint(("[ndis_hk] new_ReceiveHandler: NdisAllocatePacket: 0x%x!\n", status));
				
				status = NDIS_STATUS_NOT_ACCEPTED;
				__leave;
			}
			
			NdisChainBufferAtFront(packet, data_buffer);
			
			PROTOCOL_RESERVED(packet)->buffer = hdr_buffer;		// save hdr_buffer for completion
			
			// call _original_ NdisTransferData (see ndis.h for definition of NdisTransferData)
			
			status = adapter->old_TransferDataHandler(
				((PNDIS_OPEN_BLOCK)(adapter->NdisBindingHandle))->MacBindingHandle,
				MacReceiveContext, 0, PacketSize, packet, &bytes);
			
			if (status == NDIS_STATUS_SUCCESS) {
				// chain header at the first of packet
				NdisChainBufferAtFront(packet, hdr_buffer);
				
				// got the whole packet for filter!
				result = filter_packet(DIRECTION_IN, adapter->adapter_index, packet);
				
			} else if (status == NDIS_STATUS_PENDING) {
				result = FALSE;	// wait for NdisTransferDataComplete to be called
				status = NDIS_STATUS_SUCCESS;
				
				// don't free packet and buffers
				buf = NULL;
				hdr_buffer = NULL;
				data_buffer = NULL;
				packet = NULL;
				
			} else
				result = FALSE;	// drop the packet
			
		}
		
	} __except((status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)) {

		KdPrint(("[ndis_hk] new_ReceiveHandler: exception 0x%x!\n", status));

		status = NDIS_STATUS_NOT_ACCEPTED;
		result = FALSE;
	}
	
	if (result) {
		// call original handler (specify fake MacReceiveContext = buf see new_TransferDataHandler)
		
		if (buf == NULL) {
			
			// LookaheadBufferSize == PacketSize
			status = PCHARS_OLD_CHARS(pchars)->ReceiveHandler(ProtocolBindingContext,
				(NDIS_HANDLE)LookaheadBuffer, HeaderBuffer, HeaderBufferSize,
				LookaheadBuffer, LookaheadBufferSize, LookaheadBufferSize);

		} else {

			// LookaheadBufferSize < PacketSize
			status = PCHARS_OLD_CHARS(pchars)->ReceiveHandler(ProtocolBindingContext,
				(NDIS_HANDLE)((char *)buf + HeaderBufferSize), HeaderBuffer, HeaderBufferSize,
				(char *)buf + HeaderBufferSize, PacketSize, PacketSize);

		}
	
	}
	
	// cleanup
	if (buf != NULL)
		free(buf);
	if (hdr_buffer != NULL)
		NdisFreeBuffer(hdr_buffer);
	if (data_buffer != NULL)
		NdisFreeBuffer(data_buffer);
	if (packet != NULL)
		NdisFreePacket(packet);

	return status;
}

/**
 * Hooked TransferDataCompleteHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * This function is called _only_ when we manually call original NdisTransferData and this
 * function returns NDIS_STATUS_PENDING. We got the whole packet and call original
 * ReceiveHandler.
 * We call original ReceiveHandler with our buffer as MacReceiveContext. If protocol driver want to
 * call hooked NdisTransferData we extract data for him from this buffer.
 */
VOID
new_TransferDataCompleteHandler(
	struct PROTOCOL_CHARS *pchars,				/* added by ASM stub */
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet,
    IN NDIS_STATUS  Status,
    IN UINT  BytesTransferred)
{
	struct ADAPTER_PROTOCOL *adapter;
	PNDIS_BUFFER hdr_buffer, data_buffer;
	void *buf;
	ULONG data_size, hdr_size;

	_CHECK_PCHARS(pchars);

	// XXX Somebody, please test the code below. I never seen it's called
	_TEST_ME_

	// get adapter
	for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
		if (adapter->ProtocolBindingContext == ProtocolBindingContext)
			break;

	// this function can be called only for our packet! (is it true?)
	NdisQueryPacket(Packet, NULL, NULL, &data_buffer, NULL);
	
	hdr_buffer = PROTOCOL_RESERVED(Packet)->buffer;

	// chain header buffer at the begin of packet
	NdisChainBufferAtFront(Packet, hdr_buffer);

	// A HACK! actually NDIS_BUFFER is MDL!
	buf = MmGetSystemAddressForMdl(hdr_buffer);

	NdisQueryBuffer(hdr_buffer, NULL, &hdr_size);
	NdisQueryBuffer(data_buffer, NULL, &data_size);

	if (adapter != NULL) {

		// can't use UNICODE %S to output (see DbgPrint documentation)
		KdPrint(("[ndis_hk] new_TransferDataCompleteHandler: (%d) pkt %u; bytes %u\n",
			adapter->adapter_index,
			hdr_size + data_size, BytesTransferred));

		// filter packet
		if (filter_packet(DIRECTION_IN, adapter->adapter_index, Packet)) {

			// call original receive handler with packet (don't care about status)
			PCHARS_OLD_CHARS(pchars)->ReceiveHandler(ProtocolBindingContext,
				(NDIS_HANDLE)((char *)buf + hdr_size), buf, hdr_size,
				(char *)buf + hdr_size, data_size, data_size);

		}
	}

	// cleanup
	free(buf);
	NdisFreeBuffer(hdr_buffer);
	NdisFreeBuffer(data_buffer);
	NdisFreePacket(Packet);
}

/**
 * Hooked ReceivePacketHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * NDIS miniport driver indicates packet using new-style packet transfer.
 * We got the whole packet! We can filter it and call original handler if we want.
 */
INT
new_ReceivePacketHandler(
	struct PROTOCOL_CHARS		*pchars,				/* added by ASM stub */
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet)
{
	struct ADAPTER_PROTOCOL *adapter;

	_CHECK_PCHARS(pchars);

	// get adapter
	for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
		if (adapter->ProtocolBindingContext == ProtocolBindingContext)
			break;

	if (adapter != NULL) {

		// can't use UNICODE %S to output (see DbgPrint documentation)
		KdPrint(("[ndis_hk] new_ReceivePacketHandler: (%d)\n",
			adapter->adapter_index));
		
		if (adapter->adapter_index != 0 &&
			!filter_packet(DIRECTION_IN, adapter->adapter_index, Packet)) {
			KdPrint(("[ndis_hk] new_ReceivePacketHandler: DROP!\n"));
			return 0;
		}
		
	} else
		KdPrint(("[ndis_hk] new_ReceivePacketHandler: adapter not found!\n"));

	// call original handler
	return PCHARS_OLD_CHARS(pchars)->ReceivePacketHandler(ProtocolBindingContext, Packet);
}

/**
 * Hooked SendCompleteHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * Check if it is completion for our call of NdisSend free our buffer and don't call original handler.
 */
VOID
new_SendCompleteHandler(
	struct PROTOCOL_CHARS *pchars,				/* added by ASM stub */
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet,
    IN NDIS_STATUS Status)
{
	struct protocol_reserved *pr = PROTOCOL_RESERVED(Packet);

	_CHECK_PCHARS(pchars);

	/** @todo maybe using magic as ProtocolReserved is not stable solution... */
	if (pr->magic == send_out_packet) {
		// our completion! free buffer & packet

		KdPrint(("[ndis_hk] new_SendCompleteHandler(our packet): Status 0x%x\n", Status));

		free(pr->data);
		NdisFreeBuffer(pr->buffer);
		NdisFreePacket(Packet);

	} else {
		// call original handler
		if (PCHARS_OLD_CHARS(pchars)->SendCompleteHandler != NULL)
			PCHARS_OLD_CHARS(pchars)->SendCompleteHandler(ProtocolBindingContext, Packet, Status);
	}
}

/**
 * Hooked RequestCompleteHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * Check if it is completion for our call of NdisRequest set event and don't call original handler.
 */
VOID
new_RequestCompleteHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status)
{
	if (NdisRequest == g_request.pend_req) {
		// it's our request

		KdPrint(("[ndis_hk] new_RequestCompleteHandler(our request): Status 0x%x\n", Status));
		
		g_request.status = Status;
		NdisSetEvent(&g_request.event);
	
	} else {
		// call original handler
		if (PCHARS_OLD_CHARS(pchars)->RequestCompleteHandler != NULL)
			PCHARS_OLD_CHARS(pchars)->RequestCompleteHandler(ProtocolBindingContext, NdisRequest, Status);
	}
}

/**
 * Hooked PnPEventHandler from NDIS_PROTOCOL_CHARACTERISTICS.
 * Call pnp_event() in filter chain
 */
NDIS_STATUS
new_PnPEventHandler(
	struct PROTOCOL_CHARS		*pchars,		/* added by ASM stub */
    IN NDIS_HANDLE				ProtocolBindingContext,
    IN PNET_PNP_EVENT			NetPnPEvent)
{
	struct ADAPTER_PROTOCOL *adapter;
	NDIS_STATUS status;

	_CHECK_PCHARS(pchars);

	// get adapter
	for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
		if (adapter->ProtocolBindingContext == ProtocolBindingContext)
			break;

	if (adapter != NULL)
		status = call_pnp_events(adapter->adapter_index, NetPnPEvent);
	else
		status = NDIS_STATUS_SUCCESS;

	if (status == NDIS_STATUS_SUCCESS) {
		// call original handler
		status = PCHARS_OLD_CHARS(pchars)->PnPEventHandler(ProtocolBindingContext, NetPnPEvent);
	}
	
	return status;
}

/*
 * --- NDIS functions from NDIS_OPEN_BLOCK ---
 */

/**
 * Hooked SendHandler (NdisSend) from NDIS_OPEN_BLOCK.
 * Filter packet and maybe call original handler
 */
NDIS_STATUS
new_SendHandler(
	struct ADAPTER_PROTOCOL		*adapter,			/* added by ASM stub */
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	PNDIS_PACKET			Packet)
{
	// we get pchars only to check adapter
	_CHECK_PCHARS(adapter->pchars);

	// can't use UNICODE %S to output (see DbgPrint documentation)
	KdPrint(("[ndis_hk] new_SendHandler: (%d)\n", adapter->adapter_index));

	if (adapter->adapter_index != 0 &&
		!filter_packet(DIRECTION_OUT, adapter->adapter_index, Packet)) {
		KdPrint(("[ndis_hk] new_SendHandler: DENY!\n"));
		return NDIS_STATUS_SUCCESS;		// let protocol driver to think it's all right
	}

	return adapter->old_SendHandler(MacBindingHandle, Packet);	
}

/**
 * Hooked SendPacketsHandler (NdisSendPackets) from NDIS_OPEN_BLOCK.
 * Filter packets and call or not original handler or call it with modified array of packets
 */
VOID
new_SendPacketsHandler(
	struct ADAPTER_PROTOCOL		*adapter,			/* added by ASM stub */
	IN NDIS_HANDLE				NdisBindingHandle,
	IN PPNDIS_PACKET			PacketArray,
	IN UINT						NumberOfPackets)
{
	struct PROTOCOL_CHARS *pchars = adapter->pchars;

	// we get pchars only to check adapter
	_CHECK_PCHARS(pchars);

	// can't use UNICODE %S to output (see DbgPrint documentation)
	KdPrint(("[ndis_hk] new_SendPacketsHandler: (%d)\n", adapter->adapter_index));

	if (adapter->adapter_index != 0) {
		UINT i, n_allow, j;

		// first get number of allow packets (and set status for deny packets)

		n_allow = NumberOfPackets;

		for (i = 0; i < NumberOfPackets; i++) {
			if (!filter_packet(DIRECTION_OUT, adapter->adapter_index, PacketArray[i])) {
				n_allow--;
				NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_FAILURE);
			} else
				NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_SUCCESS);
		}

		if (n_allow < NumberOfPackets) {
			PPNDIS_PACKET new_PacketArray;

			// deny some packets

			KdPrint(("new_SendPacketsHandler: DENY!\n"));

			if (n_allow == 0) {

				// deny all packets
				for (i = 0; i < NumberOfPackets; i++) {
					// call ProtocolSendComplete handler for each packet
					PCHARS_OLD_CHARS(pchars)->SendCompleteHandler(adapter->ProtocolBindingContext,
						PacketArray[i], NDIS_STATUS_SUCCESS);	// let protocol driver to think it's all right
				}

				return;
			}

			/** @test Somebody, please test the code below. I never seen it's called */
			_TEST_ME_

			// allocate new PacketArray

			new_PacketArray = (PPNDIS_PACKET)malloc_np(sizeof(PPNDIS_PACKET) * n_allow);
			if (new_PacketArray == NULL) {

				// have not resources
				KdPrint(("new_SendPacketsHandler: malloc_np!\n"));

				for (i = 0; i < NumberOfPackets; i++) {
					// call ProtocolSendComplete handler for each packet
					PCHARS_OLD_CHARS(pchars)->SendCompleteHandler(adapter->ProtocolBindingContext,
						PacketArray[i], NDIS_STATUS_RESOURCES);
				}

				return;
			}

			// fill new PacketArray

			j = 0;
			for (i = 0; i < NumberOfPackets; i++) 
				if (NDIS_GET_PACKET_STATUS(PacketArray[i]) == NDIS_STATUS_SUCCESS) {

					// save allowed packet in new PacketArray

					// sanity check
					if (j >= n_allow)
						break;

					new_PacketArray[j++] = PacketArray[i];
				
				} else {
					// call ProtocolSendComplete handler to deny packet
					PCHARS_OLD_CHARS(pchars)->SendCompleteHandler(adapter->ProtocolBindingContext,
						PacketArray[i], NDIS_STATUS_SUCCESS);	// let protocol driver to think it's all right
				}

			// call original handler with new_PacketArray

			adapter->old_SendPacketsHandler(NdisBindingHandle, new_PacketArray, NumberOfPackets);

			// DDK: As soon as a protocol calls NdisSendPackets, it relinquishes ownership of the following...

			// So, don't care about packets in new_PacketArray

			// finally free new PacketArray and return
			free(new_PacketArray);
			return;
		}
	}

	// call original handler
	adapter->old_SendPacketsHandler(NdisBindingHandle, PacketArray, NumberOfPackets);	
}

/**
 * Hooked TransferDataHandler (NdisTransferData) from NDIS_OPEN_BLOCK.
 * MacReceiveContext is our buffer so just copy data from our buffer to packet
 */
NDIS_STATUS
new_TransferDataHandler(
	struct ADAPTER_PROTOCOL		*adapter,			/* added by ASM stub */
	IN	NDIS_HANDLE				MacBindingHandle,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer,
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred)
{
	PNDIS_BUFFER buffer;

	// we get pchars only to check adapter
	_CHECK_PCHARS(adapter->pchars);

	/** @test Somebody, please test the code below. I never seen it's called */
	_TEST_ME_

	// can't use UNICODE %S to output (see DbgPrint documentation)
	KdPrint(("[ndis_hk] new_TransferDataHandler: (%d)\n", adapter->adapter_index));

	// this function can be called _only_ inside new_ReceiveHandler -> old_ReceiveHandler (is it true?)
	// so MacReceiveContext is a buffer to the whole packet data: copy them

	NdisQueryPacket(Packet, NULL, NULL, &buffer, NULL);
	
	// A HACK! actually NDIS_BUFFER is MDL so use TdiCopyBufferToMdl (i don't know NDIS equialent)
	TdiCopyBufferToMdl((PVOID)MacReceiveContext, 0, BytesToTransfer, (PMDL)buffer, ByteOffset, BytesTransferred);

	return NDIS_STATUS_SUCCESS;

}

NTSTATUS
send_out_packet(int iface, PNDIS_PACKET packet)
{
	NTSTATUS status;
	int i;
	struct PROTOCOL_CHARS *pchars;
	struct ADAPTER_PROTOCOL *adapter;
	PNDIS_BUFFER buffer, new_buffer = NULL;
	PNDIS_PACKET new_packet = NULL;
	struct protocol_reserved *pr;
	char *data = NULL;
	ULONG size;

	for (i = 0; i < TCPIP_PCHARS_N; i++) {
		pchars = g_tcpip_pchars[i];
		if (pchars == NULL)
			continue;

		// get adapter
		for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
			if (adapter->adapter_index == iface)
				break;

		if (adapter != NULL)
			break;
	}

	// sanity check
	if (pchars == NULL) {
		KdPrint(("[ndis_hk] send_out_packet: no TCPIP protocol!\n"));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	if (adapter == NULL) {
		KdPrint(("[ndis_hk] send_out_packet: no adapter %d!\n", iface));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	NdisQueryPacket(packet, NULL, NULL, &buffer, &size);

	data = (char *)malloc_np(size);
	if (data == NULL) {
		KdPrint(("[ndis_hk] send_out_packet: malloc_np!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	__try {
		
		// A HACK! actually NDIS_BUFFER is MDL so use TdiCopyBufferToMdl (i don't know NDIS equialent)
		TdiCopyMdlToBuffer((PMDL)buffer, 0, data, 0, size, &size);
		
		// now create our own new_packet
		
		NdisAllocateBuffer(&status, &new_buffer, g_buffer_pool, data, size);
		if (status != NDIS_STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] send_out_packet: NdisAllocateBuffer: 0x%x\n", status));
			__leave;
		}
		
		NdisAllocatePacket(&status, &new_packet, g_packet_pool);
		if (status != NDIS_STATUS_SUCCESS) {
			KdPrint(("[ndis_hk] send_out_packet: NdisAllocatePacket: 0x%x\n", status));
			__leave;
		}
		
		NdisChainBufferAtFront(new_packet, new_buffer);
		
		// setup ProtocolReserved for completion
		
		pr = PROTOCOL_RESERVED(new_packet);
		
		pr->magic = send_out_packet;
		pr->buffer = new_buffer;
		pr->data = data;
		
		// send it! (call original handler see NdisSend() macro in ndis.h)
		
		KdPrint(("[ndis_hk] send_out_packet: sending new_packet...\n"));
		
		status = adapter->old_SendHandler(
			((PNDIS_OPEN_BLOCK)(adapter->NdisBindingHandle))->MacBindingHandle, new_packet);
		
		if (status == NDIS_STATUS_PENDING) {
			// don't free resources (will be freed in completion)
			data = NULL;
			new_buffer = NULL;
			new_packet = NULL;
		}
		
		KdPrint(("[ndis_hk] send_out_packet: status: 0x%x\n", status));
	
	} __except((status = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)) {
		KdPrint(("[ndis_hk] send_out_packet: exception 0x%x!\n", status));
	}
		
	if (data != NULL)
		free(data);
	if (new_buffer != NULL)
		NdisFreeBuffer(new_buffer);
	if (new_packet != NULL)
		NdisFreePacket(new_packet);
	
	return status;
}

NTSTATUS
send_in_packet(int iface, ULONG hdr_size, ULONG data_size, char *packet_data)
{
	NTSTATUS status;
	int i;
	struct PROTOCOL_CHARS *pchars;
	struct ADAPTER_PROTOCOL *adapter;
	KIRQL irql;

	for (i = 0; i < TCPIP_PCHARS_N; i++) {
		pchars = g_tcpip_pchars[i];
		if (pchars == NULL)
			continue;

		// get adapter
		for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
			if (adapter->adapter_index == iface)
				break;

		if (adapter != NULL)
			break;
	}

	// sanity check
	if (pchars == NULL) {
		KdPrint(("[ndis_hk] send_in_packet: no TCPIP protocol!\n"));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	if (adapter == NULL) {
		KdPrint(("[ndis_hk] send_in_packet: no adapter %d!\n", iface));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}
	
	/* send packet to protocol driver */
	
	// sanity check
	if (PCHARS_OLD_CHARS(pchars)->ReceiveHandler == NULL) {
		KdPrint(("[ndis_hk] send_in_packet: no ProtocolReceive!\n"));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	/** @todo Need to simulate NdisGetReceivedPacket for stability? */

	// raise IRQL to DISPATCH_LEVEL for ReceiveHandler
	KeRaiseIrql(DISPATCH_LEVEL, &irql);

    // simulate ProtocolReceive is called (MacReceiveContext is our buffer)
	status = PCHARS_OLD_CHARS(pchars)->ReceiveHandler(adapter->ProtocolBindingContext,
		(NDIS_HANDLE)(packet_data + hdr_size), packet_data, hdr_size,
		packet_data + hdr_size, data_size, data_size);

	KeLowerIrql(irql);
	return status;
}

NDIS_STATUS
ndis_request(int iface, NDIS_REQUEST *req)
{
	int i;
	struct PROTOCOL_CHARS *pchars;
	struct ADAPTER_PROTOCOL *adapter;
	NDIS_STATUS status;

	for (i = 0; i < TCPIP_PCHARS_N; i++) {
		pchars = g_tcpip_pchars[i];
		if (pchars == NULL)
			continue;

		// get adapter
		for (adapter = pchars->adapter; adapter != NULL; adapter = adapter->next)
			if (adapter->adapter_index == iface)
				break;

		if (adapter != NULL)
			break;
	}

	// sanity check
	if (pchars == NULL) {
		KdPrint(("[ndis_hk] ndis_request: no TCPIP protocol!\n"));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	if (adapter == NULL) {
		KdPrint(("[ndis_hk] ndis_request: no adapter %d!\n", iface));
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}
	
	// to simplify processing serialize requests
	KeWaitForSingleObject(&g_request.guard, Executive, KernelMode, FALSE, NULL);

	g_request.pend_req = req;

	NdisResetEvent(&g_request.event);
	NdisRequest(&status, adapter->NdisBindingHandle, req);

	if (status == NDIS_STATUS_PENDING) {
		NdisWaitEvent(&g_request.event, 0);
		status = g_request.status;
	}

	g_request.pend_req = NULL;

	KeReleaseMutex(&g_request.guard, FALSE);
	return status;
}

/*@}*/