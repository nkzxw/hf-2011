//********************************************************************
//	created:	11:8:2008   19:32
//	file:		arcemul.h
//	author:		tiamo
//	purpose:	enum layer
//********************************************************************

#pragma once

//
// arc error code
//
enum
{
	ESUCCESS,
	E2BIG,
	EACCES,
	EAGAIN,
	EBADF,
	EBUSY,
	EFAULT,
	EINVAL,
	EIO,
	EISDIR,
	EMFILE,
	EMLINK,
	ENAMETOOLONG,
	ENODEV,
	ENOENT,
	ENOEXEC,
	ENOMEM,
	ENOSPC,
	ENOTDIR,
	ENOTTY,
	ENXIO,
	EROFS,
	EMAXIMUM
};

//
// firmware vector entry
//
typedef enum _FIRMWARE_ENTRY
{
	//
	// load
	//
	LoadRoutine,

	//
	// invoke
	//
	InvokeRoutine,

	//
	// execute
	//
	ExecuteRoutine,

	//
	// halt
	//
	HaltRoutine,

	//
	// power down
	//
	PowerDownRoutine,

	//
	// restart
	//
	RestartRoutine,

	//
	// reboot
	//
	RebootRoutine,

	//
	// enter interative mode
	//
	InteractiveModeRoutine,

	//
	// empty
	//
	Reserved1,

	//
	// get peer
	//
	GetPeerRoutine,

	//
	// get child
	//
	GetChildRoutine,

	//
	// get parent
	//
	GetParentRoutine,

	//
	// get data
	//
	GetDataRoutine,

	//
	// add child
	//
	AddChildRoutine,

	//
	// delete component
	//
	DeleteComponentRoutine,

	//
	// get component
	//
	GetComponentRoutine,

	//
	// save config
	//
	SaveConfigurationRoutine,

	//
	// get system id
	//
	GetSystemIdRoutine,

	//
	// memory
	//
	MemoryRoutine,

	//
	// empty
	//
	Reserved2,

	//
	// get time
	//
	GetTimeRoutine,

	//
	// get relative time
	//
	GetRelativeTimeRoutine,

	//
	// get dirent
	//
	GetDirectoryEntryRoutine,

	//
	// open
	//
	OpenRoutine,

	//
	// close
	//
	CloseRoutine,

	//
	// read
	//
	ReadRoutine,

	//
	// read status
	//
	ReadStatusRoutine,

	//
	// write
	//
	WriteRoutine,

	//
	// seek
	//
	SeekRoutine,

	//
	// mount
	//
	MountRoutine,

	//
	// get env
	//
	GetEnvironmentRoutine,

	//
	// set env
	//
	SetEnvironmentRoutine,

	//
	// get file info
	//
	GetFileInformationRoutine,

	//
	// set file info
	//
	SetFileInformationRoutine,

	//
	// flush caches
	//
	FlushAllCachesRoutine,

	//
	// test unicode
	//
	TestUnicodeCharacterRoutine,

	//
	// get display status
	//
	GetDisplayStatusRoutine,

	//
	// max
	//
	MaximumRoutine,
}FIRMWARE_ENTRY;

//
// configuration class
//
typedef enum _CONFIGURATION_CLASS
{
	//
	// system device
	//
	SystemClass,

	//
	// processor
	//
	ProcessorClass,

	//
	// cache
	//
	CacheClass,

	//
	// adapter
	//
	AdapterClass,

	//
	// controller
	//
	ControllerClass,

	//
	// peripheral
	//
	PeripheralClass,

	//
	// memory
	//
	MemoryClass,

	//
	// max type
	//
	MaximumClass
}CONFIGURATION_CLASS,*PCONFIGURATION_CLASS;

//
// seek mode
//
typedef enum _SEEK_MODE
{
	//
	// from offset 0
	//
	SeekAbsolute,

	//
	// from current
	//
	SeekRelative,

	//
	// max
	//
	SeekMaximum,
}SEEK_MODE;

//
// moute type
//
typedef enum _MOUNT_OPERATION
{
	//
	// mount
	//
	MountLoadMedia,

	//
	// umount
	//
	MountUnloadMedia,

	//
	// max
	//
	MountMaximum
}MOUNT_OPERATION;

//
// read only file
//
#define ArcReadOnlyFile									1

//
// hidden
//
#define ArcHiddenFile									2

//
// system
//
#define ArcSystemFile									4

//
// archive
//
#define ArcArchiveFile									8

//
// dir
//
#define ArcDirectoryFile								16

