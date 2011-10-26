//-----------------------------------------------------------------------------------------
//
// Nyx.functions
//
//-----------------------------------------------------------------------------------------

__declspec(dllexport) long __stdcall NyxOpenArchive(char* szFileName, DWORD OpenMode);
__declspec(dllexport) long __stdcall NyxOpenArchiveW(wchar_t* szFileName, DWORD OpenMode);
__declspec(dllexport) bool __stdcall NyxIdentifyArchive(long NyxFileHandle);
__declspec(dllexport) char* __stdcall NyxGetArchiveName(long NyxFileHandle);
__declspec(dllexport) void __stdcall NyxScanArchive(long NyxFileHandle, void* NyxReportCallBack);
__declspec(dllexport) void __stdcall NyxInspectArchive(long NyxFileHandle, void* NyxReportCallBack);
__declspec(dllexport) bool __stdcall NyxValidateArchive(long NyxFileHandle, LPDWORD NyxArchiveValidation);
__declspec(dllexport) bool __stdcall NyxExtractFileSlice(long NyxFileHandle, char* szExtractedFile, DWORD nyxDataStart, DWORD nyxDataSize);
__declspec(dllexport) bool __stdcall NyxExtractFileSliceW(long NyxFileHandle, wchar_t* szExtractedFile, DWORD nyxDataStart, DWORD nyxDataSize);
__declspec(dllexport) long __stdcall NyxRecoverFile(long NyxFileHandle, int NyxFileId, char* szExtractedFile);
__declspec(dllexport) long __stdcall NyxRecoverFileW(long NyxFileHandle, int NyxFileId, wchar_t* szExtractedFile);
__declspec(dllexport) char* __stdcall NyxGetReportedIssueDescription(int NyxEncodedReportID);
__declspec(dllexport) char* __stdcall NyxGetReportedIssueRichDescription(int NyxEncodedReportRichID);
__declspec(dllexport) bool __stdcall NyxGetArchiveProperties(long NyxFileHandle, void* NyxArchiveInfo);
__declspec(dllexport) bool __stdcall NyxGetSelectedFile(long NyxFileHandle, int NyxFileId, void* NyxFileInfo);
__declspec(dllexport) bool __stdcall NyxGetPreviousFile(long NyxFileHandle, void* NyxFileInfo);
__declspec(dllexport) bool __stdcall NyxGetNextFile(long NyxFileHandle, void* NyxFileInfo);
__declspec(dllexport) bool __stdcall NyxSetFileSegment(long NyxFileHandle, DWORD64 nyxSegmentStart, DWORD64 nyxSegmentSize);
__declspec(dllexport) bool __stdcall NyxResetFileCurrsor(long NyxFileHandle);
__declspec(dllexport) void __stdcall NyxCloseArchive(long NyxFileHandle);

//-----------------------------------------------------------------------------------------
//
// Nyx.structures
//
//-----------------------------------------------------------------------------------------

#define NYX_ARCHIVE_ZIP 1
#define NYX_ARCHIVE_ZIP64 2
#define NYX_ARCHIVE_CAB 3
#define NYX_ARCHIVE_GZIP 4
#define NYX_ARCHIVE_RAR 5

#define NYX_VALIDATION_FILE_VALID 1
#define NYX_VALIDATION_FILE_BROKEN 2
#define NYX_VALIDATION_FILE_BROKEN_BUT_FIXABLE 3

#define NYX_SELECT_CURRENT_FILE -1
#define NYX_SELECT_PREVIOUS_FILE -2
#define NYX_SELECT_NEXT_FILE -3

#define NYX_RECOVER_ERROR -1
#define NYX_RECOVER_SUCCESS 0
#define NYX_RECOVER_NOT_SUPPORTED 1

