//======================================================================
// 
// Filemon.c
//
// Sysinternals - www.sysinternals.com
// Copyright (C) 1996-2001 Mark Russinovich and Bryce Cogswell
//
// Passthrough file system filter device driver.
//
// Notes: The reason that we use NonPagedPool even though the driver
// only accesses allocated buffer at PASSIVE_LEVEL, is that touching
// a paged pool buffer can generate a page fault, and if the paging
// file is on a drive that filemon is monitoring filemon would be
// reentered to handle the page fault. We want to avoid that and so
// we only use nonpaged pool.
//
//======================================================================
#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "..\exe\ioctlcmd.h"
#include "filemon.h"


//----------------------------------------------------------------------
//           F O R W A R D   D E C L A R A T I O N S 
//---------------------------------------------------------------------- 

//
// These are prototypes for Filemon's Fast I/O hooks. The originals 
// prototypes can be found in NTDDK.H
// 
BOOLEAN  
FilemonFastIoCheckifPossible( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoRead( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoWrite( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset,                             
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoQueryBasicInfo( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait, 
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoQueryStandardInfo( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait, 
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoLock( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length, 
    PEPROCESS ProcessId, 
    ULONG Key,
    BOOLEAN FailImmediately, 
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject
    );
BOOLEAN  
FilemonFastIoUnlockSingle( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length, 
    PEPROCESS ProcessId, 
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoUnlockAll( 
    IN PFILE_OBJECT FileObject, 
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoUnlockAllByKey( 
    IN PFILE_OBJECT FileObject, 
    PEPROCESS ProcessId, ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoDeviceControl( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait,
    IN PVOID InputBuffer, 
    IN ULONG InputBufferLength, 
    OUT PVOID OutbufBuffer, 
    IN ULONG OutputBufferLength, 
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
VOID     
FilemonFastIoAcquireFile( 
    PFILE_OBJECT FileObject 
    );
VOID     
FilemonFastIoReleaseFile(
    PFILE_OBJECT FileObject 
    );
VOID     
FilemonFastIoDetachDevice( 
    PDEVICE_OBJECT SourceDevice, 
    PDEVICE_OBJECT TargetDevice 
    );

//
// These are new NT 4.0 Fast I/O calls
//
BOOLEAN  
FilemonFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait, 
    OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
    OUT struct _IO_STATUS_BLOCK *IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    );
NTSTATUS 
FilemonFastIoAcquireForModWrite( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset, 
    OUT struct _ERESOURCE **ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoMdlRead( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PMDL *MdlChain, 
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoMdlReadComplete( 
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoPrepareMdlWrite( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN ULONG LockKey,
    OUT PMDL *MdlChain, 
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );
BOOLEAN  
FilemonFastIoMdlWriteComplete( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoReadCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PVOID Buffer, 
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoWriteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    IN PVOID Buffer, 
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoMdlReadCompleteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN  
FilemonFastIoMdlWriteCompleteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject 
    );
BOOLEAN 
FilemonFastIoQueryOpen( 
    IN struct _IRP *Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject 
    );
NTSTATUS 
FilemonFastIoReleaseForModWrite( 
    IN PFILE_OBJECT FileObject,
    IN struct _ERESOURCE *ResourceToRelease, 
    IN PDEVICE_OBJECT DeviceObject 
    );
NTSTATUS
FilemonFastIoAcquireForCcFlush( 
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject 
    );
NTSTATUS
FilemonFastIoReleaseForCcFlush( 
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject 
    );

BOOLEAN
ApplyFilters( 
    PCHAR Text
    );

//
// Unload routine (debug builds only)
//
VOID
FilemonUnload( 
    IN PDRIVER_OBJECT DriverObject 
    );

//----------------------------------------------------------------------
//                       G L O B A L S
//---------------------------------------------------------------------- 

//
// This is our Driver Object
//
PDRIVER_OBJECT      FilemonDriver;

//
// Indicates if the GUI wants activity to be logged
//
BOOLEAN             FilterOn = FALSE;

//
// Global filter (sent to us by the GUI)
//
FILTER              FilterDef;

//
// This lock protects access to the filter array
//
ERESOURCE           FilterResource;

//
// Array of process and path filters 
//
ULONG               NumIncludeFilters = 0;
PCHAR               IncludeFilters[MAXFILTERS];
ULONG               NumExcludeFilters = 0;
PCHAR               ExcludeFilters[MAXFILTERS];

//
// Once a load is initiated, this flag prevents the processing of
// further IRPs. This is required because an unload can only take
// place if there are any IRP's for which an IoCompletion has
// been registered that has not actually completed.
//
BOOLEAN             UnloadInProgress = FALSE;

//
// This is the offset into a KPEB of the current process name. This is determined
// dynamically by scanning the process block belonging to the GUI for the name
// of the system process, in who's context we execute in DriverEntry
//
ULONG               ProcessNameOffset;

//
// This variable keeps track of the outstanding IRPs (ones for which
// a completion routine has been registered, but that have not yet
// passed through the completion routine), which is used in 
// the unload determination logic. The CountMutex protects data
// races on updating the count.
//
#if DBG
ULONG               OutstandingIRPCount = 0;
#endif // DBG
KSPIN_LOCK          CountMutex;

//
// Table of our hook devices for each drive letter. This makes it
// easy to look up the device object that was created to hook a 
// particular drive.
//
PDEVICE_OBJECT      DriveHookDevices[26];

//
// Current bitmask of hooked drives
//
ULONG               CurrentDriveSet = 0;

//
// The special file system hook devices
//
PDEVICE_OBJECT      NamedPipeHookDevice = NULL;
PDEVICE_OBJECT      MailSlotHookDevice = NULL;

//
// Hash table for keeping names around. This is necessary because 
// at any time the name information in the fileobjects that we
// see can be deallocated and reused. If we want to print accurate
// names, we need to keep them around ourselves. 
//
PHASH_ENTRY	        HashTable[NUMHASH];

//
// Reader/Writer lock to protect hash table.
//
ERESOURCE	        HashResource;

//
// The current output buffer
//
PLOG_BUF            CurrentLog = NULL;

//
// Each IRP is given a sequence number. This allows the return status
// of an IRP, which is obtained in the completion routine, to be 
// associated with the IRPs parameters that were extracted in the Dispatch
// routine.
//
ULONG               Sequence = 0;

//
// This mutex protects the output buffer
//
FAST_MUTEX          LogMutex;

//
// Filemon keeps track of the number of distinct output buffers that
// have been allocated, but not yet uploaded to the GUI, and caps
// the amount of memory (which is in non-paged pool) it takes at
// 1MB.
//
ULONG               NumLog = 0;
ULONG               MaxLog = (1024*1024)/LOGBUFSIZE;

//
// Full path name lookaside for dispatch entry
//
NPAGED_LOOKASIDE_LIST  FullPathLookaside;

//
// We use this string for a path name when we're out of resources
//
CHAR InsufficientResources[] = "<INSUFFICIENT MEMORY>";

//
// These are the text representations of the classes of IRP_MJ_SET/GET_INFORMATION
// calls
//
CHAR *FileInformation[] = {
    "",
    "FileDirectoryInformation",
    "FileFullDirectoryInformation",
    "FileBothDirectoryInformation",
    "FileBasicInformation",
    "FileStandardInformation",
    "FileInternalInformation",
    "FileEaInformation",
    "FileAccessInformation",
    "FileNameInformation",
    "FileRenameInformation",
    "FileLinkInformation",
    "FileNamesInformation",
    "FileDispositionInformation",
    "FilePositionInformation",
    "FileFullEaInformation",
    "FileModeInformation",
    "FileAlignmentInformation",
    "FileAllInformation",
    "FileAllocationInformation",
    "FileEndOfFileInformation",
    "FileAlternateNameInformation",
    "FileStreamInformation",
    "FilePipeInformation",
    "FilePipeLocalInformation",
    "FilePipeRemoteInformation",
    "FileMailslotQueryInformation",
    "FileMailslotSetInformation",
    "FileCompressionInformation",
    "FileCopyOnWriteInformation",
    "FileCompletionInformation",
    "FileMoveClusterInformation",
    "FileOleClassIdInformation",
    "FileOleStateBitsInformation",
    "FileNetworkOpenInformation",
    "FileObjectIdInformation",
    "FileOleAllInformation",
    "FileOleDirectoryInformation",
    "FileContentIndexInformation",
    "FileInheritContentIndexInformation",
    "FileOleInformation",
    "FileMaximumInformation",
};


//
// These are textual representations of the IRP_MJ_SET/GET_VOLUME_INFORMATION
// classes
//
CHAR *VolumeInformation[] = {
    "",
    "FileFsVolumeInformation",
    "FileFsLabelInformation",
    "FileFsSizeInformation",
    "FileFsDeviceInformation",
    "FileFsAttributeInformation",
    "FileFsQuotaQueryInformation",
    "FileFsQuotaSetInformation",
    "FileFsControlQueryInformation",
    "FileFsControlSetInformation",
    "FileFsMaximumInformation",
};


//
// These are Win2K Plug-and-Play minor IRP codes
//
CHAR *PnpMinorCode[] = {
	"IRP_MN_START_DEVICE",
	"IRP_MN_QUERY_REMOVE_DEVICE",
	"IRP_MN_REMOVE_DEVICE",
	"IRP_MN_CANCEL_REMOVE_DEVICE",
	"IRP_MN_STOP_DEVICE",                
	"IRP_MN_QUERY_STOP_DEVICE",          
	"IRP_MN_CANCEL_STOP_DEVICE",         
	"IRP_MN_QUERY_DEVICE_RELATIONS",      
	"IRP_MN_QUERY_INTERFACE",             
	"IRP_MN_QUERY_CAPABILITIES",          
	"IRP_MN_QUERY_RESOURCES",             
	"IRP_MN_QUERY_RESOURCE_REQUIREMENTS", 
	"IRP_MN_QUERY_DEVICE_TEXT",           
	"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
	"IRP_MN_READ_CONFIG",                 
	"IRP_MN_WRITE_CONFIG",                
	"IRP_MN_EJECT",                       
	"IRP_MN_SET_LOCK",                    
	"IRP_MN_QUERY_ID",                    
	"IRP_MN_QUERY_PNP_DEVICE_STATE",      
	"IRP_MN_QUERY_BUS_INFORMATION",       
	"IRP_MN_DEVICE_USAGE_NOTIFICATION",   
	"IRP_MN_SURPRISE_REMOVAL",            
	"IRP_MN_QUERY_LEGACY_BUS_INFORMATION",
};

#define MAX_NTFS_METADATA_FILE 11
CHAR *NtfsMetadataFileNames[] = {
    "$Mft",
    "$MftMirr",
    "$LogFile",
    "$Volume",
    "$AttrDef",
    "$Root",
    "$Bitmap",
    "$Boot",
    "$BadClus",
    "$Secure",
    "$UpCase",
    "$Extend"
};
    

    
//
// This Filemon's Fast I/O dispatch table. Note that NT assumes that
// file system drivers support some Fast I/O calls, so this table must
// be present for an file system filter driver
//
FAST_IO_DISPATCH    FastIOHook = {
    sizeof(FAST_IO_DISPATCH), 
    FilemonFastIoCheckifPossible,
    FilemonFastIoRead,
    FilemonFastIoWrite,
    FilemonFastIoQueryBasicInfo,
    FilemonFastIoQueryStandardInfo,
    FilemonFastIoLock,
    FilemonFastIoUnlockSingle,
    FilemonFastIoUnlockAll,
    FilemonFastIoUnlockAllByKey,
    FilemonFastIoDeviceControl,
    FilemonFastIoAcquireFile,
    FilemonFastIoReleaseFile,
    FilemonFastIoDetachDevice,

    //
    // new for NT 4.0
    //
    FilemonFastIoQueryNetworkOpenInfo,
    FilemonFastIoAcquireForModWrite,
    FilemonFastIoMdlRead,
    FilemonFastIoMdlReadComplete,
    FilemonFastIoPrepareMdlWrite,
    FilemonFastIoMdlWriteComplete,
    FilemonFastIoReadCompressed,
    FilemonFastIoWriteCompressed,
    FilemonFastIoMdlReadCompleteCompressed,
    FilemonFastIoMdlWriteCompleteCompressed,
    FilemonFastIoQueryOpen,
    FilemonFastIoReleaseForModWrite,
    FilemonFastIoAcquireForCcFlush,
    FilemonFastIoReleaseForCcFlush
};

//----------------------------------------------------------------------
//      P A T T E R N   M A T C H I N G   R O U T I N E S
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//
// MatchOkay
//
// Only thing left after compare is more mask. This routine makes
// sure that its a valid wild card ending so that its really a match.
//
//----------------------------------------------------------------------
BOOLEAN 
MatchOkay( 
    PCHAR Pattern 
    )
{
    //
    // If pattern isn't empty, it must be a wildcard
    //
    if( *Pattern && *Pattern != '*' ) {
 
        return FALSE;
    }

    //
    // Matched
    //
    return TRUE;
}


//----------------------------------------------------------------------
//
// MatchWithPattern
//
// Performs nifty wildcard comparison.
//
//----------------------------------------------------------------------
BOOLEAN 
MatchWithPattern( 
    PCHAR Pattern, 
    PCHAR Name 
    )
{
	char matchchar;

    //
    // End of pattern?
    //
    if( !*Pattern ) {

        return FALSE;
    }

    //
    // If we hit a wild card, do recursion
    //
    if( *Pattern == '*' ) {

        Pattern++;
        while( *Name && *Pattern ) {

			matchchar = *Name;
			if( matchchar >= 'a' && 
				matchchar <= 'z' ) {

				matchchar -= 'a' - 'A';
			}

            //
            // See if this substring matches
            //
		    if( *Pattern == matchchar ) {

  		        if( MatchWithPattern( Pattern+1, Name+1 )) {

                    return TRUE;
                }
            }

            //
            // Try the next substring
            //
            Name++;
        }

        //
        // See if match condition was met
        //
        return MatchOkay( Pattern );
    } 

    //
    // Do straight compare until we hit a wild card
    //
    while( *Name && *Pattern != '*' ) {

		matchchar = *Name;
		if( matchchar >= 'a' && 
			matchchar <= 'z' ) {

			matchchar -= 'a' - 'A';
		}

        if( *Pattern == matchchar ) {
            Pattern++;
            Name++;

        } else {

            return FALSE;
		}
    }

    //
    // If not done, recurse
    //
    if( *Name ) {

        return MatchWithPattern( Pattern, Name );
    }

    //
    // Make sure its a match
    //
    return MatchOkay( Pattern );
}

//----------------------------------------------------------------------
//            B U F F E R   M A N A G E M E N T
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FilemonFreeLog
//
// Frees all the data output buffers that we have currently allocated.
//
//----------------------------------------------------------------------
VOID 
FilemonFreeLog(
    VOID 
    )
{
    PLOG_BUF  prev;
    
    //
    // Just traverse the list of allocated output buffers
    //
    while( CurrentLog ) {
        prev = CurrentLog->Next;
        ExFreePool( CurrentLog );
        CurrentLog = prev;
    }
}   


//----------------------------------------------------------------------
//
// FilemonAllocateLog
//
// Called when the current buffer has filled up. This allocates a new
// buffer and stick it at the head (newest) entry of our buffer list.
//
//----------------------------------------------------------------------
void 
FilemonAllocateLog( 
    VOID
    )
{
    PLOG_BUF prev = CurrentLog, newLog;

    //
    // If we've already allocated the allowed number of buffers, just
    // reuse the current one. This will result in output records being
    // lost, but it takes ALOT of file system activity to cause this.
    //
    if( MaxLog == NumLog ) {

        DbgPrint(("Filemon ***** Dropping records at sequence number %d\n", Sequence ));
        CurrentLog->Len = 0;
        return; 
    }

    DbgPrint(("FilemonAllocateLog: num: %d max: %d\n", NumLog, MaxLog ));

    //
    // If the output buffer we currently are using is empty, just
    // use it.
    //
    if( !CurrentLog->Len ) {

        return;
    }

    //
    // Allocate a new output buffer
    //
    newLog = ExAllocatePool( NonPagedPool, sizeof(*CurrentLog) );
    if( newLog ) { 

        //
        // Allocation was successful so add the buffer to the list
        // of allocated buffers and increment the buffer count.
        //
        CurrentLog       = newLog;
        CurrentLog->Len  = 0;
        CurrentLog->Next = prev;
        NumLog++;

    } else {

        //
        // The allocation failed - just reuse the current buffer
        //
        CurrentLog->Len = 0;
    }
}


//----------------------------------------------------------------------
//
// FilemonGetOldestLog
//
// Traverse the list of allocated buffers to find the last one, which
// will be the oldest (as we want to return the oldest data to the GUI
// first).
//
//----------------------------------------------------------------------
PLOG_BUF 
FilemonGetOldestLog( 
    VOID 
    )
{
    PLOG_BUF  ptr = CurrentLog, prev = NULL;

    //
    // Traverse the list
    //    
    while( ptr->Next ) {

        ptr = (prev = ptr)->Next;
    }

    //
    // Remove the buffer from the list
    //
    if( prev ) {

        prev->Next = NULL;    
        NumLog--;
    }
    return ptr;
}


//----------------------------------------------------------------------
//
// FilemonResetLog
//
// When a GUI instance has close communication (exited), but the driver
// can't unload due to oustdanding IRPs, all the output buffers except
// one are all deallocated so that the memory footprint is shrunk as much 
// as possible.
//
//----------------------------------------------------------------------
VOID 
FilemonResetLog(
    VOID
    )
{
    PLOG_BUF  current, next;

    ExAcquireFastMutex( &LogMutex );

    //
    // Traverse the list of output buffers
    //
    current = CurrentLog->Next;
    while( current ) {

        //
        // Free the buffer
        //
        next = current->Next;
        ExFreePool( current );
        current = next;
    }

    // 
    // Move the output pointer in the buffer that's being kept
    // the start of the buffer.
    // 
    NumLog = 1;
    CurrentLog->Len = 0;
    CurrentLog->Next = NULL;
    ExReleaseFastMutex( &LogMutex );    
}


//----------------------------------------------------------------------
//
// LogRecord
//
// This "printfs" a string into an output buffer.
//
//----------------------------------------------------------------------
BOOLEAN
LogRecord( 
    BOOLEAN ProcessFilters,
    PULONG SeqNum, 
    PLARGE_INTEGER dateTime, 
    PLARGE_INTEGER perfTime,
    const CHAR * format, 
    ... 
    ) 
{   
    PENTRY             Entry;
    int                len;
    ULONG              recordSequence;
    va_list            arg_ptr;
    static CHAR        text[MAXPATHLEN];
    BOOLEAN            passedFilters = FALSE;

    //
    // If no GUI is there to receive the output or if no filtering is desired, don't bother
    //     
    if( !FilterOn ) {
     
        return FALSE;
    }

    // 
    // Lock the output buffer and Log.
    // 
    ExAcquireFastMutex( &LogMutex );

    //
    // Send text out as debug output  This is x86 specific.
    //      
#define A (&format)
    DbgPrint(( (char *)format, A[1], A[2], A[3], A[4], A[5], A[6] ));
    DbgPrint(( "\n" ));
#undef A

    //
    // Vsprintf to determine the length of the buffer
    //
    va_start( arg_ptr, format );
    len = vsprintf( text, format, arg_ptr );
    va_end( arg_ptr );

    //
    // ULONG align for Alpha
    //
    len += 4; len &=  0xFFFFFFFC; 

    //
    // Only log it if it passes the filters. Note that IRP completion
    // passes a false for ProcessFilters because if we've logged
    // the initial action, we have to go ahead and log the completion.
    //
    passedFilters = !ProcessFilters || ApplyFilters( text );
    if( passedFilters ) {

        //
        // Assign a sequence number if we weren't passed one
        //
        if( !SeqNum || (SeqNum && *SeqNum == (ULONG) -1)) {

            recordSequence = InterlockedIncrement( &Sequence );
            if( SeqNum ) *SeqNum = recordSequence;

        } else {

            recordSequence = *SeqNum;
        }

        //
        // If the current output buffer is near capacity, move to a new 
        // output buffer
        //
        if( CurrentLog->Len + len + sizeof(ENTRY) +1 >= LOGBUFSIZE ) {

            FilemonAllocateLog();
        }

        //
        // Log the entry
        //
        Entry = (void *)(CurrentLog->Data+CurrentLog->Len);
        Entry->seq = recordSequence;
        Entry->datetime.QuadPart = 0;
        Entry->perftime.QuadPart = 0;
        if( dateTime ) Entry->datetime = *dateTime;
        if( perfTime ) Entry->perftime = *perfTime;
        memcpy( Entry->text, text, len );
 
        //
        // Log the length of the string, plus 1 for the terminating
        // NULL  
        //   
        CurrentLog->Len += ((ULONG) (Entry->text - (PCHAR) Entry )) + len;
    }

    //
    // Release the output buffer lock
    //
    ExReleaseFastMutex( &LogMutex );
    return passedFilters;
}

//----------------------------------------------------------------------
//       H A S H   T A B L E   M A N A G E M E N T
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FilemonHashCleanup
//
// Called when we are unloading to free any memory that we have 
// in our possession.
//
//----------------------------------------------------------------------
VOID
FilemonHashCleanup(
    VOID
    )
{
    PHASH_ENTRY		hashEntry, nextEntry;
    ULONG		    i;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &HashResource, TRUE );

    //
    // Free the hash table entries
    //
    for( i = 0; i < NUMHASH; i++ ) {

        hashEntry = HashTable[i];

        while( hashEntry ) {
            nextEntry = hashEntry->Next;
            ExFreePool( hashEntry );
            hashEntry = nextEntry;
        }

        HashTable[i] = NULL;
    }
    ExReleaseResourceLite( &HashResource );
    KeLeaveCriticalRegion();
}


//----------------------------------------------------------------------
//
// FilemonFreeHashEntry
//
// When we see a file close, we can free the string we had associated
// with the fileobject being closed since we know it won't be used
// again.
//
//----------------------------------------------------------------------
VOID 
FilemonFreeHashEntry( 
    PFILE_OBJECT fileObject 
    )
{
    PHASH_ENTRY		hashEntry, prevEntry;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &HashResource, TRUE );

    //
    // Look-up the entry
    //
    hashEntry = HashTable[ HASHOBJECT( fileObject ) ];
    prevEntry = NULL;

    while( hashEntry && hashEntry->FileObject != fileObject ) {

        prevEntry = hashEntry;
        hashEntry = hashEntry->Next;
    }
 
    //  
    // If we fall of the hash list without finding what we're looking
    // for, just return.
    //
    if( !hashEntry ) {

        ExReleaseResourceLite( &HashResource );
        KeLeaveCriticalRegion();
        return;
    }

    //
    // Got it! Remove it from the list
    //
    if( prevEntry ) {

        prevEntry->Next = hashEntry->Next;

    } else {

        HashTable[ HASHOBJECT( fileObject )] = hashEntry->Next;
    }

    //
    // Free the entry's memory
    //
    ExFreePool( hashEntry );

    ExReleaseResourceLite( &HashResource );
    KeLeaveCriticalRegion();
}

//----------------------------------------------------------------------
//       P A T H  A N D  P R O C E S S  N A M E  R O U T I N E S
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//
// FilemonFreeFilters
//
// Fress storage we allocated for filter strings.
//
//----------------------------------------------------------------------
VOID 
FilemonFreeFilters(
    VOID
    )
{
    ULONG   i;

    for( i = 0; i < NumIncludeFilters; i++ ) {

        ExFreePool( IncludeFilters[i] );
    }
    for( i = 0; i < NumExcludeFilters; i++ ) {

        ExFreePool( ExcludeFilters[i] );
    }
    NumIncludeFilters = 0;
    NumExcludeFilters = 0;
}


//----------------------------------------------------------------------
//
// MakeFilterArray
//
// Takes a filter string and splits into components (a component
// is seperated with a ';')
//
//----------------------------------------------------------------------
VOID
MakeFilterArray( 
    PCHAR FilterString,
    PCHAR FilterArray[],
    PULONG NumFilters 
    )
{
    PCHAR filterStart;
    ULONG filterLength;
    CHAR  saveChar;

    //
    // Scan through the process filters
    //
    filterStart = FilterString;
    while( *filterStart ) {

        filterLength = 0;
        while( filterStart[filterLength] &&
               filterStart[filterLength] != ';' ) {

            filterLength++;
        }

        //
        // Ignore zero-length components
        //
        if( filterLength ) {

            //
            // Conservatively allocate so that we can prepend and append
            // wildcards
            //
            FilterArray[ *NumFilters ] = 
                ExAllocatePool( PagedPool, filterLength + 1 + 2*sizeof('*') );
            
            //
            // Only fill this in if there's enough memory
            //
            if( FilterArray[ *NumFilters] ) {

                saveChar = *(filterStart + filterLength );
                *(filterStart + filterLength) = 0;
                sprintf( FilterArray[ *NumFilters ], "%s%s%s",
                         *filterStart == '*' ? "" : "*",
                         filterStart,
                         *(filterStart + filterLength - 1 ) == '*' ? "" : "*" );
                *(filterStart + filterLength) = saveChar;
                (*NumFilters)++;
            }
        }
    
        //
        // Are we done?
        //
        if( !filterStart[filterLength] ) break;

        //
        // Move to the next component (skip over ';')
        //
        filterStart += filterLength + 1;
    }
}


//----------------------------------------------------------------------
//
// FilemonUpdateFilters
//
// Takes a new filter specification and updates the filter
// arrays with them.
//
//----------------------------------------------------------------------
VOID 
FilemonUpdateFilters(
    VOID
    )
{
    //
    // Free old filters (if any)
    //
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &FilterResource, TRUE );
    FilemonFreeFilters();

    //
    // Create new filter arrays
    //
    MakeFilterArray( FilterDef.includefilter,
                     IncludeFilters, &NumIncludeFilters );
    MakeFilterArray( FilterDef.excludefilter,
                     ExcludeFilters, &NumExcludeFilters );
    ExReleaseResourceLite( &FilterResource );
    KeLeaveCriticalRegion();
}


//----------------------------------------------------------------------
//
// ApplyFilters
//
// If the name matches the exclusion mask, we do not log it. Else if
// it doesn't match the inclusion mask we do not log it. 
//
//----------------------------------------------------------------------
BOOLEAN
ApplyFilters( 
    PCHAR Text
    )
{
    ULONG i;

    //
    // If no GUI or no filename return FALSE
    //
    if( !Text ) return FALSE;

    //   
    // If it matches the exclusion string, do not log it
    //
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite( &FilterResource, TRUE );

    for( i = 0; i < NumExcludeFilters; i++ ) {

        if( MatchWithPattern( ExcludeFilters[i], Text ) ) {

            ExReleaseResourceLite( &FilterResource );
            KeLeaveCriticalRegion();
            return FALSE;
        }
    }
 
    //
    // If it matches an include filter then log it
    //
    for( i = 0; i < NumIncludeFilters; i++ ) {

        if( MatchWithPattern( IncludeFilters[i], Text )) {

            ExReleaseResourceLite( &FilterResource );
            KeLeaveCriticalRegion();
            return TRUE;
        }
    }

    //
    // It didn't match any include filters so don't log
    //
    ExReleaseResourceLite( &FilterResource );
    KeLeaveCriticalRegion();
    return FALSE;
}


//----------------------------------------------------------------------
//
// FilemonQueryFileComplete
//
// This routine is used to handle I/O completion for our self-generated
// IRP that is used to query a file's name or number.
//
//----------------------------------------------------------------------
NTSTATUS 
FilemonQueryFileComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    //
    // Copy the status information back into the "user" IOSB.
    //
    *Irp->UserIosb = Irp->IoStatus;
    if( !NT_SUCCESS(Irp->IoStatus.Status) ) {

        DbgPrint(("   ERROR ON IRP: %x\n", Irp->IoStatus.Status ));
    }
    
    //
    // Set the user event - wakes up the mainline code doing this.
    //
    KeSetEvent(Irp->UserEvent, 0, FALSE);
    
    //
    // Free the IRP now that we are done with it.
    //
    IoFreeIrp(Irp);
    
    //
    // We return STATUS_MORE_PROCESSING_REQUIRED because this "magic" return value
    // tells the I/O Manager that additional processing will be done by this driver
    // to the IRP - in fact, it might (as it is in this case) already BE done - and
    // the IRP cannot be completed.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


//----------------------------------------------------------------------
//
// FilemonQueryFile
//
// This function retrieves the "standard" information for the
// underlying file system, asking for the filename in particular.
//
//----------------------------------------------------------------------
BOOLEAN 
FilemonQueryFile( 
    PDEVICE_OBJECT DeviceObject, 
    PFILE_OBJECT FileObject,
    FILE_INFORMATION_CLASS FileInformationClass,
    PVOID FileQueryBuffer,
    ULONG FileQueryBufferLength
    )
{
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION ioStackLocation;

    DbgPrint(("Getting file name for %x\n", FileObject));

    //
    // Initialize the event
    //
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Allocate an irp for this request.  This could also come from a 
    // private pool, for instance.
    //
    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if(!irp) {

        //
        // Failure!
        //
        return FALSE;
    }
  
    //
    // Build the IRP's main body
    //  
    irp->AssociatedIrp.SystemBuffer = FileQueryBuffer;
    irp->UserEvent = &event;
    irp->UserIosb = &IoStatusBlock;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->RequestorMode = KernelMode;
    irp->Flags = 0;

    //
    // Set up the I/O stack location.
    //
    ioStackLocation = IoGetNextIrpStackLocation(irp);
    ioStackLocation->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    ioStackLocation->DeviceObject = DeviceObject;
    ioStackLocation->FileObject = FileObject;
    ioStackLocation->Parameters.QueryFile.Length = FileQueryBufferLength;
    ioStackLocation->Parameters.QueryFile.FileInformationClass = FileInformationClass;

    //
    // Set the completion routine.
    //
    IoSetCompletionRoutine(irp, FilemonQueryFileComplete, 0, TRUE, TRUE, TRUE);

    //
    // Send it to the FSD
    //
    (void) IoCallDriver(DeviceObject, irp);

    //
    // Wait for the I/O
    //
    KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, 0);

    //
    // Done! Note that since our completion routine frees the IRP we cannot 
    // touch the IRP now.
    //
    return NT_SUCCESS( IoStatusBlock.Status );
}


//----------------------------------------------------------------------
//
// FilemonGetFullPath
//
// Takes a fileobject and filename and returns a canonical path,
// nicely formatted, in fullpathname.
//
//----------------------------------------------------------------------
VOID 
FilemonGetFullPath( 
    BOOLEAN createPath, 
    PFILE_OBJECT fileObject, 
    PHOOK_EXTENSION hookExt, 
    PCHAR fullPathName 
    )
{
    ULONG               pathLen, prefixLen, slashes;
    PCHAR               pathOffset, ptr;
    BOOLEAN             gotPath;
    PFILE_OBJECT        relatedFileObject;
    PHASH_ENTRY         hashEntry, newEntry;
    ANSI_STRING         fileName;
    ANSI_STRING         relatedName;
    PFILE_NAME_INFORMATION fileNameInfo;
    FILE_INTERNAL_INFORMATION fileInternalInfo;
    UNICODE_STRING      fullUniName;
    ULONGLONG           mftIndex;

    //
    // Only do this if a GUI is active and filtering is on
    //
    if( fullPathName ) fullPathName[0] = 0;
    if( !FilterOn || !hookExt || !hookExt->Hooked || !fullPathName) {
     
        return;
    }

    //
    // Lookup the object in the hash table to see if a name 
    // has already been generated for it
    //
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite( &HashResource, TRUE );

    hashEntry = HashTable[ HASHOBJECT( fileObject ) ];
    while( hashEntry && hashEntry->FileObject != fileObject )  {

        hashEntry = hashEntry->Next;
    }

    //
    // Did we find an entry?
    //
    if( hashEntry ) {

        //
        // Yes, so get the name from the entry.
        //
        strcpy( fullPathName, hashEntry->FullPathName );
        ExReleaseResourceLite( &HashResource );
        KeLeaveCriticalRegion();
        return;
    }

    ExReleaseResourceLite( &HashResource );
    KeLeaveCriticalRegion();

    //
    // We didn't find the name in the hash table so let's either ask
    // the file system for it or construct it from the file objects.
    //

    //
    // Calculate prefix length
    //
    switch( hookExt->Type ) {
    case NPFS: 
        prefixLen = NAMED_PIPE_PREFIX_LENGTH;
        break;
    case MSFS: 
        prefixLen = MAIL_SLOT_PREFIX_LENGTH;
        break;
    default: 
        if( !fileObject ||
            fileObject->DeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM ) {

            prefixLen = 0;

        } else {

            prefixLen = 2; // "C:"
        }
        break;
    }

    //
    // If there's no file object, we can't even ask for a name.
    //
    if( !fileObject ) { 

        if( hookExt->Type == NPFS )      strcpy( fullPathName, NAMED_PIPE_PREFIX );
        else if( hookExt->Type == MSFS ) strcpy( fullPathName, MAIL_SLOT_PREFIX );
        else                             sprintf( fullPathName, "%C:", hookExt->LogicalDrive );
        return;
    }

    //
    // Initialize variables
    //
    fileName.Buffer = NULL;
    relatedName.Buffer = NULL;
    gotPath = FALSE;

    //
    // Check for special case first: NTFS volume and a file object
    // with no name. It might be a metadata file that we "know" the name of. This
    // special case also stops us from querying NTFS for the name of a metadata
    // file on versions of NTFS prior to Whistler, which is a good thing since
    // that causes hangs and crashes. On Whistler metadata files have file names.
    //
    if( !fileObject->FileName.Buffer && hookExt->FsAttributes &&
        !memcmp( hookExt->FsAttributes->FileSystemName, L"NTFS", sizeof(L"NTFS")-sizeof(WCHAR))) {

        //
        // The only file that is opened without a name is a volume
        //
        if( createPath ) {

            sprintf( fullPathName, "%C:", hookExt->LogicalDrive );

            //
            // Return right here without inserting this into the hash table, since this might
            // be the cleanup path of a metadata file and we can retrieve the metada's index
            // at a later point.
            //
            return;

        } else if( FilemonQueryFile( hookExt->FileSystem, fileObject, FileInternalInformation,
                              &fileInternalInfo, sizeof( fileInternalInfo ))) {
            
            //
            // Use the name in the metadata name index
            //
            mftIndex = fileInternalInfo.IndexNumber.QuadPart & ~0xF0000000;
            if( mftIndex <= MAX_NTFS_METADATA_FILE ) {

                sprintf( fullPathName, "%C:\\%s", hookExt->LogicalDrive, NtfsMetadataFileNames[ mftIndex ] );
                gotPath = TRUE;
            }                
        } 
    }

    //
    // If we are not in the create path, we can ask the file system for the name. If we
    // are in the create path, we can't ask the file system for the name of the file object, since
    // the file system driver hasn't even seen the file object yet.
    //
    if( !gotPath && !createPath ) {

        //
        // Ask the file system for the name of the file, which its required to be
        // able to provide for the Win32 filename query function. We could use the
        // undocumented ObQueryNameString, but then we'd have to worry about
        // re-entrancy issues, since that call generates the IRP that we create
        // manually here. Since we send the IRP to the FSD below us, we don't need
        // to worry about seeing the IRP in our dispatch entry point. This can fail
        // in some cases, so we fall back on constructing the name ourselves if
        // we have to.
        //
        fileNameInfo = (PFILE_NAME_INFORMATION) ExAllocatePool( NonPagedPool, 
                                                                MAXPATHLEN*sizeof(WCHAR) );

        if( fileNameInfo && 
            FilemonQueryFile(hookExt->FileSystem, fileObject, FileNameInformation, 
                             fileNameInfo, (MAXPATHLEN - prefixLen - 1)*sizeof(WCHAR) )) {

            fullUniName.Length = (SHORT) fileNameInfo->FileNameLength;
            fullUniName.Buffer = fileNameInfo->FileName;
            if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &fileName, &fullUniName, TRUE ))) { 

                fullPathName[ fileName.Length + prefixLen ] = 0;

                if( hookExt->Type == NPFS ) {
                    
                    strcpy( fullPathName, NAMED_PIPE_PREFIX );

                } else if( hookExt->Type == MSFS ) {

                    strcpy( fullPathName, MAIL_SLOT_PREFIX );

                } else if( fileObject->DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM ) {

                    sprintf( fullPathName, "%C:", hookExt->LogicalDrive );

                } else {
                
                    //
                    // No prefix for network devices
                    //
                }

                memcpy( &fullPathName[prefixLen], fileName.Buffer, fileName.Length );
                gotPath = TRUE;
                RtlFreeAnsiString( &fileName );
                fileName.Buffer = NULL;
            }
        } 
        if( fileNameInfo ) ExFreePool( fileNameInfo );
    }

    //
    // If we don't have a name yet then we are in the create path, or we failed
    // when we asked the file system for the name. In that case we'll go ahead
    // and construct the name based on file object names.
    //
    if( !gotPath ) {

        //
        // If there is no file name at this point, just return "DEVICE" to indicate
        // raw access to a device
        //
        if( !fileObject->FileName.Buffer ) {

            if( hookExt->Type == NPFS )      strcpy( fullPathName, NAMED_PIPE_PREFIX );
            else if( hookExt->Type == MSFS ) strcpy( fullPathName, MAIL_SLOT_PREFIX );
            else                             sprintf( fullPathName, "%C:", hookExt->LogicalDrive );
            return;
        }
    
        //
        // Create the full path name. First, calculate the length taking into 
        // account space for seperators and the leading prefix
        //
        if( !NT_SUCCESS( RtlUnicodeStringToAnsiString( &fileName, &fileObject->FileName, TRUE ))) {

            if( hookExt->Type == NPFS )      sprintf( fullPathName, "%s: <Out of Memory>", NAMED_PIPE_PREFIX );
            else if( hookExt->Type == MSFS ) sprintf( fullPathName, "%s: <Out of Memory>", MAIL_SLOT_PREFIX );
            else                             sprintf( fullPathName, "%C: <Out of Memory>", hookExt->LogicalDrive );
            return;
        }

        pathLen = fileName.Length + prefixLen;
        relatedFileObject = fileObject->RelatedFileObject;
    
        //
        // Only look at related file object if this is a relative name
        //
        if( fileObject->FileName.Buffer[0] != L'\\' && 
            relatedFileObject && relatedFileObject->FileName.Length ) {
	        
			if( !NT_SUCCESS( RtlUnicodeStringToAnsiString( &relatedName, &relatedFileObject->FileName, TRUE ))) {

                if( hookExt->Type == NPFS )      sprintf( fullPathName, "%s: <Out of Memory>", NAMED_PIPE_PREFIX );
                else if( hookExt->Type == MSFS ) sprintf( fullPathName, "%s: <Out of Memory>", MAIL_SLOT_PREFIX );
                else                             sprintf( fullPathName, "%C: <Out of Memory>", hookExt->LogicalDrive );
                RtlFreeAnsiString( &fileName );
                return;
            }
            pathLen += relatedName.Length+1;
        }

        //
        // Add the drive letter first at the front of the name
        //
        if( hookExt->Type == NPFS )      strcpy( fullPathName, NAMED_PIPE_PREFIX );
        else if( hookExt->Type == MSFS ) strcpy( fullPathName, MAIL_SLOT_PREFIX );
        else if( fileObject->DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM ) {

            sprintf( fullPathName, "%C:", hookExt->LogicalDrive );
        }

        //
        // If the name is too long, quit now
        //
        if( pathLen >= MAXPATHLEN ) {
            
            strcat( fullPathName, " <Name Too Long>" );

        } else {
    
            //
            // Now we can build the path name
            //
            fullPathName[ pathLen ] = 0;
            
            pathOffset = fullPathName + pathLen - fileName.Length;
            memcpy( pathOffset, fileName.Buffer, fileName.Length + 1 );
    
            if( fileObject->FileName.Buffer[0] != L'\\' && 
                relatedFileObject && relatedFileObject->FileName.Length ) {

                //
                // Copy the component, adding a slash separator
                //
                *(pathOffset - 1) = '\\';
                pathOffset -= relatedName.Length + 1;
                    
                memcpy( pathOffset, relatedName.Buffer, relatedName.Length );

                //
                // If we've got to slashes at the front zap one
                //
                if( pathLen > 3 && fullPathName[2] == '\\' && fullPathName[3] == '\\' )  {
                    
                    strcpy( fullPathName + 2, fullPathName + 3 );
                }
            }
        }  
    } 
    if( fileName.Buffer ) RtlFreeAnsiString( &fileName );
    if( relatedName.Buffer ) RtlFreeAnsiString( &relatedName );

    //
    // Network redirector names already specify a share name that we 
    // have to strip:
    // 
    //     \X:\computer\share\realpath
    //
    // And we want to present:
    //
    //     X:\realpath
    //
    // to the user.
    //
    if( fileObject->DeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM &&
        strlen( fullPathName ) >= strlen("\\X:\\") ) {

        //
        // If this is Win2k the name is specified like this:
        //
        //    \;X:0\computer\share\realpath
        //
        // so we have to handle that case as well
        //
        if( fullPathName[1] == ';' ) {
            
            //
            // Win2K-style name. Grab the drive letter
            // and skip over the share
            //
            fullPathName[0] = fullPathName[2];
            fullPathName[1] = ':';
            fullPathName[2] = '\\';

            //
            // The third slash after the drive is the
            // start of the real path (we start scanning
            // at the ':' since we don't want to make assumptions
            // about the length of the number).
            //
            slashes = 0;
            ptr = &fullPathName[3];
            while( *ptr && slashes != 3 ) {
                
                if( *ptr == '\\' ) slashes++;
                ptr++;
            }
            strcpy( &fullPathName[3], ptr );

        } else if( fullPathName[2] == ':' ) {

            //
            // NT 4-style name. Skip the share name 
            //
            fullPathName[0] = fullPathName[1];
            fullPathName[1] = ':';
            fullPathName[2] = '\\';
                
            //
            // The second slash after the drive's slash (x:\)
            // is the start of the real path
            //
            slashes = 0;
            ptr = &fullPathName[3];
            while( *ptr && slashes != 3 ) {
                
                if( *ptr == '\\' ) slashes++;
                ptr++;
            }
            strcpy( &fullPathName[3], ptr );

        } else {

            //
            // Its a UNC path, so add a leading slash
            //
            RtlMoveMemory( &fullPathName[1], fullPathName, strlen( fullPathName ) + 1);
            fullPathName[0] = '\\';
        }
    }

    //
    // Allocate a hash entry
    //
    newEntry = ExAllocatePool( NonPagedPool, 
                               sizeof(HASH_ENTRY ) + strlen( fullPathName ) + 1);

    //
    // If no memory for a new entry, oh well.
    //
    if( newEntry ) {

        //
        // Fill in the new entry 
        //
        newEntry->FileObject = fileObject;
        strcpy( newEntry->FullPathName, fullPathName );

        //
        // Put it in the hash table
        //
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite( &HashResource, TRUE );

        newEntry->Next = HashTable[ HASHOBJECT(fileObject) ];
        HashTable[ HASHOBJECT(fileObject) ] = newEntry;	

        ExReleaseResourceLite( &HashResource );
        KeLeaveCriticalRegion();
    }
}


//----------------------------------------------------------------------
//
// FilemonGetProcessNameOffset
//
// In an effort to remain version-independent, rather than using a
// hard-coded into the KPEB (Kernel Process Environment Block), we
// scan the KPEB looking for the name, which should match that
// of the system process. This is because we are in the system process'
// context in DriverEntry, where this is called.
//
//----------------------------------------------------------------------
ULONG 
FilemonGetProcessNameOffset(
    VOID
    )
{
    PEPROCESS       curproc;
    int             i;

    curproc = PsGetCurrentProcess();

    //
    // Scan for 12KB, hoping the KPEB never grows that big!
    //
    for( i = 0; i < 3*PAGE_SIZE; i++ ) {
     
        if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) )) {

            return i;
        }
    }

    //
    // Name not found - oh, well
    //
    return 0;
}


//----------------------------------------------------------------------
//
// FilemonGetProcess
//
// Uses undocumented data structure offsets to obtain the name of the
// currently executing process.
//
//----------------------------------------------------------------------
PCHAR
FilemonGetProcess( 
    PCHAR ProcessName 
    )
{
    PEPROCESS       curproc;
    char            *nameptr;
    ULONG           i;

    //
    // We only do this if we determined the process name offset
    //
    if( ProcessNameOffset ) {
      
        //
        // Get a pointer to the current process block
        //
        curproc = PsGetCurrentProcess();

        //
        // Dig into it to extract the name. Make sure to leave enough room
        // in the buffer for the appended process ID.
        //
        nameptr   = (PCHAR) curproc + ProcessNameOffset;
         
        strncpy( ProcessName, nameptr, NT_PROCNAMELEN-1 );
        ProcessName[NT_PROCNAMELEN-1] = 0;
#if defined(_IA64_)
        sprintf( ProcessName + strlen(ProcessName), ":%I64d", PsGetCurrentProcessId());
#else
        sprintf( ProcessName + strlen(ProcessName), ":%d", PsGetCurrentProcessId());
#endif

    } else {

        strcpy( ProcessName, "???" );
    }
    return ProcessName;
}


//----------------------------------------------------------------------
//          H O O K / U N H O O K   R O U T I N E S
//----------------------------------------------------------------------

#if DBG        
//----------------------------------------------------------------------
//
// UnloadDetach
//
// Detaches from all devices for an unload
//
//----------------------------------------------------------------------
VOID 
UnloadDetach( 
    VOID 
    )
{
    ULONG           drive, i;
    PDEVICE_OBJECT  device;
    PHOOK_EXTENSION hookExt;
    
    //
    // Detach from file system devices
    //
    for( drive = 0; drive < 26; drive++ ) {

        if( DriveHookDevices[drive] ) {

            device = DriveHookDevices[drive];
            hookExt = device->DeviceExtension;
            IoDetachDevice( hookExt->FileSystem );
            IoDeleteDevice( device );

            for( i =0; i < 26; i++ ) {

                if( DriveHookDevices[i] == device ) {

                    DriveHookDevices[i] = NULL;
                }
            }
        }
    }

    //
    // Detach from special devices
    //
    if( NamedPipeHookDevice ) {

        IoDetachDevice( NamedPipeHookDevice );
        IoDeleteDevice( NamedPipeHookDevice );
    }
    if( MailSlotHookDevice ) {

        IoDetachDevice( MailSlotHookDevice );
        IoDeleteDevice( MailSlotHookDevice );
    }
}
#endif // DBG

//----------------------------------------------------------------------
//
// HookSpecialFs
//
// Hook the named pipe or mail slot file system.
//
//----------------------------------------------------------------------
BOOLEAN 
HookSpecialFs( 
    IN PDRIVER_OBJECT DriverObject, 
    FILE_SYSTEM_TYPE FsType 
    )
{
    IO_STATUS_BLOCK     ioStatus;
    HANDLE              ntFileHandle;   
    OBJECT_ATTRIBUTES   objectAttributes;
    PDEVICE_OBJECT      fileSysDevice;
    PDEVICE_OBJECT      topAttachDevice;
    PDEVICE_OBJECT      hookDevice;
    UNICODE_STRING      fileNameUnicodeString;
    WCHAR               npfsFilename[] = L"\\Device\\NamedPipe";
    WCHAR               msfsFilename[] = L"\\Device\\MailSlot";
    NTSTATUS            ntStatus;
    ULONG               i;
    PFILE_OBJECT        fileObject;
    PHOOK_EXTENSION     hookExtension;

    //
    // If we've already hooked it, just return success
    //
    if( FsType == NPFS && NamedPipeHookDevice ) return TRUE;
    if( FsType == MSFS && MailSlotHookDevice ) return TRUE;
    
    //
    // We have to figure out what device to hook - first open the volume's 
    // root directory
    //
    if( FsType == NPFS ) RtlInitUnicodeString( &fileNameUnicodeString, npfsFilename );
    else                 RtlInitUnicodeString( &fileNameUnicodeString, msfsFilename );
    InitializeObjectAttributes( &objectAttributes, &fileNameUnicodeString, 
                                OBJ_CASE_INSENSITIVE, NULL, NULL );
    ntStatus = ZwCreateFile( &ntFileHandle, SYNCHRONIZE|FILE_ANY_ACCESS, 
                             &objectAttributes, &ioStatus, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 
                             FILE_OPEN, 
                             FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE, 
                             NULL, 0 );
    if( !NT_SUCCESS( ntStatus ) ) {

        DbgPrint(("Filemon: Could not open %s\n", FsType == NPFS ? "NPFS" : "MSFS", ntStatus ));

        return FALSE;
    }

    DbgPrint(("Filemon:  opened the root directory!!! handle: %x\n", ntFileHandle));   

    //
    // Got the file handle, so now look-up the file-object it refers to
    //
    ntStatus = ObReferenceObjectByHandle( ntFileHandle, FILE_READ_DATA, 
                                          NULL, KernelMode, &fileObject, NULL );
    if( !NT_SUCCESS( ntStatus )) {

        DbgPrint(("Filemon: Could not get fileobject from %s handle: %x\n", 
                  FsType == NPFS ? "NPFS" : "MSFS", ntStatus ));
        ZwClose( ntFileHandle );

        return FALSE;
    }

    //  
    // Next, find out what device is associated with the file object by getting its related
    // device object
    //
    fileSysDevice = IoGetRelatedDeviceObject( fileObject );

    if( ! fileSysDevice ) {

        DbgPrint(("Filemon: Could not get related device object for %s: %x\n", 
                  FsType == NPFS ? "NPFS" : "MSFS", ntStatus ));

        ObDereferenceObject( fileObject );
        ZwClose( ntFileHandle );

        return FALSE;
    }

    //
    // The file system's device hasn't been hooked already, so make a hooking device
    //  object that will be attached to it.
    //
    ntStatus = IoCreateDevice( DriverObject,
                               sizeof(HOOK_EXTENSION),
                               NULL,
                               fileSysDevice->DeviceType,
                               0,
                               FALSE,
                               &hookDevice );
    if( !NT_SUCCESS(ntStatus) ) {

        DbgPrint(("Filemon: failed to create associated device %s: %x\n", 
                  FsType == NPFS ? "NPFS" : "MSFS", ntStatus ));

        ObDereferenceObject( fileObject );
        ZwClose( ntFileHandle );

        return FALSE;
    }

    //
    // Clear the device's init flag as per NT DDK KB article on creating device 
    // objects from a dispatch routine
    //
    hookDevice->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Finally, attach to the device. The second we're successfully attached, we may 
    // start receiving IRPs targetted at the device we've hooked.
    //
    topAttachDevice = IoAttachDeviceToDeviceStack( hookDevice, fileSysDevice );
    if( !topAttachDevice )  {

        //
        // Couldn' attach for some reason
        //
        DbgPrint(("Filemon: Connect with Filesystem failed: %s (%x) =>%x\n", 
                  FsType == NPFS ? "NPFS" : "MSFS", fileSysDevice, ntStatus ));

        //
        // Derefence the object and get out
        //
        ObDereferenceObject( fileObject );
        ZwClose( ntFileHandle );

        return FALSE;

    } else {

        DbgPrint(("Filemon: Successfully connected to Filesystem device %s\n",
                  FsType == NPFS ? "NPFS" : "MSFS" ));
    }

    //
    // Setup the device extensions. The drive letter and file system object are stored
    // in the extension.
    //
    hookExtension = hookDevice->DeviceExtension;
    hookExtension->LogicalDrive = '\\';
    hookExtension->FileSystem   = topAttachDevice;
    hookExtension->Hooked       = TRUE;
    hookExtension->Type = FsType;
    
    //
    // Close the file and update the hooked drive list by entering a
    // pointer to the hook device object in it.
    //
    ObDereferenceObject( fileObject );
    ZwClose( ntFileHandle );

    if( FsType == NPFS ) NamedPipeHookDevice = hookDevice;
    else                 MailSlotHookDevice  = hookDevice;

    return TRUE;
}


//----------------------------------------------------------------------
//
// UnhookSpecialFs
//
// Unhook the named pipe file or mail slot system.
//
//----------------------------------------------------------------------
VOID 
UnhookSpecialFs( 
    FILE_SYSTEM_TYPE FsType 
    )
{
    PHOOK_EXTENSION   hookExt;

    if( FsType == NPFS && NamedPipeHookDevice ) {
        
        hookExt = NamedPipeHookDevice->DeviceExtension;
        hookExt->Hooked = FALSE;
        NamedPipeHookDevice = NULL;

    } else if( FsType == MSFS && MailSlotHookDevice ) {

        hookExt = MailSlotHookDevice->DeviceExtension;
        hookExt->Hooked = FALSE;
        MailSlotHookDevice = NULL;
    }
}


//----------------------------------------------------------------------
//
// HookDrive
//
// Hook the drive specified by determining which device object to 
// attach to. The algorithm used here is similar to the one used
// internally by NT to determine which device object a file system request
// is directed at.
//
//----------------------------------------------------------------------
BOOLEAN 
HookDrive( 
    IN ULONG Drive, 
    IN PDRIVER_OBJECT DriverObject 
    )
{
    IO_STATUS_BLOCK     ioStatus;
    HANDLE              ntFileHandle;   
    OBJECT_ATTRIBUTES   objectAttributes;
    PDEVICE_OBJECT      fileSysDevice;
    PDEVICE_OBJECT      hookDevice;
    UNICODE_STRING      fileNameUnicodeString;
    PFILE_FS_ATTRIBUTE_INFORMATION fileFsAttributes;
    ULONG               fileFsAttributesSize;
    WCHAR               filename[] = L"\\DosDevices\\A:\\";
    NTSTATUS            ntStatus;
    ULONG               i;
    PFILE_OBJECT        fileObject;
    PHOOK_EXTENSION     hookExtension;
    
    //
    // Is it a legal drive letter?
    //
    if( Drive >= 26 )  {

        return FALSE;
    }

    //
    // Has this drive already been hooked?
    //
    if( DriveHookDevices[Drive] == NULL )  {

        //
        // Frob the name to make it refer to the drive specified in the input 
        // parameter.
        //
        filename[12] = (CHAR) ('A'+Drive);

        //
        // We have to figure out what device to hook - first open the volume's 
        // root directory
        //
        RtlInitUnicodeString( &fileNameUnicodeString, filename );
        InitializeObjectAttributes( &objectAttributes, &fileNameUnicodeString, 
                                    OBJ_CASE_INSENSITIVE, NULL, NULL );
        ntStatus = ZwCreateFile( &ntFileHandle, SYNCHRONIZE|FILE_ANY_ACCESS, 
                                 &objectAttributes, &ioStatus, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 
                                 FILE_OPEN, 
                                 FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE, 
                                 NULL, 0 );
        if( !NT_SUCCESS( ntStatus ) ) {

            DbgPrint(("Filemon: Could not open drive %c: %x\n", 'A'+Drive, ntStatus ));
            return FALSE;
        }

        DbgPrint(("Filemon:  opened the root directory!!! handle: %x\n", ntFileHandle));   

        //
        // Got the file handle, so now look-up the file-object it refers to
        //
        ntStatus = ObReferenceObjectByHandle( ntFileHandle, FILE_READ_DATA, 
                                              NULL, KernelMode, &fileObject, NULL );
        if( !NT_SUCCESS( ntStatus )) {

            DbgPrint(("Filemon: Could not get fileobject from handle: %c\n", 'A'+Drive ));
            ZwClose( ntFileHandle );
            return FALSE;
        }

        //  
        // Next, find out what device is associated with the file object by getting its related
        // device object
        //
        fileSysDevice = IoGetRelatedDeviceObject( fileObject );

        if( ! fileSysDevice ) {

            DbgPrint(("Filemon: Could not get related device object: %c\n", 'A'+Drive ));
            ObDereferenceObject( fileObject );
            ZwClose( ntFileHandle );
            return FALSE;
        }

        //  
        // Check the device list to see if we've already attached to this particular device. 
        // This can happen when more than one drive letter is being handled by the same network
        // redirecter
        //  
        for( i = 0; i < 26; i++ ) {

            if( DriveHookDevices[i] == fileSysDevice ) {

                //
                // If we're already watching it, associate this drive letter
                // with the others that are handled by the same network driver. This
                // enables us to intelligently update the hooking menus when the user
                // specifies that one of the group should not be watched -we mark all
                // of the related drives as unwatched as well
                //
                ObDereferenceObject( fileObject );
                ZwClose( ntFileHandle );
                DriveHookDevices[ Drive ] = fileSysDevice;
                return TRUE;
            }
        }

        //
        // The file system's device hasn't been hooked already, so make a hooking device
        //  object that will be attached to it.
        //
        ntStatus = IoCreateDevice( DriverObject,
                                   sizeof(HOOK_EXTENSION),
                                   NULL,
                                   fileSysDevice->DeviceType,
                                   0,
                                   FALSE,
                                   &hookDevice );
        if( !NT_SUCCESS(ntStatus) ) {

            DbgPrint(("Filemon: failed to create associated device: %c\n", 'A'+Drive ));   

            ObDereferenceObject( fileObject );
            ZwClose( ntFileHandle );

            return FALSE;
        }

        //
        // Clear the device's init flag as per NT DDK KB article on creating device 
        // objects from a dispatch routine
        //
        hookDevice->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Setup the device extensions. The drive letter and file system object are stored
        // in the extension.
        //
        hookExtension = hookDevice->DeviceExtension;
        hookExtension->LogicalDrive = 'A'+Drive;
        hookExtension->FileSystem   = fileSysDevice;
        hookExtension->Hooked       = TRUE;
        hookExtension->Type         = STANDARD;

        //
        // Finally, attach to the device. The second we're successfully attached, we may 
        // start receiving IRPs targetted at the device we've hooked.
        //
        ntStatus = IoAttachDeviceByPointer( hookDevice, fileSysDevice );
        if( !NT_SUCCESS(ntStatus) )  {

            //
            // Couldn' attach for some reason
            //
            DbgPrint(("Filemon: Connect with Filesystem failed: %c (%x) =>%x\n", 
                      'A'+Drive, fileSysDevice, ntStatus ));

            //
            // Derefence the object and get out
            //
            ObDereferenceObject( fileObject );
            ZwClose( ntFileHandle );

            return FALSE;

        } else {

            // 
            // Make a new drive group for the device,l if it does not have one 
            // already
            // 
            DbgPrint(("Filemon: Successfully connected to Filesystem device %c\n", 'A'+Drive ));
        }

        //
        // Determine if this is a NTFS drive
        //
        fileFsAttributesSize = sizeof( FILE_FS_ATTRIBUTE_INFORMATION) + MAXPATHLEN;
        hookExtension->FsAttributes = (PFILE_FS_ATTRIBUTE_INFORMATION) ExAllocatePool( NonPagedPool, 
                                                                                       fileFsAttributesSize );
        if( hookExtension->FsAttributes &&
            !NT_SUCCESS( IoQueryVolumeInformation( fileObject, FileFsAttributeInformation,
                                                   fileFsAttributesSize, hookExtension->FsAttributes, 
                                                   &fileFsAttributesSize ))) {

            //
            // On failure, we just don't have attributes for this file system
            //
            ExFreePool( hookExtension->FsAttributes );
            hookExtension->FsAttributes = NULL;
        } 

        //
        // Close the file and update the hooked drive list by entering a
        // pointer to the hook device object in it.
        //
        ObDereferenceObject( fileObject );

        ZwClose( ntFileHandle );

        DriveHookDevices[Drive] = hookDevice;
        
    } else {

        hookExtension = DriveHookDevices[Drive]->DeviceExtension;
        hookExtension->Hooked = TRUE;
    }
    return TRUE;
}


//----------------------------------------------------------------------
//
// UnhookDrive
//
// Unhook a previously hooked drive.
//
//----------------------------------------------------------------------
VOID 
UnhookDrive( 
    IN ULONG Drive 
    )
{
    PHOOK_EXTENSION hookExt;

    //
    // If the drive has been hooked, unhook it and delete the hook
    // device object
    //
    if( DriveHookDevices[Drive] )  {

        hookExt = DriveHookDevices[Drive]->DeviceExtension;
        hookExt->Hooked = FALSE;
    }
}


//----------------------------------------------------------------------
//
// HookDriveSet
//
// Hook/Unhook a set of drives specified by user. Return the set 
// that is currently hooked.
//
//----------------------------------------------------------------------
ULONG 
HookDriveSet( 
    IN ULONG DriveSet, 
    IN PDRIVER_OBJECT DriverObject 
    )
{
    PHOOK_EXTENSION hookExt;
    ULONG           drive, i;
    ULONG           bit;

    //
    // Scan the drive table, looking for hits on the DriveSet bitmask
    //
    for ( drive = 0; drive < 26; ++drive )  {

        bit = 1 << drive;

        //
        // Are we supposed to hook this drive?
        //
        if( (bit & DriveSet) &&
            !(bit & CurrentDriveSet) )  {

            //
            // Try to hook drive 
            //
            if( !HookDrive( drive, DriverObject ) ) {
             
                //
                // Remove from drive set if can't be hooked
                //
                DriveSet &= ~bit;

            } else {

                //
                // hook drives in same drive group      
                //
                for( i = 0; i < 26; i++ ) {

                    if( DriveHookDevices[i] == DriveHookDevices[ drive ] ) {

                        DriveSet |= ( 1<<i );
                    }
                }
            }

        } else if( !(bit & DriveSet) && 
                   (bit & CurrentDriveSet) ) {

            // 
            // Unhook this drive and all in the group
            //
            for( i = 0; i< 26; i++ ) {

                if( DriveHookDevices[i] == DriveHookDevices[ drive ] ) {

                    UnhookDrive( i );
                    DriveSet &= ~(1 << i); 
                }
            }
        }
    }

    //
    // Return set of drives currently hooked
    //
    CurrentDriveSet = DriveSet;
    return DriveSet;
}

//----------------------------------------------------------------------
//
// ControlCodeString
//
// Takes a control code and sees if we know what it is.
//
//----------------------------------------------------------------------
PCHAR 
ControlCodeString( 
    PIO_STACK_LOCATION IrpSp,
    ULONG ControlCode, 
    PCHAR Buffer, 
    PCHAR Other 
    )
{
    Other[0] = 0;
    switch( ControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        strcpy( Buffer, "FSCTL_REQUEST_OPLOCK_LEVEL_1" );
        break;
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        strcpy( Buffer, "FSCTL_REQUEST_OPLOCK_LEVEL_2" );
        break;
    case FSCTL_REQUEST_BATCH_OPLOCK:
        strcpy( Buffer, "FSCTL_REQUEST_BATCH_OPLOCK" );
        break;        
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
        strcpy( Buffer, "FSCTL_OPLOCK_BREAK_ACKNOWLEDGE" );
        break;
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
        strcpy( Buffer, "FSCTL_OPBATCH_ACK_CLOSE_PENDING" );
        break;
    case FSCTL_OPLOCK_BREAK_NOTIFY:
        strcpy( Buffer, "FSCTL_OPLOCK_BREAK_NOTIFY" );
        break;
    case FSCTL_LOCK_VOLUME:
        strcpy( Buffer, "FSCTL_LOCK_VOLUME" );
        break;
    case FSCTL_UNLOCK_VOLUME:
        strcpy( Buffer, "FSCTL_UNLOCK_VOLUME" );
        break;
    case FSCTL_DISMOUNT_VOLUME:
        strcpy( Buffer, "FSCTL_DISMOUNT_VOLUME" );
        break;
    case FSCTL_IS_VOLUME_MOUNTED:
        strcpy( Buffer, "FSCTL_IS_VOLUME_MOUNTED" );
        break;
    case FSCTL_IS_PATHNAME_VALID:
        strcpy( Buffer, "FSCTL_IS_PATHNAME_VALID" );
        break;
    case FSCTL_MARK_VOLUME_DIRTY:
        strcpy( Buffer, "FSCTL_MARK_VOLUME_DIRTY" );
        break;
    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        strcpy( Buffer, "FSCTL_QUERY_RETRIEVAL_POINTERS" );
        break;
    case FSCTL_GET_COMPRESSION:
        strcpy( Buffer, "FSCTL_GET_COMPRESSION" );
        break;
    case FSCTL_SET_COMPRESSION:
        strcpy( Buffer, "FSCTL_SET_COMPRESSION" );
        break;
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:
        strcpy( Buffer, "FSCTL_OPLOCK_BREAK_ACK_NO_2" );
        break;
    case FSCTL_QUERY_FAT_BPB:
        strcpy( Buffer, "FSCTL_QUERY_FAT_BPB" );
        break;
    case FSCTL_REQUEST_FILTER_OPLOCK:
        strcpy( Buffer, "FSCTL_REQUEST_FILTER_OPLOCK" );
        break;
    case FSCTL_FILESYSTEM_GET_STATISTICS:
        strcpy( Buffer, "FSCTL_FILESYSTEM_GET_STATISTICS" );
        break;
    case FSCTL_GET_NTFS_VOLUME_DATA:
        strcpy( Buffer, "FSCTL_GET_NTFS_VOLUME_DATA" );
        break;
    case FSCTL_GET_NTFS_FILE_RECORD:
        strcpy( Buffer, "FSCTL_GET_NTFS_FILE_RECORD" );
        break;
    case FSCTL_GET_VOLUME_BITMAP:
        strcpy( Buffer, "FSCTL_GET_VOLUME_BITMAP" );
        break;
    case FSCTL_GET_RETRIEVAL_POINTERS:
        strcpy( Buffer, "FSCTL_GET_RETRIEVAL_POINTERS" );
        break;
    case FSCTL_MOVE_FILE:
        strcpy( Buffer, "FSCTL_MOVE_FILE" );
        break;
    case FSCTL_IS_VOLUME_DIRTY:
        strcpy( Buffer, "FSCTL_IS_VOLUME_DIRTY" );
        break;
    case FSCTL_ALLOW_EXTENDED_DASD_IO:
        strcpy( Buffer, "FSCTL_ALLOW_EXTENDED_DASD_IO" );
        break;
        //
        // *** new to Win2K (NT 5.0)
        //
    case FSCTL_READ_PROPERTY_DATA:
        strcpy( Buffer, "FSCTL_READ_PROPERTY_DATA" );
        break;
    case FSCTL_WRITE_PROPERTY_DATA:
        strcpy( Buffer, "FSCTL_WRITE_PROPERTY_DATA" );
        break;
    case FSCTL_FIND_FILES_BY_SID:
        strcpy( Buffer, "FSCTL_FIND_FILES_BY_SID" );
        break;
    case FSCTL_DUMP_PROPERTY_DATA:
        strcpy( Buffer, "FSCTL_DUMP_PROPERTY_DATA" );
        break;
    case FSCTL_SET_OBJECT_ID:
        strcpy( Buffer, "FSCTL_SET_OBJECT_ID" );
        break;
    case FSCTL_GET_OBJECT_ID:
        strcpy( Buffer, "FSCTL_GET_OBJECT_ID" );
        break;
    case FSCTL_DELETE_OBJECT_ID:
        strcpy( Buffer, "FSCTL_DELETE_OBJECT_ID" );
        break;
    case FSCTL_SET_REPARSE_POINT:
        strcpy( Buffer, "FSCTL_SET_REPARSE_POINT" );
        break;
    case FSCTL_GET_REPARSE_POINT:
        strcpy( Buffer, "FSCTL_GET_REPARSE_POINT" );
        break;
    case FSCTL_DELETE_REPARSE_POINT:
        strcpy( Buffer, "FSCTL_DELETE_REPARSE_POINT" );
        break;
    case FSCTL_ENUM_USN_DATA:
        strcpy( Buffer, "FSCTL_ENUM_USN_DATA" );
        break;
    case FSCTL_SECURITY_ID_CHECK:
        strcpy( Buffer, "FSCTL_SECURITY_ID_CHECK" );
        break;
    case FSCTL_READ_USN_JOURNAL:
        strcpy( Buffer, "FSCTL_READ_USN_JOURNAL" );
        break;
    case FSCTL_SET_OBJECT_ID_EXTENDED:
        strcpy( Buffer, "FSCTL_SET_OBJECT_ID_EXTENDED" );
        break;
    case FSCTL_CREATE_OR_GET_OBJECT_ID:
        strcpy( Buffer, "FSCTL_CREATE_OR_GET_OBJECT_ID" );
        break;
    case FSCTL_SET_SPARSE:
        strcpy( Buffer, "FSCTL_SET_SPARSE" );
        break;
    case FSCTL_SET_ZERO_DATA:
        strcpy( Buffer, "FSCTL_SET_ZERO_DATA" );
        break;
    case FSCTL_QUERY_ALLOCATED_RANGES:
        strcpy( Buffer, "FSCTL_QUERY_ALLOCATED_RANGES" );
        break;
    case FSCTL_ENABLE_UPGRADE:
        strcpy( Buffer, "FSCTL_ENABLE_UPGRADE" );
        break;
    case FSCTL_SET_ENCRYPTION:
        strcpy( Buffer, "FSCTL_SET_ENCRYPTION" );
        break;
    case FSCTL_ENCRYPTION_FSCTL_IO:
        strcpy( Buffer, "FSCTL_ENCRYPTION_FSCTL_IO" );
        break;
    case FSCTL_WRITE_RAW_ENCRYPTED:
        strcpy( Buffer, "FSCTL_WRITE_RAW_ENCRYPTED" );
        break;
    case FSCTL_READ_RAW_ENCRYPTED:
        strcpy( Buffer, "FSCTL_READ_RAW_ENCRYPTED" );
        break;
    case FSCTL_CREATE_USN_JOURNAL:
        strcpy( Buffer, "FSCTL_CREATE_USN_JOURNAL" );
        break;
    case FSCTL_READ_FILE_USN_DATA:
        strcpy( Buffer, "FSCTL_READ_FILE_USN_DATA" );
        break;
    case FSCTL_WRITE_USN_CLOSE_RECORD:
        strcpy( Buffer, "FSCTL_WRITE_USN_CLOSE_RECORD" );
        break;
    case FSCTL_EXTEND_VOLUME:
        strcpy( Buffer, "FSCTL_EXTEND_VOLUME" );
        break;
        //
        // Named pipe file system controls 
        // (these are all undocumented)
        //
    case FSCTL_PIPE_DISCONNECT:
        strcpy( Buffer, "FSCTL_PIPE_DISCONNECT" );
        break;
    case FSCTL_PIPE_ASSIGN_EVENT:
        strcpy( Buffer, "FSCTL_PIPE_ASSIGN_EVENT" );
        break;
    case FSCTL_PIPE_QUERY_EVENT:
        strcpy( Buffer, "FSCTL_PIPE_QUERY_EVENT" );
        break;
    case FSCTL_PIPE_LISTEN:
        strcpy( Buffer, "FSCTL_PIPE_LISTEN" );
        break;
    case FSCTL_PIPE_IMPERSONATE:
        strcpy( Buffer, "FSCTL_PIPE_IMPERSONATE" );
        break;
    case FSCTL_PIPE_WAIT:
        strcpy( Buffer, "FSCTL_PIPE_WAIT" );
        break;
    case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
        strcpy( Buffer, "FSCTL_QUERY_CLIENT_PROCESS" );
        break;
    case FSCTL_PIPE_SET_CLIENT_PROCESS:
        strcpy( Buffer, "FSCTL_PIPE_SET_CLIENT_PROCESS");
        break;
    case FSCTL_PIPE_PEEK:
        strcpy( Buffer, "FSCTL_PIPE_PEEK" );
        break;
    case FSCTL_PIPE_INTERNAL_READ:
        strcpy( Buffer, "FSCTL_PIPE_INTERNAL_READ" );
        sprintf( Other, "ReadLen: %d", 
                 IrpSp->Parameters.DeviceIoControl.InputBufferLength );
        break;
    case FSCTL_PIPE_INTERNAL_WRITE:
        strcpy( Buffer, "FSCTL_PIPE_INTERNAL_WRITE" );
        sprintf( Other, "WriteLen: %d", 
                 IrpSp->Parameters.DeviceIoControl.InputBufferLength );
        break;
    case FSCTL_PIPE_TRANSCEIVE:
        strcpy( Buffer, "FSCTL_PIPE_TRANSCEIVE" );
        sprintf( Other, "WriteLen: %d ReadLen: %d",
                 IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                 IrpSp->Parameters.DeviceIoControl.OutputBufferLength );
        break;
    case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
        strcpy( Buffer, "FSCTL_PIPE_INTERNAL_TRANSCEIVE" );
        sprintf( Other, "WriteLen: %d ReadLen: %d",
                 IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                 IrpSp->Parameters.DeviceIoControl.OutputBufferLength );
        break;        
        //
        // Mail slot file system controls
        // (these are all undocumented)
        //
    case FSCTL_MAILSLOT_PEEK:
        strcpy( Buffer, "FSCTL_MAILSLOT_PEEK" );
        break;

        //
        // Undocumented network redirector controls
        //
    case FSCTL_NETWORK_GET_CONNECTION_INFO:
        strcpy( Buffer, "FSCTL_NETWORK_GET_CONNECTION_INFO" );
        break;
    case FSCTL_NETWORK_ENUMERATE_CONNECTIONS:
        strcpy( Buffer, "FSCTL_NETWORK_ENUMERATE_CONNECTIONS");
        break;
    case FSCTL_NETWORK_DELETE_CONNECTION:
        strcpy( Buffer, "FSCTL_NETWORK_DELETE_CONNECTION" );
        break;
    case FSCTL_NETWORK_SET_CONFIGURATION_INFO:
        strcpy( Buffer, "FSCTL_NETWORK_SET_CONFIGURATION_INFO" );
        break;
    case FSCTL_NETWORK_GET_CONFIGURATION_INFO:
        strcpy( Buffer, "FSCTL_NETWORK_GET_CONFIGURATION_INFO" );
        break;
    case FSCTL_NETWORK_GET_STATISTICS:
        strcpy( Buffer, "FSCTL_NETWORK_GET_STATISTICS" );
        break;
    case FSCTL_NETWORK_SET_DOMAIN_NAME:
        strcpy( Buffer, "FSCTL_NETWORK_SET_DOMAIN_NAME" );
        break;
    case FSCTL_NETWORK_REMOTE_BOOT_INIT_SCRT:
        strcpy( Buffer, "FSCTL_NETWORK_REMOTE_BOOT_INIT_SCRT" );
        break;
    default:

        sprintf( Buffer, "IOCTL: 0x%X", ControlCode );
        break;
    }
    return Buffer;
}


//----------------------------------------------------------------------
//
// ErrorString
//
// Returns string representing the passed error condition.
//
//----------------------------------------------------------------------
PCHAR 
ErrorString( 
    NTSTATUS RetStat, 
    PCHAR Buffer
    ) 
{
    switch( RetStat ) {

    case STATUS_SUCCESS:
        strcpy( Buffer, "SUCCESS" );
        break;
    case STATUS_CRC_ERROR:
        strcpy( Buffer, "CRC ERROR" );
        break;
    case STATUS_NOT_IMPLEMENTED:
        strcpy( Buffer, "NOT IMPLEMENTED" );
        break;
    case STATUS_EAS_NOT_SUPPORTED:
        strcpy( Buffer, "EAS NOT SUPPORTED" );
        break;
    case STATUS_EA_TOO_LARGE:
        strcpy( Buffer, "EA TOO LARGE");
        break;
    case STATUS_NONEXISTENT_EA_ENTRY:
        strcpy( Buffer, "NONEXISTENT EA ENTRY");
        break;
    case STATUS_BAD_NETWORK_NAME:
        strcpy( Buffer, "BAD NETWORK NAME" );
        break;
    case STATUS_NOTIFY_ENUM_DIR:
        strcpy( Buffer, "NOTIFY ENUM DIR" );
        break;
    case STATUS_FILE_CORRUPT_ERROR:
        strcpy( Buffer, "FILE CORRUPT" );
        break;
    case STATUS_DISK_CORRUPT_ERROR:
        strcpy( Buffer, "DISK CORRUPT" );
        break;
    case STATUS_RANGE_NOT_LOCKED:
        strcpy( Buffer, "RANGE NOT LOCKED" );
        break;
    case STATUS_FILE_CLOSED:
        strcpy( Buffer, "FILE CLOSED" );
        break;
    case STATUS_IN_PAGE_ERROR:
        strcpy( Buffer, "IN PAGE ERROR" );
        break;
    case STATUS_CANCELLED:
        strcpy( Buffer, "CANCELLED" );
        break;
    case STATUS_QUOTA_EXCEEDED:
        strcpy( Buffer, "QUOTA EXCEEDED" );
        break;
    case STATUS_NOT_SUPPORTED:
        strcpy( Buffer, "NOT SUPPORTED" );
        break;
    case STATUS_NO_MORE_FILES:
        strcpy( Buffer, "NO MORE FILES" );
        break;
    case STATUS_BUFFER_TOO_SMALL:
        strcpy( Buffer, "BUFFER TOO SMALL" );
        break;
    case STATUS_OBJECT_NAME_INVALID:
        strcpy( Buffer, "NAME INVALID" );
        break;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        strcpy( Buffer, "FILE NOT FOUND" );
        break;
    case STATUS_NOT_A_DIRECTORY:
        strcpy( Buffer, "NOT A DIRECTORY" );
        break;
    case STATUS_NO_SUCH_FILE:
        strcpy( Buffer, "NO SUCH FILE" );
        break;
    case STATUS_OBJECT_NAME_COLLISION:
        strcpy( Buffer, "NAME COLLISION" );
        break;
    case STATUS_NONEXISTENT_SECTOR:
        strcpy( Buffer, "NONEXISTENT SECTOR" );
        break;
    case STATUS_BAD_NETWORK_PATH:
        strcpy( Buffer, "BAD NETWORK PATH" );
        break;
    case STATUS_OBJECT_PATH_NOT_FOUND:
        strcpy( Buffer, "PATH NOT FOUND" );
        break;
    case STATUS_NO_SUCH_DEVICE:
        strcpy( Buffer, "INVALID PARAMETER" );
        break;
    case STATUS_END_OF_FILE:
        strcpy( Buffer, "END OF FILE" );
        break;
    case STATUS_NOTIFY_CLEANUP:
        strcpy( Buffer, "NOTIFY CLEANUP" );
        break;
    case STATUS_BUFFER_OVERFLOW:
        strcpy( Buffer, "BUFFER OVERFLOW" );
        break;
    case STATUS_NO_MORE_ENTRIES:
        strcpy( Buffer, "NO MORE ENTRIES" );
        break;
    case STATUS_ACCESS_DENIED:
        strcpy( Buffer, "ACCESS DENIED" );
        break;
    case STATUS_SHARING_VIOLATION:
        strcpy( Buffer, "SHARING VIOLATION" );
        break;       
    case STATUS_INVALID_PARAMETER:
        strcpy( Buffer, "INVALID PARAMETER" );
        break;       
    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        strcpy( Buffer, "OPLOCK BREAK" );
        break;        
    case STATUS_OPLOCK_NOT_GRANTED:
        strcpy( Buffer, "OPLOCK NOT GRANTED" );
        break;
    case STATUS_FILE_LOCK_CONFLICT:
        strcpy( Buffer, "FILE LOCK CONFLICT" );
        break;
    case STATUS_PENDING:
        strcpy( Buffer, "PENDING" );
        break;       
    case STATUS_REPARSE:
        strcpy( Buffer, "REPARSE" );
        break;       
    case STATUS_MORE_ENTRIES:
        strcpy( Buffer, "MORE" );
        break;       
    case STATUS_DELETE_PENDING:
        strcpy( Buffer, "DELETE PEND" );
        break;       
    case STATUS_CANNOT_DELETE:
        strcpy( Buffer, "CANNOT DELETE" );
        break;       
    case STATUS_LOCK_NOT_GRANTED:
        strcpy( Buffer, "NOT GRANTED" );
        break;       
    case STATUS_FILE_IS_A_DIRECTORY:
        strcpy( Buffer, "IS DIRECTORY" );
        break;
    case STATUS_ALREADY_COMMITTED:
        strcpy( Buffer, "ALREADY COMMITTED" );
        break;
    case STATUS_INVALID_EA_FLAG:
        strcpy( Buffer, "INVALID EA FLAG" );
        break;
    case STATUS_INVALID_INFO_CLASS:
        strcpy( Buffer, "INVALID INFO CLASS" );
        break;
    case STATUS_INVALID_HANDLE:
        strcpy( Buffer, "INVALID HANDLE" );
        break;
    case STATUS_INVALID_DEVICE_REQUEST:
        strcpy( Buffer, "INVALID DEVICE REQUEST" );
        break;
    case STATUS_WRONG_VOLUME:
        strcpy( Buffer, "WRONG VOLUME" );
        break;
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        strcpy( Buffer, "NETWORK ERROR" );
        break;
    case STATUS_DFS_UNAVAILABLE:
        strcpy( Buffer, "DFS UNAVAILABLE" );
        break;
    case STATUS_LOG_FILE_FULL:
        strcpy( Buffer, "LOG FILE FULL" );
    	break;
    case STATUS_INVALID_DEVICE_STATE:
        strcpy( Buffer, "INVALID DEVICE STATE" );
        break;
    case STATUS_NO_MEDIA_IN_DEVICE:
        strcpy( Buffer, "NO MEDIA");
        break;
    case STATUS_DISK_FULL:
        strcpy( Buffer, "DISK FULL");
        break;
    case STATUS_DIRECTORY_NOT_EMPTY:
        strcpy( Buffer, "NOT EMPTY");
        break;

        //
        // Named pipe errors
        //
    case STATUS_INSTANCE_NOT_AVAILABLE:
        strcpy( Buffer, "INSTANCE NOT AVAILABLE" );
        break;
    case STATUS_PIPE_NOT_AVAILABLE:
        strcpy( Buffer, "PIPE NOT AVAILABLE" );
        break;
    case STATUS_INVALID_PIPE_STATE:
        strcpy( Buffer, "INVALID PIPE STATE" );
        break;
    case STATUS_PIPE_BUSY:
        strcpy( Buffer, "PIPE BUSY" );
        break;
    case STATUS_PIPE_DISCONNECTED:
        strcpy( Buffer, "PIPE DISCONNECTED" );
        break;
    case STATUS_PIPE_CLOSING:
        strcpy( Buffer, "PIPE CLOSING" );
        break;
    case STATUS_PIPE_CONNECTED:
        strcpy( Buffer, "PIPE CONNECTED" );
        break;
    case STATUS_PIPE_LISTENING:
        strcpy( Buffer, "PIPE LISTENING" );
        break;
    case STATUS_INVALID_READ_MODE:
        strcpy( Buffer, "INVALID READ MODE" );
        break;
    case STATUS_PIPE_EMPTY:
        strcpy( Buffer, "PIPE EMPTY" );
        break;
    case STATUS_PIPE_BROKEN:
        strcpy( Buffer, "PIPE BROKEN" );
        break;
    case STATUS_IO_TIMEOUT:
        strcpy( Buffer, "IO TIMEOUT" );
        break;
    default:
        sprintf( Buffer, "* 0x%X", RetStat );
        break;
    }

    return Buffer;
}

//----------------------------------------------------------------------
//
// CreateOptionsString
//
// Takes the options mask and returns a string that represents
// the settings.
//
//----------------------------------------------------------------------
PCHAR 
CreateOptionsString( 
    ULONG Options, 
    PCHAR Buffer 
    )
{
    ULONG  disposition;

    Buffer[0] = 0;
    disposition = (Options >> 24) & 0xFF;
    
    switch( disposition ) {
    case FILE_SUPERSEDE:
        strcat( Buffer, "Supersede " );
        break;
    case FILE_CREATE:
        strcat( Buffer, "Create " );
        break;
    case FILE_OPEN_IF:
        strcat( Buffer, "OpenIf " );
        break;
    case FILE_OPEN:
        strcat( Buffer, "Open " );
        break;
    case FILE_OVERWRITE:
        strcat( Buffer, "Overwrite " );
        break;
    case FILE_OVERWRITE_IF:
        strcat( Buffer, "OverwriteIf " );
        break;
    }

    if( Options & FILE_DIRECTORY_FILE ) 
        strcat( Buffer, "Directory " );
    if( Options & FILE_WRITE_THROUGH )
        strcat( Buffer, "WriteThrough " );
    if( Options & FILE_SEQUENTIAL_ONLY )
        strcat( Buffer, "Sequential " );
    if( Options & FILE_NO_INTERMEDIATE_BUFFERING )
        strcat( Buffer, "NoBuffer" );
    if( Options & FILE_OPEN_BY_FILE_ID ) 
        strcat( Buffer, "ByID");
    return Buffer;
}

//----------------------------------------------------------------------
//
// CreateAttributesString
//
// Take attributes and return a string that represents them.
//
//----------------------------------------------------------------------
PCHAR 
CreateAttributesString( 
    USHORT Attributes, 
    PCHAR Buffer 
    )
{
    Buffer[0] = 0;
    if( !Attributes ) {
        strcat( Buffer, "Any" );
        return Buffer;
    }
    if( Attributes & FILE_ATTRIBUTE_COMPRESSED) strcat( Buffer, "C" );
    if( Attributes & FILE_ATTRIBUTE_TEMPORARY) strcat( Buffer, "T" );
    if( Attributes & FILE_ATTRIBUTE_DIRECTORY) strcat( Buffer, "D" );
    if( Attributes & FILE_ATTRIBUTE_READONLY) strcat( Buffer, "R" );
    if( Attributes & FILE_ATTRIBUTE_HIDDEN )  strcat( Buffer, "H" );
    if( Attributes & FILE_ATTRIBUTE_SYSTEM )  strcat( Buffer, "S" );
    if( Attributes & FILE_ATTRIBUTE_ARCHIVE ) strcat( Buffer, "A" );
    if( Attributes & FILE_ATTRIBUTE_NORMAL )  strcat( Buffer, "N" );
    return Buffer;
}


//----------------------------------------------------------------------
//                F A S T I O   R O U T I N E S
//
// NOTE: There is no need for us to worry about accessing fastio 
// parameters within try/except because the I/O manager has either
// probed the validity of the arguments or calls within its own 
// try/except block (it doesn't trust us anyway :-) ).
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FilemonFastIoCheckIfPossible
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoCheckifPossible( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN         retval = FALSE;
    PHOOK_EXTENSION hookExt;
    CHAR            *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER   timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER   dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoCheckIfPossible ) ) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoCheckIfPossible( 
            FileObject, FileOffset, Length,
            Wait, LockKey, CheckForReadOperation, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();

            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_CHECK_IF_POSSIBLE\t%s\t%s Offset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ),fullPathName, 
                       CheckForReadOperation ? "Read:" : "Write:",
                       FileOffset->LowPart, Length, 
                       retval?"SUCCESS":"FAILURE" ); 
        }
        FREEPATHNAME();
    }

    return retval;
}


