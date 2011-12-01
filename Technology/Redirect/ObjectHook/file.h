#include "ntifs.h"

#pragma once

#pragma pack(push, 1) 
typedef struct _SERVICE_TABLE_DESCRIPTOR { 
 
	PULONG	ServiceTableBase;		/* table of function pointers		*/ 
	PVOID	ServiceCounterTable;	/* used in checked build only		*/ 
	ULONG	NumberOfServices;		/* number of services in this table	*/ 
	/* extra LONG on IA64 goes here */ 
	PVOID	ParamTableBase;			/* number of parameters				*/ 
 
} SERVICE_TABLE_DESCRIPTOR, *PSERVICE_TABLE_DESCRIPTOR; 
#pragma pack(pop) 
 
 
/* 
 * The Service Descriptor Table index (4 bytes following the mov opcode) 
 * 
 * The index format is as follows: 
 * 
 * Leading 18 bits are all zeroes 
 * Following 2 bits are system service table index (3 bits on Win64) 
 * Following 12 bits are service number 
 */ 
 
#define	SERVICE_TABLE_INDEX_BITS	2 
#define	NUMBER_SERVICE_TABLES		(1 << SERVICE_TABLE_INDEX_BITS) 
 
#define	SERVICE_ID_NUMBER_BITS		12 
#define	SERVICE_ID_NUMBER_MASK		((1 << SERVICE_ID_NUMBER_BITS) - 1) 
 
/* 
 * The kernel's service descriptor table, which is used to find the address 
 * of the service dispatch tables to use for a service ID. 
 * 
 * Descriptor 0 is used for core services (NTDLL) 
 * Descriptor 1 is used for GUI services (WIN32K) 
 * Descriptors 2 and 3 are unused on current versions of Windows NT. 
 */ 

//
// 以下定义KeServiceDescriptorTable，
// 只有取&KeServiceDescriptorTable才能得到
// 正确的内存KeServiceDescriptorTable值
//
//__declspec(dllimport) PSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable; 

//
// 以下定义KeServiceDescriptorTable，直接可以得到
// KeServiceDescriptorTable值(&KeServiceDescriptorTable一样)
//
__declspec(dllimport) SERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[NUMBER_SERVICE_TABLES]; 

#define $UNUSED                          (0X0)

#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $PROPERTY_SET                    (0xF0)
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x100)
#define $END                             (0xFFFFFFFF)

#define  FILE_SYSTEM_FAT	1
#define  FILE_SYSTEM_NTFS	2

#define FILE_SYSTEM_NAME_NTFS	L"\\FileSystem\\Ntfs"
#define FILE_SYSTEM_NAME_FAT	L"\\FileSystem\\FastFat"

#define kmalloc(_t, _s)	ExAllocatePoolWithTag(_t, _s, 'elif')
#define kfree(_p)		ExFreePoolWithTag(_p, 'elif')

#define FILE_MAX_PATH  0x400

#define AllocateEmptyUnicodeString(_S) {										\
	(_S)->Buffer = kmalloc( NonPagedPool, FILE_MAX_PATH);						\
	{																			\
		if ((_S)->Buffer)	{													\
			RtlZeroMemory((_S)->Buffer, FILE_MAX_PATH);							\
			(_S)->Length = 0;													\
			(_S)->MaximumLength = FILE_MAX_PATH;								\
		}																		\
	}																			\
}

KPROCESSOR_MODE KeGetPreviousMode();

NTSTATUS
NTAPI
ObReferenceObjectByName (
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE PassedAccessState OPTIONAL,
	IN ACCESS_MASK DesiredAccess OPTIONAL,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	IN KPROCESSOR_MODE AccessMode,
	IN OUT PVOID ParseContext OPTIONAL,
	OUT PVOID *Object
	);

extern POBJECT_TYPE *IoDriverObjectType;
extern POBJECT_TYPE *IoDeviceObjectType;

NTSTATUS
fnGetFileNameByFileObject(
	IN PFILE_OBJECT FileObject,
	IN PUNICODE_STRING FullPath
	);

void fnInitialize();