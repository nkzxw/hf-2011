// $Id: thread.h,v 1.1.1.1 2002/09/24 11:12:17 dev Exp $

#ifndef _thread_h_
#define _thread_h_

#include <process.h>

#define lib_CreateThread(	\
	lpThreadAttributes,		\
	dwStackSize,			\
	lpStartAddress,			\
	lpParameter,			\
	dwCreationFlags,		\
	lpThreadId)				\
							\
	(HANDLE)_beginthreadex((lpThreadAttributes), (unsigned)(dwStackSize), (lpStartAddress), \
		(lpParameter), (unsigned)(dwCreationFlags), (unsigned *)(lpThreadId))

#endif
