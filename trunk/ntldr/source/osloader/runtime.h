//********************************************************************
//	created:	11:8:2008   18:54
//	file:		runtime.h
//	author:		tiamo
//	purpose:	runtime lib
//********************************************************************

#pragma once

//
// get argument value
//
PCHAR BlGetArgumentValue(__in ULONG Argc,__in PCHAR Argv[],__in PCHAR ArgumentName);

//
// init ansi string
//
VOID RtlInitAnsiString(__out PANSI_STRING DestinationString,__in_opt PCSZ SourceString);

//
// init unicode string
//
VOID RtlInitUnicodeString(__out PUNICODE_STRING DestinationString,__in_opt PCWCH SourceString);

//
// assert
//
VOID RtlAssert(__in PVOID FailedAssertion,__in PVOID FileName,__in ULONG LineNumber,__in_opt PCHAR Message);

//
// ansi string to unicode string
//
NTSTATUS RtlAnsiStringToUnicodeString(__out PUNICODE_STRING DestinationString,__in PANSI_STRING SourceString,__in BOOLEAN AllocateDestinationString);

//
// append unicode to string
//
NTSTATUS RtlAppendUnicodeToString(__inout PUNICODE_STRING Destination,__in PCWCH Source);

//
// append unicode string to string
//
NTSTATUS RtlAppendUnicodeStringToString(__in PUNICODE_STRING Destination,__in PCUNICODE_STRING Source);

//
// compare unicode string
//
BOOLEAN RtlEqualUnicodeString(__in PCUNICODE_STRING String1,__in PCUNICODE_STRING String2,__in BOOLEAN CaseInSensitive);

//
// compare unicode string
//
LONG RtlCompareUnicodeString(__in PCUNICODE_STRING String1,__in PCUNICODE_STRING String2,__in BOOLEAN CaseInSensitive);

//
// upcase unicode to multi byte N
//
NTSTATUS RtlUpcaseUnicodeToMultiByteN(__out PCHAR MultiByteString,__in ULONG MaxBytesInMultiByteString,__out_opt PULONG BytesInMultiByteString,
									  __in PWCH UnicodeString,__in ULONG BytesInUnicodeString);

//
// unicode to multi byte
//
NTSTATUS RtlUnicodeToMultiByteN(__out PCHAR MultiByteString,__in ULONG MaxBytesInMultiByteString,__out_opt PULONG BytesInMultiByteString,
								__in PWCH UnicodeString,__in ULONG BytesInUnicodeString);

//
// ansi to unicode
//
WCHAR RtlAnsiCharToUnicodeChar(__inout PUCHAR *SourceCharacter);

//
// upcase unicode char
//
WCHAR RtlUpcaseUnicodeChar(__in WCHAR SourceCharacter);

//
// compute crc32
//
ULONG RtlComputeCrc32(__in ULONG Input,__in PVOID Buffer,__in ULONG Size);

//
// initialize bitmpa
//
VOID RtlInitializeBitMap(__in PRTL_BITMAP BitMapHeader,__in PULONG BitMapBuffer,__in ULONG SizeOfBitMap);

//
// clear all bits
//
VOID RtlClearAllBits(__in PRTL_BITMAP BitMapHeader);

//
// compare memory
//
SIZE_T RtlCompareMemory(__in const VOID* Source1,__in const VOID* Source2,__in SIZE_T Length);

//
// unicode string to integer
//
NTSTATUS RtlUnicodeStringToInteger(__in PCUNICODE_STRING String,__in_opt ULONG Base,__out PULONG Value);

//
// decompress buffer
//
NTSTATUS RtlDecompressBuffer(__in USHORT CompressionFormat,__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,
							 __in PUCHAR CompressedBuffer,__in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize);

//
// describe chunk
//
NTSTATUS RtlDescribeChunk(__in USHORT CompressionFormat,__inout PUCHAR *CompressedBuffer,__in PUCHAR EndOfCompressedBufferPlus1,
						  __out PUCHAR *ChunkBuffer,__out PULONG ChunkSize);