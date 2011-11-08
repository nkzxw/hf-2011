//======================================================================
//
// FILEMON.c - main module for VxD FILEMON
//
// SysInternals - www.sysinternals.com
// Copyright (C) 1996-2000 Mark Russinovich and Bryce Cogswell
//
//======================================================================
#define   DEVICE_MAIN
#include  <vtoolsc.h>
#include  "..\exe\ioctlcmd.h"
#include  "filemon.h"
#undef    DEVICE_MAIN

//----------------------------------------------------------------------
//                     G L O B A L   D A T A 
//----------------------------------------------------------------------

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
// Real service pointers with the hook thunks
//
ppIFSFileHookFunc       PrevIFSHookProc;

//
// Hash table data 
//
PHASH_ENTRY		        HashTable[NUMHASH];

//
// Buffer data
//
PLOG_BUF		        Log 	 = NULL;
ULONG			        Sequence = 0;

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

//----------------------------------------------------------------------
//                    F O R W A R D S
//----------------------------------------------------------------------

BOOLEAN
ApplyFilters(
    PCHAR Text
    );

//----------------------------------------------------------------------
//                   V X D  C O N T R O L
//----------------------------------------------------------------------

//
// Device declaration
//
Declare_Virtual_Device(FILEMON)

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
__cdecl ControlDispatcher(
    DWORD dwControlMessage,
    DWORD EBX,
    DWORD EDX,
    DWORD ESI,
    DWORD EDI,
    DWORD ECX
    )
{
    START_CONTROL_DISPATCH

        ON_W32_DEVICEIOCONTROL(OnW32Deviceiocontrol);
        ON_SYS_DYNAMIC_DEVICE_INIT(OnSysDynamicDeviceInit);
        ON_SYS_DYNAMIC_DEVICE_EXIT(OnSysDynamicDeviceExit);

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
// FilemonNewLog
//
// Called when the current buffer has filled up. This moves us to the
// pre-allocated buffer and then allocates another buffer.
//
// Returns FALSE if another thread is already allocating a buffer.
//
//----------------------------------------------------------------------
BOOLEAN 
FilemonNewLog( VOID 
    )
{
    PLOG_BUF prev = Log, newLog;
    static busyAllocating = FALSE;
    MEMHANDLE hNewLog;

    //
    // If we have maxed out or haven't accessed the current Log
    // just return.
    //
    if( MaxLog == NumLog ) {

        Log->Len = 0;
        return TRUE;	
    }

    //
    // If the output buffer we currently are using is empty, just
    // use it, or if we are busy already allocating a buffer, return
    //
    if( !Log->Len || busyAllocating ) {

        return !busyAllocating;
    }

    //
    // Allocate a new output buffer. Release lock to prevent deadlock
    // on reentrance (allocating memory can result in file I/O)
    //
    busyAllocating = TRUE;
    dprintf("Pageallocate: num:%d\n", NumLog );
    Signal_Semaphore( LogMutex );

    PageAllocate(LOGBUFPAGES, PG_SYS, 0, 0, 0, 0, NULL, PAGELOCKED, 
                 (PMEMHANDLE) &hNewLog, (PVOID) &newLog );

    Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );
    dprintf("Pageallocate done: num:%d\n", NumLog );
    busyAllocating = FALSE;

    if( newLog ) { 

        //
        // Allocation was successful so add the buffer to the list
        // of allocated buffers and increment the buffer count.
        //
        Log       = newLog;
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
    return TRUE;
}


//----------------------------------------------------------------------
//
// FilemonOldestLog
//
// Goes through the linked list of storage buffers and returns the 
// oldest one.
//
//----------------------------------------------------------------------
PLOG_BUF 
FilemonOldestLog( 
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
// FilemonResetLog
//
// When a GUI is no longer communicating with us, but we can't unload,
// we reset the storage buffers.
//
//----------------------------------------------------------------------
VOID 
FilemonResetLog(
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
    ULONG time, 
    ULONG datetimelo,
    ULONG datetimehi,
    const char *format, 
    ... 
    )
{	
    PENTRY		Entry;
    ULONG		len;
    va_list		arg_ptr;
    static CHAR text[MAXPATHLEN*3];

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
    // Only log it if the text passes the filters
    //
    if( ApplyFilters( text )) {

        //
        // If the current output buffer is near capacity, move to a new
        // output buffer
        //
        if( (ULONG) (Log->Len + len + sizeof(ENTRY) +1) >= LOGBUFSIZE ) {

            if( !FilemonNewLog() ) {
  
                //
                // Just return if a thread is in the process
                // of allocating a buffer.
                //
                Signal_Semaphore( LogMutex );
                return;
            }
        } 

        //
        // Extract the sequence number and Log it
        //
        Entry = (void *)(Log->Data+Log->Len);
        Entry->seq = Sequence++;
        Entry->perftime.u.LowPart = time;
        Entry->perftime.u.HighPart = 0;
        Entry->datetime.u.HighPart = datetimehi;
        Entry->datetime.u.LowPart  = datetimelo;
        _asm cld;
        memcpy( Entry->text, text, len + 1 );
 
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
  
    //
    // Free the hash table entries
    //
    for( i = 0; i < NUMHASH; i++ ) {

        hashEntry = HashTable[i];
        while( hashEntry ) {
            nextEntry = hashEntry->Next;
            HeapFree( hashEntry, 0 );
            hashEntry = nextEntry;
        }
    }
}

//----------------------------------------------------------------------
//
// FilemonLogHash
//
// Logs the key and associated fullpath in the hash table.
//
//----------------------------------------------------------------------
VOID
FilemonLogHash( 
    int Drive, 
    fh_t Filenumber, 
    PCHAR Fullname 
    )
{
    PHASH_ENTRY     newEntry;

    //
    // Allocate a new entry
    //
    newEntry = HeapAllocate( sizeof(HASH_ENTRY) + strlen(Fullname)+1, 0 );
    if( !newEntry ) return;

    //
    // Initialize the new entry.
    //
    newEntry->filenumber = Filenumber;
    newEntry->drive      = Drive & 0xFF;
    strcpy( newEntry->FullName, Fullname );

    //
    // Lock the hash table and insert the new entry.
    //
    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );
    newEntry->Next = HashTable[ HASHOBJECT(Filenumber) ];
    HashTable[ HASHOBJECT(Filenumber) ] = newEntry;	
    Signal_Semaphore( HashMutex );
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
    int Drive, 
    fh_t Filenumber 
    )
{
    PHASH_ENTRY		hashEntry, prevEntry;

    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );

    //
    // Look-up the entry.
    //
    hashEntry = HashTable[ HASHOBJECT( Filenumber ) ];
    prevEntry = NULL;

    while( hashEntry && 
           hashEntry->filenumber != Filenumber && 
           hashEntry->drive != (Drive & 0xFF)) {

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

        HashTable[ HASHOBJECT( Filenumber )] = hashEntry->Next;
    }

    //
    // Free the memory associated with the name of the free entry.
    //
    HeapFree( hashEntry, 0 );
    Signal_Semaphore( HashMutex );
}

//----------------------------------------------------------------------
//    F I L T E R  A N D  P R O C E S S  N A M E  R O U T I N E S
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
    switch( retval ) {
    case ERROR_INVALID_FUNCTION:
        return "INVALIDFUNC"; 
    case ERROR_SUCCESS:
        return "SUCCESS";
    case ERROR_OUTOFMEMORY:
        return "OUTOFMEM";
    case ERROR_ACCESS_DENIED:
        return "ACCDENIED";
    case ERROR_PATH_NOT_FOUND:
        return "NOTFOUND";
    case ERROR_TOO_MANY_OPEN_FILES:
        return "TOOMANYOPEN";
    case ERROR_FILE_NOT_FOUND:
        return "NOTFOUND";
    case ERROR_NO_MORE_ITEMS:
        return "NOMORE";
    case ERROR_GEN_FAILURE:
        return "GENFAILURE";
    case ERROR_MORE_DATA:
        return "MOREDATA";
    case ERROR_INVALID_DRIVE:
        return "INVALIDDRIVE";
    case ERROR_NOT_SAME_DEVICE:
        return "DIFFERENTDEVICE";
    case ERROR_WRITE_PROTECT:
        return "WRITEPROTECTED";
    case ERROR_SHARING_VIOLATION:
        return "SHARING";
    case ERROR_BAD_UNIT:
        return "BADUNIT";
    case ERROR_NOT_READY:
        return "NOTREADY";
    case ERROR_NO_MORE_FILES:
        return "NOMORE";
    case ERROR_BAD_COMMAND:
        return "BADCOMMAND";
    case ERROR_INVALID_HANDLE:
        return "INVALIDHANDLE";
    case ERROR_DEV_NOT_EXIST:
        return "DEVDOESNOTEXIST";
    default:
        sprintf(errstring, "0x%x", retval );
        return errstring;
    }
}


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

            //
            // Conservatively allocate so that we can prepend and append
            // wildcards
            //
            FilterArray[ *NumFilters ] = 
                HeapAllocate( filterLength + 1 + 2* sizeof('*'), 0 );

            if( FilterArray[ *NumFilters ]) {

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
    Wait_Semaphore( FilterMutex, BLOCK_SVC_INTS );
    FilemonFreeFilters();

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
// wstrlen
//
// Determine the length of a wide-character string.
//
//----------------------------------------------------------------------
int 
wstrlen( 
    unsigned short *unistring 
    )
{
    int i = 0;
    int len = 0;
  
    while( unistring[i++] != 0 ) len+=2;
    return len;
}


//----------------------------------------------------------------------
//
// FilmonGetProcess
//
// Retrieves the process name.
//
//----------------------------------------------------------------------
PCHAR
FilemonGetProcess( 
    PCHAR ProcessName 
    )
{
    PVOID       CurProc;
    PVOID       ring3proc;
    char        *name;
    ULONG       i;

    //
    // Get the ring0 process pointer.
    //
    CurProc = VWIN32_GetCurrentProcessHandle();
  
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
    PCHAR Text
    )
{
    ULONG    i;

    //   
    // If it matches the exclusion string, do not log it
    //
    Wait_Semaphore( FilterMutex, BLOCK_SVC_INTS );
    for( i = 0; i < NumExcludeFilters; i++ ) {

        if( MatchWithPattern( ExcludeFilters[i], Text ) ) {

            Signal_Semaphore( FilterMutex );
            return FALSE;
        }
    }
 
    //
    // If it matches an include filter then log it
    //
    for( i = 0; i < NumIncludeFilters; i++ ) {

        if( MatchWithPattern( IncludeFilters[i], Text )) {

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
//                 P A T H   P A R S I N G
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FilemonConvertUnparsedPath
//
// Converts an unparsed unicode path to ANSI. This is only used for
// UNC paths except for the special cases of renames and findopens.
//
//----------------------------------------------------------------------
VOID
FilemonConvertUnparsedPath(
    pioreq pir,
    PCHAR fullpathname
    )
{
    _QWORD  result;

    UniToBCS( fullpathname, pir->ir_upath, wstrlen( pir->ir_upath ),
              MAXPATHLEN, BCS_WANSI, &result );
    fullpathname[ result.ddLower ] = 0;
}


//----------------------------------------------------------------------
//
// FilemonConvertParsedPath
// 
// Converts a parsed unicode path to ANSI.
//
//----------------------------------------------------------------------
VOID
FilemonConvertParsedPath(
    int drive,
    path_t ppath,
    int codepage,
    PCHAR fullpathname 
    )
{
    int  i = 0;
    _QWORD  result;

    if( drive != 0xFF ) {

        //
        // Its a volume-based path
        // 
        fullpathname[0] = drive+'A'-1;
        fullpathname[1] = ':';
        i = 2;
    }
    fullpathname[i] = 0;
    UniToBCSPath( &fullpathname[i], ppath->pp_elements, 
                  MAXPATHLEN-1, codepage, &result );
    fullpathname[ i + result.ddLower ] = 0;
}


//----------------------------------------------------------------------
//
// FilemonConvertMixedPath
//
// This converts a mix of unparsed and parsed paths to ANSI. The 
// unparsed path is used for server/share, whereas the parsed
// path is used for the directory/file. Only UNC rename and findopen 
// use this.
// 
//----------------------------------------------------------------------
VOID
FilemonConvertMixedPath(
    pioreq pir,
    path_t ppath,
    int codepage,
    PCHAR fullpathname
    )
{
    int     i, slashes;
    _QWORD  result;

    UniToBCS( fullpathname, pir->ir_upath, wstrlen( pir->ir_upath ), MAXPATHLEN-1,
              codepage, &result );
    fullpathname[ result.ddLower ] = 0;

    slashes = 0;
    for( i = 0; i < result.ddLower; i++ ) {
            
        //
        // We find the 4th slash: \\Server\share\...
        //
        if( fullpathname[i] == '\\' && ++slashes == 4 ) break;
    }
    if( slashes == 4 ) {

        FilemonConvertParsedPath( 0xFF, ppath, codepage, &fullpathname[i]);
    }
}


//----------------------------------------------------------------------
//
// FilemonConvertPath
//
// Converts a unicode path name to ANSI.
//
//----------------------------------------------------------------------
PCHAR 
FilemonConvertPath( 
    CONVERT_TYPE converttype,
    int drive, 
    pioreq pir,
    int codepage,
    PCHAR fullpathname 
    )
{
    if( drive != 0xFF ) {

        //
        // Its a volume-based path
        // 
        switch( converttype ) {
        case CONVERT_RENAME_TARGET:
            FilemonConvertParsedPath( drive, pir->ir_ppath2, codepage, fullpathname );
            break;

        default:
            FilemonConvertParsedPath( drive, pir->ir_ppath, codepage, fullpathname );
            break;
        }

    } else {
        
        //
        // Its a UNC path. The parsed path doesn't include the
        // server/share, so we get that from the unparsed path.
        //
        switch( converttype ) {
        case CONVERT_STANDARD:
            FilemonConvertUnparsedPath( pir, fullpathname );
            break;

        case CONVERT_FINDOPEN:
        case CONVERT_RENAME_SOURCE:
            FilemonConvertMixedPath( pir, pir->ir_ppath, codepage, fullpathname );
            break;

        case CONVERT_RENAME_TARGET:
            FilemonConvertMixedPath( pir, pir->ir_ppath2, codepage, fullpathname );
            break;
        }
    }
    return fullpathname;
}


//----------------------------------------------------------------------
//
// FilemonGetFullPath
//
// Returns the full pathname of a file, if we can obtain one, else
// returns a handle.
//
//----------------------------------------------------------------------
PCHAR 
FilemonGetFullPath(  
    fh_t filenumber, 
    PCHAR fullname,
    int Drive, 
    int ResType, 
    int CodePage, 
    pioreq pir 
    )
{
    PHASH_ENTRY		hashEntry;
    pIFSFunc        enumFunc;
    ifsreq          ifsr;
    path_t          uniFullName;
    int             retval;

    //
    // See if we find the key in the hash table.
    //
    Wait_Semaphore( HashMutex, BLOCK_SVC_INTS );

    hashEntry = HashTable[ HASHOBJECT( filenumber ) ];

    while( hashEntry && 
           hashEntry->filenumber != filenumber &&
           hashEntry->drive != (Drive & 0xFF)) {

        hashEntry = hashEntry->Next;
    }

    Signal_Semaphore( HashMutex );

    fullname[0] = 0;
    if( hashEntry ) {

        strcpy( fullname, hashEntry->FullName );

    } else {

        //
        // File name isn't in the table, so ask the
        // underlying file system
        //
        sprintf( fullname, "0x%X", filenumber );

        uniFullName = IFSMgr_GetHeap( MAXPATHLEN * sizeof(WCHAR) + sizeof( path_t));
        if( uniFullName ) {
            
            //
            // Send a query file name request
            //
            memcpy( &ifsr, pir, sizeof( ifsreq ));
            ifsr.ifsir.ir_flags = ENUMH_GETFILENAME;
            ifsr.ifsir.ir_ppath = uniFullName;
            enumFunc = ifsr.ifs_hndl->hf_misc->hm_func[HM_ENUMHANDLE];

            retval = (*PrevIFSHookProc)(enumFunc, IFSFN_ENUMHANDLE, 
                                        Drive, ResType, CodePage, 
                                        (pioreq) &ifsr);

            if( retval == ERROR_SUCCESS ) {
                
                FilemonConvertParsedPath( Drive, uniFullName, CodePage, fullname );
                FilemonLogHash( Drive, filenumber, fullname );
            }
            IFSMgr_RetHeap( (void *) uniFullName );
        } 
    }
    return fullname;
}


//----------------------------------------------------------------------
//                     H O O K   R O U T I N E
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//
// FilemonHookProc
//
// All (most) IFS functions come through this routine for us to look
// at.
//
//----------------------------------------------------------------------
#pragma optimize("", off)
int 
_cdecl 
FilemonHookProc(
    pIFSFunc pfn, 
    int fn, 
    int Drive, 
    int ResType,
    int CodePage,
    pioreq pir
    )
{
    int                retval;
    char               fullpathname[MAXPATHLEN];
    char               processname[64];
    char               data[MAXPATHLEN];
    char               drivestring[4];
    ifsreq             origifsr;
    pioreq             origir;
    _WIN32_FIND_DATA   *finddata;
    struct srch_entry  *search;
    BOOLEAN            log;
    _QWORD             result;
    DWORD              timelo, timehi;
    DWORD              timelo1, timehi1;
    DWORD              dostime, dosdate;
    DWORD              datetimelo, datetimehi;
    int                i, j;

    // 
    // Inititlize default data.
    //
    data[0] = 0;

    //
    // Save original iorequest because some entries get modified.
    //
    origir = (pioreq) &origifsr;
    memcpy( &origifsr, pir, sizeof( ifsreq ));

    //
    // Get the current process name.
    //
    FilemonGetProcess( processname );

    //
    // Get the time
    //
    VTD_Get_Real_Time( &timehi, &timelo );
    datetimehi = IFSMgr_Get_DOSTime( &datetimelo );
    dostime = VTD_Get_Date_And_Time( &dosdate );
    datetimelo = dostime - ((datetimehi >> 11)& 0x1F)*60*60*1000 - 
            ((datetimehi >> 5) & 0x3F)*60*1000 - 
            ((datetimehi & 0x1F)*2000);

    //
    // Special case for close call, since after the file's closed 
    // we can't query its name
    //
    if( fn == IFSFN_CLOSE ) {

        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
    }

    //
    // Call the previous hooker first, to get the return code.
    //
    retval = (*PrevIFSHookProc)(pfn, fn, Drive, ResType, CodePage, pir);

    //
    // Now extract parameters based on the function type.
    //
    dprintf("%d: %d\n", fn, pir->ir_fh ); 
    switch( fn ) {

    case IFSFN_OPEN:

        FilemonConvertPath( CONVERT_STANDARD, Drive, origir, CodePage, fullpathname );

        TIME_DIFF();
        sprintf(data,"");
        if( origir->ir_options & ACTION_CREATENEW ) strcat(data,"CREATENEW ");
        if( origir->ir_options & ACTION_OPENEXISTING ) strcat(data,"OPENEXISTING ");
        if( origir->ir_options & ACTION_REPLACEEXISTING ) strcat(data,"REPLACEEXISTING ");
        switch (origir->ir_flags & ACCESS_MODE_MASK) {

        case ACCESS_READONLY:
            strcat(data,"READONLY ");
            break;
        case ACCESS_WRITEONLY:
            strcat(data,"WRITEONLY ");
            break;
        case ACCESS_READWRITE:
            strcat(data,"READWRITE ");
            break;
        case ACCESS_EXECUTE:
            strcat(data,"EXECUTE ");
            break;
        default:
            break;
        }
        switch (origir->ir_flags & SHARE_MODE_MASK) {
        case SHARE_COMPATIBILITY:
            strcat(data,"COMPATIBILITY ");
            break;
        case SHARE_DENYREADWRITE:
            strcat(data,"DENYREADWRITE ");
            break;
        case SHARE_DENYWRITE:
            strcat(data,"DENYWRITE ");
            break;
        case SHARE_DENYREAD:
            strcat(data,"DENYREAD ");
            break;
        case SHARE_DENYNONE:
            strcat(data,"DENYNONE ");
            break;
        default:
            break;
        }
        LogRecord( timelo, datetimelo, datetimehi, "%s\tOpen\t%s\t%s\t%s", 
                   processname, fullpathname,
                   data, ErrorString( retval ));
        FilemonLogHash( Drive, pir->ir_fh, fullpathname );
        break;

    case IFSFN_READ:
    case IFSFN_WRITE:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( ((fn == IFSFN_READ && FilterDef.logreads ) ||
             (fn == IFSFN_WRITE && FilterDef.logwrites )) ) {

            TIME_DIFF();
            sprintf( data, "Offset: %ld Length: %ld", origir->ir_pos, origir->ir_length );
            LogRecord( timelo, datetimelo, datetimehi, "%s\t%s\t%s\t%s\t%s", 
                       processname, fn == IFSFN_READ? "Read" : "Write", 
                       fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_CLOSE:
        if( FilterDef.logreads ) {

            TIME_DIFF();
            switch( origir->ir_flags ) {
            case CLOSE_HANDLE:      sprintf(data, "CLOSE_HANDLE");      break;
            case CLOSE_FOR_PROCESS: sprintf(data, "CLOSE_FOR_PROCESS"); break;
            case CLOSE_FINAL:       sprintf(data, "CLOSE_FINAL");       break;
            default: sprintf(data,"0x%02X",origir->ir_flags);            break;
            }
            LogRecord( timelo, datetimelo, datetimehi, "%s\tClose\t%s\t%s\t%s", processname, 
                       fullpathname, data, ErrorString( retval ));
        }
        if( origir->ir_flags == CLOSE_FINAL ) FilemonFreeHashEntry( Drive, origir->ir_fh);
        break;

    case IFSFN_DIR:

        //
        // This works around a special case I've seen when hiting the "browse" button in the 
        // "have disk" dialog of the hardware wizard
        //
        if( origir->ir_flags != 0xFF ) {

            FilemonConvertPath( CONVERT_STANDARD, Drive, origir, CodePage, fullpathname );

        } else {

            sprintf( fullpathname, "%c:", Drive+'A'-1);
        }
        TIME_DIFF();
        log = FALSE;
        switch( origir->ir_flags ) {
        case CREATE_DIR:
            sprintf(data, "CREATE");
            if( FilterDef.logwrites ) log = TRUE;
            break;
        case DELETE_DIR:
            sprintf(data,"DELETE");
            if( FilterDef.logwrites ) log = TRUE;
            break;
        case CHECK_DIR:
            sprintf(data,"CHECK");
            if( FilterDef.logreads ) log = TRUE;
            break;
        default:
            sprintf(data,"QUERY");
            if( FilterDef.logreads ) log = TRUE;
            break;
        }
        if( log ) {
            LogRecord( timelo, datetimelo, datetimehi, "%s\tDirectory\t%s\t%s\t%s", 
                       processname, 
                       fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_SEEK:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logreads) {

            TIME_DIFF();
            sprintf(data, "%s Offset: %ld / New offset: %ld",
                    origir->ir_flags == FILE_BEGIN ? "Beginning" : "End",
                    origir->ir_pos, origir->ir_pos );        
            LogRecord( timelo, datetimelo, datetimehi, "%s\tSeek\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_COMMIT:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logwrites) {

            TIME_DIFF();
            sprintf(data, "%s", origir->ir_options == FILE_COMMIT_ASYNC ? 
                    "ASYNC" : "NOACCESSUPDATE" );
            LogRecord( timelo, datetimelo, datetimehi, "%s\tCommit\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_FILELOCKS:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logreads) {

            TIME_DIFF();
            sprintf(data, "Offset: %ld Length:%ld", origir->ir_pos, origir->ir_locklen );
            LogRecord( timelo, datetimelo, datetimehi, "%s\t%s\t%s\t%s\t%s", 
                       processname, origir->ir_flags == LOCK_REGION ? "Lock" : "Unlock",
                       fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_FINDOPEN:
        if( FilterDef.logreads) {	

            FilemonConvertPath( CONVERT_FINDOPEN, Drive, origir, CodePage, fullpathname );
            TIME_DIFF();
            if( !retval ) {

                finddata = (_WIN32_FIND_DATA *) origir->ir_data;
                UniToBCS( data, finddata->cFileName, wstrlen(finddata->cFileName), MAXPATHLEN-1, BCS_WANSI, &result );
                data[ result.ddLower ] = 0;
            }
            LogRecord( timelo, datetimelo, datetimehi, "%s\tFindOpen\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        FilemonLogHash( Drive, pir->ir_fh, fullpathname );
        break;

    case IFSFN_FINDNEXT:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logreads) {	

            TIME_DIFF();
            if( !retval ) {
                finddata = (_WIN32_FIND_DATA *) origir->ir_data;
                UniToBCS( data, finddata->cFileName, wstrlen(finddata->cFileName), MAXPATHLEN-1, BCS_WANSI, &result );
                data[ result.ddLower ] = 0;
            }

            LogRecord( timelo, datetimelo, datetimehi, "%s\tFindNext\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_FINDCLOSE:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logreads) {	

            TIME_DIFF();
            LogRecord( timelo, datetimelo, datetimehi, "%s\tFindClose\t%s\t\t%s", 
                       processname, fullpathname,
                       ErrorString( retval ));
        }
        FilemonFreeHashEntry( Drive, origir->ir_fh );
        break;

    case IFSFN_FILEATTRIB:
        if( FilterDef.logreads) {    

            FilemonConvertPath( CONVERT_STANDARD, Drive, origir, CodePage, fullpathname );
            TIME_DIFF();
            switch(origir->ir_flags ) {
            case GET_ATTRIBUTES:
                sprintf(data,"GetAttributes");
                break;
            case SET_ATTRIBUTES:
                sprintf(data, "SetAttributes" );
                break;
            case GET_ATTRIB_COMP_FILESIZE:
                sprintf(data, "GET_ATTRIB_COMP_FILESIZE" );
                break;
            case SET_ATTRIB_MODIFY_DATETIME:
                sprintf(data, "SET_ATTRIB_MODIFY_DATETIME");
                break;
            case SET_ATTRIB_LAST_ACCESS_DATETIME:
                sprintf(data, "SET_ATTRIB_LAST_ACCESS_DATETIME");
                break;
            case GET_ATTRIB_LAST_ACCESS_DATETIME:
                sprintf(data, "GET_ATTRIB_LAST_ACCESS_DATETIME");
                break;
            case SET_ATTRIB_CREATION_DATETIME:
                sprintf(data, "SET_ATTRIB_CREATION_DATETIME");
                break;
            case GET_ATTRIB_CREATION_DATETIME:
                sprintf(data, "GET_ATTRIB_CREATION_DATETIME");
                break;
            }
            LogRecord( timelo, datetimelo, datetimehi, "%s\tAttributes\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_FILETIMES:
        FilemonGetFullPath( origir->ir_fh, fullpathname, Drive, ResType, CodePage, origir );
        if( FilterDef.logreads) {	

            TIME_DIFF();
            switch( origir->ir_flags ) {
            case GET_MODIFY_DATETIME:
                sprintf(data, "Get Modify");
                break;
            case SET_MODIFY_DATETIME:
                sprintf(data, "Set Modify");
                break;
            case GET_LAST_ACCESS_DATETIME:
                sprintf(data, "Get Access");
                break;
            case SET_LAST_ACCESS_DATETIME:
                sprintf(data, "Set Access");
                break;
            case GET_CREATION_DATETIME:
                sprintf(data, "Get Creation");
                break;
            case SET_CREATION_DATETIME:
                sprintf(data, "Set Creation");
                break;
            }
            LogRecord( timelo, datetimelo, datetimehi, "%s\tAttributes\t%s\t%s\t%s", 
                       processname, fullpathname,
                       data, ErrorString( retval ));
        }
        break;

    case IFSFN_FLUSH:
        if( FilterDef.logwrites) {

            TIME_DIFF();
            LogRecord( timelo, datetimelo, datetimehi, "%s\tFlushVolume\t\t\t%s",
                       processname, ErrorString( retval ));
        }
        break;

    case IFSFN_DELETE:
        if( FilterDef.logwrites) {    

            FilemonConvertPath( CONVERT_STANDARD, Drive, origir, CodePage, fullpathname );
            TIME_DIFF();
            LogRecord( timelo, datetimelo, datetimehi, "%s\tDelete\t%s\t\t%s", 
                       processname, fullpathname, ErrorString( retval ));
        }
        FilemonFreeHashEntry( Drive, origir->ir_fh );
        break;

    case IFSFN_SEARCH:
        if( FilterDef.logreads ) {

            if( origir->ir_flags == SEARCH_FIRST ) 
                FilemonConvertPath( CONVERT_STANDARD, Drive, origir, CodePage, fullpathname );
            else
                sprintf(fullpathname, "SearchNext" );
            TIME_DIFF();
            if( !retval ) {
                j = 0;
                if( origir->ir_attr & FILE_ATTRIBUTE_LABEL ) {
                    sprintf(data, "VolumeLabel: " );
                    j = strlen( data );
                }
                search = (struct srch_entry *) origir->ir_data;
                for( i = 0; i < 13; i++ ) 
                    if( search->se_name[i] != ' ' ) data[j++] = search->se_name[i];
                data[j] = 0;
            }
            LogRecord( timelo, datetimelo, datetimehi, "%s\tSearch\t%s\t%s\t%s", 
                       processname, fullpathname, data, ErrorString( retval ));    
        }
        break;
    
    case IFSFN_GETDISKINFO:

        if( FilterDef.logreads ) {

            TIME_DIFF();
            if( !retval ) sprintf(data, "Free Space");
            drivestring[0] = Drive+'A'-1;
            drivestring[1] = ':';
            drivestring[2] = 0;
            LogRecord( timelo, datetimelo, datetimehi, "%s\tGetDiskInfo\t%s\t%s\t%s",
                       processname, drivestring, data, ErrorString( retval ));
        }
        break;

    case IFSFN_RENAME:
        if( FilterDef.logwrites) {          

            FilemonConvertPath( CONVERT_RENAME_SOURCE, Drive, origir, CodePage, fullpathname );
            TIME_DIFF();
            LogRecord( timelo, datetimelo, datetimehi, "%s\tRename\t%s\t%s\t%s",
                       processname, fullpathname,
                       FilemonConvertPath( CONVERT_RENAME_TARGET, Drive, origir, CodePage, data ),
                       ErrorString( retval ));		 
        }
        break;
    case IFSFN_IOCTL16DRIVE:
        if( FilterDef.logreads || FilterDef.logwrites) {

            TIME_DIFF();
            sprintf(data, "Subfunction: %02Xh", origir->ir_flags );
            drivestring[0] = Drive+'A'-1;
            drivestring[1] = ':';
            drivestring[2] = 0;
            LogRecord( timelo, datetimelo, datetimehi, "%s\tIoctl\t%s\t%s\t%s",
                       processname, drivestring, data, ErrorString( retval ));
        }
        break;
    }
    dprintf("==>%d\n", fn );
    return retval;
}
#pragma optimize("", on)

//----------------------------------------------------------------------
//
// OnSysDynamicDeviceInit
//
// Dynamic init. Install a file system filter hook.
//
//----------------------------------------------------------------------
BOOL 
OnSysDynamicDeviceInit(
    VOID
    )
{
    int i;
    MEMHANDLE hLog;

    //
    // Initialize the locks.
    //
    LogMutex = Create_Semaphore(1);
    HashMutex  = Create_Semaphore(1);
    FilterMutex  = Create_Semaphore(1);

    // 
    // Zero hash table.
    //
    for(i = 0; i < NUMHASH; i++ ) HashTable[i] = NULL;

    //
    // Allocate the initial output buffer.
    //
    PageAllocate(LOGBUFPAGES, PG_SYS, 0, 0, 0, 0, NULL, PAGELOCKED, 
                 (PMEMHANDLE) &hLog, (PVOID) &Log );
    Log->Handle = hLog;
    Log->Len = 0;
    Log->Next = NULL;
    NumLog = 1;

    //
    // Hook IFS functions.
    //
    PrevIFSHookProc = IFSMgr_InstallFileSystemApiHook(FilemonHookProc);
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
    //
    // Unhook IFS functions.
    //
    IFSMgr_RemoveFileSystemApiHook(FilemonHookProc);

    //
    // Free all memory.
    //
    FilemonHashCleanup();
    FilemonFreeLog();
    FilemonFreeFilters();
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
    PLOG_BUF      old;

    switch( p->dioc_IOCtlCode ) {
    case 0:
        return ERROR_SUCCESS;

    case IOCTL_FILEMON_ZEROSTATS:

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
        Signal_Semaphore( LogMutex );
        Sequence = 0;
        return ERROR_SUCCESS;

    case IOCTL_FILEMON_GETSTATS:

        //
        // Copy buffer into user space.
        Wait_Semaphore( LogMutex, BLOCK_SVC_INTS );
        if ( LOGBUFSIZE > p->dioc_cbOutBuf ) {

            //
            // Buffer is too small. Return error.
            //
            Signal_Semaphore( LogMutex );
            return ERROR_INSUFFICIENT_BUFFER;

        } else if ( Log->Len  ||  Log->Next ) {

            //
            // Switch to a new buffer.
            //
            FilemonNewLog();

            //
            // Fetch the oldest buffer to give to caller.
            //
            old = FilemonOldestLog();
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

        } else {

            //
            // There is no unread data.
            //
            Signal_Semaphore( LogMutex );
            *p->dioc_bytesret = 0;
        }
        return ERROR_SUCCESS;

    case IOCTL_FILEMON_STOPFILTER:

        FilterOn = FALSE;
        return ERROR_SUCCESS;

    case IOCTL_FILEMON_STARTFILTER:

        FilterOn = TRUE;
        return ERROR_SUCCESS;

    case IOCTL_FILEMON_SETFILTER:

        FilterDef = * (PFILTER) p->dioc_InBuf;
        FilemonUpdateFilters();
        return ERROR_SUCCESS;

    default:
        return ERROR_INVALID_FUNCTION;
    }
}
