// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: adapters.h,v 1.2 2003/05/13 12:48:13 dev Exp $

/**
 * @file adapters.h
 * Set of functions to work with list of network adapters
 */

#ifndef _adapters_h_
#define _adapters_h_

/**
 * Initialize list of adapters
 */
void	init_adapter_list(void);

/**
 * Deinitialize list of adapters
 */
void	free_adapter_list(void);

/**
 * Add adapter to list
 * @param	name	name of adapter
 * @return			assigned number of adapter
 * @retval	0		error
 */
int		add_adapter(const wchar_t *name);

/**
 * Get list of adapters.
 * Function copies the whole list into wide-char buffer. Names are delimited by (wchar_t)0.
 * Buffer ends by empty unicode string (double (wchar_t)0, (wchar_t)0 at the end of buffer)
 *
 * @param	buf			output buffer for adapter names
 * @param	buf_size	size in wchar_t of buf (can be 0)
 *
 * @return				number of wchar_t has to be in buffer
 *						if greater than buf_size only partial information has been copied
 */
int		get_adapter_list(wchar_t *buf, int buf_size);

#endif
