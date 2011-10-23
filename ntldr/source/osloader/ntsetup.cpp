//********************************************************************
//	created:	22:8:2008   0:26
//	file:		ntsetup.cpp
//	author:		tiamo
//	purpose:	setup for nt
//********************************************************************

#include "stdafx.h"

//
// allocate pae tables
//
ARC_STATUS BlpAllocatePAETables();

//
// initialize pae tables
//
VOID BlpInitializePAETables();

//
// truncate descriptors
//
VOID BlpTruncateDescriptors(__in ULONG MaxPage);

//
// fix processor context
//
VOID NSFixProcessorContext(__in ULONG Pcr,__in ULONG Tss)
{
#pragma pack(push,2)
	static struct
	{
		USHORT Limit;
		ULONG Base;
	}GdtDef,IdtDef;
#pragma pack(pop)

	//
	// kernel expects the PCR to be zero-filled on startup
	//
	RtlZeroMemory(reinterpret_cast<PVOID>(Pcr),PAGE_SIZE);

	//
	// get gdtr idtr
	//
	__asm
	{
		sgdt		GdtDef;
		sidt		IdtDef;
	}

	//
	// move them to kernel space
	//
	GdtDef.Base											= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS(GdtDef.Base)) + BlVirtualBias;
	IdtDef.Base											= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS(IdtDef.Base)) + BlVirtualBias;

	PKGDTENTRY GdtArray									= reinterpret_cast<PKGDTENTRY>(GdtDef.Base);

	//
	// initialize selector that points to PCR
	//
	GdtArray[6].BaseLow									= static_cast<USHORT>(Pcr & 0xffff);
	GdtArray[6].HighWord.Bytes.BaseMid					= static_cast<UCHAR>((Pcr >> 16) & 0xff);
	GdtArray[6].HighWord.Bytes.BaseHi					= static_cast<UCHAR>((Pcr >> 24) & 0xff);

	//
	// initialize selector that points to TSS
	//
	GdtArray[5].BaseLow									= static_cast<USHORT>(Tss & 0xffff);
	GdtArray[5].HighWord.Bytes.BaseMid					= static_cast<UCHAR>((Tss >> 16) & 0xff);
	GdtArray[5].HighWord.Bytes.BaseHi					= static_cast<UCHAR>((Tss >> 24) & 0xff);

	//
	// reload gdtr ldtr
	//
	__asm
	{
		lgdt		GdtDef
		lidt		IdtDef;
	}
}

//
// enable pae start
//
#pragma optimize("",off)
#pragma code_seg(push,".PAE")
extern "C" VOID __declspec(naked) BlpEnablePAE(__in PHARDWARE_PTE_PAE Pdpt)
{
	__asm
	{
		mov     edx, [esp+4]											// get pdpt
		mov     ecx, cr3												// flush tlb
		mov     cr3, ecx
		mov     eax, cr0
		and     eax, 7FFFFFFFh
		mov     cr0, eax												// disable pagging
		jmp     short $+2												// flush pipeline
		_emit	0x0f
		_emit	0x20
		_emit	0xe0													// mov     eax, cr4
		or      eax, 20h												// enable pae
		mov     ecx, cr3
		_emit	0x0f
		_emit	0x22
		_emit	0xe0													// mov     cr4, eax
		mov     cr3, edx												// set pdpt
		mov     ecx, cr0
		or      ecx, 80000000h
		mov     cr0, ecx												// enable pagging
		jmp     short $+2												// flush pipeline
		retn    4
	}
}

//
// enable pae end
//
extern "C" ULONG BlpEnablePAEEnd()
{
	return 0x67891234;
}
#pragma code_seg(pop)
#pragma optimize("",on)

