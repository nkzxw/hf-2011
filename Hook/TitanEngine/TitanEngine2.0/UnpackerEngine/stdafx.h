// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#define UE_PLATFORM_x86 1
#define UE_PLATFORM_x64 2
#define UE_PLATFORM_ALL 3

// Engine.Internal:
#define MAX_IMPORT_ALLOC 256 * 256
#define MAX_RELOC_ALLOC 1024 * 1024
#define UE_MAX_RESERVED_MEMORY_LEFT 32
#define MAXIMUM_SECTION_NUMBER 32
#define MAX_INSTRUCTIONS (1000)
#define MAXIMUM_BREAKPOINTS 1000
#define MAXIMUM_INSTRUCTION_SIZE 40
#define MAX_RET_SEARCH_INSTRUCTIONS 100

#define UE_OPTION_IMPORTER_REALIGN_LOCAL_APIADDRESS 0
#define UE_OPTION_IMPORTER_REALIGN_APIADDRESS 1
#define UE_OPTION_IMPORTER_RETURN_APINAME 2
#define UE_OPTION_IMPORTER_RETURN_APIADDRESS 3
#define UE_OPTION_IMPORTER_RETURN_DLLNAME 4
#define UE_OPTION_IMPORTER_RETURN_DLLINDEX 5
#define UE_OPTION_IMPORTER_RETURN_DLLBASE 6
#define UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME 7
#define UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX 8
#define UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME 9
#define UE_OPTION_IMPORTER_RETURN_NEAREST_APIADDRESS 10
#define UE_OPTION_IMPORTER_RETURN_NEAREST_APINAME 11

typedef struct{
	bool ExpertModeActive;
	char* szFileName;
	bool ReserveModuleBase;
	char* szCommandLine;
	char* szCurrentFolder;
	LPVOID EntryCallBack;
}ExpertDebug, *PExpertDebug;

typedef struct{
	ULONG_PTR fLoadLibrary;
	ULONG_PTR fFreeLibrary;
	ULONG_PTR fGetModuleHandle;
	ULONG_PTR fGetProcAddress;
	ULONG_PTR fVirtualFree;
	ULONG_PTR fExitProcess;
	HMODULE fFreeLibraryHandle;
	DWORD fExitProcessCode;
}InjectCodeData, *PInjectCodeData;

typedef struct{
	ULONG_PTR fTrace;
	ULONG_PTR fCreateFileA;
	ULONG_PTR fCloseHandle;
	ULONG_PTR fCreateFileMappingA;
	ULONG_PTR AddressToTrace;
}InjectImpRecCodeData, *PInjectImpRecCodeData;

#define UE_MAX_BREAKPOINT_SIZE 2
#define UE_BREAKPOINT_INT3 1
#define UE_BREAKPOINT_LONG_INT3 2
#define UE_BREAKPOINT_UD2 3

typedef struct{
	BYTE BreakPointActive;
	ULONG_PTR BreakPointAddress;
	DWORD BreakPointSize;
	BYTE OriginalByte[10];
	BYTE BreakPointType;
	BYTE AdvancedBreakPointType;
	BYTE MemoryBpxRestoreOnHit;
	DWORD NumberOfExecutions;
	DWORD CmpRegister;
	BYTE CmpCondition;
	ULONG_PTR CmpValue;
	ULONG_PTR ExecuteCallBack;
	ULONG_PTR CompareCallBack;
	ULONG_PTR RemoveCallBack;
	DWORD UniqueLinkId;
}BreakPointDetail, *PBreakPointDetail;

typedef struct{
	bool DrxEnabled;
	bool DrxExecution;
	DWORD DrxBreakPointType;
	DWORD DrxBreakPointSize;
	ULONG_PTR DrxBreakAddress;
	ULONG_PTR DrxCallBack;
}HARDWARE_DATA, *PHARDWARE_DATA;

typedef struct{
	ULONG_PTR chBreakPoint;
	ULONG_PTR chSingleStep;
	ULONG_PTR chAccessViolation;
	ULONG_PTR chIllegalInstruction;
	ULONG_PTR chNonContinuableException;
	ULONG_PTR chArrayBoundsException;
	ULONG_PTR chFloatDenormalOperand;
	ULONG_PTR chFloatDevideByZero;
	ULONG_PTR chIntegerDevideByZero;
	ULONG_PTR chIntegerOverflow;
	ULONG_PTR chPrivilegedInstruction;
	ULONG_PTR chPageGuard;
	ULONG_PTR chEverythingElse;
	ULONG_PTR chCreateThread;
	ULONG_PTR chExitThread;
	ULONG_PTR chCreateProcess;
	ULONG_PTR chExitProcess;
	ULONG_PTR chLoadDll;
	ULONG_PTR chUnloadDll;
	ULONG_PTR chOutputDebugString;
}CustomHandler, *PCustomHandler;

