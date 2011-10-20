/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_ENV_ENV_H
#define _WEHNTRUST_WEHNTRUST_ENV_ENV_H

//
// The Env class manages the synchronization of static state from user-mode to
// the driver in times of need, such as immediately following installation or
// after a person installs an update that causes on of the image entries to
// become invaldiated.
//
class Env
{
	public:
		static DWORD CheckSynchronize();
		static DWORD CheckSynchronizeInstall();
	protected:
		static DWORD GetZwProtectVirtualMemoryIndex();

		static DWORD DoCheckSynchronize(
				OUT PBOOLEAN Updated = NULL);
};

#include "Image.h"

#endif
