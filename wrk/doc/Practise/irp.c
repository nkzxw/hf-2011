#define IoSkipCurrentIrpStackLocation( Irp ) { \
    (Irp)->CurrentLocation++; \
    (Irp)->Tail.Overlay.CurrentStackLocation++; }


#define IoCopyCurrentIrpStackLocationToNext( Irp ) { \
    PIO_STACK_LOCATION __irpSp; \
    PIO_STACK_LOCATION __nextIrpSp; \
    __irpSp = IoGetCurrentIrpStackLocation( (Irp) ); \
    __nextIrpSp = IoGetNextIrpStackLocation( (Irp) ); \
    RtlCopyMemory( __nextIrpSp, __irpSp, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
    __nextIrpSp->Control = 0; }


#define IoSetNextIrpStackLocation( Irp ) {      \
    (Irp)->CurrentLocation--;                   \
    (Irp)->Tail.Overlay.CurrentStackLocation--; }


#define IoSetCompletionRoutine( Irp, Routine, CompletionContext, Success, Error, Cancel ) { \
    PIO_STACK_LOCATION __irpSp;                                               \
    ASSERT( ((Success) | (Error) | (Cancel)) ? (Routine) != NULL : TRUE );    \
    __irpSp = IoGetNextIrpStackLocation( (Irp) );                             \
    __irpSp->CompletionRoutine = (Routine);                                   \
    __irpSp->Context = (CompletionContext);                                   \
    __irpSp->Control = 0;                                                     \
    if ((Success)) { __irpSp->Control = SL_INVOKE_ON_SUCCESS; }               \
    if ((Error)) { __irpSp->Control |= SL_INVOKE_ON_ERROR; }                  \
    if ((Cancel)) { __irpSp->Control |= SL_INVOKE_ON_CANCEL; } }

#define IoSetCancelRoutine( Irp, NewCancelRoutine ) (  \
    (PDRIVER_CANCEL) (ULONG_PTR) InterlockedExchangePointer( (PVOID *) &(Irp)->CancelRoutine, (PVOID) (ULONG_PTR)(NewCancelRoutine) ) )

#define IoMarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control |= SL_PENDING_RETURNED )



#define IoGetNextIrpStackLocation( Irp ) (\
    (Irp)->Tail.Overlay.CurrentStackLocation - 1 )

#define IoGetCurrentIrpStackLocation( Irp ) ( (Irp)->Tail.Overlay.CurrentStackLocation )


#define IopInitializeIrp( Irp, PacketSize, StackSize ) {          \
    RtlZeroMemory( (Irp), (PacketSize) );                         \
    (Irp)->Type = (CSHORT) IO_TYPE_IRP;                           \
    (Irp)->Size = (USHORT) ((PacketSize));                        \
    (Irp)->StackCount = (CCHAR) ((StackSize));                    \
    (Irp)->CurrentLocation = (CCHAR) ((StackSize) + 1);           \
    (Irp)->ApcEnvironment = KeGetCurrentApcEnvironment();         \
    InitializeListHead (&(Irp)->ThreadListEntry);                 \
    (Irp)->Tail.Overlay.CurrentStackLocation =                    \
        ((PIO_STACK_LOCATION) ((UCHAR *) (Irp) +                  \
            sizeof( IRP ) +                                       \
            ( (StackSize) * sizeof( IO_STACK_LOCATION )))); }
