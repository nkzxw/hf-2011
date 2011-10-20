/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

#ifndef IMAGE_SIZEOF_BASE_RELOCATION
#define IMAGE_SIZEOF_BASE_RELOCATION 8
#endif

extern PVOID RtlImageDirectoryEntryToData(
		IN PVOID ImageBase,
		IN BOOLEAN UnknownBoolean,
		IN ULONG ImageDirectoryEntry,
		OUT PULONG SectionSize);

PVOID LdrProcessRelocationBlockLongLong(
		IN PVOID RelocationBase,
		IN ULONG NumFixups,
		IN PSHORT Fixup,
		IN ULONG BaseAddressDifference);

static ULONG_PTR ImageRvaToBasedVa(
		IN PVOID View,
		IN ULONG Offset);
static ULONG_PTR ImageBasedVaToVa(
		IN PIMAGE_NT_HEADERS NtHeaders,
		IN PVOID Base,
		IN ULONG_PTR Va,
		OUT PIMAGE_SECTION_HEADER* LastSectionHeader OPTIONAL);
static ULONG_PTR ImageVaToBasedVa(
		IN PIMAGE_NT_HEADERS NtHeaders,
		IN PVOID Base,
		IN ULONG_PTR Va);

//
// Probes the supplied NT headers to make sure they are within valid user-mode
// addresses.
//
VOID LdrVerifyNtHeaders(
		IN PIMAGE_NT_HEADERS NtHeaders)
{
	PIMAGE_SECTION_HEADER Section;

	ProbeForRead(
			NtHeaders,
			sizeof(IMAGE_NT_HEADERS),
			1);

	//
	// Verify the section headers are sane.  NumberOfSections is a USHORT so no 
	// need to worry about a wrap.
	//
	Section = IMAGE_FIRST_SECTION(NtHeaders);

	ProbeForRead(
			Section,
			NtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
			1);
}

//
// Relocate the image at the provided base address
//
// Note: If this is called on a USER MAPPED IMAGE, then the caller MUST use
// SEH to guard against a malicious program unmapping the image under us and
// causing a fault here !!!
//
NTSTATUS LdrRelocateImage(
		IN PVOID ImageBase,
		IN ULONG ImageSize)
{
	PIMAGE_BASE_RELOCATION Relocation;
	PIMAGE_NT_HEADERS      NtHeaders;
	NTSTATUS               Status = STATUS_SUCCESS;
	ULONG                  BaseAddressDifference;
	ULONG                  RelocationSize;

	do
	{
		//
		// Verify that the image mapping is sane
		//
		ProbeForRead(
				ImageBase,
				ImageSize,
				1);

		//
		// Check to see if the image is somewhat valid
		//
		if (!(NtHeaders = RtlImageNtHeader(ImageBase)))
		{
			Status = STATUS_INVALID_IMAGE_FORMAT;
			break;
		}

		LdrVerifyNtHeaders(
				NtHeaders);

		//
		// Get a pointer to the relocation table
		//
		Relocation = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
				ImageBase,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_BASERELOC,
				&RelocationSize);

		//
		// No relocation information?
		//
		if ((!Relocation) ||
		    (!RelocationSize))
		{
			Status = STATUS_CONFLICTING_ADDRESSES;
			break;
		}

		ProbeForRead(
				Relocation,
				RelocationSize,
				1);

		//
		// Calculate the difference between the bases
		//
		BaseAddressDifference = (ULONG)ImageBase - NtHeaders->OptionalHeader.ImageBase;

		//
		// Enumerate through the relocations
		//
		while (RelocationSize)
		{
			//
			// Check to make sure the size of the relocation block is valid.
			//
			if ((Relocation->SizeOfBlock < 10) ||
			    (Relocation->SizeOfBlock > RelocationSize))
			{
				Status = STATUS_INVALID_IMAGE_FORMAT;
				break;
			}

			ProbeForRead(
					(PVOID)((ULONG_PTR)ImageBase + Relocation->VirtualAddress),
					Relocation->SizeOfBlock,
					1);

			//
			// Subtract the size of the relocation entry
			//
			RelocationSize -= Relocation->SizeOfBlock;

			//
			// Process the individual relocation
			//
			Relocation = LdrProcessRelocationBlockLongLong(
					(PVOID)(Relocation->VirtualAddress + (ULONG_PTR)ImageBase),
					(Relocation->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(USHORT),
					(PSHORT)((PUCHAR)Relocation + sizeof(IMAGE_BASE_RELOCATION)),
					BaseAddressDifference);

			if (!Relocation)
			{
				Status = STATUS_INVALID_IMAGE_FORMAT;
				break;
			}
		}

	} while (0);

	return Status;
}

