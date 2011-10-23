//********************************************************************
//	created:	10:8:2008   18:26
//	file:		memory.cpp
//	author:		tiamo
//	purpose:	memory support
//********************************************************************

#include "stdafx.h"

//
// 1-megabyte boundary line (in pages)
//
#define _1MB											(0x100000UL >> PAGE_SHIFT)

//
// 16-megabyte boundary line (in pages)
//
#define _16MB											(0x1000000UL >> PAGE_SHIFT)

//
// bogus memory line.  (We don't ever want to use the memory that is in the 0x40 pages just under the 16Mb line.)
//
#define _16MB_BOGUS										((0x1000000UL - 0x40UL * PAGE_SIZE) >> PAGE_SHIFT)

//
// 4-gb boundary line (in pages)
//
#define _4GB											(0x100000000ULL >> PAGE_SHIFT)

//
// rom start page
//
#define ROM_START_PAGE									(0x0A0000UL >> PAGE_SHIFT)

//
// rom end page
//
#define ROM_END_PAGE									(0x100000UL >> PAGE_SHIFT)

//
// memory descriptor array size
//
#define MAX_DESCRIPTORS									60

//
// hyper space pde entry
//
#define HYPER_SPACE_ENTRY								768

//
// permanent heap start page
//
#define PERMANENT_HEAP_START							0x39

//
// temporary heap start page
//
#define TEMPORARY_HEAP_START							0x50

//
// osloader stack start
//
#define OSLOADER_STACK_START							0x60

//
// osloader stack end
//
#define OSLOADER_STACK_END								0x62

//
//heap allocation block granularity.
//
#define BL_GRANULARITY									8

//
// number of loader heap page
//
#define BL_HEAP_PAGES									16

//
// number of stack page
//
#define BL_STACK_PAGES									8

//
// disk cache
//
PVOID													FwDiskCache = reinterpret_cast<PVOID>(0x30000);

//
// temporary heap
//
ULONG													FwTemporaryHeap	= TEMPORARY_HEAP_START << PAGE_SHIFT;

//
// permanent heap
//
ULONG													FwPermanentHeap = PERMANENT_HEAP_START << PAGE_SHIFT;

//
// firmware descriptors is valid
//
BOOLEAN													FwDescriptorsValid = TRUE;

//
// memory descriptors count
//
ULONG													NumberDescriptors;

//
// memory descriptors array
//
MEMORY_DESCRIPTOR										MDArray[MAX_DESCRIPTORS];

//
// heap free
//
ULONG													BlHeapFree;

//
// heap limit
//
ULONG													BlHeapLimit;

//
// allocation policy for heap
//
ALLOCATION_POLICY										BlHeapAllocationPolicy = BlAllocateBestFit;

//
// allocation policy
//
ALLOCATION_POLICY										BlMemoryAllocationPolicy = BlAllocateBestFit;

//
// /3GB
//
ULONG													BlVirtualBias;

//
// Tss base page
//
ULONG													TssBasePage;

//
// pcr base page
//
ULONG													PcrBasePage;

//
// PAE enabled
//
BOOLEAN													PaeEnabled;

//
// lowest page
//
ULONG													BlLowestPage;

//
// highest page
//
ULONG													BlHighestPage;

//
// old kernel
//
BOOLEAN													BlOldKernel;

//
// restoring
//
BOOLEAN													BlRestoring;

//
// firmware pool end
//
ULONG													FwPoolEnd;

//
// pool start
//
ULONG													FwPoolStart;

//
// hal pte
//
PHARDWARE_PTE											HalPT;

//
// PDE
//
PHARDWARE_PTE											PDE;

//
// pdpt
//
PHARDWARE_PTE_PAE										PDPT;

//
// usable base
//
ULONG													BlUsableBase;

//
// usable limit
//
ULONG													BlUsableLimit = 0x1000;

//
// highest pde
//
ULONG													HighestPde = 4;

//
// split descriptor
//
PMEMORY_ALLOCATION_DESCRIPTOR							SplitDescriptor;

//
// zero pte
//
HARDWARE_PTE											ZeroPte;

//
// track memory usage
//
VOID BlpTrackUsage(__in TYPE_OF_MEMORY MemoryType,__in ULONG StartPage,__in ULONG Count)
{
	if(BlRestoring)
		return;

	if(MemoryType == MemoryFree || MemoryType == MemoryBad || MemoryType == MemoryFirmwareTemporary || MemoryType == MemorySpecialMemory)
		return;

	if(MemoryType == LoaderXIPMemory || MemoryType == LoaderReserved)
		return;

	if(BlOldKernel)
		return;

	if(StartPage + Count <= 0x200)
		return;

	if(StartPage + Count > BlHighestPage)
		BlHighestPage									= StartPage + Count;

	if(!BlLowestPage || StartPage < BlLowestPage)
		BlLowestPage									= StartPage;
}

//
// set descriptor region
//
ARC_STATUS MempSetDescriptorRegion(__in ULONG StartPage,__in ULONG EndPage,__in TYPE_OF_MEMORY MemoryType)
{
	//
	// This is a completely bogus memory descriptor. Ignore it.
	//
	if(EndPage <= StartPage)
		return ESUCCESS;

	BOOLEAN RegionAdded									= FALSE;

	//
	// clip, remove, any descriptors in target area
	//
	for(ULONG i = 0; i < NumberDescriptors; i ++)
	{
		ULONG sp										= MDArray[i].BasePage;
		ULONG ep										= MDArray[i].BasePage + MDArray[i].PageCount;
		MEMORY_TYPE	 mt									= MDArray[i].MemoryType;

		if(sp < StartPage)
		{
			if(ep > StartPage  &&  ep <= EndPage)
			{
				//
				//          **************
				//     ----------
				//
				// truncate this descriptor
				//
				ep										= StartPage;
			}

			if(ep > EndPage)
			{
				//
				//       *******
				//   --------------
				//
				// target area is contained totally within this descriptor.
				// split the descriptor into two ranges
				//
				if(NumberDescriptors == MAX_DESCRIPTORS)
					return ENOMEM;

				//
				// add descriptor for EndPage - ep
				//
				MDArray[NumberDescriptors].MemoryType	= mt;
				MDArray[NumberDescriptors].BasePage		= EndPage;
				MDArray[NumberDescriptors].PageCount	= ep - EndPage;
				NumberDescriptors						+= 1;

				//
				// adjust current descriptor for sp - StartPage
				//
				ep										= StartPage;
			}
		}
		else
		{
			//
			// sp >= StartPage
			//
			if(sp < EndPage)
			{
				if(ep < EndPage)
				{
					//
					//   *********
					//     ----
					//
					// this descriptor is totally within the target area - remove it
					//
					ep									= sp;
				}
				else
				{
					//
					//    **********
					//       ---------
					//
					// bump begining page of this descriptor
					//
					sp									= EndPage;
				}
			}
		}

		//
		// check if the new range can be appended or prepended to this descriptor
		//
		if(mt == MemoryType && !RegionAdded)
		{
			if(sp == EndPage)
			{
				//
				// prepend region being set
				//
				sp										= StartPage;
				RegionAdded								= TRUE;
			}
			else if (ep == StartPage)
			{
				//
				// append region being set
				//
				ep										= EndPage;
				RegionAdded								= TRUE;
			}
		}

		//
		// descriptor was not editted
		//
		if(MDArray[i].BasePage == sp  &&  MDArray[i].PageCount == ep - sp)
			continue;

		//
		// reset this descriptor
		//
		MDArray[i].BasePage								= sp;
		MDArray[i].PageCount							= ep - sp;

		if(ep == sp)
		{
			//
			// descriptor vanished - remove it
			//
			NumberDescriptors							-= 1;
			if(i < NumberDescriptors)
				MDArray[i]								= MDArray[NumberDescriptors];

			//
			// backup & recheck current position
			//
			i											-= 1;
		}
	}

	//
	// if region wasn't already added to a neighboring region, then create a new descriptor now
	//
	if(!RegionAdded  &&  MemoryType < LoaderMaximum)
	{
		if(NumberDescriptors == MAX_DESCRIPTORS)
			return ENOMEM;

		MDArray[NumberDescriptors].MemoryType			= MemoryType;
		MDArray[NumberDescriptors].BasePage				= StartPage;
		MDArray[NumberDescriptors].PageCount			= EndPage - StartPage;
		NumberDescriptors								+= 1;
	}

	return ESUCCESS;
}

//
// carves out a specific memory descriptor from the memory descriptors that have already been created
//
ARC_STATUS MempAllocDescriptor(__in ULONG StartPage,__in ULONG EndPage,__in TYPE_OF_MEMORY MemoryType)
{
	//
	// walk through the memory descriptors until we find one that
	// contains the start of the descriptor.
	//
	ULONG i;
	for (i = 0; i < NumberDescriptors; i ++)
	{
		if( MDArray[i].MemoryType == MemoryFree &&
			MDArray[i].BasePage <= StartPage &&
			MDArray[i].BasePage + MDArray[i].PageCount > StartPage &&
			MDArray[i].BasePage + MDArray[i].PageCount >= EndPage)
		{
			break;
		}
	}

	if(i == NumberDescriptors)
		return ENOMEM;

	if(MDArray[i].BasePage == StartPage)
	{
		if(MDArray[i].BasePage + MDArray[i].PageCount == EndPage)
		{
			//
			// the new descriptor is identical to the existing descriptor.
			// simply change the memory type of the existing descriptor in place.
			//
			MDArray[i].MemoryType						= MemoryType;
		}
		else
		{
			//
			// the new descriptor starts on the same page, but is smaller than the existing descriptor.
			// shrink the existing descriptor by moving its start page up, and create a new descriptor.
			//
			if(NumberDescriptors == MAX_DESCRIPTORS)
				return ENOMEM;

			MDArray[i].BasePage							= EndPage;
			MDArray[i].PageCount						-= (EndPage - StartPage);

			MDArray[NumberDescriptors].BasePage			= StartPage;
			MDArray[NumberDescriptors].PageCount		= EndPage - StartPage;
			MDArray[NumberDescriptors].MemoryType		= MemoryType;
			NumberDescriptors							+= 1;
		}
	}
	else if(MDArray[i].BasePage+MDArray[i].PageCount == EndPage)
	{
		//
		// the new descriptor ends on the same page.
		// shrink the existing by decreasing its page count, and create a new descriptor.
		//
		if(NumberDescriptors == MAX_DESCRIPTORS)
			return ENOMEM;

		MDArray[i].PageCount							= StartPage - MDArray[i].BasePage;

		MDArray[NumberDescriptors].BasePage				= StartPage;
		MDArray[NumberDescriptors].PageCount			= EndPage - StartPage;
		MDArray[NumberDescriptors].MemoryType			= MemoryType;
		NumberDescriptors								+= 1;
	}
	else
	{
		//
		// the new descriptor is in the middle of the existing descriptor.
		// shrink the existing descriptor by decreasing its page count, and create two new descriptors.
		//
		if(NumberDescriptors + 1 >= MAX_DESCRIPTORS)
			return ENOMEM;

		MDArray[NumberDescriptors].BasePage				= EndPage;
		MDArray[NumberDescriptors].PageCount			= MDArray[i].PageCount - EndPage-MDArray[i].BasePage;
		MDArray[NumberDescriptors].MemoryType			= LoaderFree;
		NumberDescriptors								+= 1;

		MDArray[i].PageCount							= StartPage - MDArray[i].BasePage;

		MDArray[NumberDescriptors].BasePage				= StartPage;
		MDArray[NumberDescriptors].PageCount			= EndPage - StartPage;
		MDArray[NumberDescriptors].MemoryType			= MemoryType;
		NumberDescriptors								+= 1;
	}

	BlpTrackUsage(MemoryType,StartPage,EndPage - StartPage);

	return ESUCCESS;
}

