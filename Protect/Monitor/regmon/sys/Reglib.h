//======================================================================
// 
// Reglib.h
//
// Copyright (C) 2000 Mark Russinovich and Bryce Cogswell
//
// Prototypes for library functions that implement the secret sauce
// that allows us to run on Whistler.
//
//======================================================================

//
// Service hook descriptor
//
typedef struct {
    BOOLEAN             Hooked;
#if defined(_M_IA64)
    PLABEL_DESCRIPTOR   FuncDesc;
#endif;
} SERVICE_HOOK_DESCRIPTOR, *PSERVICE_HOOK_DESCRIPTOR;


VOID
RegmonUnmapMem(
    PVOID Pointer,
    PMDL Mdl
    );
PVOID 
RegmonMapMem( 
    PVOID Pointer, 
    ULONG Length, 
    PMDL *MapMdl 
    );
VOID
RegmonUnmapServiceTable( 
    PVOID KeServiceTablePointers
    );
PVOID *
RegmonMapServiceTable(
    SERVICE_HOOK_DESCRIPTOR **ServiceIsHooked
    );