//
// This function relocates an executable image that has not been mapped into
// memory as an image but rather has been raw-mapped into memory.
//
// This function must be called inside SEH guard.
//
NTSTATUS LdrRelocateRawImage(
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize)
{
	PIMAGE_BASE_RELOCATION Relocation;
	PIMAGE_NT_HEADERS      NtHeaders;
	NTSTATUS               Status = STATUS_SUCCESS;
	PVOID                  OldImageBase;
	ULONG                  BaseAddressDifference;
	ULONG                  RelocationSize;

	do
	{
		ProbeForRead(
				RawImageBase,
				RawImageSize,
				1);

		//
		// Check to see if the image is somewhat valid
		//
		if (!(NtHeaders = RtlImageNtHeader(RawImageBase)))
		{
			Status = STATUS_INVALID_IMAGE_FORMAT;
			break;
		}

		LdrVerifyNtHeaders(
				NtHeaders);

		//
		// Calculate the difference between the bases
		//
		BaseAddressDifference = (ULONG)RandomizedImageBase - NtHeaders->OptionalHeader.ImageBase;

		//
		// Save the old image base for TLS relocation
		//
		OldImageBase = (PVOID)NtHeaders->OptionalHeader.ImageBase;

		//
		// Change the base address to the raw image base
		//
		NtHeaders->OptionalHeader.ImageBase = (ULONG)RawImageBase;

		//
		// Get a pointer to the relocation table
		//
		Relocation = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
				RawImageBase,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_BASERELOC,
				&RelocationSize);

		Relocation = (PIMAGE_BASE_RELOCATION)ImageBasedVaToVa(
				NtHeaders,
				RawImageBase,
				(ULONG_PTR)Relocation,
				NULL);

		//
		// No relocation information?
		//
		if ((!Relocation) ||
		    (!RelocationSize))
		{
			Status = STATUS_CONFLICTING_ADDRESSES;
			break;
		}

		ProbeForRead(
				Relocation,
				RelocationSize,
				1);

		//
		// Enumerate through the relocations
		//
		while (RelocationSize)
		{
			PVOID RelocationBase = NULL;

			//
			// Don't allow bogus relocation blocks
			//
			if ((Relocation->SizeOfBlock < 10) ||
			    (Relocation->SizeOfBlock > RelocationSize))
			{
				Status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			//
			// Subtract the size of the relocation entry
			//
			RelocationSize -= Relocation->SizeOfBlock;

			//
			// Calculate the relocation base
			//
			RelocationBase = (PVOID)ImageBasedVaToVa(
					NtHeaders,
					RawImageBase,
					Relocation->VirtualAddress + NtHeaders->OptionalHeader.ImageBase,
					NULL);

			if (!RelocationBase)
				break;

			ProbeForRead(
					RelocationBase,
					Relocation->SizeOfBlock,
					1);

			//
			// Process the individual relocation
			//
			Relocation = LdrProcessRelocationBlockLongLong(
					RelocationBase,
					(Relocation->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(USHORT),
					(PSHORT)((PUCHAR)Relocation + sizeof(IMAGE_BASE_RELOCATION)),
					BaseAddressDifference);

			if (!Relocation)
			{
				Status = STATUS_INVALID_IMAGE_FORMAT;
				break;
			}
		}

	} while (0);

	//
	// If we were successful, try to relocate tls as needed
	//
	if (NT_SUCCESS(Status))
	{
		LdrRelocateTls(
				OldImageBase,
				RandomizedImageBase,
				RawImageBase,
				RawImageSize);
	}

	return Status;
}

//
// Process a relocation block
//
PVOID LdrProcessRelocationBlockLongLong(
		IN PVOID RelocationBase,
		IN ULONG NumFixups,
		IN PSHORT Fixup,
		IN ULONG BaseAddressDifference)
{
	LONGLONG FixupTemp64;
	PVOID    NextRelocationBase;
	LONG     FixupTemp32;

	if (!NumFixups)
		return RelocationBase;

	//
	// Walk all of the types for this relocation base
	//
	while (NumFixups--)
	{
		PVOID VirtualAddress = (PVOID)((*Fixup & 0x0fff) + (ULONG_PTR)RelocationBase);

		switch (*Fixup >> 12)
		{
			case IMAGE_REL_BASED_ABSOLUTE:
				break;
			case IMAGE_REL_BASED_HIGH:
				FixupTemp32               = (*(PUSHORT)VirtualAddress << 16) + BaseAddressDifference;
				*(PUSHORT)VirtualAddress  = (USHORT)(FixupTemp32 >> 16);
				break;
			case IMAGE_REL_BASED_LOW:
				FixupTemp32               = (*(PUSHORT)VirtualAddress) + (USHORT)BaseAddressDifference;
				*(PUSHORT)VirtualAddress  = (USHORT)FixupTemp32;
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				*(PLONG)VirtualAddress   += (LONG)BaseAddressDifference;
				break;
			case IMAGE_REL_BASED_HIGHADJ:
				FixupTemp32               = *(PUSHORT)VirtualAddress << 16;
				FixupTemp32              += (LONG)(*(PSHORT)(++Fixup)) + BaseAddressDifference + 0x8000;
				*(PUSHORT)VirtualAddress  = (USHORT)(FixupTemp32 >> 16);
				NumFixups--;
				break;
			case IMAGE_REL_BASED_MIPS_JMPADDR:
				FixupTemp32               = (*(PULONG)VirtualAddress & 0x3fffffff) << 2;
				FixupTemp32              += BaseAddressDifference;
				*(PULONG)VirtualAddress   = (*(PULONG)VirtualAddress & ~0x3fffffff) | ((FixupTemp32 >> 2) & 0x3fffffff);
				break;
			case IMAGE_REL_BASED_DIR64:
				*(PULONG_PTR)VirtualAddress += BaseAddressDifference;
				break;
#if 0
			case IMAGE_REL_BASED_HIGH3ADJ:
				FixupTemp64               = *(PUSHORT)VirtualAddress << 16;
				FixupTemp64              += (LONG)((SHORT)(Fixup[1]));
				FixupTemp64              <<= 16;
				FixupTemp64              += (LONG)((USHORT)(Fixup[0])) + BaseAddressDifference + 0x8000;
				FixupTemp64              >>= 16;
				FixupTemp64              += 0x8000;
				Fixup                    += 2;
				*(PUSHORT)VirtualAddress  = (USHORT)(FixupTemp64 >> 16);
				NumFixups                -= 2;
				break;
#endif
			case IMAGE_REL_BASED_SECTION:
			case IMAGE_REL_BASED_REL32:
			case IMAGE_REL_BASED_MIPS_JMPADDR16:
			//case IMAGE_REL_BASED_IA64_IMM64:
			default:
				DebugPrint(("LdrProcessRelocationBlockLongLong(): Unimplemented relocation type: %d.", *Fixup >> 12));
				ASSERT(0);
				break;
		}

		Fixup++;
	}

	return (PIMAGE_BASE_RELOCATION)Fixup;
}

static ULONG_PTR ImageRvaToBasedVa(
		IN PVOID View,
		IN ULONG Offset)
{
	PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader(View);

	if(!NtHeaders)
		return 0;

	return (ULONG_PTR)(NtHeaders->OptionalHeader.ImageBase + Offset);
}

static ULONG_PTR ImageBasedVaToVa(
		IN PIMAGE_NT_HEADERS NtHeaders,
		IN PVOID Base,
		IN ULONG_PTR Va,
		OUT PIMAGE_SECTION_HEADER* LastSectionHeader OPTIONAL)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG i;

	Section = IMAGE_FIRST_SECTION(NtHeaders);

	for(i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
		if(Va >= Section->VirtualAddress+NtHeaders->OptionalHeader.ImageBase
			&& Va < Section->VirtualAddress+NtHeaders->OptionalHeader.ImageBase+Section->Misc.VirtualSize) {
			if(LastSectionHeader)
				*LastSectionHeader = Section;
			return (ULONG_PTR)(((Va - NtHeaders->OptionalHeader.ImageBase) - Section->VirtualAddress) + Section->PointerToRawData + (ULONG_PTR)Base);
		}

		Section++;
	}

	return 0;
}