//
// this allocates pages from the private heap.
// the memory descriptor for the LoaderMemoryData area is grown to include the returned pages,
// while the memory descriptor for the temporary heap is shrunk by the same amount.
//
// DO NOT call this routine after we have passed control to BlOsLoader
//
PVOID FwAllocateHeapPermanent(__in ULONG NumberPages)
{
	if(FwPermanentHeap + (NumberPages << PAGE_SHIFT) > FwTemporaryHeap)
	{
		//
		// our heaps collide, so we are out of memory
		//
		BlPrint("Out of permanent heap!\r\n");

		while(1);

		return 0;
	}

	//
	// find the memory descriptor which describes the LoaderMemoryData area, so we can grow it to include the just-allocated pages.
	//
	PMEMORY_DESCRIPTOR Descriptor						= MDArray;

	while(Descriptor->MemoryType != LoaderMemoryData)
	{
		Descriptor										+= 1;

		if(Descriptor > MDArray + MAX_DESCRIPTORS)
		{
			BlPrint("ERROR - FwAllocateHeapPermanent couldn't find the\r\n");
			BlPrint("        LoaderMemoryData descriptor!\r\n");
			while(1);

			return 0;
		}
	}

	Descriptor->PageCount								+= NumberPages;

	//
	// we know that the memory descriptor after this one is the firmware temporary heap descriptor.
	// since it is physically contiguous with our LoaderMemoryData block, we remove the pages from its descriptor.
	//
	Descriptor											+= 1;

	Descriptor->PageCount								-= NumberPages;
	Descriptor->BasePage								+= NumberPages;

	PVOID MemoryPointer									= reinterpret_cast<PVOID>(FwPermanentHeap);

	FwPermanentHeap										+= NumberPages << PAGE_SHIFT;

	return MemoryPointer;
}

//
// allocates memory from the "firmware" temporary heap.
//
PVOID FwAllocateHeap(__in ULONG Size)
{
	if(FwTemporaryHeap - FwPermanentHeap < Size && FwDescriptorsValid && BlLoaderBlock)
	{
		//
		// large allocations get their own descriptor so miniports that have huge device extensions don't suck up all of the heap.
		//
		// note that we can only do this while running in "firmware" mode.
		// once we call into the osloader, it sucks all the memory descriptors out of the "firmware" and changes to this list will not show up there.
		//
		// we are looking for a descriptor that is MemoryFree and <16Mb.
		//
		ULONG SizeInPages								= BYTES_TO_PAGES(Size);
		ULONG i;
		for( i = 0; i < NumberDescriptors; i ++)
		{
			if(MDArray[i].MemoryType == MemoryFree && MDArray[i].BasePage <= _16MB_BOGUS && MDArray[i].PageCount >= SizeInPages)
				break;
		}

		if(i < NumberDescriptors)
		{
			ULONG StartPage							= MDArray[i].BasePage + MDArray[i].PageCount - SizeInPages;
			ARC_STATUS Status						= MempAllocDescriptor(StartPage,StartPage + SizeInPages,LoaderFirmwareTemporary);
			if(Status == ESUCCESS)
				return reinterpret_cast<PVOID>(StartPage << PAGE_SHIFT);
		}
	}

	FwTemporaryHeap										-= Size;

	//
	// round down to 16-byte boundary
	//
	FwTemporaryHeap										&= ~((ULONG)0xf);

	if (FwTemporaryHeap < FwPermanentHeap)
	{
		BlPrint("Out of temporary heap!\r\n");
		return 0;
	}

	return reinterpret_cast<PVOID>(FwTemporaryHeap);
}

//
// This routine allocates memory from the firmware pool.
// Note that this memory is NOT under the 1MB line, so it cannot be used for anything that must be accessed from real mode
//
PVOID FwAllocatePool(__in ULONG Size)
{
	//
	// round size up to 16 byte boundary
	//
	ULONG NewSize										= (Size + 15) & ~0xf;
	if(FwPoolStart + NewSize <= FwPoolEnd)
	{
		PVOID Buffer									= reinterpret_cast<PVOID>(FwPoolStart);
		FwPoolStart										+= NewSize;

		return Buffer;
	}

	//
	// we've used up all our pool, try to allocate from the heap.
	//
	return FwAllocateHeap(Size);
}

//
// allocates memory from the "firmware" temporary heap.
// this memory is always allocated on a page boundary, so it can readily be used for temporary page tables
//
PVOID FwAllocateHeapAligned(__in ULONG Size)
{
	FwTemporaryHeap										-= Size;

	//
	// round down to a page boundary
	//
	FwTemporaryHeap										= reinterpret_cast<ULONG>(PAGE_ALIGN(FwTemporaryHeap));

	if(FwTemporaryHeap < FwPermanentHeap)
	{
		BlPrint("Out of temporary heap!\r\n");
		return 0;
	}

	RtlZeroMemory(reinterpret_cast<PVOID>(FwTemporaryHeap),Size);

	return reinterpret_cast<PVOID>(FwTemporaryHeap);
}

//
// allocates the Page Directory and as many Page Tables as are required to identity map the lowest 1Mb of memory and map all of physical memory into KSEG0.
//
ARC_STATUS MempSetupPaging(__in ULONG StartPage,__in ULONG NumberPages)
{
	if(!PDE)
	{
		//
		// this is our first call, so we need to allocate a Page Directory.
		//
		PDE												= static_cast<PHARDWARE_PTE>(FwAllocateHeapPermanent(1));
		if(!PDE)
			return ENOMEM;

		RtlZeroMemory(PDE,PAGE_SIZE);

		//
		// now we map the page directory onto itself at 0xC0000000
		//
		PDE[HYPER_SPACE_ENTRY].PageFrameNumber			= reinterpret_cast<ULONG>(PDE) >> PAGE_SHIFT;
		PDE[HYPER_SPACE_ENTRY].Valid					= 1;
		PDE[HYPER_SPACE_ENTRY].Write					= 1;

		//
		// allocate one page for the HAL to use to map memory.
		// this goes in the very last PDE slot.  (V.A.  0xFFC00000 - 0xFFFFFFFF )
		//
		HalPT											= static_cast<PHARDWARE_PTE>(FwAllocateHeapPermanent(1));
		if(!HalPT)
			return ENOMEM;

		RtlZeroMemory(HalPT,PAGE_SIZE);

		PDE[1023].PageFrameNumber						= reinterpret_cast<ULONG>(HalPT) >> PAGE_SHIFT;
		PDE[1023].Valid									= 1;
		PDE[1023].Write									= 1;
	}

	//
	// all the page tables we use to set up the physical == virtual mapping are marked as FirmwareTemporary.
	// they get blasted as soon as memory management gets initialized, so we don't need them lying around any longer.
	//
	for(ULONG Page = StartPage; Page < StartPage + NumberPages; Page ++)
	{
		PHARDWARE_PTE PhysPageTable;
		PHARDWARE_PTE KsegPageTable;
		ULONG Entry										= Page >> 10;

		if(reinterpret_cast<PULONG>(PDE)[Entry] == 0)
		{
			PhysPageTable								= static_cast<PHARDWARE_PTE>(FwAllocateHeapAligned(PAGE_SIZE));
			if(!PhysPageTable)
				return ENOMEM;

			RtlZeroMemory(PhysPageTable,PAGE_SIZE);

			KsegPageTable								= static_cast<PHARDWARE_PTE>(FwAllocateHeapPermanent(1));
			if(!KsegPageTable)
				return ENOMEM;

			if(Entry > HighestPde)
				HighestPde								= Entry;

			RtlZeroMemory(KsegPageTable,PAGE_SIZE);

			PDE[Entry].PageFrameNumber					= reinterpret_cast<ULONG>(PhysPageTable) >> PAGE_SHIFT;
			PDE[Entry].Valid							= 1;
			PDE[Entry].Write							= 1;

			ULONG Entry1								= Entry + (KSEG0_BASE >> 22);
			PDE[Entry1].PageFrameNumber					= reinterpret_cast<ULONG>(KsegPageTable) >> PAGE_SHIFT;
			PDE[Entry1].Valid							= 1;
			PDE[Entry1].Write							= 1;

			ULONG Entry2								= Entry + (0xe0000000 >> 22);
			PDE[Entry2].PageFrameNumber					= reinterpret_cast<ULONG>(KsegPageTable) >> PAGE_SHIFT;
			PDE[Entry2].Valid							= 1;
			PDE[Entry2].Write							= 1;
		}
		else
		{
			PhysPageTable								= reinterpret_cast<PHARDWARE_PTE>(PDE[Entry].PageFrameNumber << PAGE_SHIFT);
			KsegPageTable								= reinterpret_cast<PHARDWARE_PTE>(PDE[Entry + (KSEG0_BASE >> 22)].PageFrameNumber << PAGE_SHIFT);
		}

		if(!Page)
		{
			PhysPageTable[Page & 0x3ff].PageFrameNumber = Page;
			PhysPageTable[Page & 0x3ff].Valid			= 0;
			PhysPageTable[Page & 0x3ff].Write			= 0;

			KsegPageTable[Page & 0x3ff].PageFrameNumber = Page;
			KsegPageTable[Page & 0x3ff].Valid			= 0;
			KsegPageTable[Page & 0x3ff].Write			= 0;

		}
		else
		{
			PhysPageTable[Page & 0x3ff].PageFrameNumber = Page;
			PhysPageTable[Page & 0x3ff].Valid			= 1;
			PhysPageTable[Page & 0x3ff].Write			= 1;

			KsegPageTable[Page & 0x3ff].PageFrameNumber = Page;
			KsegPageTable[Page & 0x3ff].Valid			= 1;
			KsegPageTable[Page & 0x3ff].Write			= 1;
		}
	}

	return ESUCCESS;
}

//
// sets up the page tables and enables paging
//
ARC_STATUS MempTurnOnPaging()
{
	//
	// walk down the memory descriptor list and call MempSetupPaging for each descriptor in it.
	//
	for(ULONG i = 0; i < NumberDescriptors; i ++)
	{
		if(MDArray[i].BasePage < _16MB)
		{
			ARC_STATUS Status							= MempSetupPaging(MDArray[i].BasePage,MDArray[i].PageCount);
			if(Status != ESUCCESS)
			{
				BlPrint("ERROR - MempSetupPaging(%lx, %lx) failed\r\n",MDArray[i].BasePage,MDArray[i].PageCount);
				return Status;
			}
		}
	}

	//
	// turn on paging
	//
	__asm
	{
		//
		// load physical address of page directory
		//
		mov eax,PDE
		mov cr3,eax

		//
		// enable paging mode
		//
		mov eax,cr0
		or  eax,0x80000000
		mov cr0,eax
	}

	return ESUCCESS;
}