//
// deleted file
//
#define ArcDeleteFile									32

//
// file open mod
//
typedef enum _OPEN_MODE
{
	//
	// read only
	//
	ArcOpenReadOnly,

	//
	// write only
	//
	ArcOpenWriteOnly,

	//
	// read write
	//
	ArcOpenReadWrite,

	//
	// create write
	//
	ArcCreateWriteOnly,

	//
	// create read write
	//
	ArcCreateReadWrite,

	//
	// supersed write
	//
	ArcSupersedeWriteOnly,

	//
	// supersed read write
	//
	ArcSupersedeReadWrite,

	//
	// open directory
	//
	ArcOpenDirectory,

	//
	// create directory
	//
	ArcCreateDirectory,

	//
	// max
	//
	ArcOpenMaximumMode
}OPEN_MODE;

//
// configuration component
//
typedef struct _CONFIGURATION_COMPONENT
{
	//
	// class
	//
	CONFIGURATION_CLASS									Class;

	//
	// type
	//
	CONFIGURATION_TYPE									Type;

	//
	// device flags
	//
	DEVICE_FLAGS										Flags;

	//
	// version
	//
	USHORT												Version;

	//
	// revision
	//
	USHORT												Revision;

	//
	// key
	//
	ULONG												Key;

	//
	// affinity mask
	//
	ULONG												AffinityMask;

	//
	// data length
	//
	ULONG												ConfigurationDataLength;

	//
	// id length
	//
	ULONG												IdentifierLength;

	//
	// id
	//
	CHAR * FIRMWARE_PTR									Identifier;
}CONFIGURATION_COMPONENT, * FIRMWARE_PTR PCONFIGURATION_COMPONENT;

//
// configuration component data
//
typedef struct _CONFIGURATION_COMPONENT_DATA
{
	//
	// parent
	//
	struct _CONFIGURATION_COMPONENT_DATA*				Parent;

	//
	// child
	//
	struct _CONFIGURATION_COMPONENT_DATA*				Child;

	//
	// sibling
	//
	struct _CONFIGURATION_COMPONENT_DATA*				Sibling;

	//
	// entry
	//
	CONFIGURATION_COMPONENT								ComponentEntry;

	//
	// data
	//
	PVOID												ConfigurationData;
}CONFIGURATION_COMPONENT_DATA,*PCONFIGURATION_COMPONENT_DATA;

//
// file info
//
typedef struct _FILE_INFORMATION
{
	//
	// start address
	//
	LARGE_INTEGER										StartingAddress;

	//
	// end address
	//
	LARGE_INTEGER										EndingAddress;

	//
	// current position
	//
	LARGE_INTEGER										CurrentPosition;

	//
	// type
	//
	CONFIGURATION_TYPE									Type;

	//
	// file name length
	//
	ULONG												FileNameLength;

	//
	// file attribute
	//
	UCHAR												Attributes;

	//
	// file name
	//
	CHAR												FileName[32];
}FILE_INFORMATION,*FIRMWARE_PTR PFILE_INFORMATION;

//
// dir entry
//
typedef struct _DIRECTORY_ENTRY
{
	//
	// length of file name
	//
	ULONG												FileNameLength;

	//
	// file attribute
	//
	UCHAR												FileAttribute;

	//
	// file name
	//
	CHAR												FileName[32];
}DIRECTORY_ENTRY,*PDIRECTORY_ENTRY;

//
// system id
//
typedef struct _SYSTEM_ID
{
	//
	// vendor
	//
	CHAR												VendorId[8];

	//
	// product
	//
	CHAR												ProductId[8];
}SYSTEM_ID,*PSYSTEM_ID;

//
// display status
//
typedef struct _ARC_DISPLAY_STATUS
{
	//
	// cursor x position
	//
	USHORT												CursorXPosition;

	//
	// y position
	//
	USHORT												CursorYPosition;

	//
	// max x position
	//
	USHORT												CursorMaxXPosition;

	//
	// max y position
	//
	USHORT												CursorMaxYPosition;

	//
	// foreground color
	//
	UCHAR												ForegroundColor;

	//
	// background color
	//
	UCHAR												BackgroundColor;

	//
	// high intensity
	//
	BOOLEAN												HighIntensity;

	//
	// underscored
	//
	BOOLEAN												Underscored;

	//
	// reversed video
	//
	BOOLEAN												ReverseVideo;
}ARC_DISPLAY_STATUS,*PARC_DISPLAY_STATUS;

