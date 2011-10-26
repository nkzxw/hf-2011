// Engine.Libs:

#if defined(_WIN64)
        #pragma comment(lib, "sdk\\TitanEngine_x64.lib")
#else
        #pragma comment(lib, "sdk\\TitanEngine_x86.lib")
#endif

// Global.Constant.Structure.Declaration:
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

#define UE_ON_LIB_LOAD 1
#define UE_ON_LIB_UNLOAD 2
#define UE_ON_LIB_ALL 3

#define UE_APISTART 0
#define UE_APIEND 1

#define UE_PLATFORM_x86 1
#define UE_PLATFORM_x64 2
#define UE_PLATFORM_ALL 3

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
        WORD TLSTableSize;
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

// Global.Function.Declaration:
// TitanEngine.Dumper.functions:
__declspec(dllexport) bool __stdcall DumpProcess(HANDLE hProcess, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool __stdcall DumpProcessEx(DWORD ProcessId, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool __stdcall DumpMemory(HANDLE hProcess, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName);
__declspec(dllexport) bool __stdcall DumpMemoryEx(DWORD ProcessId, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName);
__declspec(dllexport) bool __stdcall DumpRegions(HANDLE hProcess, char* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool __stdcall DumpRegionsEx(DWORD ProcessId, char* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool __stdcall DumpModule(HANDLE hProcess, LPVOID ModuleBase, char* szDumpFileName);
__declspec(dllexport) bool __stdcall DumpModuleEx(DWORD ProcessId, LPVOID ModuleBase, char* szDumpFileName);
__declspec(dllexport) bool __stdcall PastePEHeader(HANDLE hProcess, LPVOID ImageBase, char* szDebuggedFileName);
__declspec(dllexport) bool __stdcall ExtractSection(char* szFileName, char* szDumpFileName, DWORD SectionNumber);
__declspec(dllexport) bool __stdcall ResortFileSections(char* szFileName);
__declspec(dllexport) bool __stdcall FindOverlay(char* szFileName, LPDWORD OverlayStart, LPDWORD OverlaySize);
__declspec(dllexport) bool __stdcall ExtractOverlay(char* szFileName, char* szExtactedFileName);
__declspec(dllexport) bool __stdcall AddOverlay(char* szFileName, char* szOverlayFileName);
__declspec(dllexport) bool __stdcall CopyOverlay(char* szInFileName, char* szOutFileName);
__declspec(dllexport) bool __stdcall RemoveOverlay(char* szFileName);
__declspec(dllexport) bool __stdcall MakeAllSectionsRWE(char* szFileName);
__declspec(dllexport) long __stdcall AddNewSectionEx(char* szFileName, char* szSectionName, DWORD SectionSize, DWORD SectionAttributes, LPVOID SectionContent, DWORD ContentSize);
__declspec(dllexport) long __stdcall AddNewSection(char* szFileName, char* szSectionName, DWORD SectionSize);
__declspec(dllexport) bool __stdcall ResizeLastSection(char* szFileName, DWORD NumberOfExpandBytes, bool AlignResizeData);
__declspec(dllexport) void __stdcall SetSharedOverlay(char* szFileName);
__declspec(dllexport) char* __stdcall GetSharedOverlay();
__declspec(dllexport) bool __stdcall DeleteLastSection(char* szFileName);
__declspec(dllexport) bool __stdcall DeleteLastSectionEx(char* szFileName, DWORD NumberOfSections);
__declspec(dllexport) long long __stdcall GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData);
__declspec(dllexport) long long __stdcall GetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData);
__declspec(dllexport) bool __stdcall GetPE32DataFromMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage);
__declspec(dllexport) bool __stdcall GetPE32DataEx(char* szFileName, LPVOID DataStorage);
__declspec(dllexport) bool __stdcall SetPE32DataForMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
__declspec(dllexport) bool __stdcall SetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
__declspec(dllexport) bool __stdcall SetPE32DataForMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage);
__declspec(dllexport) bool __stdcall SetPE32DataEx(char* szFileName, LPVOID DataStorage);
__declspec(dllexport) long __stdcall GetPE32SectionNumberFromVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert);
__declspec(dllexport) long long __stdcall ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
__declspec(dllexport) long long __stdcall ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType);
__declspec(dllexport) long long __stdcall ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
__declspec(dllexport) long long __stdcall ConvertFileOffsetToVAEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool ReturnType);
// TitanEngine.Realigner.functions:
__declspec(dllexport) bool __stdcall FixHeaderCheckSum(char* szFileName);
__declspec(dllexport) long __stdcall RealignPE(ULONG_PTR FileMapVA, DWORD FileSize, DWORD RealingMode);
__declspec(dllexport) long __stdcall RealignPEEx(char* szFileName, DWORD RealingFileSize, DWORD ForcedFileAlignment);
__declspec(dllexport) bool __stdcall WipeSection(char* szFileName, int WipeSectionNumber, bool RemovePhysically);
__declspec(dllexport) bool __stdcall IsPE32FileValidEx(char* szFileName, DWORD CheckDepth, LPVOID FileStatusInfo);
__declspec(dllexport) bool __stdcall FixBrokenPE32FileEx(char* szFileName, LPVOID FileStatusInfo, LPVOID FileFixInfo);
__declspec(dllexport) bool __stdcall IsFileDLL(char* szFileName, ULONG_PTR FileMapVA);
// TitanEngine.Hider.functions:
__declspec(dllexport) bool __stdcall HideDebugger(HANDLE hThread, HANDLE hProcess, DWORD PatchAPILevel);
// TitanEngine.Relocater.functions:
__declspec(dllexport) void __stdcall RelocaterCleanup();
__declspec(dllexport) void __stdcall RelocaterInit(DWORD MemorySize, ULONG_PTR OldImageBase, ULONG_PTR NewImageBase);
__declspec(dllexport) void __stdcall RelocaterAddNewRelocation(HANDLE hProcess, ULONG_PTR RelocateAddress, DWORD RelocateState);
__declspec(dllexport) long __stdcall RelocaterEstimatedSize();
__declspec(dllexport) bool __stdcall RelocaterExportRelocation(ULONG_PTR StorePlace, DWORD StorePlaceRVA, ULONG_PTR FileMapVA);
__declspec(dllexport) bool __stdcall RelocaterExportRelocationEx(char* szFileName, char* szSectionName, ULONG_PTR StorePlace, DWORD StorePlaceRVA);
__declspec(dllexport) bool __stdcall RelocaterGrabRelocationTable(HANDLE hProcess, ULONG_PTR MemoryStart, DWORD MemorySize);
__declspec(dllexport) bool __stdcall RelocaterGrabRelocationTableEx(HANDLE hProcess, ULONG_PTR MemoryStart, ULONG_PTR MemorySize, DWORD NtSizeOfImage);
__declspec(dllexport) bool __stdcall RelocaterMakeSnapshot(HANDLE hProcess, char* szSaveFileName, LPVOID MemoryStart, ULONG_PTR MemorySize);
__declspec(dllexport) bool __stdcall RelocaterCompareTwoSnapshots(HANDLE hProcess, ULONG_PTR LoadedImageBase, ULONG_PTR NtSizeOfImage, char* szDumpFile1, char* szDumpFile2, ULONG_PTR MemStart);
__declspec(dllexport) bool __stdcall RelocaterChangeFileBase(char* szFileName, ULONG_PTR NewImageBase);
__declspec(dllexport) bool __stdcall RelocaterWipeRelocationTable(char* szFileName);
// TitanEngine.Resourcer.functions:
__declspec(dllexport) long long __stdcall ResourcerLoadFileForResourceUse(char* szFileName);
__declspec(dllexport) bool __stdcall ResourcerFreeLoadedFile(LPVOID LoadedFileBase);
__declspec(dllexport) bool __stdcall ResourcerExtractResourceFromFileEx(ULONG_PTR FileMapVA, char* szResourceType, char* szResourceName, char* szExtractedFileName);
__declspec(dllexport) bool __stdcall ResourcerExtractResourceFromFile(char* szFileName, char* szResourceType, char* szResourceName, char* szExtractedFileName);
// TitanEngine.Threader.functions:
__declspec(dllexport) void* __stdcall ThreaderGetThreadInfo(HANDLE hThread, DWORD ThreadId);
__declspec(dllexport) void __stdcall ThreaderEnumThreadInfo(void* EnumCallBack);
__declspec(dllexport) bool __stdcall ThreaderPauseThread(HANDLE hThread);
__declspec(dllexport) bool __stdcall ThreaderResumeThread(HANDLE hThread);
__declspec(dllexport) bool __stdcall ThreaderTerminateThread(HANDLE hThread, DWORD ThreadExitCode);
__declspec(dllexport) bool __stdcall ThreaderPauseAllThreads(bool LeaveMainRunning);
__declspec(dllexport) bool __stdcall ThreaderResumeAllThreads(bool LeaveMainPaused);
__declspec(dllexport) bool __stdcall ThreaderPauseProcess();
__declspec(dllexport) bool __stdcall ThreaderResumeProcess();
__declspec(dllexport) long long __stdcall ThreaderCreateRemoteThread(ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId);
__declspec(dllexport) bool __stdcall ThreaderInjectAndExecuteCode(LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize);
__declspec(dllexport) long long __stdcall ThreaderCreateRemoteThreadEx(HANDLE hProcess, ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId);
__declspec(dllexport) bool __stdcall ThreaderInjectAndExecuteCodeEx(HANDLE hProcess, LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize);
__declspec(dllexport) void __stdcall ThreaderSetCallBackForNextExitThreadEvent(LPVOID exitThreadCallBack);
__declspec(dllexport) bool __stdcall ThreaderIsThreadStillRunning(HANDLE hThread);
__declspec(dllexport) bool __stdcall ThreaderIsThreadActive(HANDLE hThread);
__declspec(dllexport) bool __stdcall ThreaderIsAnyThreadActive();
__declspec(dllexport) bool __stdcall ThreaderExecuteOnlyInjectedThreads();
__declspec(dllexport) long long __stdcall ThreaderGetOpenHandleForThread(DWORD ThreadId);
__declspec(dllexport) void* __stdcall ThreaderGetThreadData();
__declspec(dllexport) bool __stdcall ThreaderIsExceptionInMainThread();
// TitanEngine.Debugger.functions:
__declspec(dllexport) void* __stdcall StaticDisassembleEx(ULONG_PTR DisassmStart, LPVOID DisassmAddress);
__declspec(dllexport) void* __stdcall StaticDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) void* __stdcall DisassembleEx(HANDLE hProcess, LPVOID DisassmAddress);
__declspec(dllexport) void* __stdcall Disassemble(LPVOID DisassmAddress);
__declspec(dllexport) long __stdcall StaticLengthDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) long __stdcall LengthDisassembleEx(HANDLE hProcess, LPVOID DisassmAddress);
__declspec(dllexport) long __stdcall LengthDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) void* __stdcall InitDebug(char* szFileName, char* szCommandLine, char* szCurrentFolder);
__declspec(dllexport) void* __stdcall InitDebugEx(char* szFileName, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) void* __stdcall InitDLLDebug(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) bool __stdcall StopDebug();
__declspec(dllexport) void __stdcall SetBPXOptions(long DefaultBreakPointType);
__declspec(dllexport) bool __stdcall IsBPXEnabled(ULONG_PTR bpxAddress);
__declspec(dllexport) bool __stdcall EnableBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool __stdcall DisableBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool __stdcall SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall SetBPXEx(ULONG_PTR bpxAddress, DWORD bpxType, DWORD NumberOfExecution, DWORD CmpRegister, DWORD CmpCondition, ULONG_PTR CmpValue, LPVOID bpxCallBack, LPVOID bpxCompareCallBack, LPVOID bpxRemoveCallBack);
__declspec(dllexport) bool __stdcall DeleteBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool __stdcall SafeDeleteBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool __stdcall SetAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxType, DWORD bpxPlace, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall DeleteAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxPlace);
__declspec(dllexport) bool __stdcall SafeDeleteAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxPlace);
__declspec(dllexport) bool __stdcall SetMemoryBPX(ULONG_PTR MemoryStart, DWORD SizeOfMemory, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall SetMemoryBPXEx(ULONG_PTR MemoryStart, DWORD SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall RemoveMemoryBPX(ULONG_PTR MemoryStart, DWORD SizeOfMemory);
__declspec(dllexport) long long __stdcall GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister);
__declspec(dllexport) long long __stdcall GetContextData(DWORD IndexOfRegister);
__declspec(dllexport) bool __stdcall SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue);
__declspec(dllexport) bool __stdcall SetContextData(DWORD IndexOfRegister, ULONG_PTR NewRegisterValue);
__declspec(dllexport) void __stdcall ClearExceptionNumber();
__declspec(dllexport) long __stdcall CurrentExceptionNumber();
__declspec(dllexport) long long __stdcall FindEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard);
__declspec(dllexport) long long __stdcall Find(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard);
__declspec(dllexport) bool __stdcall FillEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte);
__declspec(dllexport) bool __stdcall Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte);
__declspec(dllexport) bool __stdcall PatchEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP);
__declspec(dllexport) bool __stdcall Patch(LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP);
__declspec(dllexport) bool __stdcall ReplaceEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard);
__declspec(dllexport) bool __stdcall Replace(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard);
__declspec(dllexport) void* __stdcall GetDebugData();
__declspec(dllexport) void* __stdcall GetTerminationData();
__declspec(dllexport) long __stdcall GetExitCode();
__declspec(dllexport) long long __stdcall GetDebuggedDLLBaseAddress();
__declspec(dllexport) long long __stdcall GetDebuggedFileBaseAddress();
__declspec(dllexport) bool __stdcall GetRemoteString(HANDLE hProcess, LPVOID StringAddress, LPVOID StringStorage, int MaximumStringSize);
__declspec(dllexport) long long __stdcall GetFunctionParameter(HANDLE hProcess, DWORD FunctionType, DWORD ParameterNumber, DWORD ParameterType);
__declspec(dllexport) long long __stdcall GetJumpDestinationEx(HANDLE hProcess, ULONG_PTR InstructionAddress, bool JustJumps);
__declspec(dllexport) long long __stdcall GetJumpDestination(HANDLE hProcess, ULONG_PTR InstructionAddress);
__declspec(dllexport) bool __stdcall IsJumpGoingToExecuteEx(HANDLE hProcess, HANDLE hThread, ULONG_PTR InstructionAddress, ULONG_PTR RegFlags);
__declspec(dllexport) bool __stdcall IsJumpGoingToExecute();
__declspec(dllexport) void __stdcall SetCustomHandler(DWORD ExceptionId, LPVOID CallBack);
__declspec(dllexport) void __stdcall ForceClose();
__declspec(dllexport) void __stdcall StepInto(LPVOID traceCallBack);
__declspec(dllexport) void __stdcall StepOver(LPVOID traceCallBack);
__declspec(dllexport) void __stdcall SingleStep(DWORD StepCount, LPVOID StepCallBack);
__declspec(dllexport) bool __stdcall SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall DeleteHardwareBreakPoint(DWORD IndexOfRegister);
__declspec(dllexport) bool __stdcall RemoveAllBreakPoints(DWORD RemoveOption);
__declspec(dllexport) void* __stdcall GetProcessInformation();
__declspec(dllexport) void* __stdcall GetStartupInformation();
__declspec(dllexport) void __stdcall DebugLoop();
__declspec(dllexport) void __stdcall SetDebugLoopTimeOut(DWORD TimeOut);
__declspec(dllexport) void __stdcall SetNextDbgContinueStatus(DWORD SetDbgCode);
__declspec(dllexport) bool __stdcall AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack);
__declspec(dllexport) bool __stdcall DetachDebugger(DWORD ProcessId);
__declspec(dllexport) bool __stdcall DetachDebuggerEx(DWORD ProcessId);
__declspec(dllexport) void __stdcall DebugLoopEx(DWORD TimeOut);
__declspec(dllexport) void __stdcall AutoDebugEx(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, DWORD TimeOut, LPVOID EntryCallBack);
// TitanEngine.FindOEP.functions:
__declspec(dllexport) void __stdcall FindOEPInit();
__declspec(dllexport) bool __stdcall FindOEPviaModule(HMODULE hModuleHandle, LPVOID CallBack);
__declspec(dllexport) bool __stdcall FindOEPGenerically(char* szFileName, LPVOID TraceInitCallBack, LPVOID CallBack);
// TitanEngine.Importer.functions:
__declspec(dllexport) void __stdcall ImporterCleanup();
__declspec(dllexport) void __stdcall ImporterSetImageBase(ULONG_PTR ImageBase);
__declspec(dllexport) void __stdcall ImporterSetUnknownDelta(ULONG_PTR DeltaAddress);
__declspec(dllexport) long long __stdcall ImporterGetCurrentDelta();
__declspec(dllexport) void __stdcall ImporterInit(DWORD MemorySize, ULONG_PTR ImageBase);
__declspec(dllexport) void __stdcall ImporterAddNewDll(char* szDLLName, ULONG_PTR FirstThunk);
__declspec(dllexport) void __stdcall ImporterAddNewAPI(char* szAPIName, ULONG_PTR ThunkValue);
__declspec(dllexport) long __stdcall ImporterGetAddedDllCount();
__declspec(dllexport) long __stdcall ImporterGetAddedAPICount();
__declspec(dllexport) void __stdcall ImporterMoveIAT();
__declspec(dllexport) bool __stdcall ImporterExportIAT(ULONG_PTR StorePlace, ULONG_PTR FileMapVA);
__declspec(dllexport) long __stdcall ImporterEstimatedSize();
__declspec(dllexport) bool __stdcall ImporterExportIATEx(char* szExportFileName, char* szSectionName);
__declspec(dllexport) long long __stdcall ImporterFindAPIWriteLocation(char* szAPIName);
__declspec(dllexport) long long __stdcall ImporterFindAPIByWriteLocation(ULONG_PTR APIWriteLocation);
__declspec(dllexport) long long __stdcall ImporterFindDLLByWriteLocation(ULONG_PTR APIWriteLocation);
__declspec(dllexport) void* __stdcall ImporterGetAPIName(ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetAPINameEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) long long __stdcall ImporterGetRemoteAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) long long __stdcall ImporterGetRemoteAPIAddressEx(char* szDLLName, char* szAPIName);
__declspec(dllexport) long long __stdcall ImporterGetLocalAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetDLLNameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetAPINameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) long __stdcall ImporterGetDLLIndexEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) long __stdcall ImporterGetDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) long long __stdcall ImporterGetRemoteDLLBase(HANDLE hProcess, HMODULE LocalModuleBase);
__declspec(dllexport) bool __stdcall ImporterRelocateWriteLocation(ULONG_PTR AddValue);
__declspec(dllexport) bool __stdcall ImporterIsForewardedAPI(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetForewardedAPIName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetForewardedDLLName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) long __stdcall ImporterGetForewardedDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) long long __stdcall ImporterGetNearestAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* __stdcall ImporterGetNearestAPIName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) bool __stdcall ImporterCopyOriginalIAT(char* szOriginalFile, char* szDumpFile);
__declspec(dllexport) bool __stdcall ImporterLoadImportTable(char* szFileName);
__declspec(dllexport) bool __stdcall ImporterMoveOriginalIAT(char* szOriginalFile, char* szDumpFile, char* szSectionName);
__declspec(dllexport) void __stdcall ImporterAutoSearchIAT(HANDLE hProcess, char* szFileName, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize);
__declspec(dllexport) void __stdcall ImporterAutoSearchIATEx(HANDLE hProcess, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize);
__declspec(dllexport) void __stdcall ImporterEnumAddedData(LPVOID EnumCallBack);
__declspec(dllexport) long __stdcall ImporterAutoFixIATEx(HANDLE hProcess, char* szDumpedFile, char* szSectionName, bool DumpRunningProcess, bool RealignFile, ULONG_PTR EntryPointAddress, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep, bool TryAutoFix, bool FixEliminations, LPVOID UnknownPointerFixCallback);
__declspec(dllexport) long __stdcall ImporterAutoFixIAT(HANDLE hProcess, char* szDumpedFile, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep);
// Global.Engine.Tracer.functions:
__declspec(dllexport) void __stdcall TracerInit();
__declspec(dllexport) long long __stdcall TracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace);
__declspec(dllexport) long long __stdcall HashTracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD InputNumberOfInstructions);
__declspec(dllexport) long __stdcall TracerDetectRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace);
__declspec(dllexport) long long __stdcall TracerFixKnownRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD RedirectionId);
__declspec(dllexport) long long __stdcall TracerFixRedirectionViaModule(HMODULE hModuleHandle, HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD IdParameter);
__declspec(dllexport) long long __stdcall TracerDetectRedirectionViaModule(HMODULE hModuleHandle, HANDLE hProcess, ULONG_PTR AddressToTrace, PDWORD ReturnedId);
__declspec(dllexport) long __stdcall TracerFixRedirectionViaImpRecPlugin(HANDLE hProcess, char* szPluginName, ULONG_PTR AddressToTrace);
// TitanEngine.Exporter.functions:
__declspec(dllexport) void __stdcall ExporterCleanup();
__declspec(dllexport) void __stdcall ExporterSetImageBase(ULONG_PTR ImageBase);
__declspec(dllexport) void __stdcall ExporterInit(DWORD MemorySize, ULONG_PTR ImageBase, DWORD ExportOrdinalBase, char* szExportModuleName);
__declspec(dllexport) bool __stdcall ExporterAddNewExport(char* szExportName, DWORD ExportRelativeAddress);
__declspec(dllexport) bool __stdcall ExporterAddNewOrdinalExport(DWORD OrdinalNumber, DWORD ExportRelativeAddress);
__declspec(dllexport) long __stdcall ExporterGetAddedExportCount();
__declspec(dllexport) long __stdcall ExporterEstimatedSize();
__declspec(dllexport) bool __stdcall ExporterBuildExportTable(ULONG_PTR StorePlace, ULONG_PTR FileMapVA);
__declspec(dllexport) bool __stdcall ExporterBuildExportTableEx(char* szExportFileName, char* szSectionName);
__declspec(dllexport) bool __stdcall ExporterLoadExportTable(char* szFileName);
// TitanEngine.Librarian.functions:
__declspec(dllexport) bool __stdcall LibrarianSetBreakPoint(char* szLibraryName, DWORD bpxType, bool SingleShoot, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall LibrarianRemoveBreakPoint(char* szLibraryName, DWORD bpxType);
__declspec(dllexport) void* __stdcall LibrarianGetLibraryInfo(char* szLibraryName);
__declspec(dllexport) void* __stdcall LibrarianGetLibraryInfoEx(void* BaseOfDll);
__declspec(dllexport) void __stdcall LibrarianEnumLibraryInfo(void* EnumCallBack);
// TitanEngine.Process.functions:
__declspec(dllexport) long __stdcall GetActiveProcessId(char* szImageName);
__declspec(dllexport) void __stdcall EnumProcessesWithLibrary(char* szLibraryName, void* EnumFunction);
// TitanEngine.TLSFixer.functions:
__declspec(dllexport) bool __stdcall TLSBreakOnCallBack(LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall TLSGrabCallBackData(char* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks);
__declspec(dllexport) bool __stdcall TLSBreakOnCallBackEx(char* szFileName, LPVOID bpxCallBack);
__declspec(dllexport) bool __stdcall TLSRemoveCallback(char* szFileName);
__declspec(dllexport) bool __stdcall TLSRemoveTable(char* szFileName);
__declspec(dllexport) bool __stdcall TLSBackupData(char* szFileName);
__declspec(dllexport) bool __stdcall TLSRestoreData();
__declspec(dllexport) bool __stdcall TLSBuildNewTable(ULONG_PTR FileMapVA, ULONG_PTR StorePlace, ULONG_PTR StorePlaceRVA, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks);
__declspec(dllexport) bool __stdcall TLSBuildNewTableEx(char* szFileName, char* szSectionName, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks);
// TitanEngine.TranslateName.functions:
__declspec(dllexport) void* __stdcall TranslateNativeName(char* szNativeName);
// TitanEngine.Handler.functions:
__declspec(dllexport) long __stdcall HandlerGetActiveHandleCount(DWORD ProcessId);
__declspec(dllexport) bool __stdcall HandlerIsHandleOpen(DWORD ProcessId, HANDLE hHandle);
__declspec(dllexport) void* __stdcall HandlerGetHandleName(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, bool TranslateName);
__declspec(dllexport) long __stdcall HandlerEnumerateOpenHandles(DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount);
__declspec(dllexport) long long __stdcall HandlerGetHandleDetails(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, DWORD InformationReturn);
__declspec(dllexport) bool __stdcall HandlerCloseRemoteHandle(HANDLE hProcess, HANDLE hHandle);
__declspec(dllexport) long __stdcall HandlerEnumerateLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated, LPVOID HandleDataBuffer, DWORD MaxHandleCount);
__declspec(dllexport) bool __stdcall HandlerCloseAllLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
__declspec(dllexport) bool __stdcall HandlerIsFileLocked(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
// TitanEngine.Handler[Mutex].functions:
__declspec(dllexport) long __stdcall HandlerEnumerateOpenMutexes(HANDLE hProcess, DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount);
__declspec(dllexport) long long __stdcall HandlerGetOpenMutexHandle(HANDLE hProcess, DWORD ProcessId, char* szMutexString);
__declspec(dllexport) long __stdcall HandlerGetProcessIdWhichCreatedMutex(char* szMutexString);
// TitanEngine.Injector.functions:
__declspec(dllexport) bool __stdcall RemoteLoadLibrary(HANDLE hProcess, char* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool __stdcall RemoteFreeLibrary(HANDLE hProcess, HMODULE hModule, char* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool __stdcall RemoteExitProcess(HANDLE hProcess, DWORD ExitCode);
// TitanEngine.StaticUnpacker.functions:
__declspec(dllexport) bool __stdcall StaticFileLoad(char* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, LPDWORD FileMapVA);
__declspec(dllexport) bool __stdcall StaticFileUnload(char* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, DWORD FileMapVA);
__declspec(dllexport) void __stdcall StaticMemoryDecrypt(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey);
__declspec(dllexport) void __stdcall StaticMemoryDecryptEx(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionKeySize, void* DecryptionCallBack);
__declspec(dllexport) void __stdcall StaticSectionDecrypt(ULONG_PTR FileMapVA, DWORD SectionNumber, bool SimulateLoad, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey);
// TitanEngine.Engine.functions:
__declspec(dllexport) void __stdcall SetEngineVariable(DWORD VariableId, bool VariableSet);