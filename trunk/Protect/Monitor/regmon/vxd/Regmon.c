//======================================================================
//
// REGMON.c - main module for VxD REGMON
//
// Copyright(C) 1996-1999 Mark Russinovich and Bryce Cogswell
// Systems Internals - http://www.sysinternals.com
//
//======================================================================
#define   DEVICE_MAIN
#include  <vtoolsc.h>
#include  "..\exe\ioctlcmd.h"
#include  "regmon.h"
#undef    DEVICE_MAIN

#if DEBUG
#define dprint(arg) dprintf arg
#else
#define dprint(arg)
#endif

//----------------------------------------------------------------------
//                     G L O B A L   D A T A 
//----------------------------------------------------------------------

//
// Real service pointers with the hook thunks
//
HDSC_Thunk ROKThunk;
LONG (*RealRegOpenKey)(HKEY, PCHAR, PHKEY );
HDSC_Thunk RCKThunk;
LONG (*RealRegCloseKey)(HKEY );
HDSC_Thunk RFKThunk;
LONG (*RealRegFlushKey)(HKEY );
HDSC_Thunk RCRKThunk;
LONG (*RealRegCreateKey)(HKEY, PCHAR, PHKEY );
HDSC_Thunk RDKThunk;
LONG (*RealRegDeleteKey)(HKEY, PCHAR );
HDSC_Thunk RDVThunk;
LONG (*RealRegDeleteValue)(HKEY, PCHAR );
HDSC_Thunk REKThunk;
LONG (*RealRegEnumKey)(HKEY, DWORD, PCHAR, DWORD );
HDSC_Thunk REVThunk;
LONG (*RealRegEnumValue)(HKEY, DWORD, PCHAR, PDWORD, PDWORD, PDWORD,
                         PBYTE, PDWORD );
HDSC_Thunk RQIKThunk;
LONG (*RealRegQueryInfoKey)(HKEY, PCHAR, PDWORD, DWORD, PDWORD, PDWORD,
                            PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, 
                            PFILETIME );
LONG (__stdcall *RealWin32RegQueryInfoKey)(PCLIENT_STRUCT, DWORD, 
                                           HKEY, PDWORD, PDWORD, PDWORD, 
                                           PDWORD, PDWORD );
HDSC_Thunk RQVThunk;
LONG (*RealRegQueryValue)( HKEY, PCHAR, PCHAR, PLONG );
HDSC_Thunk RQVEThunk;
LONG (*RealRegQueryValueEx)(HKEY, PCHAR, PDWORD, PDWORD, PBYTE, PDWORD );
HDSC_Thunk RSVThunk;
LONG (*RealRegSetValue)( HKEY, PCHAR, DWORD, PCHAR, DWORD );
HDSC_Thunk RSVEThunk;
LONG (*RealRegSetValueEx)(HKEY, PCHAR, DWORD, DWORD, PBYTE, DWORD );
HDSC_Thunk RRPDKThunk;
LONG (*RealRegRemapPreDefKey)(HKEY, HKEY );
HDSC_Thunk RQMVThunk;
LONG (*RealRegQueryMultipleValues)(HKEY, PVALENT, DWORD, PCHAR, PDWORD );
HDSC_Thunk RCDKThunk;
LONG (*RealRegCreateDynKey)( PCHAR, PVOID, PVOID, PVOID, DWORD, PVMMHKEY);

//
// Indicates if the GUI wants activity to be logged
//
BOOLEAN                 FilterOn = FALSE;

//
// Global filter (sent to us by the GUI)
//
FILTER                  FilterDef;

//
// Array of process and path filters 
//
ULONG                   NumIncludeFilters = 0;
PCHAR                   IncludeFilters[MAXFILTERS];
ULONG                   NumExcludeFilters = 0;
PCHAR                   ExcludeFilters[MAXFILTERS];

//
// Hash table data 
//
PHASH_ENTRY		        HashTable[NUMHASH];


//
// Buffer data
//
PLOG_BUF		        Log 		= NULL;
ULONG			        Sequence 	= 0;

//
// Time of last clear operation
//
LARGE_INTEGER           StartTime;

//
// Maximum amount of buffers we will grab for buffered unread data
//
ULONG			        NumLog 	= 0;
ULONG			        MaxLog 	= 5;

//
// Semaphore for critical sections
//
SEMHANDLE               LogMutex, HashMutex, FilterMutex;

//
// Unknown error string
//
CHAR                    errstring[32];

//
// VMM's Win32 service table (at least one Win32 registry
// call gets routed through here, bypassing the
// standard VMM VxD service entry points!)
//
PDWORD                  VMMWin32ServiceTable;

//----------------------------------------------------------------------
//                     F O R W A R D S 
//----------------------------------------------------------------------

BOOLEAN
ApplyFilters( 
    PCHAR fullname 
    );

//----------------------------------------------------------------------
//                   V X D  C O N T R O L
//----------------------------------------------------------------------

//
// Device declaration
//
Declare_Virtual_Device(REGMON)

//
// Message handlers - we only care about dynamic loading and unloading
//
    DefineControlHandler(SYS_DYNAMIC_DEVICE_INIT, OnSysDynamicDeviceInit);
    DefineControlHandler(SYS_DYNAMIC_DEVICE_EXIT, OnSysDynamicDeviceExit);
    DefineControlHandler(W32_DEVICEIOCONTROL, OnW32Deviceiocontrol);

