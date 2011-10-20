/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_ENV_IMAGE_H
#define _WEHNTRUST_WEHNTRUST_ENV_IMAGE_H

//
// This class is responsible for providing an interface to a logical 'image' in
// the registry.  An image is something that will be looked at during the
// initialization (and possible re-synchronization) of the driver in order to
// validate the image's checksum and calculate jump tables if necessary.
//
class Image
{
	public:
		Image(IN LPCTSTR ImageName);
		virtual ~Image();

		//
		// Extendable methods
		//
		virtual DWORD Check();
		virtual DWORD Synchronize();

		//
		// Base methods
		//
		DWORD Open();
		DWORD Close();

		DWORD LoadLibrary();
		PVOID GetProcAddress(
				IN LPCSTR SymbolName);
		DWORD FreeLibrary();

		DWORD SetOriginalBaseAddress(
				IN PVOID OriginalBaseAddress OPTIONAL);
		DWORD SetChecksum(
				IN IMAGE_CHECKSUM_TYPE Type,
				IN LPBYTE Checksum,
				IN DWORD ChecksumSize);
		DWORD GetChecksum(
				OUT PIMAGE_CHECKSUM_TYPE Type,
				OUT LPBYTE Checksum,
				IN OUT LPDWORD CheckumSize);
		DWORD SetPath(
				IN LPCSTR Path OPTIONAL);
		DWORD SetFlags(
				IN DWORD Flags);

		DWORD AddAddressTableEntry(
				IN PVOID VirtualAddress,
				IN LPCSTR Function = NULL OPTIONAL,
				IN LPBYTE Prepend = NULL OPTIONAL,
				IN DWORD PrependSize = 0 OPTIONAL);
		DWORD FlushAddressTable();

		BOOL IsValidDataPointer(
				IN PVOID VirtualAddress);
	protected:

		DWORD GetImageFileName(
				OUT LPSTR FileName,
				IN DWORD FileNameSize);
		DWORD CalculateChecksum(
				IN IMAGE_CHECKSUM_TYPE Type,
				OUT LPBYTE Checksum,
				IN OUT LPDWORD ChecksumSize);

		//
		// Attributes
		//

		LPCTSTR ImageName;
		HKEY    ImageKey;
		HMODULE ImageModule;
};

//
// Individual image includes
//
#include "Images/Ntdll.h"
#include "Images/Kernel32.h"
#include "Images/User32.h"

#endif
