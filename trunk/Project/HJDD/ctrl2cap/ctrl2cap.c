///
/// @file		    ctrl2cap.c
/// @author	wowocock
/// @date		2009-1-27
/// 

#include <wdm.h>
#include <ntddkbd.h>
#include "ctrl2cap.h"

typedef struct _C2P_DEV_EXT 
{ 
    // ����ṹ�Ĵ�С
    ULONG NodeSize; 
    // �����豸����
    PDEVICE_OBJECT pFilterDeviceObject;
    // ͬʱ����ʱ�ı�����
    KSPIN_LOCK IoRequestsSpinLock;
    // ���̼�ͬ������  
    KEVENT IoInProgressEvent; 
    // �󶨵��豸����
    PDEVICE_OBJECT TargetDeviceObject; 
    // ��ǰ�ײ��豸����
    PDEVICE_OBJECT LowerDeviceObject; 
} C2P_DEV_EXT, *PC2P_DEV_EXT;

// flags for keyboard status
#define	S_SHIFT				1
#define	S_CAPS				2
#define	S_NUM				4
static int kb_status = S_NUM;
void __stdcall print_keystroke(UCHAR sch)
{
	UCHAR	ch = 0;
	int		off = 0;

	if ((sch & 0x80) == 0)	//make
	{
		if ((sch < 0x47) || 
			((sch >= 0x47 && sch < 0x54) && (kb_status & S_NUM))) // Num Lock
		{
			ch = asciiTbl[off+sch];
		}

		switch (sch)
		{
		case 0x3A:
			kb_status ^= S_CAPS;
			break;

		case 0x2A:
		case 0x36:
			kb_status |= S_SHIFT;
			break;

		case 0x45:
			kb_status ^= S_NUM;
		}
	}
	else		//break
	{
		if (sch == 0xAA || sch == 0xB6)
			kb_status &= ~S_SHIFT;
	}

	if (ch >= 0x20 && ch < 0x7F)
	{
		DbgPrint("%C \n",ch);
	}

}

NTSTATUS 
c2pDevExtInit( 
    IN PC2P_DEV_EXT devExt, 
    IN PDEVICE_OBJECT pFilterDeviceObject, 
    IN PDEVICE_OBJECT pTargetDeviceObject, 
    IN PDEVICE_OBJECT pLowerDeviceObject ) 
{ 
    memset(devExt, 0, sizeof(C2P_DEV_EXT)); 
    devExt->NodeSize = sizeof(C2P_DEV_EXT); 
    devExt->pFilterDeviceObject = pFilterDeviceObject; 
    KeInitializeSpinLock(&(devExt->IoRequestsSpinLock)); 
    KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE); 
    devExt->TargetDeviceObject = pTargetDeviceObject; 
    devExt->LowerDeviceObject = pLowerDeviceObject; 
    return( STATUS_SUCCESS ); 
}

// �����������ʵ���ڵģ�ֻ���ĵ���û�й���������һ��
// �Ϳ���ֱ��ʹ���ˡ�
NTSTATUS
ObReferenceObjectByName(
                        PUNICODE_STRING ObjectName,
                        ULONG Attributes,
                        PACCESS_STATE AccessState,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_TYPE ObjectType,
                        KPROCESSOR_MODE AccessMode,
                        PVOID ParseContext,
                        PVOID *Object
                        );

extern POBJECT_TYPE IoDriverObjectType;
ULONG gC2pKeyCount = 0;
PDRIVER_OBJECT gDriverObject = NULL;

