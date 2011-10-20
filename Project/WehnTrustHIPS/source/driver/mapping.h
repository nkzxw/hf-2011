/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_MAPPING_H
#define _WEHNTRUST_DRIVER_MAPPING_H

#define IsAddressOutOfRange(a, base, size)          \
	(((ULONG_PTR)(a) <  (ULONG_PTR)(base)) ||        \
    ((ULONG_PTR)(a) >= ((ULONG_PTR)(base) + size)))
#define IsAddressInsideRange(a, base, size)         \
	(((ULONG_PTR)(a) >= (ULONG_PTR)(base)) &&        \
	 ((ULONG_PTR)(a) <  ((ULONG_PTR)(base) + size)))

#define IsAddressInsideRegion(a, start, end)        \
	(((ULONG_PTR)(a) >= (ULONG_PTR)(start)) &&       \
	 ((ULONG_PTR)(a) <  (ULONG_PTR)(end)))

typedef enum 
{
	ImageSetMappingInProgress,
	ImageSetMappingInitialized,
	ImageSetMappingExpired
} IMAGE_SET_MAPPING_STATE, *PIMAGE_SET_MAPPING_STATE;

typedef struct _IMAGE_SET_MAPPING
{
	LIST_ENTRY               Entry;
	LONG                     References;
	PSECTION_OBJECT          SectionObject;
	UNICODE_STRING           FileName;
	UNICODE_STRING           RandomizedFileName;
	LARGE_INTEGER            FileModificationTime;
	PVOID                    MappingBaseAddress;
	PVOID                    ImageBaseAddress;
	ULONG                    ImageFlags;
	ULONG                    ViewSize;
	ULONG                    BasePadding;
	BOOLEAN                  IsExecutable;
	ULONG                    NumberOfTimesMapped;
	IMAGE_SET_MAPPING_STATE  State;
	KEVENT                   StateEvent;

	//
	// NRE state information
	//
	BOOLEAN                  NreMirrorEnabled;
	ULONG_PTR                NreOriginalImageBase;
	ULONG                    NreOriginalImageSize;
} IMAGE_SET_MAPPING, *PIMAGE_SET_MAPPING;

typedef struct _SORTED_ADDRESS
{
	LIST_ENTRY               Entry;
	ULONG_PTR                BeginAddress;
	ULONG_PTR                EndAddress;
	ULONG                    Size;
} SORTED_ADDRESS, *PSORTED_ADDRESS;

//
// This structure represents a logical image set which contains zero or more
// image set mappings.
//
typedef struct _IMAGE_SET
{
	LIST_ENTRY               Entry;
	LONG                     References;
	ULONG                    Identifier;
	UNICODE_STRING           StorageDirectoryPath;
	POBJECT_NAME_INFORMATION QueryNameStringBuffer;
	ULONG                    QueryNameStringBufferSize;

	LIST_ENTRY               ImageMappingList;
	KMUTEX                   ImageMappingListMutex;

	ULONG_PTR                ImageMappingLowStartAddress;
	ULONG_PTR                ImageMappingHighStartAddress;
	LIST_ENTRY               SortedAddressList;

	//
	// This structure is populated by CacheSystemDllSymbols the first time
	// NTDLL.DLL is randomized in the context of the image set.
	//
	NRER_NT_DISPATCH_TABLE   NtDispatchTable;
	BOOLEAN                  NtDispatchTableInitialized;

	//
	// The cached location information for NTDLL within this image set.
	//
	PVOID                    NtdllImageBase;
	ULONG                    NtdllImageSize;
} IMAGE_SET, *PIMAGE_SET;

//
// Image Sets
//
NTSTATUS ImageSetsInitialize();

NTSTATUS GetImageSetForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		OUT PIMAGE_SET *ImageSet);
NTSTATUS CreateImageSet(
		OUT PIMAGE_SET *ImageSet);
NTSTATUS CreateImageSetStorageDirectory(
		IN PIMAGE_SET ImageSet);
