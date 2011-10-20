/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_ENV_IMAGES_NTDLL_H
#define _WEHNTRUST_WEHNTRUST_ENV_IMAGES_NTDLL_H

//
// This class handles synchronization with the NTDLL jump table
//
class Ntdll : 
	public Image
{
	public:
		Ntdll();
		~Ntdll();

		//
		// Pure virtual implementors
		//
		DWORD Synchronize();
	protected:
};

#endif