//----------------------------------------------------------------------
// 
// ControlDispatcher
//
// Multiplexes incoming VxD messages from Windows to their handlers.
//
//----------------------------------------------------------------------
BOOL 
__cdecl 
ControlDispatcher(
    DWORD dwControlMessage,
    DWORD EBX,
    DWORD EDX,
    DWORD ESI,
    DWORD EDI,
    DWORD ECX
    )
{
    START_CONTROL_DISPATCH

    ON_SYS_DYNAMIC_DEVICE_INIT(OnSysDynamicDeviceInit);
    ON_SYS_DYNAMIC_DEVICE_EXIT(OnSysDynamicDeviceExit);
    ON_W32_DEVICEIOCONTROL(OnW32Deviceiocontrol);

    END_CONTROL_DISPATCH

    return TRUE;
}


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
    CHAR   upcase;

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

            if( *Name >= 'a' && *Name <= 'z' )
                upcase = *Name - 'a' + 'A';
            else
                upcase = *Name;

            //
            // See if this substring matches
            //
            if( *Pattern == upcase || *Name == '*' ) {

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

        if( *Name >= 'a' && *Name <= 'z' )
            upcase = *Name - 'a' + 'A';
        else
            upcase = *Name;

        if( *Pattern == upcase ) {

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
// B U F F E R  M A N A G E M E N T  A N D  P R I N T  R O U T I N E S
//----------------------------------------------------------------------


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
    PLOG_BUF 	prev;
    
    //
    // Just traverse the list of allocated output buffers
    //
    while( Log ) {

        prev = Log->Next;
        PageFree( Log->Handle, 0 );
        Log = prev;
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
    MEMHANDLE hNewLog;

    //
    // If we have maxed out or haven't accessed the current Log
    // just return.
    //
    if( MaxLog == NumLog ) {

        Log->Len = 0;
        return;	
    }

    //
    // If the output buffer we currently are using is empty, just
    // use it.
    //
    if( !Log->Len ) {

        return;
    }

    //
    // Allocate a new output buffer
    //
    PageAllocate(LOGBUFPAGES, PG_SYS, 0, 0, 0, 0, NULL, PAGELOCKED, 
                 (PMEMHANDLE) &hNewLog, (PVOID) &newLog );
    if( newLog ) { 

        //
        // Allocation was successful so add the buffer to the list
        // of allocated buffers and increment the buffer count.
        //
        Log   = newLog;
        Log->Handle = hNewLog;
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
    while ( ptr->Next ) {

        ptr = (prev = ptr)->Next;
    }

    //
    // Remove the buffer from the list
    //
    if ( prev ) {

        prev->Next = NULL;    
    }
    NumLog--;
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

    //
    // Traverse the list of output buffers
    //
    current = Log->Next;
    while( current ) {

        //
        // Free the buffer
        //
        next = current->Next;

        PageFree( current->Handle, 0 );

        current = next;
    }

    // 
    // Move the output pointer in the buffer that's being kept
    // the start of the buffer.
    // 
    Log->Len = 0;
    Log->Next = NULL;
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
    PENTRY		Entry;
    ULONG		len;
    va_list		arg_ptr;
    static CHAR text[MAXPATHLEN];
    DWORD       timehi;
    DWORD       time, date;

    //
    // If no filtering is desired, don't bother
    //
    if( !FilterOn ) {
 
        return;
    }

    //
    // Lock the output buffer.
    //
    Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );

    //
    // Vsprintf to determine length of the buffer
    //
    _asm cld;
    va_start( arg_ptr, format );
    len = vsprintf( text, format, arg_ptr );
    va_end( arg_ptr );

    //
    // Only log it if it passes the filtes
    //
    if( ApplyFilters( text )) {

        //
        // If the current output buffer is near capacity, move to a new
        // output buffer
        //
        if( (ULONG) (Log->Len + len + sizeof(ENTRY) +1) >= LOGBUFSIZE ) {
	
            RegmonNewLog();
        }

        //
        // Extract the sequence number and Log it
        //
        dprintf("%s\n", text );
        Entry = (void *)(Log->Data+Log->Len);
        _asm cld;
        memcpy( Entry->text, text, len+1 );
        Entry->time.u.HighPart = IFSMgr_Get_DOSTime( &Entry->time.u.LowPart );

        //
        // We calculate milliseconds to get around a bug in 
        // IFSMgr_Get_DOSTime 
        //
        time = VTD_Get_Date_And_Time( &date );
        Entry->time.u.LowPart = time - ((Entry->time.u.HighPart >> 11)& 0x1F)*60*60*1000 - 
            ((Entry->time.u.HighPart >> 5) & 0x3F)*60*1000 - 
            ((Entry->time.u.HighPart & 0x1F)*2000);

        VTD_Get_Real_Time( &Entry->perftime.u.HighPart, &Entry->perftime.u.LowPart );
        Entry->seq = Sequence++;
        Entry->perftime.u.HighPart -= StartTime.u.HighPart;
        Entry->perftime.u.LowPart  -= StartTime.u.LowPart;

        //
        // Log the length of the string, plus 1 for the terminating
        // NULL  
        //   
        Log->Len += (Entry->text - (PCHAR) Entry) + len + 1;
    }

    //
    // Release the output buffer lock
    //
    Signal_Semaphore( LogMutex );
}

//----------------------------------------------------------------------
//       H A S H   T A B L E   M A N A G E M E N T
//----------------------------------------------------------------------


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
    PHASH_ENTRY		hashEntry, nextEntry;
    ULONG		i;
  
    //
    // First, free the hash table entries
    //
    for( i = 0; i < NUMHASH; i++ ) {

        hashEntry = HashTable[i];

        while( hashEntry ) {
            nextEntry = hashEntry->Next;
            HeapFree( hashEntry->FullName, 0 );
            HeapFree( hashEntry, 0 );
            hashEntry = nextEntry;
        }
    }
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
    HKEY hkey, 
    PCHAR fullname 
    )
{
    PHASH_ENTRY     newEntry;

    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );

    //
    // Find or allocate an entry to use
    //
    newEntry = HeapAllocate( sizeof(HASH_ENTRY) + strlen(fullname)+1, 0 );

    //
    // Initialize the new entry.
    //
    if( newEntry ) {

        newEntry->hkey = hkey;
        newEntry->Next = HashTable[ HASHOBJECT(hkey) ];
        HashTable[ HASHOBJECT(hkey) ] = newEntry;	
        strcpy( newEntry->FullName, fullname );
    }
    Signal_Semaphore( HashMutex );
}

