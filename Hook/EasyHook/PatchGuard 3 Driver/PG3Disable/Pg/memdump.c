/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/

#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif

typedef union _SYSTEM_BASIC_INFORMATION {
	struct
	{
		ULONG			Reserved;
		ULONG			TimerResolution;
		ULONG			PageSize;
		ULONG			NumberOfPhysicalPages;
		ULONG			LowestPhysicalPageNumber;
		ULONG			HighestPhysicalPageNumber;
		ULONG			AllocationGranularity;
		ULONG_PTR		MinimumUserModeAddress;
		ULONG_PTR		MaximumUserModeAddress;
		KAFFINITY		ActiveProcessorsAffinityMask;
		CCHAR			NumberOfProcessors;
	};
	ULONGLONG			Data[8];
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;


/************************************************************************
*************************************** PgProcessMemoryPatchList
*************************************************************************

Description:
	
	This method is the core part of PG3Disable. It provides a mechanism
	to search for any byte sequence in the entire physical RAM.

	I applied many optimizations to make this search as fast as possible.
	It performs quite well on my 4 GB machine and I think this is a
	representative system configuration!
*/
NTSTATUS PgProcessMemoryPatchList(
		PMEMORY_PATCH_INFO InPatchList,
		ULONG InPatchCount)
{
	UNICODE_STRING					UnicodeString;    
	OBJECT_ATTRIBUTES				ObjectAttributes; 
	HANDLE							hPhysicalMemoryHandle;  
	PHYSICAL_ADDRESS				MappedLength;       
	UCHAR*							pViewBase;
	PHYSICAL_ADDRESS				PhysicalOffset;
	PHYSICAL_ADDRESS				TmpPhysical;
	NTSTATUS						NtStatus;
	LONGLONG						Index;
	ULONGLONG						VirtualAddress;
	SYSTEM_BASIC_INFORMATION		BasicInfo;
	ULONG							RequiredSize;
	ULONG							iPatch;
	PMEMORY_PATCH_INFO				Patch;

	/*
		This will simultanously scan the page for all patch vectors, which will
		bring us a significant performance gain in constrast to mapping all those
		pages for each patch again...
	*/
	for(iPatch = 0; iPatch < InPatchCount; iPatch++)
	{
		Patch = &InPatchList[iPatch];

		for(VirtualAddress = KernelImageStart; VirtualAddress < KernelImageEnd - Patch->ByteCount; VirtualAddress++)
		{
			if(Patch->ReplaceShift < 0)
			{
				if(VirtualAddress < -Patch->ReplaceShift)
					continue;
			}
			else
			{
				if(VirtualAddress > KernelImageEnd - Patch->ByteCount - Patch->ReplaceShift)
					break;
			}

			/*
				Scan whole image...
			*/
			if(memcmp(VirtualAddress, Patch->FindBytes, Patch->ByteCount) == 0)
			{
				if(Patch->LocCount + 1 <= MEMORY_PATCH_MAXLOC)
				{
					Patch->LocArray[Patch->LocCount++] = VirtualAddress;
				}

				if(Patch->CanReplace)
					memcpy(VirtualAddress + Patch->ReplaceShift, Patch->ReplaceBytes, Patch->ByteCount);
			}
		}
	}

	return STATUS_SUCCESS;
}