//
// Frees as many Page Tables as are required from KSEG0
//
VOID MempDisablePages()
{
	//
	// cleanup KSEG0.
	// the MM PFN database is an array of entries which track each page of main memory.
	// large enough memory holes will cause this array to be sparse.
	// MM requires enabled PTEs to have entries in the PFN database,so locate any memory hole and remove their PTEs.
	//
	for(ULONG i = 0; i < NumberDescriptors; i ++)
	{
		if(MDArray[i].MemoryType == MemorySpecialMemory || MDArray[i].MemoryType == MemoryFirmwarePermanent)
		{
			ULONG Page									= MDArray[i].BasePage;
			ULONG EndPage								= Page + MDArray[i].PageCount;

			//
			// KSEG0 only maps to 16MB, so clip the high end there
			//
			if(EndPage > _16MB)
				EndPage = _16MB;

			//
			// some PTEs below 1M may need to stay mapped since they may have been put into ABIOS selectors.
			// instead of determining which PTEs they may be, we will leave PTEs below 1M alone.
			// this doesn't cause the PFN any problems since we know there is some memory below then 680K mark and some more memory at the 1M mark.
			// thus there is not a large enough "memory hole" to cause the PFN database to be sparse below 1M.
			//
			// clip starting address to 1MB
			//
			if(Page < _1MB)
				Page = _1MB;

			//
			// for each page in this range make sure it disabled in KSEG0.
			//
			while(Page < EndPage)
			{
				ULONG Entry								= (Page >> 10) + (KSEG0_BASE >> 22);

				if(PDE[Entry].Valid == 1)
				{
					PHARDWARE_PTE KsegPT				= reinterpret_cast<PHARDWARE_PTE>(PDE[Entry].PageFrameNumber << PAGE_SHIFT);

					KsegPT[Page & 0x3ff].PageFrameNumber= 0;
					KsegPT[Page & 0x3ff].Valid			= 0;
					KsegPT[Page & 0x3ff].Write			= 0;
				}

				Page									+= 1;
			}
		}
	}
}

//
// copy su's gdt to our heep
//
ARC_STATUS MempCopyGdt()
{
#pragma pack(push,2)
	static struct
	{
		//
		// lime
		//
		USHORT											Limit;

		//
		// base
		//
		PUCHAR											Base;
	}GdtDef,IdtDef;
#pragma pack(pop)

	//
	// get the current location of the GDT & IDT
	//
	__asm
	{
		sgdt GdtDef;
		sidt IdtDef;
	}

	if(GdtDef.Base + GdtDef.Limit + 1 != IdtDef.Base)
	{
		//
		// just a sanity check to make sure that the IDT immediately follows the GDT. (As set up in data.asm)
		//
		BlPrint("ERROR - GDT and IDT are not contiguous!\r\n");
		BlPrint("GDT - %lx (%x)  IDT - %lx (%x)\r\n",GdtDef.Base, GdtDef.Limit,IdtDef.Base,IdtDef.Limit);

		while (1);
	}

	ULONG NumPages										= BYTES_TO_PAGES(GdtDef.Limit + 1 + IdtDef.Limit + 1);

	PKGDTENTRY NewGdt									= static_cast<PKGDTENTRY>(FwAllocateHeapPermanent(NumPages));
	if(!NewGdt)
		return ENOMEM;

	RtlMoveMemory(NewGdt,GdtDef.Base,NumPages << PAGE_SHIFT);

	GdtDef.Base											= static_cast<PUCHAR>(static_cast<PVOID>(NewGdt));
	IdtDef.Base											= GdtDef.Base + GdtDef.Limit + 1;

	//
	// setup debug interrupt handler
	//
	PKIDTENTRY Idt										= static_cast<PKIDTENTRY>(static_cast<PVOID>(IdtDef.Base));
	Idt[0x01].Offset									= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap01) & 0xffff);
	Idt[0x01].ExtendedOffset							= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap01) >> 0x10);

	//
	// ring0 cs selector
	//
	Idt[0x01].Selector									= 8;

	//
	// interrupt gate + present + ring0
	//
	Idt[0x01].Access									= 0x8e00;

	Idt[0x03].Offset									= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap03) & 0xffff);
	Idt[0x03].ExtendedOffset							= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap03) >> 0x10);
	Idt[0x03].Selector									= 8;
	Idt[0x03].Access									= 0x8e00;

	Idt[0x0d].Offset									= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap0d) & 0xffff);
	Idt[0x0d].ExtendedOffset							= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap0d) >> 0x10);
	Idt[0x0d].Selector									= 8;
	Idt[0x0d].Access									= 0x8e00;

	Idt[0x0e].Offset									= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap0e) & 0xffff);
	Idt[0x0e].ExtendedOffset							= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap0e) >> 0x10);
	Idt[0x0e].Selector									= 8;
	Idt[0x0e].Access									= 0x8e00;

	Idt[0x2d].Offset									= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap2d) & 0xffff);
	Idt[0x2d].ExtendedOffset							= static_cast<USHORT>(reinterpret_cast<ULONG>(&BdTrap2d) >> 0x10);
	Idt[0x2d].Selector									= 8;
	Idt[0x2d].Access									= 0x8e00;

	__asm
	{
		lgdt GdtDef;
		lidt IdtDef;
	}

	//
	// su did not copy file headers to image base,but those headers is needed by debugger to correctly handle symbols
	// so copy it here
	//
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(reinterpret_cast<PVOID>(OsLoaderBase));
	if(!NtHeaders)
	{
		//
		// HACK HACK HACK,we known SU is loaded below 1MB and ourself is appended at the end of SU,and external services is in the SU module
		// so search from external services to 1MB to find image nt header
		// it's ugly but works
		//
		ULONG DosHeaders								= reinterpret_cast<ULONG>(ExternalServicesTable);
		for( ; DosHeaders < 1 * 1024 * 1024; DosHeaders ++)
		{
			NtHeaders									= RtlImageNtHeader(reinterpret_cast<PVOID>(DosHeaders));
			if(NtHeaders)
				break;
		}

		//
		// nt header found,copy it
		//
		if(NtHeaders)
		{
			RtlCopyMemory(reinterpret_cast<PVOID>(OsLoaderBase),reinterpret_cast<PVOID>(DosHeaders),NtHeaders->OptionalHeader.SizeOfHeaders);
			NtHeaders									= RtlImageNtHeader(reinterpret_cast<PVOID>(OsLoaderBase));
		}
	}

	if(NtHeaders)
	{
		//
		// copy cmd line
		//
		PCHAR CmdLine									= 0;
		ULONG CmdLineLength								= 0;

		//
		// hope the e_res field is initialized with 0
		//
		RtlCopyMemory(&CmdLine,Add2Ptr(OsLoaderBase,FIELD_OFFSET(IMAGE_DOS_HEADER,e_res),PVOID),sizeof(CmdLine));
		RtlCopyMemory(&CmdLineLength,Add2Ptr(OsLoaderBase,FIELD_OFFSET(IMAGE_DOS_HEADER,e_res) + sizeof(CmdLine),PVOID),sizeof(CmdLineLength));

		//
		// check and copy cmd line
		//
		if(CmdLine && Add2Ptr(CmdLine,CmdLineLength,ULONG) < 1 * 1024 * 1024 && CmdLineLength < sizeof(BlSuCmdLine))
		{
			for(ULONG i = 0; i < CmdLineLength && CmdLine[i]; i ++)
			{
				CHAR ch									= CmdLine[i];
				if(!isascii(ch))
				{
					//
					// invalid charaters
					//
					BlSuCmdLine[0]						= 0;
					break;
				}

				BlSuCmdLine[i]							= ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch;
			}
		}

		//
		// reset e_res to zero
		//
		RtlZeroMemory(Add2Ptr(OsLoaderBase,FIELD_OFFSET(IMAGE_DOS_HEADER,e_res),PVOID),sizeof(CmdLine) + sizeof(CmdLineLength));
	}

	//
	// init debugger first phase
	//
	BdInitDebugger("osloader.exe",OsLoaderBase,BlSuCmdLine);

	return ESUCCESS;
}

