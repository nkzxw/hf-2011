#ifndef _FILE_FILTER_H
#define _FILE_FILTER_H

#include <ntifs.h>
#define FileFltTag 0X12345679

#define  MAX_DEVICE_NAME 260

typedef struct _FILE_FILTER_EXT
{
	PDEVICE_OBJECT AttachToDevice;  //记录自己Attach上时的下层设备 即Attach之前 要Attach的设备所在设备栈的栈顶
	PDEVICE_OBJECT StorageDevObj;
	//WCHAR AttachToDevName[MAX_DEVICE_NAME];
	UNICODE_STRING AttachToDevName;
	WCHAR StorageDevName[MAX_DEVICE_NAME];  //相应的storage的name
	//UNICODE_STRING AttachToDrvName;
}FILE_FILTER_EXT;

//DRIVER_FS_NOTIFICATION FileFltNotification;

#define wProtectPath L"\\Device\\HarddiskVolume1\\FlyFire"

typedef struct _OBJECT_CREATE_INFORMATION {
	ULONG Attributes;
	HANDLE RootDirectory;
	PVOID ParseContext;
	KPROCESSOR_MODE ProbeMode;
	ULONG PagedPoolCharge;
	ULONG NonPagedPoolCharge;
	ULONG SecurityDescriptorCharge;
	PSECURITY_DESCRIPTOR SecurityDescriptor;
	PSECURITY_QUALITY_OF_SERVICE SecurityQos;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_HEADER {
	LONG_PTR PointerCount;
	union {
		LONG_PTR HandleCount;
		PVOID NextToFree;
	};
	POBJECT_TYPE Type;
	UCHAR NameInfoOffset;
	UCHAR HandleInfoOffset;
	UCHAR QuotaInfoOffset;
	UCHAR Flags;

	union {
		POBJECT_CREATE_INFORMATION ObjectCreateInfo;
		PVOID QuotaBlockCharged;
	};

	PSECURITY_DESCRIPTOR SecurityDescriptor;
	QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER( o ) \
	CONTAINING_RECORD( (o), OBJECT_HEADER, Body )   //在OBJECT_HEADER链表中 查找某个节点 该OBJECT_HEADER的Body是O


typedef struct _OBJECT_HEADER_NAME_INFO {
	PVOID Directory;
	UNICODE_STRING Name;
	ULONG QueryReferences;
#if DBG
	ULONG Reserved2;
	LONG DbgDereferenceCount;
#ifdef _WIN64
	ULONG64  Reserved3;   // Win64 requires these structures to be 16 byte aligned.
#endif
#endif
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

/*
POBJECT_HEADER_NAME_INFO
OBJECT_HEADER_TO_NAME_INFO_EXISTS (
								   IN POBJECT_HEADER ObjectHeader
								   )
{
	ASSERT(ObjectHeader->NameInfoOffset != 0);
	return (POBJECT_HEADER_NAME_INFO)((PUCHAR)ObjectHeader -
		ObjectHeader->NameInfoOffset);
}

POBJECT_HEADER_NAME_INFO
OBJECT_HEADER_TO_NAME_INFO (
							IN POBJECT_HEADER ObjectHeader
							)
{
	POBJECT_HEADER_NAME_INFO nameInfo;

	if (ObjectHeader->NameInfoOffset != 0) {
		nameInfo = OBJECT_HEADER_TO_NAME_INFO_EXISTS(ObjectHeader);
		__assume(nameInfo != NULL);
	} else {
		nameInfo = NULL;
	}

	return nameInfo;
}*/


NTSTATUS FZGetDevName(PDEVICE_OBJECT pDevObj, PUNICODE_STRING pDevName);

NTSTATUS FileFilterInit(IN PDRIVER_OBJECT pDrvObj);

BOOLEAN FZCheckFileName(PDEVICE_OBJECT pDevObj, WCHAR* fileName);

NTSTATUS FZFileDefaultFilter(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp );

NTSTATUS FZFltCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);

NTSTATUS FZFltSetInfo(PDEVICE_OBJECT pDevObj, PIRP pIrp);

NTSTATUS FZFltWrite(PDEVICE_OBJECT pDevObj, PIRP pIrp);

NTSTATUS FZEnumerateAttach(IN PDEVICE_OBJECT pDevObj);

NTSTATUS FZEnumerateDetach(IN PDEVICE_OBJECT pDevObj);

#endif