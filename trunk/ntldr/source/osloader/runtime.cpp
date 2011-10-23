//********************************************************************
//	created:	11:8:2008   18:51
//	file:		runtime.cpp
//	author:		tiamo
//	purpose:	runtime lib
//********************************************************************

#include "stdafx.h"

//
// rtl assert
//
VOID RtlAssert(__in PVOID FailedAssertion,__in PVOID FileName,__in ULONG LineNumber,__in_opt PCHAR Message)
{
	BlPrint("\r\n*** Assertion failed %s in %s line %d\r\n",FailedAssertion,FileName,LineNumber);
	if(Message)
		BlPrint(Message);

	while(1);
}

//
// get value from name
// argument strings are specified as: ArgumentName=ArgumentValue
//
PCHAR BlGetArgumentValue(__in ULONG Argc,__in PCHAR Argv[],__in PCHAR ArgumentName)
{
	//
	// scan the argument strings until either a match is found or all of the strings have been scanned.
	//
	while(Argc > 0)
	{
		PCHAR String									= Argv[Argc - 1];
		if(String)
		{
			PCHAR Name									= ArgumentName;
			while(*Name && *String)
			{
				if(toupper(*Name) != toupper(*String))
					break;

				Name									+= 1;
				String									+= 1;
			}

			if(*Name == 0 && *String == '=')
				return									String + 1;

			Argc										-= 1;
		}
	}

	return 0;
}

//
// ansi string to unicode string
//
NTSTATUS RtlAnsiStringToUnicodeString(__out PUNICODE_STRING DestinationString,__in PANSI_STRING SourceString,__in BOOLEAN AllocateDestinationString)
{
	ULONG Length										= SourceString->Length * sizeof(WCHAR) + sizeof(WCHAR);
	if(Length > 0xffff)
		return STATUS_INVALID_PARAMETER_2;

	DestinationString->Length							= static_cast<USHORT>(Length - sizeof(WCHAR));
	if(AllocateDestinationString)
		return STATUS_NO_MEMORY;

	if(DestinationString->Length >= DestinationString->MaximumLength)
		return STATUS_BUFFER_OVERFLOW;

	for(USHORT i = 0; i < SourceString->Length; i ++)
	{
		DestinationString->Buffer[i]					= SourceString->Buffer[i];
	}

	DestinationString->Buffer[SourceString->Length]		= 0;

	return STATUS_SUCCESS;
}

//
// append unicode to string
//
NTSTATUS RtlAppendUnicodeToString(__inout PUNICODE_STRING Dest,__in PCWCH Source)
{
	if(!Source)
		return STATUS_SUCCESS;

	UNICODE_STRING SourceString;
	RtlInitUnicodeString(&SourceString,Source);

	if(SourceString.Length + Dest->Length > Dest->MaximumLength)
		return STATUS_BUFFER_TOO_SMALL;

	RtlCopyMemory(Dest->Buffer + Dest->Length / sizeof(WCHAR),SourceString.Buffer,SourceString.Length);
	Dest->Length										+= SourceString.Length;

	if(Dest->Length < Dest->MaximumLength)
		Dest->Buffer[Dest->Length / sizeof(WCHAR)]		= 0;

	return STATUS_SUCCESS;
}

//
// append unicode string to string
//
NTSTATUS RtlAppendUnicodeStringToString(__in PUNICODE_STRING Destination,__in PCUNICODE_STRING Source)

{
	USHORT n											= Source->Length;
	if(n)
	{
		if(n + Destination->Length > Destination->MaximumLength)
			return STATUS_BUFFER_TOO_SMALL;

		UNALIGNED WCHAR *dst							= &Destination->Buffer[Destination->Length / sizeof(WCHAR)];

		RtlMoveMemory(dst,Source->Buffer,n);

		Destination->Length								+= n;

		if(Destination->Length < Destination->MaximumLength)
			dst[n / sizeof(WCHAR) ]						= 0;
	}

	return STATUS_SUCCESS;
}