//
// initialize memory sub system
//
ARC_STATUS InitializeMemorySubsystem(__in PBOOT_CONTEXT BootContext)
{
	ARC_STATUS Status;

	//
	// default rom start
	//
	ULONG RomStart										= ROM_START_PAGE;

	//
	// start by creating memory descriptors to describe all of the memory we know about.
	// then setup the page tables.
	// finally, allocate descriptors that describe our memory layout.
	//

	//
	// we know that one of the SU descriptors is for < 1Mb, and we don't care about that,
	// since we know everything we'll run on will have at least 1Mb of memory.
	// the rest are for extended memory, and those are the ones we are interested in.
	//
	PSU_MEMORY_DESCRIPTOR SuMemory						= BootContext->MemoryDescriptorList;
	while(SuMemory->BlockSize)
	{
		ULONG BAddr										= SuMemory->BlockBase;
		ULONG EAddr										= BAddr + SuMemory->BlockSize - 1;

		//
		// round the starting address to a page boundry.
		//
		ULONG BRound									= BYTE_OFFSET(BAddr);
		if(BRound)
			BAddr										= BAddr + PAGE_SIZE - BRound;

		//
		// round the ending address to a page boundry minus 1
		//
		ULONG ERound									= BYTE_OFFSET(EAddr + 1);
		if(ERound)
			EAddr										-= ERound;

		//
		// Covert begining & ending address to page
		//
		ULONG PageStart									= BAddr >> PAGE_SHIFT;
		ULONG PageEnd									= (EAddr + 1) >> PAGE_SHIFT;

		//
		// if this memory descriptor describes conventional ( <640k ) memory, then assume the ROM starts immediately after it ends.
		//
		if(PageStart == 0)
			RomStart									= PageEnd;

		//
		// if PageStart was rounded up to a page boundry, then add the fractional page as SpecialMemory
		//
		if(BRound)
		{
			Status										= MempSetDescriptorRegion(PageStart - 1,PageStart,MemorySpecialMemory);
			if(Status != ESUCCESS)
				break;
		}

		//
		// if PageEnd was rounded down to a page boundry, then add the fractional page as SpecialMemory
		//
		if(ERound)
		{
			Status										= MempSetDescriptorRegion(PageEnd,PageEnd + 1,MemorySpecialMemory);
			if(Status != ESUCCESS)
				break;

			//
			// romStart starts after the reserved page
			//
			if(RomStart == PageEnd)
				RomStart								+= 1;
		}

		//
		// add memory range PageStart though PageEnd
		//
		if(PageEnd <= _16MB_BOGUS)
		{
			//
			// this memory descriptor is all below the 16MB_BOGUS mark
			//
			Status										= MempSetDescriptorRegion(PageStart,PageEnd,MemoryFree);
		}
		else if(PageStart >= _16MB)
		{
			//
			// this memory descriptor is all above the 16MB mark.
			// we never use memory above 16Mb in the loader environment, mainly so we don't have to worry about DMA transfers from ISA cards.
			//
			// memory above 16MB is not used by the loader, so it's flagged as FirmwareTemporary
			//
			Status										= MempSetDescriptorRegion(PageStart,PageEnd,LoaderReserved);
		}
		else
		{
			//
			// this memory descriptor describes memory within the last 40h pages of the 16MB mark - otherwise known as 16MB_BOGUS.
			//
			if(PageStart < _16MB_BOGUS)
			{
				//
				// clip starting address to 16MB_BOGUS mark, and add memory below 16MB_BOGUS as useable memory.
				//
				Status									= MempSetDescriptorRegion(PageStart,_16MB_BOGUS,MemoryFree);
				if(Status != ESUCCESS)
					break;

				PageStart								= _16MB_BOGUS;
			}

			//
			// add remaining memory as FirmwareTemporary.  Memory above 16MB is never used within the loader.
			// the bogus range will be reset later on.
			//
			Status										= MempSetDescriptorRegion(PageStart,PageEnd,LoaderReserved);
		}

		if(Status != ESUCCESS)
			break;

		//
		// move to the next memory descriptor
		//
		SuMemory										+= 1;
	}

	if(Status != ESUCCESS)
	{
		BlPrint("MempSetDescriptorRegion failed %lx\r\n",Status);
		return Status;
	}

	//
	// set the range 16MB_BOGUS - 16MB as unusable
	//
	Status												= MempSetDescriptorRegion(_16MB_BOGUS,_16MB,MemorySpecialMemory);
	if(Status != ESUCCESS)
		return Status;

	//
	// hack for EISA machines that insist there is usable memory in the ROM area, where we know darn well there isn't.
	//

	//
	// remove anything in this range..
	//
	MempSetDescriptorRegion(ROM_START_PAGE, ROM_END_PAGE,LoaderMaximum);

	//
	// describe the BIOS area
	//
	MempSetDescriptorRegion(RomStart,ROM_END_PAGE,MemoryFirmwarePermanent);

	//
	// ramdisk
	//
	if(BootContext->FSContextPointer->BootDrive == 0x40)
	{
		ULONG MemorySizeKB								= (static_cast<ULONG>(*reinterpret_cast<PUSHORT>(0x413)) << 10) >> PAGE_SHIFT;
		if(MemorySizeKB < RomStart)
			MempSetDescriptorRegion(MemorySizeKB,RomStart,MemoryFirmwareTemporary);
	}

	//
	// now we have descriptors that map all of physical memory.
	// carve out descriptors from these that describe the parts that we are currently using.
	//

	//
	// create the descriptors which describe the low 1Mb of memory.
	//

	//
	// 00000 - 00fff  real-mode interrupt vectors
	//
	Status												= MempAllocDescriptor(0,1,MemoryFirmwarePermanent);
	if(Status != ESUCCESS)
		return Status;

	//
	// 01000 - 1ffff  loadable miniport drivers, free memory.
	//	ntdetect.com start at 10000
	//
	Status												= MempAllocDescriptor(1,0x20,MemoryFree);
	if(Status != ESUCCESS)
		return Status;

	//
	// 20000 - 2ffff  SU module, SU stack
	// 30000 - 38ffff firmware cache
	//
	Status												= MempAllocDescriptor(0x20,PERMANENT_HEAP_START,MemoryFirmwareTemporary);
	if(Status != ESUCCESS)
		return Status;

	//
	// 39000 - 39000  firmware permanent
	//  this starts out as zero-length.
	//	it grows into the firmware temporary heap descriptor as we allocate permanent pages for the Page Directory and Page Tables
	//
	Status												= MempAllocDescriptor(PERMANENT_HEAP_START,PERMANENT_HEAP_START,LoaderMemoryData);
	if(Status != ESUCCESS)
		return Status;

	//
	// 39000 - 4ffff  firmware temporary heap
	// 50000 - 5ffff  firmware configuration tree
	//
	Status												= MempAllocDescriptor(PERMANENT_HEAP_START,OSLOADER_STACK_START,MemoryFirmwareTemporary);
	if(Status != ESUCCESS)
		return Status;

	//
	// 60000 - 61ffff stack we are currently running on.
	//
	Status												= MempAllocDescriptor(OSLOADER_STACK_START,OSLOADER_STACK_END,MemoryFirmwareTemporary);
	if(Status != ESUCCESS)
		return Status;

	//
	// describe the osloader memory image
	//
	ULONG LoaderBase									= BootContext->OsLoaderBase >> PAGE_SHIFT;
	ULONG LoaderEnd										= BYTES_TO_PAGES(BootContext->OsLoaderEnd);
	Status												= MempAllocDescriptor(LoaderBase,LoaderEnd,MemoryLoadedProgram);
	if(Status != ESUCCESS)
		return Status;

	//
	// describe the memory pool used to allocate memory for the SCSI miniports.
	//
	Status												= MempAllocDescriptor(LoaderEnd,LoaderEnd + 0x60,MemoryFirmwareTemporary);
	if(Status != ESUCCESS)
		return Status;

	FwPoolStart											= LoaderEnd << PAGE_SHIFT;
	FwPoolEnd											= FwPoolStart + (0x60 << PAGE_SHIFT);

	//
	// HACKHACK - try to mark a page just below the osloader as firmwaretemp, so it will not get used for heap/stack.
	// this is to force our heap/stack to be < 1Mb.
	// see BlInitializeMemory for detail
	//
	MempAllocDescriptor((BootContext->OsLoaderBase >> PAGE_SHIFT) - 1,BootContext->OsLoaderBase >> PAGE_SHIFT,MemoryFirmwareTemporary);

	//
	// turn on paging
	//
	Status = MempTurnOnPaging();
	if(Status != ESUCCESS)
		return Status;

	//
	// copy old gdt to new location
	//
	return MempCopyGdt();
}

//
// initialize memory descriptors
//
VOID InitializeMemoryDescriptors()
{
	E820FRAME       Frame;

	//
	// first key
	//
	Frame.Key											= 0;
	do
	{
		//
		// setup size
		//
		Frame.Size										= sizeof(Frame.Descriptor);

		//
		// get frame
		//
		ExternalServicesTable->GetMemoryDescriptor(&Frame);

		//
		// break if we got an error status
		//
		if(Frame.ErrorFlag  ||  Frame.Size < sizeof (Frame.Descriptor))
			break;

		ULONGLONG Start									= Frame.Descriptor.BaseAddress.QuadPart;
		ULONGLONG End									= Frame.Descriptor.Size.QuadPart + Start - 1;

		//
		// based upon the address range descriptor type, find the available memory and add it to the descriptor list
		//
		switch(Frame.Descriptor.MemoryType)
		{
		case 1:
			{
				//
				// this is a memory descriptor - it's already been handled by su (eisac.c)
				//
				// however, any memory within 16MB_BOGUS - 16MB was considered unuseable.
				// reclaim memory within this region which is described via this interface.
				//
				ULONG StartPage							= static_cast<ULONG>((Start + PAGE_SIZE - 1) >> PAGE_SHIFT);
				ULONG EndPage							= static_cast<ULONG>(End >> PAGE_SHIFT) + 1;

				//
				// clip to bogus range
				//
				if(StartPage < _16MB_BOGUS  &&  EndPage >= _16MB_BOGUS)
					StartPage							= _16MB_BOGUS;

				if(EndPage > (_16MB - 1) &&  StartPage <= _16MB - 1)
					EndPage								= _16MB - 1;

				//
				// reclaim memory within the bogus range by setting it to FirmwareTemporary
				//
				if(StartPage >= _16MB_BOGUS  &&  EndPage <= _16MB - 1)
					MempSetDescriptorRegion(static_cast<ULONG>(StartPage),static_cast<ULONG>(EndPage),MemoryFirmwareTemporary);

				StartPage								= static_cast<ULONG>((Start + PAGE_SIZE - 1) >> PAGE_SHIFT);
				EndPage									= static_cast<ULONG>(End >> PAGE_SHIFT) + 1;

				if(EndPage >= _4GB)
				{
					if(StartPage <= _4GB)
						StartPage						= _4GB;

					MempSetDescriptorRegion(StartPage,EndPage,MemoryFirmwareTemporary);
				}
			}
			break;

			//
			// unkown types are treated as Reserved
			//
		default:
			{
				//
				// this memory descriptor is a reserved address range
				//
				MempSetDescriptorRegion(static_cast<ULONG>(Start >> PAGE_SHIFT),static_cast<ULONG>((End + PAGE_SIZE) >> PAGE_SHIFT),MemorySpecialMemory);
			}
			break;
		}
	}while(Frame.Key);

	//
	// disable pages from KSEG0 which are disabled
	//
	MempDisablePages();
}

//
// map io space
//
PVOID MmMapIoSpace(__in PHYSICAL_ADDRESS PhysicalAddress,__in ULONG NumberOfBytes,__in MEMORY_CACHING_TYPE CacheType)
{
	ULONG NumberPages									= BYTES_TO_PAGES(NumberOfBytes);

	//
	// we use the HAL's PDE for mapping memory buffers,find enough free PTEs.
	//
	for(ULONG i = 0; i <= 1024 - NumberPages; i ++)
	{
		ULONG j;
		for(j = 0; j < NumberPages; j ++)
		{
			if(Add2Ptr(HalPT,0,PULONG)[i+j])
				break;
		}

		if(j == NumberPages)
		{
			for(j = 0; j < NumberPages; j ++)
			{
				HalPT[i + j].PageFrameNumber			= (PhysicalAddress.LowPart >> PAGE_SHIFT) + j;
				HalPT[i + j].Valid						= 1;
				HalPT[i + j].Write						= 1;
				HalPT[i + j].WriteThrough				= 1;

				if(CacheType == MmNonCached)
					HalPT[i + j].CacheDisable			= 1;
			}

			return Add2Ptr(HAL_PAGE_BASE | (i << 12) | (PhysicalAddress.LowPart & 0xfff),0,PVOID);
		}
	}
	return	0;
}

