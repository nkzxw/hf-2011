// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: adapters.c,v 1.2 2003/05/13 12:48:13 dev Exp $

/** @addtogroup hook_driver
 *@{
 */

/**
 * @file adapters.c
 * Implementation of functions to work with list of network adapters
 */

#include <ntddk.h>

#include "adapters.h"
#include "memtrack.h"
#include "except.h"

/** entry of single-linked list of adapters */
struct adapter_entry {
	struct	adapter_entry *next;	/**< next entry */
	wchar_t	name[0];				/**< name of adapter */
};

static struct adapter_entry *g_first;	/**< first entry of list of adapters */
static struct adapter_entry *g_last;	/**< the last entry of list of adapters */
static KSPIN_LOCK g_list_guard;			/**< guard spinlock of list of adapters */
static int g_count;						/**< count of entries in list of adapters */

void
init_adapter_list(void)
{
	KeInitializeSpinLock(&g_list_guard);
	g_first = g_last = NULL;
	g_count = 0;
}

void
free_adapter_list(void)
{
	KIRQL irql;
	struct adapter_entry *adapter;

	KeAcquireSpinLock(&g_list_guard, &irql);

	__try {

		for (adapter = g_first; adapter != NULL;) {
			struct adapter_entry *adapter2 = adapter->next;
			free(adapter);
			adapter = adapter2;
		}

		g_first = g_last = NULL;
		g_count = 0;

	} __finally {
		KeReleaseSpinLock(&g_list_guard, irql);
	}
}

int
add_adapter(const wchar_t *name)
{
	KIRQL irql;
	struct adapter_entry *adapter;
	int result;

	KeAcquireSpinLock(&g_list_guard, &irql);

	__try {
		
		// first try to find adapter by name
		
		if (g_first != NULL) {
			for (adapter = g_first, result = 1; adapter != NULL; adapter = adapter->next, result++)
				if (wcscmp(adapter->name, name) == 0)
					__leave;
		}
		
		// not found: add adapter to list
		
		adapter = (struct adapter_entry *)malloc_np(sizeof(*adapter) +
			(wcslen(name) + 1) * sizeof(wchar_t));
		if (adapter == NULL) {
			KdPrint(("[ndis_hk] add_adapter: malloc_np!\n"));
			result = 0;
			__leave;
		}
		
		adapter->next = NULL;
		wcscpy(adapter->name, name);
		
		if (g_first == NULL)
			g_first = g_last = adapter;
		else {
			g_last->next = adapter;
			g_last = adapter;
		}
		
		result = ++g_count;

	} __finally {
		KeReleaseSpinLock(&g_list_guard, irql);
	}

	return result;
}

int
get_adapter_list(wchar_t *buf, int buf_size)
{
	KIRQL irql;
	struct adapter_entry *adapter;
	int buf_len, buf_pos;

	KeAcquireSpinLock(&g_list_guard, &irql);

	__try {

		buf_pos = 0;
		buf_len = 0;
		for (adapter = g_first; adapter != NULL; adapter = adapter->next) {
			int len = wcslen(adapter->name);

			if (buf_pos + len + 1 <= buf_size - 1) {
				wcscpy(&buf[buf_pos], adapter->name);
				buf_pos += len + 1;
			}
			
			buf_len += len + 1;
		}
		if (buf_pos < buf_size)
			buf[buf_pos] = L'\0';

	} __finally {
		KeReleaseSpinLock(&g_list_guard, irql);
	}

	return buf_len + 1;
}

/*@}*/