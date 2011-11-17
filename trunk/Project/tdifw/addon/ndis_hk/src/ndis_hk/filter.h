// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: filter.h,v 1.1 2003/05/13 12:48:34 dev Exp $

/**
 * @file filter.h
 * Set of functions to work with stack of packet filter drivers
 */

#ifndef _filter_h_
#define _filter_h_

/**
 * Initialize stack of packet filter drivers
 */
void	init_filter(void);

/**
 * Attach or remove packet filter into or from stack
 * @param	flt		information about packet filter
 * @param	add		TRUE - attach filter; FALSE - remove filter
 * @param	to_top	TRUE - for attaching to top of stack (useless with add == FALSE)
 */
void	attach_filter(struct filter_nfo *flt, BOOLEAN add, BOOLEAN to_top);

/**
 * Function is called for each packet.
 * Works with packet filters in filter stack
 * @param	direction	DIRECTION_IN (incoming) or DIRECTION_OUT (outgoing) packet
 * @param	iface		index of interface (see adapters.h)
 * @param	packet		packet to filter
 * @retval	TRUE		pass this packet to protocol driver (incoming) or to network (outgoing)
 * @retval	FALSE		don't pass this packet to protocol driver or to network
 */
BOOLEAN	filter_packet(int direction, int iface, PNDIS_PACKET packet);

/**
 * Function to send PnP event through filter chain from top to bottom
 * @param	iface		index if interface
 * @param	NetPnPEvent	see DDK documentation for info
 * @return				status of execution (if not NDIS_STATUS_SUCCESS don't call original handler)
 */
NDIS_STATUS call_pnp_events(int iface, PNET_PNP_EVENT NetPnPEvent);

#endif
