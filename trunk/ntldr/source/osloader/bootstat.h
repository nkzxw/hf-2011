//********************************************************************
//	created:	18:8:2008   19:29
//	file:		bootstat.h
//	author:		tiamo
//	purpose:	boot status
//********************************************************************

#pragma once

//
// lock status data
//
ARC_STATUS BlLockBootStatusData(__in_opt ULONG DeviceId,__in PCHAR DevicePath,__in PCHAR LoadPath,__out PULONG FileId);

//
// get set boot status data
//
ARC_STATUS BlGetSetBootStatusData(__in ULONG FileId,__in BOOLEAN GetData,__in ULONG DataType,__inout PVOID Buffer,__in ULONG BufferLength,__out_opt PULONG ActualLength);

//
// get last boot status
//
ULONG BlGetLastBootStatus(__in ULONG FileId,__out PULONG BootStatus);

//
// write boot status flags
//
ARC_STATUS BlWriteBootStatusFlags(__in_opt ULONG DeviceId,__in PCHAR LoadPath,__in UCHAR Value4,__in UCHAR Value5);

//
// unlock boot status
//
ARC_STATUS BlUnlockBootStatusData(__in ULONG FileId);