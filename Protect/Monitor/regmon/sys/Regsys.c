//======================================================================
// 
// Regsys.c
//
// Copyright (C) 1996-2002 Mark Russinovich and Bryce Cogswell
// Sysinternals - www.sysinternals.com
//
// This program is protected by copyright. You may not use
// this code or derivatives in commerical or freeware applications
// without a license. Contact licensing@sysinternals.com for inquiries.
//
// On NT 4, Win2K and XP it hooks the registry by replacing registry 
// related calls in the system service table with pointers to our
//  hook routines. On .NET Server it uses the kernel's Registry 
// callback API.
//
//======================================================================
#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "..\Exe\ioctlcmd.h"
#include "ksecdd.h"
#include "ntsec.h"
#include "regsys.h"
#include "reglib.h"
#if defined(_M_IA64)
#include "ia64\ia64stub.h"
#endif


//----------------------------------------------------------------------
//                         FORWARD DEFINES
//---------------------------------------------------------------------- 

BOOLEAN
ApplyFilters( 
    PCHAR fullname 
    );


//----------------------------------------------------------------------
//                         GLOBALS
//---------------------------------------------------------------------- 

//
// Our user-inteface device object
//
PDEVICE_OBJECT          GUIDevice;

//
// Is a GUI talking to us?
//
BOOLEAN                 GUIActive = FALSE;

//
// Are we logging a boot sequence?
//
BOOLEAN                 BootLogging = FALSE;
KEVENT                  LoggingEvent;
HANDLE                  LogFile = INVALID_HANDLE_VALUE;
PLOG_BUF                BootSavedLogList = NULL;
PLOG_BUF                BootSavedLogTail;

//
// Is registry hooked?
//
BOOLEAN                 RegHooked = FALSE;

#ifdef WNET
//
// WinXP Registry hook callback ID
//
LARGE_INTEGER           HookCallbackId;
#endif

//
// Global error string
//
CHAR                    errstring[256];

//
// Global filter (sent to us by the GUI)
//
FILTER                  FilterDef;

//
// Lock to protect filter arrays
//
KMUTEX                  FilterMutex;

//
// Array of process and path filters 
//
ULONG                   NumIncludeFilters = 0;
PCHAR                   IncludeFilters[MAXFILTERS];
ULONG                   NumExcludeFilters = 0;
PCHAR                   ExcludeFilters[MAXFILTERS];

//
// Pointer to system global service table
//
#ifdef WNET
//
// WinXP Registry hook callback ID
//
LARGE_INTEGER           HookCallbackId;

//
// Post context lookaside
//
PAGED_LOOKASIDE_LIST    PostContextLookaside;

//
// Post context list
//
LIST_ENTRY              PostContextList;
KMUTEX                  PostContextMutex;

#else
PVOID                   *KeServiceTablePointers;
SERVICE_HOOK_DESCRIPTOR *HookDescriptors;
#endif

//
// This is the offset into a KPEB of the current process name. This is determined
// dynamically by scanning the process block belonging to the GUI for its name.
//
ULONG                   ProcessNameOffset;

//
// A unicode string constant for the "default" value
//
#define DEFAULTNAMELEN  (9*sizeof(WCHAR))
WCHAR                   DefaultValueString[] = L"(Default)";
UNICODE_STRING          DefaultValue = {
    DEFAULTNAMELEN,
    DEFAULTNAMELEN,
    DefaultValueString
};

//
// Full path name lookaside 
//
PAGED_LOOKASIDE_LIST  FullPathLookaside;

//
// A filter to use if we're monitoring boot activity
//
FILTER  BootFilter = {
    "*", "", 
    TRUE, TRUE, TRUE, TRUE , TRUE
};

//
// We save off pointers to the actual Registry functions in these variables
//
NTSTATUS (*RealRegOpenKey)( IN PHANDLE, IN OUT ACCESS_MASK, IN POBJECT_ATTRIBUTES );
NTSTATUS (*RealRegQueryKey)( IN HANDLE, IN KEY_INFORMATION_CLASS,
                             OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegSetInformationKey)( IN HANDLE, IN KEY_INFORMATION_CLASS,
                                      OUT PVOID, IN ULONG );
NTSTATUS (*RealRegQueryValueKey)( IN HANDLE, IN PUNICODE_STRING, 
                                  IN KEY_VALUE_INFORMATION_CLASS,
                                  OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegEnumerateValueKey)( IN HANDLE, IN ULONG,  
                                      IN KEY_VALUE_INFORMATION_CLASS,
                                      OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegEnumerateKey)( IN HANDLE, IN ULONG,
                                 IN KEY_INFORMATION_CLASS,
                                 OUT PVOID, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegSetValueKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,
                                IN ULONG TitleIndex, IN ULONG Type, 
                                IN PVOID Data, IN ULONG DataSize );
NTSTATUS (*RealRegCreateKey)( OUT PHANDLE, IN ACCESS_MASK,
                              IN POBJECT_ATTRIBUTES , IN ULONG,
                              IN PUNICODE_STRING, IN ULONG, OUT PULONG );
NTSTATUS (*RealRegDeleteValueKey)( IN HANDLE, IN PUNICODE_STRING );
NTSTATUS (*RealRegCloseKey)( IN HANDLE );
NTSTATUS (*RealRegDeleteKey)( IN HANDLE );
NTSTATUS (*RealRegFlushKey)( IN HANDLE );
NTSTATUS (*RealRegLoadKey)( IN POBJECT_ATTRIBUTES, 
                            IN POBJECT_ATTRIBUTES );
NTSTATUS (*RealRegUnloadKey)( IN POBJECT_ATTRIBUTES );


//
// Lenghs of rootkeys (filled in at init). This table allows us to translate 
// path names into better-known forms. Current user is treated specially since
// its not a full match.
//
ROOTKEY CurrentUser[2] = {
    { "\\\\REGISTRY\\USER\\S", "HKCU", 0 },
    { "HKU\\S", "HKCU", 0 }
};

ROOTKEY RootKey[NUMROOTKEYS] = {
    { "\\\\REGISTRY\\USER", "HKU", 0 },
    { "\\\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT", 
      "HKCC", 0 },
    { "\\\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES", "HKCR", 0 },
    { "\\\\REGISTRY\\MACHINE", "HKLM", 0 }
};

//
// This is a hash table for keeping names around for quick lookup.
//
PHASH_ENTRY             HashTable[NUMHASH];

//
// Mutex for hash table accesses
//
KMUTEX                  HashMutex;

//
// Data structure for storing messages we generate
//
PLOG_BUF                Log           = NULL;
ULONG                   Sequence      = 0;
LARGE_INTEGER           StartTime;
KMUTEX                  LogMutex;

//
// Maximum amount of data we will grab for buffered unread data
//
ULONG                   NumLog        = 0;
ULONG                   MaxLog        = MAXMEM/LOGBUFSIZE;


//======================================================================
//      P A T T E R N   M A T C H I N G   R O U T I N E S
//======================================================================


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

//======================================================================
//       B O O T   L O G G I N G   W O R K   R O U T I N E S
//======================================================================