#define NYX_VULN_FILENAME_TOO_LONG 1
#define NYX_VULN_SUSPICIOUS_COMPRESSION_RATIO 2
#define NYX_STEGO_FILENAME_UNPRINTABLE 3
#define NYX_VULN_FILENAME_TOO_SHORT 4
#define NYX_STEGO_SUSPICIOUS_UNPRINTABLE_DATA 5
#define NYX_STEGO_SUSPICIOUS_PRINTABLE_DATA 6
#define NYX_VULN_EXTRACT_VER_REQUIREMENT_SUSPICIOUS 7
#define NYX_CORRUPTION_CANNOT_ACCESS_DATA 8
#define NYX_CORRUPTION_CHECKSUM_MISMATCH 9
#define NYX_STEGO_UNREPORTED_FILE_FOUND 10
#define NYX_STEGO_FOUND_CLOAKED_DATA 11
#define NYX_VULN_INCORRECT_HEADER_DATA 12
#define NYX_VULN_CHECKSUM_NOT_SET 13
#define NYX_VULN_POSSIBLE_MULTIDISK_TAMPERING 14
#define NYX_VULN_FILE_WILL_EXECUTE_UPON_EXTRACTION 15
#define NYX_VULN_DUPLICATED_FILE_NAME 16

#define NYX_MAX_VULNERABILITIES 32
#define NYX_MAX_STEGANOGRAPHY 32
#define NYX_MAX_CORRUPTIONS 32

typedef struct NYX_INSPECT_ENTRY{
	bool nyxDetectedVulnerability;
	bool nyxDetectedSteganography;
	bool nyxDetectedCorruptions;
	DWORD nyxReportedInfoId;
	DWORD nyxReportedRichInfoId;
	DWORD64 nyxReportedDataStart;
	DWORD nyxReportedDataSize;
	bool nyxIsDetectedAnomalyFixable;
}NYX_INSPECT_ENTRY, *PNYX_INSPECT_ENTRY;

typedef struct NYX_INFO_ENTRY{
	DWORD nyxReportedInfoId;
	DWORD nyxReportedRichInfoId;
	DWORD64 nyxReportedDataStart;
	DWORD nyxReportedDataSize;
	bool nyxIsDetectedAnomalyFixable;
}NYX_INFO_ENTRY, *PNYX_INFO_ENTRY;

typedef struct NYX_FILE_ARCHIVE_INFORMATION{
	DWORD nyxArchiveType;
	DWORD nyxTotalDiskNumber;
	DWORD nyxCurrentDiskNumber;
	DWORD nyxNumberOfFilesInCurrentDisk;
	DWORD nyxTotalNumberOfFilesInAllDisks;
	bool nyxDetectedFileIsInOverlay;
	DWORD64 nyxArchiveHeaderLocation;
	DWORD64 nyxArchiveHeaderStart;
}NYX_FILE_ARCHIVE_INFORMATION, *PNYX_FILE_ARCHIVE_INFORMATION;

typedef struct NYX_FILE_INFO{
	DWORD nyxCurrentFileId;
	long nyxCompressionAlgorithm;
	DWORD64 nyxPackedContentHeader;
	DWORD64 nyxPackedContentLocation;
	DWORD64 nyxPackedContentSize;
	bool nyxFileNameIsUTF;
	char nyxFileName[MAX_PATH];
	DWORD nyxCompressedSize;
	DWORD nyxUncompressedSize;
	DWORD nyxFileStartsOnDiskID;
	DWORD nyxFileChecksum;
	DWORD nyxFileCreationTime;
	DWORD nyxFileCreationDate;
	DWORD nyxFileAttributes;
	bool nyxFileIsPasswordProtected;
	bool nyxDetectedVulnerability;
	bool nyxDetectedSteganography;
	bool nyxDetectedCorruptions;
	NYX_INFO_ENTRY nyxVulnerabilityIDs[NYX_MAX_VULNERABILITIES];
	NYX_INFO_ENTRY nyxSteganographyIDs[NYX_MAX_STEGANOGRAPHY];
	NYX_INFO_ENTRY nyxCorruptionIDs[NYX_MAX_CORRUPTIONS];
}NYX_FILE_INFO, *PNYX_FILE_INFO;

//-----------------------------------------------------------------------------------------