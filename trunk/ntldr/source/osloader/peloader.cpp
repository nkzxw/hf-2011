//********************************************************************
//	created:	14:8:2008   14:52
//	file:		peloader.cpp
//	author:		tiamo
//	purpose:	pe file loader
//********************************************************************

#include "stdafx.h"

//
// image cache
//
typedef struct _IMAGE_CACHE
{
	//
	// file id
	//
	ULONG												FileId;

	//
	// current offset
	//
	LARGE_INTEGER										CurrentOffset;

	//
	// file size
	//
	ULONG												FileSize;

	//
	// physical address
	//
	ULONG												BaseAddress;
}IMAGE_CACHE,*PIMAGE_CACHE;

//
// initialize image cache
//
ARC_STATUS BlImageInitCache(__out PIMAGE_CACHE ImageCache,__in ULONG FileId)
{
	//
	// initialize the structure
	//
	ImageCache->FileId									= FileId;
	ImageCache->CurrentOffset.QuadPart					= 0;
	ImageCache->BaseAddress								= 0;
	ImageCache->FileSize								= 0;

	//
	// read file info and check file size is too big to cache
	//
	FILE_INFORMATION FileInfo;
	ARC_STATUS Status									= BlGetFileInformation(FileId,&FileInfo);
	if(Status != ESUCCESS)
		return Status;

	if(FileInfo.EndingAddress.HighPart)
		return E2BIG;

	//
	// allocate a descriptor to descript this cache memory
	//
	ULONG FileSize										= FileInfo.EndingAddress.LowPart;
	ULONG ActualBase									= 0;
	Status												= BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0,BYTES_TO_PAGES(FileSize),0x10,&ActualBase);
	if(Status != ESUCCESS)
		return Status;

	//
	// seek to the beginning
	//
	LARGE_INTEGER SeekOffset;
	SeekOffset.QuadPart									= 0;
	Status												= BlSeek(FileId,&SeekOffset,SeekAbsolute);
	if(Status == ESUCCESS)
	{
		//
		// read the whole file content
		//
		Status											= BlRead(FileId,MAKE_KERNEL_ADDRESS_PAGE(ActualBase),FileSize,&FileSize);
		if(Status == ESUCCESS && FileSize == FileInfo.EndingAddress.LowPart)
		{
			//
			// seek to beginning
			//
			SeekOffset.QuadPart							= 0;
			Status										= BlSeek(FileId,&SeekOffset,SeekAbsolute);
			if(Status == ESUCCESS)
			{
				ImageCache->FileSize					= FileSize;
				ImageCache->BaseAddress					= reinterpret_cast<ULONG>(MAKE_KERNEL_ADDRESS_PAGE(ActualBase));
			}
		}
		else if(Status == ESUCCESS)
		{
			//
			// means FileSize != FileInfo.EndingAddress.LowPart,.etc we can not read the whole file content
			//
			Status										= EIO;
		}
	}

	//
	// if we failed,free the allocated memory allocation descriptor
	//
	if(Status != ESUCCESS)
		BlFreeDescriptor(ActualBase);

	return Status;
}

//
// seek image cache
//
ARC_STATUS BlImageSeek(__in PIMAGE_CACHE ImageCache,__in ULONG FileId,__inout PLARGE_INTEGER SeekOffset,__in SEEK_MODE SeekMode)
{
	//
	// too big
	//
	if(SeekOffset->HighPart)
		return E2BIG;

	//
	// seek the file
	//
	ARC_STATUS Status									= BlSeek(FileId,SeekOffset,SeekMode);
	if(Status != ESUCCESS)
		return Status;

	//
	// update cache info
	//
	ImageCache->CurrentOffset.QuadPart					= SeekOffset->QuadPart;

	return ESUCCESS;
}

//
// read image cache
//
ARC_STATUS BlImageRead(__in PIMAGE_CACHE ImageCache,__in ULONG FileId,__out PVOID Buffer,__in ULONG Count,__out PULONG ActualReadCount)
{
	//
	// did not cached
	//
	if(!ImageCache->FileSize || !ImageCache->BaseAddress)
		return BlRead(FileId,Buffer,Count,ActualReadCount);

	*ActualReadCount									= 0;

	//
	// check left bytes count
	//
	if(ImageCache->FileSize > ImageCache->CurrentOffset.LowPart)
		*ActualReadCount								= ImageCache->FileSize - ImageCache->CurrentOffset.LowPart;

	//
	// check wanted count
	//
	if(*ActualReadCount > Count)
		*ActualReadCount								= Count;

	//
	// copy to caller's buffer
	//
	RtlCopyMemory(Buffer,Add2Ptr(ImageCache->BaseAddress,ImageCache->CurrentOffset.LowPart,PVOID),*ActualReadCount);

	//
	// update current offset
	//
	ImageCache->CurrentOffset.LowPart					+= *ActualReadCount;

	return ESUCCESS;
}

//
// free image cache
//
VOID BlImageFreeCache(__in PIMAGE_CACHE ImageCache)
{
	//
	// set file size to 0
	//
	ImageCache->FileSize								= 0;

	//
	// did not cached
	//
	if(!ImageCache->BaseAddress)
		return;

	//
	// free descritpor
	//
	BlFreeDescriptor(GET_PHYSICAL_PAGE(ImageCache->BaseAddress));
	ImageCache->BaseAddress								= 0;
}