//----------------------------------------------------------------------
//
// RegmonOpenBootLog
//
// Open a log file.
//
//----------------------------------------------------------------------
NTSTATUS 
RegmonOpenBootLog(
    VOID
    )
{
    WCHAR                   logFileNameBuffer[] =  L"\\SystemRoot\\REGMON.LOG";
    UNICODE_STRING          logFileUnicodeString;
    OBJECT_ATTRIBUTES       objectAttributes;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                ntStatus;

    RtlInitUnicodeString( &logFileUnicodeString, logFileNameBuffer );
    InitializeObjectAttributes( &objectAttributes, &logFileUnicodeString,
                                OBJ_CASE_INSENSITIVE, NULL, NULL );

    ntStatus = ZwCreateFile( &LogFile, FILE_WRITE_DATA|SYNCHRONIZE,
                             &objectAttributes, &ioStatus, NULL, 
                             FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                             FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    return ntStatus;
}

//----------------------------------------------------------------------
//
// RegmonCloseBootLog - worker thread routine
//
// Close the boot log file.
//
//----------------------------------------------------------------------
VOID 
RegmonCloseBootLog( 
    PVOID Context 
    )
{
    ZwClose( LogFile );
    KeSetEvent( &LoggingEvent, 0, FALSE );
    LogFile = INVALID_HANDLE_VALUE;
}

//----------------------------------------------------------------------
//
// RegmonWriteBuffer
//
// Dumps a buffer to the log file.
//
//----------------------------------------------------------------------
VOID 
RegmonWriteBuffer( 
    PLOG_BUF LogLog 
    )
{
    ULONG       len;
    ULONG       itemcnt;
    UCHAR       seqtext[64];
    static CHAR diskFullError[] = "Not enough disk space for log file\n"; 
    PCHAR       textptr, items[10];
    PENTRY      entry;
    FILE_END_OF_FILE_INFORMATION zeroLengthFile;
    IO_STATUS_BLOCK ioStatus;

    //
    // Process the buffer
    //
    for( entry = (PENTRY) LogLog->Data; entry < (PENTRY) ((PCHAR) LogLog + LogLog->Len); ) {
        
        len = strlen( entry->text );
        len += 4; len &= 0xFFFFFFFC;
        
        //
        // Write out the entry. 
        //
        sprintf( seqtext, "%d: ", entry->seq );
        ZwWriteFile( LogFile, NULL, NULL, NULL, &ioStatus,
                     seqtext, strlen(seqtext), NULL, NULL );        
        ZwWriteFile( LogFile, NULL, NULL, NULL, &ioStatus,
                     entry->text, strlen(entry->text), NULL, NULL );
        ZwWriteFile( LogFile, NULL, NULL, NULL, &ioStatus,
                     "\r\n", strlen("\r\n"), NULL, NULL );
            
        //
        // If the disk is full, delete the log file
        // and tell the user there wasn't enough room for it.
        //
        if( ioStatus.Status == STATUS_DISK_FULL ) {

            zeroLengthFile.EndOfFile.QuadPart = 0;
            ZwSetInformationFile( LogFile, &ioStatus, 
                                  &zeroLengthFile, sizeof(zeroLengthFile), FileEndOfFileInformation );
            ZwWriteFile( LogFile, NULL, NULL, NULL, &ioStatus,
                         diskFullError, strlen(diskFullError), NULL, NULL );
            ZwClose( LogFile );
            LogFile = INVALID_HANDLE_VALUE;
            BootLogging = FALSE;
            break;
        }
        entry = (PVOID) (entry->text + len);
    }
}


//----------------------------------------------------------------------
//
// RegmonWriteBootLog - worker thread routine
//
// Writes a buffer out to the log file. We do this in a worker routine
// because the log file handle, which we opened in DriverEntry, is
// only valid in the System process, and worker threads execute in 
// the system process. We are protected by the Log mutex while
// in this procedure, since we are called from NewLog, which 
// is called after the Log mutex is acquired.
//
// NOTE: When Regmon is configured to log activity during a boot it
// is marked to start as the very first driver in the boot sequence.
// Because the SystemRoot symbolic link is not initialized until
// all boot drivers have finished initializing, Regmon is not able
// to open a boot log until some point later. In order that we can
// capture all registry activity we Log away output buffers on a list
// until we try and succeed at opening the boot log. When we can we
// send out the accumulated data and then begin dumping buffers
// as they are generated.
//
//----------------------------------------------------------------------
VOID 
RegmonWriteBootLog( 
    PVOID Context
    )
{
    PLOG_BUF    currentLog = Context;
    PLOG_BUF    saveLog, curSaveLog;
    NTSTATUS    ntStatus;

    //
    // If boot logging is still on, but the log file hasn't been opened,
    // try to open it
    //
    if( BootLogging && LogFile == INVALID_HANDLE_VALUE ) {

        ntStatus = RegmonOpenBootLog();
        
        if( NT_SUCCESS( ntStatus )) {

            //
            // Finally! Process all the buffers we've saved away
            //
            curSaveLog = BootSavedLogList;
            while( curSaveLog ) {

                RegmonWriteBuffer( curSaveLog );
                BootSavedLogList = curSaveLog->Next;
                ExFreePool( curSaveLog );
                curSaveLog = BootSavedLogList;
            }
        }
    }
    
    //
    // Either write out the current buffer or save it away to
    // write out later
    //
    if( LogFile != INVALID_HANDLE_VALUE ) {

        RegmonWriteBuffer( currentLog );

    } else {

        //
        // Save this buffer away until we can successfully open
        // the log file and write it out to disk
        //
        saveLog = ExAllocatePool( PagedPool, sizeof(*saveLog));
        if( saveLog ) {

            memcpy( saveLog, currentLog, sizeof(*saveLog) );
            saveLog->Next = NULL;
            if( BootSavedLogList ) {

                BootSavedLogTail->Next = saveLog;
                BootSavedLogTail = saveLog;

            } else {
            
                BootSavedLogList = saveLog;
                BootSavedLogTail = saveLog;
            }
        }
    }

    //
    // Signal the event
    //
    KeSetEvent( &LoggingEvent, 0, FALSE );
}


//======================================================================
//                   B U F F E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// RegmonFreeLog
//
// Frees all the data output buffers that we have currently allocated.
//
//----------------------------------------------------------------------
VOID 
RegmonFreeLog(
    VOID
    )
{
    PLOG_BUF      next;
    
    //
    // Just traverse the list of allocated output buffers
    //
    while( Log ) {
        next = Log->Next;
        ExFreePool( Log );
        Log = next;
    }
}       


//----------------------------------------------------------------------
//
// RegmonNewLog
//
// Called when the current buffer has filled up. This moves us to the
// pre-allocated buffer and then allocates another buffer.
//
//----------------------------------------------------------------------
VOID 
RegmonNewLog( 
    VOID 
    )
{
    PLOG_BUF prev = Log, newLog;
    WORK_QUEUE_ITEM  workItem;

    //
    // If we're boot logging, write the current Log out to disk
    //
    if( BootLogging ) {

        ExInitializeWorkItem( &workItem, RegmonWriteBootLog, Log );
        ExQueueWorkItem( &workItem, CriticalWorkQueue );
        KeWaitForSingleObject( &LoggingEvent, Executive, KernelMode, FALSE, NULL );
    }

    //
    // If we have maxed out or haven't accessed the current Log
    // just return
    //
    if( MaxLog == NumLog ) {

        Log->Len = 0;
        return; 
    }

    //
    // See if we can re-use a Log
    //
    if( !Log->Len ) {

        return;
    }

    //
    // Move to the next buffer and allocate another one
    //
    newLog = ExAllocatePool( PagedPool, sizeof(*Log) );
    if( newLog ) { 

        //
        // Allocation was successful so add the buffer to the list
        // of allocated buffers and increment the buffer count.
        //
        Log   = newLog;
        Log->Len  = 0;
        Log->Next = prev;
        NumLog++;

    } else {

        //
        // The allocation failed - just reuse the current buffer
        //
        Log->Len = 0;
    }
}


//----------------------------------------------------------------------
//
// RegmonOldestLog
//
// Goes through the linked list of storage buffers and returns the 
// oldest one.
//
//----------------------------------------------------------------------
PLOG_BUF 
RegmonOldestLog( 
    VOID 
    )
{
    PLOG_BUF  ptr = Log, prev = NULL;

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
// RegmonResetLog
//
// When a GUI is no longer communicating with us, but we can't unload,
// we reset the storage buffers.
//
//----------------------------------------------------------------------
VOID
RegmonResetLog(
    VOID
    )
{
    PLOG_BUF  current, next;

    MUTEX_ACQUIRE( LogMutex );

    //
    // Traverse the list of output buffers
    //
    current = Log->Next;
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
    Log->Len = 0;
    Log->Next = NULL;

    MUTEX_RELEASE( LogMutex );
}



//----------------------------------------------------------------------
//
// LogRecord
//
// Add a new string to Log, if it fits. 
//
//----------------------------------------------------------------------
VOID 
LogRecord( 
    const char * format, 
    ... 
    )
{ 
    PENTRY          Entry;
    ULONG           len;
    va_list         arg_ptr;
    static CHAR     text[MAXPATHLEN + MAXDATALEN + MAXPROCNAMELEN + MAXERRORLEN];

#define A (&format)
    KdPrint(( (char *)format, A[1], A[2], A[3], A[4], A[5], A[6] ));
    KdPrint(( "\n" ));
#undef A

    //
    // only do this if a GUI is active
    //
    if( !GUIActive ) return;

    //
    // Lock the buffer pool
    //
    MUTEX_ACQUIRE( LogMutex );

    //
    // Sprint the string to get the length
    //
    va_start( arg_ptr, format );
    len = vsprintf( text, format, arg_ptr );
    va_end( arg_ptr );    

    //
    // Only log it if it passes the filters
    //
    if( ApplyFilters( text )) {

        //
        // Get a sequence numnber
        //
        InterlockedIncrement( &Sequence );

        //
        // ULONG align for Alpha
        //
        len += 4; len &=  0xFFFFFFFC; // +1 to include null terminator and +3 to allign on longword

        //
        // See if its time to switch to extra buffer
        //
        if( Log->Len + len + sizeof(*Entry) + 1 >= LOGBUFSIZE ) {

            RegmonNewLog();
        }

        //
        // Log the sequence number so that 
        // a call's result can be paired with its
        // initial data collected when it was made.
        //
        Entry = (void *)(Log->Data+Log->Len);
        Entry->seq = Sequence;
        KeQuerySystemTime( &Entry->time );
        Entry->perftime = KeQueryPerformanceCounter( NULL );
        Entry->perftime.QuadPart -= StartTime.QuadPart;

        memcpy( Entry->text, text, len );

        //
        // Log the length of the string, plus 1 for the terminating
        // NULL  
        //   
        Log->Len += ((ULONG) (Entry->text - (PCHAR) Entry )) + len;
    }

    //
    // Release the buffer pool
    //
    MUTEX_RELEASE( LogMutex );
}


//----------------------------------------------------------------------
// 
// Minimum
//
// Returns min of two numbers
//
//----------------------------------------------------------------------
ULONG
Minimum( 
    ULONG Value1, 
    ULONG Value2
    )
{       
    return Value1 < Value2 ? Value1 : Value2;
}

//======================================================================
//                   H A S H  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// RegmonHashCleanup
//
// Called when we are unloading to free any memory that we have 
// in our possession.
//
//----------------------------------------------------------------------
VOID 
RegmonHashCleanup(
    VOID
    )
{
    PHASH_ENTRY             hashEntry, nextEntry;
    ULONG                   i;

    MUTEX_ACQUIRE( HashMutex );    

    //
    // First free the hash table entries
    //       
    for( i = 0; i < NUMHASH; i++ ) {

        hashEntry = HashTable[i];
        while( hashEntry ) {
            nextEntry = hashEntry->Next;
            ExFreePool( hashEntry->FullPathName );
            ExFreePool( hashEntry );
            hashEntry = nextEntry;
        }
        HashTable[i] = NULL;
    }
    MUTEX_RELEASE( HashMutex );
}

//----------------------------------------------------------------------
//
// RegmonLogHash
//
// Logs the key and associated fullpath in the hash table.
//
//----------------------------------------------------------------------
VOID 
RegmonLogHash( 
    POBJECT object, 
    PCHAR fullname 
    )
{
    PHASH_ENTRY     newEntry;

    newEntry = ExAllocatePool( PagedPool, sizeof(HASH_ENTRY) + strlen(fullname) + 1);

    //
    // Fill in the new entry, but beware of out-of-memory conditions
    //
    if( newEntry ) {

        newEntry->Object                = object;
        MUTEX_ACQUIRE( HashMutex );
        newEntry->Next                  = HashTable[ HASHOBJECT( object) ];
        HashTable[ HASHOBJECT(object) ] = newEntry;     
        strcpy( newEntry->FullPathName, fullname );
        MUTEX_RELEASE( HashMutex );
    }
}


//----------------------------------------------------------------------
//
// RegmonFreeHashEntry
//
// When we see a key close, we can free the string we had associated
// with the fileobject being closed since we know it won't be used
// again.
//
//----------------------------------------------------------------------
VOID 
RegmonFreeHashEntry( 
    POBJECT object 
    )
{
    PHASH_ENTRY             hashEntry, prevEntry;

    MUTEX_ACQUIRE( HashMutex );

    //
    // look-up the entry
    //
    hashEntry = HashTable[ HASHOBJECT( object ) ];
    prevEntry = NULL;

    while( hashEntry && hashEntry->Object != object ) {

        prevEntry = hashEntry;
        hashEntry = hashEntry->Next;
    }
  
    //
    // If we fall off (didn''t find it), just return
    //
    if( !hashEntry ) {

        MUTEX_RELEASE( HashMutex );
        return;
    }

    //
    // Remove it from the hash list 
    //
    if( prevEntry ) {

        prevEntry->Next = hashEntry->Next;
        
    } else {

        HashTable[ HASHOBJECT( object )] = hashEntry->Next;
    }

    //
    // Free the memory associated with it
    //
    ExFreePool( hashEntry );
    MUTEX_RELEASE( HashMutex );
}

//======================================================================
//  R E G I S T R Y  P A R A M E T E R  S U P P O R T  R O U T I N E S
//======================================================================

//----------------------------------------------------------------------
//
// ConverToUpper
//
// Obvious.
//
//----------------------------------------------------------------------
VOID 
ConvertToUpper( 
    PCHAR Dest, 
    PCHAR Source, 
    ULONG Len 
    )
{
    ULONG   i;
    
    for( i = 0; i < Len; i++ ) {

        if( Source[i] >= 'a' && Source[i] <= 'z' ) {

            Dest[i] = Source[i] - 'a' + 'A';

        } else {

            Dest[i] = Source[i];
        }
        if( Source[i] == 0 ) return;
    }
}


//----------------------------------------------------------------------
//
// GetPointer
//
// Translates a handle to an object pointer. In a build for .NET
// server we simply return the passed object pointer so as to 
// avoid having to modify the code further.
//
//----------------------------------------------------------------------
POBJECT 
GetPointer( 
    HANDLE KeyOrHandle 
    )
{
    POBJECT         pKey = NULL;

    //
    // Ignore null handles.
    //
    if( !KeyOrHandle ) return NULL;

    //
    // Get the pointer the handle refers to.
    //
#ifdef WNET
    pKey = KeyOrHandle;
#else
    //
    // Make sure that we're not going to access
    // the kernel handle table from a non-system process
    //
    if( (LONG)(ULONG_PTR) KeyOrHandle < 0 &&
        ExGetPreviousMode() != KernelMode ) {

        return NULL;
    }
    if( !NT_SUCCESS( ObReferenceObjectByHandle( KeyOrHandle, 0, NULL, KernelMode, &pKey, NULL ))) {

        KdPrint(("Error %x getting key pointer\n"));
        pKey = NULL;
    } 
#endif
    return pKey;
}


//----------------------------------------------------------------------
//
// ReleasePointer
//
// Dereferences the object.
//
//----------------------------------------------------------------------
VOID 
ReleasePointer( 
    POBJECT object 
    )
{
#ifndef WNET
    if( object ) ObDereferenceObject( object );
#endif
}

//----------------------------------------------------------------------
//
// AppendKeySetInformation
//
// Appends key set information to the output buffer.
//
//----------------------------------------------------------------------
VOID 
AppendKeySetInformation( 
    IN KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    IN PVOID KeyInformation, 
    PCHAR Buffer,
    ULONG BufferLength
    )
{
    switch( KeySetInformationClass ) {
    case KeyWriteTimeInformation:
        sprintf( Buffer, "WriteTime: %I64x", ((PKEY_WRITE_TIME_INFORMATION) KeyInformation)->LastWriteTime.QuadPart );
        break;

    case KeyUserFlagsInformation:
        sprintf( Buffer, "User Flags: 0x%08X", ((PKEY_USER_FLAGS_INFORMATION) KeyInformation)->UserFlags );
        break;

    default:
        sprintf( Buffer, "Unknown Info Class" );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendKeyInformation
//
// Appends key enumerate and query information to the output buffer.
//
//----------------------------------------------------------------------
VOID 
AppendKeyInformation( 
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation, 
    PCHAR Buffer,
    ULONG BufferLength
    )
{
    PKEY_BASIC_INFORMATION  pBasicInfo;
    PKEY_FULL_INFORMATION   pFullInfo;
    PKEY_NODE_INFORMATION   pNodeInfo;
	PKEY_NAME_INFORMATION	pNameInfo;
    PKEY_CACHED_INFORMATION pCachedInfo;
    PKEY_FLAGS_INFORMATION  pFlagsInfo;
    UNICODE_STRING          ukeyname;
    ANSI_STRING             akeyname;
    ULONG                   maxCopyLength;

    maxCopyLength = BufferLength-strlen("Name: ")-sizeof(CHAR);
    switch( KeyInformationClass ) {

    case KeyBasicInformation:
        pBasicInfo = (PKEY_BASIC_INFORMATION) KeyInformation;
        ukeyname.Length = (USHORT) pBasicInfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pBasicInfo->NameLength;
        ukeyname.Buffer = pBasicInfo->Name;
        if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE ))) {

            if( akeyname.Length > maxCopyLength ) akeyname.Buffer[maxCopyLength] = 0;
            sprintf( Buffer, "Name: %s", akeyname.Buffer );
            RtlFreeAnsiString( &akeyname );
        }
        break;

    case KeyFullInformation:
        pFullInfo = (PKEY_FULL_INFORMATION) KeyInformation;
        sprintf( Buffer, "Subkeys = %d", pFullInfo->SubKeys );
        break;  
        
    case KeyNodeInformation:
        pNodeInfo = (PKEY_NODE_INFORMATION) KeyInformation;
        ukeyname.Length = (USHORT) pNodeInfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pNodeInfo->NameLength;
        ukeyname.Buffer = pNodeInfo->Name;
        if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE ))) {

            if( akeyname.Length > maxCopyLength ) akeyname.Buffer[maxCopyLength] = 0;
            sprintf( Buffer, "Name: %s", akeyname.Buffer );
            RtlFreeAnsiString( &akeyname );
        }
        break;

	case KeyNameInformation:	// New format for Windows 2000
		pNameInfo = (PKEY_NAME_INFORMATION) KeyInformation;
        ukeyname.Length = (USHORT) pNameInfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pNameInfo->NameLength;
        ukeyname.Buffer = pNameInfo->Name;
        if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE ))) {

            if( akeyname.Length > maxCopyLength ) akeyname.Buffer[maxCopyLength] = 0;
            sprintf( Buffer, "Name: %s", akeyname.Buffer );
            RtlFreeAnsiString( &akeyname );
        }
        break;

    case KeyCachedInformation:  // New to Windows XP
        pCachedInfo = (PKEY_CACHED_INFORMATION) KeyInformation;
        sprintf( Buffer, "Subkeys = %d", pCachedInfo->SubKeys );
        break;

    case KeyFlagsInformation:   // New to Windows XP
        pFlagsInfo = (PKEY_FLAGS_INFORMATION) KeyInformation;
        sprintf( Buffer, "User Flags: %0x08x", pFlagsInfo->UserFlags );
        break;

    default:
        sprintf( Buffer, "Unknown Info Class" );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendRegValueType
//
// Returns the string form of an registry value type.
//
//----------------------------------------------------------------------
VOID 
AppendRegValueType( 
    ULONG Type, 
    PCHAR Buffer 
    )
{
    CHAR            tmp[MAXDATALEN];

    switch( Type ) {
    case REG_BINARY:
        strcat( Buffer, "BINARY" );
        break;
    case REG_DWORD_LITTLE_ENDIAN:
        strcat( Buffer, "DWORD_LITTLE_END" );
        break;
    case REG_DWORD_BIG_ENDIAN:
        strcat( Buffer, "DWORD_BIG_END" );
        break;
    case REG_EXPAND_SZ:
        strcat( Buffer, "EXPAND_SZ" );
        break;
    case REG_LINK:
        strcat( Buffer, "LINK" );
        break;
    case REG_MULTI_SZ:
        strcat( Buffer, "MULTI_SZ" );
        break;
    case REG_NONE:
        strcat( Buffer, "NONE" );
        break;
    case REG_SZ:
        strcat( Buffer, "SZ" );
        break;
    case REG_RESOURCE_LIST:
        strcat( Buffer, "RESOURCE_LIST" );
        break;
    case REG_RESOURCE_REQUIREMENTS_LIST:
        strcat( Buffer, "REQ_LIST" );
        break;
    case REG_FULL_RESOURCE_DESCRIPTOR:
        strcat( Buffer, "DESCRIPTOR" );
        break;
#ifdef WNET
    case REG_QWORD:
        strcat( Buffer, "QWORD" );
        break;
#endif
    default:
        sprintf( tmp, "UNKNOWN TYPE: %d", Type );
        strcat( Buffer, tmp );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendRegValueData
//
// We expand certain registry types to provide more information. In
// all cases, calculate the length of the data being copied so 
// we don't overflow the buffer that's passed in. The length of Buffer 
// must be MAXVALLEN.
//
//----------------------------------------------------------------------
VOID 
AppendRegValueData( 
    IN ULONG Type, 
    IN PVOID Data, 
    IN ULONG Length, 
    IN OUT PCHAR Buffer 
    )
{
    PWCHAR                  pstring;
    PULONG                  pulong;
    PUCHAR                  pbinary;
    CHAR                    tmp[MAXDATALEN];
    UNICODE_STRING          ukeyname;     
#ifdef WNET
    PULONGLONG              pqword;
#endif
    ANSI_STRING             akeyname;
    int                     len, i;

    switch( Type ) {
    case REG_SZ:    
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        pstring = (PWCHAR) Data;
        ukeyname.Length = (USHORT) Length;
        ukeyname.MaximumLength = (USHORT) Length;
        ukeyname.Buffer = pstring;
        if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, 
                                                      &ukeyname, TRUE ))) {
            strcat( Buffer, "\"");
            strncat( Buffer+1, akeyname.Buffer, Minimum( akeyname.Length, MAXVALLEN - 6));
            if( akeyname.Length > MAXVALLEN - 6 ) strcat( Buffer,"...");
            strcat( Buffer, "\"");
            RtlFreeAnsiString( &akeyname );
        }
        break;

    case REG_DWORD:
        pulong = (PULONG) Data;
        sprintf( tmp, "0x%X", *pulong );
        strcat( Buffer, tmp );
        break;
#ifdef WNET
    case REG_QWORD:
        pqword = (PULONGLONG) Data;
        sprintf( tmp, "0x%I64x", *pqword );
        strcat( Buffer, tmp );
        break;
#endif
    case REG_BINARY:
    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
        pbinary = (PCHAR) Data;
        if( Length > 8 ) len = 8;
        else len = Length;
        for( i = 0; i < len; i++ ) {
            sprintf( tmp, "%02X ", (UCHAR) pbinary[i]);
            strcat( Buffer, tmp );
        }
        if( Length > 8) strcat( Buffer, "...");
        break;

    default:
        AppendRegValueType( Type, Buffer );
        break;
    }
}


//----------------------------------------------------------------------
//
// AppendValueInformation
//
// Appends value enumerate and query information to the output buffer.
//
//----------------------------------------------------------------------
VOID 
AppendValueInformation( 
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation, 
    PCHAR Buffer, 
    PCHAR ValueName 
    )
{
    PKEY_VALUE_BASIC_INFORMATION    pBasicInfo;
    PKEY_VALUE_FULL_INFORMATION     pFullInfo;
    PKEY_VALUE_PARTIAL_INFORMATION  ppartinfo;
#ifdef WNET
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 ppartInfo64;
#endif
    UNICODE_STRING                  ukeyname;       
    ANSI_STRING                     akeyname;

    switch( KeyValueInformationClass ) {

    case KeyValueBasicInformation:
        pBasicInfo = (PKEY_VALUE_BASIC_INFORMATION)
            KeyValueInformation;
        sprintf( Buffer, "Type: ");
        AppendRegValueType( pBasicInfo->Type, Buffer );
        strncat( Buffer, " Name: ", MAXVALLEN - 1 - strlen(Buffer) );
        ukeyname.Length = (USHORT) pBasicInfo->NameLength;
        ukeyname.MaximumLength = (USHORT) pBasicInfo->NameLength;
        ukeyname.Buffer = pBasicInfo->Name;
        if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE ))) {

            strncat( Buffer, akeyname.Buffer, Minimum( akeyname.Length, MAXVALLEN - 1 - strlen(Buffer) ));
            if( ValueName ) strncpy( ValueName, akeyname.Buffer, MAXVALLEN - 1 );
            RtlFreeAnsiString( &akeyname );                 
        }
        break;

#ifdef WNET
    case KeyValueFullInformationAlign64:
#endif
    case KeyValueFullInformation:   
        pFullInfo = (PKEY_VALUE_FULL_INFORMATION) 
            KeyValueInformation;
        AppendRegValueData( pFullInfo->Type, 
                            (PVOID) ((PCHAR) pFullInfo + pFullInfo->DataOffset), 
                            pFullInfo->DataLength, Buffer );
        if( ValueName ) {
            ukeyname.Length = (USHORT) pFullInfo->NameLength;
            ukeyname.MaximumLength = (USHORT) pFullInfo->NameLength;
            ukeyname.Buffer = pFullInfo->Name;
            if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &akeyname, &ukeyname, TRUE ))) {

                strncpy( ValueName, akeyname.Buffer, MAXVALLEN - 1 );
                RtlFreeAnsiString( &akeyname ); 
            }
        }
        break;

#ifdef WNET
    case KeyValuePartialInformationAlign64:
        ppartInfo64 = (PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64) 
            KeyValueInformation;
        AppendRegValueData( ppartInfo64->Type, 
                            (PVOID) ppartInfo64->Data, 
                            ppartInfo64->DataLength, Buffer );
        break;
#endif

    case KeyValuePartialInformation:
        ppartinfo = (PKEY_VALUE_PARTIAL_INFORMATION)
            KeyValueInformation;
        AppendRegValueData( ppartinfo->Type, 
                            (PVOID) ppartinfo->Data, 
                            ppartinfo->DataLength, Buffer );
        break;

    default:
        sprintf( Buffer, "Unknown Info Class" );
        break;
    }
}

//----------------------------------------------------------------------
//
// ErrorString
//
// Returns the string form of an error code.
//
//----------------------------------------------------------------------
PCHAR 
ErrorString( 
    NTSTATUS retval 
    )
{
    //
    // Before transating, apply error filter
    //
    if( retval == STATUS_SUCCESS && !FilterDef.logsuccess ) return NULL;
    if( retval != STATUS_SUCCESS && !FilterDef.logerror   ) return NULL;

    //
    // Passed filter, so log it
    //
    switch( retval ) {
    case STATUS_BUFFER_TOO_SMALL:
        return "BUFTOOSMALL";
    case STATUS_SUCCESS:
        return "SUCCESS";
    case STATUS_KEY_DELETED:
        return "KEYDELETED";
    case STATUS_REGISTRY_IO_FAILED:
        return "IOFAILED";
    case STATUS_REGISTRY_CORRUPT:
        return "CORRUPT";
    case STATUS_NO_MEMORY:
        return "OUTOFMEM";
    case STATUS_ACCESS_DENIED:
        return "ACCDENIED";
    case STATUS_NO_MORE_ENTRIES:
        return "NOMORE";
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return "NOTFOUND";
    case STATUS_BUFFER_OVERFLOW:
        return "BUFOVRFLOW";
    case STATUS_OBJECT_PATH_SYNTAX_BAD:
        return "SYNTAXERR";
    case STATUS_OBJECT_NAME_COLLISION:
        return "NAMECOLLISION";
    case STATUS_REPARSE:
        return "REPARSE";
    case STATUS_BAD_IMPERSONATION_LEVEL:
        return "BADIMPERSONATION";
    default:
        sprintf(errstring, "%x", retval );
        return errstring;
    }
}