//
// compare unicode string
//
BOOLEAN RtlEqualUnicodeString(__in PCUNICODE_STRING String1,__in PCUNICODE_STRING String2,__in BOOLEAN CaseInSensitive)
{
	LONG n1												= String1->Length;
	LONG n2												= String2->Length;

	if(n1 == n2)
	{
		PWCHAR s1										= String1->Buffer;
		PWCHAR s2										= String2->Buffer;
		PWCHAR Limit									= Add2Ptr(s1,n1,PWCHAR);

		if(CaseInSensitive)
		{
			while(s1 < Limit)
			{
				WCHAR c1								= *s1++;
				WCHAR c2								= *s2++;

				if(c1 != c2 && RtlUpcaseUnicodeChar(c1) != RtlUpcaseUnicodeChar(c2))
					return FALSE;
			}

			return TRUE;
		}

		while(s1 < Limit)
		{
			WCHAR c1									= *s1++;
			WCHAR c2									= *s2++;
			if(c1 != c2)
				return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

//
// compare unicode string
//
LONG RtlCompareUnicodeString(__in PCUNICODE_STRING String1,__in PCUNICODE_STRING String2,__in BOOLEAN CaseInSensitive)
{
	PWCHAR s1											= String1->Buffer;
	PWCHAR s2											= String2->Buffer;
	USHORT n1											= String1->Length / sizeof(WCHAR);
	USHORT n2											= String2->Length / sizeof(WCHAR);

	while(n1 && n2)
	{
		WCHAR c1										= *s1++;
		WCHAR c2										= *s2++;

		if(CaseInSensitive)
		{
			c1											= RtlUpcaseUnicodeChar(c1);
			c2											= RtlUpcaseUnicodeChar(c2);
		}

		LONG cDiff										= static_cast<LONG>(c1) - static_cast<LONG>(c2);
		if(cDiff)
			return cDiff;

		n1												-= 1;
		n2												-= 1;
	}

	return n1 - n2;
}

//
// initialize bitmpa
//
VOID RtlInitializeBitMap(__in PRTL_BITMAP BitMapHeader,__in PULONG BitMapBuffer,__in ULONG SizeOfBitMap)
{
	BitMapHeader->SizeOfBitMap							= SizeOfBitMap;
	BitMapHeader->Buffer								= BitMapBuffer;
}

//
// clear all bits
//
VOID RtlClearAllBits(__in PRTL_BITMAP BitMapHeader)
{
	RtlZeroMemory(BitMapHeader->Buffer,((BitMapHeader->SizeOfBitMap + 31) / 32) * 4);
}

//
// compare memory
//
SIZE_T __declspec(naked) RtlCompareMemory(__in const VOID* Source1,__in const VOID* Source2,__in SIZE_T Length)
{
	__asm
	{
		push    esi
        push    edi
        cld
        mov     esi,[esp + 12]
        mov     edi,[esp + 16]

		mov     ecx,[esp + 20]
        shr     ecx,2
        jz      rcm20
        repe    cmpsd
        jnz     rcm40

rcm20:  mov     ecx,[esp + 20]
        and     ecx,3
        jz      rcm30
        repe    cmpsb
        jnz     rcm50

rcm30:  mov     eax,[esp + 20]
        pop     edi
        pop     esi
		retn	0x0c

rcm40:  sub     esi,4
        sub     edi,4
        mov     ecx,5
        repe    cmpsb

rcm50:  dec     esi
        sub     esi,[esp + 12]
        mov     eax,esi
        pop     edi
        pop     esi
		retn	0x0c
	}
}

//
// init ansi string
//
VOID __declspec(naked) RtlInitAnsiString(__out PANSI_STRING DestinationString,__in_opt PCSZ SourceString)
{
	__asm
	{
		push    edi
        mov     edi,[esp]+8+4
        mov     edx,[esp]+4+4
        mov     DWORD PTR [edx], 0
        mov     DWORD PTR [edx]+4, edi
        or      edi, edi
        jz      exit_out
        or      ecx, -1
        xor     eax, eax
        repne   scasb
        not     ecx
        mov     [edx]+2, cx
        dec     ecx
        mov     [edx], cx

exit_out:
		pop     edi
		ret		8
	}
}

//
// init unicode string
//
VOID __declspec(naked) RtlInitUnicodeString(__out PUNICODE_STRING DestinationString,__in_opt PCWCH SourceString)
{
	__asm
	{
		push    ebp
		mov     ebp, esp
		mov     ecx, [ebp+8]
		mov     eax, [ebp+0Ch]
		xor     edx, edx
		and     [ecx], dx
		test    eax, eax
		mov     [ecx+4], eax
		jnz     buffer_not_null

		and     [ecx+2], ax

	exit_proc:
		pop     ebp
		retn    8

	count_loop:
		inc     eax
		inc     eax
		inc     edx
		inc     edx

	buffer_not_null:
		cmp     word ptr [eax], 0
		jnz     short count_loop

		mov     [ecx], dx
		add     edx, 2
		mov     [ecx+2], dx
		jmp     short exit_proc
	}
}

//
// upcase unicode to multi byte N
//
NTSTATUS RtlUpcaseUnicodeToMultiByteN(__out PCHAR MultiByteString,__in ULONG MaxBytesInMultiByteString,__out_opt PULONG BytesInMultiByteString,
									  __in PWCH UnicodeString,__in ULONG BytesInUnicodeString)
{
	//
	// convert unicode byte count to character count.
	// byte count of multibyte string is equivalent to character count.
	//
	ULONG CharsInUnicodeString							= BytesInUnicodeString / sizeof(WCHAR);
	ULONG LoopCount										= CharsInUnicodeString < MaxBytesInMultiByteString ? CharsInUnicodeString : MaxBytesInMultiByteString;

	if(BytesInMultiByteString)
		*BytesInMultiByteString							= LoopCount;


	for(ULONG i = 0;i < LoopCount; i ++)
		MultiByteString[i]								= static_cast<UCHAR>(RtlUpcaseUnicodeChar(static_cast<UCHAR>(UnicodeString[i])));

	return STATUS_SUCCESS;
}

//
// unicode to multi byte
//
NTSTATUS RtlUnicodeToMultiByteN(__out PCHAR MultiByteString,__in ULONG MaxBytesInMultiByteString,__out_opt PULONG BytesInMultiByteString,
								__in PWCH UnicodeString,__in ULONG BytesInUnicodeString)
{
	//
	// convert Unicode byte count to character count.
	// byte count of multibyte string is equivalent to character count.
	//
	ULONG CharsInUnicodeString							= BytesInUnicodeString / sizeof(WCHAR);
	ULONG LoopCount										= CharsInUnicodeString < MaxBytesInMultiByteString ? CharsInUnicodeString : MaxBytesInMultiByteString;

	if(BytesInMultiByteString)
		*BytesInMultiByteString							= LoopCount;


	for(ULONG i = 0; i < LoopCount; i ++)
		MultiByteString[i]								= static_cast<CHAR>(UnicodeString[i]);

	return STATUS_SUCCESS;
}

//
// ansi to unicode
//
WCHAR RtlAnsiCharToUnicodeChar(__inout PUCHAR *SourceCharacter)
{
	WCHAR UnicodeCharacter							= static_cast<WCHAR>(**SourceCharacter);
	(*SourceCharacter)								+= 1;
	return UnicodeCharacter;
}

//
// upcase unicode char
//
WCHAR RtlUpcaseUnicodeChar(__in WCHAR SourceCharacter)
{
	return SourceCharacter >= L'a' && SourceCharacter <= L'z' ? SourceCharacter - (L'a' - L'A') : SourceCharacter;
}

//
// unicode string to integer
//
NTSTATUS RtlUnicodeStringToInteger(__in PCUNICODE_STRING String,__in_opt ULONG Base,__out PULONG Value)
{
	WCHAR Sign											= 0;
	ULONG Digit											= 0;
	ULONG Shift											= 0;
	PCWSTR s											= String->Buffer;
	ULONG nChars										= String->Length / sizeof(WCHAR);

	while(nChars-- && (Sign = *s++) <= ' ')
	{
		if(!nChars)
		{
			Sign										= 0;
			break;
		}
	}

	WCHAR c												= Sign;
	if(c == L'-' || c == L'+')
	{
		if(nChars)
		{
			nChars										-= 1;
			c											= *s++;
		}
		else
		{
			c											= 0;
		}
	}

	if(!Base)
	{
		Base											= 10;
		Shift											= 0;

		if(c == L'0')
		{
			if(nChars)
			{
				nChars									-= 1;
				c										= *s++;

				if(c == L'x')
				{
					Base								= 16;
					Shift								= 4;
				}
				else if(c == L'o')
				{
					Base								= 8;
					Shift								= 3;
				}
				else if(c == L'b')
				{
					Base								= 2;
					Shift								= 1;
				}
				else
				{
					nChars								+= 1;
					s									-= 1;
				}
			}

			if(nChars)
			{
				nChars									-= 1;
				c										= *s++;
			}
			else
			{
				c										= 0;
			}
		}
	}
	else
	{
		switch(Base)
		{
		case 16:
			Shift										= 4;
			break;

		case  8:
			Shift										= 3;
			break;

		case  2:
			Shift										= 1;
			break;

		case 10:
			Shift										= 0;
			break;

		default:
			return STATUS_INVALID_PARAMETER;
			break;
		}
	}

	ULONG Result										= 0;
	while(c)
	{
		if(c >= L'0' && c <= L'9')
			Digit										= c - L'0';
		else if(c >= L'A' && c <= L'F')
			Digit										= c - L'A' + 10;
		else if(c >= L'a' && c <= L'f')
			Digit										= c - L'a' + 10;
		else
			break;

		if(Digit >= Base)
			break;

		if(!Shift)
			Result										= (Base * Result) + Digit;
		else
			Result										= (Result << Shift) | Digit;

		if(!nChars)
			break;

		nChars											-= 1;
		c												= *s++;
	}

	if(Sign == L'-')
		Result											= static_cast<ULONG>(-static_cast<LONG>(Result));

	*Value												= Result;

	return STATUS_SUCCESS;
}

//
// unsupported decompress method
//
NTSTATUS RtlDecompressBufferNS(__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,__in PUCHAR CompressedBuffer,
							   __in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize)
{
	return STATUS_UNSUPPORTED_COMPRESSION;
}

//
// decompress buffer
//
NTSTATUS RtlDecompressBuffer(__in USHORT CompressionFormat,__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,
							 __in PUCHAR CompressedBuffer,__in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize)
{
	USHORT Format										= CompressionFormat & 0x00ff;

	//
	//  make sure the format is sort of supported
	//
	if(Format == 0/*COMPRESSION_FORMAT_NONE*/ || Format == 1/*COMPRESSION_FORMAT_DEFAULT*/)
		return STATUS_INVALID_PARAMETER;

	if(Format & 0x00f0)
		return STATUS_UNSUPPORTED_COMPRESSION;

	typedef NTSTATUS (*PRTL_DECOMPRESS_BUFFER)(__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,__in PUCHAR CompressedBuffer,
											   __in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize);

	extern NTSTATUS RtlDecompressBufferLZNT1(__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,__in PUCHAR CompressedBuffer,
											 __in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize);

	static PRTL_DECOMPRESS_BUFFER RtlDecompressBufferProcs[8] =
	{
		0,
		0,
		&RtlDecompressBufferLZNT1,
		&RtlDecompressBufferNS,
		&RtlDecompressBufferNS,
		&RtlDecompressBufferNS,
		&RtlDecompressBufferNS,
		&RtlDecompressBufferNS,
	};

	//
	// call the compression routine for the individual format
	//
	return RtlDecompressBufferProcs[Format](UncompressedBuffer,UncompressedBufferSize,CompressedBuffer,CompressedBufferSize,FinalUncompressedSize);
}

//
// unsupported
//
NTSTATUS RtlDescribeChunkNS(__inout PUCHAR *CompressedBuffer, __in PUCHAR EndOfCompressedBufferPlus1,__out PUCHAR *ChunkBuffer,__out PULONG ChunkSize)
{
	return STATUS_UNSUPPORTED_COMPRESSION;
}

//
// describe chunk
//
NTSTATUS RtlDescribeChunk(__in USHORT CompressionFormat,__inout PUCHAR *CompressedBuffer,__in PUCHAR EndOfCompressedBufferPlus1,
						  __out PUCHAR *ChunkBuffer,__out PULONG ChunkSize)
{
	USHORT Format										= CompressionFormat & 0x00ff;
	if(Format == 0/*COMPRESSION_FORMAT_NONE*/ || Format == 1/*COMPRESSION_FORMAT_DEFAULT*/)
		return STATUS_INVALID_PARAMETER;

	if(Format & 0x00f0)
		return STATUS_UNSUPPORTED_COMPRESSION;

	typedef NTSTATUS (*PRTL_DESCRIBE_CHUNK)(__inout PUCHAR *CompressedBuffer, __in PUCHAR EndOfCompressedBufferPlus1,__out PUCHAR *ChunkBuffer,__out PULONG ChunkSize);
	extern NTSTATUS RtlDescribeChunkLZNT1(__inout PUCHAR *CompressedBuffer, __in PUCHAR EndOfCompressedBufferPlus1,__out PUCHAR *ChunkBuffer,__out PULONG ChunkSize);

	static PRTL_DESCRIBE_CHUNK RtlDescribeChunkProcs[8] =
	{
		0,
		0,
		&RtlDescribeChunkLZNT1,
		&RtlDescribeChunkNS,
		&RtlDescribeChunkNS,
		&RtlDescribeChunkNS,
		&RtlDescribeChunkNS,
		&RtlDescribeChunkNS,
	};

	//
	// call the compression routine for the individual format
	//
	return RtlDescribeChunkProcs[Format](CompressedBuffer,EndOfCompressedBufferPlus1,ChunkBuffer,ChunkSize);
}

//
// HACK HACK HACK,put it into crt?
//
__int64 __cdecl _atoi64(const char *nptr)
{
	//
	// skip whitespace
	//
	while(isspace(static_cast<int>(static_cast<UCHAR>(*nptr))))
		nptr											+= 1;

	int c												= static_cast<int>(static_cast<UCHAR>(*nptr++));
	int sign											= c;
	if(c == '-' || c == '+')
		c												= static_cast<int>(static_cast<UCHAR>(*nptr++));

	__int64 total										= 0;

	while(isdigit(c))
	{
		total											= 10 * total + (c - '0');
		c												= static_cast<int>(static_cast<UCHAR>(*nptr++));
	}

	if(sign == '-')
		total											= -total;

	return total;
}

//
// calc crc32
//
ULONG RtlComputeCrc32(__in ULONG CrcValue,__in PVOID Buffer,__in ULONG Size)
{
	extern ULONG RtlCrc32Table[];
	PUCHAR BufferChar									= static_cast<PUCHAR>(Buffer);

	CrcValue											= ~CrcValue;

	for(ULONG i = 0; i < Size; i ++)
		CrcValue										= (CrcValue >> 8) ^ RtlCrc32Table[(CrcValue & 0xff) ^ BufferChar[i]];

	return ~CrcValue;
}

//
// crc32 table
//
ULONG RtlCrc32Table[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};