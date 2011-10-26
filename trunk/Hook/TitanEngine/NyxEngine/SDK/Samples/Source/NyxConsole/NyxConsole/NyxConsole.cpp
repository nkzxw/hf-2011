// NyxConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "NyxSDK.h"
#pragma comment(lib, "NyxEngine_x86.lib")

bool bReportBannerShown = false;
bool bInteractiveScan = false;

void __stdcall nyxReportFileStructureInfo(long nyxOpenFileHandle, PNYX_INSPECT_ENTRY NyxInfoEntry){

	char cResponse[10] = {};
	char szFileName[MAX_PATH] = {};

	if(!bReportBannerShown){
		printf(" [x] Detailed archive inspection:\r\n");
		bReportBannerShown = true;
	}
	if(NyxInfoEntry->nyxDetectedSteganography){
		printf("      [!] Detected steganography:\r\n");
		printf("          Steganography ID: %#08x\r\n", NyxInfoEntry->nyxReportedInfoId);
		printf("           Description: %s\r\n", NyxGetReportedIssueDescription(NyxInfoEntry->nyxReportedInfoId));
		if(NyxInfoEntry->nyxReportedRichInfoId != NULL){
			printf("           Rich Description: %s\r\n", NyxGetReportedIssueRichDescription(NyxInfoEntry->nyxReportedRichInfoId));
		}
		printf("            Data start: %#I64x; Data size: %#08x\r\n", NyxInfoEntry->nyxReportedDataStart, NyxInfoEntry->nyxReportedDataSize);
		if(bInteractiveScan == true && NyxInfoEntry->nyxReportedInfoId == NYX_STEGO_UNREPORTED_FILE_FOUND){
			printf("            Recover this file (Y/N)? ");
			scanf_s("%s",cResponse);
			if(cResponse[0] == 89 || cResponse[0] == 121){
				printf("            Enter recovered file name: ");
				scanf_s("%s",szFileName);
				if(NyxRecoverFileEx(nyxOpenFileHandle, (DWORD)NyxInfoEntry->nyxReportedDataStart, (DWORD)NyxInfoEntry->nyxReportedDataSize, szFileName) == NYX_RECOVER_SUCCESS){
					printf("               [x] File was successfully recovered!\r\n");
				}else{
					printf("               [!] File was not successfully recovered!\r\n");
				}
			}else{
				printf("\r\n");
			}
		}
	}else if(NyxInfoEntry->nyxDetectedVulnerability){
		printf("      [!] Detected vulnerability:\r\n");
		printf("          Vulnerability ID: %#08x\r\n", NyxInfoEntry->nyxReportedInfoId);
		printf("           Description: %s\r\n", NyxGetReportedIssueDescription(NyxInfoEntry->nyxReportedInfoId));
		if(NyxInfoEntry->nyxReportedRichInfoId != NULL){
			printf("           Rich Description: %s\r\n", NyxGetReportedIssueRichDescription(NyxInfoEntry->nyxReportedRichInfoId));
		}
		printf("            Data start: %#I64x; Data size: %#08x\r\n", NyxInfoEntry->nyxReportedDataStart, NyxInfoEntry->nyxReportedDataSize);
	}else if(NyxInfoEntry->nyxDetectedCorruptions){
		printf("      [!] Detected corruption:\r\n");
		printf("          Corruption ID: %#08x\r\n", NyxInfoEntry->nyxReportedInfoId);
		printf("           Description: %s\r\n", NyxGetReportedIssueDescription(NyxInfoEntry->nyxReportedInfoId));
		if(NyxInfoEntry->nyxReportedRichInfoId != NULL){
			printf("           Rich Description: %s\r\n", NyxGetReportedIssueRichDescription(NyxInfoEntry->nyxReportedRichInfoId));
		}
		printf("            Data start: %#I64x; Data size: %#08x\r\n", NyxInfoEntry->nyxReportedDataStart, NyxInfoEntry->nyxReportedDataSize);
	}
}