// ��������������졣�ܴ���������Kbdclass��Ȼ���
// ����������е��豸��
NTSTATUS 
c2pAttachDevices( 
                  IN PDRIVER_OBJECT DriverObject, 
                  IN PUNICODE_STRING RegistryPath 
                  ) 
{ 
    NTSTATUS status = 0; 
    UNICODE_STRING uniNtNameString; 
    PC2P_DEV_EXT devExt; 
    PDEVICE_OBJECT pFilterDeviceObject = NULL; 
    PDEVICE_OBJECT pTargetDeviceObject = NULL; 
    PDEVICE_OBJECT pLowerDeviceObject = NULL; 

    PDRIVER_OBJECT KbdDriverObject = NULL; 

    KdPrint(("MyAttach\n")); 

    // ��ʼ��һ���ַ���������Kdbclass���������֡�
    RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME); 
    // �����ǰ����豸��������ӡ�ֻ������򿪵�����������
    status = ObReferenceObjectByName ( 
        &uniNtNameString, 
        OBJ_CASE_INSENSITIVE, 
        NULL, 
        0, 
        IoDriverObjectType, 
        KernelMode, 
        NULL, 
        &KbdDriverObject 
        ); 
    // ���ʧ���˾�ֱ�ӷ���
    if(!NT_SUCCESS(status)) 
    { 
        KdPrint(("MyAttach: Couldn't get the MyTest Device Object\n")); 
        return( status ); 
    }
    else
    {
        // �������Ҫ��Ӧ�á�����������֮�����ǡ�
        ObDereferenceObject(DriverObject);
    }

    // �����豸���еĵ�һ���豸	
    pTargetDeviceObject = KbdDriverObject->DeviceObject;
    // ���ڿ�ʼ��������豸��
    while (pTargetDeviceObject) 
    {
        // ����һ�������豸������ǰ�����ѧϰ���ġ������IN���OUT�궼��
        // �պֻ꣬�б�־�����壬�������������һ������������������
        status = IoCreateDevice( 
            IN DriverObject, 
            IN sizeof(C2P_DEV_EXT), 
            IN NULL, 
            IN pTargetDeviceObject->DeviceType, 
            IN pTargetDeviceObject->Characteristics, 
            IN FALSE, 
            OUT &pFilterDeviceObject 
            ); 

        // ���ʧ���˾�ֱ���˳���
        if (!NT_SUCCESS(status)) 
        { 
            KdPrint(("MyAttach: Couldn't create the MyFilter Filter Device Object\n")); 
            return (status); 
        } 

        // �󶨡�pLowerDeviceObject�ǰ�֮��õ�����һ���豸��Ҳ����
        // ǰ�泣��˵����ν��ʵ�豸��
        pLowerDeviceObject = 
            IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject); 
        // �����ʧ���ˣ�����֮ǰ�Ĳ������˳���
        if(!pLowerDeviceObject) 
        { 
            KdPrint(("MyAttach: Couldn't attach to MyTest Device Object\n")); 
            IoDeleteDevice(pFilterDeviceObject); 
            pFilterDeviceObject = NULL; 
            return( status ); 
        } 

        // �豸��չ������Ҫ��ϸ�����豸��չ��Ӧ�á�
        devExt = (PC2P_DEV_EXT)(pFilterDeviceObject->DeviceExtension); 
        c2pDevExtInit( 
            devExt, 
            pFilterDeviceObject, 
            pTargetDeviceObject, 
            pLowerDeviceObject ); 

        // ����Ĳ�����ǰ����˴��ڵĲ�������һ�¡����ﲻ�ٽ����ˡ�
        pFilterDeviceObject->DeviceType=pLowerDeviceObject->DeviceType; 
        pFilterDeviceObject->Characteristics=pLowerDeviceObject->Characteristics; 
        pFilterDeviceObject->StackSize=pLowerDeviceObject->StackSize+1; 
        pFilterDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE) ; 
        //next device 
        pTargetDeviceObject = pTargetDeviceObject->NextDevice;
    }
    return status; 
} 

VOID 
c2pDetach(IN PDEVICE_OBJECT pDeviceObject) 
{ 
	PC2P_DEV_EXT devExt; 
	BOOLEAN NoRequestsOutstanding = FALSE; 
	devExt = (PC2P_DEV_EXT)pDeviceObject->DeviceExtension; 
	__try 
	{ 
		__try 
		{ 
			IoDetachDevice(devExt->TargetDeviceObject);
			devExt->TargetDeviceObject = NULL; 
			IoDeleteDevice(pDeviceObject); 
			devExt->pFilterDeviceObject = NULL; 
			DbgPrint(("Detach Finished\n")); 
		} 
		__except (EXCEPTION_EXECUTE_HANDLER){} 
	} 
	__finally{} 
	return; 
}


#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

