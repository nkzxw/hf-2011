#ifndef __MONITOR_H__
#define _MONITOR_H__

typedef struct  _IMAGE_INFO {
    union {
        ULONG  Properties;
        struct {
            ULONG ImageAddressingMode  : 8; //code addressing mode
            ULONG SystemModeImage      : 1; //system mode image
            ULONG ImageMappedToAllPids : 1; //mapped in all processes
            ULONG Reserved             : 22;
        };
    };
    PVOID  ImageBase;
    ULONG  ImageSelector;
    ULONG  ImageSize;
    ULONG  ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

NTSTATUS
  PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE  NotifyRoutine
  );

VOID
(*PLOAD_IMAGE_NOTIFY_ROUTINE) (
    IN PUNICODE_STRING  FullImageName,
    IN HANDLE  ProcessId, // where image is mapped
    IN PIMAGE_INFO  ImageInfo
    );


NTSTATUS
  PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE  NotifyRoutine,
  IN BOOLEAN  Remove
  );

VOID
(*PCREATE_PROCESS_NOTIFY_ROUTINE) (
    IN HANDLE  ParentId,
    IN HANDLE  ProcessId,
    IN BOOLEAN  Create
    );


NTSTATUS
  PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE  NotifyRoutine
  );

VOID
(*PCREATE_THREAD_NOTIFY_ROUTINE) (
    IN HANDLE  ProcessId,
    IN HANDLE  ThreadId,
    IN BOOLEAN  Create
    );
   
NTSTATUS PsLookupProcessByProcessId(
		IN ULONG ulProcId, 
		OUT PEPROCESS * pEProcess
		);    
     
VOID ProcessCreateMon ( 
		IN HANDLE  hParentId, 
		IN HANDLE PId,
		IN BOOLEAN bCreate
		);

VOID ThreadCreateMon (
		IN HANDLE  PId, 
		IN HANDLE TId, 
		IN BOOLEAN  bCreate
		);

VOID ImageCreateMon (
		IN PUNICODE_STRING  FullImageName, 
		IN HANDLE  ProcessId, 
		IN PIMAGE_INFO  ImageInfo 
		);

#endif //_MONITOR_H__