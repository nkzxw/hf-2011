//======================================================================
// 
// Ia64stub.c
//
// Copyright (C) 1996-2002 Mark Russinovich and Bryce Cogswell
// Sysinternals - www.sysinternals.com
//
// This file contains stub functions that are patched at runtime
// to load the regmon GP register before calling our actuall hook
// routines.
//
//======================================================================
#include "ntddk.h"
#include "..\reglib.h"
#include "ia64stub.h"

//
// This serves as the load immediate instruction that we can
// patch to load the GP register with Regmon's global pointer
//
#define SET_GP()     volatile ULONGLONG gp; gp = 0x1234567ABCDEF0

//
// Signature of the above instruction
//
#define PATCH_SLOT2         0x0048D159EA
#define PATCH_SLOT3         0xCDEF2E0000


//----------------------------------------------------------------------
//
// PatchStub
//
// Searches for a dummy instruction in each stub routine
// and changes it to read the Regsys gp register value into the gp.
//
//----------------------------------------------------------------------
VOID
PatchStub( 
    ULONGLONG GpReg,
    PVOID StubProc 
    )
{
    ULONG_PTR      *ptr, *remapPtr;
    ULONGLONG      template, slot1, slot2, slot3;
    PMDL           remapMdl;
    int            i;
    ULONGLONG      imm64;

    //
    // Scan for the instruction we're
    // going to patch. It's a 
    // { 
    //   nop.m 0
    //   movl  r28, 00123456'7abcdef0
    // }
    ptr = (ULONG_PTR *) StubProc;
    for( i = 0; i < 30; i++ ) {

        KdPrint(("%I64x: %I64x\n",
                 ptr + i, *(ptr+i)));

        //
        // Break the instruction into its composite parts
        //
        template = *(ptr+i) & 0x1F;
        slot1 = (*(ptr+i) >> 5) & 0x1FFFFFFFFFF;
        slot2 = ((*(ptr+i+1) & 0x3FFFFF) << 18)| (*(ptr+i) >> 46);
        slot3 = *(ptr+i+1) >> 23;
        KdPrint(("  Slot 2: %I64x Slot 3: %I64x\n",
                 slot2, slot3 ));

        if( slot2 == PATCH_SLOT2 && (slot3 & ~0x1FFF) == PATCH_SLOT3 ) {

            break;
        }
    }
    
    //
    // Patch the instruction
    //
    ASSERT( i != 30 );
#if DBG
    imm64 = (((slot3 >> 36) & 1) << 63) |
        (slot2 << 22) |
        (((slot3 >> 21) & 1) << 21) |
        (((slot3 >> 22) & 0x1F) << 16) |
        (((slot3 >> 27) & 0x1FF) << 7) |
        (((slot3 >> 13) & 0x7F));
    KdPrint(("   Imm64: %I64x\n", imm64 ));
#endif

    //
    // Put the regsys gp reg into the immediate
    // encoding the following:
    // {
    //    nop.m 0
    //    movl gp, REGSYS-GP
    // }
    //
    slot2 = (GpReg >> 22) & 0x1FFFFFFFFFF;
    slot3 = ((ULONGLONG) 0x6 << 37) |
        ((GpReg >> 63) << 36) |
        (((GpReg >> 7) & 0x1FF) << 27) |
        (((GpReg >> 16) & 0x1F) << 22) |
        (((GpReg >> 21) & 0x1) << 21) |
        ((GpReg & 0x7F) << 13) |
        ((ULONGLONG) 1 << 6); // Reg1, the gp register

#if DBG
    imm64 = (((slot3 >> 36) & 1) << 63) |
        (slot2 << 22) |
        (((slot3 >> 21) & 1) << 21) |
        (((slot3 >> 22) & 0x1F) << 16) |
        (((slot3 >> 27) & 0x1FF) << 7) |
        (((slot3 >> 13) & 0x7F));
    KdPrint(("   Imm64: %I64x\n", imm64 ));
    KdPrint(("  Slot 2: %I64x Slot 3: %I64x\n",
              slot2, slot3 ));
#endif

    remapPtr = (ULONG_PTR *) RegmonMapMem( ptr, 2*sizeof(ULONG64), &remapMdl );
    *(remapPtr+i) = (slot2 << 46) | (slot1 << 5) | template;
    *(remapPtr+i+1) = (slot3 << 23) | (slot2 >> 18);
    RegmonUnmapMem( remapPtr, remapMdl );
}


//
// Stub Functions
//

NTSTATUS 
NTAPI
StubHookRegOpenKey( 
    IN OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess, 
    IN POBJECT_ATTRIBUTES pOpenInfo 
    )
{
    NTSTATUS  status;
    
	SET_GP();
    status = HookRegOpenKey( pHandle, 
                             ReqAccess,
                             pOpenInfo );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegCreateKey( 
    OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess,
    IN POBJECT_ATTRIBUTES pOpenInfo, 
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class, 
    IN ULONG CreateOptions, 
    OUT PULONG Disposition 
    )
{
	NTSTATUS status;

	SET_GP();
	status = HookRegCreateKey( pHandle, 
                               ReqAccess, 
                               pOpenInfo, 
                               TitleIndex,	
                               Class, 
                               CreateOptions, 
                               Disposition );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegCloseKey( 
    IN HANDLE Handle 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegCloseKey( Handle );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegFlushKey( 
    IN HANDLE Handle 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegFlushKey( Handle );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegDeleteKey( 
    IN HANDLE Handle 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegDeleteKey( Handle );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegDeleteValueKey( 
    IN HANDLE Handle, 
    PUNICODE_STRING Name 
    )
{
    NTSTATUS status;
    
    SET_GP();
    status = HookRegDeleteValueKey( Handle,
                                    Name );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegSetValueKey( 
    IN HANDLE KeyHandle, 
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex, 
    IN ULONG Type, 
    IN PVOID Data, 
    IN ULONG DataSize 
    )
{
    NTSTATUS status;
    
    SET_GP();
    status = HookRegSetValueKey( KeyHandle, 
                                 ValueName,
                                 TitleIndex, 
                                 Type, 
                                 Data, 
                                 DataSize );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegEnumerateKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation, 
    IN ULONG Length, 
    OUT PULONG pResultLength 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegEnumerateKey( KeyHandle, 
                                  Index,
                                  KeyInformationClass,
                                  KeyInformation, 
                                  Length, 
                                  pResultLength );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegQueryKey( 
    IN HANDLE  KeyHandle, 
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation, 
    IN ULONG  Length, 
    OUT PULONG  pResultLength 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegQueryKey( KeyHandle, 
                              KeyInformationClass,
                              KeyInformation, 
                              Length, 
                              pResultLength );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegEnumerateValueKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegEnumerateValueKey( KeyHandle, 
                                       Index,
                                       KeyValueInformationClass,
                                       KeyValueInformation, 
                                       Length,
                                       pResultLength );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegQueryValueKey( 
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    )
{
    NTSTATUS status;
    
    SET_GP();
    status = HookRegQueryValueKey( KeyHandle,
                                   ValueName,
                                   KeyValueInformationClass,
                                   KeyValueInformation, 
                                   Length,
                                   pResultLength );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegLoadKey( 
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES HiveFile 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegLoadKey( TargetKey,
                             HiveFile );
    return status;
}

NTSTATUS 
NTAPI
StubHookRegUnloadKey( 
    IN POBJECT_ATTRIBUTES TargetKey 
    )
{
    NTSTATUS status;

    SET_GP();
    status = HookRegUnloadKey( TargetKey );
    return status;
}