//
// system parameter block
//
typedef struct _SYSTEM_PARAMETER_BLOCK
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// length
	//
	ULONG												Length;

	//
	// version
	//
	USHORT												Version;

	//
	// revision
	//
	USHORT												Revision;

	//
	// restart block
	//
	PVOID												RestartBlock;

	//
	// debug block
	//
	PVOID												DebugBlock;

	//
	// generate exception vector
	//
	VOID * FIRMWARE_PTR									GenerateExceptionVector;

	//
	// tlb miss exception vector
	//
	VOID * FIRMWARE_PTR									TlbMissExceptionVector;

	//
	// firmware vector length
	//
	ULONG												FirmwareVectorLength;

	//
	// firmware vector
	//
	VOID * FIRMWARE_PTR * FIRMWARE_PTR					FirmwareVector;

	//
	// vendor vector length
	//
	ULONG												VendorVectorLength;

	//
	// vendor vector
	//
	VOID * FIRMWARE_PTR * FIRMWARE_PTR					VendorVector;

	//
	// adapter count
	//
	ULONG												AdapterCount;

	//
	// adapter type
	//
	ULONG												Adapter0Type;

	//
	// adapter length
	//
	ULONG												Adapter0Length;

	//
	// adapter vector
	//
	VOID * FIRMWARE_PTR * FIRMWARE_PTR					Adapter0Vector;
}SYSTEM_PARAMETER_BLOCK,* FIRMWARE_PTR PSYSTEM_PARAMETER_BLOCK;

//
// execute
//
typedef ARC_STATUS (*PARC_EXECUTE_ROUTINE)(__in PCHAR ImagePath,__in ULONG Argc,__in PCHAR Argv[],__in PCHAR Envp[]);

//
// invoke
//
typedef ARC_STATUS (*PARC_INVOKE_ROUTINE)(__in ULONG EntryAddress,__in ULONG StackAddress,__in ULONG Argc,__in PCHAR Argv[],__in PCHAR Envp[]);

//
// load
//
typedef ARC_STATUS (*PARC_LOAD_ROUTINE)(__in PCHAR ImagePath,__in ULONG TopAddress,__in PULONG EntryAddress,__in PULONG LowAddress);

//
// halt
//
typedef VOID (*PARC_HALT_ROUTINE)();

//
// power down
//
typedef VOID (*PARC_POWERDOWN_ROUTINE)();

//
// restart
//
typedef VOID (*PARC_RESTART_ROUTINE)();

//
// reboot
//
typedef VOID (*PARC_REBOOT_ROUTINE)();

//
// enter interactive mode
//
typedef VOID (*PARC_INTERACTIVE_MODE_ROUTINE)();

//
// get memory descriptor
//
typedef PMEMORY_DESCRIPTOR (*PARC_MEMORY_ROUTINE)(__in_opt PMEMORY_DESCRIPTOR MemoryDescriptor);

//
// get child
//
typedef PCONFIGURATION_COMPONENT (*PARC_GET_CHILD_ROUTINE)(__in_opt PCONFIGURATION_COMPONENT Component);

//
// get parent
//
typedef PCONFIGURATION_COMPONENT (*PARC_GET_PARENT_ROUTINE)(__in PCONFIGURATION_COMPONENT Component);

//
// get peer
//
typedef PCONFIGURATION_COMPONENT (*PARC_GET_PEER_ROUTINE)(__in PCONFIGURATION_COMPONENT Component);

//
// add child
//
typedef PCONFIGURATION_COMPONENT (*PARC_ADD_CHILD_ROUTINE)(__in PCONFIGURATION_COMPONENT Component,__in PCONFIGURATION_COMPONENT NewComponent,__in PVOID ConfigData);

//
// delete component
//
typedef ARC_STATUS (*PARC_DELETE_COMPONENT_ROUTINE)(__in PCONFIGURATION_COMPONENT Component);

//
// get component
//
typedef PCONFIGURATION_COMPONENT (*PARC_GET_COMPONENT_ROUTINE)(__in PCHAR Path);

//
// get data
//
typedef ARC_STATUS (*PARC_GET_DATA_ROUTINE)(__out PVOID ConfigurationData,__in PCONFIGURATION_COMPONENT Component);

//
// save config
//
typedef ARC_STATUS (*PARC_SAVE_CONFIGURATION_ROUTINE)();

//
// get system id
//
typedef PSYSTEM_ID (*PARC_GET_SYSTEM_ID_ROUTINE)();

//
// get time
//
typedef PTIME_FIELDS (*PARC_GET_TIME_ROUTINE)();

