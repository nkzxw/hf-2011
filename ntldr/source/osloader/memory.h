//********************************************************************
//	created:	11:8:2008   17:44
//	file:		memory.h
//	author:		tiamo
//	purpose:	memory
//********************************************************************

#pragma once

//
// memory type
//
typedef enum _TYPE_OF_MEMORY
{
	LoaderExceptionBlock,								//  0
	LoaderSystemBlock,									//  1
	LoaderFree,											//  2
	LoaderBad,											//  3
	LoaderLoadedProgram,								//  4
	LoaderFirmwareTemporary,							//  5
	LoaderFirmwarePermanent,							//  6
	LoaderOsloaderHeap,									//  7
	LoaderOsloaderStack,								//  8
	LoaderSystemCode,									//  9
	LoaderHalCode,										//  a
	LoaderBootDriver,									//  b
	LoaderConsoleInDriver,								//  c
	LoaderConsoleOutDriver,								//  d
	LoaderStartupDpcStack,								//  e
	LoaderStartupKernelStack,							//  f
	LoaderStartupPanicStack,							// 10
	LoaderStartupPcrPage,								// 11
	LoaderStartupPdrPage,								// 12
	LoaderRegistryData,									// 13
	LoaderMemoryData,									// 14
	LoaderNlsData,										// 15
	LoaderSpecialMemory,								// 16
	LoaderBBTMemory,									// 17
	LoaderReserved,										// 18
	LoaderXIPMemory,									// 19
	LoaderType1a,										// 1a
	LoaderMaximum,										// 1b

	MemoryExceptionBlock								= LoaderExceptionBlock,
	MemorySystemBlock									= LoaderSystemBlock,
	MemoryFree											= LoaderFree,
	MemoryBad											= LoaderBad,
	MemoryLoadedProgram									= LoaderLoadedProgram,
	MemoryFirmwareTemporary								= LoaderFirmwareTemporary,
	MemoryFirmwarePermanent								= LoaderFirmwarePermanent,
	MemoryFreeContiguous								= LoaderOsloaderHeap,
	MemorySpecialMemory									= LoaderOsloaderStack,
	MemoryMaximum										= LoaderSystemCode,
}TYPE_OF_MEMORY,MEMORY_TYPE;

//
// page size
//
#define PAGE_SIZE										0x1000

//
// page shift
//
#define PAGE_SHIFT										12L

//
// kernel segment
//
#define KSEG0_BASE										0x80000000

//
// hal page base
//
#define HAL_PAGE_BASE									0xffc00000

//
// PDE base for PAE
//
#define PDE_BASE_X86PAE									0xc0600000

//
// PDE base
//
#define PDE_BASE_X86									0xc0300000

//
// PTE base
//
#define PTE_BASE_X86									0xc0000000

//
// make kernel address
//
#define MAKE_KERNEL_ADDRESS(A)							((PVOID)((ULONG_PTR)(A) | KSEG0_BASE))

//
// make kernel address from page
//
#define MAKE_KERNEL_ADDRESS_PAGE(P)						((PVOID)(((ULONG_PTR)(P) << PAGE_SHIFT) | KSEG0_BASE))

//
// get physical address
//
#define GET_PHYSICAL_ADDRESS(A)							((PVOID)((ULONG_PTR)(A) & ~KSEG0_BASE))

//
// get physical page
//
#define GET_PHYSICAL_PAGE(A)							(((ULONG_PTR)(A) & ~KSEG0_BASE) >> PAGE_SHIFT)

//
// memory allocation policy
//
typedef enum _ALLOCATION_POLICY
{
	//
	// lowest fit(the first one)
	//
	BlAllocateLowestFit,

	//
	// best fit(size best)
	//
	BlAllocateBestFit,

	//
	//
	// highest fit(the last one)
	BlAllocateHighestFit,
}ALLOCATION_POLICY,*PALLOCATION_POLICY;

