/*
 *                                    SSDT��������
 *                                     Made By Adly
 *
 *                                   QQ:369046978
 *
 */
/////////////////////////////////////////////////////////////////////////
//IOCTL.h
#define	FILE_DRIVER_SSDT	0x0000420
#define	SystemModuleInfo	0x0B

#define	DRIVER_NAME		L"\\Device\\SSDT"
#define	DEVICE_NAME		L"\\DosDevices\\SSDT"

#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#endif // CTL_CODE

#ifndef METHOD_NEITHER
#define METHOD_NEITHER 3
#endif // METHOD_NEITHER

//////////////////////////////////////////////////////////////////////////
//SSDT �ṹ��
typedef struct _tagSSDT {
    PVOID pvSSDTBase;
    PVOID pvServiceCounterTable;
    ULONG ulNumberOfServices;
    PVOID pvParamTableBase;
} SSDT, *PSSDT;
/////////////////////////////////////////////////////////////////////////
// ModuleInfo �ṹ��
typedef struct ModuleInfo_t {
	ULONG Unused;
	ULONG Always0;
	ULONG ModuleBaseAddress;
	ULONG ModuleSize;
	ULONG Unknown;
	ULONG ModuleEntryIndex;
	/* Length of module name not including the path, this field contains valid value only for NTOSKRNL module*/
	USHORT ModuleNameLength; 
	USHORT ModulePathLength; /*Length of 'directory path' part of modulename*/
	char ModuleName[256];
} DRIVERMODULEINFO, *PDRIVERMODULEINFO;
////////////////////////////////////////////////////////////////////
//��ȡSSDT�ṹ
#define IOCTL_GETSSDT  (ULONG)CTL_CODE( FILE_DRIVER_SSDT, 0x01, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

//����SSDT�ṹ
#define IOCTL_SETSSDT  (ULONG)CTL_CODE( FILE_DRIVER_SSDT, 0x02, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

//��ѯSSDT HOOK������ַ
#define IOCTL_GETHOOK  (ULONG)CTL_CODE( FILE_DRIVER_SSDT, 0x03, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

//����SSDT HOOK������ַ
#define IOCTL_SETHOOK  (ULONG)CTL_CODE( FILE_DRIVER_SSDT, 0x04, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )