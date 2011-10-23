//********************************************************************
//	created:	19:8:2008   1:11
//	file:		loaddump.cpp
//	author:		tiamo
//	purpose:	load triage dump
//********************************************************************

#include "stdafx.h"

//
// load triage dump
// yes,xp sp2's osloader.exe has a lot of bugs in this routine,so rewrite it
//
ARC_STATUS BlLoadTriageDump(__in ULONG DeviceId,__out PVOID* DumpBlock)
{
	*DumpBlock											= 0;

	//
	// open file
	//
	ULONG FileId;
	ARC_STATUS Status									= BlOpen(DeviceId,"\\pagefile.sys",ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// read signature and check it
	//
	ULONG Signature[2];
	ULONG Count											= 0;
	ULONG BasePage										= 0;
	Status												= BlRead(FileId,Signature,sizeof(Signature),&Count);
	if(Status == ESUCCESS && Signature[0] == 'EGAP' && Signature[1] == 'PMUD')
	{
		//
		// read type
		//
		LARGE_INTEGER SeekOffset;
		SeekOffset.QuadPart								= 0xf88;
		Status											= BlSeek(FileId,&SeekOffset,SeekAbsolute);
		if(Status == ESUCCESS)
		{
			ULONG Type;
			Status										= BlRead(FileId,&Type,sizeof(Type),&Count);
			if(Status == ESUCCESS && Type == 4)
			{
				//
				// allocated buffer,max triage dump size is 64KB,16 pages
				//
				Status									= BlAllocateAlignedDescriptor(LoaderOsloaderHeap,0,64 * 1024 * 1024 / PAGE_SIZE,1,&BasePage);
				if(Status == ESUCCESS)
				{
					//
					// read 64KB data
					//
					PVOID Buffer						= MAKE_KERNEL_ADDRESS_PAGE(BasePage);
					Status								= BlReadAtOffset(FileId,0,64 * 1024 * 1024,Buffer);
					if(Status == ESUCCESS)
					{
						//
						// check triage dump signature
						//
						ULONG Offset					= *Add2Ptr(Buffer,PAGE_SIZE + 8,PULONG);
						if(Offset <= 64 * 1024 * 1024 - 4 && *Add2Ptr(Buffer,Offset,PULONG) == 'DGRT')
							*DumpBlock					= Buffer;
						else
							Status						= EINVAL;
					}
				}
			}
		}
	}

	BlClose(FileId);

	if(Status != ESUCCESS && BasePage)
		BlFreeDescriptor(BasePage);

	return Status;
}