//
// check memory mapped
//
ARC_STATUS MempCheckMapping(__in ULONG BasePage,__in ULONG PageCount)
{
	//
	// < 16MB memory is always mapped
	//
	if(BasePage < _16MB)
		return ESUCCESS;

	ULONG LastPage										= BasePage + PageCount;

	for(ULONG i = BasePage; i < LastPage; i ++)
	{
		//
		// first check it
		//
		ULONG PdeIndex									= (i >> 10);
		PHARDWARE_PTE KernelPTE							= 0;
		PHARDWARE_PTE PhysicalPTE						= 0;

		if(!PDE[PdeIndex].Valid)
		{
			//
			// allocate three pages one for direct map,one for kernel map,the other is used for alignment
			//
			PVOID Pages									= BlAllocateHeapAligned(PAGE_SIZE * 3);
			if(!Pages)
				return ENOMEM;

			PVOID AlignedPages							= reinterpret_cast<PVOID>(ROUND_TO_PAGES(Pages));
			RtlZeroMemory(AlignedPages,PAGE_SIZE * 2);

			//
			// direct map
			//
			PDE[PdeIndex].Valid							= TRUE;
			PDE[PdeIndex].Write							= TRUE;
			PDE[PdeIndex].PageFrameNumber				= GET_PHYSICAL_PAGE(AlignedPages);

			//
			// kernel map
			//
			PDE[PdeIndex + 512].Valid					= TRUE;
			PDE[PdeIndex + 512].Write					= TRUE;
			PDE[PdeIndex + 512].PageFrameNumber			= GET_PHYSICAL_PAGE(Add2Ptr(AlignedPages,PAGE_SIZE,ULONG));

			//
			// for virtual bias
			//
			if(BlVirtualBias)
			{
				ULONG Index								= PdeIndex + (BlVirtualBias >> 22);
				PDE[Index].Valid						= TRUE;
				PDE[Index].Write						= TRUE;
				PDE[Index].PageFrameNumber				= GET_PHYSICAL_PAGE(Add2Ptr(AlignedPages,PAGE_SIZE,ULONG));
			}

			if(PdeIndex > HighestPde)
				HighestPde								= PdeIndex;

			PhysicalPTE									= static_cast<PHARDWARE_PTE>(AlignedPages);
			KernelPTE									= Add2Ptr(AlignedPages,PAGE_SIZE,PHARDWARE_PTE);
		}
		else
		{
			//
			// already mapped,read kernel pte and physical pte
			//
			KernelPTE									= reinterpret_cast<PHARDWARE_PTE>(PDE[PdeIndex + 512].PageFrameNumber << PAGE_SHIFT);
			PhysicalPTE									= reinterpret_cast<PHARDWARE_PTE>(PDE[PdeIndex].PageFrameNumber << PAGE_SHIFT);
		}

		//
		// skip the first page which contain real mode interrupt vector and bios data
		//
		if(i)
		{
			ULONG PTEIndex								= i & 0x3ff;

			PhysicalPTE[PTEIndex].Valid					= TRUE;
			PhysicalPTE[PTEIndex].Write					= TRUE;
			PhysicalPTE[PTEIndex].PageFrameNumber		= i;

			KernelPTE[PTEIndex].Valid					= TRUE;
			KernelPTE[PTEIndex].Write					= TRUE;
			KernelPTE[PTEIndex].PageFrameNumber			= i;
		}
	}

	//
	// reload page table and flush TLB
	//
	__asm
	{
		mov		eax,cr3
		mov		cr3,eax
	}

	return ESUCCESS;
}

//
// enable disable access to physical page 0
//
ARC_STATUS MempSetPageZeroOverride(__in BOOLEAN Enable)
{
	//
	// make sure virtual address 0x00000000 or 0x800000000 map to physical address 0x00000000
	//
	if(!PDE[0].Valid)
		return ENOMEM;

	if( !reinterpret_cast<PHARDWARE_PTE>(PDE[0].PageFrameNumber << PAGE_SHIFT)->PageFrameNumber ||
		!reinterpret_cast<PHARDWARE_PTE>(PDE[512].PageFrameNumber << PAGE_SHIFT)->PageFrameNumber)
	{
		reinterpret_cast<PHARDWARE_PTE>(PDE[0].PageFrameNumber << PAGE_SHIFT)->Valid	= Enable;
		reinterpret_cast<PHARDWARE_PTE>(PDE[512].PageFrameNumber << PAGE_SHIFT)->Valid	= Enable;

		return ESUCCESS;
	}

	return ENOMEM;
}

//
// insert a descriptor into allocation list
//
VOID BlInsertDescriptor(__in PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
	//
	// find the first descriptor in the list that starts above the new descriptor.
	// the new descriptor goes in front of this descriptor.
	//
	PLIST_ENTRY ListHead								= &BlLoaderBlock->MemoryDescriptorListHead;
	PLIST_ENTRY PreviousEntry							= ListHead;
	PLIST_ENTRY NextEntry								= ListHead->Flink;
	PMEMORY_ALLOCATION_DESCRIPTOR PreviousDescriptor	= 0;
	PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor		= 0;

	while(NextEntry != ListHead)
	{
		NextDescriptor									= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);
		if(NewDescriptor->BasePage < NextDescriptor->BasePage)
			break;

		PreviousEntry									= NextEntry;
		PreviousDescriptor								= NextDescriptor;
		NextEntry										= NextEntry->Flink;
	}

	//
	// if the new descriptor doesn't describe free memory, just insert it in the list in front of the previous entry.
	// otherwise, check to see if free blocks can be merged.
	//
	if (NewDescriptor->MemoryType != LoaderFree)
	{
		InsertHeadList(PreviousEntry,&NewDescriptor->ListEntry);
	}
	else
	{
		//
		// if the previous block also describes free memory, and it's contiguous with the new block, merge them by adding the page count from the new
		//
		if( PreviousEntry != ListHead &&
			PreviousDescriptor->MemoryType == LoaderFree &&
			PreviousDescriptor->BasePage + PreviousDescriptor->PageCount == NewDescriptor->BasePage)
		{
			PreviousDescriptor->PageCount				+= NewDescriptor->PageCount;
			NewDescriptor								= PreviousDescriptor;
		}
		else
		{
			InsertHeadList(PreviousEntry,&NewDescriptor->ListEntry);
		}

		if(NextEntry != ListHead && NextDescriptor->MemoryType == LoaderFree && NewDescriptor->BasePage + NewDescriptor->PageCount == NextDescriptor->BasePage)
		{
			NewDescriptor->PageCount					+= NextDescriptor->PageCount;
			RemoveEntryList(&NextDescriptor->ListEntry);
		}
	}
}

//
// alloacte heap,return buffer is round up to a cache line boundary
//
PVOID BlAllocateHeapAligned(__in ULONG Size)
{
	PVOID Buffer										= BlAllocateHeap(Size + BlDcacheFillSize - 1);

	//
	// round up to a cache line boundary
	//
	if(Buffer)
		Buffer											= ALIGN_BUFFER(Buffer);

	return Buffer;
}

//
// allocate from heap
//
PVOID BlAllocateHeap(__in ULONG Size)
{
	//
	// round size up to next allocation boundary and attempt to allocate a block of the requested size.
	//
	Size												= (Size + (BL_GRANULARITY - 1)) & (~(BL_GRANULARITY - 1));
	ULONG Block											= BlHeapFree;

	if(BlHeapFree + Size <= BlHeapLimit)
	{
		BlHeapFree										+= Size;
		return reinterpret_cast<PVOID>(Block);
	}

#if 0
	TotalHeapAbandoned									+= (BlHeapLimit - BlHeapFree);
	BlLogPrint(LOG_ALL_W,"ABANDONING %d bytes of heap; total abandoned %d\n",(BlHeapLimit - BlHeapFree),TotalHeapAbandoned);
#endif

	//
	// our heap is full.BlHeapLimit always reserves enough space for one more MEMORY_ALLOCATION_DESCRIPTOR,
	// so use that to go try and find more free memory we can use.
	//
	PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor	= reinterpret_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlHeapLimit);

	//
	// attempt to find a free memory descriptor big enough to hold this allocation or BL_HEAP_PAGES, whichever is bigger.
	//
	PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor		= 0;
	ULONG NewHeapPages									= BYTES_TO_PAGES(Size + sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
	if(NewHeapPages < BL_HEAP_PAGES)
		NewHeapPages = BL_HEAP_PAGES;

	//
	// set policy
	//
	BlHeapAllocationPolicy								= BlOldKernel || !BlVirtualBias ? BlAllocateLowestFit : BlAllocateHighestFit;

	do
	{
		PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor	= 0;
		PLIST_ENTRY NextEntry							= BlLoaderBlock->MemoryDescriptorListHead.Flink;

		while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
		{
			NextDescriptor								= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

			if(NextDescriptor->MemoryType == LoaderFree && NextDescriptor->PageCount >= NewHeapPages)
			{
				//
				// if the allocation policy is LowestFit, take this block (the memory list is sorted).
				// otherwise, if this block best meets the allocation policy, remember it and keep looking.
				//
				if(BlHeapAllocationPolicy == BlAllocateLowestFit)
				{
					FreeDescriptor						= NextDescriptor;
					break;
				}

				if(!FreeDescriptor || BlHeapAllocationPolicy == BlAllocateHighestFit || (FreeDescriptor && NextDescriptor->PageCount < FreeDescriptor->PageCount))
					FreeDescriptor = NextDescriptor;
			}

			NextEntry									= NextEntry->Flink;
		}

		//
		// if we were unable to find a block of the desired size, memory must be getting tight, so try again, this time looking just enough to keep us going.
		// (The first time through, we try to allocate at least BL_HEAP_PAGES.)
		//
		if(FreeDescriptor)
			break;

		ULONG LastAttempt								= NewHeapPages;
		NewHeapPages									= BYTES_TO_PAGES(Size + sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
		if(NewHeapPages == LastAttempt)
			break;

	}while(1);

	//
	// no free memory left.
	//
	if(!FreeDescriptor)
		return 0;

	//
	// we've found a descriptor that's big enough.just carve a piece off the end and use that for our heap.
	// if we're taking all of the memory from the descriptor, remove it from the memory list.
	// (This wastes a descriptor, but that's life.)
	//
	FreeDescriptor->PageCount							-= NewHeapPages;
	if(!FreeDescriptor->PageCount)
		RemoveEntryList(&FreeDescriptor->ListEntry);

	//
	// initialize our new descriptor and add it to the list.
	//
	AllocationDescriptor->MemoryType					= LoaderOsloaderHeap;
	AllocationDescriptor->BasePage						= FreeDescriptor->BasePage + FreeDescriptor->PageCount;
	AllocationDescriptor->PageCount						= NewHeapPages;

	BlpTrackUsage(LoaderOsloaderHeap,AllocationDescriptor->BasePage,NewHeapPages);

	BlInsertDescriptor(AllocationDescriptor);

	//
	// initialize new heap values and return pointer to newly alloc'd memory.
	//
	BlHeapFree											= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS_PAGE(AllocationDescriptor->BasePage));
	BlHeapLimit											= BlHeapFree + (NewHeapPages << PAGE_SHIFT) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

	RtlZeroMemory(reinterpret_cast<PVOID>(BlHeapFree),NewHeapPages << PAGE_SHIFT);

	Block												= BlHeapFree;
	if(BlHeapFree + Size < BlHeapLimit)
	{
		BlHeapFree										+= Size;
		return reinterpret_cast<PVOID>(Block);
	}

	return 0;
}

//
// allocates a new heap block
//
VOID BlGenerateNewHeap(__in PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,__in ULONG BasePage,__in ULONG PageCount)
{
	//
	// BlHeapLimit always reserves enough space for one more MEMORY_ALLOCATION_DESCRIPTOR, so use that to describe the new heap block.
	//
	PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor	= reinterpret_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlHeapLimit);

	//
	// allocate the new heap from either the front or the back of the specified descriptor, whichever fits best.
	// we'd like to allocate BL_HEAP_PAGES pages, but we'll settle for less.
	//
	ULONG AvailableAtFront								= BasePage - MemoryDescriptor->BasePage;
	ULONG AvailableAtBack								= MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - BasePage + PageCount;
	ULONG NewHeapPages									= 0;

	if(!AvailableAtFront || (AvailableAtBack && AvailableAtBack < AvailableAtFront))
	{
		NewHeapPages									= min(AvailableAtBack,BL_HEAP_PAGES);
		AllocationDescriptor->BasePage					= MemoryDescriptor->BasePage + MemoryDescriptor->PageCount - NewHeapPages;
	}
	else
	{
		NewHeapPages									= min(AvailableAtFront,BL_HEAP_PAGES);
		AllocationDescriptor->BasePage					= MemoryDescriptor->BasePage;
		MemoryDescriptor->BasePage						+= NewHeapPages;
	}

	MemoryDescriptor->PageCount							-= NewHeapPages;

	//
	// initialize our new descriptor and add it to the list.
	//
	AllocationDescriptor->MemoryType					= LoaderOsloaderHeap;
	AllocationDescriptor->PageCount						= NewHeapPages;

	BlInsertDescriptor(AllocationDescriptor);

	BlpTrackUsage(LoaderOsloaderHeap,AllocationDescriptor->BasePage,AllocationDescriptor->PageCount);

	//
	// initialize new heap values.
	//
	BlHeapFree											= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS_PAGE(AllocationDescriptor->BasePage));
	BlHeapLimit											= BlHeapFree + (NewHeapPages << PAGE_SHIFT) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

	RtlZeroMemory(reinterpret_cast<PVOID>(BlHeapFree),NewHeapPages << PAGE_SHIFT);
}