//
// get image nt header
//
PIMAGE_NT_HEADERS RtlImageNtHeader(__in PVOID ImageBase)
{
	if(!ImageBase || reinterpret_cast<ULONG>(ImageBase) == 0xffffffff)
		return 0;

	PIMAGE_DOS_HEADER DosHeader							= static_cast<PIMAGE_DOS_HEADER>(ImageBase);
	if(DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	PIMAGE_NT_HEADERS NtHeader							= Add2Ptr(ImageBase,DosHeader->e_lfanew,PIMAGE_NT_HEADERS);
	if(NtHeader->Signature == IMAGE_NT_SIGNATURE)
		return NtHeader;

	return 0;
}

//
// calc pe checksum
//
USHORT ChkSum(__in ULONG PartialSum,__in PVOID Source,__in ULONG Length)
{
	//
	// compute the word wise checksum allowing carries to occur into the high order half of the checksum longword.
	//
	PUSHORT Buffer										= static_cast<PUSHORT>(Source);
	while(Length--)
	{
		PartialSum										+= *Buffer++;
		PartialSum										= (PartialSum >> 16) + (PartialSum & 0xffff);
	}

	//
	// fold final carry into a single word result and return the resultant value.
	//
	return static_cast<USHORT>(((PartialSum >> 16) + PartialSum) & 0xffff);
}

//
// get section table
//
PIMAGE_SECTION_HEADER RtlSectionTableFromVirtualAddress(__in PIMAGE_NT_HEADERS NtHeaders,__in PVOID Base,__in ULONG Address)
{
	PIMAGE_SECTION_HEADER NtSection						= IMAGE_FIRST_SECTION(NtHeaders);

	for(ULONG i = 0; i < NtHeaders->FileHeader.NumberOfSections; i ++)
	{
		if(Address >= NtSection->VirtualAddress && Address < NtSection->VirtualAddress + NtSection->SizeOfRawData)
			return NtSection;

		NtSection										+= 1;
	}

	return 0;
}

//
// return a seek address of the data
//
PVOID RtlAddressInSectionTable(__in PIMAGE_NT_HEADERS NtHeaders,__in PVOID Base,__in ULONG Address)
{
	PIMAGE_SECTION_HEADER NtSection						= RtlSectionTableFromVirtualAddress(NtHeaders,Base,Address);
	if(!NtSection)
		return 0;

	return Add2Ptr(Base,Address - NtSection->VirtualAddress + NtSection->PointerToRawData,PVOID);
}

//
// locate a directory entry 32bits
//
PVOID RtlImageDirectoryEntryToData32(__in PVOID Base,__in BOOLEAN MappedAsImage,__in USHORT DirectoryEntry,__out PULONG Size)
{
	PIMAGE_NT_HEADERS32 NtHeaders						= RtlImageNtHeader(Base);

	if(!NtHeaders || DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes)
		return 0;

	ULONG DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[DirectoryEntry].VirtualAddress;
	if(!DirectoryAddress)
		return 0;

	*Size												= NtHeaders->OptionalHeader.DataDirectory[DirectoryEntry].Size;

	if(MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders)
		return Add2Ptr(Base,DirectoryAddress,PVOID);

	return RtlAddressInSectionTable(reinterpret_cast<PIMAGE_NT_HEADERS>(NtHeaders),Base,DirectoryAddress);
}

//
// locate a directory entry 64bits
//
PVOID RtlImageDirectoryEntryToData64(__in PVOID Base,__in BOOLEAN MappedAsImage,__in USHORT DirectoryEntry,__out PULONG Size)
{
	PIMAGE_NT_HEADERS64 NtHeaders						= static_cast<PIMAGE_NT_HEADERS64>(static_cast<PVOID>(RtlImageNtHeader(Base)));

	if(!NtHeaders || DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes)
		return 0;

	ULONG DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[DirectoryEntry].VirtualAddress;
	if(!DirectoryAddress)
		return 0;

	*Size												= NtHeaders->OptionalHeader.DataDirectory[DirectoryEntry].Size;

	if(MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders)
		return Add2Ptr(Base,DirectoryAddress,PVOID);

	return RtlAddressInSectionTable(reinterpret_cast<PIMAGE_NT_HEADERS>(NtHeaders),Base,DirectoryAddress);
}

//
// locate a directory entry
//
PVOID RtlImageDirectoryEntryToData(__in PVOID Base,__in BOOLEAN MappedAsImage,__in USHORT DirectoryEntry,__out PULONG Size)
{
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(Base);
	if(NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return RtlImageDirectoryEntryToData32(Base,MappedAsImage,DirectoryEntry,Size);
	else if(NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		return RtlImageDirectoryEntryToData64(Base,MappedAsImage,DirectoryEntry,Size);

	return 0;
}

//
// process relocation block
//
PIMAGE_BASE_RELOCATION LdrProcessRelocationBlockLongLong(__in ULONG VA,__in ULONG SizeOfBlock,__in PUSHORT NextOffset,__in LONGLONG Diff)
{
	LONG Temp											= 0;
	LONG TempOrig										= 0;
	LONG ActualDiff										= 0;
	LONGLONG Value64									= 0;
	PULONG VA2											= 0;

	while(SizeOfBlock --)
	{
		USHORT Offset									= *NextOffset & 0xfff;
		PVOID FixupVA									= Add2Ptr(VA,Offset,PVOID);

		//
		// Apply the fixups.
		//
		switch(*NextOffset >> 12)
		{
			//
			// Absolute - no fixup required.
			//
		case IMAGE_REL_BASED_SECTION:
		case IMAGE_REL_BASED_REL32:
		case IMAGE_REL_BASED_ABSOLUTE :
			break;

			//
			// High - (16-bits) relocate the high half of an address.
			//
		case IMAGE_REL_BASED_HIGH :
			Temp										= *static_cast<PUSHORT>(FixupVA) << 16;
			Temp										+= static_cast<LONG>(Diff);
			*static_cast<PUSHORT>(FixupVA)				= static_cast<USHORT>(Temp >> 16);
			break;

			//
			// Low - (16-bit) relocate the low half of an address.
			//
		case IMAGE_REL_BASED_LOW :
			Temp										= *static_cast<PSHORT>(FixupVA);
			Temp										+= static_cast<LONG>(Diff);;
			*static_cast<PUSHORT>(FixupVA)				= static_cast<USHORT>(Temp);
			break;

			//
			// HighLow - (32-bits) relocate the high and low half of an address.
			//
		case IMAGE_REL_BASED_HIGHLOW :
			*static_cast<LONG UNALIGNED *>(FixupVA)		+= static_cast<LONG>(Diff);
			break;

			//
			// Adjust high - (16-bits) relocate the high half of an address and adjust for sign extension of low half.
			//
		case IMAGE_REL_BASED_HIGHADJ :
			Temp										= *static_cast<PUSHORT>(FixupVA) << 16;
			TempOrig									= Temp;
			NextOffset									+= 1;
			SizeOfBlock									-= 1;
			Temp										+= static_cast<LONG>(*static_cast<PSHORT>(static_cast<PVOID>(NextOffset)));
			Temp										+= static_cast<LONG>(Diff);
			Temp										+= 0x8000;
			*static_cast<PUSHORT>(FixupVA)				= static_cast<USHORT>(Temp >> 16);
			ActualDiff									= (static_cast<ULONG>(Temp - TempOrig) >> 16) - (static_cast<ULONG>(Diff) >> 16);

			//
			// 1 = mark the relocation as needing an increment if it is relocated again.
			// 2 = mark the relocation as cannot be reprocessed.
			//
			if(ActualDiff == 1)
				*(NextOffset - 1)						|= 1;
			else
				*(NextOffset - 1)						|= 2;
			break;

			//
			// JumpAddress - (32-bits) relocate a MIPS jump address.
			//
		case IMAGE_REL_BASED_MIPS_JMPADDR :
			Temp										= (*static_cast<PULONG>(FixupVA) & 0x3ffffff) << 2;
			Temp										+= static_cast<LONG>(Diff);
			*static_cast<PULONG>(FixupVA)				= (*static_cast<PULONG>(FixupVA) & ~0x3ffffff) | ((Temp >> 2) & 0x3ffffff);
			break;

		case IMAGE_REL_BASED_IA64_IMM64:
			//
			// align it to bundle address before fixing up the 64-bit immediate value of the movl instruction.
			//
			VA2											= reinterpret_cast<PULONG>(reinterpret_cast<ULONG>(FixupVA) & ~(15));
			Value64										= 0;

			//
			// Extract the lower 32 bits of IMM64 from bundle
			//
			EXT_IMM64(Value64,VA2+EMARCH_ENC_I17_IMM7B_INST_WORD_X,EMARCH_ENC_I17_IMM7B_SIZE_X,EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,EMARCH_ENC_I17_IMM7B_VAL_POS_X);
			EXT_IMM64(Value64,VA2+EMARCH_ENC_I17_IMM9D_INST_WORD_X,EMARCH_ENC_I17_IMM9D_SIZE_X,EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,EMARCH_ENC_I17_IMM9D_VAL_POS_X);
			EXT_IMM64(Value64,VA2+EMARCH_ENC_I17_IMM5C_INST_WORD_X,EMARCH_ENC_I17_IMM5C_SIZE_X,EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,EMARCH_ENC_I17_IMM5C_VAL_POS_X);
			EXT_IMM64(Value64,VA2+EMARCH_ENC_I17_IC_INST_WORD_X,EMARCH_ENC_I17_IC_SIZE_X,EMARCH_ENC_I17_IC_INST_WORD_POS_X,EMARCH_ENC_I17_IC_VAL_POS_X);
			EXT_IMM64(Value64,VA2+EMARCH_ENC_I17_IMM41a_INST_WORD_X,EMARCH_ENC_I17_IMM41a_SIZE_X,EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,EMARCH_ENC_I17_IMM41a_VAL_POS_X);

			//
			// update 64-bit address
			//
			Value64										+= Diff;

			//
			// insert IMM64 into bundle
			//
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM7B_INST_WORD_X),EMARCH_ENC_I17_IMM7B_SIZE_X,EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,EMARCH_ENC_I17_IMM7B_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM9D_INST_WORD_X),EMARCH_ENC_I17_IMM9D_SIZE_X,EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,EMARCH_ENC_I17_IMM9D_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM5C_INST_WORD_X),EMARCH_ENC_I17_IMM5C_SIZE_X,EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,EMARCH_ENC_I17_IMM5C_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IC_INST_WORD_X),EMARCH_ENC_I17_IC_SIZE_X,EMARCH_ENC_I17_IC_INST_WORD_POS_X,EMARCH_ENC_I17_IC_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM41a_INST_WORD_X),EMARCH_ENC_I17_IMM41a_SIZE_X,EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,EMARCH_ENC_I17_IMM41a_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM41b_INST_WORD_X),EMARCH_ENC_I17_IMM41b_SIZE_X,EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,EMARCH_ENC_I17_IMM41b_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_IMM41c_INST_WORD_X),EMARCH_ENC_I17_IMM41c_SIZE_X,EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,EMARCH_ENC_I17_IMM41c_VAL_POS_X);
			INS_IMM64(Value64,(VA2+EMARCH_ENC_I17_SIGN_INST_WORD_X),EMARCH_ENC_I17_SIGN_SIZE_X,EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,EMARCH_ENC_I17_SIGN_VAL_POS_X);
			break;

			//
			// HighLow - (64-bits) relocate the 64bits address
			//
		case IMAGE_REL_BASED_DIR64:
			*static_cast<LONGLONG UNALIGNED64*>(FixupVA)+= Diff;
			break;

		default :
			//
			// Illegal - illegal relocation type.
			//
			return 0;
		}

		NextOffset										+= 1;
	}

	return reinterpret_cast<PIMAGE_BASE_RELOCATION>(NextOffset);
}