static ULONG_PTR RawImageBasedVaToVa(
		IN PIMAGE_NT_HEADERS NtHeaders,
		IN ULONG_PTR Base,
		IN ULONG_PTR Va,
		OUT PIMAGE_SECTION_HEADER* LastSectionHeader OPTIONAL)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG i;

	Section = IMAGE_FIRST_SECTION(NtHeaders);

	for(i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
		if(Va >= Section->VirtualAddress+Base
			&& Va < Section->VirtualAddress+Base+Section->Misc.VirtualSize) {
			if(LastSectionHeader)
				*LastSectionHeader = Section;
			return (ULONG_PTR)(((Va - Base) - Section->VirtualAddress) + Section->PointerToRawData + (ULONG_PTR)Base);
		}

		Section++;
	}

	return 0;
}

static ULONG_PTR ImageVaToBasedVa(
		IN PIMAGE_NT_HEADERS NtHeaders,
		IN PVOID Base,
		IN ULONG_PTR Va)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG i;

	Section = IMAGE_FIRST_SECTION(NtHeaders);

	for(i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
		if(Va >= Section->PointerToRawData+(ULONG_PTR)Base && Va < Section->PointerToRawData+(ULONG_PTR)Base+Section->SizeOfRawData)
			return (Va - (ULONG_PTR)Base) + Section->VirtualAddress + NtHeaders->OptionalHeader.ImageBase - Section->PointerToRawData;

		Section++;
	}

	return 0;
}