//
// find allocation descriptor
//
PMEMORY_ALLOCATION_DESCRIPTOR BlFindMemoryDescriptor(__in ULONG BasePage)
{
	PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor		= 0;
	PLIST_ENTRY NextEntry								= BlLoaderBlock->MemoryDescriptorListHead.Flink;

	while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
	{
		MemoryDescriptor								= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		if(MemoryDescriptor->BasePage <= BasePage && MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > BasePage)
			break;

		NextEntry										= NextEntry->Flink;
	}

	if(NextEntry == &BlLoaderBlock->MemoryDescriptorListHead)
		return 0;

	return MemoryDescriptor;
}

//
// build memory allocation descriptor
//
ARC_STATUS BlGenerateDescriptor(__in PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,__in MEMORY_TYPE MemoryType,__in ULONG BasePage,__in ULONG PageCount)
{
	//
	// if the specified region totally consumes the free region, then no additional descriptors need to be allocated.
	// if the specified region is at the start or end of the free region, then only one descriptor needs to be allocated.
	// otherwise, two additional descriptors need to be allocated.
	//
	LONG Offset											= BasePage - MemoryDescriptor->BasePage;

	if(!Offset && PageCount == MemoryDescriptor->PageCount)
	{
		//
		// the specified region totally consumes the free region.
		//
		MemoryDescriptor->MemoryType					= MemoryType;
	}
	else
	{
		//
		// mark the entire given memory descriptor as in use.
		// if we are out of heap, BlAllocateHeap will search for a new descriptor to grow the heap
		// and this prevents both routines from trying to use the same descriptor.
		//
		TYPE_OF_MEMORY OldType							= MemoryDescriptor->MemoryType;
		MemoryDescriptor->MemoryType					= LoaderSpecialMemory;

		//
		// a memory descriptor must be generated to describe the allocated memory.
		//
		BOOLEAN SecondDescriptorNeeded					= BasePage != MemoryDescriptor->BasePage && Offset + PageCount != MemoryDescriptor->PageCount;

		PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor1	= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));

		//
		// if allocation of the first additional memory descriptor failed, then generate new heap using the block from which we are allocating.
		// this can only be done if the block is free.
		// BlGenerateNewHeap cannot fail, because we know there is at least one more page in the block than we want to take from it.
		// the allocation following BlGenerateNewHeap is guaranteed to succeed.
		//
		if(!NewDescriptor1)
		{
			if(OldType != LoaderFree)
			{
				MemoryDescriptor->MemoryType			= OldType;

				return ENOMEM;
			}

			BlGenerateNewHeap(MemoryDescriptor,BasePage,PageCount);
			NewDescriptor1								= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));
			Offset										-= MemoryDescriptor->BasePage;
		}

		//
		// if a second descriptor is needed, allocate it.as above, if the allocation fails, generate new heap using our block.
		//
		PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor2	= 0;
		if(SecondDescriptorNeeded)
		{
			NewDescriptor2								= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));
			if(!NewDescriptor2)
			{
				if(OldType != LoaderFree)
				{
					MemoryDescriptor->MemoryType		= OldType;
					return ENOMEM;
				}

				BlGenerateNewHeap(MemoryDescriptor,BasePage,PageCount);
				NewDescriptor2							= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));
				Offset									-= MemoryDescriptor->BasePage;
			}
		}

		NewDescriptor1->MemoryType						= MemoryType;
		NewDescriptor1->BasePage						= BasePage;
		NewDescriptor1->PageCount						= PageCount;

		if(BasePage == MemoryDescriptor->BasePage)
		{
			//
			// the specified region lies at the start of the free region.
			//
			MemoryDescriptor->BasePage					+= PageCount;
			MemoryDescriptor->PageCount					-= PageCount;
			MemoryDescriptor->MemoryType				= OldType;
		}
		else if(static_cast<ULONG>(Offset + PageCount) == MemoryDescriptor->PageCount)
		{
			//
			// the specified region lies at the end of the free region.
			//
			MemoryDescriptor->PageCount					-= PageCount;
			MemoryDescriptor->MemoryType				= OldType;
		}
		else
		{
			//
			// the specified region lies in the middle of the free region.
			//
			NewDescriptor2->MemoryType					= LoaderFree;
			NewDescriptor2->BasePage					= BasePage + PageCount;
			NewDescriptor2->PageCount					= MemoryDescriptor->PageCount - (PageCount + Offset);

			MemoryDescriptor->PageCount					= Offset;
			MemoryDescriptor->MemoryType				= OldType;

			BlInsertDescriptor(NewDescriptor2);
		}

		BlInsertDescriptor(NewDescriptor1);
	}

	BlpTrackUsage(MemoryType,BasePage,PageCount);

	return ESUCCESS;
}

ARC_STATUS BlpMarkExtendedVideoRegionOffLimits()
{
	ARC_STATUS Status									= MempSetPageZeroOverride(TRUE);
	if(Status != ESUCCESS)
		return Status;

	ULONG Offset744										= *reinterpret_cast<PULONG>(0x744);
	ULONG Offset740										= *reinterpret_cast<PULONG>(0x740);

	Status												= MempSetPageZeroOverride(FALSE);
	if(Status != ESUCCESS)
		return Status;

	if(!Offset740 || !Offset744 || !BlLoaderBlock)
		return ESUCCESS;

	ULONG Count											= ((Offset744 + Offset740) >> PAGE_SHIFT) - (Offset740 >> PAGE_SHIFT) + 1;
	for(ULONG i = Offset740 >> PAGE_SHIFT; i < Count; )
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Desc				= BlFindMemoryDescriptor(i);
		if(!Desc)
			break;

		ULONG PagesCount								= Count;
		if(i + Count > Desc->BasePage + Desc->PageCount)
			PagesCount									= Desc->BasePage + Desc->PageCount - i;

		BlGenerateDescriptor(Desc,LoaderFirmwarePermanent,i,PagesCount);

		i												+= PagesCount;
	}

	return ESUCCESS;
}

//
// initialize loader block,loader heap and loader stack
//
ARC_STATUS BlMemoryInitialize()
{
	//
	// find the memory descriptor that describes the allocation for the OS loader itself.
	//
	PMEMORY_DESCRIPTOR ProgramDescriptor				= 0;
	while(ProgramDescriptor = ArcGetMemoryDescriptor(ProgramDescriptor))
	{
		if(ProgramDescriptor->MemoryType == MemoryLoadedProgram)
			break;
	}

	//
	// if a loaded program memory descriptor was found, then it must be for the OS loader since that is the only program that can be loaded.
	// if a loaded program memory descriptor was not found, then firmware is not functioning properly and an unsuccessful status is returned.
	//
	if(!ProgramDescriptor)
		return ENOMEM;

	//
	// find the free memory descriptor that is just below the loaded program in memory.
	// there should be several megabytes of free memory just preceeding the OS loader.
	//
	PMEMORY_DESCRIPTOR HeapDescriptor					= 0;
	while(HeapDescriptor = ArcGetMemoryDescriptor(HeapDescriptor))
	{
		if( (HeapDescriptor->MemoryType == MemoryFree || HeapDescriptor->MemoryType == MemoryFreeContiguous) &&
			(HeapDescriptor->BasePage + HeapDescriptor->PageCount == ProgramDescriptor->BasePage))
		{
			break;
		}
	}

	//
	// if a free memory descriptor was not found that describes the free memory just below the OS loader,
	// or the memory descriptor is not large enough for the OS loader stack and heap,
	// then try and find a suitable one.
	//
	if(!HeapDescriptor || HeapDescriptor->PageCount < BL_HEAP_PAGES + BL_STACK_PAGES)
	{
		HeapDescriptor									= 0;
		while(HeapDescriptor = ArcGetMemoryDescriptor(HeapDescriptor))
		{
			if( (HeapDescriptor->MemoryType == MemoryFree || HeapDescriptor->MemoryType == MemoryFreeContiguous) &&
				(HeapDescriptor->PageCount >= BL_HEAP_PAGES + BL_STACK_PAGES))
			{
				break;
			}
		}
	}

	//
	// a suitable descriptor could not be found, return an unsuccessful status.
	//
	if(!HeapDescriptor)
	{
		BlPrint("Couldn't find HeapDescriptor\r\n");
		return ENOMEM;
	}

	BlpTrackUsage(LoaderOsloaderHeap,HeapDescriptor->BasePage,HeapDescriptor->PageCount);

	//
	// compute the address of the loader heap, initialize the heap allocation variables, and zero the heap memory.
	//
	ULONG EndPage										= HeapDescriptor->BasePage + HeapDescriptor->PageCount;
	ULONG StackBasePage									= EndPage - BL_STACK_PAGES;
	ULONG HeapBasePage									= EndPage - BL_STACK_PAGES - BL_HEAP_PAGES;
	BlHeapFree											= KSEG0_BASE | (HeapBasePage << PAGE_SHIFT);

	//
	// always reserve enough space in the heap for one more memory descriptor, so we can go create more heap if we run out.
	//
	BlHeapLimit											= BlHeapFree + (BL_HEAP_PAGES << PAGE_SHIFT) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

	RtlZeroMemory(reinterpret_cast<PVOID>(BlHeapFree),BL_HEAP_PAGES << PAGE_SHIFT);

	//
	// allocate the loader parameter block.
	//
	BlLoaderBlock										= static_cast<PLOADER_PARAMETER_BLOCK>(BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK)));
	if(!BlLoaderBlock)
	{
		BlPrint("Couldn't initialize loader block\r\n");
		return ENOMEM;
	}

	//
	// allocate the load parameter block extension
	//
	BlLoaderBlock->Extension							= static_cast<PLOADER_PARAMETER_EXTENSION>(BlAllocateHeap(sizeof(LOADER_PARAMETER_EXTENSION)));
	if(!BlLoaderBlock->Extension)
	{
		BlPrint("Couldn't initialize loader block extension\r\n");
		return ENOMEM;
	}

	BlLoaderBlock->Extension->Size						= sizeof(LOADER_PARAMETER_EXTENSION);
	BlLoaderBlock->Extension->MajorVersion				= MAJOR_VERSION;
	BlLoaderBlock->Extension->MinorVersion				= MINOR_VERSION;

	InitializeListHead(&BlLoaderBlock->LoadOrderListHead);
	InitializeListHead(&BlLoaderBlock->MemoryDescriptorListHead);
	InitializeListHead(&BlLoaderBlock->Extension->FirmwareTable64ListHead);

	//
	// copy the memory descriptor list from firmware into the local heap and deallocate the loader heap and stack from the free memory descriptor.
	//
	PMEMORY_DESCRIPTOR MemoryDescriptor					= 0;
	PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor	= 0;
	while(MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor))
	{
		AllocationDescriptor							= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));

		if(!AllocationDescriptor)
		{
			BlPrint("Couldn't allocate heap for memory allocation descriptor\r\n");
			return ENOMEM;
		}

		//
		// fix memory type
		//
		AllocationDescriptor->MemoryType				= MemoryDescriptor->MemoryType;
		if(MemoryDescriptor->MemoryType == MemoryFreeContiguous)
			AllocationDescriptor->MemoryType			= LoaderFree;
		else if(MemoryDescriptor->MemoryType == MemorySpecialMemory)
			AllocationDescriptor->MemoryType			= LoaderSpecialMemory;

		AllocationDescriptor->BasePage					= MemoryDescriptor->BasePage;
		AllocationDescriptor->PageCount					= MemoryDescriptor->PageCount;

		//
		// heap will have its own descriptor
		//
		if(MemoryDescriptor == HeapDescriptor)
			AllocationDescriptor->PageCount				-= (BL_HEAP_PAGES + BL_STACK_PAGES);

		//
		// if the final count is not zero,insert it into allocation list
		//
		if(AllocationDescriptor->PageCount)
			BlInsertDescriptor(AllocationDescriptor);
	}

	//
	// allocate a memory descriptor for the loader stack.
	//
	AllocationDescriptor								= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));
	if(!AllocationDescriptor)
	{
		BlPrint("Couldn't allocate heap for loader stack\r\n");
		return ENOMEM;
	}

	AllocationDescriptor->MemoryType					= LoaderOsloaderStack;
	AllocationDescriptor->BasePage						= StackBasePage;
	AllocationDescriptor->PageCount						= BL_STACK_PAGES;

	BlInsertDescriptor(AllocationDescriptor);

	//
	// allocate a memory descriptor for the loader heap.
	//
	AllocationDescriptor								= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));
	if(!AllocationDescriptor)
	{
		BlPrint("Couldn't allocate heap for loader heap\r\n");
		return ENOMEM;
	}

	AllocationDescriptor->MemoryType					= LoaderOsloaderHeap;
	AllocationDescriptor->BasePage						= HeapBasePage;
	AllocationDescriptor->PageCount						= BL_HEAP_PAGES;

	BlInsertDescriptor(AllocationDescriptor);

	return ESUCCESS;
}