//----------------------------------------------------------------------
//
// RegmonFreeHashEntry
//
// When we see a file close, we can free the string we had associated
// with the fileobject being closed since we know it won't be used
// again.
//
//----------------------------------------------------------------------
VOID 
RegmonFreeHashEntry( 
    HKEY hkey 
    )
{
    PHASH_ENTRY		hashEntry, prevEntry;

    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );

    //
    // Look-up the entry.
    //
    hashEntry = HashTable[ HASHOBJECT( hkey ) ];
    prevEntry = NULL;

    while( hashEntry && hashEntry->hkey != hkey ) {

        prevEntry = hashEntry;
        hashEntry = hashEntry->Next;
    }
  
    //  
    // If we fall of the hash list without finding what we're looking
    // for, just return.
    //
    if( !hashEntry ) {

        Signal_Semaphore( HashMutex );
        return;
    }

    //
    // Got it! Remove it from the list
    //
    if( prevEntry ) {

        prevEntry->Next = hashEntry->Next;

    } else {

        HashTable[ HASHOBJECT( hkey )] = hashEntry->Next;
    }

    //
    // Free the memory associated with it
    //
    HeapFree( hashEntry, 0 );
    Signal_Semaphore( HashMutex );
}

//----------------------------------------------------------------------
//       P A T H  A N D  P R O C E S S  N A M E  R O U T I N E S
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// ErrorString
//
// Returns the string form of an error code.
//
//----------------------------------------------------------------------
PCHAR 
ErrorString( 
    DWORD retval 
    )
{
    //
    // Before transating, apply error filter
    //
    if( retval == ERROR_SUCCESS && !FilterDef.logsuccess ) return NULL;
    if( retval != ERROR_SUCCESS && !FilterDef.logerror   ) return NULL;

    //
    // Passed filter, so log it
    //
    switch( retval ) {
    case ERROR_SUCCESS:
        return "SUCCESS";
    case ERROR_KEY_DELETED:
        return "KEYDELETED";
    case ERROR_BADKEY:
        return "BADKEY";
    case ERROR_REGISTRY_IO_FAILED:
        return "IOFAILED";
    case ERROR_REGISTRY_CORRUPT:
        return "CORRUPT";
    case ERROR_BADDB:
        return "BADDB";
    case ERROR_OUTOFMEMORY:
        return "OUTOFMEM";
    case ERROR_ACCESS_DENIED:
        return "ACCDENIED";
    case ERROR_FILE_NOT_FOUND:
        return "NOTFOUND";
    case ERROR_NO_MORE_ITEMS:
        return "NOMORE";
    case ERROR_MORE_DATA:
        return "MOREDATA";
    default:
        sprintf(errstring, "%x\n", retval );
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

        HeapFree( IncludeFilters[i], 0 );
    }
    for( i = 0; i < NumExcludeFilters; i++ ) {

        HeapFree( ExcludeFilters[i], 0 );
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

            FilterArray[ *NumFilters ] = 
                HeapAllocate( filterLength + 1 + 2*sizeof('*'), 0 );

            saveChar = *(filterStart + filterLength );
            *(filterStart + filterLength) = 0;
            sprintf( FilterArray[ *NumFilters ], "%s%s%s",
                     *filterStart == '*' ? "" : "*",
                     filterStart,
                     *(filterStart + filterLength - 1 ) == '*' ? "" : "*" );
            *(filterStart + filterLength) = saveChar;
            (*NumFilters)++;
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
    Wait_Semaphore( FilterMutex, BLOCK_SVC_INTS );

    RegmonFreeFilters();

    //
    // Create new filter arrays
    //
    MakeFilterArray( FilterDef.includefilter,
                     IncludeFilters, &NumIncludeFilters );
    MakeFilterArray( FilterDef.excludefilter,
                     ExcludeFilters, &NumExcludeFilters );
    Signal_Semaphore( FilterMutex );
}


//----------------------------------------------------------------------
//
// GetProcess
//
// Retrieves the process name.
//
//----------------------------------------------------------------------
PCHAR
GetProcess( 
    PCHAR ProcessName 
    )
{
    PVOID       CurProc;
    PVOID       ring3proc;
    char        *name;

    //
    // Get the ring0 process pointer.
    //
    CurProc = VWIN32_GetCurrentProcessHandle();
    if( !CurProc ) {

        strcpy( ProcessName, "System" );
        return ProcessName;
    }
  
    //
    // Now, map the ring3 PCB 
    //
    ring3proc = (PVOID) SelectorMapFlat( Get_Sys_VM_Handle(), 
                                         (DWORD) (*(PDWORD) ((char *) CurProc + 0x38)) | 0x7, 0 );
    if( ring3proc == (PVOID) -1 ) {

        strcpy( ProcessName, "???");
    } else {

        //
        // copy out the process name (max 8 characters)
        //
        name = ((char *)ring3proc) + 0xF2;
        if( name[0] >= 'A' && name[0] < 'z' ) {

            strcpy( ProcessName, name );
            ProcessName[8] = 0;
        } else {

            strcpy( ProcessName, "???" );
        }
    }
    return ProcessName;
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
    PCHAR fullname 
    )
{
    ULONG    i;

    //   
    // If it matches the exclusion string, do not log it
    //
    Wait_Semaphore( FilterMutex, BLOCK_SVC_INTS );
    for( i = 0; i < NumExcludeFilters; i++ ) {

        if( MatchWithPattern( ExcludeFilters[i], fullname ) ) {

            Signal_Semaphore( FilterMutex );
            return FALSE;
        }
    }
 
    //
    // If it matches an include filter then log it
    //
    for( i = 0; i < NumIncludeFilters; i++ ) {

        if( MatchWithPattern( IncludeFilters[i], fullname )) {

            Signal_Semaphore( FilterMutex );
            return TRUE;
        }
    }

    //
    // It didn't match any include filters so don't log
    //
    Signal_Semaphore( FilterMutex );
    return FALSE;
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
    HKEY hKey, 
    PCHAR lpszSubKey, 
    PCHAR lpszValue, 
    PCHAR fullname 
    )
{
    PHASH_ENTRY		hashEntry;
    CHAR                tmpkey[16];

    //
    // See if we find the key in the hash table
    //
    fullname[0] = 0;
    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );

    hashEntry = HashTable[ HASHOBJECT( hKey ) ];
    while( hashEntry && hashEntry->hkey != hKey ) {

        hashEntry = hashEntry->Next;
    }

    Signal_Semaphore( HashMutex );

    if( hashEntry ) {

        strcpy( fullname, hashEntry->FullName );

    } else {

        //
        // Okay, make a name
        //
        switch( hKey ) {

        case HKEY_CLASSES_ROOT:
            strcat(fullname, "HKCR");
            break;

        case HKEY_CURRENT_USER:
            strcat(fullname, "HKCU");
            break;

        case HKEY_LOCAL_MACHINE:
            strcat(fullname, "HKLM");
            break;

        case HKEY_USERS:
            strcat(fullname, "HKU");
            break;

        case HKEY_CURRENT_CONFIG:
            strcat(fullname, "HKCC");
            break;

        case HKEY_DYN_DATA:
            strcat(fullname, "HKDD");
            break;

        default:

            //
            // We will only get here if key was created before we loaded
            //
            sprintf( tmpkey, "0x%X", hKey );					
            strcat(fullname, tmpkey );
            break;
        }
    }

    //
    // Append subkey and value, if they are there
    //
    if( lpszSubKey ) {

        if( lpszSubKey[0] ) {

            strcat( fullname, "\\" );
            strcat( fullname, lpszSubKey );
        }
    }

    if( lpszValue ) {

        if( lpszValue[0] ) {

            strcat( fullname, "\\" );
            strcat( fullname, lpszValue );
        }
    }
}

