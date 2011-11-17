// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: ndis_hk.h,v 1.5 2003/07/07 11:41:56 dev Exp $

/**
 * @file ndis_hk.h
 * NDIS hooking engine: prototypes of hooked functions
 */

#ifndef _ndis_hk_h_
#define _ndis_hk_h_

#if _WIN32_WINNT >= 0x0500
#	define NDIS50				1
#else
#	define NDIS40				1
#endif
#define BINARY_COMPATIBLE		0
#include <ndis.h>

#include "ndis_hk_ioctl.h"

/*
 * hooked functions
 */

/** typedef of NdisRegisterProtocol */
typedef VOID
NdisRegisterProtocol_t(
	OUT	PNDIS_STATUS			Status,
	OUT	PNDIS_HANDLE			NdisProtocolHandle,
	IN	PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
	IN	UINT					CharacteristicsLength
	);

/** typedef of NdisDeregisterProtocol */
typedef VOID
NdisDeregisterProtocol_t(
    OUT PNDIS_STATUS			Status,
    IN NDIS_HANDLE				NdisProtocolHandle
    );

/** typedef of NdisOpenAdapter */
typedef VOID
NdisOpenAdapter_t(
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
	IN	PSTRING					AddressingInformation OPTIONAL
	);

/** typedef of NdisCloseAdapter */
typedef VOID
NdisCloseAdapter_t(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				NdisBindingHandle
	);

/* indexes for functions in g_hook_fn array */

enum {
	NdisRegisterProtocol_n = 0,		/**< index of NdisRegisterProtocol */
	NdisDeregisterProtocol_n,		/**< index of NdisDeregisterProtocol */
	NdisOpenAdapter_n,				/**< index of NdisOpenAdapter */
	NdisCloseAdapter_n,				/**< index of NdisCloseAdapter */
	
	MAX_HOOK_FN						/**< sizeof of g_hook_fn array */
};

/** entry for array of hooked functions g_hook_fn */
struct hook_fn {
	char	*name;					/**< name of function */
	void	*old_fn;				/**< original address of function */
	void	*new_fn;				/**< address of new function */
};

/** array of hooked functions */
extern struct hook_fn g_hook_fn[MAX_HOOK_FN];

/** macro to simplify usage of original hooked functions */
#define HOOKED_OLD_FN(name) \
	((name##_t *)(g_hook_fn[name##_n].old_fn))

/* new functions */

NdisRegisterProtocol_t		new_NdisRegisterProtocol;		/**< new NdisRegisterProtocol */
NdisDeregisterProtocol_t	new_NdisDeregisterProtocol;		/**< new NdisDeregisterProtocol */
NdisOpenAdapter_t			new_NdisOpenAdapter;			/**< new NdisOpenAdapter */
NdisCloseAdapter_t			new_NdisCloseAdapter;			/**< new NdisCloseAdapter */

extern NDIS_HANDLE g_packet_pool;	/**< NDIS packet pool */
extern NDIS_HANDLE g_buffer_pool;	/**< NDIS buffer pool */

/** struct to store in NDIS packet ProtocolReserved field */
struct protocol_reserved {
	void			*magic;		/**< magic value to indenify this struct */
	PNDIS_BUFFER	buffer;		/**< NDIS buffer with data */
	char			*data;		/**< pointer to data */
};

/** macro to simplify to get struct protocol_reserved from NDIS packet */
#define PROTOCOL_RESERVED(packet)		((struct protocol_reserved *)((packet)->ProtocolReserved))

/**
 * Send packet to network (out).
 * Function can be called at IQL <= DISPATCH_LEVEL.
 * You can safely free packet after calling this function.
 * @param	iface			number of interface (see adapters.h)
 * @param	packet			NDIS packet
 * @retval	STATUS_SUCCESS	no error
 */
NTSTATUS	send_out_packet(int iface, PNDIS_PACKET packet);

/**
 * Send packet to protocol driver (in)
 * Function can be called at IQL <= DISPATCH_LEVEL.
 * You can safely free packet_data after calling this function.
 * @param	iface			number of interface (see adapters.h)
 * @param	hdr_size		size of frame (ethernet) header
 * @param	data_size		size of frame data
 * @param	packet_data		the whole frame to send (size = hdr_size + data_size)
 * @retval	STATUS_SUCCESS	no error
 */
NTSTATUS	send_in_packet(int iface, ULONG hdr_size, ULONG data_size, char *packet_data);

/**
 * NDIS request on hooked adapter
 * @param	iface	interface index
 * @param	req		request (see DDK documentation)
 * @return			status
 */
NDIS_STATUS	ndis_request(int iface, NDIS_REQUEST *req);

#endif
