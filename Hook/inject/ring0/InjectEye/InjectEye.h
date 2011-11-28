/*
FileName:    InjectEye.h
Author    :    ejoyc
Data    :    [03/05/2010]~[09/22/2010]
Targer    :    Hook NtMapViewOfSection,Then watch any  process to create 
*/
#pragma once

#include <ntifs.h>
#include <ntimage.h>
#include "xde.h"

#define URLMONPATH            L"\\WINDOWS\\SYSTEM32\\URLMON.DLL"
#define URLMONPATHSIZE        (sizeof(URLMONPATH)-2)

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING str);
VOID InjectEyeUnload(IN PDRIVER_OBJECT DriverObject);
BOOLEAN HookFunc(BOOLEAN IsHook);
PULONG    RetrieveFuncAddrFromKnownDLLs(IN WCHAR DllName[],IN CHAR FuncName[],IN CHAR *FuncHeadInfo);

NTSTATUS
DetourNtMapViewOfSection(IN HANDLE SectionHandle,
IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN ULONG ZeroBits,
IN ULONG CommitSize,
IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
IN OUT PULONG ViewSize,
IN SECTION_INHERIT InheritDisposition,
IN ULONG AllocationType,
IN ULONG Protect);

NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(IN HANDLE SectionHandle,
IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN ULONG ZeroBits,
IN ULONG CommitSize,
IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
IN OUT PULONG ViewSize,
IN SECTION_INHERIT InheritDisposition,
IN ULONG AllocationType,
IN ULONG Protect);

NTSYSAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN ULONG ZeroBits,
IN OUT PULONG AllocationSize,
IN ULONG AllocationType,
IN ULONG Protect);
//------------------------------------------------------------------------------------------------------------------------
//ZwProtectVirtualMemory未导出,需要编写专门的程序获取
typedef NTSTATUS (*PZwProtectVirtualMemory)(IN HANDLE ProcessHandle,
IN OUT PVOID *BaseAddress,
IN OUT PULONG ProtectSize,
IN ULONG NewProtect,
OUT PULONG OldProtect);

//ZwProtectVirtualMemory未导出,需要编写专门的程序获取
typedef NTSTATUS (*PZwWriteVirtualMemory)(IN HANDLE ProcessHandle,
IN PVOID BaseAddress,
IN PVOID Buffer,
IN ULONG BufferLength,
OUT PULONG ReturnLength OPTIONAL);

//------------------------------------------------------------------------------------------------------------------------
//win7和win2003的大部分内核数据结构系统
typedef struct _CONTROL_AREA
{
PVOID Segment          ;//Ptr32 _SEGMENT
LIST_ENTRY DereferenceList  ;// _LIST_ENTRY
ULONG NumberOfSectionReferences ;// Uint4B
ULONG NumberOfPfnReferences ;// Uint4B
ULONG NumberOfMappedViews ;// Uint4B
ULONG NumberOfSystemCacheViews ;// Uint4B
ULONG NumberOfUserReferences ;// Uint4B
ULONG u                ;// __unnamed
PFILE_OBJECT FilePointer      ;// Ptr32 _FILE_OBJECT
PVOID WaitingForDeletion ;// Ptr32 _EVENT_COUNTER
USHORT ModifiedWriteCount ;// Uint2B
USHORT FlushInProgressCount ;// Uint2B
ULONG WritableUserReferences ;// Uint4B
ULONG QuadwordPad      ;// Uint4B
}CONTROL_AREA,*PCONTROL_AREA;

typedef struct _SEGMENT
{
PCONTROL_AREA ControlArea      ;// Ptr32 _CONTROL_AREA
ULONG TotalNumberOfPtes ;// Uint4B
ULONG NonExtendedPtes  ;// Uint4B
ULONG Spare0           ;// Uint4B
ULONG64 SizeOfSegment    ;//  Uint8B
ULONG64 SegmentPteTemplate ;//  _MMPTE
ULONG NumberOfCommittedPages ;// Uint4B
PVOID ExtendInfo       ;//  Ptr32 _MMEXTEND_INFO
ULONG SegmentFlags     ;// _SEGMENT_FLAGS
PVOID BasedAddress     ;// Ptr32 Void——win7,小心
ULONG u1               ;// __unnamed
ULONG u2               ;// __unnamed
PVOID PrototypePte     ;// Ptr32 _MMPTE
PVOID ThePtes          ;// [1] _MMPTE
}SEGMENT,*PSEGMENT;

typedef struct _SECTION_OBJECT
{
PVOID StartingVa;//Ptr32 Void
PVOID EndingVa;//Ptr32 Void
PVOID Parent;//Ptr32 Void
PVOID LeftChild ;//Ptr32 Void
PVOID RightChild ;//Ptr32 Void
PSEGMENT Segment;//Ptr32 _SEGMENT
}SECTION_OBJECT,*PSECTION_OBJECT;