typedef struct{
	DWORD OrdinalBase;
	DWORD NumberOfExportFunctions;
	char FileName[512];
}EXPORT_DATA, *PEXPORT_DATA;

typedef struct{
	DWORD ExportedItem;
}EXPORTED_DATA, *PEXPORTED_DATA;

typedef struct{
	WORD OrdinalNumber;
}EXPORTED_DATA_WORD, *PEXPORTED_DATA_WORD;

typedef struct{
	BYTE DataByte[50];
}MEMORY_CMP_HANDLER, *PMEMORY_CMP_HANDLER;

typedef struct{
	BYTE DataByte;
}MEMORY_CMP_BYTE_HANDLER, *PMEMORY_CMP_BYTE_HANDLER;

typedef struct MEMORY_COMPARE_HANDLER{
	union {
		BYTE bArrayEntry[1];		
		WORD wArrayEntry[1];
		DWORD dwArrayEntry[1];
		DWORD64 qwArrayEntry[1];
	} Array;
}MEMORY_COMPARE_HANDLER, *PMEMORY_COMPARE_HANDLER;

#define MAX_DEBUG_DATA 512

typedef struct{
	HANDLE hThread;
	DWORD dwThreadId;
	void* ThreadStartAddress;
	void* ThreadLocalBase;
}THREAD_ITEM_DATA, *PTHREAD_ITEM_DATA;

typedef struct{
	HANDLE hProcess;
	DWORD dwProcessId;
	HANDLE hThread;
	DWORD dwThreadId;
	HANDLE hFile;
	void* BaseOfImage;
	void* ThreadStartAddress;
	void* ThreadLocalBase;
}PROCESS_ITEM_DATA, *PPROCESS_ITEM_DATA;

typedef struct{
    HANDLE hFile;
	void* BaseOfDll;
	HANDLE hFileMapping;
	void* hFileMappingView; 
	char szLibraryPath[MAX_PATH];
	char szLibraryName[MAX_PATH];
}LIBRARY_ITEM_DATA, *PLIBRARY_ITEM_DATA;

#define MAX_LIBRARY_BPX 64
#define UE_ON_LIB_LOAD 1
#define UE_ON_LIB_UNLOAD 2
#define UE_ON_LIB_ALL 3

typedef struct{
	char szLibraryName[128];
	void* bpxCallBack;
	bool bpxSingleShoot;
	int bpxType;
}LIBRARY_BREAK_DATA, *PLIBRARY_BREAK_DATA;

// Engine.External:
#define UE_ACCESS_READ 0
#define UE_ACCESS_WRITE 1
#define UE_ACCESS_ALL 2

#define UE_ENGINE_ALOW_MODULE_LOADING 1
#define UE_ENGINE_AUTOFIX_FORWARDERS 2
#define UE_ENGINE_PASS_ALL_EXCEPTIONS 3
#define UE_ENGINE_NO_CONSOLE_WINDOW 4
#define UE_ENGINE_BACKUP_FOR_CRITICAL_FUNCTIONS 5
#define UE_ENGINE_CALL_PLUGIN_CALLBACK 6

#define UE_OPTION_REMOVEALL 1
#define UE_OPTION_DISABLEALL 2
#define UE_OPTION_REMOVEALLDISABLED 3
#define UE_OPTION_REMOVEALLENABLED 4

#define UE_STATIC_DECRYPTOR_XOR 1
#define UE_STATIC_DECRYPTOR_SUB 2
#define UE_STATIC_DECRYPTOR_ADD 3

#define UE_STATIC_KEY_SIZE_1 1
#define UE_STATIC_KEY_SIZE_2 2
#define UE_STATIC_KEY_SIZE_4 4
#define UE_STATIC_KEY_SIZE_8 8