//----------------------------------------------------------------------
// 
// FilemonFastIoRead
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoRead( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoRead ) ) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoRead( 
            FileObject, FileOffset, Length,
            Wait, LockKey, Buffer, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_READ\t%s\tOffset: %d Length: %ld\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length, 
                       retval?ErrorString( IoStatus->Status, errorBuf):"FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoWrite
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoWrite( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length, 
    IN BOOLEAN Wait, 
    IN ULONG LockKey, 
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN              retval = FALSE;
    PHOOK_EXTENSION      hookExt;
    CHAR                 *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER        timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER        dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoWrite )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoWrite( 
            FileObject, FileOffset, Length, Wait, LockKey, 
            Buffer, IoStatus, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_WRITE\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length, 
                       retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" ); 
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoQueryBasicinfo
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoQueryBasicInfo( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait, 
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    CHAR                attributeString[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoQueryBasicInfo ) ) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoQueryBasicInfo( 
            FileObject, Wait, Buffer, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            if( retval ) {
                LogRecord( TRUE, NULL, 
                           &dateTime, &timeResult,
                           "%s\tFASTIO_QUERY_BASIC_INFO\t%s\tAttributes: %s\t%s", 
                           FilemonGetProcess( name ), fullPathName, 
                           NT_SUCCESS(IoStatus->Status) ? 
                           CreateAttributesString((USHORT)((PFILE_BASIC_INFORMATION) Buffer)->FileAttributes,
                                                  attributeString ) :
                           "Error",
                           retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" );

            } else { 
                LogRecord( TRUE, NULL, 
                           &dateTime, &timeResult,
                           "%s\tFASTIO_QUERY_BASIC_INFO\t%s\t\t%s", 
                           FilemonGetProcess( name ), fullPathName, retval?"SUCCESS":"FAILURE" );
            }
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoQueryStandardInfo
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoQueryStandardInfo( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait, 
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoQueryStandardInfo ) ) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoQueryStandardInfo( 
            FileObject, Wait, Buffer, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            if( retval ) {
                LogRecord( TRUE, NULL, 
                           &dateTime, &timeResult,
                           "%s\tFASTIO_QUERY_STANDARD_INFO\t%s\tSize: %d\t%s", 
                           FilemonGetProcess( name ), fullPathName,
                           ((PFILE_STANDARD_INFORMATION) Buffer)->EndOfFile.LowPart, 
                           retval?"SUCCESS":"FAILURE" );
            } else {
                LogRecord( TRUE, NULL, 
                           &dateTime, &timeResult,
                           "%s\tFASTIO_QUERY_STANDARD_INFO\t%s\t\t%s", 
                           FilemonGetProcess( name ), fullPathName, 
                           retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" );
            }
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoLock
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoLock( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length, 
    PEPROCESS ProcessId, 
    ULONG Key,
    BOOLEAN FailImmediately, 
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoLock )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoLock( 
            FileObject, FileOffset, Length, ProcessId, Key, FailImmediately, 
            ExclusiveLock, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_LOCK\t%s\tExcl: %s Offset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName,
                       ExclusiveLock ? "Yes":"No", FileOffset ? FileOffset->LowPart : 0,
                       Length ? Length->LowPart : 0, retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoUnlockSingle
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoUnlockSingle( 
    IN PFILE_OBJECT FileObject, 
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length, 
    PEPROCESS ProcessId, 
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;   
    
    if( FASTIOPRESENT( hookExt, FastIoUnlockSingle )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoUnlockSingle(
            FileObject, FileOffset, Length, ProcessId, Key, 
            IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_UNLOCK\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset? FileOffset->LowPart : 0, Length ? Length->LowPart : 0,
                       retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoUnlockAll
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoUnlockAll( 
    IN PFILE_OBJECT FileObject, 
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT(hookExt, FastIoUnlockAll ) ) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoUnlockAll( 
            FileObject, ProcessId, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_UNLOCK_ALL\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       retval?ErrorString( IoStatus->Status, errorBuf ):"FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoUnlockAllByKey
//
//----------------------------------------------------------------------    
BOOLEAN  
FilemonFastIoUnlockAllByKey( 
    IN PFILE_OBJECT FileObject, 
    PEPROCESS ProcessId, 
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoUnlockAllByKey )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoUnlockAllByKey( 
            FileObject, ProcessId, Key, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_UNLOCK_ALL_BY_KEY\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       retval?ErrorString( IoStatus->Status, errorBuf):"FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoQueryNetworkOpenInfo
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoQueryNetworkOpenInfo )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoQueryNetworkOpenInfo( 
            FileObject, Wait, Buffer, IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_QUERY_NETWORK_OPEN_INFO\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       retval ? ErrorString( IoStatus->Status, errorBuf ): "FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoAcquireForModWrite
//
//----------------------------------------------------------------------    
NTSTATUS 
FilemonFastIoAcquireForModWrite( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT struct _ERESOURCE **ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    NTSTATUS            retval = STATUS_NOT_IMPLEMENTED;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, errval[ERRORLEN], name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, AcquireForModWrite )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->AcquireForModWrite( 
            FileObject, EndingOffset, ResourceToRelease, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_ACQUIRE_FOR_MOD_WRITE\t%s\tEndOffset: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, EndingOffset, 
                       ErrorString( retval, errval ) );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoMdlRead
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoMdlRead( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, MdlRead )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->MdlRead( 
            FileObject, FileOffset, Length, LockKey, MdlChain, 
            IoStatus, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_MDL_READ\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length, 
                       retval ? ErrorString( IoStatus->Status, errorBuf ): "FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoMdlReadComplete
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoMdlReadComplete( 
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, MdlReadComplete )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = (BOOLEAN) hookExt->FileSystem->DriverObject->FastIoDispatch->MdlReadComplete( FileObject, 
                                                                                               MdlChain, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_MDL_READ_COMPLETE\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, "OK" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoPrepareMdlWrite
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoPrepareMdlWrite( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;
    IoStatus->Status      = STATUS_NOT_IMPLEMENTED;
    IoStatus->Information = 0;

    if( FASTIOPRESENT( hookExt, PrepareMdlWrite )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->PrepareMdlWrite( 
            FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, 
            hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_PREPARE_MDL_WRITE\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length, 
                       retval ? ErrorString( IoStatus->Status, errorBuf ): "FAILURE" );
        }
        FREEPATHNAME();
    } 
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoMdlWriteComplete
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoMdlWriteComplete( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;
    
    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, MdlWriteComplete )) { 

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->MdlWriteComplete( 
            FileObject, FileOffset, MdlChain, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_MDL_WRITE_COMPLETE\t%s\tOffset: %d\tOK", 
                       FilemonGetProcess( name ), fullPathName, FileOffset->LowPart );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoReadCompressed
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoReadCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PVOID Buffer,
    OUT PMDL *MdlChain, 
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoReadCompressed )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoReadCompressed( 
            FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatus,
            CompressedDataInfo, CompressedDataInfoLength, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_READ_COMPRESSED\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length,
                       retval ? ErrorString( IoStatus->Status, errorBuf ) : "FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoWriteCompressed
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoWriteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN ULONG Length,
    IN ULONG LockKey, 
    OUT PVOID Buffer,
    OUT PMDL *MdlChain, 
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN], errorBuf[ERRORLEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoWriteCompressed )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoWriteCompressed( 
            FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatus,
            CompressedDataInfo, CompressedDataInfoLength, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked ) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_WRITE_COMPRESSED\t%s\tOffset: %d Length: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, 
                       FileOffset->LowPart, Length, 
                       retval ? ErrorString( IoStatus->Status, errorBuf ) : "FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoMdlReadCompleteCompressed
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoMdlReadCompleteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, MdlReadCompleteCompressed )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->MdlReadCompleteCompressed( 
            FileObject, MdlChain, hookExt->FileSystem );

        if( FilterDef.logreads && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_MDL_READ_COMPLETE_COMPRESSED\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, "OK" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoMdlWriteCompleteCompressed
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoMdlWriteCompleteCompressed( 
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset, 
    IN PMDL MdlChain, 
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension; 

    if( FASTIOPRESENT( hookExt, MdlWriteCompleteCompressed )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->MdlWriteCompleteCompressed( 
            FileObject, FileOffset, MdlChain, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_MDL_WRITE_COMPLETE_COMPRESSED\t%s\tOffset: %d\t%s", 
                       FilemonGetProcess( name ), fullPathName, FileOffset->LowPart, "OK" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoQueryOpen
//
// This call actually passes an IRP! 
//
//----------------------------------------------------------------------    
BOOLEAN 
FilemonFastIoQueryOpen( 
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    BOOLEAN             retval = FALSE;
    PHOOK_EXTENSION     hookExt;
    PFILE_OBJECT        FileObject;
    CHAR                *fullPathName, name[PROCNAMELEN];
    PIO_STACK_LOCATION  currentIrpStack;
    PIO_STACK_LOCATION  nextIrpStack;
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return FALSE;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, FastIoQueryOpen )) {

        currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
        nextIrpStack    = IoGetNextIrpStackLocation(Irp);
        FileObject      = currentIrpStack->FileObject;

        //
        // copy parameters down to next level in the stack
        //
        *nextIrpStack = *currentIrpStack;
        nextIrpStack->DeviceObject = hookExt->FileSystem;
        IoSetNextIrpStackLocation( Irp );

        //
        // Get path and timestamp
        //
        GETPATHNAME(TRUE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoQueryOpen( 
            Irp, NetworkInformation, hookExt->FileSystem );

        //
        // Reset the stack location because pre-NT 5.0 checked builds complain
        //
        Irp->CurrentLocation++;
        Irp->Tail.Overlay.CurrentStackLocation++;

        if( FilterDef.logreads && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_QUERY_OPEN\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, retval ? "SUCCESS" : "FAILURE" );
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoReleaseForModWrite
//
//----------------------------------------------------------------------    
NTSTATUS 
FilemonFastIoReleaseForModWrite( 
    IN PFILE_OBJECT FileObject,
    IN struct _ERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    NTSTATUS            retval = STATUS_NOT_IMPLEMENTED;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, errval[ERRORLEN], name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;
    
    if( !DeviceObject ) return STATUS_NOT_IMPLEMENTED;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, ReleaseForModWrite )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->ReleaseForModWrite( 
            FileObject,  ResourceToRelease, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_RELEASE_FOR_MOD_WRITE\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, ErrorString( retval, errval ));
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoAcquireForCcFlush
//
//----------------------------------------------------------------------    
NTSTATUS 
FilemonFastIoAcquireForCcFlush( 
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    NTSTATUS            retval = STATUS_NOT_IMPLEMENTED;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, errval[ERRORLEN], name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return STATUS_NOT_IMPLEMENTED;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, AcquireForCcFlush )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->AcquireForCcFlush( 
            FileObject, hookExt->FileSystem );
        if( FilterDef.logwrites && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_ACQUIRE_FOR_CC_FLUSH\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, ErrorString( retval, errval));
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoReleaseForCcFlush
//
//----------------------------------------------------------------------    
NTSTATUS 
FilemonFastIoReleaseForCcFlush( 
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject 
    )
{
    NTSTATUS            retval = STATUS_NOT_IMPLEMENTED;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, errval[ERRORLEN], name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    if( !DeviceObject ) return STATUS_NOT_IMPLEMENTED;

    hookExt = DeviceObject->DeviceExtension;

    if( FASTIOPRESENT( hookExt, ReleaseForCcFlush )) {

        GETPATHNAME(FALSE);
        TIMESTAMPSTART();

        retval = hookExt->FileSystem->DriverObject->FastIoDispatch->ReleaseForCcFlush( 
            FileObject, hookExt->FileSystem );

        if( FilterDef.logwrites && hookExt->Hooked) {

            TIMESTAMPSTOP();
            LogRecord( TRUE, NULL, 
                       &dateTime, &timeResult,
                       "%s\tFASTIO_RELEASE_FOR_CC_FLUSH\t%s\t\t%s", 
                       FilemonGetProcess( name ), fullPathName, ErrorString( retval, errval) ); 
        }
        FREEPATHNAME();
    }
    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoDeviceControl
//
//----------------------------------------------------------------------
BOOLEAN  
FilemonFastIoDeviceControl( 
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait,
    IN PVOID InputBuffer, 
    IN ULONG InputBufferLength, 
    OUT PVOID OutputBuffer, 
    IN ULONG OutputBufferLength, 
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
    BOOLEAN             retval = FALSE;
    BOOLEAN             logMutexReleased;
    PHOOK_EXTENSION     hookExt;
    PLOG_BUF            oldLog, savedCurrentLog;
    CHAR                fullPathName[MAXPATHLEN], name[PROCNAMELEN], errorBuf[ERRORLEN];
    KIRQL               oldirql;
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    hookExt = DeviceObject->DeviceExtension;
    if( hookExt->Type == GUIINTERFACE ) {

        //
        // Its a message from our GUI!
        //
        IoStatus->Status      = STATUS_SUCCESS; // Assume success
        IoStatus->Information = 0;      // Assume nothing returned

        switch ( IoControlCode ) {

        case IOCTL_FILEMON_VERSION:

            //
            // Version #
            //
            if( OutputBufferLength >= sizeof(ULONG)) {

                *(ULONG *)OutputBuffer = FILEMONVERSION;
                IoStatus->Information = sizeof(ULONG);

            } else {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            }            
            break;

        case IOCTL_FILEMON_SETDRIVES:

            //
            // Hook and/or unhook drives
            //
            DbgPrint (("Filemon: set drives\n"));

            if( InputBufferLength >= sizeof(ULONG) &&
                 OutputBufferLength >= sizeof(ULONG)) {

                *(ULONG *)OutputBuffer = HookDriveSet( *(ULONG *)InputBuffer, DeviceObject->DriverObject );
                IoStatus->Information = sizeof(ULONG);

            } else {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_FILEMON_HOOKSPECIAL:

            if( InputBufferLength >= sizeof(FILE_SYSTEM_TYPE )) {

                if( !HookSpecialFs( DeviceObject->DriverObject, *(PFILE_SYSTEM_TYPE) InputBuffer )) {
                
                    IoStatus->Status = STATUS_UNSUCCESSFUL;
                }
            } else {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_FILEMON_UNHOOKSPECIAL:

            if( InputBufferLength >= sizeof(FILE_SYSTEM_TYPE )) {

                UnhookSpecialFs( *(PFILE_SYSTEM_TYPE) InputBuffer );

            } else {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_FILEMON_STOPFILTER:
            
            //
            // Turn off logging
            //
            DbgPrint(("Filemon: stop logging\n"));
            FilterOn = FALSE;
            break;

        case IOCTL_FILEMON_STARTFILTER:
          
            //
            // Turn on logging 
            //
            DbgPrint(("Filemon: start logging\n"));
            FilterOn = TRUE;
            break;

        case IOCTL_FILEMON_SETFILTER:
  
            //
            // Gui is updating the filter functions
            //
            DbgPrint(("Filemon: set filter\n"));

            if( InputBufferLength >= sizeof(FILTER) ) {

                FilterDef = *(PFILTER) InputBuffer;
                FilemonUpdateFilters();

            } else {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_FILEMON_UNLOADQUERY:
#if DBG
            //
            // Is it possible to unload?
            //
            KeAcquireSpinLock( &CountMutex, &oldirql );
            IoStatus->Information = OutstandingIRPCount;

            //
            // Any outstanding Irps?
            //
            if( !OutstandingIRPCount ) {

                //
                // Nope, so don't process anymore
                //
                UnloadInProgress = TRUE;

                KeReleaseSpinLock( &CountMutex, oldirql );

                //
                // Stop capturing drives
                //
                HookDriveSet( 0, DeviceObject->DriverObject );
                UnhookSpecialFs( NPFS );
                UnhookSpecialFs( MSFS );

                //
                // Detach from all devices
                //
                UnloadDetach();

            } else {

                KeReleaseSpinLock( &CountMutex, oldirql );
            }
#else // DBG
            IoStatus->Information = 1;
#endif // DBG
            break;

        case IOCTL_FILEMON_ZEROSTATS:

            //
            // Reset all output buffers
            //
            DbgPrint (("Filemon: zero stats\n"));

            ExAcquireFastMutex( &LogMutex );

            while( CurrentLog->Next )  {

                //
                // Free all but the first output buffer
                //
                oldLog = CurrentLog->Next;
                CurrentLog->Next = oldLog->Next;

                ExFreePool( oldLog );
                NumLog--;
            }
 
            //
            // Set the output pointer to the start of the output buffer
            //
            CurrentLog->Len = 0;
            Sequence = 0;

            ExReleaseFastMutex( &LogMutex );
            break;

        case IOCTL_FILEMON_GETSTATS:

            //
            // Copy the oldest output buffer to the caller
            //
            DbgPrint (("Filemon: get stats\n"));

			//
            // If the output buffer is too large to fit into the caller's buffer
            //
            if( LOGBUFSIZE > OutputBufferLength )  {

                IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            //
            // Probe the output buffer
            //
            try {                 

                ProbeForWrite( OutputBuffer,
                               OutputBufferLength,
                               sizeof( UCHAR ));

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                IoStatus->Status = STATUS_INVALID_PARAMETER;
                return FALSE;
            }            

            //
            // We're okay, lock the buffer pool
            //
            ExAcquireFastMutex( &LogMutex );
            if( CurrentLog->Len  ||  CurrentLog->Next ) {

                //
                // Start output to a new output buffer
                //
                FilemonAllocateLog();

                //
                // Fetch the oldest to give to user
                //
                oldLog = FilemonGetOldestLog();

                if( oldLog != CurrentLog ) {

                    logMutexReleased = TRUE;
                    ExReleaseFastMutex( &LogMutex );

                } else {

                    logMutexReleased = FALSE;
                }

                //
                // Copy it to the caller's buffer
                //
                memcpy( OutputBuffer, oldLog->Data, oldLog->Len );

                //
                // Return length of copied info
                //
                IoStatus->Information = oldLog->Len;

                //
                // Deallocate buffer - unless its the last one
                //
                if( logMutexReleased ) {
                    
                    ExFreePool( oldLog );

                } else {

                    CurrentLog->Len = 0;
                    ExReleaseFastMutex( &LogMutex );                    
                }

            } else {

                //
                // There is no unread data
                //
                ExReleaseFastMutex( &LogMutex );
				IoStatus->Information = 0;
            }
            break;
 
        default:

            //
            // Unknown control
            // 
            DbgPrint (("Filemon: unknown IRP_MJ_DEVICE_CONTROL\n"));
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        retval = TRUE;

    } else {

        //
        // Its a call for a file system, so pass it through
        //
        if( FASTIOPRESENT( hookExt, FastIoDeviceControl ) ) {
        
            FilemonGetFullPath( FALSE, FileObject, hookExt, fullPathName );
            TIMESTAMPSTART();

            retval = hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoDeviceControl( 
                FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, 
                OutputBufferLength, IoControlCode, IoStatus, hookExt->FileSystem );

            if(hookExt->Hooked) {

                TIMESTAMPSTOP();
                LogRecord( TRUE, NULL, &dateTime, &timeResult, 
                           "%s\tFASTIO_DEVICE_CONTROL\t%s\tIOCTL: 0x%X\t%s", 
                           FilemonGetProcess( name ), fullPathName,
                           IoControlCode, 
                           retval ?  ErrorString( IoStatus->Status, errorBuf ) : "FAILURE" );
            }
        }
    }

    return retval;
}


//----------------------------------------------------------------------
//
// FilemonFastIoAcquireFile
//
//----------------------------------------------------------------------
VOID 
FilemonFastIoAcquireFile( 
    PFILE_OBJECT FileObject 
    ) 
{
    PDEVICE_OBJECT      deviceObject, checkDevice;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    //
    // We've got to locate our own device object
    //
    checkDevice = FileObject->DeviceObject->Vpb->DeviceObject;
    while( checkDevice ) {

        if( checkDevice->DriverObject == FilemonDriver ) {
    
            //
            // Found it
            //
            deviceObject = checkDevice;
            hookExt = deviceObject->DeviceExtension;
            if( FASTIOPRESENT( hookExt, AcquireFileForNtCreateSection )) {

                GETPATHNAME(FALSE);
                TIMESTAMPSTART();

                hookExt->FileSystem->DriverObject->FastIoDispatch->AcquireFileForNtCreateSection( 
                    FileObject );

                if( FilterDef.logreads && hookExt->Hooked) {

                    TIMESTAMPSTOP();
                    LogRecord( TRUE, NULL, &dateTime, &timeResult, 
                               "%s\tFASTIO_ACQUIRE_FILE\t%s\t\tOK", FilemonGetProcess( name ), 
                               fullPathName );
                }
                FREEPATHNAME();
            }
            return;
        }
        checkDevice = checkDevice->AttachedDevice;
    }
}


//----------------------------------------------------------------------
//
// FilemonFastIoReleaseFile
//
//----------------------------------------------------------------------
VOID 
FilemonFastIoReleaseFile( 
    PFILE_OBJECT FileObject 
    ) 
{
    PDEVICE_OBJECT      deviceObject, checkDevice;
    PHOOK_EXTENSION     hookExt;
    CHAR                *fullPathName, name[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    //
    // We've got to locate our own device object
    //
    checkDevice = FileObject->DeviceObject->Vpb->DeviceObject;
    while( checkDevice ) {

        if( checkDevice->DriverObject == FilemonDriver ) {
    
            deviceObject = IoGetRelatedDeviceObject( FileObject );
            hookExt = deviceObject->DeviceExtension;
            
            if( FASTIOPRESENT( hookExt, ReleaseFileForNtCreateSection )) {

                GETPATHNAME(FALSE);
                TIMESTAMPSTART();

                hookExt->FileSystem->DriverObject->FastIoDispatch->ReleaseFileForNtCreateSection( FileObject );

                if( FilterDef.logreads && hookExt->Hooked) {

                    TIMESTAMPSTOP();
                    LogRecord( TRUE, NULL, &dateTime, &timeResult, 
                               "%s\tFASTIO_RELEASE_FILE\t%s\t\tOK", FilemonGetProcess( name ),
                               fullPathName );
                }
                FREEPATHNAME();
            }
            return;
        }
        checkDevice = checkDevice->AttachedDevice;
    }
}


//----------------------------------------------------------------------
//
// FilemonFastIoDetachDevice
//
// We get this call when a device that we have hooked is being deleted.
// This happens when, for example, a floppy is formatted. We have
// to detach from it and delete our device. We should notify the GUI
// that the hook state has changed, but its not worth the trouble.
//
//----------------------------------------------------------------------
VOID 
FilemonFastIoDetachDevice( 
    PDEVICE_OBJECT SourceDevice, 
    PDEVICE_OBJECT TargetDevice 
    ) 
{
    PHOOK_EXTENSION     hookExt;
    ULONG               i;
    CHAR                name[PROCNAMELEN], drive[PROCNAMELEN];
    LARGE_INTEGER       timeStampStart, timeStampComplete, timeResult;
    LARGE_INTEGER       dateTime;

    //
    // See if a device (like a floppy) is being removed out from under us. If so,
    // we have to detach from it before it disappears  
    //
    for( i = 0; i < 26; i++ ) {

        if( SourceDevice == DriveHookDevices[i] ) {

            //
            // We've hooked it, so we must detach
            //
            hookExt = SourceDevice->DeviceExtension;

            DbgPrint(("Filemon: Detaching from drive: %c\n", 
                      hookExt->LogicalDrive ));

            TIMESTAMPSTART();

            sprintf( drive, "%c:", hookExt->LogicalDrive );
            if( hookExt->Hooked ) {

                TIMESTAMPSTOP();
                LogRecord( TRUE, NULL, &dateTime, &timeResult, 
                           "%s\tFASTIO_DETACH_DEVICE\t%s\t\tOK", 
                           FilemonGetProcess( name ), drive );
            }
            IoDetachDevice( TargetDevice );
            IoDeleteDevice( SourceDevice );

            DriveHookDevices[i] = NULL;
            return;
        }
    }

    //
    // It wasn't for us, so pass it on.
    //
    hookExt = SourceDevice->DeviceExtension;
    if( FASTIOPRESENT( hookExt, FastIoDetachDevice )) {

        hookExt->FileSystem->DriverObject->FastIoDispatch->FastIoDetachDevice( 
            SourceDevice, TargetDevice );
    }
}


//----------------------------------------------------------------------
//     D I S P A T C H   A N D   H O O K   E N T R Y   P O I N T S
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// 
// FilemonHookDoneWork
//
// Worker routine that simply calls update Log. Since we want
// to avoid using spin locks in order to improve SMP performance
// we need to do everything at passive. When our completion routine
// is called at dispatch, we queue the update off to a worker thread.
//
//----------------------------------------------------------------------
VOID 
FilemonHookDoneWork( 
    PVOID Context 
    )
{
    PFILEMON_WORK  filemonWork = (PFILEMON_WORK) Context;

    DbgPrint(("HookWorkRoutine\n"));
    LogRecord( FALSE, &filemonWork->Sequence,
               NULL, 
               &filemonWork->TimeResult,
               filemonWork->ErrString );
    ExFreePool( filemonWork );
}


//----------------------------------------------------------------------
// 
// FilemonHookDone
//
// Gets control after a filesystem operation has completed so that
// we can get return status information about it.
//
//----------------------------------------------------------------------
NTSTATUS 
FilemonHookDone( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PVOID Context 
    )
{
    PIO_STACK_LOCATION   IrpSp;
#if defined(_IA64_)
    ULONG                seqNum = (ULONG) ((ULONG_PTR)Context);
#else
    ULONG                seqNum = (ULONG) Context;
#endif
    CHAR                 errval[ERRORLEN], errString[ERRORLEN];
    KIRQL                oldirql;
    LARGE_INTEGER        timeStampStart, timeStampComplete, timeResult;
    PFILEMON_WORK        filemonWorkContext;

    //
    // A request completed - look at the result 
    //
    IrpSp = IoGetCurrentIrpStackLocation( Irp );    

    //
    // Log the return status in the output buffer. Tag it with the 
    // sequence number so that the GUI can match it with the IRP input information.
    //
    if( FilterOn ) {

        //
        // Quick, get the completion time
        //
        timeStampStart = IrpSp->Parameters.Read.ByteOffset;
        timeStampComplete   = KeQueryPerformanceCounter(NULL);
        timeResult.QuadPart = timeStampComplete.QuadPart - timeStampStart.QuadPart;

        //
        // Queue off to a worker thread if we have to 
        //
        if( KeGetCurrentIrql() == DISPATCH_LEVEL ) {

            filemonWorkContext = ExAllocatePool( NonPagedPool, sizeof(FILEMON_WORK));
            if( filemonWorkContext ) {

                filemonWorkContext->Sequence   = seqNum;
                filemonWorkContext->TimeResult = timeResult;
                sprintf( filemonWorkContext->ErrString, "\t\t\t\t%s", 
                         ErrorString( Irp->IoStatus.Status, errval ));
                ExInitializeWorkItem( &filemonWorkContext->WorkItem, 
                                      FilemonHookDoneWork, filemonWorkContext );
                ExQueueWorkItem( &filemonWorkContext->WorkItem, CriticalWorkQueue );
            } 
        } else {

            sprintf( errString, "\t\t\t\t%s", ErrorString( Irp->IoStatus.Status, errval ));
            LogRecord( FALSE, &seqNum, NULL, &timeResult, errString );
        }
    }

#if DBG
    //
    // We have finished processing an IRP so decrement oustanding IRP count
    //
    KeAcquireSpinLock( &CountMutex, &oldirql );
    OutstandingIRPCount--;
    DbgPrint(("-%d: %x\n", OutstandingIRPCount, Irp ));;
    if( !OutstandingIRPCount ) FilemonDriver->DriverUnload = FilemonUnload;
    KeReleaseSpinLock( &CountMutex, oldirql );
#endif

    //
    // Now we have to mark Irp as pending if necessary
    //
    if( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }
    return Irp->IoStatus.Status;
}


//----------------------------------------------------------------------
//
// FilemonHookRoutine
//
// This routine is the main hook routine where we figure out what
// calls are being sent to the file system.
//
//----------------------------------------------------------------------
NTSTATUS 
FilemonHookRoutine( 
    PDEVICE_OBJECT HookDevice, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpStack    = IoGetNextIrpStackLocation(Irp);
    PMOVE_FILE_DATA     moveFile;
    PQUERY_DIRECTORY    queryDirectory;
    PFILE_OBJECT        FileObject;
    PHOOK_EXTENSION     hookExt;
    LARGE_INTEGER       dateTime;
    LARGE_INTEGER       perfTime;
    PCHAR               fullPathName = NULL;
    BOOLEAN             hookCompletion, createPath;
    CHAR                controlCodeBuffer[ERRORLEN];
    CHAR                attributeString[ERRORLEN];
    CHAR                optionString[ERRORLEN];
    CHAR                name[PROCNAMELEN];
    ULONG               i;
    ANSI_STRING         directoryFilter;
    PCHAR               queryFilter;
    ULONG               seqNum;
    KIRQL               oldirql;

    //
    // Extract the file object from the IRP
    //
    FileObject = currentIrpStack->FileObject;

    //
    // Point at the device extension, which contains information on which
    // file system this IRP is headed for
    //
    hookExt = HookDevice->DeviceExtension;

    //
    // We note open cases so that when we query the file name
    // we don't ask the file system for the name (since it won't
    // have seen the file object yet).
    //
    if( currentIrpStack->MajorFunction == IRP_MJ_CREATE ||
        currentIrpStack->MajorFunction == IRP_MJ_CREATE_NAMED_PIPE ||
        currentIrpStack->MajorFunction == IRP_MJ_CREATE_MAILSLOT ) {

        //
        // Clear any existing fileobject/name association stored in the
        // hash table
        //
        FilemonFreeHashEntry( FileObject );
        createPath = TRUE;

    } else if( currentIrpStack->MajorFunction == IRP_MJ_CLOSE ) {

        //
        // We treat close as a special case of create for name querying
        // since calling into NTFS during a close can result in a deadlock.
        //
        createPath = TRUE;

    } else if( currentIrpStack->MajorFunction == IRP_MJ_CLEANUP &&
               FileObject->Flags & FO_STREAM_FILE ) {

        //
        // Treat cleanup of stream file objects as special create case, because
        // querying them causes NTFS to screwup on NT 4
        //
        createPath = TRUE;

    } else {

        createPath = FALSE;
    }

    //
    // Allocate a buffer and get the name only if we have to
    //
    if( FilterOn && hookExt->Hooked ) {

        GETPATHNAME( createPath );
    } 

    //
    // Only log it if it passes the filter
    //
    if( hookExt->Hooked && fullPathName ) {

        // 
        // If measuring absolute time go and get the timestamp.
        // 
        KeQuerySystemTime( &dateTime );
        perfTime = KeQueryPerformanceCounter( NULL );

        //
        // We want to watch this IRP complete
        //
        seqNum = (ULONG) -1;
        hookCompletion = FALSE;

        //
        // Determine what function we're dealing with
        //
        FilemonGetProcess( name );
        switch( currentIrpStack->MajorFunction ) {

        case IRP_MJ_CREATE:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_CREATE\t%s\tAttributes: %s Options: %s", 
                                       name, fullPathName,
                                       CreateAttributesString( currentIrpStack->Parameters.Create.FileAttributes,
                                                               attributeString ),
                                       CreateOptionsString( currentIrpStack->Parameters.Create.Options,
                                                            optionString ));
            
            //
            // If its an open-by-id we free the hash entry now so that on the next access to 
            // the file we'll pick up the file's real name.
            //
            if( currentIrpStack->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID ) {

                FilemonFreeHashEntry( FileObject );
            }
            break;

        case IRP_MJ_CREATE_NAMED_PIPE:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_CREATE_NAMED_PIPE\t%s\tAttributes: %s Options: %s", 
                                       name, fullPathName,
                                       CreateAttributesString( currentIrpStack->Parameters.Create.FileAttributes,
                                                               attributeString ),
                                       CreateOptionsString( currentIrpStack->Parameters.Create.Options,
                                                            optionString ));
            break;

        case IRP_MJ_CREATE_MAILSLOT:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_CREATE_MAILSLOT\t%s\tAttributes: %s Options: %s", 
                                       name, fullPathName,
                                       CreateAttributesString( currentIrpStack->Parameters.Create.FileAttributes,
                                                               attributeString ),
                                       CreateOptionsString( currentIrpStack->Parameters.Create.Options,
                                                            optionString ));
            break;

        case IRP_MJ_READ:

            if( FilterDef.logreads ) {
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                           "%s\tIRP_MJ_READ%c\t%s\tOffset: %d Length: %d", 
                                           name, 
                                           (Irp->Flags & IRP_PAGING_IO) || 
                                                  (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) ? '*' : ' ',
                                           fullPathName, 
                                           currentIrpStack->Parameters.Read.ByteOffset.LowPart,
                                           currentIrpStack->Parameters.Read.Length );
            }
            break;

        case IRP_MJ_WRITE:

            if( FilterDef.logwrites ) {
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                           "%s\tIRP_MJ_WRITE%c\t%s\tOffset: %d Length: %d", 
                                           name, 
                                           (Irp->Flags & IRP_PAGING_IO) || 
                                                  (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) ? '*' : ' ',
                                           fullPathName, 
                                           currentIrpStack->Parameters.Write.ByteOffset.LowPart,
                                           currentIrpStack->Parameters.Write.Length );
            }
            break;

        case IRP_MJ_CLOSE:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_CLOSE%c\t%s\t", 
                                       name, 
                                       (Irp->Flags & IRP_PAGING_IO) || 
                                              (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) ? '*' : ' ',
                                       fullPathName );

            //
            // This fileobject/name association can be discarded now.
            //
            FilemonFreeHashEntry( FileObject );
            break;

        case IRP_MJ_FLUSH_BUFFERS:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_FLUSH\t%s\t", name, fullPathName );
            break;

        case IRP_MJ_QUERY_INFORMATION:
 
            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_QUERY_INFORMATION\t%s\t%s", 
                                       name, fullPathName, 
                                       FileInformation[currentIrpStack->Parameters.QueryFile.FileInformationClass] );
            break;

        case IRP_MJ_SET_INFORMATION:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_SET_INFORMATION%c\t%s\t%s", 
                                       name, 
                                       (Irp->Flags & IRP_PAGING_IO) || 
                                             (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) ? '*' : ' ',
                                       fullPathName,
                                       FileInformation[currentIrpStack->Parameters.SetFile.FileInformationClass] );

            //
            // If its a rename, cleanup the name association.
            //
            if( currentIrpStack->Parameters.SetFile.FileInformationClass == 
                FileRenameInformation ) {

                FilemonFreeHashEntry( FileObject );
            }
            break;

        case IRP_MJ_QUERY_EA:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_QUERY_EA\t%s\t", name, fullPathName );
            break;

        case IRP_MJ_SET_EA:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_SET_EA\t%s\t", name, fullPathName );
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_QUERY_VOLUME_INFORMATION\t%s\t%s", 
                                       name, fullPathName,
                                       VolumeInformation[currentIrpStack->Parameters.QueryVolume.FsInformationClass] );
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_SET_VOLUME_INFORMATION\t%s\t%s", 
                                       name, fullPathName,
                                       VolumeInformation[currentIrpStack->Parameters.QueryVolume.FsInformationClass] );
            break;

        case IRP_MJ_DIRECTORY_CONTROL:

            switch( currentIrpStack->MinorFunction ) {
            case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                           "%s\tIRP_MJ_DIRECTORY_CONTROL\t%s\tChange Notify", 
                                           name, fullPathName );
                break;
            case IRP_MN_QUERY_DIRECTORY:
                queryDirectory = (PQUERY_DIRECTORY)&currentIrpStack->Parameters;
                queryFilter = NULL;
                if( queryDirectory->FileName ) {

                    if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &directoryFilter,
                                                                  queryDirectory->FileName, TRUE ))) {
                         
                        queryFilter = ExAllocatePool( PagedPool, directoryFilter.Length + 1 );
                        if( queryFilter ) {

                            memcpy( queryFilter, directoryFilter.Buffer, directoryFilter.Length );
                            queryFilter[ directoryFilter.Length ] = 0;

                            //
                            // Massage DOS-internal wildcards
                            //
                            for( i = 0; i < strlen( queryFilter ); i++ ) {
                                if( queryFilter[i] == '<' ) queryFilter[i] = '*';
                                else if( queryFilter[i] == '>' ) queryFilter[i] = '?';
                            }
                        }
                        RtlFreeAnsiString( &directoryFilter );
                    }
                } 
                if( queryFilter ) {

                    hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                               "%s\tIRP_MJ_DIRECTORY_CONTROL\t%s\t%s: %s", 
                                               name, fullPathName, 
                                               FileInformation[queryDirectory->FileInformationClass],
                                               queryFilter );
                    ExFreePool( queryFilter );

                } else {

                    hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                               "%s\tIRP_MJ_DIRECTORY_CONTROL\t%s\t%s", 
                                               name, fullPathName, 
                                               FileInformation[queryDirectory->FileInformationClass] );
                }
                break; 
            default:
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                           "%s\tIRP_MJ_DIRECTORY_CONTROL\t%s\t", 
                                           name, fullPathName );
                break;
            }
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:

            switch( currentIrpStack->Parameters.DeviceIoControl.IoControlCode ) {
            case FSCTL_MOVE_FILE:
                moveFile = (PMOVE_FILE_DATA) Irp->AssociatedIrp.SystemBuffer;
                sprintf( optionString, "Vcn: %d Len: %d Target: %d",
                         moveFile->StartingVcn.LowPart,
                         moveFile->ClusterCount,
                         moveFile->StartingLcn.LowPart );
                ObReferenceObjectByHandle( moveFile->FileHandle, 0, NULL, KernelMode, &FileObject, NULL );
                FilemonGetFullPath( FALSE, FileObject, hookExt, fullPathName );
                ObDereferenceObject( FileObject );
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                            "%s\tFSCTL_MOVE_FILE\t%s\t%s", 
                                            name, fullPathName, optionString );
                break;
            default:
                hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                            "%s\t%s\t%s\t%s", 
                                            name, 
                                            ControlCodeString( currentIrpStack, 
                                                               currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                                                               controlCodeBuffer, optionString ),
                                            fullPathName, optionString );
            }
            break;

        case IRP_MJ_SHUTDOWN:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_SHUTDOWN\t\t", name );
            break;

        case IRP_MJ_LOCK_CONTROL:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_LOCK_CONTROL\t%s\tOffset: %d Length: %d",
                                       name, fullPathName,
                                       ((PLOCK_CONTROL)&currentIrpStack->Parameters)->ByteOffset.LowPart,
                                       ((PLOCK_CONTROL)&currentIrpStack->Parameters)->Length ?
                                       ((PLOCK_CONTROL)&currentIrpStack->Parameters)->Length->LowPart : 0 );
            break;

        case IRP_MJ_CLEANUP:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_CLEANUP\t%s\t", name, fullPathName );
            break;

        case IRP_MJ_DEVICE_CONTROL:
 
            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_DEVICE_CONTROL\t%s\tIOCTL: 0x%X", name, 
                                       fullPathName, currentIrpStack->Parameters.DeviceIoControl.IoControlCode );
            break;

        case IRP_MJ_QUERY_SECURITY:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_QUERY_SECURITY\t%s\t", 
                                       name, fullPathName );
            break;

        case IRP_MJ_SET_SECURITY:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_SET_SECURITY\t%s\t", 
                                       name, fullPathName );
            break;

        case IRP_MJ_POWER:
            
            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_POWER\t%s\tMinor: %x", 
                                       name, fullPathName,
                                       currentIrpStack->MinorFunction );
            break;

        case IRP_MJ_PNP:
            
            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\tIRP_MJ_PNP\t%s\t%s", 
                                       name, fullPathName,
                                       currentIrpStack->MinorFunction <= IRP_MN_QUERY_LEGACY_BUS_INFORMATION ?
                                       PnpMinorCode[currentIrpStack->MinorFunction] : "New minor code" );
            break;

        default:

            hookCompletion = LogRecord( TRUE, &seqNum, &dateTime, NULL, 
                                       "%s\t*UNKNOWN* 0x%X\t\t", name, currentIrpStack->MajorFunction );
            break;
        }

    } else {

        //
        // We don't care about this IRP's completion
        //
        hookCompletion = FALSE;

        //
        // Do name processing for the sake of keeping the hash table current
        //
        switch( currentIrpStack->MajorFunction ) {
        case IRP_MJ_CLOSE:

            //
            // This fileobject/name association can be discarded now.
            //
            FilemonFreeHashEntry( FileObject );
            break;
        }        
    }

    //
    // Free the buffer if we have one
    //
    if( fullPathName && fullPathName != InsufficientResources ) {

        ExFreeToNPagedLookasideList( &FullPathLookaside, fullPathName );
    }

    //
    // Copy parameters down to next level in the stack for the driver below us
    //
    *nextIrpStack = *currentIrpStack;