NTSTATUS CreateImageSetMapping(
		IN PUNICODE_STRING FileName OPTIONAL,
		IN PUNICODE_STRING RandomizedFileName OPTIONAL,
		OUT PIMAGE_SET_MAPPING *Mapping);
NTSTATUS EnumerateImageSets(
		IN ULONG Index,
		OUT PIMAGE_SET *ImageSet);

NTSTATUS InsertImageSetMapping(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN BOOLEAN LockList);
NTSTATUS RemoveImageSetMapping(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping);

VOID LockImageSetMappingList(
		IN PIMAGE_SET ImageSet);
VOID UnlockImageSetMappingList(
		IN PIMAGE_SET ImageSet);

VOID ReferenceImageSet(
		IN PIMAGE_SET ImageSet);
VOID DereferenceImageSet(
		IN PIMAGE_SET ImageSet);

ULONG_PTR ClaimNextImageSetMappingAddress(
		IN PIMAGE_SET ImageSet,
		IN ULONG ImageSize);
VOID UnclaimImageSetMappingAddress(
		IN PIMAGE_SET ImageSet,
		IN ULONG_PTR BaseAddress);

#define GetImageSetLowStartAddress(Set) \
	(Set)->ImageMappingLowStartAddress
#define GetImageSetHighStartAddress(Set) \
	(Set)->ImageMappingHighStartAddress

//
// Image Set Mappings
//
NTSTATUS LookupRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN ULONG ViewSize,
		OUT PIMAGE_SET_MAPPING *Mapping);
NTSTATUS FindRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PFILE_OBJECT FileObject,
		OUT PIMAGE_SET_MAPPING *Mapping);
NTSTATUS CreateRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN ULONG ViewSize,
		IN PIMAGE_SET_MAPPING NewMapping);
BOOLEAN ExpireImageMappingIfNecessary(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PFILE_OBJECT FileObject);

VOID SetImageSetMappingLogicalFileName(
		IN PIMAGE_SET_MAPPING Mapping,
		IN PFILE_OBJECT FileObject);

VOID ReferenceImageSetMapping(
		IN PIMAGE_SET_MAPPING Mapping);
VOID DereferenceImageSetMapping(
		IN PIMAGE_SET_MAPPING Mapping);

NTSTATUS GetRandomizedImageMapping(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ViewSize,
		OUT PSECTION_OBJECT *NewSectionObject,
		OUT PIMAGE_SET_MAPPING *OutMapping);

BOOLEAN IsRandomizableSectionMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		OUT PIMAGE *OutImage);
NTSTATUS ImageSetAcquireRandomizedRange(
		IN PIMAGE_SET ImageSet,
		IN ULONG ViewSize,
		OUT PVOID *MappingBase,
		OUT PULONG BasePadding);
NTSTATUS FixupRelocatedImageHeaders(
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase);
BOOLEAN IsImageRelocateable(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PVOID ImageBase,
		IN ULONG ImageSize,
		OUT PBOOLEAN IsExecutable);

NTSTATUS StartMappingExpirationMonitor();
NTSTATUS StopMappingExpirationMonitor();
VOID ExpireUnreferencedImageSetMappings(
		IN PVOID Context);

//
// Randomized region interface
//
PVOID GetRandomizedBaseForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG RegionSize);
BOOLEAN CheckProcessRegionBaseExists(
		IN PPROCESS_OBJECT ProcessObject,
		IN PVOID RegionBase);

NTSTATUS RandomizeHighestUserAddress();

//
// Randomized Image File
//
NTSTATUS PerformImageFileRandomization(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PSECTION_OBJECT RawImageFileSectionObject,
		IN ULONG ViewSize);
NTSTATUS BuildRandomizedImageMappingFilePath(
		IN PIMAGE_SET ImageSet,
		IN PUNICODE_STRING FilePath,
		OUT PUNICODE_STRING *RandomizedFilePath);
NTSTATUS CreateRandomizedImageFile(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PUNICODE_STRING RandomizedFilePath,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize);

#endif
