/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_ENV_IMAGES_USER32_H
#define _WEHNTRUST_WEHNTRUST_ENV_IMAGES_USER32_H

//
// This class handles synchronization with the USER32 jump table
//
class User32 : 
	public Image
{
	public:
		User32();
		~User32();

		//
		// Pure virtual implementors
		//
		DWORD Synchronize();
	protected:

		DWORD GetWin32EntryTableSymbols();
};

#endif