#if DBG
    //
    // If an unload isn't in progress, we should register a completion callback
    // so that the IRP's return status can be examined.
    //
    KeAcquireSpinLock( &CountMutex, &oldirql );
#endif

    if( !UnloadInProgress && hookCompletion ) {

#if DBG
        //
        // Increment the outstanding IRP count since this IRP will be headed
        // for our completion routine
        //
        FilemonDriver->DriverUnload = NULL;
        OutstandingIRPCount++;
        DbgPrint(("+%d: %x\n", OutstandingIRPCount, Irp ));;
#endif // DBG
        //
        // Grab the time stamp and Log it in the current stack location. This
        // is legal since the stack location is ours, and we're done looking at 
        // the parameters. This makes it easy to pass this to the completion routine. The
        // DiskPerf example in the NT DDK uses this trick.
        //
        currentIrpStack->Parameters.Read.ByteOffset = perfTime;
#if defined(_IA64_)
        IoSetCompletionRoutine( Irp, FilemonHookDone, (PVOID) (ULONG_PTR) seqNum, TRUE, TRUE, TRUE );
#else
        IoSetCompletionRoutine( Irp, FilemonHookDone, (PVOID) seqNum, TRUE, TRUE, TRUE );
#endif

    } else {

        //
        // Set no completion routine
        //
        IoSetCompletionRoutine( Irp, FilemonHookDone, NULL, FALSE, FALSE, FALSE );
    }