//----------------------------------------------------------------------
//
// RegmonFreeFilters
//
// Fress storage we allocated for filter strings.
//
//----------------------------------------------------------------------
VOID 
RegmonFreeFilters(
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
// RegmonUpdateFilters
//
// Takes a new filter specification and updates the filter
// arrays with them.
//
//----------------------------------------------------------------------
VOID 
RegmonUpdateFilters(
    VOID
    )
{
    //
    // Free old filters (if any)
    //
    MUTEX_ACQUIRE( FilterMutex );

    RegmonFreeFilters();

    //
    // Create new filter arrays
    //
    MakeFilterArray( FilterDef.includefilter,
                     IncludeFilters, &NumIncludeFilters );
    MakeFilterArray( FilterDef.excludefilter,
                     ExcludeFilters, &NumExcludeFilters );
    MUTEX_RELEASE( FilterMutex );
}


//----------------------------------------------------------------------
//
// ApplyNameFilter
//
// If the name matches the exclusion mask, we do not log it. Else if
// it doesn't match the inclusion mask we do not log it. 
//
//----------------------------------------------------------------------
BOOLEAN
ApplyFilters( 
    PCHAR fullname 
    )
{
    ULONG    i;

    //   
    // If it matches the exclusion string, do not log it
    //
    MUTEX_ACQUIRE( FilterMutex );

    for( i = 0; i < NumExcludeFilters; i++ ) {

        if( MatchWithPattern( ExcludeFilters[i], fullname ) ) {

            MUTEX_RELEASE( FilterMutex );
            return FALSE;
        }
    }
 
    //
    // If it matches an include filter then log it
    //
    for( i = 0; i < NumIncludeFilters; i++ ) {

        if( MatchWithPattern( IncludeFilters[i], fullname )) {

            MUTEX_RELEASE( FilterMutex );
            return TRUE;
        }
    }

    //
    // It didn't match any include filters so don't log it
    //
    MUTEX_RELEASE( FilterMutex );
    return FALSE;
}

//----------------------------------------------------------------------
//
// GetSecurityInfoWorkRoutine
//
// Gets security information in the context of the System process.
//
//----------------------------------------------------------------------
VOID 
GetSecurityInfoWorkRoutine(
    PVOID Context
    )
{
    PGETSECINFO_WORK_ITEM workItem = (PGETSECINFO_WORK_ITEM) Context;
    PSecurityUserData     userInformation = NULL;
    ANSI_STRING           ansiName;
    PCHAR                 bufPtr;

    NTSTATUS status;

    workItem->Status = GetSecurityUserInfo( &workItem->LogonId,
                                             UNDERSTANDS_LONG_NAMES,
                                             &userInformation );
    if( NT_SUCCESS( workItem->Status )) {

        //
        // Translate the name to ansi
        //
        bufPtr = workItem->Output;
        status = RtlUnicodeStringToAnsiString( &ansiName, &userInformation->LogonDomainName, TRUE );
        strncpy( bufPtr, ansiName.Buffer, ansiName.Length );

        bufPtr[ansiName.Length] = '\\';
        bufPtr = workItem->Output + ansiName.Length + 1;
        RtlFreeAnsiString( &ansiName );

        RtlUnicodeStringToAnsiString( &ansiName, &userInformation->UserName, TRUE );
        strncpy( bufPtr, ansiName.Buffer, ansiName.Length );
        bufPtr[ansiName.Length] = 0;
        RtlFreeAnsiString( &ansiName );

        
        //
        // Free the buffer
        //
        LsaFreeReturnBuffer( userInformation );

    } else {

        KdPrint(("Error getting user info: %x\n", workItem->Status ));
    }
    KeSetEvent( &workItem->Event, 0, FALSE );
}


//----------------------------------------------------------------------
//
// RegmonOpenToken
//
// Opens a handle to the current thread's effective token.
//
//----------------------------------------------------------------------
HANDLE 
RegmonOpenToken(
    VOID
    )
{
    NTSTATUS     status;
    PVOID        token;
    HANDLE       hToken;
    BOOLEAN      copyOnOpen, effectiveOnly;
    SECURITY_IMPERSONATION_LEVEL impersonationLevel;
                                      
    token = PsReferenceImpersonationToken( PsGetCurrentThread(),
                                           &copyOnOpen,
                                           &effectiveOnly,
                                           &impersonationLevel );

    if( !token ) {
        
        token = PsReferencePrimaryToken( PsGetCurrentProcess());
    }

    //
    // Now that we have a token reference, get a handle to it
    // so that we can query it.
    //
    status = ObOpenObjectByPointer( token, 
                                    0, 
                                    NULL,
                                    TOKEN_QUERY, 
                                    NULL, 
                                    KernelMode, 
                                    &hToken
                                    );

    ObDereferenceObject( token );
    if( !NT_SUCCESS( status )) {

        //
        // We coudn't open the token!!
        //
        return NULL;
    }    
    return hToken;
}


//----------------------------------------------------------------------
//
// GetUserName
//
// Retreives the user domain and name.
//
//----------------------------------------------------------------------
VOID
GetUserName(
    PCHAR Output
    )
{
    NTSTATUS              status;
    ULONG                 requiredLength;
    TOKEN_STATISTICS      tokenStats;
    HANDLE                hToken;
    GETSECINFO_WORK_ITEM  workItem;
    LUID                  logonId;

    //
    // Open the effective token
    //
    Output[0] = 0;
    hToken = RegmonOpenToken();
    if( !hToken ) {
        
        KdPrint(("Error opening token\n"));
        return;
    }

    //
    // Get the logon ID from the token so that we can
    // ask KSECDD about it.
    // 
    status = ZwQueryInformationToken( hToken,
                                      TokenStatistics,
                                      &tokenStats,
                                      sizeof( tokenStats ),
                                      &requiredLength 
                                      );
    if( NT_SUCCESS( status )) {

            //
            // Now get the user and domain names
            //
            logonId = tokenStats.AuthenticationId;
            if( logonId.LowPart == SYSTEMACCOUNT_LOW && 
                logonId.HighPart == SYSTEMACCOUNT_HIGH ) {

                strcpy( Output, "NT_AUTHORITY\\SYSTEM" );

            } else {

                //
                // We queue a work item to get the user name. After the
                // work item is complete the user name is in the output buffer
                //
                ExInitializeWorkItem( &workItem.Item, GetSecurityInfoWorkRoutine, &workItem );
                KeInitializeEvent( &workItem.Event, SynchronizationEvent, FALSE );
                workItem.Output = Output;
                workItem.LogonId = logonId;

                ExQueueWorkItem( (PWORK_QUEUE_ITEM) &workItem, CriticalWorkQueue );

                KeWaitForSingleObject( &workItem.Event, Executive,
                                       KernelMode, FALSE, NULL );
            }
    } else {

        KdPrint(("Error querying token: %x\n", status ));
    }
    ZwClose( hToken );
}
    
 
//----------------------------------------------------------------------
//
// GetFullName
//
// Returns the full pathname of a key, if we can obtain one, else
// returns a handle.
//
//----------------------------------------------------------------------
VOID 
GetFullName( 
    HANDLE hKey, 
    PUNICODE_STRING lpszSubKeyVal, 
    PCHAR fullname 
    )
{
    PHASH_ENTRY             hashEntry;
    POBJECT                 pKey = NULL;
    CHAR                    tmpkey[16];
    ANSI_STRING             keyname;
    PCHAR                   tmpname;
    PCHAR                   cmpname;
    PCHAR                   nameptr;
    PUNICODE_STRING         fullUniName;
    ULONG                   actualLen;
    int                     i;

    //
    // If the fullname buffer is NULL, bail now
    //
    if( !fullname ) return;

    //
    // Allocate a temporary buffer
    //
    cmpname = ExAllocatePool( PagedPool, MAXROOTLEN );
    tmpname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    if( !tmpname || !cmpname ) {

        //
        // Not enough memory for a buffer
        //
        if( cmpname ) ExFreePool( cmpname );
        if( tmpname ) ExFreeToPagedLookasideList( &FullPathLookaside, tmpname );
        strcpy( fullname, "<INSUFFICIENT MEMORY>");
        return;
    }

    //
    // Translate the hkey into a pointer
    //
    fullname[0] = 0;
    tmpname[0] = 0;

    //
    // Is it a valid handle?
    //
    if( pKey = GetPointer( hKey )) {

        //
        // See if we find the key in the hash table
        //
        ReleasePointer( pKey );
        MUTEX_ACQUIRE( HashMutex );
        hashEntry = HashTable[ HASHOBJECT( pKey ) ];
        while( hashEntry && hashEntry->Object != pKey ) {

            hashEntry = hashEntry->Next;
        }
        if( hashEntry ) {

            strcpy( tmpname, hashEntry->FullPathName );
            MUTEX_RELEASE( HashMutex );

        } else {

            //
            // We will only get here if key was created before we loaded - ask the Configuration
            // Manager what the name of the key is.
            //
            MUTEX_RELEASE( HashMutex );
            if( pKey ) {

                fullUniName = ExAllocatePool( PagedPool, MAXPATHLEN*sizeof(WCHAR)+2*sizeof(ULONG));
                if( !fullUniName ) {

                    //
                    // Out of memory
                    //
                    strcpy( fullname, "<INSUFFICIENT MEMORY>" );
                    ExFreePool( cmpname );
                    ExFreeToPagedLookasideList( &FullPathLookaside, tmpname );
                    return;
                }

                fullUniName->MaximumLength = MAXPATHLEN*sizeof(WCHAR);
                if( NT_SUCCESS(ObQueryNameString( pKey, fullUniName, MAXPATHLEN, &actualLen ) )) {

                    if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &keyname, fullUniName, TRUE ))) { 

                        if( keyname.Buffer[0] ) {

                            strcpy( tmpname, "\\" );
                            strncat( tmpname, keyname.Buffer, Minimum( keyname.Length, MAXPATHLEN -2 ));
                        }
                        RtlFreeAnsiString( &keyname );
                    }
                }
                ExFreePool( fullUniName );
            }
        }
    }

    //
    // Append subkey and value, if they are there
    //
    try {

        if( lpszSubKeyVal ) {
            keyname.Buffer = NULL;
            if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &keyname, lpszSubKeyVal, TRUE ))) {
                
                if( keyname.Buffer[0] ) {

                    //
                    // See if this is an absolute rather than relative path, which 
                    // can be the case on Open/Create when the Registry callback API
                    // is used (.NET Server and higher)
                    //
                    ConvertToUpper( cmpname, keyname.Buffer, strlen("\\REGISTRY")+1);
                    if( !strncmp( cmpname, "\\REGISTRY", strlen("\\REGISTRY"))) {

                        strcpy( tmpname, "\\" );

                    } else {

                        strcat( tmpname, "\\" );
                    }
                    strncat( tmpname, keyname.Buffer, Minimum( keyname.Length, MAXPATHLEN - 1 - strlen(tmpname) ));
                }
                RtlFreeAnsiString( &keyname );
            }
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {

        if( keyname.Buffer ) RtlFreeAnsiString( &keyname );
        strcat( tmpname, "*** Invalid Name ****" );
    }

    //
    // See if it matches current user
    //
    for( i = 0; i < 2; i++ ) {

        ConvertToUpper( cmpname, tmpname, CurrentUser[i].RootNameLen );
        if( !strncmp( cmpname, CurrentUser[i].RootName,
                      CurrentUser[i].RootNameLen )) {

            KdPrint(( " CurrentUser(%d) %s ==> %s\n", i, 
                       tmpname, CurrentUser[i].RootName ));

            //
            // Its current user. Process to next slash
            //
            nameptr = tmpname + CurrentUser[i].RootNameLen;
            while( *nameptr && *nameptr != '\\' ) nameptr++;
            strcpy( fullname, CurrentUser[i].RootShort );
#if 0
            cmpname = nameptr - sizeof(USER_CLASSES);
            ConvertToUpper (cmpname, cmpname, sizeof(USER_CLASSES));
            if (!strncmp( cmpname, USER_CLASSES, sizeof(USER_CLASSES))) {

                strcat (fullname, "\\Software\\Classes");
            }
#endif
            strcat( fullname, nameptr );
            ExFreePool( cmpname );
            ExFreeToPagedLookasideList( &FullPathLookaside, tmpname );
            return;
        }
    }     

    //
    // Now, see if we can translate a root key name
    //
    for( i = 0; i < NUMROOTKEYS; i++ ) {
        ConvertToUpper( cmpname, tmpname, RootKey[i].RootNameLen );
        if( !strncmp( cmpname, RootKey[i].RootName, 
                      RootKey[i].RootNameLen )) {
            nameptr = tmpname + RootKey[i].RootNameLen;
            strcpy( fullname, RootKey[i].RootShort );
            strcat( fullname, nameptr );
            ExFreePool( cmpname );
            ExFreeToPagedLookasideList( &FullPathLookaside, tmpname );
            return;
        }
    }

    //
    // No translation
    //
    strcpy( fullname, tmpname );
    ExFreeToPagedLookasideList( &FullPathLookaside, tmpname );
    ExFreePool( cmpname );
}


