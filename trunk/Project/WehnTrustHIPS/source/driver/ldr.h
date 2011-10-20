/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_LDR_H
#define _WEHNTRUST_DRIVER_LDR_H

NTSTATUS LdrRelocateImage(
		IN PVOID ImageBase,
		IN ULONG ImageSize);
NTSTATUS LdrRelocateRawImage(
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize);

NTSTATUS LdrRelocateTls(
		IN PVOID OldImageBase,
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize);

NTSTATUS LdrGetProcAddress(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PSZ SymbolName,
		OUT PVOID *SymbolAddress);

BOOLEAN LdrCheckImportedDll(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PIMAGE_NT_HEADERS NtHeader,
		IN PSZ DllName);

BOOLEAN LdrCheckIncompatibleSections(
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize,
		IN PIMAGE_NT_HEADERS NtHeader);

#endif