#if DBG
    KeReleaseSpinLock( &CountMutex, oldirql );
#endif

    //
    // Return the results of the call to the caller
    //
    return IoCallDriver( hookExt->FileSystem, Irp );
}


//----------------------------------------------------------------------
//
// FilemonDeviceRoutine
//
// In this routine we handle requests to our own device. The only 
// requests we care about handling explicitely are IOCTL commands that
// we will get from the GUI. We also expect to get Create and Close 
// commands when the GUI opens and closes communications with us.
//
//----------------------------------------------------------------------
NTSTATUS 
FilemonDeviceRoutine( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION  irpStack;
    PVOID               inputBuffer;
    PVOID               outputBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    ULONG               ioControlCode;

    //
    // Go ahead and set the request up as successful
    //
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer        = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBuffer       = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode      = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:

        DbgPrint(("Filemon: IRP_MJ_CREATE\n"));

        // 
        // Start the sequence number at 0
        // 
        Sequence = 0;
        break;

    case IRP_MJ_CLOSE:

        DbgPrint(("Filemon: IRP_MJ_CLOSE\n"));

        //
        // A GUI is closing communication
        //
        FilterOn  = FALSE;

        //
        // If the GUI has no more references to us, reset the output
        // buffers and hash table.
        //
        FilemonResetLog();
        FilemonHashCleanup();

        //
        // Stop capturing drives
        //
        HookDriveSet( 0, DeviceObject->DriverObject );
        UnhookSpecialFs( NPFS );
        UnhookSpecialFs( MSFS );
        break;

    case IRP_MJ_DEVICE_CONTROL:

        //
        // This path will never execute because we have registered a 
        // fast I/O path for device control. That means that the fast I/O entry 
        // point will ALWAYS be called for Device Control operations
        //
        DbgPrint (("Filemon: IRP_MJ_DEVICE_CONTROL\n"));

        //
        // Get output buffer if its passed as an MDL
        //
        if( Irp->MdlAddress ) {

            outputBuffer = MmGetSystemAddressForMdl( Irp->MdlAddress );
        }

        //
        // Its a request from the GUI. Simply call our fast handler.
        //
        FilemonFastIoDeviceControl( irpStack->FileObject, TRUE,
                                    inputBuffer, inputBufferLength, 
                                    outputBuffer, outputBufferLength,
                                    ioControlCode, &Irp->IoStatus, DeviceObject );
        break;
    }

    //
    // Complete the IRP
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;   
}


