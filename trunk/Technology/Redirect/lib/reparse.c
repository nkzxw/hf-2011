//#define RING3_USER 1

#ifdef RING3_USER
	#include <Windows.h>
	#include <WinIoCtl.h>
	#include <conio.h>
	#include <malloc.h>
	#include <stdio.h>
	#include <stdlib.h>

	#pragma pack (1)

	typedef struct _REPARSE_DATA_BUFFER {
		ULONG  ReparseTag;
		USHORT  ReparseDataLength;
		USHORT  Reserved;
		union {
			struct {
				USHORT  SubstituteNameOffset;
				USHORT  SubstituteNameLength;
				USHORT  PrintNameOffset;
				USHORT  PrintNameLength;
				WCHAR  PathBuffer[1];
			} SymbolicLinkReparseBuffer;
			struct {
				USHORT  SubstituteNameOffset;
				USHORT  SubstituteNameLength;
				USHORT  PrintNameOffset;
				USHORT  PrintNameLength;
				WCHAR  PathBuffer[1];
			} MountPointReparseBuffer;
			struct {
				UCHAR  DataBuffer[1];
			} GenericReparseBuffer;
		};
	} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

	#define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

	#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  ( 16 * 1024 )

	#pragma pack ()

	#define _TCHAR UCHAR
#else 
	#include "ntifs.h"
	#include "windef.h"
#endif

//
int ReparseDirectory( 
	WCHAR * Target, 
#ifdef RING3_USER
	UCHAR * Source 
#else
	WCHAR * Source,
	NTSTATUS * Status
#endif
	)
{
    PREPARSE_DATA_BUFFER    ReparseData;
    HANDLE					hJunction;
    DWORD					BytesReturned = 0;
//
#ifdef RING3_USER
    ReparseData = (PREPARSE_DATA_BUFFER)malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ZeroMemory(ReparseData, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	CopyMemory(ReparseData->MountPointReparseBuffer.PathBuffer, Target, sizeof(WCHAR) * wcslen(Target));
#else 
	ReparseData = (PREPARSE_DATA_BUFFER)ExAllocatePoolWithTag(NonPagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 'APER');
	RtlZeroMemory(ReparseData, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	RtlCopyMemory(ReparseData->MountPointReparseBuffer.PathBuffer, Target, sizeof(WCHAR) * wcslen(Target));
#endif

    ReparseData->ReparseTag			= IO_REPARSE_TAG_MOUNT_POINT;
    ReparseData->ReparseDataLength	= sizeof(WCHAR) * wcslen(Target) + 12; //MAXIMUM_REPARSE_DATA_BUFFER_SIZE - sizeof(REPARSE_DATA_BUFFER);
    ReparseData->MountPointReparseBuffer.SubstituteNameLength	= sizeof(WCHAR) * wcslen(Target);
    ReparseData->MountPointReparseBuffer.PrintNameOffset		= ReparseData->MountPointReparseBuffer.SubstituteNameLength + 2;
//
#ifdef RING3_USER
    CreateDirectory(Source, NULL);
    hJunction = CreateFile(Source, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
#else
	{
		OBJECT_ATTRIBUTES oa = { 0 };
		UNICODE_STRING FileName = { 0 };
		IO_STATUS_BLOCK	iosb = { 0 };

		RtlInitUnicodeString( &FileName, Source );
		InitializeObjectAttributes( &oa, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL );

		*Status = ZwCreateFile( &hJunction, FILE_READ_DATA | FILE_WRITE_DATA, &oa, &iosb,
			NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF,
			FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_OPEN_REPARSE_POINT, 
			NULL, 0
			);
		if (!NT_SUCCESS( *Status )) {
			
			KdPrint(("ReparseDirectory: ZwCreateFile Failed %X %wZ\n", *Status, &FileName));
			return 1;
		}
	}
#endif

//
#ifdef RING3_USER
    if (hJunction != INVALID_HANDLE_VALUE)
    {

        if ( DeviceIoControl(hJunction, FSCTL_SET_REPARSE_POINT, ReparseData, 
                REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseData->ReparseDataLength, NULL, 0, &BytesReturned, NULL) )
        {
			CloseHandle(hJunction);
			free(ReparseData);
			return 0;
        }
        else
        {
            printf("ReparseDirectory: DeviceIoControl Failed 0x%0.8x.\n", GetLastError());
        }

        ZeroMemory(ReparseData, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        ReparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

        DeviceIoControl(hJunction, FSCTL_DELETE_REPARSE_POINT, ReparseData,
            REPARSE_DATA_BUFFER_HEADER_SIZE, NULL, 0, &BytesReturned, NULL);

        CloseHandle(hJunction);

        RemoveDirectory(Source);
    }
    else
    {

		printf("ReparseDirectory: Open %s Failed.\n", Source);
    }
	free(ReparseData);
	return 1;
#else 
	{
		IO_STATUS_BLOCK	iosb = { 0 };
		*Status = ZwFsControlFile(hJunction, 0, 0, 0, &iosb, FSCTL_SET_REPARSE_POINT, ReparseData, 
			REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseData->ReparseDataLength, 
			NULL, 0);
		if (!NT_SUCCESS(*Status)) {
			KdPrint(("ReparseDirectory: ZwFsControlFile Failed %X\n", *Status));
		}
	}
	ZwClose(hJunction);
	ExFreePoolWithTag( ReparseData, 'APER' );
	return NT_SUCCESS(*Status)? 0: 1;
#endif
}

#ifdef RING3_USER

int __cdecl main(int argc, _TCHAR* argv[])
{
	
	ReparseDirectory( L"\\??\\e:\\test", "c:\\test" );
	system("pause");
	return 0;
}
#else 
void DriverUnload( PDRIVER_OBJECT DriverObject )
{
}

NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath )
{
	NTSTATUS status = STATUS_SUCCESS;
	DriverObject->DriverUnload = DriverUnload;
	
	ReparseDirectory( L"\\??\\e:\\test", L"\\??\\c:\\test", &status );
	return status;
}
#endif