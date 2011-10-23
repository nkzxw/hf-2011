//********************************************************************
//	created:	18:9:2008   7:43
//	file: 		lznt1.cpp
//	author:		tiamo
//	purpose:	lzx compress/decompress
//********************************************************************

#include "stdafx.h"

#define FORMAT412										0
#define FORMAT511										1
#define FORMAT610										2
#define FORMAT79										3
#define FORMAT88										4
#define FORMAT97										5
#define FORMAT106										6
#define FORMAT115										7
#define FORMAT124										8

//
// max uncompressed chunk size
//
#define MAX_UNCOMPRESSED_CHUNK_SIZE						4096

//
// get compressed chunk size
//
#define GetCompressedChunkSize(CH)						((CH).Chunk.CompressedChunkSizeMinus3 + 3)

//
// get uncompressed chunk size
//
#define GetUncompressedChunkSize(CH)					MAX_UNCOMPRESSED_CHUNK_SIZE

#define GetLZNT1Length(F,CT) (                   \
    ( F == FORMAT412 ? (CT).Fields412.Length + 3 \
    : F == FORMAT511 ? (CT).Fields511.Length + 3 \
    : F == FORMAT610 ? (CT).Fields610.Length + 3 \
    : F == FORMAT79  ? (CT).Fields79.Length  + 3 \
    : F == FORMAT88  ? (CT).Fields88.Length  + 3 \
    : F == FORMAT97  ? (CT).Fields97.Length  + 3 \
    : F == FORMAT106 ? (CT).Fields106.Length + 3 \
    : F == FORMAT115 ? (CT).Fields115.Length + 3 \
    :                  (CT).Fields124.Length + 3 \
    )                                            \
)

#define GetLZNT1Displacement(F,CT) (                   \
    ( F == FORMAT412 ? (CT).Fields412.Displacement + 1 \
    : F == FORMAT511 ? (CT).Fields511.Displacement + 1 \
    : F == FORMAT610 ? (CT).Fields610.Displacement + 1 \
    : F == FORMAT79  ? (CT).Fields79.Displacement  + 1 \
    : F == FORMAT88  ? (CT).Fields88.Displacement  + 1 \
    : F == FORMAT97  ? (CT).Fields97.Displacement  + 1 \
    : F == FORMAT106 ? (CT).Fields106.Displacement + 1 \
    : F == FORMAT115 ? (CT).Fields115.Displacement + 1 \
    :                  (CT).Fields124.Displacement + 1 \
    )                                                  \
)

//
// chunk header
//
typedef union _COMPRESSED_CHUNK_HEADER
{
	struct
	{
		//
		// size - 3
		//
		USHORT											CompressedChunkSizeMinus3 : 12;

		//
		// signature
		//
		USHORT											ChunkSignature            :  3;

		//
		// compressed
		//
		USHORT											IsChunkCompressed         :  1;

	}Chunk;

	//
	// as ushort
	//
	USHORT												Short;

}COMPRESSED_CHUNK_HEADER,*PCOMPRESSED_CHUNK_HEADER;

//
// copy token
//
typedef union _LZNT1_COPY_TOKEN
{
	struct { USHORT Length : 12; USHORT Displacement :  4; } Fields412;
	struct { USHORT Length : 11; USHORT Displacement :  5; } Fields511;
	struct { USHORT Length : 10; USHORT Displacement :  6; } Fields610;
	struct { USHORT Length :  9; USHORT Displacement :  7; } Fields79;
	struct { USHORT Length :  8; USHORT Displacement :  8; } Fields88;
	struct { USHORT Length :  7; USHORT Displacement :  9; } Fields97;
	struct { USHORT Length :  6; USHORT Displacement : 10; } Fields106;
	struct { USHORT Length :  5; USHORT Displacement : 11; } Fields115;
	struct { USHORT Length :  4; USHORT Displacement : 12; } Fields124;

	UCHAR Bytes[2];

}LZNT1_COPY_TOKEN,*PLZNT1_COPY_TOKEN;

ULONG FormatMaxLength[]       = { 4098, 2050, 1026,  514,  258,  130,   66,   34,   18 };
ULONG FormatMaxDisplacement[] = {   16,   32,   64,  128,  256,  512, 1024, 2048, 4096 };