#define UE_PE_OFFSET 0
#define UE_IMAGEBASE 1
#define UE_OEP 2
#define UE_SIZEOFIMAGE 3
#define UE_SIZEOFHEADERS 4
#define UE_SIZEOFOPTIONALHEADER 5
#define UE_SECTIONALIGNMENT 6
#define UE_IMPORTTABLEADDRESS 7
#define UE_IMPORTTABLESIZE 8
#define UE_RESOURCETABLEADDRESS 9
#define UE_RESOURCETABLESIZE 10
#define UE_EXPORTTABLEADDRESS 11
#define UE_EXPORTTABLESIZE 12
#define UE_TLSTABLEADDRESS 13
#define UE_TLSTABLESIZE 14
#define UE_RELOCATIONTABLEADDRESS 15
#define UE_RELOCATIONTABLESIZE 16
#define UE_TIMEDATESTAMP 17
#define UE_SECTIONNUMBER 18
#define UE_CHECKSUM 19
#define UE_SUBSYSTEM 20
#define UE_CHARACTERISTICS 21
#define UE_NUMBEROFRVAANDSIZES 22
#define UE_SECTIONNAME 23
#define UE_SECTIONVIRTUALOFFSET 24
#define UE_SECTIONVIRTUALSIZE 25
#define UE_SECTIONRAWOFFSET 26
#define UE_SECTIONRAWSIZE 27
#define UE_SECTIONFLAGS 28

#define UE_CH_BREAKPOINT 1
#define UE_CH_SINGLESTEP 2
#define UE_CH_ACCESSVIOLATION 3
#define UE_CH_ILLEGALINSTRUCTION 4
#define UE_CH_NONCONTINUABLEEXCEPTION 5
#define UE_CH_ARRAYBOUNDSEXCEPTION 6
#define UE_CH_FLOATDENORMALOPERAND 7
#define UE_CH_FLOATDEVIDEBYZERO 8
#define UE_CH_INTEGERDEVIDEBYZERO 9
#define UE_CH_INTEGEROVERFLOW 10
#define UE_CH_PRIVILEGEDINSTRUCTION 11
#define UE_CH_PAGEGUARD 12
#define UE_CH_EVERYTHINGELSE 13
#define UE_CH_CREATETHREAD 14
#define UE_CH_EXITTHREAD 15
#define UE_CH_CREATEPROCESS 16
#define UE_CH_EXITPROCESS 17
#define UE_CH_LOADDLL 18
#define UE_CH_UNLOADDLL 19
#define UE_CH_OUTPUTDEBUGSTRING 20

#define UE_OPTION_HANDLER_RETURN_HANDLECOUNT 1
#define UE_OPTION_HANDLER_RETURN_ACCESS 2
#define UE_OPTION_HANDLER_RETURN_FLAGS 3
#define UE_OPTION_HANDLER_RETURN_TYPENAME 4

typedef struct{
	ULONG ProcessId;
	HANDLE hHandle;
}HandlerArray, *PHandlerArray;

#define UE_BPXREMOVED 0
#define UE_BPXACTIVE 1
#define UE_BPXINACTIVE 2

#define UE_BREAKPOINT 0
#define UE_SINGLESHOOT 1
#define UE_HARDWARE 2
#define UE_MEMORY 3
#define UE_MEMORY_READ 4
#define UE_MEMORY_WRITE 5

#define UE_HARDWARE_EXECUTE 4
#define UE_HARDWARE_WRITE 5
#define UE_HARDWARE_READWRITE 6

#define UE_HARDWARE_SIZE_1 7
#define UE_HARDWARE_SIZE_2 8
#define UE_HARDWARE_SIZE_4 9

#define UE_APISTART 0
#define UE_APIEND 1

#define UE_FUNCTION_STDCALL 1
#define UE_FUNCTION_CCALL 2
#define UE_FUNCTION_FASTCALL 3
#define UE_FUNCTION_STDCALL_RET 4
#define UE_FUNCTION_CCALL_RET 5
#define UE_FUNCTION_FASTCALL_RET 6
#define UE_FUNCTION_STDCALL_CALL 7
#define UE_FUNCTION_CCALL_CALL 8
#define UE_FUNCTION_FASTCALL_CALL 9
#define UE_PARAMETER_BYTE 0
#define UE_PARAMETER_WORD 1
#define UE_PARAMETER_DWORD 2
#define UE_PARAMETER_QWORD 3
#define UE_PARAMETER_PTR_BYTE 4
#define UE_PARAMETER_PTR_WORD 5
#define UE_PARAMETER_PTR_DWORD 6
#define UE_PARAMETER_PTR_QWORD 7
#define UE_PARAMETER_STRING 8
#define UE_PARAMETER_UNICODE 9

