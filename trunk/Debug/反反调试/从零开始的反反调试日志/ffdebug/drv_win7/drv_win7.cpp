///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - <company name here>
///
/// Original filename: drv_win7.cpp
/// Project          : drv_win7
/// Date of creation : 2010-10-29
/// Author(s)        : <author name(s)>
///
/// Purpose          : <description>
///
/// Revisions:
///  0000 [2010-10-29] Initial revision.
///
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifdef __cplusplus
extern "C" {
#endif
#include <ntddk.h>
#include <windef.h>
#include <string.h>
#ifdef __cplusplus
}; // extern "C"
#endif

#include "drv_win7.h"
#include "../my_common.h"

#ifdef __cplusplus
namespace { // anonymous namespace to limit the scope of this global variable!
#endif
PDRIVER_OBJECT pdoGlobalDrvObj = 0;
#ifdef __cplusplus
}; // anonymous namespace
#endif

#define IOCTL_CODE_MODULELIST		(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x901,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CODE_SETSYSTEMDEBUG	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x902,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CODE_UNSETSYSTEMDEBUG	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x903,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CODE_PROCESSLIST		(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN,0x904,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union {
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union {
		struct {
			ULONG TimeDateStamp;
		};
		struct {
			PVOID LoadedImports;
		};
	};
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

void DisplayDebugFlags() ;
void SetSystemDebug( PVOID lpBase , BOOL bSetDebug ) ;
PEPROCESS FindProcess(const char* pDestProcess) ;
BOOL AddApolloGoodList() ;
void ListThread() ;
void ResetPojun() ;

PLDR_DATA_TABLE_ENTRY FindModule (IN PDRIVER_OBJECT DriverObject, PUNICODE_STRING pDestDriverName) ;

unsigned int InitIOServer(LPSTUSER_COMMAND lpOutBuff , const unsigned int iBuffSize )
{
	//����������µķ���
	//���û�����ķ�����Ϣ
	stUSER_COMMAND myServer[10] = {0} ;
	myServer[0].dwEx = IOCTL_CODE_MODULELIST ;
	wcscpy(myServer[0].strDescribe,L"ö��ϵͳģ��,�� DebugView �����");

	myServer[1].dwEx = IOCTL_CODE_SETSYSTEMDEBUG ;
	wcscpy(myServer[1].strDescribe,L"����ϵͳΪ Debug ģʽ");

	myServer[2].dwEx = IOCTL_CODE_UNSETSYSTEMDEBUG ;
	wcscpy(myServer[2].strDescribe,L"����ϵͳΪ����ģʽ");

	myServer[3].dwEx = IOCTL_CODE_PROCESSLIST ;
	wcscpy(myServer[3].strDescribe,L"ö�ٽ���,�� DebugView �����");


	//���´�������Ķ�..
	//�� OutBuff д�����������Ӧ�ķ�����Ϣ
	int max = (iBuffSize / sizeof(stUSER_COMMAND)) > (sizeof(myServer)/sizeof(myServer[0])) \
			? (sizeof(myServer)/sizeof(myServer[0])) \
			: (iBuffSize / sizeof(stUSER_COMMAND)) ;

	int wr = 0 ;
	for ( int i = 0 ; i < max ; i++ )
	{
		if ( myServer[i].dwEx )
		{
			memcpy(&lpOutBuff[wr],&myServer[i],sizeof(stUSER_COMMAND)) ;
			wr++ ;
		}
	}
	return wr * sizeof(stUSER_COMMAND) ;
}

