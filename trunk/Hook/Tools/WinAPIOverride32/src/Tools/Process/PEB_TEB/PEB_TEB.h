/*********************************************************************
 * Structures and definitions undocumented or included in the NTDDK. *
 *********************************************************************/
#pragma once
#include <winnt.h> // for NT_TIB

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation = 0,
    ThreadIsIoPending = 16
} THREADINFOCLASS;

typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

template <class TSIZE>
class CTemplatePebTeb
{
public:

typedef LONG KPRIORITY;



//typedef enum _SYSTEM_INFORMATION_CLASS {
//    SystemBasicInformation = 0,
//    SystemPerformanceInformation = 2,
//    SystemTimeOfDayInformation = 3,
//    SystemProcessInformation = 5,
//    SystemProcessorPerformanceInformation = 8,
//    SystemInterruptInformation = 23,
//    SystemExceptionInformation = 33,
//    SystemRegistryQuotaInformation = 37,
//    SystemLookasideInformation = 45
//} SYSTEM_INFORMATION_CLASS;

typedef struct _CLIENT_IDT {
    /*HANDLE*/ TSIZE UniqueProcess;
    /*HANDLE*/ TSIZE UniqueThread;
} CLIENT_IDT;

typedef struct _UNICODE_STRINGT
{
    USHORT Length;
    USHORT MaximumLength;
    /*PWSTR*/ TSIZE Buffer;
} UNICODE_STRINGT;

typedef struct _RTL_DRIVE_LETTER_CURDIRT
{
    USHORT                  Flags;
    USHORT                  Length;
    ULONG                   TimeStamp;
    UNICODE_STRINGT          DosPath;
} RTL_DRIVE_LETTER_CURDIRT;


typedef struct _RTL_USER_PROCESS_PARAMETERST 
{
    ULONG                   MaximumLength;
    ULONG                   Length;
    ULONG                   Flags;
    ULONG                   DebugFlags;
    /*PVOID*/TSIZE          ConsoleHandle;
    ULONG                   ConsoleFlags;
    /*HANDLE*/ TSIZE        StdInputHandle;
    /*HANDLE*/ TSIZE        StdOutputHandle;
    /*HANDLE*/ TSIZE        StdErrorHandle;
    UNICODE_STRINGT          CurrentDirectoryPath;
    /*HANDLE*/ TSIZE        CurrentDirectoryHandle;
    UNICODE_STRINGT          DllPath;
    UNICODE_STRINGT          ImagePathName;
    UNICODE_STRINGT          CommandLine;
    /*PVOID*/TSIZE          Environment;
    ULONG                   StartingPositionLeft;
    ULONG                   StartingPositionTop;
    ULONG                   Width;
    ULONG                   Height;
    ULONG                   CharWidth;
    ULONG                   CharHeight;
    ULONG                   ConsoleTextAttributes;
    ULONG                   WindowFlags;
    ULONG                   ShowWindowFlags;
    UNICODE_STRINGT          WindowTitle;
    UNICODE_STRINGT          DesktopName;
    UNICODE_STRINGT          ShellInfo;
    UNICODE_STRINGT          RuntimeData;
    RTL_DRIVE_LETTER_CURDIRT DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERST;

typedef struct _LIST_ENTRYT {
    /*struct _LIST_ENTRY * */ TSIZE Flink;
    /*struct _LIST_ENTRY * */ TSIZE Blink;
} LIST_ENTRYT;

typedef struct _LDR_MODULET
{
    LIST_ENTRYT InLoadOrderModuleList;
    LIST_ENTRYT InMemoryOrderModuleList;
    LIST_ENTRYT InInitializationOrderModuleList;
    /*PVOID*/TSIZE BaseAddress;
    /*PVOID*/TSIZE EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRINGT FullDllName;
    UNICODE_STRINGT BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRYT HashTableEntry;
    ULONG TimeDateStamp;
} LDR_MODULET;


typedef struct _PEB_LDR_DATAT
{
    ULONG                   Length;
    BOOLEAN                 Initialized;
    /*PVOID*/TSIZE          SsHandle;
    LIST_ENTRYT             InLoadOrderModuleList;
    LIST_ENTRYT             InMemoryOrderModuleList;
    LIST_ENTRYT             InInitializationOrderModuleList;
} PEB_LDR_DATAT;

typedef struct _PEB_FREE_BLOCKT 
{
    /*_PEB_FREE_BLOCK* */ TSIZE     Next;
    ULONG                           Size;
} PEB_FREE_BLOCKT;


typedef void (*PPEBLOCKROUTINE)(/*PVOID*/TSIZE PebLock); 

// PEB (Process Environment Block) data structure (FS:[0x30])
// Located at addr. 0x7FFDF000

// windows public definition :D
//typedef struct _PEB {
//    BYTE Reserved1[2];
//    BYTE BeingDebugged;
//    BYTE Reserved2[229];
//    PVOID Reserved3[59];
//    ULONG SessionId;
//} PEB, *PPEB;
typedef struct _PEBT {

    BOOLEAN                 InheritedAddressSpace;
    BOOLEAN                 ReadImageFileExecOptions;
    BOOLEAN                 BeingDebugged;
    BOOLEAN                 Spare;
    /*HANDLE*/ TSIZE                  Mutant;
    /*PVOID*/TSIZE                   ImageBaseAddress;
    /*PPEB_LDR_DATA*/TSIZE           LoaderData;
    /*PRTL_USER_PROCESS_PARAMETERS*/TSIZE ProcessParameters;
    /*PVOID*/TSIZE                   SubSystemData;
    /*PVOID*/TSIZE                   ProcessHeap;
    /*PVOID*/TSIZE                   FastPebLock;
    /*PPEBLOCKROUTINE*/TSIZE         FastPebLockRoutine;
    /*PPEBLOCKROUTINE*/TSIZE         FastPebUnlockRoutine;
    ULONG                   EnvironmentUpdateCount;
    /*PPVOID*/TSIZE                  KernelCallbackTable;
    /*PVOID*/TSIZE                   EventLogSection;
    /*PVOID*/TSIZE                   EventLog;
    /*PPEB_FREE_BLOCK*/TSIZE         FreeList;
    ULONG                   TlsExpansionCounter;
    /*PVOID*/TSIZE                   TlsBitmap;
    ULONG                   TlsBitmapBits[0x2];
    /*PVOID*/TSIZE                   ReadOnlySharedMemoryBase;
    /*PVOID*/TSIZE                   ReadOnlySharedMemoryHeap;
    /*PPVOID*/TSIZE                  ReadOnlyStaticServerData;
    /*PVOID*/TSIZE                   AnsiCodePageData;
    /*PVOID*/TSIZE                   OemCodePageData;
    /*PVOID*/TSIZE                   UnicodeCaseTableData;
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
    /*PPVOID*/TSIZE                  *ProcessHeaps;
    /*PVOID*/TSIZE                   GdiSharedHandleTable;
    /*PVOID*/TSIZE                   ProcessStarterHelper;
    /*PVOID*/TSIZE                   GdiDCAttributeList;
    /*PVOID*/TSIZE                   LoaderLock;
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

/*
ULONG_PTR  ImageProcessAffinityMask;
GDI_HANDLE_BUFFER  GdiHandleBuffer;
PPS_POST_PROCESS_INIT_ROUTINE  PostProcessInitRoutine;

PVOID  TlsExpansionBitmap;
ULONGTlsExpansionBitmapBits[32];     // TLS_EXPANSION_SLOTS bits

//
// Id of the Hydra session in which this process is running
//
ULONGSessionId;

//
// Filled in by LdrpInstallAppcompatBackend
//
ULARGE_INTEGERAppCompatFlags;

//
// ntuser appcompat flags
//
ULARGE_INTEGERAppCompatFlagsUser;

//
// Filled in by LdrpInstallAppcompatBackend
//
PVOID  pShimData;

//
// Filled in by LdrQueryImageFileExecutionOptions
//
PVOID  AppCompatInfo;

//
// Used by GetVersionExW as the szCSDVersion string
//
PEBTEB_STRUCT(UNICODE_STRING)  CSDVersion;

//
// Fusion stuff
//
const  struct  _ACTIVATION_CONTEXT_DATA*  ActivationContextData;
struct  _ASSEMBLY_STORAGE_MAP*  ProcessAssemblyStorageMap;
const  struct  _ACTIVATION_CONTEXT_DATA*  SystemDefaultActivationContextData;
struct  _ASSEMBLY_STORAGE_MAP*  SystemAssemblyStorageMap;

//
// Enforced minimum initial commit stack
//
SIZE_T  MinimumStackCommit;

//
// Fiber local storage.
//

PPVOID  FlsCallback;
PEBTEB_STRUCT(LIST_ENTRY)  FlsListHead;
PVOID  FlsBitmap;
ULONGFlsBitmapBits[FLS_MAXIMUM_AVAILABLE/  (sizeof(ULONG)  *  8)];
ULONGFlsHighIndex;
*/
} PEBT;

typedef struct
{
    TSIZE ExitStatus;
    /*PPEB*/ TSIZE PebBaseAddress;
    TSIZE AffinityMask;
    TSIZE BasePriority;
    TSIZE UniqueProcessId;
    TSIZE InheritedFromUniqueProcessId;
}   PROCESS_BASIC_INFORMATIONT;


typedef struct _NT_TIBT {
    /*struct _EXCEPTION_REGISTRATION_RECORD* */TSIZE ExceptionList;
    /*PVOID*/TSIZE StackBase;
    /*PVOID*/TSIZE StackLimit;
    /*PVOID*/TSIZE SubSystemTib;
#if defined(_MSC_EXTENSIONS)
    union {
        /*PVOID*/TSIZE FiberData;
        DWORD Version;
    };
#else
    /*PVOID*/TSIZE FiberData;
#endif
    /*PVOID*/TSIZE ArbitraryUserPointer;
    /*struct _NT_TIB* */TSIZE Self;
} NT_TIBT;

// TEB (Thread Environment Block) data structure (FS:[0x18])
// Located at 0x7FFDE000, 0x7FFDD000, ...
typedef struct _TEBT {

    NT_TIBT                  Tib;
    /*PVOID*/TSIZE                    EnvironmentPointer;
    CLIENT_IDT               Cid;
    /*PVOID*/TSIZE                   ActiveRpcInfo;
    /*PVOID*/TSIZE                   ThreadLocalStoragePointer;
    /*PPEB*/ TSIZE                    Peb;
    ULONG                   LastErrorValue;
    ULONG                   CountOfOwnedCriticalSections;
    /*PVOID*/TSIZE                   CsrClientThread;
    /*PVOID*/TSIZE                   Win32ThreadInfo;
    ULONG                   Win32ClientInfo[0x1F];
    /*PVOID*/TSIZE                   WOW32Reserved;
    ULONG                   CurrentLocale;
    ULONG                   FpSoftwareStatusRegister;
    /*PVOID*/TSIZE                   SystemReserved1[0x36];
    /*PVOID*/TSIZE                   Spare1;
    ULONG                   ExceptionCode;
    ULONG                   SpareBytes1[0x28];
    /*PVOID*/TSIZE                   SystemReserved2[0xA];
    ULONG                   GdiRgn;
    ULONG                   GdiPen;
    ULONG                   GdiBrush;
    CLIENT_IDT               RealClientId;
    /*PVOID*/TSIZE                   GdiCachedProcessHandle;
    ULONG                   GdiClientPID;
    ULONG                   GdiClientTID;
    /*PVOID*/TSIZE                   GdiThreadLocaleInfo;
    /*PVOID*/TSIZE                   UserReserved[5];
    /*PVOID*/TSIZE                   GlDispatchTable[0x118];
    ULONG                   GlReserved1[0x1A];
    /*PVOID*/TSIZE                   GlReserved2;
    /*PVOID*/TSIZE                   GlSectionInfo;
    /*PVOID*/TSIZE                   GlSection;
    /*PVOID*/TSIZE                   GlTable;
    /*PVOID*/TSIZE                   GlCurrentRC;
    /*PVOID*/TSIZE                   GlContext;
    NTSTATUS                LastStatusValue;
    UNICODE_STRINGT          StaticUnicodeString;
    WCHAR                   StaticUnicodeBuffer[0x105];
    /*PVOID*/TSIZE                   DeallocationStack;
    /*PVOID*/TSIZE                   TlsSlots[0x40];
    LIST_ENTRY              TlsLinks;
    /*PVOID*/TSIZE                   Vdm;
    /*PVOID*/TSIZE                   ReservedForNtRpc;
    /*PVOID*/TSIZE                   DbgSsReserved[0x2];
    ULONG                   HardErrorDisabled;
    /*PVOID*/TSIZE                   Instrumentation[0x10];
    /*PVOID*/TSIZE                   WinSockData;
    ULONG                   GdiBatchCount;
    ULONG                   Spare2;
    ULONG                   Spare3;
    ULONG                   Spare4;
    /*PVOID*/TSIZE                   ReservedForOle;
    ULONG                   WaitingOnLoaderLock;
    /*PVOID*/TSIZE                   StackCommit;
    /*PVOID*/TSIZE                   StackCommitMax;
    /*PVOID*/TSIZE                   StackReserved;

} TEBT;

typedef struct _THREAD_BASIC_INFORMATIONT {
     NTSTATUS  ExitStatus;
     /*NT_TIBT* */ TSIZE TebBaseAddress;
     CLIENT_IDT ClientId;
     /*KAFFINITY*/ TSIZE AffinityMask;
     KPRIORITY Priority;
     KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATIONT;
};