//
// Relocates TLS entries if necessary.  For some reason certain images have
// relocation information yet they don't have relocation information that fixes
// up the TLS directories.  This code will check to see if the TLS directories
// have been relocated, and, if not, manually attempts to relocate their
// references.  I can't really fathom how this would ever happen in the real
// world, yet we managed to run into a case where it did with this Any2Icon.exe
// program.  Stupid.
//
NTSTATUS LdrRelocateTls(
		IN PVOID OldImageBase,
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize)
{
	PIMAGE_TLS_DIRECTORY TlsDirectory = NULL;
	PIMAGE_NT_HEADERS    NtHeaders;
	ULONG_PTR            OldImageBaseStart, OldImageBaseEnd;
	ULONG_PTR            NewImageBase = (ULONG_PTR)RandomizedImageBase;
	NTSTATUS             Status = STATUS_SUCCESS;
	ULONG                Entries = 0;
	ULONG                TlsSize = 0;

	do
	{
		//
		// Extract the image's NT header.  NT headers have already been verified 
		// at this point so we don't need to call LdrVerifyNtHeaders again.
		//
		if (!(NtHeaders = RtlImageNtHeader(
				RawImageBase)))
		{
			Status = STATUS_INVALID_IMAGE_FORMAT;
			break;
		}

		//
		// Initialize the old image start and end 
		//
		OldImageBaseStart = (ULONG_PTR)OldImageBase;
		OldImageBaseEnd   = OldImageBaseStart + NtHeaders->OptionalHeader.SizeOfImage;

		//
		// If the new image base is within the old image base we cannot safely
		// relocate TLS entries because we cannot perform the math operation that
		// determines if TLS stuff was fixed up by LdrRelocateImage
		//
		if ((NewImageBase >= OldImageBaseStart) &&
		    (NewImageBase <  OldImageBaseEnd))
		{
			DebugPrint(("LdrRelocateTls(): WARNING: Cannot relocate TLS due to merged region."));
			break;
		}

		//
		// Get a pointer to the TLS table if one exists
		//
		TlsDirectory = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
				RawImageBase,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_TLS,
				&TlsSize);

		// 
		// If the image doesn't have any TLS directories, then who cares.
		//
		if ((!TlsDirectory) ||
		    (!TlsSize))
			break;

		ProbeForRead(
				TlsDirectory,
				TlsSize,
				1);

		//
		// HMM: It seems like this needs to be here, but the one application
		// (Any2Icon.exe) breaks if it is.  Anyone care to explain? :)
		//
		//TlsDirectory = (PIMAGE_TLS_DIRECTORY)ImageBasedVaToVa(
		//		NtHeaders,
		//		RawImageBase,
		//		(ULONG_PTR)TlsDirectory,
		//		NULL);

		Entries = TlsSize / sizeof(IMAGE_TLS_DIRECTORY);

		//
		// Until we run out of entries, fixup the addresses that may be relative
		// to the old base address
		// 
		while (Entries-- > 0)
		{
			if (IsAddressInsideRegion(
					TlsDirectory[Entries].StartAddressOfRawData,
					OldImageBaseStart,
					OldImageBaseEnd))
				TlsDirectory[Entries].StartAddressOfRawData = 
					(TlsDirectory[Entries].StartAddressOfRawData - OldImageBaseStart) +
					NewImageBase;

			if (IsAddressInsideRegion(
					TlsDirectory[Entries].EndAddressOfRawData,
					OldImageBaseStart,
					OldImageBaseEnd))
				TlsDirectory[Entries].EndAddressOfRawData = 
					(TlsDirectory[Entries].EndAddressOfRawData - OldImageBaseStart) +
					NewImageBase;

			if (IsAddressInsideRegion(
					TlsDirectory[Entries].AddressOfIndex,
					OldImageBaseStart,
					OldImageBaseEnd))
				TlsDirectory[Entries].AddressOfIndex = 
					(TlsDirectory[Entries].AddressOfIndex - OldImageBaseStart) +
					NewImageBase;

			if (IsAddressInsideRegion(
					TlsDirectory[Entries].AddressOfCallBacks,
					OldImageBaseStart,
					OldImageBaseEnd))
				TlsDirectory[Entries].AddressOfCallBacks = 
					(TlsDirectory[Entries].AddressOfCallBacks - OldImageBaseStart) +
					NewImageBase;
		}

	} while (0);

	return Status;
}