//
// decompress chunk
//
NTSTATUS LZNT1DecompressChunk(__out PUCHAR UncompressedBuffer,__in PUCHAR EndOfUncompressedBufferPlus1,__in PUCHAR CompressedBuffer,
							  __in PUCHAR EndOfCompressedBufferPlus1,__out PULONG FinalUncompressedChunkSize)
{
	//
	// the two pointers will slide through our input and input buffer.
	// for the input buffer we skip over the chunk header.
	//
	PUCHAR OutputPointer								= UncompressedBuffer;
	PUCHAR InputPointer									= CompressedBuffer;
	ULONG Format										= FORMAT412;

	//
	// the flag byte stores a copy of the flags for the current run and the flag bit denotes the current bit position within the flag that we are processing
	//
	UCHAR FlagByte										= *InputPointer ++;
	ULONG FlagBit										= 0;

	//
	// while we haven't exhausted either the input or output buffer we will do some more decompression
	//
	while(OutputPointer < EndOfUncompressedBufferPlus1 && InputPointer < EndOfCompressedBufferPlus1)
	{
		while(UncompressedBuffer + FormatMaxDisplacement[Format] < OutputPointer)
			Format										+= 1;

		//
		// check the current flag if it is zero then the current input token is a literal byte that we simply copy over to the output buffer
		//
		if(!(FlagByte & (1 << FlagBit)))
		{
			*OutputPointer ++							= *InputPointer ++;
		}
		else
		{
			//
			// the current input is a copy token so we'll get the copy token into our variable and extract the length and displacement from the token
			//
			if(InputPointer + 1 >= EndOfCompressedBufferPlus1)
			{
				*FinalUncompressedChunkSize				= reinterpret_cast<ULONG>(InputPointer);
				return STATUS_BAD_COMPRESSION_BUFFER;
			}

			//
			// grab the next input byte and extract the length and displacement from the copy token
			//
			LZNT1_COPY_TOKEN CopyToken;
			CopyToken.Bytes[0]							= *InputPointer ++;
			CopyToken.Bytes[1]							= *InputPointer ++;

			LONG Displacement							= GetLZNT1Displacement(Format,CopyToken);
			LONG Length									= GetLZNT1Length(Format,CopyToken);

			//
			// at this point we have the length and displacement from the copy token,
			// we need to make sure that the displacement doesn't send us outside the uncompressed buffer
			//
			if(Displacement > OutputPointer - UncompressedBuffer)
			{
				*FinalUncompressedChunkSize				= reinterpret_cast<ULONG>(InputPointer);
				return STATUS_BAD_COMPRESSION_BUFFER;
			}

			//
			// we also need to adjust the length to keep the copy from overflowing the output buffer
			//
			if(OutputPointer + Length >= EndOfUncompressedBufferPlus1)
				Length									= EndOfUncompressedBufferPlus1 - OutputPointer;

			//
			// we cannot use Rtl Move Memory here because it does the copy backwards from what the LZ algorithm needs.
			//
			while(Length > 0)
			{
				*OutputPointer							= *(OutputPointer - Displacement);
				Length									-= sizeof(UCHAR);
				OutputPointer							+= 1;
			}
		}

		//
		// before we go back to the start of the loop we need to adjust the flag bit value (it goes from 0, 1, ... 7)
		// and if the flag bit is back to zero we need to read in the next flag byte.
		// in this case we are at the end of the input buffer we'll just break out of the loop because we're done.
		//
		FlagBit											= (FlagBit + 1) % 8;

		if(!FlagBit)
		{
			if(InputPointer >= EndOfCompressedBufferPlus1)
				break;

			FlagByte									= *InputPointer ++;
		}
	}

	//
	// the decompression is done so now set the final uncompressed chunk size and return success to our caller
	//
	*FinalUncompressedChunkSize							= OutputPointer - UncompressedBuffer;

	return STATUS_SUCCESS;
}