//
// memory descriptor
//
typedef struct _MEMORY_DESCRIPTOR
{
	//
	// type
	//
	MEMORY_TYPE											MemoryType;

	//
	// start page
	//
	ULONG												BasePage;

	//
	// page count
	//
	ULONG												PageCount;
}MEMORY_DESCRIPTOR,*PMEMORY_DESCRIPTOR;

//
// memory allocation descriptor
//
typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
	//
	// list entry
	//
	LIST_ENTRY											ListEntry;

	//
	// memory type
	//
	TYPE_OF_MEMORY										MemoryType;

	//
	// start page
	//
	ULONG												BasePage;

	//
	// count
	//
	ULONG												PageCount;
}MEMORY_ALLOCATION_DESCRIPTOR,*PMEMORY_ALLOCATION_DESCRIPTOR;

//
// allocate permanent heap
//
PVOID FwAllocateHeapPermanent(__in ULONG NumberPages);

//
// allocate temporary heap
//
PVOID FwAllocateHeap(__in ULONG Size);

//
// allocate pool
//
PVOID FwAllocatePool(__in ULONG Size);

//
// allocate aligned pool
//
PVOID FwAllocateHeapAligned(__in ULONG Size);

//
// initialize memory sub system
//
ARC_STATUS InitializeMemorySubsystem(__in PBOOT_CONTEXT BootContext);

//
// initialize memory descriptors
//
VOID InitializeMemoryDescriptors();

//
// map io space
//
PVOID MmMapIoSpace(__in PHYSICAL_ADDRESS PhysicalAddress,__in ULONG NumberOfBytes,__in MEMORY_CACHING_TYPE CacheType);

//
// initialize boot loader memory
//
ARC_STATUS BlMemoryInitialize();

//
// allocate from heap
//
PVOID BlAllocateHeap(__in ULONG Size);

//
// alloacte heap,return buffer is round up to a cache line boundary
//
PVOID BlAllocateHeapAligned(__in ULONG Size);

//
// allocate descriptor
//
ARC_STATUS BlAllocateAlignedDescriptor(__in TYPE_OF_MEMORY MemoryType,__in ULONG BasePage,__in ULONG PageCount,__in ULONG Alignment,__out PULONG ActualBase);

//
// free descriptor
//
VOID BlFreeDescriptor(__in ULONG BasePage);

//
// unmap free memory
//
VOID NSUnmapFreeDescriptors(__in PLIST_ENTRY ListHead);

//
// fix heap memory mappings
//
VOID NSFixMappings(__in PLIST_ENTRY ListHead);

//
// disk cache
//
extern PVOID FwDiskCache;

//
// temporary heap
//
extern ULONG FwTemporaryHeap;

//
// permanent heap
//
extern ULONG FwPermanentHeap;

//
// firmware descriptors is valid
//
extern BOOLEAN FwDescriptorsValid;

//
// fw pool start
//
extern ULONG											FwPoolStart;

//
// fw pool end
//
extern ULONG											FwPoolEnd;

//
// pde
//
extern PHARDWARE_PTE									PDE;

//
// hal pte
//
extern PHARDWARE_PTE									HalPT;

//
// pdpt
//
extern PHARDWARE_PTE_PAE								PDPT;

//
// pcr base page
//
extern ULONG											PcrBasePage;

//
// tss page
//
extern ULONG											TssBasePage;

//
// restoring
//
extern BOOLEAN											BlRestoring;

//
// lowest page
//
extern ULONG											BlLowestPage;

//
// highest page
//
extern ULONG											BlHighestPage;

//
// old kernel
//
extern BOOLEAN											BlOldKernel;

//
// pae enabled
//
extern BOOLEAN											PaeEnabled;

//
// usable base
//
extern ULONG											BlUsableBase;

//
// usable limit
//
extern ULONG											BlUsableLimit;

//
// virtual bias
//
extern ULONG											BlVirtualBias;

//
// highest pde
//
extern ULONG											HighestPde;

//
// zero pte
//
extern HARDWARE_PTE										ZeroPte;