//
// Gets the address of the exported procedure in the provided image.  This
// function must be called in a context that allows it to directly reference the
// ImageBase that's passed in.
//
NTSTATUS LdrGetProcAddress(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PSZ SymbolName,
		OUT PVOID *SymbolAddress)
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	PIMAGE_NT_HEADERS       NtHeaders;
	NTSTATUS                Status = STATUS_SUCCESS;
	PUSHORT                 ExportNameOrdinalsTable;
	PULONG                  FunctionsTable;
	ULONG                   FunctionsTableSize;
	ULONG                   OrdinalsTableSize;
	ULONG                   NamesTableSize;
	ULONG                   NumberOfNames;
	ULONG                   ExportDirectorySize;
	ULONG                   Index;
	PSZ                     *ExportNamesTable;

	ASSERT(SymbolAddress);

	//
	// Initialize the output pointer to NULL
	//
	*SymbolAddress = NULL;

	__try
	{
		//
		// Check to see if the image is somewhat valid
		//
		if (!(NtHeaders = RtlImageNtHeader(
				(PVOID)ImageBase)))
		{
			Status = STATUS_INVALID_IMAGE_FORMAT;
			__leave;
		}

		ProbeForRead(
				NtHeaders,
				sizeof(IMAGE_NT_HEADERS),
				1);

		//
		// Get a pointer to the export table
		//
		ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
				(PVOID)ImageBase,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_EXPORT,
				&ExportDirectorySize);

		//
		// If the image has no export directory, out
		//
		if (!ExportDirectory)
		{
			Status = STATUS_NOT_FOUND;
			__leave;
		}

		//
		// Capture the table offsets and sizes
		//
		ProbeForRead(
				ExportDirectory,
				sizeof(IMAGE_EXPORT_DIRECTORY),
				1);

		FunctionsTable          = (PULONG)(ImageBase + ExportDirectory->AddressOfFunctions);
		ExportNamesTable        = (PSZ *)(ImageBase + ExportDirectory->AddressOfNames);
		ExportNameOrdinalsTable = (PUSHORT)(ImageBase + ExportDirectory->AddressOfNameOrdinals);
		NumberOfNames           = ExportDirectory->NumberOfNames;

		Status = RtlULongMult(
				ExportDirectory->NumberOfFunctions,
				sizeof(ULONG),
				&FunctionsTableSize);

		if (!NT_SUCCESS(Status))
			__leave;

		Status = RtlULongMult(
				NumberOfNames,
				sizeof(ULONG),
				&OrdinalsTableSize);

		if (!NT_SUCCESS(Status))
			__leave;

		Status = RtlULongMult(
				NumberOfNames,
				sizeof(ULONG),
				&NamesTableSize);

		if (!NT_SUCCESS(Status))
			__leave;

		//
		// Probe the export tables to make sure they are valid
		//
		ProbeForRead(
				FunctionsTable,
				FunctionsTableSize,
				1);
		ProbeForRead(
				ExportNamesTable,
				NamesTableSize,
				1);
		ProbeForRead(
				ExportNameOrdinalsTable,
				OrdinalsTableSize,
				1);

		//
		// Enumerate all of the names
		//
		for (Index = 0;
		     Index < NumberOfNames;
		     Index++)
		{
			PSZ StringName = (PSZ)(ImageBase + ExportNamesTable[Index]);

			//
			// Make sure the string is safe to reference.  This doesn't verify the
			// whole string.
			//
			ProbeForRead(
					StringName,
					1,
					1);

			//
			// If the string does not match, continue.  This could throw an access
			// violation.
			//
			if (strcmp(
					StringName,
					(PSZ)SymbolName))
				continue;

			//
			// Make sure the address we're going to read from is valid
			//
			ProbeForRead(
					&FunctionsTable[ExportNameOrdinalsTable[Index]],
					sizeof(ULONG),
					1);

			*SymbolAddress = (PVOID)(ImageBase + (ULONG_PTR)FunctionsTable[ExportNameOrdinalsTable[Index]]);

			DebugPrint(("LdrGetProcAddress(): Symbol '%s' is at 0x%p.",
					SymbolName,
					*SymbolAddress));

			break;
		}

	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = GetExceptionCode();
	}

	//
	// Depending on whether or not we found the symbol
	//
	if (NT_SUCCESS(Status))
	{
		if (*SymbolAddress)
			Status = STATUS_SUCCESS;
		else
			Status = STATUS_NOT_FOUND;
	}

	return Status;
}

