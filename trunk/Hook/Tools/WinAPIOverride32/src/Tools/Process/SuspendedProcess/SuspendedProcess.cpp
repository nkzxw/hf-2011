/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: class helper for suspended process (process created with CREATE_SUSPENDED flag)
//-----------------------------------------------------------------------------


// Thx to gleat 
// http://www.codeproject.com/KB/cpp/selfdel.aspx?fid=373388&df=90&mpp=25&noise=3&sort=Position&view=Quick&fr=26
//
// Gets the address of the entry point routine given a
// handle to a process and its primary thread.
//
#include "SuspendedProcess.h"

typedef LONG NTSTATUS, *PNTSTATUS;
#include <ntsecapi.h>


typedef LPVOID *PPVOID;

typedef LONG KPRIORITY;

typedef struct _CLIENT_ID{
    DWORD                    ClientID0;
    DWORD                    ClientID1; // thread id
} CLIENT_ID, *PCLIENT_ID;

typedef struct _LDR_MODULE {
    LIST_ENTRY                InLoadOrderModuleList;
    LIST_ENTRY                InMemoryOrderModuleList;
    LIST_ENTRY                InInitializationOrderModuleList;
    PVOID                    BaseAddress;
    PVOID                    EntryPoint;
    ULONG                    SizeOfImage;
    UNICODE_STRING            FullDllName;
    UNICODE_STRING            BaseDllName;
    ULONG                    Flags;
    SHORT                    LoadCount;
    SHORT                    TlsIndex;
    LIST_ENTRY                HashTableEntry;
    ULONG                    TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _PEB_LDR_DATA {
    ULONG                    Length;
    BOOL                    Initialized;
    PVOID                    SsHandle;
    LIST_ENTRY                InLoadOrderModuleList;
    LIST_ENTRY                InMemoryOrderModuleList;
    LIST_ENTRY                InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
    USHORT                    Flags;
    USHORT                    Length;
    ULONG                    TimeStamp;
    UNICODE_STRING            DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG                    MaximumLength;
    ULONG                    Length;
    ULONG                    Flags;
    ULONG                    DebugFlags;
    PVOID                    ConsoleHandle;
    ULONG                    ConsoleFlags;
    HANDLE                    StdInputHandle;
    HANDLE                    StdOutputHandle;
    HANDLE                    StdErrorHandle;
    UNICODE_STRING            CurrentDirectoryPath;
    HANDLE                    CurrentDirectoryHandle;
    UNICODE_STRING            DllPath;
    UNICODE_STRING            ImagePathName;
    UNICODE_STRING            CommandLine;
    PVOID                    Environment;
    ULONG                    StartingPositionLeft;
    ULONG                    StartingPositionTop;
    ULONG                    Width;
    ULONG                    Height;
    ULONG                    CharWidth;
    ULONG                    CharHeight;
    ULONG                    ConsoleTextAttributes;
    ULONG                    WindowFlags;
    ULONG                    ShowWindowFlags;
    UNICODE_STRING            WindowTitle;
    UNICODE_STRING            DesktopName;
    UNICODE_STRING            ShellInfo;
    UNICODE_STRING            RuntimeData;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID                    EntryPoint;
    ULONG                    StackZeroBits;
    ULONG                    StackReserved;
    ULONG                    StackCommit;
    ULONG                    ImageSubsystem;
    WORD                    SubsystemVersionLow;
    WORD                    SubsystemVersionHigh;
    ULONG                    Unknown1;
    ULONG                    ImageCharacteristics;
    ULONG                    ImageMachineType;
    ULONG                    Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _RTL_USER_PROCESS_INFORMATION {
    ULONG                    Size;
    HANDLE                    ProcessHandle;
    HANDLE                    ThreadHandle;
    CLIENT_ID                ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

typedef void (*PPEBLOCKROUTINE)( PVOID PebLock ); 

typedef struct _PEB_FREE_BLOCK {
    struct _PEB_FREE_BLOCK    *Next;
    ULONG                    Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _INITIAL_TEB {
    PVOID                    StackBase;
    PVOID                    StackLimit;
    PVOID                    StackCommit;
    PVOID                    StackCommitMax;
    PVOID                    StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS                ExitStatus;
    PVOID                    TebBaseAddress;
    CLIENT_ID                ClientId;
    KAFFINITY                AffinityMask;
    KPRIORITY                Priority;
    KPRIORITY                BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _THREAD_TIMES_INFORMATION {
    LARGE_INTEGER            CreationTime;
    LARGE_INTEGER            ExitTime;
    LARGE_INTEGER            KernelTime;
    LARGE_INTEGER            UserTime;
} THREAD_TIMES_INFORMATION, *PTHREAD_TIMES_INFORMATION;



typedef struct _UNDOCUMENTED_PEB {
    BOOLEAN                 InheritedAddressSpace;
    BOOLEAN                 ReadImageFileExecOptions;
    BOOLEAN                 BeingDebugged;
    BOOLEAN                 Spare;
    HANDLE                  Mutant;
    PVOID                   ImageBaseAddress;
    PPEB_LDR_DATA           LoaderData;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID                   SubSystemData;
    PVOID                   ProcessHeap;
    PVOID                   FastPebLock;
    PPEBLOCKROUTINE         FastPebLockRoutine;
    PPEBLOCKROUTINE         FastPebUnlockRoutine;
    ULONG                   EnvironmentUpdateCount;
    PVOID*                  KernelCallbackTable;
    PVOID                   EventLogSection;
    PVOID                   EventLog;
    PPEB_FREE_BLOCK         FreeList;
    ULONG                   TlsExpansionCounter;
    PVOID                   TlsBitmap;
    ULONG                   TlsBitmapBits[0x2];
    PVOID                   ReadOnlySharedMemoryBase;
    PVOID                   ReadOnlySharedMemoryHeap;
    PVOID*                  ReadOnlyStaticServerData;
    PVOID                   AnsiCodePageData;
    PVOID                   OemCodePageData;
    PVOID                   UnicodeCaseTableData;
    ULONG                   NumberOfProcessors;
    ULONG                   NtGlobalFlag;
    BYTE                    Spare2[0x4];
    LARGE_INTEGER           CriticalSectionTimeout;
    ULONG                   HeapSegmentReserve;
    ULONG                   HeapSegmentCommit;
    ULONG                   HeapDeCommitTotalFreeThreshold;
    ULONG                   HeapDeCommitFreeBlockThreshold;
    ULONG                   NumberOfHeaps;
    ULONG                   MaximumNumberOfHeaps;
    PVOID*                  *ProcessHeaps;
    PVOID                   GdiSharedHandleTable;
    PVOID                   ProcessStarterHelper;
    PVOID                   GdiDCAttributeList;
    PVOID                   LoaderLock;
    ULONG                   OSMajorVersion;
    ULONG                   OSMinorVersion;
    ULONG                   OSBuildNumber;
    ULONG                   OSPlatformId;
    ULONG                   ImageSubSystem;
    ULONG                   ImageSubSystemMajorVersion;
    ULONG                   ImageSubSystemMinorVersion;
    ULONG                   GdiHandleBuffer[0x22];
    ULONG                   PostProcessInitRoutine;
    ULONG                   TlsExpansionBitmap;
    BYTE                    TlsExpansionBitmapBits[0x80];
    ULONG                   SessionId;
} UNDOCUMENTED_PEB, *PUNDOCUMENTED_PEB;

typedef struct _UNDOCUMENTED_TEB {
    NT_TIB                   Tib;
    PVOID                    EnvironmentPointer;
    CLIENT_ID                Cid;
    PVOID                    ActiveRpcInfo;
    PVOID                    ThreadLocalStoragePointer;
    PUNDOCUMENTED_PEB        Peb;
    ULONG                    LastErrorValue;
    ULONG                    CountOfOwnedCriticalSections;
    PVOID                    CsrClientThread;
    PVOID                    Win32ThreadInfo;
    ULONG                    Win32ClientInfo[0x1F];
    PVOID                    WOW32Reserved;
    ULONG                    CurrentLocale;
    ULONG                    FpSoftwareStatusRegister;
    PVOID                    SystemReserved1[0x36];
    PVOID                    Spare1;
    ULONG                    ExceptionCode;
    ULONG                    SpareBytes1[0x28];
    PVOID                    SystemReserved2[0xA];
    ULONG                    GdiRgn;
    ULONG                    GdiPen;
    ULONG                    GdiBrush;
    CLIENT_ID                RealClientId;
    PVOID                    GdiCachedProcessHandle;
    ULONG                    GdiClientPID;
    ULONG                    GdiClientTID;
    PVOID                    GdiThreadLocaleInfo;
    PVOID                    UserReserved[5];
    PVOID                    GlDispatchTable[0x118];
    ULONG                    GlReserved1[0x1A];
    PVOID                    GlReserved2;
    PVOID                    GlSectionInfo;
    PVOID                    GlSection;
    PVOID                    GlTable;
    PVOID                    GlCurrentRC;
    PVOID                    GlContext;
    NTSTATUS                LastStatusValue;
    UNICODE_STRING            StaticUnicodeString;
    WCHAR                    StaticUnicodeBuffer[0x105];
    PVOID                    DeallocationStack;
    PVOID                    TlsSlots[0x40];
    LIST_ENTRY               TlsLinks;
    PVOID                    Vdm;
    PVOID                    ReservedForNtRpc;
    PVOID                    DbgSsReserved[0x2];
    ULONG                    HardErrorDisabled;
    PVOID                    Instrumentation[0x10];
    PVOID                    WinSockData;
    ULONG                    GdiBatchCount;
    ULONG                    Spare2;
    ULONG                    Spare3;
    ULONG                    Spare4;
    PVOID                    ReservedForOle;
    ULONG                    WaitingOnLoaderLock;
    PVOID                    StackCommit;
    PVOID                    StackCommitMax;
    PVOID                    StackReserved;
} UNDOCUMENTED_TEB, *PUNDOCUMENTED_TEB;

//-----------------------------------------------------------------------------
// Name: GetBaseAddress
// Object: show windows api error (avoid to remove last error value)
// Parameters :
//     in  : HANDLE hProcess : Process handle returned by PROCESS_INFORMATION parameter of CreateProcess function
//           HANDLE hThread : Thread handle returned by PROCESS_INFORMATION parameter of CreateProcess function
//     out :
//     return : exe base addresse on success, 0 on error
//-----------------------------------------------------------------------------
PBYTE CSuspendedProcess::GetBaseAddress( HANDLE hProcess, HANDLE hThread )
{
#ifndef _WIN64
    // following works only with x86 in 32 bits
    // can be changed if someone knows a more portable trick

    CONTEXT             context;
    LDT_ENTRY           entry;
    UNDOCUMENTED_TEB    teb;
    UNDOCUMENTED_PEB    peb;
    DWORD               read;
    DWORD               dwFSBase;
    DWORD               dwImageBase;

    // get thread context
    context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (! ::GetThreadContext( hThread, &context ) )
        return 0;

    // use the segment register value to get a pointer to
    // the TEB
    if (! ::GetThreadSelectorEntry( hThread, context.SegFs, &entry ) )
        return 0;

    dwFSBase = ( entry.HighWord.Bits.BaseHi << 24 ) |
        ( entry.HighWord.Bits.BaseMid << 16 ) |
        ( entry.BaseLow );

    // read the teb
    if (! ::ReadProcessMemory( hProcess, (LPCVOID)dwFSBase,&teb, sizeof( UNDOCUMENTED_TEB ), &read ) )
        return 0;

    // read the peb from the location pointed at by the teb
    if (! ::ReadProcessMemory( hProcess, (LPCVOID)teb.Peb,&peb, sizeof( UNDOCUMENTED_PEB ), &read ) )
        return 0;

    // figure out where the entry point is located;
    dwImageBase = (DWORD)peb.ImageBaseAddress;

    return (PBYTE)dwImageBase;


#else
TODO
#endif
}