#ifndef _FAST_IO_H
#define _FAST_IO_H

#include <ntifs.h>

BOOLEAN FZFastIoCheckPossible(PFILE_OBJECT pFileObj, PLARGE_INTEGER FileOffset, ULONG Length, 
							  BOOLEAN Waitable,ULONG LockKey, BOOLEAN CheckForReadOp, 
							  PIO_STATUS_BLOCK StatusBlock, PDEVICE_OBJECT pDevObj);

BOOLEAN FZFastIoRead(PFILE_OBJECT pFileObj, PLARGE_INTEGER FileOffset, 
					 ULONG Length, BOOLEAN Waitable, ULONG LockKey, PVOID Buffer, 
					 PIO_STATUS_BLOCK IoStatus, PDEVICE_OBJECT pDevObj);

BOOLEAN FZFastIoWrite(  IN struct _FILE_OBJECT  *FileObject,
					  IN PLARGE_INTEGER  FileOffset, IN ULONG  Length,
					  IN BOOLEAN  Wait, IN ULONG  LockKey,
					  IN PVOID  Buffer, OUT PIO_STATUS_BLOCK  IoStatus,
					  IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoQueryBasicInfo(  IN struct _FILE_OBJECT  *FileObject,
							   IN BOOLEAN  Wait,
							   OUT PFILE_BASIC_INFORMATION  Buffer,
							   OUT PIO_STATUS_BLOCK  IoStatus,
							   IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoQueryStandardInfo( IN struct _FILE_OBJECT  *FileObject,
								  IN BOOLEAN  Wait,
								  OUT PFILE_STANDARD_INFORMATION  Buffer,
								  OUT PIO_STATUS_BLOCK  IoStatus,
								  IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoLock(IN struct _FILE_OBJECT  *FileObject,
					IN PLARGE_INTEGER  FileOffset, IN PLARGE_INTEGER  Length,
					PEPROCESS  ProcessId, ULONG  Key,
					BOOLEAN  FailImmediately, BOOLEAN  ExclusiveLock,
					OUT PIO_STATUS_BLOCK  IoStatus, 
					IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN FZFastIoUnlockSingle(IN struct _FILE_OBJECT  *FileObject,
							 IN PLARGE_INTEGER  FileOffset,
							 IN PLARGE_INTEGER  Length,
							 PEPROCESS  ProcessId,
							 ULONG  Key,
							 OUT PIO_STATUS_BLOCK  IoStatus,
							 IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN	FZFastIoUnlockAll( IN struct _FILE_OBJECT  *FileObject,
						  PEPROCESS  ProcessId,
						  OUT PIO_STATUS_BLOCK  IoStatus,
						  IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN	FZFastIoUnlockAllByKey(IN struct _FILE_OBJECT  *FileObject,
							   PVOID  ProcessId,
							   ULONG  Key,
							   OUT PIO_STATUS_BLOCK  IoStatus,
							   IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoDeviceControl(
							  IN struct _FILE_OBJECT  *FileObject,
							  IN BOOLEAN  Wait,
							  IN PVOID  InputBuffer  OPTIONAL,
							  IN ULONG  InputBufferLength,
							  OUT PVOID  OutputBuffer  OPTIONAL,
							  IN ULONG  OutputBufferLength,
							  IN ULONG  IoControlCode,
							  OUT PIO_STATUS_BLOCK  IoStatus,
							  IN struct _DEVICE_OBJECT  *DeviceObject);

VOID FZFastIoAcquireFile( IN struct _FILE_OBJECT  *FileObject);

VOID FZFastIoReleaseFile( IN struct _FILE_OBJECT  *FileObject);

VOID FZFastIoDetachDevice( IN struct _DEVICE_OBJECT  *SourceDevice,
						  IN struct _DEVICE_OBJECT  *TargetDevice);

BOOLEAN FZFastIoQueryNetworkOpenInfo(IN struct _FILE_OBJECT  *FileObject,
									 IN BOOLEAN  Wait,
									 OUT struct _FILE_NETWORK_OPEN_INFORMATION  *Buffer,
									 OUT struct _IO_STATUS_BLOCK  *IoStatus,
									 IN struct _DEVICE_OBJECT  *DeviceObject);

NTSTATUS FZFastIoAcquireForModWrite(
									IN struct _FILE_OBJECT  *FileObject,
									IN PLARGE_INTEGER  EndingOffset,
									OUT struct _ERESOURCE  **ResourceToRelease,
									IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoMdlRead( IN struct _FILE_OBJECT  *FileObject,
						IN PLARGE_INTEGER  FileOffset,
						IN ULONG  Length,
						IN ULONG  LockKey,
						OUT PMDL  *MdlChain,
						OUT PIO_STATUS_BLOCK  IoStatus,
						IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN	FZFastIoMdlReadComplete(	IN struct _FILE_OBJECT *FileObject,
								IN PMDL MdlChain,
								IN struct _DEVICE_OBJECT *DeviceObject);


BOOLEAN FZFastIoPrepareMdlWrite(     IN struct _FILE_OBJECT  *FileObject,
								IN PLARGE_INTEGER  FileOffset,
								IN ULONG  Length,
								IN ULONG  LockKey,
								OUT PMDL  *MdlChain,
								OUT PIO_STATUS_BLOCK  IoStatus,
								IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN	FZFastIoMdlWriteComplete(	IN struct _FILE_OBJECT  *FileObject,
								 IN PLARGE_INTEGER  FileOffset,
								 IN PMDL  MdlChain,
								 IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN FZFastIoReadCompressed(
							   IN struct _FILE_OBJECT  *FileObject,
							   IN PLARGE_INTEGER  FileOffset,
							   IN ULONG  Length,
							   IN ULONG  LockKey,
							   OUT PVOID  Buffer,
							   OUT PMDL  *MdlChain,
							   OUT PIO_STATUS_BLOCK  IoStatus,
							   OUT struct _COMPRESSED_DATA_INFO  *CompressedDataInfo,
							   IN ULONG  CompressedDataInfoLength,
							   IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoWriteCompressed(
								IN struct _FILE_OBJECT  *FileObject,
								IN PLARGE_INTEGER  FileOffset,
								IN ULONG  Length,
								IN ULONG  LockKey,
								IN PVOID  Buffer,
								OUT PMDL  *MdlChain,
								OUT PIO_STATUS_BLOCK  IoStatus,
								IN struct _COMPRESSED_DATA_INFO  *CompressedDataInfo,
								IN ULONG  CompressedDataInfoLength,
								IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoMdlReadCompleteCompressed(
	IN struct _FILE_OBJECT  *FileObject,
	IN PMDL  MdlChain,
	IN struct _DEVICE_OBJECT  *DeviceObject);

BOOLEAN FZFastIoMdlWriteCompleteCompressed(
	IN struct _FILE_OBJECT  *FileObject,
	IN PLARGE_INTEGER  FileOffset,
	IN PMDL  MdlChain,
	IN struct _DEVICE_OBJECT  *DeviceObject);


BOOLEAN	FZFastIoQueryOpen(	  IN struct _IRP  *Irp,
						  OUT PFILE_NETWORK_OPEN_INFORMATION  NetworkInformation,
						  IN struct _DEVICE_OBJECT  *DeviceObject);


NTSTATUS FZFastIoReleaseForModWrite(
									IN struct _FILE_OBJECT  *FileObject,
									IN struct _ERESOURCE  *ResourceToRelease,
									IN struct _DEVICE_OBJECT  *DeviceObject);

NTSTATUS FZFastIoAcquireForCCFlush( IN struct _FILE_OBJECT  *FileObject,
								   IN struct _DEVICE_OBJECT  *DeviceObject);


NTSTATUS FZFastIoReleaseForCCFlush(		IN struct _FILE_OBJECT  *FileObject,
								   IN struct _DEVICE_OBJECT  *DeviceObject);



#endif