//----------------------------------------------------------------------
//
// FilemonDispatch
//
// Based on which device the Irp is destined for we call either the
// filesystem filter function, or our own device handling routine.
//
//----------------------------------------------------------------------
NTSTATUS 
FilemonDispatch( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    //
    // Determine if its a request from the GUI to us, or one that is
    // directed at a file system driver that we've hooked
    //
    if( ((PHOOK_EXTENSION) DeviceObject->DeviceExtension)->Type == GUIINTERFACE ) {

        return FilemonDeviceRoutine( DeviceObject, Irp );

    } else {

        return FilemonHookRoutine( DeviceObject, Irp );
    }
}


//----------------------------------------------------------------------
//
// FilemonUnload
//
// Our job is done - time to leave. Note that this function is
// only called when debugging is on, since in reality it is not safe
// to detach filter devices or unload a filter driver.
//
//----------------------------------------------------------------------
VOID 
FilemonUnload( 
    IN PDRIVER_OBJECT DriverObject 
    )
{
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\Filemon";
    UNICODE_STRING         deviceLinkUnicodeString;

    //
    // Delete the symbolic link for our GUI device
    //
    RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );

    DbgPrint(("Filemon.SYS: unloading\n"));

    //
    // The only device object left should be the GUI device, since
    // we delete devices in our unload check.
    //
    IoDeleteDevice( DriverObject->DeviceObject );
    DbgPrint(("Filemon.SYS: deleted devices\n"));

    //
    // Now we can free any memory that is allocated
    //
    FilemonFreeFilters();
    FilemonHashCleanup();
    FilemonFreeLog();
    ExDeleteNPagedLookasideList( &FullPathLookaside );

    //
    // Delete the resources
    //
    ExDeleteResourceLite( &FilterResource );
    ExDeleteResourceLite( &HashResource );

    DbgPrint(("Filemon.SYS: freed memory\n"));
}