#define UE_CMP_NOCONDITION 0
#define UE_CMP_EQUAL 1
#define UE_CMP_NOTEQUAL 2
#define UE_CMP_GREATER 3
#define UE_CMP_GREATEROREQUAL 4
#define UE_CMP_LOWER 5
#define UE_CMP_LOWEROREQUAL 6
#define UE_CMP_REG_EQUAL 7
#define UE_CMP_REG_NOTEQUAL 8
#define UE_CMP_REG_GREATER 9
#define UE_CMP_REG_GREATEROREQUAL 10
#define UE_CMP_REG_LOWER 11
#define UE_CMP_REG_LOWEROREQUAL 12
#define UE_CMP_ALWAYSFALSE 13

#define UE_EAX 1
#define UE_EBX 2
#define UE_ECX 3
#define UE_EDX 4
#define UE_EDI 5
#define UE_ESI 6
#define UE_EBP 7
#define UE_ESP 8
#define UE_EIP 9
#define UE_EFLAGS 10
#define UE_DR0 11
#define UE_DR1 12
#define UE_DR2 13
#define UE_DR3 14
#define UE_DR6 15
#define UE_DR7 16
#define UE_RAX 17
#define UE_RBX 18
#define UE_RCX 19
#define UE_RDX 20
#define UE_RDI 21
#define UE_RSI 22
#define UE_RBP 23
#define UE_RSP 24
#define UE_RIP 25
#define UE_RFLAGS 26
#define UE_R8 27
#define UE_R9 28
#define UE_R10 29
#define UE_R11 30
#define UE_R12 31
#define UE_R13 32
#define UE_R14 33
#define UE_R15 34
#define UE_CIP 35
#define UE_CSP 36

typedef struct{
	DWORD PE32Offset;
	DWORD ImageBase;
    DWORD OriginalEntryPoint;
    DWORD NtSizeOfImage;
    DWORD NtSizeOfHeaders;
    WORD SizeOfOptionalHeaders;
	DWORD FileAlignment;
    DWORD SectionAligment;
    DWORD ImportTableAddress;
	DWORD ImportTableSize;
    DWORD ResourceTableAddress;
    DWORD ResourceTableSize;
    DWORD ExportTableAddress;
    DWORD ExportTableSize;
    DWORD TLSTableAddress;
    DWORD TLSTableSize;
    DWORD RelocationTableAddress;
    DWORD RelocationTableSize;
    DWORD TimeDateStamp;
    WORD SectionNumber;
    DWORD CheckSum;
	WORD SubSystem;
	WORD Characteristics;
	DWORD NumberOfRvaAndSizes;
}PE32Struct, *PPE32Struct;

typedef struct{
	DWORD PE64Offset;
	DWORD64 ImageBase;
    DWORD OriginalEntryPoint;
    DWORD NtSizeOfImage;
    DWORD NtSizeOfHeaders;
    WORD SizeOfOptionalHeaders;
	DWORD FileAlignment;
    DWORD SectionAligment;
    DWORD ImportTableAddress;
	DWORD ImportTableSize;
    DWORD ResourceTableAddress;
    DWORD ResourceTableSize;
    DWORD ExportTableAddress;
    DWORD ExportTableSize;
    DWORD TLSTableAddress;
    DWORD TLSTableSize;
    DWORD RelocationTableAddress;
    DWORD RelocationTableSize;
    DWORD TimeDateStamp;
    WORD SectionNumber;
    DWORD CheckSum;
	WORD SubSystem;
	WORD Characteristics;
	DWORD NumberOfRvaAndSizes;
}PE64Struct, *PPE64Struct;

typedef struct{
	bool NewDll;
	int NumberOfImports;
	ULONG_PTR ImageBase;
	ULONG_PTR BaseImportThunk;
	ULONG_PTR ImportThunk;
	char* APIName;
	char* DLLName;
}ImportEnumData, *PImportEnumData;

#define UE_DEPTH_SURFACE 0
#define UE_DEPTH_DEEP 1

#define UE_FIELD_OK 0
#define UE_FIELD_BROKEN_NON_FIXABLE 1
#define UE_FIELD_BROKEN_NON_CRITICAL 2
#define UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE 3
#define UE_FIELD_BROKEN_BUT_CAN_BE_EMULATED 4
#define UE_FILED_FIXABLE_NON_CRITICAL 5
#define UE_FILED_FIXABLE_CRITICAL 6
#define UE_FIELD_NOT_PRESET 7
#define UE_FIELD_NOT_PRESET_WARNING 8