//
// initialize kernel's data structures
//
ARC_STATUS BlSetupForNt(__in PLOADER_PARAMETER_BLOCK BlLoaderBlock)
{
	//
	// cleanup display,from now on,no DBCS support
	//
	TextGrTerminate();

	//
	// do not support abios
	//
	BlLoaderBlock->u.I386.CommonDataArea				= 0;

	//
	// machine type
	//
	BlLoaderBlock->u.I386.MachineType					= MachineType;

	//
	// pcr page is allocated in entry point
	// pcr should be under 16MB
	//
	if(!PcrBasePage || PcrBasePage >= 0x1000)
	{
		BlPrint("Couldn't allocate PCR descriptor in NtProcessStartup,BlSetupForNt is failing.\r\n");
		return ENOMEM;
	}

	//
	// make a copy
	//
	static ULONG Pcr									= 0;
	Pcr													= PcrBasePage;

	//
	// zero hal pte used by MmMapIo
	// if we are using 1394 or USB debugging,it should be disabled
	//
	BdStopDebugger();
	RtlZeroMemory(HalPT,PAGE_SIZE);

	//
	// setup user shared data page
	//
	PHARDWARE_PTE Pte									= HalPT + ((KI_USER_SHARED_DATA - HAL_PAGE_BASE) >> PAGE_SHIFT);
	Pte->PageFrameNumber								= Pcr + 1;
	Pte->Valid											= TRUE;
	Pte->Write											= TRUE;
	RtlZeroMemory(reinterpret_cast<PVOID>(KI_USER_SHARED_DATA),PAGE_SIZE);

	//
	// setup pcr page
	//
	Pte													= HalPT + ((KIP0PCRADDRESS - HAL_PAGE_BASE) >> PAGE_SHIFT);
	Pte->PageFrameNumber								= Pcr;
	Pte->Write											= TRUE;
	Pte->Valid											= TRUE;

	//
	// using the new pcr address
	//
	Pcr													= KIP0PCRADDRESS;

	ARC_STATUS Status									= ESUCCESS;

	//
	// if we are using pae,allocate pae table
	// otherwise throw those memory above 4GB line,we can't use them without pae
	//
	if(BlUsePae)
		Status											= BlpAllocatePAETables();
	else
		BlpTruncateDescriptors(0xfffff);

	//
	// unable to allocate pae tables
	//
	if(Status != ESUCCESS)
		return Status;

	//
	// check Tss,the same to the PCR page
	//
	if(!TssBasePage || TssBasePage >= 0x1000)
	{
		BlPrint("Couldn't allocate TSS descriptor in NtProcessStartup,BlSetupForNt is failing.\r\n");
		return ENOMEM;
	}

	static ULONG Tss									= 0;

	//
	// use kernel address
	//
	Tss													= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS_PAGE(TssBasePage));

	//
	// account in virtual bias
	//
	Tss													+= BlVirtualBias;

	//
	// clear map from [dd000000,e100000] or [dd000000,e0000000]
	//
	ULONG EndPdeIndex									= BlVirtualBias ? (BlOldKernel ? 0 : 896) : 900;
	for(ULONG i = 884; i < EndPdeIndex; i ++)
		PDE[i]											= ZeroPte;

	//
	// allocate split descriptor
	//
	extern PMEMORY_ALLOCATION_DESCRIPTOR SplitDescriptor;
	SplitDescriptor										= static_cast<PMEMORY_ALLOCATION_DESCRIPTOR>(BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR)));

	//
	// set pte to not present for those memory descriptors whose type is LoaderFree
	//
	NSUnmapFreeDescriptors(&BlLoaderBlock->MemoryDescriptorListHead);

	//
	// flush tlb
	//
	__asm
	{
		mov		eax,cr3
		mov		cr3,eax
	}

	//
	// setup pae tables,and goto pae mode
	//
	if(BlUsePae)
	{
		BlpInitializePAETables();
		BlpEnablePAE(PDPT);
		PaeEnabled										= TRUE;
	}

	//
	// setup pcr and tss entry in the gdt and map gdt,idt to kernel space
	//
	NSFixProcessorContext(Pcr,Tss);

	//
	// fix heap memory pointers
	//
	NSFixMappings(&BlLoaderBlock->MemoryDescriptorListHead);

	//
	// save highest page
	//
	BlLoaderBlock->Extension->HighestPage				= BlHighestPage + 1;
	if(BlVirtualBias)
	{
		if(BlHighestPage + 1 < 0x1000)
			BlLoaderBlock->Extension->HighestPage		= 0x1000;

		//
		// update virtual bias
		//
		if(!BlOldKernel)
			BlVirtualBias								+= (BlLowestPage << PAGE_SHIFT);
	}

	BlLoaderBlock->u.I386.VirtualBias					= BlVirtualBias;

	return Status;
}