int _tmain(int argc, _TCHAR* argv[]){

	int i = NULL;
	int j = NULL;
	DWORD nyxValidation = NULL;
	long nyxOpenFileHandle = NULL;
	NYX_FILE_ARCHIVE_INFORMATION nyxArchiveInfo = {};
	NYX_FILE_INFO nyxFileInfo = {};

	bReportBannerShown = false;
	printf(" -- NyxConsole 1.0 from ReversingLabs Corporation --\r\n    www.reversinglabs.com\r\n\r\nUsage NyxConsole [sf|sfi|vf|rf] InputArchive.ext\r\n\r\n");
	if(lstrcmpiW(argv[1], L"sf") == NULL || lstrcmpiW(argv[1], L"sfi") == NULL){
		if(lstrcmpiW(argv[1], L"sfi") == NULL){bInteractiveScan = true;}
		nyxOpenFileHandle = NyxOpenArchiveW((wchar_t*)argv[2], GENERIC_READ);
		if(nyxOpenFileHandle != NULL){
			if(NyxIdentifyArchive(nyxOpenFileHandle) && NyxGetArchiveProperties(nyxOpenFileHandle, &nyxArchiveInfo)){
				printf("[x] Detected archive: %s\r\n", NyxGetArchiveName(nyxOpenFileHandle));
				printf(" [x] Total number of disks: %04i\r\n", nyxArchiveInfo.nyxTotalDiskNumber);
				printf(" [x] Current disk number: %04i\r\n", nyxArchiveInfo.nyxCurrentDiskNumber);
				printf(" [x] Total number of files: %08i\r\n", nyxArchiveInfo.nyxTotalNumberOfFilesInAllDisks);
				printf(" [x] Number of files in current disk: %08i\r\n", nyxArchiveInfo.nyxNumberOfFilesInCurrentDisk);
				while(NyxGetNextFile(nyxOpenFileHandle, &nyxFileInfo)){
					printf("  [+] File %04i: %s\r\n", i, nyxFileInfo.nyxFileName);
					printf("      Compressed size: %08X\r\n", nyxFileInfo.nyxCompressedSize);
					printf("      Uncompressed size: %08X\r\n", nyxFileInfo.nyxUncompressedSize);
					printf("      File starts on disk: %04i\r\n", nyxFileInfo.nyxFileStartsOnDiskID);
					printf("      Checksum: %#08x\r\n", nyxFileInfo.nyxFileChecksum);
					if(nyxFileInfo.nyxFileIsPasswordProtected){
						printf("      File is password protected!\r\n");
					}
					if(nyxFileInfo.nyxDetectedVulnerability){
						j = NULL; printf("      [!] Detected vulnerabilities:\r\n");
						while(nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedInfoId != NULL){
							printf("          Vulnerability ID: %#08x\r\n", nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedInfoId);
							printf("           Description: %s\r\n", NyxGetReportedIssueDescription(nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedInfoId));
							if(nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedRichInfoId != NULL){
								printf("           Rich Description: %s\r\n", NyxGetReportedIssueRichDescription(nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedRichInfoId));
							}
							if(nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedDataStart != NULL && nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedDataSize != NULL){
								printf("            Data start: %#I64x; Data size: %#08x\r\n", nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedDataStart, nyxFileInfo.nyxVulnerabilityIDs[j].nyxReportedDataSize);
							}
							j++;
						}
					}
					if(nyxFileInfo.nyxDetectedSteganography){
						j = NULL; printf("      [!] Detected steganography:\r\n");
						while(nyxFileInfo.nyxSteganographyIDs[j].nyxReportedInfoId != NULL){
							printf("          Steganography ID: %#08x\r\n", nyxFileInfo.nyxSteganographyIDs[j].nyxReportedInfoId);
							printf("           Description: %s\r\n", NyxGetReportedIssueDescription(nyxFileInfo.nyxSteganographyIDs[j].nyxReportedInfoId));
							if(nyxFileInfo.nyxSteganographyIDs[j].nyxReportedRichInfoId != NULL){
								printf("           Rich Description: %s\r\n", NyxGetReportedIssueRichDescription(nyxFileInfo.nyxSteganographyIDs[j].nyxReportedRichInfoId));
							}
							if(nyxFileInfo.nyxSteganographyIDs[j].nyxReportedDataStart != NULL && nyxFileInfo.nyxSteganographyIDs[j].nyxReportedDataSize != NULL){
								printf("            Data start: %#I64x; Data size: %#08x\r\n", nyxFileInfo.nyxSteganographyIDs[j].nyxReportedDataStart, nyxFileInfo.nyxSteganographyIDs[j].nyxReportedDataSize);
							}
							j++;
						}
					}
					i++;
				}
				NyxInspectArchive(nyxOpenFileHandle, &nyxReportFileStructureInfo);
			}
			NyxCloseArchive(nyxOpenFileHandle);
		}
	}else if(lstrcmpiW(argv[1], L"vf") == NULL){
		nyxOpenFileHandle = NyxOpenArchiveW((wchar_t*)argv[2], GENERIC_READ);
		if(nyxOpenFileHandle != NULL){
			if(NyxIdentifyArchive(nyxOpenFileHandle) && NyxGetArchiveProperties(nyxOpenFileHandle, &nyxArchiveInfo)){
				printf("[x] Detected archive: %s\r\n", NyxGetArchiveName(nyxOpenFileHandle));
				if(NyxValidateArchive(nyxOpenFileHandle, &nyxValidation)){
					printf("  [x] File is a valid archive!\r\n");
				}else{
					if(nyxValidation == NYX_VALIDATION_FILE_BROKEN){
						printf("  [x] File is a broken archive!\r\n");
					}else if(nyxValidation == NYX_VALIDATION_FILE_BROKEN_BUT_FIXABLE){
						printf("  [x] File is a broken but fixable archive!\r\n");
					}
				}
			}
			NyxCloseArchive(nyxOpenFileHandle);
		}
	}else if(lstrcmpiW(argv[1], L"rf") == NULL){
		nyxOpenFileHandle = NyxOpenArchiveW((wchar_t*)argv[2], GENERIC_READ);
		if(nyxOpenFileHandle != NULL){
			if(NyxIdentifyArchive(nyxOpenFileHandle) && NyxGetArchiveProperties(nyxOpenFileHandle, &nyxArchiveInfo)){
				printf("[x] Detected archive: %s\r\n", NyxGetArchiveName(nyxOpenFileHandle));
				if(NyxRecoverFileW(nyxOpenFileHandle, _wtoi(argv[3]), argv[4]) == NYX_RECOVER_SUCCESS){
					printf("  [x] File was successfully recovered!\r\n");
				}else{
					printf("  [!] File was not successfully recovered!\r\n");
				}
			}
			NyxCloseArchive(nyxOpenFileHandle);
		}
	}else{
		printf(" Example usage:\r\n  nyxConsole sf input.ext\r\n  nyxConsole sfi input.ext\r\n  nyxConsole vf input.ext\r\n  nyxConsole rf input.ext <fileId:0-n> recovered.ext\r\n");
		printf(" Wrong input parameter!\r\n\r\n");
	}
	return 0;
}