VOID 
c2pUnload(IN PDRIVER_OBJECT DriverObject) 
{ 
    PDEVICE_OBJECT DeviceObject; 
    PDEVICE_OBJECT OldDeviceObject; 
    PC2P_DEV_EXT devExt; 

    LARGE_INTEGER	lDelay;
    PRKTHREAD CurrentThread;
    //delay some time 
    lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);
    CurrentThread = KeGetCurrentThread();
    // �ѵ�ǰ�߳�����Ϊ��ʵʱģʽ���Ա����������о�����Ӱ����������
    KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

    UNREFERENCED_PARAMETER(DriverObject); 
    KdPrint(("DriverEntry unLoading...\n")); 

    // ���������豸��һ�ɽ����
    DeviceObject = DriverObject->DeviceObject;
    while (DeviceObject)
    {
        // ����󶨲�ɾ�����е��豸
        c2pDetach(DeviceObject);
        DeviceObject = DeviceObject->NextDevice;
    } 
    ASSERT(NULL == DriverObject->DeviceObject);

    while (gC2pKeyCount)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
    }
    KdPrint(("DriverEntry unLoad OK!\n")); 
    return; 
} 

NTSTATUS c2pDispatchGeneral( 
                                 IN PDEVICE_OBJECT DeviceObject, 
                                 IN PIRP Irp 
                                 ) 
{ 
    // �����ķַ�������ֱ��skipȻ����IoCallDriver��IRP���͵���ʵ�豸
    // ���豸���� 
    KdPrint(("Other Diapatch!")); 
    IoSkipCurrentIrpStackLocation(Irp); 
    return IoCallDriver(((PC2P_DEV_EXT)
        DeviceObject->DeviceExtension)->LowerDeviceObject, Irp); 
} 

NTSTATUS c2pPower( 
                       IN PDEVICE_OBJECT DeviceObject, 
                       IN PIRP Irp 
                       ) 
{ 
    PC2P_DEV_EXT devExt;
    devExt =
        (PC2P_DEV_EXT)DeviceObject->DeviceExtension; 

    PoStartNextPowerIrp( Irp ); 
    IoSkipCurrentIrpStackLocation( Irp ); 
    return PoCallDriver(devExt->LowerDeviceObject, Irp ); 
} 

NTSTATUS c2pPnP( 
                     IN PDEVICE_OBJECT DeviceObject, 
                     IN PIRP Irp 
                     ) 
{ 
    PC2P_DEV_EXT devExt; 
    PIO_STACK_LOCATION irpStack; 
    NTSTATUS status = STATUS_SUCCESS; 
    KIRQL oldIrql; 
    KEVENT event; 

    // �����ʵ�豸��
    devExt = (PC2P_DEV_EXT)(DeviceObject->DeviceExtension); 
    irpStack = IoGetCurrentIrpStackLocation(Irp); 

    switch (irpStack->MinorFunction) 
    { 
    case IRP_MN_REMOVE_DEVICE: 
        KdPrint(("IRP_MN_REMOVE_DEVICE\n")); 

        // ���Ȱ�������ȥ
        IoSkipCurrentIrpStackLocation(Irp); 
        IoCallDriver(devExt->LowerDeviceObject, Irp); 
        // Ȼ�����󶨡�
        IoDetachDevice(devExt->LowerDeviceObject); 
        // ɾ�������Լ����ɵ������豸��
        IoDeleteDevice(DeviceObject); 
        status = STATUS_SUCCESS; 
        break; 

    default: 
        // �����������͵�IRP��ȫ����ֱ���·����ɡ� 
        IoSkipCurrentIrpStackLocation(Irp); 
        status = IoCallDriver(devExt->LowerDeviceObject, Irp); 
    } 
    return status; 
}

