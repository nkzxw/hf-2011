//********************************************************************
//	created:	14:8:2008   11:19
//	file:		blconfig.h
//	author:		tiamo
//	purpose:	config
//********************************************************************

#pragma once

//
// search config tree callback
//
typedef BOOLEAN (*PNODE_CALLBACK)(__in PCONFIGURATION_COMPONENT_DATA FoundComponent);

//
// get path mnemonic key
//
BOOLEAN BlGetPathMnemonicKey(__in PCHAR OpenPath,__in PCHAR Mnemonic,__out PULONG Key);

//
// builds an ARC pathname
//
VOID BlGetPathnameFromComponent(__in PCONFIGURATION_COMPONENT_DATA Component,__out PCHAR ArcName);

//
// initialize config
//
ARC_STATUS BlConfigurationInitialize(__in PCONFIGURATION_COMPONENT Parent,__in PCONFIGURATION_COMPONENT_DATA ParentEntry);

//
// find config entry
//
PCONFIGURATION_COMPONENT_DATA KeFindConfigurationEntry(__in PCONFIGURATION_COMPONENT_DATA Child,__in CONFIGURATION_CLASS Class,
													   __in CONFIGURATION_TYPE Type,__in_opt PULONG Key);

//
// get next entry
//
PCONFIGURATION_COMPONENT_DATA KeFindConfigurationNextEntry(__in PCONFIGURATION_COMPONENT_DATA Child,__in CONFIGURATION_CLASS Class,
														   __in CONFIGURATION_TYPE Type,__in_opt PULONG Key,__in PCONFIGURATION_COMPONENT_DATA *Resume);

//
// search config tree
//
BOOLEAN BlSearchConfigTree(__in PCONFIGURATION_COMPONENT_DATA Node,__in CONFIGURATION_CLASS Class,__in CONFIGURATION_TYPE Type,__in ULONG Key,__in PNODE_CALLBACK Callback);

//
// generates an NT device name prefix and a canonical ARC device name from an ARC device name.
//
ARC_STATUS BlGenerateDeviceNames(__in PCHAR ArcDeviceName,__out PCHAR ArcCanonicalName,__out_opt PCHAR NtDevicePrefix);