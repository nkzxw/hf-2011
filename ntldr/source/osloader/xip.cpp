//********************************************************************
//	created:	26:9:2008   10:36
//	file:		xip.cpp
//	author:		tiamo
//	purpose:	eXecute In Place
//********************************************************************

#include "stdafx.h"

#if _XIP_SUPPORT_

//
// xip enabled
//
BOOLEAN													XIPEnabled;

//
// page count
//
ULONG													XIPPageCount;

//
// base page
//
ULONG													XIPBasePage;

//
// xip load path
//
PCHAR													XIPLoadPath;

//
// boot flags
//
BOOLEAN													XIPBootFlag;

//
// bios parameter block
//
typedef struct BIOS_PARAMETER_BLOCK
{
	//
	// bytes per sector
	//
	USHORT												BytesPerSector;

	//
	// sector per cluster
	//
	UCHAR												SectorsPerCluster;

	//
	// reserved sectors
	//
	USHORT												ReservedSectors;

	//
	// fat count
	//
	UCHAR												Fats;

	//
	// root entries
	//
	USHORT												RootEntries;

	//
	// sectors
	//
	USHORT												Sectors;

	//
	// media
	//
	UCHAR												Media;

	//
	// sectors per fat
	//
	USHORT												SectorsPerFat;

	//
	// sectors per track
	//
	USHORT												SectorsPerTrack;

	//
	// heads
	//
	USHORT												Heads;

	//
	// hidden sectors
	//
	ULONG												HiddenSectors;

	//
	// large sectors
	//
	ULONG												LargeSectors;

	//
	// sectors per fat
	//
	ULONG												LargeSectorsPerFat;

}BIOS_PARAMETER_BLOCK,*PBIOS_PARAMETER_BLOCK;

