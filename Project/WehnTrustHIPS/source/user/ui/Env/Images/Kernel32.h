/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_ENV_IMAGES_KERNEL32_H
#define _WEHNTRUST_WEHNTRUST_ENV_IMAGES_KERNEL32_H

//
// This class handles synchronization with the KERNEL32 jump table
//
class Kernel32 : 
	public Image
{
	public:
		Kernel32();
		~Kernel32();

		//
		// Pure virtual implementors
		//
		DWORD Synchronize();
	protected:

		DWORD ResolveBaseThreadStartThunk();
		DWORD ResolveBaseProcessStartThunk();

		static VOID DummyThreadFunction();
};

#endif