//
// get relative time
//
typedef ULONG (*PARC_GET_RELATIVE_TIME_ROUTINE)();

//
// close
//
typedef ARC_STATUS (*PARC_CLOSE_ROUTINE)(__in ULONG FileId);

//
// mount
//
typedef ARC_STATUS (*PARC_MOUNT_ROUTINE)(__in PCHAR MountPath,__in MOUNT_OPERATION Operation);

//
// open
//
typedef ARC_STATUS (*PARC_OPEN_ROUTINE)(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId);

//
// read
//
typedef ARC_STATUS (*PARC_READ_ROUTINE)(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count);

//
// read satus
//
typedef ARC_STATUS (*PARC_READ_STATUS_ROUTINE)(__in ULONG FileId);

//
// seek
//
typedef ARC_STATUS (*PARC_SEEK_ROUTINE)(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode);

//
// write
//
typedef ARC_STATUS (*PARC_WRITE_ROUTINE)(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count);

//
// get file info
//
typedef ARC_STATUS (*PARC_GET_FILE_INFO_ROUTINE)(__in ULONG FileId,__out PFILE_INFORMATION FileInformation);

//
// set file info
//
typedef ARC_STATUS (*PARC_SET_FILE_INFO_ROUTINE)(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask);

//
// get dir entry
//
typedef ARC_STATUS (*PARC_GET_DIRECTORY_ENTRY_ROUTINE)(__in ULONG FileId,__out PDIRECTORY_ENTRY Buffer,__in ULONG Length,__out PULONG Count);

//
// get env
//
typedef PCHAR (*PARC_GET_ENVIRONMENT_ROUTINE)(__in PCHAR Variable);

//
// set env
//
typedef ARC_STATUS (*PARC_SET_ENVIRONMENT_ROUTINE)(__in PCHAR Variable,__in PCHAR Value);

//
// flush all caches
//
typedef VOID (*PARC_FLUSH_ALL_CACHES_ROUTINE)();

//
// test unicode
//
typedef ARC_STATUS (*PARC_TEST_UNICODE_CHARACTER_ROUTINE)(__in ULONG FileId,__in WCHAR UnicodeCharacter);

//
// get display status
//
typedef PARC_DISPLAY_STATUS (*PARC_GET_DISPLAY_STATUS_ROUTINE)(__in ULONG FileId);

//
// system block
//
#define SYSTEM_BLOCK									(&GlobalSystemBlock)

//
// execute
//
#define ArcExecute(ImagePath,Argc,Argv,Envp)			((PARC_EXECUTE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ExecuteRoutine]))((ImagePath),(Argc),(Argv),(Envp))

//
// invoke
//
#define ArcInvoke(Address,Stack,Argc,Argv,Envp)			((PARC_INVOKE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[InvokeRoutine]))((Address),(Stack),(Argc),(Argv),(Envp))

//
// load
//
#define ArcLoad(Path,Top,Entry,Low)						((PARC_LOAD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[LoadRoutine]))((Path),(Top),(Entry),(Low))

//
// halt
//
#define ArcHalt()										((PARC_HALT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[HaltRoutine]))()

//
// power down
//
#define ArcPowerDown()									((PARC_POWERDOWN_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[PowerDownRoutine]))()

//
// restrart
//
#define ArcRestart()									((PARC_RESTART_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[RestartRoutine]))()

//
// reboot
//
#define ArcReboot()										((PARC_REBOOT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[RebootRoutine]))()

//
// enter interactive mode
//
#define ArcEnterInteractiveMode()						((PARC_INTERACTIVE_MODE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[InteractiveModeRoutine]))()

//
// get child
//
#define ArcGetChild(Component)							((PARC_GET_CHILD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetChildRoutine]))((Component))

//
// get parent
//
#define ArcGetParent(Component)							((PARC_GET_PARENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetParentRoutine]))((Component))

//
// get peer
//
#define ArcGetPeer(Component)							((PARC_GET_PEER_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetPeerRoutine]))((Component))

//
// add child
//
#define ArcAddChild(Component,New,ConfigData)			((PARC_ADD_CHILD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[AddChildRoutine]))((Component),(New),(ConfigData))

//
// delete component
//
#define ArcDeleteComponent(Component)					((PARC_DELETE_COMPONENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[DeleteComponentRoutine]))((Component))

//
// get component
//
#define ArcGetComponent(Path)							((PARC_GET_COMPONENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetComponentRoutine]))((Path))