//----------------------------------------------------------------------
//
// GetProcessNameOffset
//
// In an effort to remain version-independent, rather than using a
// hard-coded into the KPEB (Kernel Process Environment Block), we
// scan the KPEB looking for the name, which should match that
// of the GUI process
//
//----------------------------------------------------------------------
ULONG 
GetProcessNameOffset(
    VOID
    )
{
    PEPROCESS       curproc;
    int             i;

    curproc = PsGetCurrentProcess();

    //
    // Scan for 12KB, hopping the KPEB never grows that big!
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
// GetProcess
//
// Uses undocumented data structure offsets to obtain the name of the
// currently executing process.
//
//----------------------------------------------------------------------
PCHAR
GetProcess( 
    PCHAR Name 
    )
{
    PEPROCESS       curproc;
    char            *nameptr;
    ULONG           i;

    //
    // We only try and get the name if we located the name offset
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
        strncpy( Name, nameptr, NT_PROCNAMELEN-1 );
        Name[NT_PROCNAMELEN-1] = 0;
#if defined(_M_IA64)
        sprintf( Name + strlen(Name), ":%I64d", PsGetCurrentProcessId());
#else
        sprintf( Name + strlen(Name), ":%d", (ULONG) PsGetCurrentProcessId());
#endif

    } else {
     
        strcpy( Name, "???");
    }
    return Name;
}

//======================================================================
//                    H O O K  R O U T I N E S
//======================================================================