NTSTATUS DRVWIN7_DispatchCreateClose(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DRVWIN7_DispatchDeviceControl(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
	KdPrint(("DRVWIN7_DispatchDeviceControl Entry")) ;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch(irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_DRVWIN7_OPERATION:
        // status = SomeHandlerFunction(irpSp);
        break;
	case DRV_GET_COMMAND_CODE:
		// ��������ķ�����û�
		{
			KdPrint(("==== DRV_GET_COMMAND_CODE ====")) ;
			//�������
			Irp->IoStatus.Information = InitIOServer((LPSTUSER_COMMAND)Irp->AssociatedIrp.SystemBuffer , irpSp->Parameters.DeviceIoControl.OutputBufferLength) ;
			Irp->IoStatus.Status = STATUS_SUCCESS ;
		}
		break ;
	case IOCTL_CODE_MODULELIST:
		{
			KdPrint(("==== DRV_MODULELIST ====")) ;
			FindModule(pdoGlobalDrvObj,NULL) ;
			Irp->IoStatus.Status = STATUS_SUCCESS ;
		}
		break ;
	case IOCTL_CODE_SETSYSTEMDEBUG:
	case IOCTL_CODE_UNSETSYSTEMDEBUG:
		{
			UNICODE_STRING destName = {0} ;
			RtlInitUnicodeString( &destName,L"ntoskrnl.exe") ;
			PLDR_DATA_TABLE_ENTRY lpNtoskrnl = FindModule(pdoGlobalDrvObj,&destName) ;
			if ( lpNtoskrnl )
			{
				BOOL bDebug = FALSE ;
				if ( IOCTL_CODE_SETSYSTEMDEBUG == irpSp->Parameters.DeviceIoControl.IoControlCode )
					bDebug = TRUE ;
				SetSystemDebug( lpNtoskrnl->DllBase , bDebug ) ;
				Irp->IoStatus.Status = STATUS_SUCCESS ;
				KdPrint(("%wZ,%wZ,  0x%04X - 0x%04X",&lpNtoskrnl->BaseDllName,&lpNtoskrnl->FullDllName,lpNtoskrnl->DllBase,lpNtoskrnl->SizeOfImage)) ;
			}
			else
				Irp->IoStatus.Status = STATUS_UNSUCCESSFUL ;
		}
		break ;
	case IOCTL_CODE_PROCESSLIST:
		{
			FindProcess(NULL) ;
		}
		break ;
    default:
		KdPrint(("==== No ====")) ;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;
    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	KdPrint(("DRVWIN7_DispatchDeviceControl Exit")) ;
    return status;
}

VOID DRVWIN7_DriverUnload(
    IN PDRIVER_OBJECT		DriverObject
    )
{
    PDEVICE_OBJECT pdoNextDeviceObj = pdoGlobalDrvObj->DeviceObject;
    IoDeleteSymbolicLink(&usSymlinkName);

    // Delete all the device objects
    while(pdoNextDeviceObj)
    {
        PDEVICE_OBJECT pdoThisDeviceObj = pdoNextDeviceObj;
        pdoNextDeviceObj = pdoThisDeviceObj->NextDevice;
        IoDeleteDevice(pdoThisDeviceObj);
    }
}

#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
{
	KdPrint(("==== DriverEntry ====")) ;

    PDEVICE_OBJECT pdoDeviceObj = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    pdoGlobalDrvObj = DriverObject;

    // Create the device object.
    if(!NT_SUCCESS(status = IoCreateDevice(
        DriverObject,
        0,
        &usDeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &pdoDeviceObj
        )))
    {
        // Bail out (implicitly forces the driver to unload).
        return status;
    };

    // Now create the respective symbolic link object
    if(!NT_SUCCESS(status = IoCreateSymbolicLink(
        &usSymlinkName,
        &usDeviceName
        )))
    {
        IoDeleteDevice(pdoDeviceObj);
        return status;
    }
	
    // NOTE: You need not provide your own implementation for any major function that
    //       you do not want to handle. I have seen code using DDKWizard that left the
    //       *empty* dispatch routines intact. This is not necessary at all!
    DriverObject->MajorFunction[IRP_MJ_CREATE] =
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DRVWIN7_DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DRVWIN7_DispatchDeviceControl;
    DriverObject->DriverUnload = DRVWIN7_DriverUnload;

    return STATUS_SUCCESS;
}
#ifdef __cplusplus
}; // extern "C"
#endif

////////////////////////////////		����Ϊ���Դ���		//////////////////////////////////////////

/*!
 *	��ʽ������ַ���
 *
 *	������ӡ�Ķ��������ݣ���ʽ���ɿɴ�ӡ�ַ�(ÿ�� 32 �ֽڣ������Ҳ�������ֽڵ� ASCII��)��
 *	ͨ�� OutDebugString��DbgPrint �����
 *
 *	@param lpSrc ����ӡ���ݵĵ�ַ
 *	@param len ����ӡ���ݵĳ���
 *	@param lpPre �Ƿ���ÿ�������ǰ�������������Ϣ��ΪNULL��ʾ��ͷ��Ϣ
 */
void OutDebugStringInLine32(const unsigned char* lpSrc, const int len ,const char* lpPre /*= NULL*/)
{
	if ( !lpSrc || len <= 0 )
		return ;
	char outmsgbuff[256] = {0} ;
	char* outLine = outmsgbuff ;
	memset( outmsgbuff,' ',sizeof(outmsgbuff)/sizeof(outmsgbuff[0])) ;
	int iPre = 0 ;
	if ( lpPre )
	{
		iPre = strlen(lpPre) ;
		if ( iPre < 100 )
		{
			outLine = &outmsgbuff[iPre] ;
			memcpy(outmsgbuff,lpPre,iPre) ;
		}
		else
		{
			DbgPrint("OutDebugStringInLine32 pre string is to long.") ;
		}
	}

	int iSrc = 0 , iInLine = 0 ;
	char asii[33] = {0} ;
	memset(asii,' ',32) ;

	for ( ; iSrc < len ; iSrc++ )
	{
		asii[iInLine] =  lpSrc[iSrc] > 0x1f ? lpSrc[iSrc] : '.' ;
		unsigned char hi = ( lpSrc[iSrc] & 0xF0 ) >> 4 ;
		hi += hi > 0x9 ? 0x37 : 0x30 ;
		unsigned char lo = ( lpSrc[iSrc] & 0x0F ) ;
		lo += lo > 0x9 ? 0x37 : 0x30 ;
		int bEx = 0 ;
		if ( iInLine >= 16 )
			bEx = 3 ;
		outLine[iInLine*3+bEx] = hi ;
		outLine[iInLine*3+1+bEx] = lo ;
		outLine[iInLine*3+2+bEx] = ' ' ;
		if ( iInLine >= 31 )
		{
			memcpy( &outLine[105] , asii, 33 ) ;
			DbgPrint(outmsgbuff) ;
			memset( outLine,' ',256-iPre) ;
			memset(asii,' ',32) ;
			iInLine = 0 ;
		}
		else
			iInLine++ ;
	}
	if ( iInLine != 0)
	{
		memcpy( &outLine[105] , asii, 33 ) ;
		DbgPrint(outmsgbuff) ;
	}
}

/*!
 *	����Ŀ��ģ��
 *
 *	ͨ������������� DriverSection ��˳���ѯ�ں�ģ�������Ƚ�ģ�����Ƿ���ͬ�������ִ�Сд��������Ŀ��ģ����Ϣ
 *
 *	@param DriverObject DriverEntry���ݽ�������������
 *	@param pDestDriverName Ŀ��ģ������
 *	@return Ŀ��ģ����Ϣ
 */
PLDR_DATA_TABLE_ENTRY FindModule(IN PDRIVER_OBJECT DriverObject, PUNICODE_STRING pDestDriverName)
{
	PLDR_DATA_TABLE_ENTRY pModuleCurrent = NULL;
	PLDR_DATA_TABLE_ENTRY PStop = NULL;

	if (DriverObject == NULL)
		return 0; 

	pModuleCurrent = (PLDR_DATA_TABLE_ENTRY)(DriverObject->DriverSection);
	if (pModuleCurrent == NULL)
		return 0; 

	PStop = pModuleCurrent;

	do 
	{
		if ( NULL == pDestDriverName)
		{
			// ntoskrnl.exe,\SystemRoot\system32\ntkrnlpa.exe,  ����ַ - ��С0x410000
			KdPrint(("%wZ [0x%04X - 0x%04X]  %wZ\n",&pModuleCurrent->BaseDllName,pModuleCurrent->DllBase,pModuleCurrent->SizeOfImage,&pModuleCurrent->FullDllName)) ;
		}
		else
		{
			if ( RtlEqualUnicodeString( &pModuleCurrent->BaseDllName,pDestDriverName,FALSE ))
				return pModuleCurrent ;
		}

		pModuleCurrent = (PLDR_DATA_TABLE_ENTRY)pModuleCurrent->InLoadOrderLinks.Flink;
	} while ( pModuleCurrent != PStop );

	return NULL;
} 

/*!
 *	����Ŀ�����
 *
 *	ͨ����������� ActiveProcessLinks ��˳���ѯ�����Ƚ�ģ�����Ƿ���ͬ����Сд���У�������Ŀ����̵� PEPROCESS ��Ϣ
 *
 *	@param lpDestProcessName Ŀ���������,����ʹ�õ���ӳ����,����15�ֽڵĲ��ֲ����ж�
 *	@return Ŀ�������Ϣ
 */
PEPROCESS FindProcess(const char* lpDestProcessName)
{
	//����ʹ�õ���ӳ�����������޷�ʹ�ô���15�ֽڵ�����
	DWORD dwOffset_ActiveProcessLinks = 0xb8 ;
	DWORD dwOffset_ImageFileName = 0x16c ;
	PEPROCESS pFirst = PsGetCurrentProcess() ;
	LIST_ENTRY *lpListEntry = NULL ;
	char *lpName = NULL ;
	PEPROCESS pNext = pFirst ;
	
	do {
		lpName = (char*)((char*)pNext+dwOffset_ImageFileName) ;
		if ( lpDestProcessName && ( 0 == strcmp(lpName , lpDestProcessName)))
			return pNext ;

		KdPrint(("%s [0x%04X]\n",lpName,(DWORD)pNext)) ;
		lpListEntry = (LIST_ENTRY*)((DWORD)pNext + dwOffset_ActiveProcessLinks) ;
		pNext = (PEPROCESS)((char*)lpListEntry->Flink - dwOffset_ActiveProcessLinks) ;
	}while (pFirst != pNext) ;

	return NULL ;
}

/*!
 * ����ϵͳģʽ
 *
 * �ڲ�ͬģʽ�·ֱ����ñ�־λ(KdpBootedNodebug��KdPitchDebugger��KdDebugEnabled)���޸�lpKidebugRoutine(Debug��Ϊ KdpTrap������Ϊ KdpTrace)
 *
 * @param lpBase lpBase Ϊ ntoskrnl.exe ģ�����ַ
 * @param bSetDebug ΪTrue ʱ����ʾ����Ϊdebugģʽ����������Ϊ����
 */
void SetSystemDebug( PVOID lpBase , BOOL bSetDebug )
{

	KdPrint(("BASE:0x%08X",lpBase)) ;
	BYTE* lpKdpBootedNoDebug = (BYTE*)lpBase+0x36efea ;				//$+0x36efea
	KdPrint(("lpKdpBootedNoDebug[0x%08X],%d",lpKdpBootedNoDebug,*lpKdpBootedNoDebug)) ;
	BYTE* lpKdPitchDebugger = (BYTE*)lpBase+0x127ce7 ;				//$+0x127ce7
	KdPrint(("lpKdPitchDebugger[0x%08X]:%d",lpKdPitchDebugger,*lpKdPitchDebugger)) ;
	BYTE* lpKdDebugEnabled = (BYTE*)lpBase+0x163cec ;				//$+0x163cec
	KdPrint(("lpKdDebugEnabled[0x%08X]:%d",lpKdDebugEnabled,*lpKdDebugEnabled)) ;
	DWORD* lpKiDebugRoutine = (DWORD*)((BYTE*)lpBase+0x1689bc) ;	//$+0x1689bc
	KdPrint(("lpKiDebugRoutine[0x%08X],0x%08X",lpKiDebugRoutine,*lpKiDebugRoutine)) ; 
	DWORD lpKdpTrap = (DWORD)lpBase + 0x32b4f2 ;
	KdPrint(("lpKdpTrap[0x%08X]",lpKdpTrap)) ;
	DWORD lpKdpStub = (DWORD)lpBase + 0xdb793 ;
	KdPrint(("lpKdpStub[0x%08X]",lpKdpStub)) ;


	if ( bSetDebug )
	{
		*lpKdpBootedNoDebug = 0;
		*lpKdPitchDebugger = 0 ;
		*lpKdDebugEnabled = 1 ;
		*lpKiDebugRoutine = lpKdpTrap ;
	}
	else
	{
		*lpKdpBootedNoDebug = 1;
		*lpKdPitchDebugger = 1 ;
		*lpKdDebugEnabled = 0 ;
		*lpKiDebugRoutine = lpKdpStub ;
	}
}
