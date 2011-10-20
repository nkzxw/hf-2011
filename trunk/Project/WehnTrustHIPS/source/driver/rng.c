/*
 * WehnTrust
 *
 * PRNG implementation by optyx, modded by skape for NTification
 */
#include "precomp.h"

VOID RngSeedInfoClass(
		IN SYSTEM_INFORMATION_CLASS InfoClass,
		IN ULONG InfoClassBufferSize,
		IN OUT PULONG PoolOffset);
VOID RngStir();

//
// Rng global variables
//
SHA1_CTX RngShaContext;
UCHAR    RngPool[64];
ULONG    RngIncrement = 0;

#pragma code_seg("INIT")

//
// Seeds the random number generator
//
VOID RngInitialize()
{
	ULONG Offset = 0;

	//
	// Initialize the SHA1 context
	//
   SHA1_Init(&RngShaContext);

	//
	// Gather entropy for the RNG pool
	//
	RngSeedInfoClass(
			SystemPerformanceInformation, 
			sizeof(SYSTEM_PERFORMANCE_INFORMATION),
			&Offset);
	RngSeedInfoClass(
			SystemTimeOfDayInformation,
			sizeof(SYSTEM_TIME_OF_DAY_INFORMATION),
			&Offset);
	RngSeedInfoClass(
			SystemProcessorStatistics,
			sizeof(SYSTEM_PROCESSOR_STATISTICS),
			&Offset);
	RngSeedInfoClass(
			SystemExceptionInformation,
			sizeof(SYSTEM_EXCEPTION_INFORMATION),
			&Offset);
	RngSeedInfoClass(
			SystemLookasideInformation,
			sizeof(SYSTEM_LOOKASIDE_INFORMATION),
			&Offset);

	//
	// Stir the RNG pool
	//
	RngStir();
}

#pragma code_seg()

//
// Generate a random four byte value
//
ULONG RngRand()
{
	ULONG ret;

	switch (RngIncrement)
	{
		case 0: ret = RngShaContext.A; break;
		case 1: ret = RngShaContext.B; break;
		case 2: ret = RngShaContext.C; break;
		case 3: ret = RngShaContext.D; break;
		default:
			RngStir();
			ret = RngShaContext.A;
			break;
	}

	RngIncrement++;

	return ret;
}

//
// Seeds the entropy pool with the hash of the supplied info class
//
VOID RngSeedInfoClass(
		IN SYSTEM_INFORMATION_CLASS InfoClass,
		IN ULONG InfoClassBufferSize,
		IN OUT PULONG PoolOffset)
{
	SHA1_CTX Sha;
	NTSTATUS Status;
	PUCHAR   InfoClassBuffer = NULL;
	ULONG    RealInfoClassSize = 0;
	UCHAR    Digest[SHA1_HASH_SIZE];
	ULONG    Index, DigestIndex = 0;
	ULONG    Cutoff, Leftover;

	SHA1_Init(
			&Sha);

	do
	{
		//
		// Allocate a temporary buffer to hold the info class' data
		//
		if (!(InfoClassBuffer = ExAllocatePoolWithTag(
				PagedPool,
				InfoClassBufferSize,
				ALLOC_TAG)))
		{
			DebugPrint(("RngSeedInfoClass(): ExAllocatePoolWithTag(%lu) failed.",
					InfoClassBufferSize));
			break;
		}

		//
		// Query system information on the provided info class
		//
		if (!NT_SUCCESS(Status = ZwQuerySystemInformation(
				InfoClass,
				InfoClassBuffer,
				InfoClassBufferSize,
				&RealInfoClassSize)))
		{
			DebugPrint(("RngSeedInfoClass(): ZwQuerySystemInformation(%d) failed, %.8x.", 
					InfoClass,
					Status));

			break;
		}

		//
		// Checksum the buffer
		//
		SHA1_Update(
				&Sha,
				InfoClassBuffer,
				RealInfoClassSize);

		//
		// Finalize the hash
		//
		SHA1_Final(
				&Sha,
				Digest);

		// 
		// Calculate the lengths to perform the copy operation on
		//
		Cutoff = sizeof(RngPool) - *PoolOffset;

		if (Cutoff < sizeof(Digest))
			Leftover = sizeof(Digest) - Cutoff;
		else
		{	
			Leftover = 0;
			Cutoff   = sizeof(Digest);
		}

		for (Index = *PoolOffset;
		     DigestIndex < Cutoff;
		     Index++, DigestIndex++)
			RngPool[Index] ^= Digest[DigestIndex];

		//
		// If we wrapped around to the start of the pool, account for that
		//
		if (Leftover)
		{
			for (Index = 0;
			     Leftover > 0;
			     Index++, DigestIndex++, Leftover--)
				RngPool[Index] ^= Digest[DigestIndex];
		}

		// 
		// Finally, set the new offset to Index
		//
		*PoolOffset = Index;

	} while (0);

	// 
	// Free the info class' temporary buffer
	//
	if (InfoClassBuffer)
		ExFreePool(InfoClassBuffer);
}

//
// Stirs the RNG pool
//
VOID RngStir()
{
	ULONG A, B;
	UCHAR Tmp;
	ULONG Pos, Diff, Off;	

	A = RngShaContext.A ^ RngShaContext.E;
	B = RngShaContext.B ^ RngShaContext.E;

	SHA1_Update(&RngShaContext, RngPool, sizeof(RngPool));

	for (Pos = 0, Diff = 1;
	     Pos < 32;
	     Pos++)
	{
		if (A & 1)
			Diff++;

		Off = B & 3;
		B   = B >> 1;
		A   = A >> 1;
		Tmp = RngPool[(Pos + Diff + Off) & (sizeof(RngPool) - 1)];
		RngPool[(Pos + Diff + Off) & (sizeof(RngPool) - 1)] = RngPool[(Pos - Diff - Off) & (sizeof(RngPool) - 1)];
		RngPool[(Pos - Diff - Off) & (sizeof(RngPool) - 1)] = Tmp;
		B   = B >> 1;
	}

	RngIncrement = 0;
}