//----------------------------------------------------------------------
//
// HookRegOpenKey
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegOpenKey( 
    IN OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess, 
    IN POBJECT_ATTRIBUTES pOpenInfo 
    )
{
    NTSTATUS                ntstatus;
    POBJECT                 regobj;
    CHAR                    data[MAXDATALEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( pOpenInfo->RootDirectory, pOpenInfo->ObjectName, fullname );
    ntstatus = RealRegOpenKey( pHandle, ReqAccess, pOpenInfo );
    if( NT_SUCCESS( ntstatus )) { 
  
        regobj = GetPointer( *pHandle );
        RegmonFreeHashEntry( regobj );
        ReleasePointer( regobj );
    }
    if( fullname ) {
        
        KdPrint(("RegOpenKey: %s => %x, %x\n", fullname, *pHandle, ntstatus ));
        data[0] = 0;
        if( NT_SUCCESS( ntstatus )) {   
            RegmonLogHash( regobj, fullname );
#if defined(_M_IA64)
            sprintf(data,"Key: 0x%I64X", regobj );
#else
            sprintf(data,"Key: 0x%X", regobj );
#endif
        } else if( ntstatus == STATUS_ACCESS_DENIED ) {

            sprintf( data, "Access: 0x%X ", ReqAccess );
            GetUserName( &data[strlen(data)] );
        }
        if( FilterDef.logaux && ErrorString( ntstatus )) {
            LogRecord( "%s\tOpenKey\t%s\t%s\t%s", GetProcess( name ),
                       fullname, ErrorString( ntstatus ), data );
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegCreateKey
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegCreateKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess,
    IN POBJECT_ATTRIBUTES pOpenInfo, 
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class, 
    IN ULONG CreateOptions, 
    OUT PULONG Disposition 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    POBJECT                 regobj;
    CHAR                    data[MAXDATALEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( pOpenInfo->RootDirectory, pOpenInfo->ObjectName, fullname );
#ifndef WNET
	ntstatus = RealRegCreateKey( pHandle, ReqAccess, pOpenInfo, TitleIndex,
                                 Class, CreateOptions, Disposition );
#endif
    if( NT_SUCCESS( ntstatus )) {   
        regobj = GetPointer( *pHandle );
        RegmonFreeHashEntry( regobj );
        ReleasePointer( regobj );
    }
    if( fullname ) {

        KdPrint(("RegCreateKey: %s => %x, %x\n", fullname, *pHandle, ntstatus ));
        data[0] = 0;
        if( NT_SUCCESS( ntstatus )) {   

            RegmonLogHash( regobj, fullname );
#if defined(_M_IA64)
            sprintf(data,"Key: 0x%I64X", regobj );
#else
            sprintf(data,"Key: 0x%X", regobj );
#endif
        } else if( ntstatus == STATUS_ACCESS_DENIED ) {

            sprintf( data, "Access: 0x%X ", ReqAccess );
            GetUserName( &data[strlen(data)] );
            GetUserName( data );
        }
        if( ((NT_SUCCESS( ntstatus ) && 
              (Disposition && *Disposition == REG_CREATED_NEW_KEY && FilterDef.logwrites)) ||
             FilterDef.logreads ) && ErrorString( ntstatus )) {

            LogRecord( "%s\tCreateKey\t%s\t%s\t%s", GetProcess( name ),
                       fullname, ErrorString( ntstatus ), data);
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegCloseKey
//
// This is actually a hook for NtClose which is used for closing any
// object. Therefore, we must ensure that we are seeing a close for 
// a registry object that we know about.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegCloseKey( 
#ifdef WNET
    IN PCHAR fullname,
#endif
    IN HANDLE KeyOrHandle 

    )
{
    NTSTATUS                ntstatus;
    POBJECT                 regobj;
    CHAR                    name[MAXPROCNAMELEN];
#ifndef WNET
    PCHAR                   fullname;
#endif
    PCHAR                   data;
    ULONG                   retlen;
    BOOLEAN                 iskey = FALSE;
    KEY_BASIC_INFORMATION   basicInfo;
    
    data     = ExAllocatePool( PagedPool, MAXVALLEN );
#ifdef WNET
    ntstatus = STATUS_SUCCESS;
    if( fullname && data ) {

        regobj = GetPointer( KeyOrHandle );
        iskey = TRUE;
    }
#else
    //
    // Determine if the object is a key by querying it
    //
    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    ntstatus = RealRegQueryKey( KeyOrHandle, KeyBasicInformation, 
                                &basicInfo, 0, &retlen );
    if( fullname && data && ntstatus != STATUS_OBJECT_TYPE_MISMATCH ) {

        iskey = TRUE;        
        GetFullName( KeyOrHandle, NULL, fullname );

        //
        // get the pointer for later. We have to keep this around
        // until after we remove the entry from our hash table, to prevent
        // races where Windows reuses the memory for another key after
        // its closed.
        //
        regobj = GetPointer( KeyOrHandle );
    }

    ntstatus = RealRegCloseKey( KeyOrHandle );
#endif
    if( iskey ) {

        KdPrint(("RegCloseKey: %s => %x, %x\n", fullname, KeyOrHandle, ntstatus ));
        if( NT_SUCCESS( ntstatus )) {

            RegmonFreeHashEntry( regobj );
            if( FilterDef.logaux && ErrorString( ntstatus )) {

#if defined(_M_IA64)
                sprintf(data,"Key: 0x%I64X", regobj );
#else
                sprintf(data,"Key: 0x%X", regobj );
#endif
                LogRecord( "%s\tCloseKey\t%s\t%s\t%s", 
                           GetProcess( name ), fullname, ErrorString( ntstatus ), data );
            }
        }
        ReleasePointer( regobj );
    }
    if( fullname ) ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    if( data )     ExFreePool( data );
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegFlushKey
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegFlushKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE Handle 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    name[MAXPROCNAMELEN];
    PCHAR                   fullname;
    POBJECT                 regobj;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( Handle, NULL, fullname );
#ifndef WNET
    ntstatus = RealRegFlushKey( Handle );
#endif
    if( fullname ) {

        KdPrint(("RegFlushKey: %s => 0x%X\n", fullname, ntstatus ));
        regobj = GetPointer( Handle );
        ReleasePointer( regobj );
        if( FilterDef.logaux && ErrorString( ntstatus )) {
            LogRecord( "%s\tFlushKey\t%s\t%s\tKey: 0x%X", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), regobj);
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegDeleteKey
//
// Once we've deleted a key, we can remove its reference in the hash 
// table.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegDeleteKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
    IN PCHAR fullname,
#endif
    IN HANDLE KeyOrHandle 
    )
{
    POBJECT                 regobj;
    CHAR                    name[MAXPROCNAMELEN];
#ifndef WNET
    NTSTATUS                ntstatus;
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( KeyOrHandle, NULL, fullname );
    regobj = GetPointer( KeyOrHandle );
    ReleasePointer( regobj );
    ntstatus = RealRegDeleteKey( KeyOrHandle );
#else
    regobj = GetPointer( KeyOrHandle );
    ReleasePointer( regobj );
#endif
    if( fullname ) {

        KdPrint(("RegDeleteKey: %s => 0x%X\n", fullname, ntstatus ));
        if( FilterDef.logwrites && ErrorString( ntstatus )) {
            LogRecord( "%s\tDeleteKey\t%s\t%s\tKey: 0x%X", 
                       GetProcess( name ), fullname, 
                       ErrorString( ntstatus ), regobj);
        } 
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );        
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegDeleteValueKey
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegDeleteValueKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE KeyOrHandle, 
    PUNICODE_STRING Name 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( KeyOrHandle, Name, fullname );
#ifndef WNET
    ntstatus = RealRegDeleteValueKey( KeyOrHandle, Name );
#endif
    if( fullname ) {

        KdPrint(("RegDeleteValueKey: %s => %x\n", fullname, ntstatus ));
        if( FilterDef.logwrites && ErrorString( ntstatus ) ) {
            LogRecord( "%s\tDeleteValueKey\t%s\t%s\t", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ));
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );        
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegSetValueKey
//
//---------------------------------------------------------------------- 
NTSTATUS 
NTAPI
HookRegSetValueKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE KeyOrHandle, 
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex, 
    IN ULONG Type, 
    IN PVOID Data, 
    IN ULONG DataSize 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    PUNICODE_STRING         valueName;
    CHAR                    data[MAXVALLEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    if( !ValueName || !ValueName->Length ) valueName = &DefaultValue;
    else                                   valueName = ValueName;
    GetFullName( KeyOrHandle, valueName, fullname );
#ifndef WNET
    ntstatus = RealRegSetValueKey( KeyOrHandle, ValueName, TitleIndex,
                                   Type, Data, DataSize );
#endif
    data[0] = 0;
    if( NT_SUCCESS( ntstatus )) {

        AppendRegValueData( Type, Data, DataSize, data );

    } else if( ntstatus == STATUS_ACCESS_DENIED ) {

        GetUserName( data );
    }
    if( fullname ) {

        KdPrint(("SetValue: %s (%s)\n", fullname, data ));
        if( FilterDef.logwrites && ErrorString( ntstatus )) {
            LogRecord( "%s\tSetValue\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegEnumerateKey
//
// This is a documented Zw-class function. 
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegEnumerateKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE KeyOrHandle, 
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation, 
    IN ULONG Length, 
    OUT PULONG pResultLength 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    data[MAXVALLEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( KeyOrHandle, NULL, fullname );
#ifndef WNET
    ntstatus = RealRegEnumerateKey( KeyOrHandle, Index, KeyInformationClass,
                                    KeyInformation, Length, pResultLength );
#endif
    data[0] = 0;
    if( NT_SUCCESS( ntstatus )) {

        AppendKeyInformation( KeyInformationClass, KeyInformation, data, MAXVALLEN );

    } else if( ntstatus == STATUS_ACCESS_DENIED ) {

        GetUserName( data );
    }
    if( fullname ) {

        KdPrint(("EnumerateKey: %s (%s) => %x\n", fullname, data, ntstatus ));
        if( FilterDef.logreads && ErrorString( ntstatus )) {
            LogRecord( "%s\tEnumerateKey\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegQueryKey
//
// This is a documented Zw-class function. This will get called
// from our CloseKey hook routine, because this is the only easy
// way we can determine if a registry key is being closed. Thus, we
// have to watch for those calls and not log any data about them.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegQueryKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE  KeyOrHandle, 
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation, 
    IN ULONG  Length, 
    OUT PULONG  pResultLength 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    name[MAXPROCNAMELEN];
    PCHAR                   fullname, data;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    data     = ExAllocatePool( PagedPool, MAXVALLEN );

    GetFullName( KeyOrHandle, NULL, fullname );
#ifndef WNET
    ntstatus = RealRegQueryKey( KeyOrHandle, KeyInformationClass,
                                KeyInformation, Length, pResultLength );
#endif
    if( fullname && data ) {

        data[0] = 0;
        if( NT_SUCCESS( ntstatus )) {

            AppendKeyInformation( KeyInformationClass, KeyInformation, data, MAXVALLEN );

        } else if( ntstatus == STATUS_ACCESS_DENIED ) {

            GetUserName( data );
        }
        KdPrint(("QueryKey: %s (%s) => %x\n", fullname, data, ntstatus ));
        if( FilterDef.logreads && ErrorString( ntstatus )) {
            LogRecord( "%s\tQueryKey\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
    }

    if( fullname ) ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    if( data )     ExFreePool( data );
    return ntstatus;
}

//----------------------------------------------------------------------
//
// HookRegSetInformationKey
//
// This is an undocumented Zw-class function.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegSetInformationKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE  KeyOrHandle, 
    IN KEY_SET_INFORMATION_CLASS  KeySetInformationClass,
    OUT PVOID  KeySetInformation, 
    IN ULONG  Length
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    name[MAXPROCNAMELEN];
    PCHAR                   fullname, data;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    data     = ExAllocatePool( PagedPool, MAXVALLEN );

    GetFullName( KeyOrHandle, NULL, fullname );
#ifndef WNET
    ntstatus = RealRegSetInformationKey( KeyOrHandle, KeySetInformationClass,
                                         KeySetInformation, Length );
#endif
    if( fullname && data ) {

        data[0] = 0;
        AppendKeySetInformation( KeySetInformationClass, KeySetInformation, data, MAXVALLEN );
        KdPrint(("SetInformationKey: %s (%s) => %x\n", fullname, data, ntstatus ));
        if( FilterDef.logreads && ErrorString( ntstatus )) {
            LogRecord( "%s\tSetInformationKey\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
    }

    if( fullname ) ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    if( data )     ExFreePool( data );
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegEnumerateValueKey
//
// This is a documented Zw-class function.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegEnumerateValueKey(
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE KeyOrHandle, 
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    CHAR                    data[MAXVALLEN]; 
    CHAR                    valuename[MAXVALLEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    GetFullName( KeyOrHandle, NULL, fullname );
#ifndef WNET
    ntstatus = RealRegEnumerateValueKey( KeyOrHandle, Index,
                                         KeyValueInformationClass,
                                         KeyValueInformation, Length, 
                                         pResultLength );
#endif
    if( fullname ) {
        
        data[0] = 0;
        if( NT_SUCCESS( ntstatus )) {
            
            AppendValueInformation( KeyValueInformationClass, 
                                    KeyValueInformation, data, valuename );
            strcat( fullname, "\\" );
            strncat( fullname, valuename, MAXPATHLEN - 1 - strlen(fullname) );

        } else if( ntstatus == STATUS_ACCESS_DENIED ) {

            GetUserName( data );
        }
        KdPrint(("EnumerateValue: %s (%s) =>%x\n", fullname, data, ntstatus ));
        if( FilterDef.logreads && ErrorString( ntstatus )) {
            LogRecord( "%s\tEnumerateValue\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegQueryValueKey
//
// This is a documented Zw-class function.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegQueryValueKey( 
#ifdef WNET
    IN NTSTATUS ntstatus,
#endif
    IN HANDLE KeyOrHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    )
{
#ifndef WNET
    NTSTATUS                ntstatus;
#endif
    PUNICODE_STRING         valueName;
    CHAR                    data[MAXVALLEN], name[MAXPROCNAMELEN];
    PCHAR                   fullname;

    fullname = ExAllocateFromPagedLookasideList( &FullPathLookaside );
    if( !ValueName || !ValueName->Length ) valueName = &DefaultValue;
    else                                   valueName = ValueName;
    GetFullName( KeyOrHandle, valueName, fullname );
#ifndef WNET
    ntstatus = RealRegQueryValueKey( KeyOrHandle, ValueName,
                                     KeyValueInformationClass,
                                     KeyValueInformation, Length, 
                                     pResultLength );
#endif
    if( fullname ) {

        data[0] = 0;
        if( NT_SUCCESS( ntstatus )) {

            AppendValueInformation( KeyValueInformationClass, 
                                    KeyValueInformation, data, FALSE );

        } else if( ntstatus == STATUS_ACCESS_DENIED ) {

            GetUserName( data );
        }
        KdPrint(("QueryValue: %s (%s) =>%x\n", fullname, data, ntstatus ));
        if( FilterDef.logreads && ErrorString( ntstatus )) {
            LogRecord( "%s\tQueryValue\t%s\t%s\t%s", 
                       GetProcess( name ), fullname, ErrorString( ntstatus ), data );
        }
        ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
    }
    return ntstatus;
}


//----------------------------------------------------------------------
//
// HookRegLoadKey
//
// Undocumented function: loads a hive from disk into the Registry.
// Regedt32 uses this for its "Load Hive" functionality.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegLoadKey( 
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES HiveFile 
    )
{
    NTSTATUS                ntstatus;
    CHAR                    hivefullname[MAXPATHLEN], keyfullname[MAXPATHLEN];
    CHAR                    name[MAXPROCNAMELEN];

    GetFullName( HiveFile->RootDirectory, HiveFile->ObjectName, hivefullname );
    GetFullName( TargetKey->RootDirectory, TargetKey->ObjectName, keyfullname );
    ntstatus = RealRegLoadKey( TargetKey, HiveFile );
    KdPrint(("RegLoadKey: %s:%s => %x\n", keyfullname, hivefullname, ntstatus ));
    if( FilterDef.logreads && ErrorString( ntstatus )) {
        LogRecord( "%s\tLoadKey\t%s\t%s\t%s", GetProcess( name ),
                   keyfullname, ErrorString( ntstatus ), hivefullname );
    }
    return ntstatus;
}
       

//----------------------------------------------------------------------
//
// HookRegUnloadKey
//
// Undocumented function: unloads a hive, previously loaded with
// ZwLoadKey, from the Registry. Regedt32 uses this for its 
// "Load Hive" functionality.
//
//----------------------------------------------------------------------
NTSTATUS 
NTAPI
HookRegUnloadKey( 
    IN POBJECT_ATTRIBUTES TargetKey 
    )
{
    NTSTATUS                ntstatus;
    CHAR                    keyfullname[MAXPATHLEN];
    CHAR                    name[MAXPROCNAMELEN];

    GetFullName( TargetKey->RootDirectory, TargetKey->ObjectName, keyfullname );
    ntstatus = RealRegUnloadKey( TargetKey );
    KdPrint(("RegUnloadKey: %s => %x\n", keyfullname ));
    if( FilterDef.logreads && ErrorString( ntstatus )) {
        LogRecord( "%s\tUnloadKey\t%s\t%s", GetProcess( name ),
                   keyfullname, ErrorString( ntstatus ));
    }
    return ntstatus;
}

#if WNET
//----------------------------------------------------------------------
//
// FindPostContext
//
// Looks for a post context on the list.
//
//----------------------------------------------------------------------
PVOID 
FindPostContext(
    PCHAR *FullName
    )
{
    PPOST_CONTEXT   postContext;
    PLIST_ENTRY     postContextEntry;
    PVOID           currentThread, argument;
    
    currentThread = (PVOID) PsGetCurrentThread();
    MUTEX_ACQUIRE( PostContextMutex );
    postContextEntry = PostContextList.Flink;
    while( postContextEntry != &PostContextList ) {

        if( CONTAINING_RECORD( postContextEntry, POST_CONTEXT, NextContext )->Thread ==
            currentThread ) {
            
            RemoveEntryList( postContextEntry );
            MUTEX_RELEASE( PostContextMutex );
            postContext = CONTAINING_RECORD( postContextEntry, POST_CONTEXT, NextContext );
            argument = postContext->Argument;
            if( FullName ) {

                *FullName = postContext->FullName;
            }
            KdPrint(("FindPostContext: Thread: %x Argument: %x\n",
                     currentThread, argument ));
            ExFreeToPagedLookasideList( &PostContextLookaside, postContext );
            return argument;
        }            
        postContextEntry = postContextEntry->Flink;
    }
    MUTEX_RELEASE( PostContextMutex );
    KdPrint(("FindPostContext: Thread: %x: NULL\n", currentThread ));
    return NULL;
}


//----------------------------------------------------------------------
//
// RegmonHookCallback
//
// This is the callback function that the Configuration Manager
// (Registry) calls us at to tell us about Registry activity and 
// give us a chance to influence the behavior of Registry functions.
//
//----------------------------------------------------------------------
NTSTATUS
RegmonHookCallback(
    PVOID Context,
    PVOID RegFunction,
    PVOID Argument
    )
{
    PREG_SET_VALUE_KEY_INFORMATION        setValueInfo;
    PREG_DELETE_VALUE_KEY_INFORMATION     deleteValueInfo;
    PREG_SET_INFORMATION_KEY_INFORMATION  setInformationInfo;
    PREG_ENUMERATE_KEY_INFORMATION        enumerateInfo;
    PREG_ENUMERATE_VALUE_KEY_INFORMATION  enumerateValueInfo;
    PREG_OPEN_KEY_INFORMATION             openInfo;
    PREG_QUERY_KEY_INFORMATION            queryInfo;
    PREG_QUERY_VALUE_KEY_INFORMATION      queryValueInfo;
    PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION queryMultipleValueInfo;
    PREG_RENAME_KEY_INFORMATION           renameInfo;
    PREG_KEY_HANDLE_CLOSE_INFORMATION     keyCloseInfo;
    PPOST_CONTEXT                         postContext;
    PVOID                                 keyObject;
    CHAR                                  name[MAXPROCNAMELEN];
    PCHAR                                 fullname;
    NTSTATUS                              ntStatus;

    //
    // Process each function we can get called for
    //
    switch( (REG_NOTIFY_CLASS) RegFunction ) {

    case RegNtPostOpenKey:
    case RegNtPostCreateKey:
    case RegNtPostRenameKey:
        FindPostContext( &fullname );
        if( fullname ) {

            if( NT_SUCCESS( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status )) {

                RegmonFreeHashEntry( ((PREG_POST_OPERATION_INFORMATION) Argument)->Object );
                RegmonLogHash( ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, fullname );
            }
            LogRecord( "%s\t%s\t%s\t%s\tKey: 0x%X", GetProcess( name ),
                       (REG_NOTIFY_CLASS) RegFunction == RegNtPostOpenKey ? 
                       "OpenKey" : 
                       (REG_NOTIFY_CLASS) RegFunction == RegNtPostCreateKey ? "CreateKey" : "RenameKey",
                       fullname, ErrorString( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status ),
                       NT_SUCCESS(((PREG_POST_OPERATION_INFORMATION) Argument)->Status) ? 
                       ((PREG_POST_OPERATION_INFORMATION) Argument)->Object : 0 );
            ExFreeToPagedLookasideList( &FullPathLookaside, fullname );
        }
        break;

    case RegNtPostDeleteKey:
        FindPostContext( &fullname );
        if( fullname ) {

            HookRegDeleteKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                              fullname, 
                              ((PREG_POST_OPERATION_INFORMATION) Argument)->Object );
        }
        break;

    case RegNtPostSetValueKey:
        setValueInfo = (PREG_SET_VALUE_KEY_INFORMATION) FindPostContext( NULL );
        if( setValueInfo ) {

            HookRegSetValueKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                setValueInfo->ValueName,
                                setValueInfo->TitleIndex,
                                setValueInfo->Type,
                                setValueInfo->Data,
                                setValueInfo->DataSize );
        }
        break;

    case RegNtPostDeleteValueKey:
        deleteValueInfo = (PREG_DELETE_VALUE_KEY_INFORMATION) FindPostContext( NULL );
        if( deleteValueInfo ) {

            HookRegDeleteValueKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                   ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                   deleteValueInfo->ValueName );
        }
        break;

    case RegNtPostEnumerateKey:
        enumerateInfo = (PREG_ENUMERATE_KEY_INFORMATION) FindPostContext( NULL );
        if( enumerateInfo ) {

            HookRegEnumerateKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                 ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                 enumerateInfo->Index,
                                 enumerateInfo->KeyInformationClass,
                                 enumerateInfo->KeyInformation,
                                 enumerateInfo->Length,
                                 enumerateInfo->ResultLength );
        }
        break;

    case RegNtPostEnumerateValueKey:
        enumerateValueInfo = (PREG_ENUMERATE_VALUE_KEY_INFORMATION) FindPostContext( NULL );
        if( enumerateValueInfo ) {

            HookRegEnumerateValueKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                      ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                      enumerateValueInfo->Index,
                                      enumerateValueInfo->KeyValueInformationClass,
                                      enumerateValueInfo->KeyValueInformation,
                                      enumerateValueInfo->Length,
                                      enumerateValueInfo->ResultLength );
        }
        break;

    case RegNtPostQueryKey:
        queryInfo = (PREG_QUERY_KEY_INFORMATION) FindPostContext( NULL );
        if( queryInfo ) {

            HookRegQueryKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                             ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                             queryInfo->KeyInformationClass,
                             queryInfo->KeyInformation,
                             queryInfo->Length,
                             queryInfo->ResultLength );
        }
        break;

    case RegNtPostQueryValueKey:
        queryValueInfo = (PREG_QUERY_VALUE_KEY_INFORMATION) FindPostContext( NULL );
        if( queryValueInfo ) {

            HookRegQueryValueKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                  ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                  queryValueInfo->ValueName,
                                  queryValueInfo->KeyValueInformationClass,
                                  queryValueInfo->KeyValueInformation,
                                  queryValueInfo->Length,
                                  queryValueInfo->ResultLength );
        }
        break;

    case RegNtPostKeyHandleClose:
        keyCloseInfo = FindPostContext( &fullname );
        HookRegCloseKey( fullname, keyCloseInfo->Object );
        break;

    case RegNtPostQueryMultipleValueKey: // New to XP(?)
        queryMultipleValueInfo = (PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION) Argument;
        break;

    case RegNtPostSetInformationKey: 
        setInformationInfo = (PREG_SET_INFORMATION_KEY_INFORMATION) Argument;
        if( setInformationInfo ) {

            HookRegSetInformationKey( ((PREG_POST_OPERATION_INFORMATION) Argument)->Status, 
                                      ((PREG_POST_OPERATION_INFORMATION) Argument)->Object, 
                                      setInformationInfo->KeySetInformationClass,
                                      setInformationInfo->KeySetInformation,
                                      setInformationInfo->KeySetInformationLength );
        }
        break;

    case RegNtPreOpenKey:
    case RegNtPreCreateKey:
    case RegNtPreDeleteKey:
    case RegNtPreSetValueKey:
    case RegNtPreDeleteValueKey:
    case RegNtPreEnumerateValueKey:
    case RegNtPreQueryKey:
    case RegNtPreEnumerateKey:
    case RegNtPreQueryValueKey:
    case RegNtPreKeyHandleClose:
    case RegNtPreRenameKey:
#if 0
    case RegNtPreQueryMultipleValueKey:
    case RegNtPreSetInformationKey:
#endif
        //
        // Store away the parameters in a list indexed by thread ID so 
        // that we can find it in the post callback 
        //
        postContext = ExAllocateFromPagedLookasideList( &PostContextLookaside );
        postContext->Thread = (PVOID) PsGetCurrentThread();
        postContext->Argument = Argument;
        KdPrint(("CreatePostContext: Req: %d Thread: %x Argument: %x\n",
                 RegFunction, 
                 postContext->Thread, postContext->Argument ));

        //
        // We have three special cases: close and delete, where we have to capture
        // the name now since it won't be available after the key is deleted or closed
        // and the post callback is invoked. 
        //
        if( (REG_NOTIFY_CLASS) RegFunction == RegNtPreDeleteKey ||
            (REG_NOTIFY_CLASS) RegFunction == RegNtPreKeyHandleClose ) {

            postContext->FullName = ExAllocateFromPagedLookasideList( &FullPathLookaside );
            if( postContext->FullName ) {

                GetFullName( ((PREG_DELETE_KEY_INFORMATION) postContext->Argument)->Object,
                              NULL, postContext->FullName );
            }
           
        } else if( (REG_NOTIFY_CLASS) RegFunction == RegNtPreOpenKey ||
                   (REG_NOTIFY_CLASS) RegFunction == RegNtPreCreateKey ||
                   (REG_NOTIFY_CLASS) RegFunction == RegNtPreRenameKey ) {

            //
            // For open, rename and create the name can be reallocated before the post handler
            // is called
            //
            postContext->FullName = ExAllocateFromPagedLookasideList( &FullPathLookaside );
            if( postContext->FullName ) {

                GetFullName( ((PREG_CREATE_KEY_INFORMATION) postContext->Argument)->RootObject,
                              ((PREG_CREATE_KEY_INFORMATION) postContext->Argument)->CompleteName, 
                             postContext->FullName );
            }
        } else {

            postContext->FullName = NULL;
        }
        MUTEX_ACQUIRE( PostContextMutex );
        InsertHeadList( &PostContextList, &postContext->NextContext );
        MUTEX_RELEASE( PostContextMutex );
        break;

    default:

        //
        // Unknown callback
        //
        KdPrint(("****** Unkown callback: %d\n", RegFunction ));
        break;
    }

    //
    // Regmon always returns success, but an active filter 
    // could change Registry behavior by returning something else
    //
    return STATUS_SUCCESS;
}
#endif // WNET


//----------------------------------------------------------------------
//           S E R V I C E   T A B L E   H O O K I N G
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// HookRegistry
//
// Replaces entries in the system service table with pointers to
// our own hook routines. We save off the real routine addresses.
//
//----------------------------------------------------------------------
NTSTATUS
HookRegistry( 
    VOID 
    )
{
    NTSTATUS   status = STATUS_SUCCESS;
#ifndef WNET

#if defined(_M_IA64)
    ULONGLONG  gpReg;
    static BOOLEAN stubsPatched = FALSE;
#endif

    if( !RegHooked ) {

        //
        // Hook everything
        //
#if defined(_M_IA64)
        //
        // On IA64 we have to patch up our stub functions to
        // set the GP register to our (regsys.sys's) GP value
        //
        gpReg = ReadGpRegister();
#endif
        HOOK_SYSCALL( ZwOpenKey, HookRegOpenKey, RealRegOpenKey );
        HOOK_SYSCALL( ZwClose, HookRegCloseKey, RealRegCloseKey );
        HOOK_SYSCALL( ZwQueryKey, HookRegQueryKey, RealRegQueryKey );
        HOOK_SYSCALL( ZwQueryKey, HookRegSetInformationKey, RealRegSetInformationKey );
        HOOK_SYSCALL( ZwQueryValueKey, HookRegQueryValueKey, RealRegQueryValueKey );
        HOOK_SYSCALL( ZwEnumerateValueKey, HookRegEnumerateValueKey, RealRegEnumerateValueKey );
        HOOK_SYSCALL( ZwEnumerateKey, HookRegEnumerateKey, RealRegEnumerateKey );
        HOOK_SYSCALL( ZwFlushKey, HookRegFlushKey, RealRegFlushKey );
        HOOK_SYSCALL( ZwDeleteKey, HookRegDeleteKey, RealRegDeleteKey );
        HOOK_SYSCALL( ZwSetValueKey, HookRegSetValueKey, RealRegSetValueKey );
        HOOK_SYSCALL( ZwCreateKey, HookRegCreateKey, RealRegCreateKey );
        HOOK_SYSCALL( ZwDeleteValueKey, HookRegDeleteValueKey, RealRegDeleteValueKey );
        HOOK_SYSCALL( ZwLoadKey, HookRegLoadKey, RealRegLoadKey );
        HOOK_SYSCALL( ZwUnloadKey, HookRegUnloadKey, RealRegUnloadKey );
        RegHooked = TRUE;
#if defined(_M_IA64)
        stubsPatched = TRUE;
#endif
    }
#else
    if( !RegHooked ) {

        status = CmRegisterCallback( RegmonHookCallback, 0, &HookCallbackId );
        if( NT_SUCCESS( status )) {

            RegHooked = TRUE;
        }
    }
#endif 
    return status;
}


//----------------------------------------------------------------------
//
// UnhookRegistry
//
// Unhooks all registry routines by replacing the hook addresses in 
// the system service table with the real routine addresses that we
// saved off.
//
//----------------------------------------------------------------------
VOID 
UnhookRegistry( 
    VOID
    )
{
    if( RegHooked ) {

        //
        // Unhook everything
        //
#ifndef WNET
        UNHOOK_SYSCALL( ZwOpenKey, HookRegOpenKey, RealRegOpenKey );
        UNHOOK_SYSCALL( ZwClose, HookRegCloseKey, RealRegCloseKey );
        UNHOOK_SYSCALL( ZwQueryKey, HookRegQueryKey, RealRegQueryKey );
        UNHOOK_SYSCALL( ZwQueryKey, HookRegSetInformationKey, RealRegSetInformationKey );
        UNHOOK_SYSCALL( ZwQueryValueKey, HookRegQueryValueKey, RealRegQueryValueKey );
        UNHOOK_SYSCALL( ZwEnumerateValueKey, HookRegEnumerateValueKey, RealRegEnumerateValueKey );
        UNHOOK_SYSCALL( ZwEnumerateKey, HookRegEnumerateKey, RealRegEnumerateKey );
        UNHOOK_SYSCALL( ZwFlushKey, HookRegFlushKey, RealRegFlushKey );
        UNHOOK_SYSCALL( ZwDeleteKey, HookRegDeleteKey, RealRegDeleteKey );
        UNHOOK_SYSCALL( ZwSetValueKey, HookRegSetValueKey, RealRegSetValueKey );
        UNHOOK_SYSCALL( ZwCreateKey, HookRegCreateKey, RealRegCreateKey );
        UNHOOK_SYSCALL( ZwDeleteValueKey, HookRegDeleteValueKey, RealRegDeleteValueKey );
        UNHOOK_SYSCALL( ZwLoadKey, HookRegLoadKey, RealRegLoadKey );
        UNHOOK_SYSCALL( ZwUnloadKey, HookRegUnloadKey, RealRegUnloadKey );
#else
        CmUnRegisterCallback( HookCallbackId );
#endif        
        RegHooked = FALSE;
    }
}

//======================================================================
//         D E V I C E - D R I V E R  R O U T I N E S 
//======================================================================

//----------------------------------------------------------------------
//
// RegmonDeviceControl
//
//----------------------------------------------------------------------
BOOLEAN  
RegmonDeviceControl( 
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
    BOOLEAN               retval = FALSE;
    PLOG_BUF              old;
    BOOLEAN               logMutexReleased;

    //
    // Its a message from our GUI!
    //
    IoStatus->Status      = STATUS_SUCCESS; // Assume success
    IoStatus->Information = 0;              // Assume nothing returned
    switch ( IoControlCode ) {

    case IOCTL_REGMON_VERSION:

        //
        // Version #
        //
        if( OutputBufferLength < sizeof(ULONG) ||
             OutputBuffer == NULL ) {

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }
            
        *(ULONG *)OutputBuffer = REGMON_VERSION;
        IoStatus->Information = sizeof(ULONG);
        break;

    case IOCTL_REGMON_HOOK:
        KdPrint (("Regmon: hook\n"));
        IoStatus->Status = HookRegistry();
        break;

    case IOCTL_REGMON_UNHOOK:
        KdPrint(("Regmon: unhook\n"));
        UnhookRegistry();
        break;

    case IOCTL_REGMON_ZEROSTATS:

        //
        // Zero contents of buffer
        //
        KdPrint (("Regmon: zero stats\n"));

        MUTEX_ACQUIRE( LogMutex );
        while( Log->Next )  {

            //
            // Free all but the first output buffer
            //
            old = Log->Next;
            Log->Next = old->Next;
            ExFreePool( old );
            NumLog--;
        }

        //
        // Set the output pointer to the start of the output buffer
        //
        Log->Len = 0;

        //
        // Reset sequence and relative start time
        //
        Sequence = 0;
        StartTime = KeQueryPerformanceCounter( NULL );
        MUTEX_RELEASE( LogMutex );
        break;

    case IOCTL_REGMON_GETSTATS:

        //
        // Copy buffer into user space.
        //
        KdPrint (("Regmon: get stats\n"));

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

        MUTEX_ACQUIRE( LogMutex );
        if( LOGBUFSIZE > OutputBufferLength )  {

            //
            // Output buffer isn't big enough
            //
            MUTEX_RELEASE( LogMutex );
            IoStatus->Status = STATUS_INVALID_PARAMETER;
            return FALSE;

        } else if( Log->Len  ||  Log->Next ) {

            //
            // Switch to a new Log
            //
            RegmonNewLog();

            //
            // Fetch the oldest to give to user
            //
            old = RegmonOldestLog();

            if( old != Log ) {

                logMutexReleased = TRUE;
                MUTEX_RELEASE( LogMutex );

            } else {

                logMutexReleased = FALSE;
            }

            //
            // Copy it
            //
            memcpy( OutputBuffer, old->Data, old->Len );

            //
            // Return length of copied info
            //
            IoStatus->Information = old->Len;

            //
            // Deallocate buffer - unless its the last one
            //
            if( logMutexReleased ) {

                ExFreePool( old );

            } else {

                Log->Len = 0;
                MUTEX_RELEASE( LogMutex );
            }

        } else {

            //
            // No unread data
            //
            MUTEX_RELEASE( LogMutex );
            IoStatus->Information = 0;
        }
        break;

    case IOCTL_REGMON_SETFILTER:

        //        
        // GUI is updating the filter
        //
        KdPrint(("Regmon: set filter\n"));

        if( InputBufferLength < sizeof(FILTER) ||
             InputBuffer == NULL ) {

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            break;
        }

        FilterDef = *(PFILTER) InputBuffer;
        RegmonUpdateFilters();
        break;
 
    default:
        KdPrint (("Regmon: unknown IRP_MJ_DEVICE_CONTROL\n"));
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    return TRUE;
}


//----------------------------------------------------------------------
//
// RegmonDispatch
//
// In this routine we handle requests to our own device. The only 
// requests we care about handling explicitely are IOCTL commands that
// we will get from the GUI. We also expect to get Create and Close 
// commands when the GUI opens and closes communications with us.
//
//----------------------------------------------------------------------
NTSTATUS 
RegmonDispatch( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   inputBuffer;
    PVOID                   outputBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
    PLOG_BUF                old;
    WORK_QUEUE_ITEM         workItem;

    //
    // Go ahead and set the request up as successful
    //
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer             = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength       = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBuffer            = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode           = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:

        KdPrint(("Regmon: IRP_MJ_CREATE\n"));

        //
        // Turn off boot logging
        //
        if( BootLogging ) {

            BootLogging = FALSE;
            IoUnregisterShutdownNotification( DeviceObject );
            UnhookRegistry();
            
            MUTEX_ACQUIRE( LogMutex );
            
            ExInitializeWorkItem( &workItem, RegmonCloseBootLog, 0 );
            ExQueueWorkItem( &workItem, CriticalWorkQueue );
            KeWaitForSingleObject( &LoggingEvent, Executive, KernelMode, FALSE, NULL );

            MUTEX_RELEASE( LogMutex ); 
        }

        GUIActive = TRUE;
        KdPrint((" GUI Active: %d\n", GUIActive ));
        break;

    case IRP_MJ_SHUTDOWN:
        
        //
        // Dump the most current buffer. We are in the system process so
        // there's no need to queue a worker thread item
        //
        RegmonWriteBootLog( Log );
        break;

    case IRP_MJ_CLOSE:

        KdPrint(("Regmon: IRP_MJ_CLOSE\n"));
        GUIActive = FALSE;
        KdPrint((" GUI closing: %d\n", GUIActive ));
        RegmonResetLog();
        break;

    case IRP_MJ_DEVICE_CONTROL:

        KdPrint (("Regmon: IRP_MJ_DEVICE_CONTROL\n"));

       	//
        // See if the output buffer is really a user buffer that we
        // can just dump data into.
        //
        if( IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER ) {

            outputBuffer = Irp->UserBuffer;
        }

        //
        // Its a request from the GUI
        //
        RegmonDeviceControl( irpStack->FileObject, TRUE,
                             inputBuffer, inputBufferLength, 
                             outputBuffer, outputBufferLength,
                             ioControlCode, &Irp->IoStatus, DeviceObject );
        break;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;   
}


//----------------------------------------------------------------------
//
// RegmonUnload
//
// Our job is done - time to leave.
//
//----------------------------------------------------------------------
VOID 
RegmonUnload( 
    IN PDRIVER_OBJECT DriverObject 
    )
{
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\Regmon";
    UNICODE_STRING          deviceLinkUnicodeString;

    KdPrint(("Regmon: unloading\n"));

    //
    // Unhook the registry
    //
    UnhookRegistry();

    //
    // Delete the symbolic link for our device
    //
    RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
    IoDeleteSymbolicLink( &deviceLinkUnicodeString );

    //
    // Delete the device object
    //
    IoDeleteDevice( DriverObject->DeviceObject );
    KdPrint(("Regmon: deleted device\n"));

    //
    // Now we can free any memory we have outstanding
    //
    RegmonHashCleanup();
    RegmonFreeLog();
#ifndef WNET
    RegmonUnmapServiceTable( KeServiceTablePointers );
#endif
    ExDeletePagedLookasideList( &FullPathLookaside );
    KdPrint(("Regmon: freed memory\n"));
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
    WCHAR                   deviceNameBuffer[]  = L"\\Device\\Regmon";
    UNICODE_STRING          deviceNameUnicodeString;
    WCHAR                   deviceLinkBuffer[]  = L"\\DosDevices\\Regmon";
    UNICODE_STRING          deviceLinkUnicodeString;    
    WCHAR                   startValueBuffer[] = L"Start";
    UNICODE_STRING          startValueUnicodeString;
    WCHAR                   bootMessage[] = 
        L"\nRegmon is logging Registry activity to \\SystemRoot\\Regmon.log\n\n";
    UNICODE_STRING          bootMessageUnicodeString;
    UNICODE_STRING          registryPath; 
    HANDLE                  driverKey;
    PETHREAD                curthread;
    ULONG                   startType, demandStart;
    RTL_QUERY_REGISTRY_TABLE paramTable[2]; 
    OBJECT_ATTRIBUTES       objectAttributes;
    int                     i;

    // 
    // Query our start type to see if we are supposed to monitor starting
    // at boot time
    //
    KdPrint (("Regmon: entering DriverEntry\n"));
    registryPath.Buffer = ExAllocatePool( PagedPool, 
                                          RegistryPath->Length + sizeof(UNICODE_NULL)); 
    if(!registryPath.Buffer) { 
 
        return STATUS_INSUFFICIENT_RESOURCES; 
    } 
 
    registryPath.Length = RegistryPath->Length + sizeof(UNICODE_NULL); 
    registryPath.MaximumLength = registryPath.Length; 

    RtlZeroMemory( registryPath.Buffer, registryPath.Length ); 
 
    RtlMoveMemory( registryPath.Buffer,  RegistryPath->Buffer, 
                   RegistryPath->Length  ); 

    RtlZeroMemory( &paramTable[0], sizeof(paramTable)); 
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT; 
    paramTable[0].Name = L"Start"; 
    paramTable[0].EntryContext = &startType;
    paramTable[0].DefaultType = REG_DWORD; 
    paramTable[0].DefaultData = &startType; 
    paramTable[0].DefaultLength = sizeof(ULONG); 

    RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                            registryPath.Buffer, &paramTable[0], 
                            NULL, NULL  );

    //
    // Set start type to demand start so that boot logging
    // only happens this boot (unless the user reconfigures it in 
    // the GUI)
    //
    InitializeObjectAttributes( &objectAttributes, RegistryPath,
                                OBJ_CASE_INSENSITIVE, NULL, NULL );
    ntStatus = ZwOpenKey( &driverKey, KEY_WRITE, &objectAttributes );
    if( NT_SUCCESS( ntStatus )) {

        demandStart = SERVICE_DEMAND_START;
        RtlInitUnicodeString( &startValueUnicodeString, startValueBuffer );
        ZwSetValueKey( driverKey, &startValueUnicodeString, 0, REG_DWORD, 
                       &demandStart, sizeof(demandStart ));
        ZwClose( driverKey );
    }

    //
    // Setup our name and symbolic link. 
    //
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );
    RtlInitUnicodeString (&deviceLinkUnicodeString,
                          deviceLinkBuffer );

    //
    // Set up the device used for GUI communications
    //
    ntStatus = IoCreateDevice ( DriverObject,
                                0,
                                &deviceNameUnicodeString,
                                FILE_DEVICE_REGMON,
                                0,
                                TRUE,
                                &GUIDevice );
    if( NT_SUCCESS(ntStatus)) {
                
        //
        // Create a symbolic link that the GUI can specify to gain access
        // to this driver/device
        //
        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString );

        //
        // Create dispatch points for all routines that must be handled
        //
        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        =
        DriverObject->MajorFunction[IRP_MJ_CREATE]          =
        DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = RegmonDispatch;
#if DBG

        //
        // Its extremely unsafe to unload a system-call hooker, so this
        // is only enabled in the debug version for testing purposes.
        //
        DriverObject->DriverUnload                          = RegmonUnload;
#endif
    }
    if( !NT_SUCCESS(ntStatus)) {

        KdPrint(("Regmon: Failed to create our device!\n"));

        //
        // Something went wrong, so clean up (free resources etc)
        //
        if( GUIDevice ) IoDeleteDevice( GUIDevice );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return ntStatus;
    }

    //
    // Initialize our mutexes
    //
    MUTEX_INIT( LogMutex );
    MUTEX_INIT( HashMutex );
    MUTEX_INIT( FilterMutex );

    //
    // Initialize rootkey lengths
    //
    for( i = 0; i < NUMROOTKEYS; i++ ) {

        RootKey[i].RootNameLen = strlen( RootKey[i].RootName );
    }
    for( i = 0; i < 2; i++ ) {

        CurrentUser[i].RootNameLen = strlen( CurrentUser[i].RootName );
    }
#ifndef WNET
    //
    // Use a secret function to obtain a pointer to the system service
    // table. The function is in the object-only regmlib.lib because
    // Microsoft requested that it not be published.
    //
    KeServiceTablePointers = RegmonMapServiceTable( &HookDescriptors );
    KdPrint(("Hookregistry: Servicetable: %x\n", KeServiceTablePointers ));
    if( !KeServiceTablePointers ) {

        IoDeleteDevice( GUIDevice );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#else
    //
    // Initialize post-callback context lookaside
    //
    ExInitializePagedLookasideList( &PostContextLookaside, NULL, NULL,
                                     0, sizeof(POST_CONTEXT), 'mgeR', 256 );
    InitializeListHead( &PostContextList );
    MUTEX_INIT( PostContextMutex );
#endif

    //
    // Find the process name offset
    //
    ProcessNameOffset = GetProcessNameOffset();

    //
    // Initialize a lookaside for key names
    //
    ExInitializePagedLookasideList( &FullPathLookaside, NULL, NULL,
                                     0, MAXPATHLEN, 'mgeR', 256 );

    //
    // Allocate the initial output buffer
    //
    Log = ExAllocatePool( PagedPool, sizeof(*Log) );
    if( !Log ) {

        IoDeleteDevice( GUIDevice );
        IoDeleteSymbolicLink( &deviceLinkUnicodeString );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Log->Len  = 0;
    Log->Next = NULL;
    NumLog = 1;

    //
    // Set up the default filter
    //
    FilterDef = BootFilter;
    RegmonUpdateFilters();

    //
    // If we're a boot driver start logging now
    //
    if( startType != SERVICE_DEMAND_START ) {

        //
        // Initiate logging
        //
        BootLogging = TRUE;
        
        KeInitializeEvent( &LoggingEvent, SynchronizationEvent, FALSE );
        GUIActive = TRUE;
        HookRegistry();

        //
        // Tell the user that boot-logging is on
        //
        RtlInitUnicodeString( &bootMessageUnicodeString, bootMessage );
        ZwDisplayString( &bootMessageUnicodeString );

        //
        // Register for shutdown notification
        //
        IoRegisterShutdownNotification( GUIDevice );
    }
    return STATUS_SUCCESS;
}

