#ifndef _WIN32K_H_
#define _WIN32K_H_

VOID
W32PrepareCall(
/*++

Routine Description

	This function prepares thread for calling WIN32

Arguments

	None

Return Value

	None

--*/
	);

VOID
W32ReleaseCall(
/*++

Routine Description

	This function unprepares thread from calling WIN32

Arguments

	None

Return Value

	None

--*/
	);


NTSTATUS
W32FindAndSwapIAT(
/*++

Routine Description

	This function should be called once at driver initialization.
	It swaps current driver IAT functions with WIN32K exported functions.
	Also, it attaches to win32, so caller have to call W32ReleaseCall after

Arguments

	None

Return Value

	None

--*/
	);


struct _SURFOBJ;
extern _SURFOBJ *pPrimarySurf;
extern PVOID pDrvCopyBits;
extern KEVENT SynchEvent;

#if 0
PVOID* pTblDrvCopyBits;
#endif

typedef struct  _DRVFN  /* drvfn */
{
    ULONG   iFunc;
    PVOID     pfn;
} DRVFN, *PDRVFN;

#define INDEX_DrvCopyBits                       19L

#define SHARED_SIGNATURE 0xDEADBEEF
typedef struct  tagDRVENABLEDATA
{
    ULONG   iDriverVersion;
    ULONG   c;
    PDRVFN   pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;
typedef struct SHARED_DISP_DATA
{
	ULONG Signature;
	DRVENABLEDATA gChildDrv;
	_SURFOBJ *pPrimarySurf;

	PVOID pDrvCopyBits;
	PVOID ppDrvCopyBits;
	PVOID Trampoline;
} *PSHARED_DISP_DATA;

extern PVOID W32BaseAddress;



typedef struct _SURFOBJ
{
    PVOID  dhsurf;
    PVOID   hsurf;
    PVOID  dhpdev;
    PVOID    hdev;
    LARGE_INTEGER   sizlBitmap;
    ULONG   cjBits;
    PVOID   pvBits;
    PVOID   pvScan0;
    LONG    lDelta;
    ULONG   iUniq;
    ULONG   iBitmapFormat;
    USHORT  iType;
    USHORT  fjBitmap;
} SURFOBJ;
#define STYPE_DEVICE    1L
#define STYPE_DEVBITMAP 3L


PSHARED_DISP_DATA GetSharedData();


#endif