//
// get configuration data
//
#define ArcGetConfigurationData(ConfigData,Component)	((PARC_GET_DATA_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDataRoutine]))((ConfigData), (Component))

//
// save configuration
//
#define ArcSaveConfiguration()							((PARC_SAVE_CONFIGURATION_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SaveConfigurationRoutine]))()

//
// get system id
//
#define ArcGetSystemId()								((PARC_GET_SYSTEM_ID_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetSystemIdRoutine]))()

//
// get memory descriptor
//
#define ArcGetMemoryDescriptor(MemoryDescriptor)		((PARC_MEMORY_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[MemoryRoutine]))((MemoryDescriptor))

//
// get time
//
#define ArcGetTime()									((PARC_GET_TIME_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetTimeRoutine]))()

//
// get relative time
//
#define ArcGetRelativeTime()							((PARC_GET_RELATIVE_TIME_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetRelativeTimeRoutine]))()

//
// close
//
#define ArcClose(FileId)								((PARC_CLOSE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[CloseRoutine]))((FileId))

//
// get read status
//
#define ArcGetReadStatus(FileId)						((PARC_READ_STATUS_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ReadStatusRoutine]))((FileId))

//
// mount
//
#define ArcMount(MountPath,Operation)					((PARC_MOUNT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[MountRoutine]))((MountPath),(Operation))

//
// open
//
#define ArcOpen(OpenPath,OpenMode,FileId)				((PARC_OPEN_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[OpenRoutine]))((OpenPath),(OpenMode),(FileId))

//
// read
//
#define ArcRead(FileId,Buffer,Length,Count)				((PARC_READ_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ReadRoutine]))((FileId),(Buffer),(Length),(Count))

//
// seek
//
#define ArcSeek(FileId,Offset,SeekMode)					((PARC_SEEK_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SeekRoutine]))((FileId),(Offset),(SeekMode))

//
// write
//
#define ArcWrite(FileId,Buffer,Length,Count)			((PARC_WRITE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[WriteRoutine]))((FileId),(Buffer),(Length),(Count))

//
// get file info
//
#define ArcGetFileInformation(FileId,FileInfo)			((PARC_GET_FILE_INFO_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetFileInformationRoutine]))((FileId),(FileInfo))

//
// set file info
//
#define ArcSetFileInformation(FileId,Flags,Mask)		((PARC_SET_FILE_INFO_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SetFileInformationRoutine]))((FileId),(Flags),(Mask))

//
// get dir entry
//
#define ArcGetDirectoryEntry(Id,B,L,C)					((PARC_GET_DIRECTORY_ENTRY_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDirectoryEntryRoutine]))((Id),(B),(L),(C))

//
// get env var
//
#define ArcGetEnvironmentVariable(Variable)				((PARC_GET_ENVIRONMENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetEnvironmentRoutine]))((Variable))

//
// set envvar
//
#define ArcSetEnvironmentVariable(Variable,Value)		((PARC_SET_ENVIRONMENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SetEnvironmentRoutine]))((Variable),(Value))

//
// flush caches
//
#define ArcFlushAllCaches()								((PARC_FLUSH_ALL_CACHES_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[FlushAllCachesRoutine]))()

//
// test unicode
//
#define ArcTestUnicodeCharacter(Id,Char)				((PARC_TEST_UNICODE_CHARACTER_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[TestUnicodeCharacterRoutine]))((Id),(Char))

//
// get display status
//
#define ArcGetDisplayStatus(FileId)						((PARC_GET_DISPLAY_STATUS_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDisplayStatusRoutine]))((FileId))

//
// fills in all the fields in the Global System Parameter Block that it can
//
VOID BlFillInSystemParameters(__in PBOOT_CONTEXT BootContextRecord);

//
// initialize stall
//
VOID AEInitializeStall();

//
// sleep
//
VOID FwStallExecution(__in ULONG Microseconds);

//
// get path mnemonic key
//
BOOLEAN FwGetPathMnemonicKey(__in PCHAR OpenPath,__in PCHAR Mnemonic,__in PULONG Key);

//
// stub
//
ARC_STATUS BlArcNotYetImplemented(__in ULONG FileId);

//
// initialize io
//
ARC_STATUS AEInitializeIo(__in ULONG DriveId);

//
// terminate io
//
VOID AETerminateIo();

//
// system parameter
//
extern SYSTEM_PARAMETER_BLOCK							GlobalSystemBlock;

//
// stall counter
//
extern ULONG											FwStallCounter;

//
// firmware config tree
//
extern PCONFIGURATION_COMPONENT_DATA					FwConfigurationTree;