//----------------------------------------------------------------------
// REGISTRY HOOKS
//
// All these hooks do essentially the same thing: dump API-specific
// info into the data buffer, call the original handler, and then
// dump any return information.
//----------------------------------------------------------------------
LONG 
HookRegOpenKey( 
    HKEY hkey, 
    PCHAR lpszSubKey,
    PHKEY phkResult
    ) 
{
    LONG     retval;
    CHAR     fullname[NAMELEN], data[DATASIZE], process[PROCESSLEN];

    GetFullName( hkey, lpszSubKey, NULL, fullname );
    retval = RealRegOpenKey( hkey, lpszSubKey, phkResult );
    data[0] = 0;
    if( retval == ERROR_SUCCESS ) {

        RegmonFreeHashEntry( *phkResult );
        RegmonLogHash( *phkResult, fullname );
        sprintf(data,"hKey: 0x%X", *phkResult );
    }  
    if( ErrorString( retval ) && FilterDef.logaux ) {

        LogRecord( "%s\tOpenKey\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data);
    }
    return retval;
}

LONG 
HookRegCreateKey( 
    HKEY hkey, 
    PCHAR lpszSubKey, 
    PHKEY phkResult
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], data[DATASIZE], process[PROCESSLEN];

    GetFullName( hkey, lpszSubKey, NULL, fullname );
    retval = RealRegCreateKey( hkey, lpszSubKey, phkResult );
    data[0] = 0;
    if( retval == ERROR_SUCCESS ) {

        RegmonFreeHashEntry( *phkResult );
        RegmonLogHash( *phkResult, fullname );
        sprintf(data,"hKey: 0x%X", *phkResult );
    }

    if( ErrorString( retval ) && FilterDef.logwrites ) {

        LogRecord( "%s\tCreateKey\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data);
    }
    return retval;
}

LONG 
HookRegDeleteKey( 
    HKEY hkey, 
    PCHAR lpszSubKey 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hkey, lpszSubKey, NULL, fullname );  
    retval = RealRegDeleteKey( hkey, lpszSubKey );

    if( ErrorString( retval ) && FilterDef.logwrites ) {

        LogRecord( "%s\tDeleteKey\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ));
    }
    return retval;
}

LONG 
HookRegDeleteValue( 
    HKEY hkey, 
    PCHAR lpszSubKey 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hkey, lpszSubKey, NULL, fullname );  
    retval = RealRegDeleteValue( hkey, lpszSubKey );

    if( ErrorString( retval ) && FilterDef.logwrites ) {

        LogRecord( "%s\tDeleteValue\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ));
    }
    return retval;
}

LONG 
HookRegCloseKey(
    HKEY hkey
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hkey, NULL, NULL, fullname );  
    retval = RealRegCloseKey( hkey );

    if( ErrorString( retval ) && FilterDef.logaux ) {

        LogRecord( "%s\tCloseKey\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ));
    }
    RegmonFreeHashEntry( hkey );
    return retval;
}

LONG 
HookRegFlushKey( 
    HKEY hkey 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hkey, NULL, NULL, fullname );  
    retval = RealRegFlushKey( hkey );

    if( ErrorString( retval ) && FilterDef.logaux ) {

        LogRecord( "%s\tFlushKey\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ));
    }
    return retval;
}

LONG 
HookRegEnumKey(
    HKEY hkey, 
    DWORD iSubKey, 
    PCHAR lpszName, 
    DWORD cchName
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];
  
    GetFullName( hkey, NULL, NULL, fullname );
    retval = RealRegEnumKey( hkey, iSubKey, lpszName, cchName );

    if( ErrorString( retval ) && FilterDef.logreads ) {
  
        LogRecord( "%s\tEnumKey\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, 
                   ErrorString( retval ),
                   retval == ERROR_SUCCESS ? lpszName : "");  
    }
    return retval;
}