//----------------------------------------------------------------------
//
// DriverEntry
//
// Installable driver initialization. Here we just set ourselves up.
//
//----------------------------------------------------------------------
NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject, 
    IN PUNICODE_STRING RegistryPath 
    )
{
    NTSTATUS                ntStatus;
    PDEVICE_OBJECT          guiDevice;
    WCHAR                   deviceNameBuffer[]  = L"\\Device\\Filemon";
    UNICODE_STRING          deviceNameUnicodeString;
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\Filemon";
    UNICODE_STRING          deviceLinkUnicodeString;
    ULONG                   i;

    DbgPrint (("Filemon.SYS: entering DriverEntry\n"));
    FilemonDriver = DriverObject;

    //    
    // Setup the device name
    //    
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );

    //
    // Create the device used for GUI communications
    //
    ntStatus = IoCreateDevice ( DriverObject,
                                sizeof(HOOK_EXTENSION),
                                &deviceNameUnicodeString,
                                FILE_DEVICE_FILEMON,
                                0,
                                TRUE,
                                &guiDevice );

    //
    // If successful, make a symbolic link that allows for the device
    // object's access from Win32 programs
    //
    if(NT_SUCCESS(ntStatus)) {

        //
        // Mark this as our GUI device
        //
        ((PHOOK_EXTENSION) guiDevice->DeviceExtension)->Type = GUIINTERFACE;

        //
        // Create a symbolic link that the GUI can specify to gain access
        // to this driver/device
        //
        RtlInitUnicodeString (&deviceLinkUnicodeString,
                              deviceLinkBuffer );
        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString );
        if(!NT_SUCCESS(ntStatus)) {

            DbgPrint (("Filemon.SYS: IoCreateSymbolicLink failed\n"));
            IoDeleteDevice( guiDevice );
            return ntStatus;            
        }

        //
        // Create dispatch points for all routines that must be handled. 
        // All entry points are registered since we might filter a
        // file system that processes all of them.
        //
        for( i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++ ) {

            DriverObject->MajorFunction[i] = FilemonDispatch;
        }