//
// decompress buffer
//
NTSTATUS RtlDecompressBufferLZNT1(__out PUCHAR UncompressedBuffer,__in ULONG UncompressedBufferSize,__in PUCHAR CompressedBuffer,
								  __in ULONG CompressedBufferSize,__out PULONG FinalUncompressedSize)
{
	//
	// the following to variables are pointers to the byte following the end of each appropriate buffer.
	// this saves us from doing the addition for each loop check
	//
	PUCHAR EndOfUncompressedBuffer						= UncompressedBuffer + UncompressedBufferSize;
	PUCHAR EndOfCompressedBuffer						= CompressedBuffer + CompressedBufferSize;
	PUCHAR CompressedChunk								= CompressedBuffer;
	PUCHAR UncompressedChunk							= UncompressedBuffer;
	LONG UncompressedChunkSize							= 0;
	LONG CompressedChunkSize							= 0;
	LONG SavedChunkSize									= 0;
	NTSTATUS Status										= STATUS_SUCCESS;
	COMPRESSED_CHUNK_HEADER ChunkHeader;

	//
	// make sure that the compressed buffer is at least four bytes long to start with,
	// and then get the first chunk header and make sure it is not an ending chunk header.
	//
	ASSERT(CompressedChunk <= EndOfCompressedBuffer - 4);

	RtlCopyMemory(&ChunkHeader,CompressedChunk,sizeof(USHORT));

	ASSERT(ChunkHeader.Short);

	//
	// while there is space in the uncompressed buffer to store data we will loop through decompressing chunks
	//
	while(TRUE)
	{
		ASSERT(ChunkHeader.Chunk.ChunkSignature == 3);

		CompressedChunkSize								= GetCompressedChunkSize(ChunkHeader);

		//
		// check that the chunk actually fits in the buffer supplied by the caller
		//
		if(CompressedChunk + CompressedChunkSize > EndOfCompressedBuffer)
		{
			ASSERTMSG("CompressedBuffer is too small", FALSE);

			*FinalUncompressedSize						= reinterpret_cast<ULONG>(CompressedChunk);

			return STATUS_BAD_COMPRESSION_BUFFER;
		}

		//
		// first make sure the chunk contains compressed data
		//
		if(ChunkHeader.Chunk.IsChunkCompressed)
		{
			//
			// decompress a chunk and return if we get an error
			//
			Status										= LZNT1DecompressChunk(UncompressedChunk,EndOfUncompressedBuffer,CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),
																			   CompressedChunk + CompressedChunkSize,reinterpret_cast<PULONG>(&UncompressedChunkSize));
			if(!NT_SUCCESS(Status))
			{
				*FinalUncompressedSize					= UncompressedChunkSize;
				return Status;
			}
		}
		else
		{
			//
			// the chunk does not contain compressed data so we need to simply copy over the uncompressed data
			//
			UncompressedChunkSize						= GetUncompressedChunkSize(ChunkHeader);

			//
			// make sure the data will fit into the output buffer
			//
			if(UncompressedChunk + UncompressedChunkSize > EndOfUncompressedBuffer)
				UncompressedChunkSize					= EndOfUncompressedBuffer - UncompressedChunk;

			//
			// check that the compressed chunk has this many bytes to copy.
			//
			if(CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER) + UncompressedChunkSize > EndOfCompressedBuffer)
			{
				ASSERTMSG("CompressedBuffer is too small", FALSE);

				*FinalUncompressedSize					= reinterpret_cast<ULONG>(CompressedChunk);

				return STATUS_BAD_COMPRESSION_BUFFER;
			}

			RtlCopyMemory(UncompressedChunk,CompressedChunk + sizeof(COMPRESSED_CHUNK_HEADER),UncompressedChunkSize);
		}

		//
		// update the compressed and uncompressed chunk pointers with the size of the compressed chunk and the number of bytes we decompressed into,
		// and then make sure we didn't exceed our buffers
		//
		CompressedChunk									+= CompressedChunkSize;
		UncompressedChunk								+= UncompressedChunkSize;

		ASSERT(CompressedChunk <= EndOfCompressedBuffer);
		ASSERT(UncompressedChunk <= EndOfUncompressedBuffer);

		//
		// if the uncompressed is full then we are done
		//
		if(UncompressedChunk == EndOfUncompressedBuffer)
			break;

		//
		// otherwise we need to get the next chunk header.
		// we first check if there is one, save the old chunk size for the chunk we just read in, get the new chunk, and then check if it is the ending chunk header
		//
		if(CompressedChunk > EndOfCompressedBuffer - 2)
			break;

		SavedChunkSize									= GetUncompressedChunkSize(ChunkHeader);

		RtlCopyMemory(&ChunkHeader,CompressedChunk,sizeof(USHORT));

		if(!ChunkHeader.Short)
			break;

		//
		// at this point we are not at the end of the uncompressed buffer and we have another chunk to process.
		// but before we go on we need to see if the last uncompressed chunk didn't fill the full uncompressed chunk size.
		//
		if(UncompressedChunkSize < SavedChunkSize)
		{
			LONG t1										= SavedChunkSize - UncompressedChunkSize;
			PUCHAR t2									= UncompressedChunk + t1;

			//
			// we only need to zero out data if the really are going to process another chunk,
			// to test for that we check if the zero will go beyond the end of the uncompressed buffer
			//
			if(t2 >= EndOfUncompressedBuffer)
				break;

			RtlZeroMemory(UncompressedChunk,t1);
			UncompressedChunk							= t2;
		}
	}

	//
	// if we got out of the loop with the compressed chunk pointer beyond the end of compressed buffer then the compression buffer is ill formed.
	//
	if(CompressedChunk > EndOfCompressedBuffer)
	{
		*FinalUncompressedSize							= reinterpret_cast<ULONG>(CompressedChunk);

		return STATUS_BAD_COMPRESSION_BUFFER;
	}

	//
	// the final uncompressed size is the difference between the start of the uncompressed buffer and where the uncompressed chunk pointer was left
	//
	*FinalUncompressedSize								= UncompressedChunk - UncompressedBuffer;

	return STATUS_SUCCESS;
}