//
// read xip rom/ram to memory
//
ARC_STATUS XipLargeRead(__in ULONG FileId,__in ULONG BasePage,__in ULONG PageCount)
{
	//
	// search for a unused PTE
	//	virtual address = 0x20000000 = 512MB
	//	index			= 0x20000000 >> 22
	//
	ULONG VirtualAddress								= 0x20000000;
	PHARDWARE_PTE Pte									= reinterpret_cast<PHARDWARE_PTE>(PDE_BASE_X86) + (VirtualAddress >> 22);
	ULONG i												= 0;

	//
	// search 128MB memory
	//
	while(i < 0x20)
	{
		//
		// unused pte found
		//
		if(!Pte->Valid)
			break;

		//
		// size = 4MB (large page size)
		//
		VirtualAddress									+= 4 * 1024 * 1024;
		Pte												+= 1;
		i												+= 1;
	}

	//
	// no memory
	//
	if(i == 0x20)
		return ENOMEM;

	//
	// enable PSE
	//
	__asm
	{
		_emit	0x0f
		_emit	0x20
		_emit	0xe0													// mov     eax, cr4
		or		eax, 10h
		_emit	0x0f
		_emit	0x22
		_emit	0xe0													// mov     cr4, eax
	}

	//
	// allocate a 64K cluster buffer
	//
	PVOID Buffer										= BlAllocateHeap(64 * 1024);
	if(!Buffer)
		return ENOMEM;

	ULONG MemoryOffset									= 0;
	ULONG FileOffset									= 0;
	ARC_STATUS Status									= ESUCCESS;

	while(Status == ESUCCESS && PageCount)
	{
		//
		// setup large page,and flush tlb
		//
		Pte->Write										= TRUE;
		Pte->Valid										= TRUE;
		Pte->LargePage									= TRUE;
		Pte->PageFrameNumber							= BasePage;
		PVOID MappedAddress								= reinterpret_cast<PVOID>(VirtualAddress);

		__asm
		{
			mov		eax,cr3
			mov		cr3,eax
		}

		//
		// compute the next page and left page count
		//
		BasePage										+= 4 * 1024 * 1024 / PAGE_SIZE;
		if(PageCount >= 4 * 1024 * 1024 / PAGE_SIZE)
			PageCount									-= 4 * 1024 * 1024 / PAGE_SIZE;
		else
			PageCount									= 0;

		//
		// total size is 4MB(large page size),for each run,we read 8KB data from file
		//
		for(ULONG i = 0; i < 4 * 1024 * 1024 / (8 * 1024); i ++)
		{
			//
			// read file into buffer,8KB size
			//
			ULONG ReadCount								= 0;
			Status										= BlRead(FileId,Buffer,8 * 1024,&ReadCount);
			if(Status != ESUCCESS)
				break;

			//
			// first sector is FAT boot sector,we need do some fixup
			//
			if(!FileOffset || ReadCount >= 8 * 1024)
			{
				//
				// BUGBUGBUG!!! misalignment?
				//
				PBIOS_PARAMETER_BLOCK Bpb				= Add2Ptr(Buffer,0x0b,PBIOS_PARAMETER_BLOCK);

				//
				// sector size must be 512,cluster size must be 4K
				//
				if(Bpb->BytesPerSector != SECTOR_SIZE || Bpb->SectorsPerCluster != PAGE_SIZE / SECTOR_SIZE)
				{
					//
					// invalid format,exit
					// XXX!!! how about return status?
					//
					PageCount							= 0;
					break;
				}

				//
				// sector per fat
				//
				ULONG SectorsPerFat						= Bpb->SectorsPerFat;
				if(!SectorsPerFat)
					SectorsPerFat						= Bpb->LargeSectorsPerFat;

				//
				// meta size,dir entry size = 32
				//
				ULONG MetaSize							= SectorsPerFat * Bpb->Fats + (Bpb->ReservedSectors << SECTOR_SHIFT) + (Bpb->RootEntries * 32);

				//
				// round to 4K
				//
				MetaSize								= BYTE_OFFSET(MetaSize);;

				//
				// check we should do some fixup?
				//
				ULONG AdjustedSize						= PAGE_SIZE - MetaSize;
				if(AdjustedSize	< PAGE_SIZE)
					Bpb->ReservedSectors				+= static_cast<USHORT>(AdjustedSize >> SECTOR_SHIFT);

				//
				// copy boot sector
				//
				RtlCopyMemory(MappedAddress,Buffer,SECTOR_SIZE);
				MappedAddress							= Add2Ptr(MappedAddress,SECTOR_SIZE,PVOID);

				//
				// zero reserved space
				//
				RtlZeroMemory(MappedAddress,AdjustedSize);
				MappedAddress							= Add2Ptr(MappedAddress,AdjustedSize,PVOID);

				//
				// copy the left bytes
				//
				RtlCopyMemory(MappedAddress,Add2Ptr(Buffer,SECTOR_SIZE,PVOID),ReadCount - SECTOR_SIZE);
				MappedAddress							= Add2Ptr(MappedAddress,ReadCount - SECTOR_SIZE,PVOID);

				//
				// read partial data
				//
				ULONG AdjustedReadSize					= 0;
				Status									= BlRead(FileId,Buffer,ReadCount - AdjustedSize,&AdjustedReadSize);
				if(Status != ESUCCESS)
					break;

				//
				// read error
				//
				if(ReadCount - AdjustedSize != AdjustedReadSize)
				{
					//
					// exit main loop
					//
					PageCount							= 0;
					break;
				}

				//
				// copy it
				//
				RtlCopyMemory(MappedAddress,Buffer,AdjustedReadSize);
				MappedAddress							= Add2Ptr(MappedAddress,AdjustedReadSize,PVOID);
				FileOffset								+= ReadCount + AdjustedReadSize;

				//
				// additional cluster has been read
				//
				i										-= 1;
			}
			else
			{
				//
				// normal file data,copy to memory
				//
				RtlCopyMemory(MappedAddress,Buffer,ReadCount);

				//
				// end of file ?
				//
				if(ReadCount < 8 * 1024)
				{
					//
					// also exit main loop
					//
					PageCount							= 0;
					break;
				}

				//
				// update pointers
				//
				MappedAddress							= Add2Ptr(MappedAddress,ReadCount,PVOID);
				FileOffset								+= ReadCount;
			}
		}
	}

	//
	// clear used large page
	//
	*Pte												= ZeroPte;

	//
	// flush tlb
	//
	__asm
	{
		mov		eax,cr3
		mov		cr3,eax
	}

	return Status;
}
#endif