#define UE_RESULT_FILE_OK 10
#define UE_RESULT_FILE_INVALID_BUT_FIXABLE 11
#define UE_RESULT_FILE_INVALID_AND_NON_FIXABLE 12
#define UE_RESULT_FILE_INVALID_FORMAT 13

typedef struct{
	BYTE OveralEvaluation;
	bool EvaluationTerminatedByException;
	bool FileIs64Bit;
	bool FileIsDLL;
	bool FileIsConsole;
	bool MissingDependencies;
	bool MissingDeclaredAPIs;
	BYTE SignatureMZ;
	BYTE SignaturePE;
	BYTE EntryPoint;
	BYTE ImageBase;
	BYTE SizeOfImage;
	BYTE FileAlignment;
	BYTE SectionAlignment;
	BYTE ExportTable;
	BYTE RelocationTable;
	BYTE ImportTable;
	BYTE ImportTableSection;
	BYTE ImportTableData;
	BYTE IATTable;
	BYTE TLSTable;
	BYTE LoadConfigTable;
	BYTE BoundImportTable;
	BYTE COMHeaderTable;
	BYTE ResourceTable;
	BYTE ResourceData;
	BYTE SectionTable;
}FILE_STATUS_INFO, *PFILE_STATUS_INFO;

typedef struct{
	BYTE OveralEvaluation;
	bool FixingTerminatedByException;
	bool FileFixPerformed;
	bool StrippedRelocation;
	bool DontFixRelocations;
	DWORD OriginalRelocationTableAddress;
	DWORD OriginalRelocationTableSize;
	bool StrippedExports;
	bool DontFixExports;
	DWORD OriginalExportTableAddress;
	DWORD OriginalExportTableSize;
	bool StrippedResources;
	bool DontFixResources;
	DWORD OriginalResourceTableAddress;
	DWORD OriginalResourceTableSize;
	bool StrippedTLS;
	bool DontFixTLS;
	DWORD OriginalTLSTableAddress;
	DWORD OriginalTLSTableSize;
	bool StrippedLoadConfig;
	bool DontFixLoadConfig;
	DWORD OriginalLoadConfigTableAddress;
	DWORD OriginalLoadConfigTableSize;
	bool StrippedBoundImports;
	bool DontFixBoundImports;
	DWORD OriginalBoundImportTableAddress;
	DWORD OriginalBoundImportTableSize;
	bool StrippedIAT;
	bool DontFixIAT;
	DWORD OriginalImportAddressTableAddress;
	DWORD OriginalImportAddressTableSize;
	bool StrippedCOM;
	bool DontFixCOM;
	DWORD OriginalCOMTableAddress;
	DWORD OriginalCOMTableSize;
}FILE_FIX_INFO, *PFILE_FIX_INFO;

#define MAX_BREAKPOINTS_PER_LEVEL 40
#define UE_UNPACKER_CURRENT_STATE_INIT 1
#define UE_UNPACKER_CURRENT_STATE_RUNNING 2

typedef struct{
	void* LeafPointer;
	ULONG_PTR BreakAddress;
	int BreakPointType;
}BreakLeafLink, *PBreakLeafLink;

typedef struct{
	bool GrabRelocationFromMemory;
	bool MakeRelocationSnapshoots;
	bool MakeSecondSnapshootAtOEP;
	bool MakeMultipleFirstSnapshoots;
	ULONG_PTR SnapshootMemoryStartRVA;
	DWORD SnapshootMemorySize;
	ULONG_PTR GrabMemoryStartRVA;
	DWORD GrabMemorySize;
}RELOCATION_INFORMATION, *PRELOCATION_INFORMATION;

typedef struct{
	bool TLSBackedUp;
	bool TLSRemoved;
	bool CreatedDependencies;
	bool ForceFakeLibrary;
}FILE_PROCESSING_INFORMATION, *PFILE_PROCESSING_INFORMATION;

typedef struct{
	bool QuarantineTermination;
	ULONG_PTR QuarantineRestoreAddress;
	char QuarantineDLL[256];
	char QuarantineAPI[256];
}FILE_QUARANTINE_INFORMATION, *PFILE_QUARANTINE_INFORMATION;