//
// determine allocation policy
//
TYPE_OF_MEMORY BlpDetermineAllocationPolicy(__in TYPE_OF_MEMORY InputType,__in ULONG BasePage,__in ULONG PageCount,__in BOOLEAN SecondChance)
{
	if(BlRestoring)
	{
		BlMemoryAllocationPolicy						= BlAllocateLowestFit;

		return LoaderFree;
	}

	if(InputType == LoaderXIPMemory)
	{
		if(PageCount <= 0x400)
		{
			BlMemoryAllocationPolicy					= BlAllocateLowestFit;

			return SecondChance ? LoaderReserved : LoaderFree;
		}

		BlMemoryAllocationPolicy						= BlAllocateHighestFit;

		return LoaderReserved;
	}

	if(BlVirtualBias)
	{
		if(BlOldKernel)
		{
			if(InputType == LoaderFree || InputType == LoaderBad || InputType == LoaderFirmwareTemporary || InputType == LoaderOsloaderStack || InputType == LoaderReserved)
				BlMemoryAllocationPolicy				= BlAllocateHighestFit;
			else
				BlMemoryAllocationPolicy				= BlAllocateLowestFit;

			return LoaderFree;
		}

		if(InputType == LoaderFree || InputType == LoaderBad || InputType == LoaderFirmwareTemporary || InputType == LoaderOsloaderStack || InputType == LoaderReserved)
			BlMemoryAllocationPolicy					= BlAllocateLowestFit;
		else
			BlMemoryAllocationPolicy					= SecondChance ? BlAllocateLowestFit : BlAllocateHighestFit;

		return SecondChance ? LoaderReserved : LoaderFree;
	}

	if(InputType == LoaderFree || InputType == LoaderBad || InputType == LoaderFirmwareTemporary || InputType == LoaderOsloaderStack || InputType == LoaderReserved)
	{
		BlMemoryAllocationPolicy						= BlAllocateHighestFit;

		InputType										= SecondChance ? LoaderFree : LoaderReserved;
	}
	else
	{
		BlMemoryAllocationPolicy						= BlAllocateLowestFit;
		InputType										= SecondChance ? LoaderReserved : LoaderFree;
	}

	if(BlOldKernel)
		InputType										= LoaderFree;

	return InputType;
}

//
// allocate descriptor
//
ARC_STATUS BlAllocateAlignedDescriptor(__in TYPE_OF_MEMORY MemoryType,__in ULONG BasePage,__in ULONG PageCount,__in ULONG Alignment,__out PULONG ActualBase)
{
	//
	// simplify the alignment checks by changing 0 to 1.
	//
	if(Alignment == 0)
		Alignment										= 1;

	//
	// and also page count
	//
	if(PageCount == 0)
		PageCount										= 1;

	//
	// determine allocation policy
	//
	ALLOCATION_POLICY SavedPolicy						= BlMemoryAllocationPolicy;
	BOOLEAN SecondChance								= FALSE;

	while(1)
	{
		//
		// determine allocation policy
		//
		TYPE_OF_MEMORY AllocateType						= BlpDetermineAllocationPolicy(MemoryType,BasePage,PageCount,SecondChance);

		//
		// if caller specified a base page,and it is in the usable range,try to find the descriptor containing that page,and allocate from it
		//
		if(BasePage && BasePage >= BlUsableBase && BasePage + PageCount <= BlUsableLimit)
		{
			//
			// find the descriptor
			//
			PMEMORY_ALLOCATION_DESCRIPTOR FoundDesc		= BlFindMemoryDescriptor(BasePage);

			//
			// we found one,but make sure the type is correct and the size is big enough
			//
			if(FoundDesc && FoundDesc->MemoryType == AllocateType && FoundDesc->BasePage + FoundDesc->PageCount >= PageCount + BasePage)
			{
				//
				// allocate from it
				//
				ARC_STATUS Status						= BlGenerateDescriptor(FoundDesc,MemoryType,BasePage,PageCount);

				//
				// track usage
				//
				BlpTrackUsage(MemoryType,BasePage,PageCount);

				//
				// setup output param
				//
				*ActualBase								= BasePage;

				//
				// check those pages are mapped
				//
				if(MempCheckMapping(BasePage,PageCount + 1) != ESUCCESS)
					Status								= ENOMEM;

				//
				// restore the original policy
				//
				BlMemoryAllocationPolicy				= SavedPolicy;

				return Status;
			}
		}

		//
		// the caller specified base page can't work,search the whole allocation list
		//
		PMEMORY_ALLOCATION_DESCRIPTOR FoundDesc			= 0;
		ULONG BestAlignedBase							= 0;
		ULONG BestLeftCount								= 0;
		PLIST_ENTRY NextEntry							= BlLoaderBlock->MemoryDescriptorListHead.Flink;

		for(; NextEntry != &BlLoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink)
		{
			PMEMORY_ALLOCATION_DESCRIPTOR NextDesc		= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

			//
			// calc aligned base page and aligned left count for this memory block
			//
			ULONG AlignedBasePage						= (NextDesc->BasePage + Alignment - 1) & ~(Alignment - 1);
			ULONG LeftCount								= NextDesc->BasePage + NextDesc->PageCount - AlignedBasePage;

			//
			// check type is correct,also make sure the address is not wrapped
			//
			if(NextDesc->MemoryType != AllocateType || LeftCount > NextDesc->PageCount)
				continue;

			//
			// isn't in the usable range
			//
			if(AlignedBasePage + LeftCount <= BlUsableBase || AlignedBasePage > BlUsableLimit)
				continue;

			//
			// truncate it to fit in the usable range
			//
			if(AlignedBasePage < BlUsableBase)
			{
				AlignedBasePage							= (BlUsableBase + Alignment - 1) & ~(Alignment - 1);
				LeftCount								= NextDesc->BasePage + NextDesc->PageCount - AlignedBasePage;
			}

			if(AlignedBasePage + LeftCount > BlUsableLimit)
				LeftCount								= BlUsableLimit - AlignedBasePage;

			//
			// size is to small
			//
			if(PageCount > LeftCount)
				continue;

			//
			// policy tells us to return the first one
			//
			if(BlMemoryAllocationPolicy == BlAllocateLowestFit)
			{
				FoundDesc								= NextDesc;
				BestAlignedBase							= AlignedBasePage;
				BestLeftCount							= LeftCount;
				break;
			}

			//
			// if this is the first on we found
			// or policy wants us to return the last one
			// or this one's size is smaller than the best one
			// record it
			//
			if (!FoundDesc || BlMemoryAllocationPolicy == BlAllocateHighestFit || LeftCount < BestLeftCount)
			{
				FoundDesc								= NextDesc;
				BestAlignedBase							= AlignedBasePage;
				BestLeftCount							= LeftCount;
			}
		}

		//
		// found a descriptor,allocate from it
		//
		if(FoundDesc)
		{
			//
			// allocate at the end of the descirptor
			//
			if(BlMemoryAllocationPolicy == BlAllocateHighestFit)
				BestAlignedBase							= (BestLeftCount + BestAlignedBase - PageCount) & ~(Alignment - 1);

			//
			// track it
			//
			*ActualBase									= BestAlignedBase;
			BlpTrackUsage(AllocateType,BestAlignedBase,PageCount);

			//
			// check mapped
			//
			ARC_STATUS Status							= MempCheckMapping(BestAlignedBase,PageCount + 1);
			BlMemoryAllocationPolicy					= SavedPolicy;

			//
			// allocate it
			//
			if(Status == ESUCCESS)
				Status									= BlGenerateDescriptor(FoundDesc,MemoryType,BestAlignedBase,PageCount);

			return Status;
		}

		//
		// we can not find a suitable one,try again  with 'SecondChance'
		// BlpDetermineAllocationPolicy will return another memory type to us
		//
		if(BlOldKernel || SecondChance)
		{
			BlMemoryAllocationPolicy					= SavedPolicy;
			return ENOMEM;
		}

		SecondChance									= TRUE;
	}

	return ENOMEM;
}

//
// free descriptor
//
VOID BlFreeDescriptor(__in ULONG BasePage)
{
	PLIST_ENTRY NextEntry								= BlLoaderBlock->MemoryDescriptorListHead.Flink;
	while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Desc				= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		//
		// found the page,but if the type is already free,nothing to do
		//
		if(Desc->BasePage == BasePage && Desc->MemoryType != LoaderFree)
		{
			//
			// update type
			//
			Desc->MemoryType							= LoaderFree;

			//
			// check and update lowest,highest page
			//
			if(Desc->BasePage + Desc->PageCount	== BlHighestPage)
				BlHighestPage							= Desc->BasePage + 1;
			else if(Desc->BasePage == BlLowestPage)
				BlLowestPage							= Desc->BasePage + Desc->PageCount;

			//
			// remove it
			//
			RemoveEntryList(&Desc->ListEntry);

			//
			// insert it again,we need to merge it with the others
			//
			BlInsertDescriptor(Desc);

			//
			// done
			//
			break;
		}

		NextEntry										= NextEntry->Flink;
	}
}

