// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: memtrack.h,v 1.2 2003/05/13 12:47:39 dev Exp $

/**
 * @file memtrack.h
 * Debug nonpaged pool support.
 * Can help you to find your memory buffers overrun and underrun and memory leaks.
 */

#ifndef _memtrack_h_
#define _memtrack_h_

/** tag for memory blocks */
#define MEM_TAG		'1VRD'

/**
 * @fn memtrack_init
 * Initialize memory tracking engine
 */

/**
 * @fn memtrack_free
 * Deinitialize memory tracking engine
 */

/**
 * @def malloc_np
 * Allocate memory from nonpaged pool
 * @param	size	size of block
 * @return			address of allocated block in nonpaged pool
 * @retval	NULL	error
 */

/**
 * @def free
 * Free block allocated by malloc_np
 * @param	ptr		pointer to memory block (can't be NULL)
 */

#if DBG

void	memtrack_init(void);

void	memtrack_free(void);

/**
 * Allocate memory from nonpaged pool and store name of file and line of code with this block
 * @param	size	size of block
 * @param	file	name of file to associate with memory block
 * @param	line	line number to associate with memory block
 * @return			address of allocated block in nonpaged pool
 * @retval	NULL	error
 */
void	*mt_malloc(ULONG size, const char *file, ULONG line);

#define malloc_np(size)	mt_malloc((size), __FILE__, __LINE__)	

void free(void *ptr);

#define _TEST_ME_	__asm int 3

#else /* DBG */

#define memtrack_init()
#define memtrack_free()

#define malloc_np(size)	ExAllocatePoolWithTag(NonPagedPool, (size), MEM_TAG)
#define free(ptr)		ExFreePool(ptr)

/** macro for debug break in checked build */
#define _TEST_ME_

#endif /* DBG */

#endif