LONG 
HookRegEnumValue(
    HKEY hkey, 
    DWORD iValue, 
    PCHAR lpszValue, 
    PDWORD lpcchValue, 
    PDWORD lpdwReserved, 
    PDWORD lpdwType, 
    PBYTE lpbData, 
    PDWORD lpcbData
    ) 
{
    LONG      retval;
    int       i, len;
    CHAR      fullname[NAMELEN], data[DATASIZE], tmp[2*BINARYLEN], process[PROCESSLEN];

    GetFullName( hkey, NULL, NULL, fullname );
    retval = RealRegEnumValue( hkey, iValue, lpszValue, lpcchValue,
                               lpdwReserved, lpdwType, lpbData, lpcbData );
    data[0] = 0;
    if( retval == ERROR_SUCCESS && lpbData && *lpcbData ) {

        strcat( data, lpszValue );
        strcat( data, ": ");
        if( !lpdwType || *lpdwType == REG_BINARY ) {

            if( *lpcbData > BINARYLEN ) len = BINARYLEN;
            else len = *lpcbData;

            for( i = 0; i < len; i++ ) {

                sprintf( tmp, "%X ", lpbData[i]);
                strcat( data, tmp );
            }

            if( *lpcbData > BINARYLEN) strcat( data, "...");

        } else if( *lpdwType == REG_SZ ) {

            strcat( data, "\"");
            strncat( data, lpbData, STRINGLEN );
            strcat( data, "\"");

        } else if( *lpdwType == REG_DWORD ) {

            sprintf( tmp, "0x%X", *(PDWORD) lpbData );
            strcat( data, tmp );
        }
    }

    if( ErrorString( retval ) && FilterDef.logreads ) {

        LogRecord( "%s\tEnumValue\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ),
                   data ); 
    }
    return retval;
}

LONG 
HookRegQueryInfoKey( 
    HKEY hKey, 
    PCHAR lpszClass, 
    PDWORD lpcchClass,  
    DWORD lpdwReserved, 
    PDWORD lpcSubKeys, 
    PDWORD lpcchMaxSubKey, 
    PDWORD  pcchMaxClass, 
    PDWORD lpcValues, 
    PDWORD lpcchMaxValueName, 
    PDWORD lpcbMaxValueData, 
    PDWORD lpcbSecurityDescriptor, 
    PFILETIME lpftLastWriteTime
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hKey, NULL, NULL, fullname );
    retval = RealRegQueryInfoKey( hKey, lpszClass, lpcchClass, lpdwReserved, 
                                  lpcSubKeys, lpcchMaxSubKey, pcchMaxClass,
                                  lpcValues, lpcchMaxValueName,
                                  lpcbMaxValueData,
                                  lpcbSecurityDescriptor,
                                  lpftLastWriteTime );
    if( ErrorString( retval ) &&
        FilterDef.logreads ) {

        if( retval == ERROR_SUCCESS ) {
            LogRecord( "%s\tQueryKey\t%s\t%s\tKeys: %d Values: %d", 
                       GetProcess( process ), fullname, ErrorString( retval ), 
                       lpcSubKeys ? *lpcSubKeys : -1, 
                       lpcValues ? *lpcValues : -1 );
        } else
            LogRecord( "%s\tQueryKey\t%s\t%s", 
                       GetProcess( process ), fullname, ErrorString( retval ));  
    }
    return retval;
}

LONG 
HookRegQueryValue( 
    HKEY hkey, 
    PCHAR lpszSubKey, 
    PCHAR lpszValue, 
    PLONG pcbValue 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], data[DATASIZE], process[PROCESSLEN];

    GetFullName( hkey, lpszSubKey, "(Default)", fullname );
    retval = RealRegQueryValue( hkey, lpszSubKey, lpszValue, pcbValue );
    data[0] = 0;
    if( retval == ERROR_SUCCESS && lpszValue && *pcbValue ) {

        strcpy( data, "\"");
        strncat( data, lpszValue, STRINGLEN );
        if( strlen( lpszValue ) > STRINGLEN ) 
            strcat( data, "..." );
        strcat( data, "\"");
    }
    if( ErrorString( retval ) && FilterDef.logreads ) {

        LogRecord( "%s\tQueryValue\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data);
    }
    return retval;
}

