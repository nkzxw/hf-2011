#ifndef _BOOTVID_H_
#define _BOOTVID_H_

extern "C"
{

typedef
VOID
(*INBV_DISPLAY_STRING_FILTER)(
    PUCHAR *Str
    );

typedef
BOOLEAN
(*INBV_RESET_DISPLAY_PARAMETERS)(
    ULONG Cols,
    ULONG Rows
    );

#ifndef _NTIFS_
VOID
InbvNotifyDisplayOwnershipLost(
    INBV_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    );

VOID
InbvInstallDisplayStringFilter(
    INBV_DISPLAY_STRING_FILTER DisplayStringFilter
    );

VOID
InbvAcquireDisplayOwnership(
    VOID
    );
#endif

BOOLEAN
InbvDriverInitialize(
    IN PVOID LoaderBlock,
    IN ULONG Count
    );

#ifndef _NTIFS_
BOOLEAN
InbvResetDisplay(
    );
#endif

VOID
InbvBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    );

#ifndef _NTIFS_
VOID
InbvSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    );

BOOLEAN
InbvDisplayString(
    PUCHAR Str
    );
#endif

VOID
InbvUpdateProgressBar(
    ULONG Percentage
    );

VOID
InbvSetProgressBarSubset(
    ULONG   Floor,
    ULONG   Ceiling
    );

VOID
InbvSetBootDriverBehavior(
    PVOID LoaderBlock
    );

VOID
InbvIndicateProgress(
    VOID
    );

VOID
InbvSaveProgressIndicatorCount(
    VOID
    );

VOID
InbvSetProgressBarCoordinates(
    ULONG x,
    ULONG y
    );

#ifndef _NTIFS_
VOID
InbvEnableBootDriver(
    BOOLEAN bEnable
    );

BOOLEAN
InbvEnableDisplayString(
    BOOLEAN bEnable
    );

BOOLEAN
InbvIsBootDriverInstalled(
    VOID
    );
#endif

PUCHAR
InbvGetResourceAddress(
    IN ULONG ResourceNumber
    );

VOID
InbvBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

VOID
InbvScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

BOOLEAN
InbvTestLock(
    VOID
    );

VOID
InbvAcquireLock(
    VOID
    );

VOID
InbvReleaseLock(
    VOID
    );

#ifndef _NTIFS_
BOOLEAN
InbvCheckDisplayOwnership(
    VOID
    );

VOID
InbvSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    );

ULONG
InbvSetTextColor(
    ULONG Color
    );
#endif

VOID
InbvSetDisplayOwnership(
    BOOLEAN DisplayOwned
    );

}

#endif