//
// relocate image
//
ULONG LdrRelocateImageWithBias(__in PVOID NewBase,__in LONGLONG Bias,__in PCHAR LoaderName,__in ULONG Success,__in ULONG Conflict,__in ULONG Invalid)
{
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(NewBase);
	ARC_STATUS Status									= Success;
	PVOID OldBase										= 0;
	PIMAGE_BASE_RELOCATION NextBlock					= 0;
	PUSHORT NextOffset									= 0;
	LONGLONG Diff										= Bias;
	ULONG SizeOfBlock									= 0;
	__try
	{
		//
		// unable to get nt header
		//
		if(!NtHeaders)
			try_leave(Status = Invalid);

		//
		// read oldbase from header
		//
		OldBase											= reinterpret_cast<PVOID>(NtHeaders->OptionalHeader.ImageBase);
		Diff											= reinterpret_cast<LONGLONG>(NewBase) - reinterpret_cast<LONGLONG>(OldBase) + Bias;

		//
		// locate the relocation section.
		//
		ULONG TotalCountBytes;
		PVOID Temp										= RtlImageDirectoryEntryToData(NewBase,TRUE,IMAGE_DIRECTORY_ENTRY_BASERELOC,&TotalCountBytes);

		//
		// the image does not contain a relocation table, and therefore cannot be relocated.
		//
		if(!Temp || !TotalCountBytes)
		{
			//
			// if the relocation info is stripped,return conflict
			//
			if(NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
			{
				DbgPrint("%s: Image can't be relocated, no fixup information.\n",LoaderName);
				Status									= Conflict;
			}
			else
			{
				Status									= Success;
			}

			try_leave(NOTHING);
		}

		//
		// if the image has a relocation table, then apply the specified fixup information to the image.
		//
		NextBlock										= static_cast<PIMAGE_BASE_RELOCATION>(Temp);
		while(TotalCountBytes)
		{
			SizeOfBlock									= NextBlock->SizeOfBlock;
			TotalCountBytes								-= SizeOfBlock;
			SizeOfBlock									-= sizeof(IMAGE_BASE_RELOCATION);
			SizeOfBlock									/= sizeof(USHORT);
			NextOffset									= Add2Ptr(NextBlock,sizeof(IMAGE_BASE_RELOCATION),PUSHORT);
			ULONG VA									= Add2Ptr(NewBase,NextBlock->VirtualAddress,ULONG);
			NextBlock									= LdrProcessRelocationBlockLongLong(VA,SizeOfBlock,NextOffset,Diff);

			if(!NextBlock)
				try_leave(DbgPrint("%s: Unknown base relocation type\n",LoaderName);Status = Invalid);
		}
	}
	__finally
	{
		if(Status != Success)
		{
			USHORT NextOffsetValue						= NextOffset ? *NextOffset : 0;
			DbgPrint("%s: %s() failed 0x%lx\n"
					 "%s: OldBase     : %p\n"
					 "%s: NewBase     : %p\n"
					 "%s: Diff        : 0x%I64x\n"
					 "%s: NextOffset  : %p\n"
					 "%s: *NextOffset : 0x%x\n"
					 "%s: SizeOfBlock : 0x%lx\n",
					 LoaderName,__FUNCTION__,Status,LoaderName,reinterpret_cast<PVOID>(OldBase),LoaderName,NewBase,LoaderName,Diff,
					 LoaderName,NextOffset,LoaderName,NextOffsetValue,LoaderName,SizeOfBlock);

			DbgBreakPoint();
		}
	}

	return Status;
}

//
// relocate image
//
ULONG LdrRelocateImage(__in PVOID NewBase,__in PCHAR LoaderName,__in ULONG Success,__in ULONG Conflict,__in ULONG Invalid)
{
	return LdrRelocateImageWithBias(NewBase,0,LoaderName,Success,Conflict,Invalid);
}

//
// load image
//
ARC_STATUS BlLoadImageEx(__in ULONG DeviceId,__in TYPE_OF_MEMORY MemoryType,__in PCHAR Path,__in USHORT ImageMachineType,
						 __in_opt ULONG ForceLoadBasePage,__in_opt ULONG Alignment,__out PVOID* LoadedImageBase)
{
	BOOLEAN CacheInitialized							= FALSE;
	ARC_STATUS Status									= ESUCCESS;
	IMAGE_CACHE ImageCache;
	ULONG FileId										= 0;
	UCHAR LocalBuffer[SECTOR_SIZE * 2 + 256];
	PVOID LocalBufferPointer							= ALIGN_BUFFER(LocalBuffer);

	//
	// caller did not give us an alignment,use 1
	//
	if(!Alignment)
		Alignment										= 0;

	__try
	{
		//
		// open image file
		//
		Status											= BlOpen(DeviceId,Path,ArcOpenReadOnly,&FileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// initialize cache if we are not booting from net
		//
		if(!BlBootingFromNet && BlImageInitCache(&ImageCache,FileId) == ESUCCESS)
			CacheInitialized							= TRUE;

		//
		// seek file to beginning
		//
		LARGE_INTEGER SeekOffset;
		SeekOffset.QuadPart								= 0;
		Status											= BlSeek(FileId,&SeekOffset,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// read first two sectors
		//
		ULONG Count									= 0;
		Status											= BlImageRead(&ImageCache,FileId,LocalBufferPointer,SECTOR_SIZE * 2,&Count);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		PIMAGE_NT_HEADERS NtHeader						= RtlImageNtHeader(LocalBufferPointer);

		//
		// check machine type
		//
		if(NtHeader->FileHeader.Machine != ImageMachineType)
			try_leave(Status = EBADF);

		//
		// must be a image
		//
		if(!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
			try_leave(Status = EBADF);

		//
		// if we are asking to load this image to a specified address,use it
		// otherwise we load it to the image base field in optional header iff the image base is a kernel space address
		//
		ULONG BasePage;
		if(ForceLoadBasePage)
			BasePage									= ForceLoadBasePage;
		else if(Alignment == 1 && NtHeader->OptionalHeader.ImageBase & 0x80000000)
			BasePage									= (NtHeader->OptionalHeader.ImageBase & 0x1fffffff) >> PAGE_SHIFT;
		else
			BasePage									= 0;

		//
		// strip the last .debug section
		//
		ULONG NumberOfSections							= NtHeader->FileHeader.NumberOfSections;
		ULONG PageCount									= NtHeader->OptionalHeader.SizeOfImage;
		PIMAGE_SECTION_HEADER SectionHeader				= IMAGE_FIRST_SECTION(NtHeader);
		if(!strcmp(reinterpret_cast<PCHAR>(SectionHeader[NumberOfSections - 1].Name),".debug"))
		{
			NumberOfSections							-= 1;
			PageCount									-= SectionHeader[NumberOfSections].SizeOfRawData;
		}

		PageCount										= BYTES_TO_PAGES(PageCount);

		//
		// try to allocate a memory allocation descriptor
		//
		ULONG ActualBasePage;
		Status											= BlAllocateAlignedDescriptor(MemoryType,BasePage,PageCount,Alignment,&ActualBasePage);

		//
		// free our image cache memory,force alignment to one and try again
		//
		if(Status != ESUCCESS && CacheInitialized)
		{
			BlImageFreeCache(&ImageCache);
			CacheInitialized							= FALSE;
			Status										= BlAllocateAlignedDescriptor(MemoryType,BasePage,PageCount,1,&ActualBasePage);
		}

		//
		// fail to allocate memory
		//
		if(Status != ESUCCESS)
			try_leave(Status = ENOMEM);

		//
		// actual image base,kernel address
		//
		PVOID ImageBase									= MAKE_KERNEL_ADDRESS_PAGE(ActualBasePage);

		//
		// seek to begin
		//
		SeekOffset.QuadPart								= 0;
		Status											= BlImageSeek(&ImageCache,FileId,&SeekOffset,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// read image headers to final image buffer
		//
		Status											= BlImageRead(&ImageCache,FileId,ImageBase,NtHeader->OptionalHeader.SizeOfHeaders,&Count);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// use image buffer instead of local buffer
		//
		NtHeader										= RtlImageNtHeader(ImageBase);
		SectionHeader									= IMAGE_FIRST_SECTION(NtHeader);

		//
		// how many bytes we copied into memory
		//
		ULONG RelocSize									= 0;

		//
		// checksum file headers
		//
		USHORT PartialSum								= ChkSum(0,ImageBase,NtHeader->OptionalHeader.SizeOfHeaders / sizeof(USHORT));

		//
		// load all sections
		//
		for(ULONG i = 0; i < NumberOfSections; i ++,SectionHeader ++)
		{
			//
			// round to even so that checksum works
			//
			ULONG VirtualSize							= (SectionHeader->Misc.VirtualSize + 1) & ~1;
			ULONG SizeOfRawData							= (SectionHeader->SizeOfRawData + 1) & ~1;

			//
			// if the virtual size is zero,then use raw size instead
			//
			if(!VirtualSize)
				VirtualSize								= SizeOfRawData;

			//
			// SizeOfRawData can be non-zero even if PointerToRawData is zero,fix it
			// and don't load more from image than is expected in memory
			//
			if(!SectionHeader->PointerToRawData)
				SizeOfRawData							= 0;
			else if(SizeOfRawData > VirtualSize)
				SizeOfRawData							= VirtualSize;

			//
			// load SizeOfRawData from file
			//
			if(SizeOfRawData)
			{
				//
				// seek to section data
				//
				SeekOffset.QuadPart						= SectionHeader->PointerToRawData;
				Status									= BlImageSeek(&ImageCache,FileId,&SeekOffset,SeekAbsolute);
				if(Status != ESUCCESS)
					try_leave(Status);

				//
				// read it
				//
				Status									= BlImageRead(&ImageCache,FileId,Add2Ptr(ImageBase,SectionHeader->VirtualAddress,PVOID),SizeOfRawData,&Count);
				if(Status != ESUCCESS)
					try_leave(Status);

				//
				// remember how far we have read.
				//
				RelocSize								= SectionHeader->PointerToRawData + SizeOfRawData;

				//
				// compute the check sum on the section.
				//
				PartialSum								= ChkSum(PartialSum,Add2Ptr(ImageBase,SectionHeader->VirtualAddress,PVOID),SizeOfRawData / sizeof(USHORT));
			}

			//
			// zero uninitialized data out
			//
			if(SizeOfRawData < VirtualSize)
				RtlZeroMemory(MAKE_KERNEL_ADDRESS(Add2Ptr(ImageBase,SizeOfRawData + SectionHeader->VirtualAddress,ULONG)),VirtualSize - SizeOfRawData);
		}

		//
		// only do the check sum if the image is stripped.
		//
		if(NtHeader->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
		{
			//
			// get the length of the file for check sum validation.
			//
			FILE_INFORMATION FileInfo;
			Status										= BlGetFileInformation(FileId,&FileInfo);
			Count										= 0;

			//
			// set the length to current end of file.
			//
			if(Status != ESUCCESS)
				FileInfo.EndingAddress.LowPart			= RelocSize;
			else
				Count									= FileInfo.EndingAddress.LowPart - RelocSize;

			while(Count != 0)
			{
				//
				// read in the rest of the image into local buffer an check sum it.
				//
				ULONG ReadCount							= Count < SECTOR_SIZE * 2 ? Count : SECTOR_SIZE * 2;;

				if(BlImageRead(&ImageCache,FileId,LocalBufferPointer,ReadCount,&ReadCount) != ESUCCESS || !ReadCount)
					break;

				PartialSum								= ChkSum(PartialSum,LocalBufferPointer,ReadCount / 2);
				Count									-= ReadCount;
			}

			PUSHORT AdjustSum							= reinterpret_cast<PUSHORT>(&NtHeader->OptionalHeader.CheckSum);
			PartialSum									-= (PartialSum < AdjustSum[0]);
			PartialSum									-= AdjustSum[0];
			PartialSum									-= (PartialSum < AdjustSum[1]);
			PartialSum									-= AdjustSum[1];

			//
			// bad checksum
			//
			if(static_cast<USHORT>(PartialSum + FileInfo.EndingAddress.LowPart) != NtHeader->OptionalHeader.CheckSum)
				try_leave(Status = EBADF);
		}

		if(BlVirtualBias)
		{
			//
			// reloc image with additional virtual bias
			//
			Status										= LdrRelocateImage(Add2Ptr(ImageBase,BlVirtualBias,PVOID),"OS Loader",ESUCCESS,0xffff0000 | EBADF,EBADF);

			//
			// got a confict,this image don't have relocation info,disable virtual bias and try with the normal base
			//
			if(Status == (0xffff0000 | EBADF))
			{
				Status									= ESUCCESS;
				BlVirtualBias							= 0;

				if(ImageBase != reinterpret_cast<PVOID>(NtHeader->OptionalHeader.ImageBase))
					Status								= LdrRelocateImage(ImageBase,"OS Loader",ESUCCESS,EBADF,EBADF);
			}
		}
		else if(ImageBase != reinterpret_cast<PVOID>(NtHeader->OptionalHeader.ImageBase))
		{
			//
			// if the loaded address is not the address in the file header,relocate it
			//
			Status										= LdrRelocateImage(ImageBase,"OS Loader",ESUCCESS,EBADF,EBADF);
		}

		//
		// setup output param
		//
		*LoadedImageBase								= Add2Ptr(ImageBase,BlVirtualBias,PVOID);

		//
		// send to debugger a notification,it can load symbolc here
		//
		if(!BdDebuggerEnabled)
			try_leave(NOTHING);

		DbgPrint("<?dml?><col fg=\"emphfg\">BD: %p %s</col>\n",*LoadedImageBase,Path);

		STRING Name;
		RtlInitAnsiString(&Name,Path);

		DbgLoadImageSymbols(&Name,*LoadedImageBase,-1);
	}
	__finally
	{
		if(CacheInitialized)
			BlImageFreeCache(&ImageCache);

		if(FileId)
			BlClose(FileId);
	}

	return Status;
}

//
// allocate a data table entry
//
ARC_STATUS BlAllocateDataTableEntry(__in PCHAR BaseDllName,__in PCHAR FullDllName,__in PVOID Base,__out PLDR_DATA_TABLE_ENTRY *AllocatedEntry)
{
	//
	// allocate a data table entry.
	//
	PLDR_DATA_TABLE_ENTRY DataTableEntry				= static_cast<PLDR_DATA_TABLE_ENTRY>(BlAllocateHeap(sizeof(LDR_DATA_TABLE_ENTRY)));
	if(!DataTableEntry)
		return ENOMEM;

	//
	// initialize the address of the DLL image file header and the entry point address.
	//
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(Base);
	DataTableEntry->DllBase								= Base;
	DataTableEntry->SizeOfImage							= NtHeaders->OptionalHeader.SizeOfImage;
	DataTableEntry->EntryPoint							= Add2Ptr(Base,NtHeaders->OptionalHeader.AddressOfEntryPoint,PVOID);
	DataTableEntry->SectionPointer						= 0;
	DataTableEntry->CheckSum							= NtHeaders->OptionalHeader.CheckSum;

	//
	// compute the length of the base DLL name, allocate a buffer to hold the name, copy the name into the buffer, and initialize the base DLL string descriptor.
	//
	USHORT Length										= static_cast<USHORT>(strlen(BaseDllName) * sizeof(WCHAR));
	PWCHAR Buffer										= static_cast<PWSTR>(BlAllocateHeap(Length + sizeof(WCHAR)));
	if(!Buffer)
		return ENOMEM;

	DataTableEntry->BaseDllName.Length					= Length;
	DataTableEntry->BaseDllName.MaximumLength			= Length + sizeof(WCHAR);
	DataTableEntry->BaseDllName.Buffer					= Buffer;

	while(*BaseDllName)
		*Buffer ++										= *BaseDllName ++;

	//
	// compute the length of the full DLL name, allocate a buffer to hold the name, copy the name into the buffer, and initialize the full DLL string descriptor.
	//
	Length												= static_cast<USHORT>(strlen(FullDllName) * sizeof(WCHAR));
	Buffer												= static_cast<PWSTR>(BlAllocateHeap(Length + sizeof(WCHAR)));
	if(!Buffer)
		return ENOMEM;

	DataTableEntry->FullDllName.Length					= Length;
	DataTableEntry->FullDllName.MaximumLength			= Length + sizeof(WCHAR);
	DataTableEntry->FullDllName.Buffer					= Buffer;

	while(*FullDllName)
		*Buffer ++										= *FullDllName ++;

	//
	// initialize the flags, load count, and insert the data table entry in the loaded module list.
	//
	DataTableEntry->Flags								= LDRP_ENTRY_PROCESSED;
	DataTableEntry->LoadCount							= 1;
	InsertTailList(&BlLoaderBlock->LoadOrderListHead,&DataTableEntry->InLoadOrderLinks);

	*AllocatedEntry										= DataTableEntry;

	return ESUCCESS;
}

//
// compare a null-terminated ansi string with unicode string
//
BOOLEAN BlpCompareDllName(__in PCHAR DllName,__in PUNICODE_STRING UnicodeString)
{
	//
	// if the DLL Name is longer, the strings are not equal.
	//
	ULONG Length										= strlen(DllName);
	if(Length * sizeof(WCHAR) > UnicodeString->Length)
		return FALSE;

	//
	// compare the two strings case insensitive, ignoring the Unicode string's extension.
	//
	PWCHAR Buffer										= UnicodeString->Buffer;
	for(USHORT Index = 0; Index < Length; Index += 1)
	{
		if(toupper(*DllName) != toupper(static_cast<CHAR>(*Buffer)))
			return FALSE;

		DllName											+= 1;
		Buffer											+= 1;
	}

	//
	// Strings match exactly or match up until the UnicodeString's extension.
	//
	if(UnicodeString->Length == Length * sizeof(WCHAR) || *Buffer == L'.')
		return(TRUE);

	return FALSE;
}

//
// reference data entry by name
//
BOOLEAN BlCheckForLoadedDll (__in PCHAR DllName,__out PLDR_DATA_TABLE_ENTRY *FoundEntry)
{
	//
	// scan the loaded data table list to determine if the specified DLL has already been loaded.
	//
	PLIST_ENTRY NextEntry								= BlLoaderBlock->LoadOrderListHead.Flink;
	while(NextEntry != &BlLoaderBlock->LoadOrderListHead)
	{
		PLDR_DATA_TABLE_ENTRY DataTableEntry			= CONTAINING_RECORD(NextEntry,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);

		if(BlpCompareDllName(DllName, &DataTableEntry->BaseDllName))
		{
			*FoundEntry									= DataTableEntry;

			//
			// reference it
			//
			DataTableEntry->LoadCount					+= 1;

			return TRUE;
		}

		NextEntry										= NextEntry->Flink;
	}

	return FALSE;
}

//
// set import thunk data
//
ARC_STATUS BlpBindImportName(__in PVOID DllBase,__in PVOID ImageBase,__in PIMAGE_THUNK_DATA ThunkEntry,
							 __in PIMAGE_EXPORT_DIRECTORY ExportDirectory,__in ULONG ExportSize,__in BOOLEAN SnapForwarder)
{
	if(!DllBase)
		DllBase											= reinterpret_cast<PVOID>(OsLoaderBase);

	//
	// if the reference is by ordinal, then compute the ordinal number.otherwise, lookup the import name in the export directory.
	//
	ULONG Ordinal										= 0;
	if(IMAGE_SNAP_BY_ORDINAL(ThunkEntry->u1.Ordinal) && !SnapForwarder)
	{
		//
		// compute the ordinal.
		//
		Ordinal											= IMAGE_ORDINAL(ThunkEntry->u1.Ordinal) - ExportDirectory->Base;
	}
	else
	{
		//
		// change AddressOfData from an RVA to a VA.
		//
		if(!SnapForwarder)
			ThunkEntry->u1.AddressOfData				= Add2Ptr(ImageBase,ThunkEntry->u1.AddressOfData,ULONG);

		//
		// lookup the import name in the export table to determine the ordinal.
		//
		PULONG NameTable								= Add2Ptr(DllBase,ExportDirectory->AddressOfNames,PULONG);
		PUSHORT OrdinalTable							= Add2Ptr(DllBase,ExportDirectory->AddressOfNameOrdinals,PUSHORT);

		//
		// if the hint index is within the limits of the name table and the import and export names match,
		// then the ordinal number can be obtained directly from the ordinal table.
		// otherwise, the name table must be searched for the specified name.
		//
		PIMAGE_IMPORT_BY_NAME AddressOfData				= reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(ThunkEntry->u1.AddressOfData);
		LONG Result;
		ULONG HintIndex									= AddressOfData->Hint;
		if(HintIndex < ExportDirectory->NumberOfNames && !strcmp(reinterpret_cast<PCHAR>(AddressOfData->Name),Add2Ptr(DllBase,NameTable[HintIndex],PCHAR)))
		{
			//
			// get the ordinal number from the ordinal table.
			//
			Ordinal										= OrdinalTable[HintIndex];
		}
		else
		{
			//
			// lookup the import name in the name table using a binary search.
			//
			LONG Low									= 0;
			LONG High									= ExportDirectory->NumberOfNames - 1;
			LONG Middle									= 0;
			while(High >= Low)
			{
				//
				// compute the next probe index and compare the import name with the export name entry.
				//
				Middle									= (Low + High) >> 1;
				LONG Result								= strcmp(reinterpret_cast<PCHAR>(AddressOfData->Name),Add2Ptr(DllBase,NameTable[Middle],PCHAR));

				if(Result < 0)
					High								= Middle - 1;
				else if(Result > 0)
					Low									= Middle + 1;
				else
					break;
			}

			//
			// if the high index is less than the low index, then a matching table entry was not found.otherwise, get the ordinal number from the ordinal table.
			//
			if(High < Low)
				return EINVAL;

			Ordinal										= OrdinalTable[Middle];
		}
	}

	//
	// if the ordinal number is valid, then bind the import reference and return success.otherwise, return an unsuccessful status.
	//
	if(Ordinal >= ExportDirectory->NumberOfFunctions)
		return EINVAL;

	PULONG FunctionTable								= Add2Ptr(DllBase,ExportDirectory->AddressOfFunctions,PULONG);
	ThunkEntry->u1.Function								= Add2Ptr(DllBase,FunctionTable[Ordinal],ULONG);

	//
	// check for a forwarder.
	//
	if(ThunkEntry->u1.Function > reinterpret_cast<ULONG>(ExportDirectory) && ThunkEntry->u1.Function < Add2Ptr(ExportDirectory,ExportSize,ULONG))
	{
		CHAR ForwardDllName[10];
		RtlCopyMemory(ForwardDllName,reinterpret_cast<PVOID>(ThunkEntry->u1.Function),sizeof(ForwardDllName));

		*strchr(ForwardDllName,'.')						= '\0';

		//
		// should load the referenced DLL here, just return failure for now.
		//
		PLDR_DATA_TABLE_ENTRY DataTableEntry			= 0;
		if(!BlCheckForLoadedDll(ForwardDllName,&DataTableEntry))
			return EINVAL;

		ULONG TargetExportSize							= 0;
		PVOID Temp										= RtlImageDirectoryEntryToData(DataTableEntry->DllBase,TRUE,IMAGE_DIRECTORY_ENTRY_EXPORT,&TargetExportSize);
		PIMAGE_EXPORT_DIRECTORY TargetExportDirectory	= static_cast<PIMAGE_EXPORT_DIRECTORY>(Temp);
		if(!TargetExportDirectory)
			return EINVAL;

		IMAGE_THUNK_DATA ThunkData;
		UCHAR Buffer[128];
		PCHAR ImportName								= strchr(reinterpret_cast<PCHAR>(ThunkEntry->u1.Function),'.') + 1;
		PIMAGE_IMPORT_BY_NAME AddressOfData				= reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(Buffer);
		RtlCopyMemory(AddressOfData->Name,ImportName,strlen(ImportName) + 1);
		AddressOfData->Hint								= 0;
		ThunkData.u1.AddressOfData						= reinterpret_cast<ULONG>(AddressOfData);
		ARC_STATUS Status								= BlpBindImportName(DataTableEntry->DllBase,ImageBase,&ThunkData,TargetExportDirectory,TargetExportSize,TRUE);
		ThunkEntry->u1									= ThunkData.u1;

		return Status;
	}

	return ESUCCESS;
}

//
// scan import address table and snap each reference
//
ARC_STATUS BlpScanImportAddressTable(__in PVOID DllBase,__in PVOID ImageBase,__in PIMAGE_THUNK_DATA ThunkTable)
{
	//
	// locate the export table in the image specified by the DLL base address.
	//
	PIMAGE_EXPORT_DIRECTORY ExportDirectory				= 0;
	ULONG Len											= 0;
	if(!DllBase)
		ExportDirectory									= reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(OsLoaderExports);
	else
		ExportDirectory									= static_cast<PIMAGE_EXPORT_DIRECTORY>(RtlImageDirectoryEntryToData(DllBase,TRUE,IMAGE_DIRECTORY_ENTRY_EXPORT,&Len));

	if(!ExportDirectory)
		return EBADF;

	//
	// scan the thunk table and bind each import reference.
	//
	while(ThunkTable->u1.AddressOfData)
	{
		ARC_STATUS Status								= BlpBindImportName(DllBase,ImageBase,ThunkTable++,ExportDirectory,Len,FALSE);
		if(Status != ESUCCESS)
			return Status;
	}

	return ESUCCESS;
}

//
// scan import table
//
ARC_STATUS BlScanImportDescriptorTable(__in PLDR_SCAN_IMPORT_SERACH_PATH_SET SearchPathSet,__in PLDR_DATA_TABLE_ENTRY ScanEntry,__in TYPE_OF_MEMORY MemoryType)
{
	//
	// locate the import table in the image specified by the data table entry.
	//
	PVOID Base											= ScanEntry->DllBase;
	ULONG Size											= 0;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor			= static_cast<PIMAGE_IMPORT_DESCRIPTOR>(RtlImageDirectoryEntryToData(Base,TRUE,IMAGE_DIRECTORY_ENTRY_IMPORT,&Size));

	//
	// if the image has an import directory, then scan the import table and load the specified DLLs.
	//
	if(!ImportDescriptor)
		return ESUCCESS;

	while(ImportDescriptor->Name && ImportDescriptor->FirstThunk)
	{
		//
		// change the name from an RVA to a VA.
		//
		PCHAR ImportName								= Add2Ptr(Base,ImportDescriptor->Name,PCHAR);

		//
		// if the DLL references itself, then skip the import entry.
		//
		if(!BlpCompareDllName(ImportName,&ScanEntry->BaseDllName))
		{
			//
			// if the DLL is not already loaded, then load the DLL and scan its import table.
			//
			PLDR_DATA_TABLE_ENTRY DllEntry				= 0;
			if(BlCheckForLoadedDll(ImportName,&DllEntry))
			{
				DllEntry->Flags							|= (ScanEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL);
			}
			else
			{
				//
				// seach those path
				//
				CHAR FullPath[0x100];
				ARC_STATUS Status						= ENOENT;
				PVOID LoadedBase						= 0;
				for(ULONG i = 0; i < SearchPathSet->SearchPathCount && Status != ESUCCESS; i ++)
				{
					strcpy(FullPath,SearchPathSet->SearchPath[i].Path);
					strcat(FullPath,SearchPathSet->PrefixPath);
					strcat(FullPath,ImportName);

					//
					// load the dll
					//
					Status								= BlLoadImageEx(SearchPathSet->SearchPath[i].DeviceId,MemoryType,FullPath,IMAGE_FILE_MACHINE_I386,0,0,&LoadedBase);
					if(Status == ESUCCESS)
						BlOutputLoadMessage(SearchPathSet->SearchPath[i].DevicePath,FullPath,0);
				}

				//
				// dll can not be found
				//
				if(Status != ESUCCESS)
					return Status;

				//
				// allocate data table entry for the dll
				//
				Status									= BlAllocateDataTableEntry(ImportName,FullPath,LoadedBase,&DllEntry);
				if(Status != ESUCCESS)
					return Status;

				//
				// scan loaded dll's import table
				//
				DllEntry->Flags							|= (ScanEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL);
				Status									= BlScanImportDescriptorTable(SearchPathSet,DllEntry,MemoryType);
				if(Status != ESUCCESS)
					return Status;

				//
				// reorder this dll
				//
				if(DllEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL)
				{
					RemoveEntryList(&DllEntry->InLoadOrderLinks);
					InsertTailList(&BlLoaderBlock->LoadOrderListHead,&DllEntry->InLoadOrderLinks);
				}
			}

			//
			// scan the import address table and snap links.
			//
			ARC_STATUS Status							= BlpScanImportAddressTable(DllEntry->DllBase,Base,Add2Ptr(Base,ImportDescriptor->FirstThunk,PIMAGE_THUNK_DATA));
			if(Status != ESUCCESS)
				return Status;
		}

		ImportDescriptor								+= 1;
	}

	return ESUCCESS;
}

#if _SCSI_SUPPORT_
//
// scan osloader's import table
//
ARC_STATUS BlScanOsloaderBoundImportTable(__in PLDR_DATA_TABLE_ENTRY ScanEntry)
{
	//
	// locate the import table in the image specified by the data table entry.
	//
	ULONG ImportTableSize;
	PVOID Temp											= RtlImageDirectoryEntryToData(ScanEntry->DllBase,TRUE,IMAGE_DIRECTORY_ENTRY_IMPORT,&ImportTableSize);
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor			= static_cast<PIMAGE_IMPORT_DESCRIPTOR>(Temp);

	//
	// if the image has an import directory, then scan the import table.
	//
	if(!ImportDescriptor)
		return ESUCCESS;

	while(ImportDescriptor->Name)
	{
		//
		// change the name from an RVA to a VA.
		//
		PCHAR ImportName								= Add2Ptr(ScanEntry->DllBase,ImportDescriptor->Name,PCHAR);

		//
		// if the DLL references itself, then skip the import entry.
		//
		if(!BlpCompareDllName(ImportName,&ScanEntry->BaseDllName))
		{
			//
			// scan the import address table and snap links.
			//
			PIMAGE_THUNK_DATA ThunkData					= Add2Ptr(ScanEntry->DllBase,ImportDescriptor->FirstThunk,PIMAGE_THUNK_DATA);
			ARC_STATUS Status							= BlpScanImportAddressTable(0,ScanEntry->DllBase,ThunkData);

			if(Status != ESUCCESS)
				return Status;
		}

		ImportDescriptor								+= 1;
	}

	return ESUCCESS;
}
#endif