LONG 
HookRegQueryValueEx( 
    HKEY hkey, 
    PCHAR lpszValueName, 
    PDWORD lpdwReserved, 
    PDWORD lpdwType, 
    PBYTE lpbData, 
    PDWORD lpcbData 
    ) 
{
    LONG      retval;
    int       i, len;
    CHAR      fullname[NAMELEN], data[DATASIZE], tmp[2*BINARYLEN], process[PROCESSLEN];
  
    GetFullName( hkey, NULL, lpszValueName, fullname );
    retval = RealRegQueryValueEx( hkey, lpszValueName, lpdwReserved, 
                                  lpdwType, lpbData, lpcbData );
    data[0] = 0;
    if( retval == ERROR_SUCCESS && lpbData ) {

        if( !lpdwType || *lpdwType == REG_BINARY ) {

            if( *lpcbData > BINARYLEN ) len = BINARYLEN;
            else len = *lpcbData;

            for( i = 0; i < len; i++ ) {

                sprintf( tmp, "%X ", lpbData[i]);
                strcat( data, tmp );
            }

            if( *lpcbData > BINARYLEN) strcat( data, "...");

        } else if( *lpdwType == REG_SZ ) {

            strcpy( data, "\"");
            strncat( data, lpbData, STRINGLEN );
            if( strlen( lpbData ) > STRINGLEN ) 
                strcat( data, "..." );
            strcat( data, "\"");

        } else if( *lpdwType == REG_DWORD ) {

            sprintf( data, "0x%X", *(PDWORD) lpbData );
        }
    } 

    if( ErrorString( retval ) &&  FilterDef.logreads) {

        LogRecord( "%s\tQueryValueEx\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data);  
    }
    return retval;
}

LONG 
HookRegSetValue( 
    HKEY hkey, 
    PCHAR lpszSubKey, 
    DWORD fdwType, 
    PCHAR lpszData, 
    DWORD cbData 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], data[DATASIZE], process[PROCESSLEN];
  
    GetFullName( hkey, lpszSubKey, "(Default)", fullname );
    retval = RealRegSetValue( hkey, lpszSubKey, fdwType, lpszData, cbData );
    data[0] = 0;
    if( lpszData ) {

        strcpy( data,"\"");
        strncat(data, lpszData, STRINGLEN );
        if( strlen( lpszData ) > STRINGLEN ) 
            strcat( data, "..." );
        strcat( data, "\"");
    }
    if( ErrorString( retval ) && FilterDef.logwrites) {

        LogRecord( "%s\tSetValue\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data );  
    }
    return retval;
}

LONG 
HookRegSetValueEx( 
    HKEY hkey, 
    PCHAR lpszValueName, 
    DWORD lpdwReserved, 
    DWORD fdwType, 
    PBYTE lpbData, 
    DWORD lpcbData 
    ) 
{
    LONG      retval;
    int       i, len;
    CHAR      fullname[NAMELEN], data[DATASIZE], tmp[2*BINARYLEN], process[PROCESSLEN];
  
    GetFullName( hkey, NULL, lpszValueName, fullname );
    retval = RealRegSetValueEx( hkey, lpszValueName, lpdwReserved, 
                                fdwType, lpbData, lpcbData );
    data[0] = 0;
    if( fdwType == REG_SZ ) {

        strcpy( data, "\"");
        strncat( data, lpbData, STRINGLEN );
        if( strlen( lpbData ) > STRINGLEN ) 
            strcat( data, "..." );
        strcat( data, "\"");

    } else if( fdwType == REG_BINARY ) {

        if( lpcbData > BINARYLEN ) len = BINARYLEN;
        else len = lpcbData;

        for( i = 0; i < len; i++ ) {

            sprintf( tmp, "%X ", lpbData[i]);
            strcat( data, tmp );
        }

        if( lpcbData > BINARYLEN) strcat( data, "...");

    } else if( fdwType == REG_DWORD ) {

        sprintf( data, "0x%X", *(PDWORD) lpbData );
    }

    if( ErrorString( retval ) && FilterDef.logwrites) {

        LogRecord( "%s\tSetValueEx\t%s\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ), data );  
    }
    return retval;
}

LONG 
HookRegRemapPreDefKey( 
    HKEY hkey, 
    HKEY hRootKey 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hkey, NULL, NULL, fullname );
    retval = RealRegRemapPreDefKey( hkey, hRootKey );

    if( ErrorString( retval )) {

        LogRecord( "%s\tRemapPreKey\t%s\t%s\tRoot: %s", 
                   GetProcess( process ), fullname, ErrorString( retval ),
                   hRootKey == HKEY_CURRENT_USER ? "HKCU" : "HKCC" );
    }
    return retval;
}

LONG 
HookRegQueryMultipleValues( 
    HKEY hKey, 
    PVALENT pValent, 
    DWORD dwNumVals, 
    PCHAR pValueBuf, 
    PDWORD pTotSize 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    GetFullName( hKey, NULL, NULL, fullname );
    retval = RealRegQueryMultipleValues( hKey, pValent,
                                         dwNumVals, pValueBuf, pTotSize );

    if( ErrorString( retval ) && FilterDef.logreads) {

        LogRecord( "%s\tQueryMultVal\t%s\t%s", 
                   GetProcess( process ), fullname, ErrorString( retval ));  
    }
    return retval;
}

LONG 
HookRegCreateDynKey( 
    PCHAR szName, 
    PVOID KeyContext, 
    PVOID pInfo, 
    PVOID pValList, 
    DWORD dwNumVals, 
    PVMMHKEY pKeyHandle 
    ) 
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    sprintf(fullname, "DYNDAT\\%s", szName );
    retval = RealRegCreateDynKey( szName, KeyContext, pInfo, pValList,
                                  dwNumVals, pKeyHandle );

    if( ErrorString( retval ) && FilterDef.logreads) {

        LogRecord( "%s\tCreateDynKey\t%s\t%s\thKey: 0x%X", 
                   GetProcess( process ), fullname, ErrorString( retval ),
                   *pKeyHandle ); 
    }
    if( retval == ERROR_SUCCESS ) {

        RegmonLogHash( *pKeyHandle, fullname );
    }
    return retval;
}