#if DBG        
        //
        // Driver unload is only set if we are debugging Filemon. This is 
        // because unloading a filter is not really safe - threads could
        // be in our fastio routines (or about to enter them), for example, 
        // and there is no way to tell. When debugging, we can risk the 
        // occasional unload crash as a trade-off for not having to 
        // reboot as often.
        //
        // DriverObject->DriverUnload = FilemonUnload;
#endif // DBG

        //
        // Set up the Fast I/O dispatch table
        //
        DriverObject->FastIoDispatch = &FastIOHook;

    } else {

        //
        // If something went wrong, cleanup the device object and don't load
        //
        DbgPrint(("Filemon: Failed to create our device!\n"));
        return ntStatus;
    }

    //
    // Initialize the name hash table
    //
    for(i = 0; i < NUMHASH; i++ ) HashTable[i] = NULL;

    //
    // Find the process name offset
    //
    ProcessNameOffset = FilemonGetProcessNameOffset();

    //
    // Initialize the synchronization objects
    //
#if DBG
    KeInitializeSpinLock( &CountMutex );
#endif
    ExInitializeFastMutex( &LogMutex );
    ExInitializeResourceLite( &FilterResource );
    ExInitializeResourceLite( &HashResource );

    //
    // Initialize a lookaside for file names
    //
    ExInitializeNPagedLookasideList( &FullPathLookaside, NULL, NULL,
				     0, MAXPATHLEN, 'mliF', 256 );

    //
    // Allocate the first output buffer
    //
    CurrentLog = ExAllocatePool( NonPagedPool, sizeof(*CurrentLog) );
    if( !CurrentLog ) {

        // 
        // Oops - we can't do anything without at least one buffer
        // 
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        IoDeleteDevice( guiDevice );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 
    // Set the buffer pointer to the start of the buffer just allocated
    // 
    CurrentLog->Len  = 0;
    CurrentLog->Next = NULL;
    NumLog = 1;

    return STATUS_SUCCESS;
}