typedef struct{
	int GlobalError;
	int CurrentState;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	char* szFileName;
	char* szOutputFile;
	char* szOutputFolder;
	bool FoundNextPackingLevel;
	int NextPackingLevelSignatureId;
	char GarbageItem[1024];
	char GarbageFile[1024];
	ULONG_PTR FileLoadBase;
	ULONG_PTR CurrentSearchPosition;
	ULONG_PTR CurrentLeafFoundAt;
	ULONG_PTR UnpackedFileOEP;
	void* CurrentLeafCallBack;
	void* ptrUnpackerHeader;
	void* ptrSignatureHeader;
	int CurrentUnpackerLevel;
#if defined(_WIN64)
	PE64Struct PEStruct;
#else
	PE32Struct PEStruct;
#endif
	PROCESS_INFORMATION ProcessInfo;
	FILE_STATUS_INFO FileStatusInfo;
	FILE_FIX_INFO FileFixStatusInfo;
	FILE_PROCESSING_INFORMATION FileProcessingInfo;
	FILE_QUARANTINE_INFORMATION FileQuarantineInfo;
	RELOCATION_INFORMATION FileRelocationInfo;
	int CurrentLeafLinkNumber;
	BreakLeafLink LeafLink[MAX_BREAKPOINTS_PER_LEVEL];
}BreakPointManagerData, *PBreakPointManagerData;

typedef struct{
	void* AllocatedSection;
	DWORD SectionVirtualOffset;
	DWORD SectionVirtualSize;
	DWORD SectionAttributes;
	DWORD SectionDataHash;
	bool AccessedAlready;
	bool WriteCheckMode;
}TracerSectionData, *PTracerSectionData;

typedef struct{
	int SectionNumber;
	TracerSectionData SectionData[MAXIMUM_SECTION_NUMBER];
	int OriginalEntryPointNum;
	ULONG_PTR OriginalImageBase;
	ULONG_PTR OriginalEntryPoint;
	ULONG_PTR LoadedImageBase;
	ULONG_PTR SizeOfImage;
	ULONG_PTR CurrentIntructionPointer;
	ULONG_PTR MemoryAccessedFrom;
	ULONG_PTR MemoryAccessed;
	ULONG_PTR AccessType;
	void* InitCallBack;
	void* EPCallBack;
	bool FileIsDLL;
	bool FileIs64bit;
}GenericOEPTracerData, *PGenericOEPTracerData;

// UnpackEngine.Handler:

#define NTDLL_SystemHandleInfo 0x10
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef enum _POOL_TYPE {
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS,
	MaxPoolType,
	NonPagedPoolSession,
	PagedPoolSession,
	NonPagedPoolMustSucceedSession,
	DontUseThisTypeSession,
	NonPagedPoolCacheAlignedSession,
	PagedPoolCacheAlignedSession,
	NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

typedef struct _LSA_UNICODE_STRING {  
	USHORT Length; 
	USHORT MaximumLength;  
	PWSTR Buffer;
} LSA_UNICODE_STRING,  *PLSA_UNICODE_STRING,  UNICODE_STRING,  *PUNICODE_STRING;

typedef struct{
	ULONG ProcessId;
	UCHAR ObjectTypeNumber;
	UCHAR Flags; // 0x01 = PROTECT_FROM_CLOSE, 0x02 = INHERIT
	USHORT hHandle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
}NTDLL_QUERY_HANDLE_INFO, *PNTDLL_QUERY_HANDLE_INFO; 

typedef struct _PUBLIC_OBJECT_BASIC_INFORMATION {
	ULONG Attributes;
	ACCESS_MASK GrantedAccess;
	ULONG HandleCount;
	ULONG PointerCount;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
	ULONG Reserved[3];
	ULONG NameInformationLength;
	ULONG TypeInformationLength;
	ULONG SecurityDescriptorLength;
	LARGE_INTEGER CreateTime;
} PUBLIC_OBJECT_BASIC_INFORMATION, *PPUBLIC_OBJECT_BASIC_INFORMATION;

typedef struct _PUBLIC_OBJECT_NAME_INFORMATION { // Information Class 1
	UNICODE_STRING Name;
} PUBLIC_OBJECT_NAME_INFORMATION, *PPUBLIC_OBJECT_NAME_INFORMATION;

typedef struct _PUBLIC_OBJECT_TYPE_INFORMATION { // Information Class 2
	UNICODE_STRING Name;
	ULONG ObjectCount;
	ULONG HandleCount;
	ULONG Reserved1[4];
	ULONG PeakObjectCount;
	ULONG PeakHandleCount;
	ULONG Reserved2[4];
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccess;
	UCHAR Unknown;
	BOOLEAN MaintainHandleDatabase;
	POOL_TYPE PoolType;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
} PUBLIC_OBJECT_TYPE_INFORMATION, *PPUBLIC_OBJECT_TYPE_INFORMATION;