LONG 
__stdcall HookWin32RegQueryInfoKey( 
    volatile PCLIENT_STRUCT pClientRegs, 
    DWORD Dummy2, 
    HKEY hKey, 
    PDWORD lpcSubKeys, 
    PDWORD lpcchMaxSubKey, 
    PDWORD lpcValues, 
    PDWORD lpcchMaxValueName, 
    PDWORD lpcbMaxValueData 
    )
{
    LONG      retval;
    CHAR      fullname[NAMELEN], process[PROCESSLEN];

    //
    // NOTE: this special hook is needed because Win95 has a bug where
    // the Win32 call to RegQueryInfoKey gets routed to VMM's Win32 
    // service table, but the service table handler does *not* call the
    // VMM RegQueryInfoKey entry point. Therefore, we have to hook the
    // VMM Win32 service as a special case.
    //
    GetFullName( hKey, NULL, NULL, fullname );

    //
    // The return code is Logd in the client registers
    //
    RealWin32RegQueryInfoKey( pClientRegs, Dummy2, 
                              hKey, lpcSubKeys, lpcchMaxSubKey, 
                              lpcValues, lpcchMaxValueName,
                              lpcbMaxValueData );
    retval = pClientRegs->CRS.Client_EAX;

    if( ErrorString( retval ) && FilterDef.logreads) {

        if( retval == ERROR_SUCCESS ) {
            LogRecord( "%s\tQueryKey\t%s\t%s\tKeys: %d Values: %d", 
                       GetProcess( process ), fullname, ErrorString( retval ), 
                       lpcSubKeys ? *lpcSubKeys : -1, 
                       lpcValues ? *lpcValues : -1 );
        } else
            LogRecord( "%s\tQueryKey\t%s\t%s", 
                       GetProcess( process ), fullname, ErrorString( retval ));  
    }
    return retval;
}

//----------------------------------------------------------------------
//
// OnSysDynamicDeviceInit
//
// Dynamic init. Hook all registry related VxD APIs.
//
//----------------------------------------------------------------------
BOOL 
OnSysDynamicDeviceInit(
    VOID
    )
{
    int i;
    PDDB vmmDDB;
    MEMHANDLE hLog;

    //
    // Initialize semaphores
    //
    LogMutex = Create_Semaphore(1);
    HashMutex  = Create_Semaphore(1);
    FilterMutex  = Create_Semaphore(1);

    //
    // Zero hash table
    //
    for(i = 0; i < NUMHASH; i++ ) HashTable[i] = NULL;

    //
    // Allocate first output buffer
    //
    PageAllocate(LOGBUFPAGES, PG_SYS, 0, 0, 0, 0, NULL, PAGELOCKED, 
                 (PMEMHANDLE) &hLog, (PVOID) &Log );
    Log->Handle = hLog;
    Log->Len = 0;
    Log->Next = NULL;
    NumLog = 1;

    //
    // Hook all registry routines
    //
    RealRegOpenKey = Hook_Device_Service_C(___RegOpenKey, HookRegOpenKey,
                                           &ROKThunk );
    RealRegCloseKey = Hook_Device_Service_C(___RegCloseKey, HookRegCloseKey,
                                            &RCKThunk );
    RealRegCreateKey = Hook_Device_Service_C(___RegCreateKey, HookRegCreateKey,
                                             &RCRKThunk );
    RealRegDeleteKey = Hook_Device_Service_C(___RegDeleteKey, HookRegDeleteKey,
                                             &RDKThunk );
    RealRegDeleteValue = Hook_Device_Service_C(___RegDeleteValue, 
                                               HookRegDeleteValue,
                                               &RDVThunk );
    RealRegEnumKey   = Hook_Device_Service_C(___RegEnumKey,
                                             HookRegEnumKey,
                                             &REKThunk );
    RealRegEnumValue = Hook_Device_Service_C(___RegEnumValue, 
                                             HookRegEnumValue,
                                             &REVThunk );
    RealRegFlushKey  = Hook_Device_Service_C(___RegFlushKey,
                                             HookRegFlushKey,
                                             &RFKThunk );
    RealRegQueryInfoKey  = Hook_Device_Service_C(___RegQueryInfoKey,
                                                 HookRegQueryInfoKey,
                                                 &RQIKThunk );
    RealRegQueryValue = Hook_Device_Service_C(___RegQueryValue,
                                              HookRegQueryValue,
                                              &RQVThunk );
    RealRegQueryValueEx = Hook_Device_Service_C(___RegQueryValueEx,
                                                HookRegQueryValueEx,
                                                &RQVEThunk );
    RealRegSetValue = Hook_Device_Service_C(___RegSetValue,
                                            HookRegSetValue,
                                            &RSVThunk );
    RealRegSetValueEx = Hook_Device_Service_C(___RegSetValueEx,
                                              HookRegSetValueEx,
                                              &RSVEThunk );
    RealRegRemapPreDefKey = Hook_Device_Service_C(___RegRemapPreDefKey,
                                                  HookRegRemapPreDefKey,
                                                  &RRPDKThunk );
    RealRegQueryMultipleValues = Hook_Device_Service_C(___RegQueryMultipleValues,
                                                       HookRegQueryMultipleValues,
                                                       &RQMVThunk );
    RealRegCreateDynKey = Hook_Device_Service_C(___RegCreateDynKey,
                                                HookRegCreateDynKey,
                                                &RCDKThunk );

    //
    // We have to hook the Win32 service for query info key
    // (isn't win9x just great?)
    //
    vmmDDB = Get_DDB( VMM_DEVICE_ID, "" );
    VMMWin32ServiceTable = (PDWORD) vmmDDB->DDB_Win32_Service_Table;
    RealWin32RegQueryInfoKey = (PVOID) VMMWin32ServiceTable[ VMMWIN32QUERYINFOKEY ];
    VMMWin32ServiceTable[ VMMWIN32QUERYINFOKEY ] = (DWORD) HookWin32RegQueryInfoKey;

    return TRUE;
}

