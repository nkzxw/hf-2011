/*++

	This is the part of NGdbg kernel debugger

	intvect.cpp

	Contains routines that work with APIC and IDT

--*/

#include <ntifs.h>

//
// IDT entry
//

#pragma pack(push, 1)
struct IDTEntry
{
    USHORT OffsetLow;
    USHORT Selector;
    UCHAR ReservedByte;
    UCHAR Type : 3;
    UCHAR D : 1;
    UCHAR UnusedBits2 : 1;
    UCHAR DPL : 2;
    UCHAR Present : 1;
    USHORT OffsetHigh;
};
#pragma pack(pop)

//
// IDTR structure
//

#pragma pack(push, 2)
struct IDTR
{
    USHORT Limit;
    IDTEntry* Table;
};
#pragma pack(pop)


IDTR Idtr;

PVOID
GetVector(
  IN UCHAR Interrupt
  );

VOID
DelVector(
  IN UCHAR Interrupt
  )
/**
	Delete IDT vector
*/
{
	if (Idtr.Table == NULL)
		__asm sidt fword ptr [Idtr]

	Idtr.Table[Interrupt].Present = FALSE;
}


ULONG GetAPICValue (PVOID pAPIC, ULONG xOffset);
ULONG GetIOAPICIntVector (ULONG Vector);

PVOID 
IoHookInterrupt (
	ULONG Vector, 
	PVOID NewRoutine);



PVOID
SetVector(
  IN UCHAR Interrupt,
  IN PVOID Handler,
  IN BOOLEAN MakeValid
  );