//
// Checks to see if the supplied image imports a given DLL by name.  This is
// done to allow for checking if images are importing ntoskrnl.exe, for 
// example.
//
BOOLEAN LdrCheckImportedDll(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PIMAGE_NT_HEADERS NtHeader,
		IN PSZ DllName)
{
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	BOOLEAN                  Imported = FALSE;
	ULONG                    Elements = 0;
	ULONG                    Index;
	ULONG                    Size;

	do
	{
		//
		// Check to see if there are any imports at all.
		//
		if (!(Size = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size))
			break;

		ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RawImageBasedVaToVa(
				NtHeader,
				ImageBase,
				ImageBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
				NULL);

		if ((!ImportDescriptor) || (!Size))
			break;

		ProbeForRead(
				ImportDescriptor,
				Size,
				1);

		//
		// Enumerate each element
		//
		for (Index = 0, Elements = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
		     Index < Elements;
		     Index++)
		{
			PSZ NameString = NULL;

			//
			// Null terminating descriptor?  Skip it.
			//
			if (!ImportDescriptor[Index].Characteristics)
				continue;
			else if (!ImportDescriptor[Index].Name)
				break;

			NameString = (PSZ)(RawImageBasedVaToVa(
					NtHeader,
					ImageBase,
					ImageBase + ImportDescriptor[Index].Name,
					NULL));

			//
			// Make sure the name string is within our range.
			//
			if (IsAddressOutOfRange(
					NameString,
					ImageBase,
					ImageSize))
				break;

			//
			// Compare the import name with the supplied DLL name
			//
			if (!_stricmp(
					DllName,
					NameString))
			{
				Imported = TRUE;
				break;
			}
		}

	} while (0);

	return Imported;
}

//
// This function checks to see if the supplied image mapping contains any
// incompatible image file sections that are known to cause problems with
// randomization, such as some packets (neolit).  It is assumed that
// LdrVerifyNtHeaders has already been called at this point.
//
BOOLEAN LdrCheckIncompatibleSections(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PIMAGE_NT_HEADERS NtHeader)
{
	PIMAGE_SECTION_HEADER Section;
	BOOLEAN               HasIncompatibleSections = FALSE;
	ULONG                 Index = 0, IncompatIndex = 0;
	PSZ                   IncompatibleSections[] =
	{
		".neolit",
		NULL
	};

	//
	// Walk all of the sections, trying to see if any are incompatible.
	//
	for (Section = IMAGE_FIRST_SECTION(NtHeader),
	     Index = 0;
	     Index < NtHeader->FileHeader.NumberOfSections;
	     Index++)
	{
		for (IncompatIndex = 0;
		     IncompatibleSections[IncompatIndex];
		     IncompatIndex++)
		{
			if (!strcmp(
					IncompatibleSections[IncompatIndex], 
					Section[Index].Name))
			{
				DebugPrint(("LdrCheckIncompatibleSections(): Detected incompatible section: %s.",
						Section[Index].Name));

				HasIncompatibleSections = TRUE;
				break;
			}

		}
	}

	return HasIncompatibleSections;
}