// ����һ��IRP��ɻص�������ԭ��
NTSTATUS c2pReadComplete( 
                              IN PDEVICE_OBJECT DeviceObject, 
                              IN PIRP Irp, 
                              IN PVOID Context 
                              ) 
{
     PIO_STACK_LOCATION IrpSp;
     ULONG buf_len = 0;
     PUCHAR buf = NULL;
     size_t i,numKeys;

	 PKEYBOARD_INPUT_DATA KeyData; 

     IrpSp = IoGetCurrentIrpStackLocation( Irp );

     //  �����������ǳɹ��ġ�����Ȼ���������ʧ���ˣ���ô��ȡ
     //   ��һ������Ϣ��û����ġ�
     if( NT_SUCCESS( Irp->IoStatus.Status ) ) 
     {
        // ��ö�������ɺ�����Ļ�����
        buf = Irp->AssociatedIrp.SystemBuffer;
		KeyData = (PKEYBOARD_INPUT_DATA)buf;
        // �������������ĳ��ȡ�һ���˵����ֵ�ж೤��������
        // Information�С�
        buf_len = Irp->IoStatus.Information;
        numKeys = buf_len / sizeof(KEYBOARD_INPUT_DATA);
        //�� �����������һ���Ĵ���������ܼ򵥵Ĵ�ӡ�����е�ɨ
        // ���롣
        //for(i=0;i<buf_len;++i)
		for(i=0;i<numKeys;++i)
        {
            //DbgPrint("ctrl2cap: %2x\r\n", buf[i]);
			DbgPrint("\n");
			DbgPrint("numKeys : %d",numKeys);
			DbgPrint("ScanCode: %x ", KeyData->MakeCode ); 
			DbgPrint("%s\n", KeyData->Flags ?"Up" : "Down" );
			print_keystroke((UCHAR)KeyData->MakeCode);

			if( KeyData->MakeCode == CAPS_LOCK) 
			{ 
				KeyData->MakeCode = LCONTROL; 
			} 
        }
    }
    gC2pKeyCount--;

	if( Irp->PendingReturned )
	{ 
		IoMarkIrpPending( Irp ); 
	} 
    return Irp->IoStatus.Status;
}


NTSTATUS c2pDispatchRead( 
                              IN PDEVICE_OBJECT DeviceObject, 
                              IN PIRP Irp ) 
{ 
    NTSTATUS status = STATUS_SUCCESS; 
    PC2P_DEV_EXT devExt; 
    PIO_STACK_LOCATION currentIrpStack; 
    KEVENT waitEvent;
    KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

	if (Irp->CurrentLocation == 1) 
	{ 
		ULONG ReturnedInformation = 0; 
		KdPrint(("Dispatch encountered bogus current location\n")); 
		status = STATUS_INVALID_DEVICE_REQUEST; 
		Irp->IoStatus.Status = status; 
		Irp->IoStatus.Information = ReturnedInformation; 
		IoCompleteRequest(Irp, IO_NO_INCREMENT); 
		return(status); 
	} 

    // ȫ�ֱ�������������1
    gC2pKeyCount++;

    // �õ��豸��չ��Ŀ����֮��Ϊ�˻����һ���豸��ָ�롣
    devExt =
        (PC2P_DEV_EXT)DeviceObject->DeviceExtension;

    // ���ûص���������IRP������ȥ�� ֮����Ĵ���Ҳ�ͽ����ˡ�
    // ʣ�µ�������Ҫ�ȴ���������ɡ�
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp); 
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine( Irp, c2pReadComplete, 
        DeviceObject, TRUE, TRUE, TRUE ); 
    return  IoCallDriver( devExt->LowerDeviceObject, Irp ); 	
}

NTSTATUS DriverEntry( 
                     IN PDRIVER_OBJECT DriverObject, 
                     IN PUNICODE_STRING RegistryPath 
                     ) 
{ 
    ULONG i; 
    NTSTATUS status; 
    KdPrint (("c2p.SYS: entering DriverEntry\n")); 

    // ��д���еķַ�������ָ��
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) 
    { 
        DriverObject->MajorFunction[i] = c2pDispatchGeneral; 
    } 

    // ��������дһ��Read�ַ���������ΪҪ�Ĺ��˾��Ƕ�ȡ���İ�����Ϣ
    // �����Ķ�����Ҫ������ַ���������д��
    DriverObject->MajorFunction[IRP_MJ_READ] = c2pDispatchRead; 

    // ��������дһ��IRP_MJ_POWER������������Ϊ���������м�Ҫ����
    // һ��PoCallDriver��һ��PoStartNextPowerIrp���Ƚ����⡣
    DriverObject->MajorFunction [IRP_MJ_POWER] = c2pPower; 

    // ������֪��ʲôʱ��һ�����ǰ󶨹����豸��ж���ˣ�����ӻ�����
    // ���ε��ˣ�������ר��дһ��PNP�����弴�ã��ַ�����
    DriverObject->MajorFunction [IRP_MJ_PNP] = c2pPnP; 

    // ж�غ�����
    DriverObject->DriverUnload = c2pUnload; 
    gDriverObject = DriverObject;
    // �����м����豸
    status =c2pAttachDevices(DriverObject, RegistryPath);

    return status; 
}

