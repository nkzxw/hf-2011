//********************************************************************
//	created:	19:8:2008   20:18
//	file:		netboot.h
//	author:		tiamo
//	purpose:	net boot
//********************************************************************

#pragma once

//
// net file context
//
typedef struct _NET_FILE_CONTEXT
{
	//
	// physical address
	//
	PVOID												FileBuffer;

	//
	// file size
	//
	ULONG												FileSize;
}NET_FILE_CONTEXT,*PNET_FILE_CONTEXT;

//
// initialize net boot
//
ARC_STATUS NetInitialize();