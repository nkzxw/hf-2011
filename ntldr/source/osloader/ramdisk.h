//********************************************************************
//	created:	26:9:2008   12:17
//	file:		ramdisk.h
//	author:		tiamo
//	purpose:	ramdisk
//********************************************************************

#pragma once

//
// initialize ramdisk
//
ARC_STATUS RamdiskInitialize(__in PCHAR Options,__in BOOLEAN FirstTime);

//
// sdi boot
//
VOID RamdiskSdiBoot(__in PCHAR SdiFilePath);

//
// ramdisk open
//
ARC_STATUS RamdiskOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId);