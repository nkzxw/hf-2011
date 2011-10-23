//********************************************************************
//	created:	17:8:2008   22:02
//	file:		dskcache.h
//	author:		tiamo
//	purpose:	disk cache
//********************************************************************

#pragma once

//
// initialize disk cache
//
ARC_STATUS BlDiskCacheInitialize();

//
// start cache on device
//
PVOID BlDiskCacheStartCachingOnDevice(__in ULONG DeviceId);

//
// stop cache and device
//
VOID BlDiskCacheStopCachingOnDevice(__in ULONG DeviceId);

//
// uninitialize disk cache
//
VOID BlDiskCacheUninitialize();

//
// cache write
//
ARC_STATUS BlDiskCacheWrite(__in ULONG DeviceId,__in PLARGE_INTEGER Offset,__in PVOID Buffer,__in ULONG Count,__in PULONG WritenCount);

//
// cache read
//
ARC_STATUS BlDiskCacheRead(__in ULONG DeviceId,__in PLARGE_INTEGER Offset,__in PVOID Buffer,__in ULONG Count,__in PULONG ReadCount,__in BOOLEAN SaveDataToCache);