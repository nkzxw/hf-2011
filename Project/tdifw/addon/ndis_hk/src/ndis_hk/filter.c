// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: filter.c,v 1.2 2003/05/19 14:20:43 dev Exp $

/** @addtogroup hook_driver
 *@{
 */

/**
 * @file ndis_hk\\filter.c
 * Implementation of functions to work with stack of packet filter drivers
 *
 * Stack of packet filter drivers:\n
 \verbatim
  +--------------------+  
  |  adapter (top)     |  
  +-----------------^--+  
    +-in            | out 
  +-V---------------|--+  
  | |  filter.1     ^  |  
  +-|---------------|--+  
  | V  filter.2     ^  |  
  +-|---------------|--+  
  | V  filter.3     |  |  
  +-|---------------^--+  
    | in            +-out 
  +-V------------------+  
  |   tcpip (bottom)   |  
  +--------------------+  
 \endverbatim
 */

#include <ntddk.h>
#include <tdikrnl.h>			// for TdiCopyMdlToBuffer

#include "except.h"
#include "memtrack.h"
#include "ndis_hk.h"
#include "filter.h"
#include "ndis_hk_ioctl.h"

/** top in filter stack is adapter */
static struct filter_nfo top;
/** bottom in filter stack is tcpip */
static struct filter_nfo bottom;
/** guard spinlock for top & bottom double linked list */
static KSPIN_LOCK guard;

static BOOLEAN	queue_for_net(
	int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self, BOOLEAN packet_unchanged);
static BOOLEAN	queue_for_tcp(
	int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self, BOOLEAN packet_unchanged);

/** param for delayed send input packet */
struct send_in_packet_param {
	ULONG			size;		/**< size of data */
	int				iface;		/**< index of interface */
	WORK_QUEUE_ITEM	item;		/**< we use work items to queue it */
	char			data[0];	/**< packet data */
};

VOID	send_in_packet_delayed(PVOID p);

void
init_filter(void)
{
	KeInitializeSpinLock(&guard);
	
	top.process_packet = queue_for_net;
	top.lower = &bottom;

	bottom.process_packet = queue_for_tcp;
	bottom.upper = &top;
}

void
attach_filter(struct filter_nfo *flt, BOOLEAN add, BOOLEAN to_top)
{
	KIRQL irql;

	KeAcquireSpinLock(&guard, &irql);

	__try {
		if (add) {
			if (to_top) {
				top.lower->upper = flt;
				flt->lower = top.lower;
				
				top.lower = flt;
				flt->upper = &top;
			
			} else {
				bottom.upper->lower = flt;
				flt->upper = bottom.upper;

				bottom.upper = flt;
				flt->lower = &bottom;
			}
		} else {
			struct filter_nfo *f;
			for (f = top.lower; f != &bottom; f = f->lower)		// from top to bottom
				if (f == flt) {
					f->upper->lower = f->lower;
					f->lower->upper = f->upper;
					break;
				}
		}

	} __finally {
		KeReleaseSpinLock(&guard, irql);
	}
}

BOOLEAN
filter_packet(int direction, int iface, PNDIS_PACKET packet)
{
	BOOLEAN result;
	KIRQL irql;
	
	KeAcquireSpinLock(&guard, &irql);		// using r/w locks give better performance?
	
	__try {

		if (direction == DIRECTION_IN)		// from top (adapter) to bottom (tcpip)
			result = top.lower->process_packet(direction, iface, packet, top.lower, TRUE);
		else								// from bottom (tcpip) to top (adapter)
			result = bottom.upper->process_packet(direction, iface, packet, bottom.upper, TRUE);

	} __finally {
		KeReleaseSpinLock(&guard, irql);
	}
	
	return result;
}

/**
 * On top of filter stack we can queue packet for sending to net if packet was changed.
 * Can be called at IRQL <= DISPACTH_LEVEL
 * @param	direction	DIRECTION_OUT only!
 * @param	iface		index of interface
 * @param	packet		NDIS packet to send (can be freed after function call)
 * @param	self		structure with packet filter information
 * @param	packet_unchanged	if TRUE function doensn't queue packet
 * @return				function returns packet_unchanged in any case (doesn't change packet)
 */
BOOLEAN
queue_for_net(int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self,
			  BOOLEAN packet_unchanged)
{
	char *packet_data;
	ULONG hdr_size, data_size, packet_size;
	PNDIS_BUFFER buffer;

	// sanity check (self == &top)
	if (self != &top) {
		KdPrint(("[ndis_hk] queue_for_net: self != &top!\n"));
		return packet_unchanged;
	}

	// sanity check (queue_for_net for DIRECTION_OUT only!)
	if (direction != DIRECTION_OUT)
		return packet_unchanged;

	if (packet_unchanged)
		return TRUE;			// no need to queue packet. it's unchanged!

    /*
	 * do async send
	 * to simplify process function they do sync only processing
	 * copy packet
	 * send it and free resources in completion
	 */

	send_out_packet(iface, packet);

	return FALSE;		// packet_unchanged == FALSE
}

/**
 * On bottom of filter stack we can queue packet for sending to TCP/IP protocol if packet was changed.
 * Can be called at IRQL <= DISPACTH_LEVEL
 * @param	direction	DIRECTION_IN only!
 * @param	iface		index of interface
 * @param	packet		NDIS packet to send (can be freed after function call)
 * @param	self		structure with packet filter information
 * @param	packet_unchanged	if TRUE function doensn't queue packet
 * @return				function returns packet_unchanged in any case (doesn't change packet)
 */
BOOLEAN
queue_for_tcp(int direction, int iface, PNDIS_PACKET packet, struct filter_nfo *self,
			  BOOLEAN packet_unchanged)
{
	PNDIS_BUFFER buffer;
	ULONG size, hdr_size;
	struct send_in_packet_param *param;

	// sanity check (self == &bottom)
	if (self != &bottom) {
		KdPrint(("[ndis_hk] queue_for_net: self != &bottom!\n"));
		return packet_unchanged;
	}
	
	// sanity check (queue_for_net for DIRECTION_IN only!)
	if (direction != DIRECTION_IN)
		return packet_unchanged;

	if (packet_unchanged)
		return TRUE;			// no need to queue packet. it's unchanged!

	/*
	 * copy packet and queue it to call simulated receive in another thread
	 */

	NdisQueryPacket(packet, NULL, NULL, &buffer, &size);

	param = (struct send_in_packet_param *)malloc_np(sizeof(*param) + size);
	if (param == NULL) {
		KdPrint(("[ndis_hk] queue_for_tcp: malloc_np!\n"));
		return packet_unchanged;
	}

	memset(param, 0, sizeof(*param));
	
	// A HACK! actually NDIS_BUFFER is MDL so use TdiCopyBufferToMdl (i don't know NDIS equialent)
	TdiCopyMdlToBuffer((PMDL)buffer, 0, param->data, 0, size, &size);

	param->size = size;
	param->iface = iface;

	/** @todo is it a good idea to use system worker thread here? */
	ExInitializeWorkItem(&param->item, send_in_packet_delayed, param);
	ExQueueWorkItem(&param->item, DelayedWorkQueue);	// DelayedWorkQueue a good value?

	return FALSE;		// packet_unchanged == FALSE
}

/**
 * We use work items to delay sending of packets for input.
 * This function is called using work items at PASSIVE_LEVEL
 * @param	p	actually points to struct send_in_packet_param
 */
VOID
send_in_packet_delayed(PVOID p)
{
	struct send_in_packet_param *param = (struct send_in_packet_param *)p;
	ULONG hdr_size = 14;		// ??? ethernet only ???

	send_in_packet(param->iface, hdr_size, param->size - hdr_size, param->data);

    free(param);
}

NDIS_STATUS
call_pnp_events(int iface, PNET_PNP_EVENT NetPnPEvent)
{
	struct filter_nfo *f;
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	for (f = &top; f != NULL; f = f->lower)
		if (f->pnp_event != NULL) {
			status = f->pnp_event(iface, NetPnPEvent);
			if (status != NDIS_STATUS_SUCCESS)
				break;
		}

	return status;
}

/*@}*/