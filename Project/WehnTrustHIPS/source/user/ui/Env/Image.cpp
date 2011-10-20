/*
 * WenTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

Image::Image(LPCTSTR InImageName)
: ImageName(InImageName),
  ImageKey(NULL),
  ImageModule(NULL)
{
}

Image::~Image()
{
	FreeLibrary();
	Close();
}

//
// The default implementation of the check handler attempts to see if an
// instance of an image on disk differs from the one that was previously used to
// calculate the jump table by comparing the checksum in the registry with the
// checksum of the file on disk.
//
DWORD Image::Check()
{
	IMAGE_CHECKSUM_TYPE RegType;
	DWORD               Result;
	DWORD               RegChecksumBufferSize, FileChecksumBufferSize;
	BYTE                RegChecksumBuffer[64], FileChecksumBuffer[64];

	RegType                = IMAGE_CHECKSUM_TYPE_NONE;
	RegChecksumBufferSize  = sizeof(RegChecksumBuffer);
	FileChecksumBufferSize = sizeof(FileChecksumBuffer);

	do
	{
		//
		// Open the registry key 
		//
		if ((Result = Open()) != ERROR_SUCCESS)
			break;

		//
		// Obtain the current checksum from the registry
		//
		if ((Result = GetChecksum(
				&RegType,
				RegChecksumBuffer,
				&RegChecksumBufferSize)) != ERROR_SUCCESS)
			break;

		//
		// Calculate the checksum of the image on disk
		//
		if ((Result = CalculateChecksum(
				RegType,
				FileChecksumBuffer,
				&FileChecksumBufferSize)) != ERROR_SUCCESS)
			break;

		//
		// Compare the two checksums, if they mismatch, return that failure
		// information to the caller
		//
		if ((FileChecksumBufferSize != RegChecksumBufferSize) ||
		    (memcmp(
				FileChecksumBuffer,
				RegChecksumBuffer,
				RegChecksumBufferSize)))
		{
			Result = ERROR_REVISION_MISMATCH;
			break;
		}

	} while (0);

	return Result;
}

//
// Synchronize information about the image to the registry, such as base
// address, path name, and checksum information.  This method should only be
// called if Check() fails.
//
DWORD Image::Synchronize()
{
	IMAGE_CHECKSUM_TYPE ChecksumType = IMAGE_CHECKSUM_TYPE_SHA1;
	DWORD               ChecksumSize;
	DWORD               Result;
	BYTE                Checksum[64];

	ChecksumSize = sizeof(Checksum);

	do
	{
		//
		// Synchronize the original base address and image path to the registry
		//
		if (((Result = Open()) != ERROR_SUCCESS) ||
		    ((Result = SetOriginalBaseAddress(NULL)) != ERROR_SUCCESS) ||
		    ((Result = SetPath(NULL) != ERROR_SUCCESS)))
			break;

		// 
		// Calculate and set the current checksum information for the image
		//
		if (((Result = CalculateChecksum(
				ChecksumType,
				Checksum,
				&ChecksumSize)) != ERROR_SUCCESS) ||
		    ((Result = SetChecksum(
				ChecksumType,
				Checksum,
				ChecksumSize)) != ERROR_SUCCESS))
			break;

	} while (0);

	return Result;
}

//
// Creates & Opens the image's registry key for storing information about it and
// its jump table
//
DWORD Image::Open()
{
	TCHAR ImageKeyName[512];

	// 
	// If the image is already open, no harm
	//
	if (ImageKey)
		return ERROR_SUCCESS;

	//
	// Build out the image's full key name
	//
	_sntprintf_s(
			ImageKeyName, 
			sizeof(ImageKeyName) / sizeof(TCHAR) - sizeof(TCHAR),
			WEHNTRUST_BASE_KEY_ROOT TEXT("\\Images\\%s"),
			ImageName);

	ImageKeyName[sizeof(ImageKeyName) - sizeof(TCHAR)] = 0;

	//
	// Create/Open the registry key
	//
	return RegCreateKey(
			WEHNTRUST_ROOT_KEY,
			ImageKeyName,
			&ImageKey);
}

//
// Closes the image's key if it was previously opened
//
DWORD Image::Close()
{
	DWORD Result;

	if (ImageKey)
	{
		Result = RegCloseKey(
				ImageKey);

		ImageKey = NULL;
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Load an instance of the library that is to be inspected
//
DWORD Image::LoadLibrary()
{
	return ((ImageModule) || 
	        ((ImageModule = ::LoadLibrary(ImageName))))
		? ERROR_SUCCESS
		: GetLastError();
}

//
// Resolves the address of a symbol in the context of the image
//
PVOID Image::GetProcAddress(
		IN LPCSTR SymbolName)
{
	return (ImageModule)
		? ::GetProcAddress(
				ImageModule,
				SymbolName)
		: NULL;
}

//
// Closes the library module instance if one was previously opened
//
DWORD Image::FreeLibrary()
{
	DWORD Result;

	if (ImageModule)
	{
		::FreeLibrary(ImageModule);

		ImageModule = NULL;
		Result      = ERROR_SUCCESS;
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Sets the image's original base address for use by the driver when calculating
// jump table offsets.  If no base address is supplied, the one that is
// associated with the image module is used.
//
DWORD Image::SetOriginalBaseAddress(
		IN PVOID OriginalBaseAddress OPTIONAL)
{
	DWORD Result = ERROR_SUCCESS;

	if ((!OriginalBaseAddress) &&
	    ((Result = LoadLibrary()) == ERROR_SUCCESS))
		OriginalBaseAddress = ImageModule;

	if ((Result == ERROR_SUCCESS) &&
	    (ImageKey))
		Result = RegSetValueEx(
				ImageKey,
				TEXT("OriginalBaseAddress"),
				0,
				REG_DWORD,
				(LPBYTE)&OriginalBaseAddress,
				sizeof(DWORD));
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Sets the image's current checksum
//
DWORD Image::SetChecksum(
		IN IMAGE_CHECKSUM_TYPE Type,
		IN LPBYTE Checksum,
		IN DWORD ChecksumSize)
{
	DWORD Result;

	if (ImageKey)
	{
		DWORD CopyLength;
		BYTE  ChecksumBuffer[64];
		
		if ((CopyLength = ChecksumSize) > sizeof(ChecksumBuffer) - 1)
			CopyLength = sizeof(ChecksumBuffer) - 1;

		//
		// The first byte of the checksum buffer holds the type of checksum
		//
		ChecksumBuffer[0] = (BYTE)Type;

		//
		// After that is the actual checksum
		//
		memcpy(
				ChecksumBuffer + 1,
				Checksum,
				CopyLength);

		Result = RegSetValueEx(
				ImageKey,
				TEXT("Checksum"),
				0,
				REG_BINARY,
				ChecksumBuffer,
				CopyLength + 1);
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Get the image's checksum from the registry
//
DWORD Image::GetChecksum(
		OUT PIMAGE_CHECKSUM_TYPE Type,
		OUT LPBYTE Checksum,
		IN OUT LPDWORD ChecksumSize)
{
	DWORD Result;

	if (ImageKey)
	{
		DWORD ChecksumBufferSize;
		BYTE  ChecksumBuffer[64];

		ChecksumBufferSize = sizeof(ChecksumBuffer);

		if ((Result = RegQueryValueEx(
				ImageKey,
				TEXT("Checksum"),
				0,
				NULL,
				ChecksumBuffer,
				&ChecksumBufferSize)) == ERROR_SUCCESS)
		{
			DWORD CopyLength;

			//
			// Set the outbound checksum type
			//
			*Type = (IMAGE_CHECKSUM_TYPE)ChecksumBuffer[0];

			//
			// Copy the checksum into the output buffer
			//
			if ((CopyLength = *ChecksumSize) > ChecksumBufferSize - 1)
				CopyLength = ChecksumBufferSize - 1;

			memcpy(
					Checksum,
					ChecksumBuffer + 1,
					CopyLength);

			*ChecksumSize = CopyLength;
		}
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Sets the NT-formatted path to the image.  If no path is supplied, the path
// associated with the module will be used.  The format of the path is:
//
// \??\C:\Windows\System32\...
//
DWORD Image::SetPath(
		IN LPCSTR Path OPTIONAL)
{
	DWORD Result = ERROR_SUCCESS;
	TCHAR NtPath[1024];

	//
	// If no path was supplied, the path associated with the image module should
	// be used.
	//
	if (!Path)
	{
		lstrcpy(NtPath, TEXT("\\??\\"));

		Result = GetImageFileName(
				NtPath + 4,
				sizeof(NtPath) - 5);
	}
	else
	{
		// 
		// Build the NT path from the supplied path
		//
		_sntprintf_s(
				NtPath,
				sizeof(NtPath) / sizeof(TCHAR) - sizeof(TCHAR),
				TEXT("\\??\\%s"),
				Path);
	}

	NtPath[sizeof(NtPath) - sizeof(TCHAR)] = 0;

	//
	// If we have thus far been successful, set the path to what was determined
	//
	if ((Result == ERROR_SUCCESS) &&
	    (ImageKey))
	{
		Result = RegSetValueEx(
				ImageKey,
				TEXT("Path"),
				0,
				REG_SZ,
				(LPBYTE)NtPath,
				strlen(NtPath) + 1);
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Set the flags for the image that will be interpreted by the driver during
// jump table creation
//
DWORD Image::SetFlags(
		IN DWORD Flags)
{
	DWORD Result;

	if (ImageKey)
		Result = RegSetValueEx(
				ImageKey,
				TEXT("Flags"),
				0,
				REG_DWORD,
				(LPBYTE)&Flags,
				sizeof(DWORD));
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Add an image address table entry to the image's jump table
//
DWORD Image::AddAddressTableEntry(
		IN PVOID VirtualAddress,
		IN LPCSTR Function OPTIONAL,
		IN LPBYTE Prepend OPTIONAL,
		IN DWORD PrependSize OPTIONAL)
{
	DWORD Result;
	TCHAR SubKeyName[512];

	// 
	// Build the sub-key name for the address table entry
	//
	_sntprintf_s(
			SubKeyName,
			sizeof(SubKeyName) / sizeof(TCHAR) - sizeof(TCHAR),
			TEXT("AddressTable\\%.8x"),
			VirtualAddress);

	SubKeyName[sizeof(SubKeyName) - 1] = 0;

	if (ImageKey)
	{
		HKEY SubKey = NULL;

		if ((Result = RegCreateKey(
				ImageKey,
				SubKeyName,
				&SubKey)) == ERROR_SUCCESS)
		{
			//
			// If a function name was supplied, set it in the registry as well
			//
			if (Function)
				RegSetValueEx(
						SubKey,
						TEXT("Function"),
						0,
						REG_SZ,
						(LPBYTE)Function,
						strlen(Function) + 1);

			//
			// If the entry has prepention information, set it in the registry
			//
			if ((Prepend) &&
			    (PrependSize))
				RegSetValueEx(
						SubKey,
						TEXT("Prepend"),
						0,
						REG_BINARY,
						Prepend,
						PrependSize);

			RegCloseKey(SubKey);
		}
	}
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Flushes all of the entries from the address table for the image
//
DWORD Image::FlushAddressTable()
{
	DWORD Result;

	if (ImageKey)
		Result = SHDeleteKey(
				ImageKey,
				TEXT("AddressTable"));
	else
		Result = ERROR_INVALID_HANDLE;

	return Result;
}

//
// Check if a pointer is a valid data address for the image
//
BOOL Image::IsValidDataPointer(
		IN PVOID VirtualAddress)
{
	return FALSE;

// XXX Not implemented yet

//	PIMAGE_DOS_HEADER DosHeader;
//	PIMAGE_NT_HEADERS NtHeaders;

//	DosHeader = (PIMAGE_DOS_HEADER)ImageModule;
}

//
// Obtains the fully qualified path to the image file
//
DWORD Image::GetImageFileName(
		OUT LPSTR FileName,
		IN DWORD FileNameSize)
{
	DWORD Result;

	if ((Result = LoadLibrary()) == ERROR_SUCCESS)
	{
		if (!GetModuleFileName(
				ImageModule,
				FileName,
				FileNameSize))
			Result = GetLastError();
	}

	return Result;
}

//
// Calculates the checksum of the current image with the provided algorithm and
// outupts the digest in the supplied buffer
//
DWORD Image::CalculateChecksum(
		IN IMAGE_CHECKSUM_TYPE Type,
		OUT LPBYTE Checksum,
		IN OUT LPDWORD ChecksumSize)
{
	SHA1_CTX Sha1Context;
	HANDLE   FileHandle = NULL;
	HANDLE   FileMappingHandle = NULL;
	LPVOID   FileViewBase = NULL;
	DWORD    FileSize = 0;
	DWORD    Result;
	TCHAR    ImageFileName[1024];

	//
	// Initialize hashing contexts
	//
	switch (Type)
	{
		case IMAGE_CHECKSUM_TYPE_SHA1:
			SHA1_Init(&Sha1Context);
			break;
		default:
			break;
	}

	do
	{
		// 
		// Get the path to the image file
		//
		if ((Result = GetImageFileName(
				ImageFileName,
				sizeof(ImageFileName) - sizeof(TCHAR))) != ERROR_SUCCESS)
			break;

		ImageFileName[sizeof(ImageFileName) - sizeof(TCHAR)] = 0;

		//
		// Get a handle to the file
		//
		if ((FileHandle = CreateFile(
				ImageFileName,
				FILE_READ_DATA,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL)) == INVALID_HANDLE_VALUE)
		{
			Result = GetLastError();
			break;
		}

		//
		// Create and map a view of the image file that is to be checksummed
		//
		if (!(FileMappingHandle = CreateFileMapping(
				FileHandle,
				NULL,
				PAGE_READONLY | SEC_COMMIT,
				0,
				0,
				NULL)))
		{
			Result = GetLastError();
			break;
		}

		if (!(FileViewBase = MapViewOfFile(
				FileMappingHandle,
				FILE_MAP_READ,
				0,
				0,
				0)))
		{
			Result = GetLastError();
			break;
		}


		//
		// Get the size of the file
		//
		if ((FileSize = GetFileSize(
				FileHandle,
				NULL)) == INVALID_FILE_SIZE)
		{
			Result = GetLastError();
			break;
		}

		//
		// Calculate the checksum
		//
		switch (Type)
		{
			case IMAGE_CHECKSUM_TYPE_SHA1:
				SHA1_Update(
						&Sha1Context,
						(PUCHAR)FileViewBase,
						FileSize);

				if (*ChecksumSize >= SHA1_HASH_SIZE)
				{
					SHA1_Final(
							&Sha1Context,
							Checksum);

					*ChecksumSize = SHA1_HASH_SIZE;
				}
				else
					Result = ERROR_BUFFER_OVERFLOW;
				break;
			default:
				//
				// Unsupported checksum algorithm
				//
				Result = ERROR_NOT_SUPPORTED;
				break;
		}

	} while (0);

	//
	// Destroy the mapping handle and view
	//
	if (FileViewBase)
		UnmapViewOfFile(FileViewBase);
	if (FileMappingHandle)
		CloseHandle(FileMappingHandle);
	if (FileHandle)
		CloseHandle(FileHandle);

	return Result;
}