//----------------------------------------------------------------------
//
// OnSysDynamicDeviceExit
//
// Dynamic exit. Unhook everything.
//
//----------------------------------------------------------------------
BOOL 
OnSysDynamicDeviceExit(
    VOID
    )
{
    Unhook_Device_Service_C(___RegOpenKey, &ROKThunk );
    Unhook_Device_Service_C(___RegCloseKey, &RCKThunk );
    Unhook_Device_Service_C(___RegCreateKey, &RCRKThunk );
    Unhook_Device_Service_C(___RegDeleteKey, &RDKThunk );
    Unhook_Device_Service_C(___RegDeleteValue, &RDVThunk );
    Unhook_Device_Service_C(___RegEnumKey, &REKThunk );
    Unhook_Device_Service_C(___RegEnumValue, &REVThunk );
    Unhook_Device_Service_C(___RegFlushKey, &RFKThunk );
    Unhook_Device_Service_C(___RegQueryInfoKey, &RQIKThunk );
    Unhook_Device_Service_C(___RegQueryValue, &RQVThunk );
    Unhook_Device_Service_C(___RegQueryValueEx, &RQVEThunk );
    Unhook_Device_Service_C(___RegSetValue, &RSVThunk );
    Unhook_Device_Service_C(___RegSetValueEx, &RSVEThunk );
    Unhook_Device_Service_C(___RegRemapPreDefKey, &RRPDKThunk );
    Unhook_Device_Service_C(___RegQueryMultipleValues, &RQMVThunk );
    Unhook_Device_Service_C(___RegCreateDynKey, &RCDKThunk );
    VMMWin32ServiceTable[ VMMWIN32QUERYINFOKEY ] = (DWORD) RealWin32RegQueryInfoKey;

    //
    // Free all memory
    //
    RegmonHashCleanup();
    RegmonFreeLog();
    RegmonFreeFilters();
    return TRUE;
}

//----------------------------------------------------------------------
//
// OnW32Deviceiocontrol
//
// Interface with the GUI.
//
//----------------------------------------------------------------------
DWORD 
OnW32Deviceiocontrol(
    PIOCTLPARAMS p
    )
{
    PLOG_BUF        old;
    static BOOLEAN  connected = FALSE;

    switch( p->dioc_IOCtlCode ) {

    case 0:
        return 0;

    case IOCTL_REGMON_ZEROSTATS:
  
        Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );
        while ( Log->Next )  {
 
            //
            // Release the next entry.
            //
            old = Log->Next;
            Log->Next = old->Next;
            Signal_Semaphore( LogMutex );
            PageFree( old->Handle, 0 );
            Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );
            NumLog--;
        }
        Log->Len = 0;
        Sequence = 0;
        VTD_Get_Real_Time( &StartTime.u.HighPart, &StartTime.u.LowPart );
        
        Signal_Semaphore( LogMutex );
        return 0;

    case IOCTL_REGMON_GETSTATS:
  
        //
        // Copy buffer into user space.
        Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );
        if ( LOGBUFSIZE > p->dioc_cbOutBuf ) {

            //
            // Buffer is too small. Return error.
            //
            Signal_Semaphore( LogMutex );

            return 1;

        } else if ( Log->Len  ||  Log->Next ) {

            //
            // Switch to a new buffer.
            //
            RegmonNewLog();

            //
            // Fetch the oldest buffer to give to caller.
            //
            old = RegmonOldestLog();
            Signal_Semaphore( LogMutex );

            //
            // Copy it into the caller's buffer.
            //
            memcpy( p->dioc_OutBuf, old->Data, old->Len );

            //
            // Return length of copied info.
            //
            *p->dioc_bytesret = old->Len;

            //   
            // Deallocate the buffer.
            //
            PageFree( old->Handle, 0 );
            NumLog--;

        } else {

            //
            // There is no unread data.
            //
            Signal_Semaphore( LogMutex );
            *p->dioc_bytesret = 0;
        }
        return 0;

    case IOCTL_REGMON_UNHOOK:

        FilterOn = FALSE;
        return 0;

    case IOCTL_REGMON_HOOK:

        FilterOn = TRUE;
        return 0;

    case IOCTL_REGMON_SETFILTER:

        FilterDef = * (PFILTER) p->dioc_InBuf;
        RegmonUpdateFilters();
        return 0;
    }
    return 0;
}
