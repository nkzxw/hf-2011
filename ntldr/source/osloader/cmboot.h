//********************************************************************
//	created:	24:8:2008   23:58
//	file:		cmboot.h
//	author:		tiamo
//	purpose:	registry support
//********************************************************************

#pragma once

//
// scan registry
//
PCHAR BlScanRegistry(__in PWCHAR BootFileSystem,__inout PBOOLEAN UseLastKnownGood,__in PLIST_ENTRY BootDriverListHead,__out PUNICODE_STRING AnsiCodePage,
					 __out PUNICODE_STRING OemCodePage,__out PUNICODE_STRING LanguageTable,__out PUNICODE_STRING OemHalFont,__out PUNICODE_STRING InfFile,
					 __out struct _SETUP_LOADER_BLOCK* SetupLoaderBlock,__out PBOOLEAN LoadSacDriver);

//
// load and init system hive
//
ARC_STATUS BlLoadAndInitSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR HiveName,
								   __in BOOLEAN IsAlternate,__out PBOOLEAN AlternateUsed,__out PBOOLEAN RestartSetup);

//
// update profile option
//
VOID BlUpdateProfileOption(__in PCM_HARDWARE_PROFILE_LIST ProfileList,__in HCELL_INDEX ControlSetKeyCell,__in ULONG Index);