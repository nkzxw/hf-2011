///
/// @file		comcap.c
/// @author	crazy_chu
/// @date		2008-6-18
/// 

#include <ntddk.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif
#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif
#define CCP_MAX_COM_ID 32

// �����豸����ʵ�豸
static PDEVICE_OBJECT s_fltobj[CCP_MAX_COM_ID] = { 0 };
static PDEVICE_OBJECT s_nextobj[CCP_MAX_COM_ID] = { 0 };

// ��һ���˿��豸
PDEVICE_OBJECT ccpOpenCom(ULONG id,NTSTATUS *status)
{
	UNICODE_STRING name_str;
	static WCHAR name[32] = { 0 };
	PFILE_OBJECT fileobj = NULL;
	PDEVICE_OBJECT devobj = NULL;

	// �����ַ�����
	memset(name,0,sizeof(WCHAR)*32);
	RtlStringCchPrintfW(
		name,32,
		L"\\Device\\Serial%d",id);
	RtlInitUnicodeString(&name_str,name);

	// ���豸����
	*status = IoGetDeviceObjectPointer(&name_str, FILE_ALL_ACCESS, &fileobj, &devobj);
	if (*status == STATUS_SUCCESS)
		ObDereferenceObject(fileobj);

	return devobj;
}

NTSTATUS
ccpAttachDevice(
				PDRIVER_OBJECT driver, 
				PDEVICE_OBJECT oldobj,
				PDEVICE_OBJECT *fltobj, 
				PDEVICE_OBJECT *next)
{
	NTSTATUS status;
	PDEVICE_OBJECT topdev = NULL;

	// �����豸��Ȼ���֮��
	status = IoCreateDevice(driver,
							0,
							NULL,
							oldobj->DeviceType,
							0,
							FALSE,
							fltobj);

	if (status != STATUS_SUCCESS)
		return status;

	// ������Ҫ��־λ��
	if(oldobj->Flags & DO_BUFFERED_IO)
		(*fltobj)->Flags |= DO_BUFFERED_IO;
	if(oldobj->Flags & DO_DIRECT_IO)
		(*fltobj)->Flags |= DO_DIRECT_IO;
	if(oldobj->Flags & DO_BUFFERED_IO)
		(*fltobj)->Flags |= DO_BUFFERED_IO;
	if(oldobj->Characteristics & FILE_DEVICE_SECURE_OPEN)
		(*fltobj)->Characteristics |= FILE_DEVICE_SECURE_OPEN;
	(*fltobj)->Flags |=  DO_POWER_PAGABLE;
	// ��һ���豸����һ���豸��
	topdev = IoAttachDeviceToDeviceStack(*fltobj,oldobj);
	if (topdev == NULL)
	{
		// �����ʧ���ˣ������豸������������
		IoDeleteDevice(*fltobj);
		*fltobj = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	*next = topdev;

	// ��������豸�Ѿ�������
	(*fltobj)->Flags = (*fltobj)->Flags & ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

// ������������еĴ��ڡ�
void ccpAttachAllComs(PDRIVER_OBJECT driver)
{
	ULONG i;
	PDEVICE_OBJECT com_ob;
	NTSTATUS status;
	for(i = 0;i<CCP_MAX_COM_ID;i++)
	{
		// ���object���á�
		com_ob = ccpOpenCom(i,&status);
		if(com_ob == NULL)
			continue;
		// ������󶨡������ܰ��Ƿ�ɹ���
		ccpAttachDevice(driver,com_ob,&s_fltobj[i],&s_nextobj[i]);
		// ȡ��object���á�
	}
}

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

void ccpUnload(PDRIVER_OBJECT drv)
{
	ULONG i;
	LARGE_INTEGER interval;

	// ���Ƚ����
	for(i=0;i<CCP_MAX_COM_ID;i++)
	{
		if(s_nextobj[i] != NULL)
			IoDetachDevice(s_nextobj[i]);
	}

	// ˯��5�롣�ȴ�����irp�������
	interval.QuadPart = (5*1000 * DELAY_ONE_MILLISECOND);		
	KeDelayExecutionThread(KernelMode,FALSE,&interval);

	// ɾ����Щ�豸
	for(i=0;i<CCP_MAX_COM_ID;i++)
	{
		if(s_fltobj[i] != NULL)
			IoDeleteDevice(s_fltobj[i]);
	}
}

NTSTATUS ccpDispatch(PDEVICE_OBJECT device,PIRP irp)
{
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status;
    ULONG i,j;

    // ���ȵ�֪�����͸����ĸ��豸���豸һ�����CCP_MAX_COM_ID
    // ������ǰ��Ĵ��뱣��õģ�����s_fltobj�С�
    for(i=0;i<CCP_MAX_COM_ID;i++)
    {
        if(s_fltobj[i] == device)
        {			
            // ���е�Դ������ȫ��ֱ�ӷŹ���
            if(irpsp->MajorFunction == IRP_MJ_POWER)
            {
                // ֱ�ӷ��ͣ�Ȼ�󷵻�˵�Ѿ��������ˡ�
                PoStartNextPowerIrp(irp);
                IoSkipCurrentIrpStackLocation(irp);
                return PoCallDriver(s_nextobj[i],irp);
            }
            // ��������ֻ����д����д����Ļ�����û������Լ��䳤�ȡ�
            // Ȼ���ӡһ�¡�
            if(irpsp->MajorFunction == IRP_MJ_WRITE)
            {
                // �����д���Ȼ�ó���
                ULONG len = irpsp->Parameters.Write.Length;
                // Ȼ���û�����
                PUCHAR buf = NULL;
                if(irp->MdlAddress != NULL)
                    buf = 
                    (PUCHAR)
                    MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);
                else
                    buf = (PUCHAR)irp->UserBuffer;
                if(buf == NULL)
                    buf = (PUCHAR)irp->AssociatedIrp.SystemBuffer;

                // ��ӡ����
                for(j=0;j<len;++j)
                {
                    DbgPrint("comcap: Send Data: %2x\r\n",
                        buf[j]);
                }
            }

            // ��Щ����ֱ���·�ִ�м��ɡ����ǲ�����ֹ���߸ı�����
            IoSkipCurrentIrpStackLocation(irp);
            return IoCallDriver(s_nextobj[i],irp);
        }
    }

    // ��������Ͳ��ڱ��󶨵��豸�У�����������ģ�ֱ�ӷ��ز�������
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    IoCompleteRequest(irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;	
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    size_t i;
    // ���еķַ����������ó�һ���ġ�
    for(i=0;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
    {
        driver->MajorFunction[i] = ccpDispatch;
    }

    // ֧�ֶ�̬ж�ء�
    driver->DriverUnload = ccpUnload;

    // �����еĴ��ڡ�
    ccpAttachAllComs(driver);

    // ֱ�ӷ��سɹ����ɡ�
    return STATUS_SUCCESS;
}






