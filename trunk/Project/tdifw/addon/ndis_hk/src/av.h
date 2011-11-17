// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: av.h,v 1.3 2003/05/13 12:47:39 dev Exp $

/**
 * @file av.h
 * Set of functions to work with list of argument-value (av) pairs
 */

#ifndef _av_h_
#define _av_h_

/**
 * Initialize av
 *
 * @retval	STATUS_SUCCESS	no error
 */
NTSTATUS	init_av(void);

/**
 * Deinitialize av
 */
void		free_av(void);

/**
 * Add av-pair into list
 *
 * @param	key			key of value (you can work with value using this key)
 * @param	value		value to be stored
 * @param	type		type of pair (key and type must be unique in av-list)
 * @param	no_guard	if (no_guard) we're already inside g_av_hash_guard spinlock
 */
NTSTATUS	add_av(const void *key, void *value, int type, BOOLEAN no_guard);

/**
 * Get value by key and type and get ownership over av-list.
 * You can give ownership back as soon as possible using:
 * KeReleaseSpinLock(&g_av_hash_guard, irql);
 *
 * @param	key		key of value
 * @param	type	type of value
 * @param	irql	saved irql for KeReleaseSpinLock (can be NULL means
 *					we're already inside g_av_hash_guard spinlock)
 * @return			saved value
 * @retval	NULL	value is not found
 */
void		*get_av(const void *key, int type, KIRQL *irql);

/**
 * Delete value by key and type
 *
 * @param	key			key of value
 * @param	type		type of pair
 * @param	no_guard	if (no_guard) we're already inside g_av_hash_guard spinlock
 *
 * @retval	STATUS_SUCCESS	value has been deleted
 */
NTSTATUS	del_av(const void *key, int type, BOOLEAN no_guard);

/** guard spinlock for av-list */
extern KSPIN_LOCK	g_av_hash_guard;

/* type of values */

enum {
	/* NOTE: if (type > 0) value can be automatically freed by free() from memtrack.c */

	PROTOCOL_TO_PCHARS = 1,				/**< map NDIS_HANDLE NdisProtocolHandle -> struct PROTOCOL_CHARS */
	BINDING_TO_ADAPTER,					/**< map NDIS_HANDLE NdisBindingHandle -> struct ADAPTER_PROTOCOL */

	/* NOTE: if (type < 0) don't free value on delete */
};

#endif