//
// truncate descriptors
//
VOID BlpTruncateDescriptors(__in ULONG MaxPage)
{
	for(PLIST_ENTRY NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink; NextEntry != &BlLoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Desc				= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		//
		// this block is completely blow the max page,keep it
		//
		if(Desc->PageCount + Desc->BasePage - 1 <= MaxPage)
			continue;

		if(Desc->BasePage > MaxPage)
		{
			//
			// this block is completely above the max page,remove it
			//
			RemoveEntryList(&Desc->ListEntry);
		}
		else
		{
			//
			// otherwise,truncate its size
			//
			Desc->PageCount								= MaxPage - Desc->BasePage + 1;
		}
	}
}

//
// map reserved to firmware temporary
//
VOID BlpRemapReserve()
{
	PLIST_ENTRY NextEntry								= BlLoaderBlock->MemoryDescriptorListHead.Flink;
	while(NextEntry	!= &BlLoaderBlock->MemoryDescriptorListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Desc				= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		if(Desc->MemoryType == LoaderReserved)
			Desc->MemoryType							= LoaderFirmwareTemporary;

		NextEntry										= NextEntry->Flink;
	}
}

//
// count pae tables
//
ULONG BlpCountPAEPagesToMapX86Page(__in PHARDWARE_PTE PteArray)
{
	ULONG Count											= 0;
	for(ULONG i = 0; i < 2; i ++)
	{
		for(ULONG j = 0; j < 512; j ++)
		{
			if(PteArray[i * 512 + j].Valid)
			{
				Count									+= 1;
				break;
			}
		}
	}

	return Count;
}

//
// allocate pae tables
//
ARC_STATUS BlpAllocatePAETables()
{
	//
	// count mapped pages
	//
	ULONG Count											= 0;
	for(ULONG i = 0; i < 1024; i ++)
	{
		if(PDE[i].Valid)
			Count										+= BlpCountPAEPagesToMapX86Page(reinterpret_cast<PHARDWARE_PTE>(PDE[i].PageFrameNumber << PAGE_SHIFT));
	}

	//
	// allocate memory
	//
	Count												+= 6;
	ULONG BasePage										= 0;
	ARC_STATUS Status									= BlAllocateAlignedDescriptor(LoaderMemoryData,0,Count,1,&BasePage);
	if(Status != ESUCCESS)
	{
		DbgPrint("BlAllocateDescriptor failed!\n");
		return Status;
	}

	PDPT												= reinterpret_cast<PHARDWARE_PTE_PAE>(BasePage << PAGE_SHIFT);
	RtlZeroMemory(PDPT,Count << PAGE_SHIFT);

	return ESUCCESS;
}

//
// get pde
//
PHARDWARE_PTE_PAE BlpFindPAEPageDirectoryEntry(__in ULONG VirtualAddress)
{
	return reinterpret_cast<PHARDWARE_PTE_PAE>(PDPT[VirtualAddress >> 30].PageFrameNumber << PAGE_SHIFT) + ((VirtualAddress >> 21) & 0x1ff);
}

//
// get pte
//
PHARDWARE_PTE_PAE BlpFindPAEPageTableEntry(__in ULONG VirtualAddress)
{
	PHARDWARE_PTE_PAE Pde								= BlpFindPAEPageDirectoryEntry(VirtualAddress);
	ASSERT(Pde->Valid);

	return reinterpret_cast<PHARDWARE_PTE_PAE>(Pde->PageFrameNumber << PAGE_SHIFT) + ((VirtualAddress >> PAGE_SHIFT) & 0x1ff);
}

//
// copy x86 to pae
//
VOID BlpCopyX86PteToPAEPte(__in PHARDWARE_PTE PteX86,__out PHARDWARE_PTE_PAE PtePae)
{
	PtePae->Accessed									= PteX86->Accessed;
	PtePae->CacheDisable								= PteX86->CacheDisable;
	PtePae->Dirty										= PteX86->Dirty;
	PtePae->Global										= PteX86->Global;
	PtePae->LargePage									= PteX86->LargePage;
	PtePae->Owner										= PteX86->Owner;
	PtePae->Valid										= PteX86->Valid;
	PtePae->Write										= PteX86->Write;
	PtePae->WriteThrough								= PteX86->WriteThrough;
}

//
// map address
//
VOID BlpMapAddress(__in ULONG Address,__in PHARDWARE_PTE PdeX86,__in PHARDWARE_PTE PteX86,__out PULONG NextPage)
{
	//
	// skip the 1st page diretory tables
	//
	if(Address >= PTE_BASE_X86 && Address < 0xc0301000)
		return;

	PHARDWARE_PTE_PAE PdePae							= BlpFindPAEPageDirectoryEntry(Address);
	if(!PdePae->Valid)
	{
		ULONG Page										= *NextPage;
		*NextPage										= Page + 1;
		BlpCopyX86PteToPAEPte(PdeX86,PdePae);
		PdePae->PageFrameNumber							= Page;

		ULONG PteAddr									= PTE_BASE_X86 + (Address >> PAGE_SHIFT) * sizeof(HARDWARE_PTE_PAE);

		PHARDWARE_PTE_PAE PtePae						= BlpFindPAEPageTableEntry(PteAddr);
		if(!PtePae->Valid || PtePae->PageFrameNumber != Page)
			DbgBreakPoint();
	}

	PHARDWARE_PTE_PAE PtePae							= BlpFindPAEPageTableEntry(Address);
	if(PtePae->Valid)
		DbgBreakPoint();

	BlpCopyX86PteToPAEPte(PteX86,PtePae);
	PtePae->PageFrameNumber								= PteX86->PageFrameNumber;
}

//
// initialize pae tables
//
VOID BlpInitializePAETables()
{
	//
	// first set Page-Directory-Pointer-Table Entry
	//
	ULONG Temp											= (reinterpret_cast<ULONG>(PDPT) >> PAGE_SHIFT) + 1;
	for(ULONG i = 0; i < 4; i ++)
	{
		PDPT[i].Valid									= TRUE;
		PDPT[i].PageFrameNumber							= Temp;

		Temp											+= 1;
	}

	//
	// then map Page-Directory-Pointer-Table Entry at c0000000
	//
	PHARDWARE_PTE_PAE Pde								= reinterpret_cast<PHARDWARE_PTE_PAE>(PDPT[3].PageFrameNumber << PAGE_SHIFT);
	for(ULONG i = 0; i < 4; i ++)
	{
		Pde[i].PageFrameNumber							= PDPT[i].PageFrameNumber;
		Pde[i].Write									= TRUE;
		Pde[i].Valid									= TRUE;
	}

	//
	// then map the others
	//
	for(ULONG i = 0; i < 1024; i ++)
	{
		if(PDE[i].Valid)
		{
			PHARDWARE_PTE Pte							= reinterpret_cast<PHARDWARE_PTE>(PDE[i].PageFrameNumber << PAGE_SHIFT);

			for(ULONG j = 0; j < 1024; j ++)
			{
				if(Pte[j].Valid)
					BlpMapAddress((i * 1024 + j) << PAGE_SHIFT,PDE + i,Pte + j,&Temp);
			}
		}
	}

	Pde													= BlpFindPAEPageDirectoryEntry(0xffe00000);
	Pde->Valid											= TRUE;
	Pde->Write											= TRUE;
	Pde->PageFrameNumber								= Temp;
}

//
// unmap free memory
//
VOID NSUnmapFreeDescriptors(__in PLIST_ENTRY ListHead)
{
	ULONG HighestPage									= 0x1000;
	if(BlOldKernel)
		BlpRemapReserve();
	else if(BlHighestPage > HighestPage)
		HighestPage										= BlHighestPage;

	//
	// go though memory allocation list
	//
	PLIST_ENTRY	NextEntry								= ListHead->Flink;
	while(NextEntry != ListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Current			= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);
		TYPE_OF_MEMORY Type								= Current->MemoryType;

		if( Type == LoaderFree ||
			Type == LoaderLoadedProgram ||
			Type == LoaderOsloaderStack ||
			((Type == LoaderFirmwareTemporary || Type == LoaderReserved) && Current->BasePage < HighestPage))
		{
			ULONG Start									= Current->BasePage | (KSEG0_BASE >> PAGE_SHIFT);
			ULONG End									= Current->BasePage + Current->PageCount;
			if(End > HighestPage)
				End										= HighestPage | (KSEG0_BASE >> PAGE_SHIFT);
			else
				End										|= (KSEG0_BASE >> PAGE_SHIFT);

			while(Start < End)
			{
				if(PDE[Start >> 10].Valid)
				{
					PHARDWARE_PTE Pte					= reinterpret_cast<PHARDWARE_PTE>(KSEG0_BASE | (PDE[Start >> 10].PageFrameNumber << PAGE_SHIFT));

					Pte[Start & 0x3ff]					= ZeroPte;
				}

				Start									+= 1;
			}
		}

		NextEntry										= NextEntry->Flink;
	}

	if(BlOldKernel)
		return;

	for(ULONG i = (BlHighestPage >> 10) + 1; i <= HighestPde; i ++)
	{
		PDE[i]											= ZeroPte;
		PDE[i + 512]									= ZeroPte;
		if(BlVirtualBias)
			PDE[i + (BlVirtualBias >> 22) + 512]		= ZeroPte;
	}
}

//
// fix heap memory mappings
//
VOID NSFixMappings(__in PLIST_ENTRY ListHead)
{
	PLIST_ENTRY NextEntry								= ListHead->Flink;
	ULONG PageBias										= BlVirtualBias ? (BlVirtualBias >> PAGE_SHIFT) : 0;
	ULONG HeapFreePage									= GET_PHYSICAL_PAGE(BlHeapFree);
	while(NextEntry != ListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Current			= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		if(Current->MemoryType == LoaderOsloaderHeap)
		{
			ULONG BiasedPage							= (Current->BasePage | (KSEG0_BASE >> PAGE_SHIFT)) + PageBias;

			if(Current->BasePage <= HeapFreePage && Current->BasePage + Current->PageCount > HeapFreePage + 1)
			{
				SplitDescriptor->MemoryType				= LoaderFirmwareTemporary;
				SplitDescriptor->BasePage				= HeapFreePage + 1;
				SplitDescriptor->PageCount				= Current->PageCount - (HeapFreePage - Current->BasePage + 1);
				Current->PageCount						= HeapFreePage - Current->BasePage + 1;

				BlInsertDescriptor(SplitDescriptor);
			}

			BOOLEAN ValidPde						= FALSE;
			if(PaeEnabled)
				ValidPde							= BlpFindPAEPageDirectoryEntry(BiasedPage << PAGE_SHIFT)->Valid;
			else
				ValidPde							= PDE[BiasedPage >> 10].Valid;

			if(!ValidPde)
				Current->MemoryType					= LoaderFirmwareTemporary;
		}

		//
		// change reserved to free
		//
		if(Current->MemoryType == LoaderReserved)
			Current->MemoryType							= LoaderFree;

		NextEntry										= NextEntry->Flink;
	}

	//
	// flush tlb
	//
	__asm
	{
		mov			eax,cr3
		mov			cr3,eax
	}
}