//
// describe chunk
//
NTSTATUS RtlDescribeChunkLZNT1(__inout PUCHAR *CompressedBuffer,__in PUCHAR EndOfCompressedBufferPlus1,__out PUCHAR *ChunkBuffer,__out PULONG ChunkSize)
{
	COMPRESSED_CHUNK_HEADER ChunkHeader					= {0};
	NTSTATUS Status										= STATUS_NO_MORE_ENTRIES;
	*ChunkBuffer										= *CompressedBuffer;
	*ChunkSize											= 0;

	//
	// make sure that the compressed buffer is at least four bytes long to start with, otherwise just return a zero chunk.
	//
	if(*CompressedBuffer <= EndOfCompressedBufferPlus1 - 4)
	{
		RtlCopyMemory(&ChunkHeader,*CompressedBuffer,sizeof(USHORT));

		//
		// check for end of chunks, terminated by USHORT of 0,first assume there are no more.
		//
		if(ChunkHeader.Short)
		{
			Status										= STATUS_SUCCESS;
			*ChunkSize									= GetCompressedChunkSize(ChunkHeader);
			*CompressedBuffer							+= *ChunkSize;

			//
			// check that the chunk actually fits in the buffer supplied by the caller.
			// if not, restore *CompressedBuffer for debug!
			//
			if(*CompressedBuffer > EndOfCompressedBufferPlus1 || ChunkHeader.Chunk.ChunkSignature != 3)
			{
				*CompressedBuffer						-= *ChunkSize;
				Status									= STATUS_BAD_COMPRESSION_BUFFER;

			}
			else if(!ChunkHeader.Chunk.IsChunkCompressed)
			{
				//
				// the uncompressed chunk must be exactly this size! if not, restore *CompressedBuffer for debug!
				//
				if(*ChunkSize != MAX_UNCOMPRESSED_CHUNK_SIZE + 2)
				{
					*CompressedBuffer					-= *ChunkSize;
					Status								= STATUS_BAD_COMPRESSION_BUFFER;
				}
				else
				{
					//
					// the chunk does not contain compressed data so we need to remove the chunk header from the chunk description.
					//
					*ChunkBuffer						+= 2;
					*ChunkSize							-= 2;
				}
			}
			else if(*ChunkSize == 6 && *(*ChunkBuffer + 2) == 2 && *(*ChunkBuffer + 3) == 0)
			{
				//
				// otherwise we have a compressed chunk, and we only need to see if it is all zeros!
				// since the header is already interpreted, we only have to see if there is exactly one literal and if it is zero -
				// it doesn't matter what the copy token says we have a chunk of zeros!
				//
				*ChunkSize									= 0;
			}
		}
	}

	return Status;
}