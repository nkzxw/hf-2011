///
/// @file         cf_sfilter.c
/// @author    crazy_chu
/// @date       2009-1-29
/// @brief      ʵ��sfilter�Ļص������� 
/// 
/// ��������
/// ������Ϊʾ�����롣δ���꾡���ԣ�����֤�ɿ��ԡ����߶�
/// �κ���ʹ�ô˴��뵼�µ�ֱ�Ӻͼ����ʧ�������Ρ�
/// 
/// ��ȨЭ��
/// ����������ڹ���crypt_file.�ǳ�������wowocockΪ��������
/// ������Windows�ں˱������Ϣ��ȫ������д���ļ�͸������
/// ʾ���������̽���֧��WindowsXP�£�FastFat�ļ�ϵͳ�¼���
/// ���ļ��ܡ�δ������ɱ��������������ļ��������������
/// �����������ȫ��Ȩ��Ϊ���߱�������������ѧϰ���Ķ�ʹ
/// �á�δ����λ����������Ȩ������ֱ�Ӹ��ơ����߻��ڴ˴�
/// ������޸ġ����ô˴����ṩ��ȫ�����߲��ּ���������ҵ
/// ��������������������Ļ�����Ϊ������Υ�������߱�����
/// �ߺͻ�ȡ�⳥֮Ȩ�����Ķ��˴��룬���Զ���Ϊ����������
/// ȨЭ�顣�粻���ܴ�Э���ߣ��벻Ҫ�Ķ��˴��롣
///

#include <ntifs.h>
#include "..\inc\sfilter\sfilter.h"
#include "cf_proc.h"
#include "cf_list.h"
#include "cf_modify_irp.h"
#include "cf_create.h"
#include "fat_headers/fat.h"
#include "fat_headers/nodetype.h"
#include "fat_headers/fatstruc.h"

#define CF_FILE_HEADER_SIZE (1024*4)

SF_RET OnSfilterIrpPre(
		IN PDEVICE_OBJECT dev,
		IN PDEVICE_OBJECT next_dev,
		IN PVOID extension,
		IN PIRP irp,
		OUT NTSTATUS *status,
		PVOID *context)
{
    // ��õ�ǰ����ջ
		PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
    PFILE_OBJECT file = irpsp->FileObject;
    // ����ǰ�����Ƿ��Ǽ��ܽ���
    BOOLEAN proc_sec = cfIsCurProcSec();
    BOOLEAN file_sec;

	// �ҽ��������ļ����� FileObject�����ڵ����һ��passthru.
	if(file == NULL)
		return SF_IRP_PASS;


	// ��Ҫ������Щ���������Ǳ�����˵ġ��������ǰpassthru����
	if( irpsp->MajorFunction != IRP_MJ_CREATE &&
		irpsp->MajorFunction != IRP_MJ_CLOSE &&
		irpsp->MajorFunction != IRP_MJ_READ &&
		irpsp->MajorFunction != IRP_MJ_WRITE &&
		irpsp->MajorFunction != IRP_MJ_CLOSE  &&
		irpsp->MajorFunction != IRP_MJ_CLEANUP &&
		irpsp->MajorFunction != IRP_MJ_SET_INFORMATION &&
		irpsp->MajorFunction != IRP_MJ_DIRECTORY_CONTROL &&
		irpsp->MajorFunction != IRP_MJ_QUERY_INFORMATION)
		return SF_IRP_PASS;

    if(!cfListInited())
        return SF_IRP_PASS;


    // �����ļ��򿪣���cfIrpCreatePreͳһ����
    if(irpsp->MajorFunction == IRP_MJ_CREATE)
    {
        if(proc_sec)
            return cfIrpCreatePre(irp,irpsp,file,next_dev);
        else
        {
            // �������������Ϊ��ͨ���̣��������һ�����ڼ�
            // �ܵ��ļ��������������޷��ж�����ļ��Ƿ����ڼ�
            // �ܣ����Է���GO_ON���жϡ�
            return SF_IRP_GO_ON;
        }
    }

    cfListLock();
    file_sec = cfIsFileCrypting(file);
    cfListUnlock();

    // ������Ǽ��ܵ��ļ��Ļ����Ϳ���ֱ��passthru�ˣ�û�б�
    // �������ˡ�
    if(!file_sec)
        return SF_IRP_PASS;

    // �����close�Ϳ���ɾ���ڵ��� 
    if(irpsp->MajorFunction == IRP_MJ_CLOSE)
        return SF_IRP_GO_ON;

 	// ��������ƫ�ơ�������������������⴦������GO_ON
    // ����������set information��������Ҫ����
	// 1.SET FILE_ALLOCATION_INFORMATION
	// 2.SET FILE_END_OF_FILE_INFORMATION
	// 3.SET FILE_VALID_DATA_LENGTH_INFORMATION
    if(irpsp->MajorFunction == IRP_MJ_SET_INFORMATION &&
		(irpsp->Parameters.SetFile.FileInformationClass == FileAllocationInformation ||
		 irpsp->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation ||
		 irpsp->Parameters.SetFile.FileInformationClass == FileValidDataLengthInformation ||
		 irpsp->Parameters.SetFile.FileInformationClass == FileStandardInformation ||
		 irpsp->Parameters.SetFile.FileInformationClass == FileAllInformation ||
		 irpsp->Parameters.SetFile.FileInformationClass == FilePositionInformation))
    {
        // ����Щset information�����޸ģ�ʹ֮��ȥǰ���4k�ļ�ͷ��
        cfIrpSetInforPre(irp,irpsp/*,next_dev,file*/);
		return SF_IRP_PASS;
    }

    if(irpsp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
    {
        // Ҫ����Щread information�Ľ�������޸ġ����Է���go on.
        // ����������cfIrpQueryInforPost(irp,irpsp);
        if(irpsp->Parameters.QueryFile.FileInformationClass == FileAllInformation ||
         irpsp->Parameters.QueryFile.FileInformationClass == FileAllocationInformation ||
		 irpsp->Parameters.QueryFile.FileInformationClass == FileEndOfFileInformation ||
         irpsp->Parameters.QueryFile.FileInformationClass == FileStandardInformation ||
		 irpsp->Parameters.QueryFile.FileInformationClass == FilePositionInformation ||
         irpsp->Parameters.QueryFile.FileInformationClass == FileValidDataLengthInformation)
            return SF_IRP_GO_ON;
        else
        {
            // KdPrint(("OnSfilterIrpPre: %x\r\n",irpsp->Parameters.QueryFile.FileInformationClass));
            return SF_IRP_PASS;
        }
    }

	// ��ʱ������
	//if(irpsp->MajorFunction == IRP_MJ_DIRECTORY_CONTROL)
	//{
	//	// Ҫ����Щread information�Ľ�������޸ġ����Է���go on.
	//	// ����������cfIrpQueryInforPost(irp,irpsp);
	//	if(irpsp->Parameters.QueryDirectory.FileInformationClass == FileDirectoryInformation ||
	//		irpsp->Parameters.QueryDirectory.FileInformationClass == FileFullDirectoryInformation ||
	//		irpsp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation)
	//		return SF_IRP_GO_ON;
	//	else
	//	{
    //           KdPrint(("OnSfilterIrpPre: Query information: %x passthru.\r\n",
    //               irpsp->Parameters.QueryDirectory.FileInformationClass));
	//		return SF_IRP_PASS;
	//	}
	//}

    // ���������read��write�������ֶ�Ҫ�޸���������´���ͬʱ��readҪ�����
    // ������ע�⣺ֻ����ֱ�Ӷ�Ӳ�̵����󡣶Ի����ļ����󲻴���
    if(irpsp->MajorFunction == IRP_MJ_READ &&
       (irp->Flags & (IRP_PAGING_IO|IRP_SYNCHRONOUS_PAGING_IO|IRP_NOCACHE)))
    {
        cfIrpReadPre(irp,irpsp);
        return SF_IRP_GO_ON;
    }
    if(irpsp->MajorFunction == IRP_MJ_WRITE &&
       (irp->Flags & (IRP_PAGING_IO|IRP_SYNCHRONOUS_PAGING_IO|IRP_NOCACHE)))
    {
        if(cfIrpWritePre(irp,irpsp,context))
            return SF_IRP_GO_ON;
        else
        {
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return SF_IRP_COMPLETED;
        }
    }

    // �����κδ���ֱ�ӷ��ء�
    return SF_IRP_PASS;
}

VOID OnSfilterIrpPost(
		IN PDEVICE_OBJECT dev,
		IN PDEVICE_OBJECT next_dev,
		IN PVOID extension,
		IN PIRP irp,
		IN NTSTATUS status,
		PVOID context)
{
    // ��õ�ǰ����ջ
		PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(irp);
    BOOLEAN crypting,sec_proc,need_crypt,need_write_header;
    PFILE_OBJECT file = irpsp->FileObject;
    ULONG desired_access;
    BOOLEAN proc_sec = cfIsCurProcSec();

    // ��ǰ�����Ƿ��Ǽ��ܽ���
    sec_proc = cfIsCurProcSec();

    // ����������ɹ�����û�б�Ҫ����
    if( !NT_SUCCESS(status) &&
        !(irpsp->MajorFunction == IRP_MJ_QUERY_INFORMATION &&
            irpsp->Parameters.QueryFile.FileInformationClass == FileAllInformation &&
            irp->IoStatus.Information > 0) &&
        irpsp->MajorFunction != IRP_MJ_WRITE)
    {
        if(irpsp->MajorFunction == IRP_MJ_READ)
        {
            KdPrint(("OnSfilterIrpPost: IRP_MJ_READ failed. status = %x information = %x\r\n",
                status,irp->IoStatus.Information));
        }
        else if(irpsp->MajorFunction == IRP_MJ_WRITE)
        {
            KdPrint(("OnSfilterIrpPost: IRP_MJ_WRITE failed. status = %x information = %x\r\n",
                status,irp->IoStatus.Information));
        }
        return;
    }

   // �Ƿ���һ���Ѿ������ܽ��̴򿪵��ļ�
    cfListLock();
    // �����create,����Ҫ�ָ��ļ����ȡ����������������pre��
    // ʱ���Ӧ���Ѿ��ָ��ˡ�
    crypting = cfIsFileCrypting(file);
    cfListUnlock();

    // �����е��ļ��򿪣��������µĹ��̲�����
    if(irpsp->MajorFunction == IRP_MJ_CREATE)
    {
        if(proc_sec)
        {
            ASSERT(crypting == FALSE);
            // ����Ǽ��ܽ��̣���׷�ӽ�ȥ���ɡ�
            if(!cfFileCryptAppendLk(file))
            {
                IoCancelFileOpen(next_dev,file);
			    irp->IoStatus.Status = STATUS_ACCESS_DENIED;
			    irp->IoStatus.Information = 0;
                KdPrint(("OnSfilterIrpPost: file %wZ failed to call cfFileCryptAppendLk!!!\r\n",&file->FileName));            
            }
            else
            {
                KdPrint(("OnSfilterIrpPost: file %wZ begin to crypting.\r\n",&file->FileName));
            }
        }
        else
        {
            // ����ͨ���̡������Ƿ��Ǽ����ļ�������Ǽ����ļ���
            // ������������
            if(crypting)
            {
                IoCancelFileOpen(next_dev,file);
			    irp->IoStatus.Status = STATUS_ACCESS_DENIED;
			    irp->IoStatus.Information = 0;
            }
        }
    }
    else if(irpsp->MajorFunction == IRP_MJ_CLOSE)
    {
        // clean up�����ˡ�����ɾ�����ܽڵ㣬ɾ�����塣
        ASSERT(crypting);
        cfCryptFileCleanupComplete(file);
    }
    else if(irpsp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
    {
        ASSERT(crypting);
        cfIrpQueryInforPost(irp,irpsp);
    }
    else if(irpsp->MajorFunction == IRP_MJ_READ)
    {
        ASSERT(crypting);
        cfIrpReadPost(irp,irpsp);
    }
    else if(irpsp->MajorFunction == IRP_MJ_WRITE)
    {
        ASSERT(crypting);
        cfIrpWritePost(irp,irpsp,context);
    }
    else
    {
        ASSERT(FALSE);
    }
}

NTSTATUS OnSfilterDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
	OUT PUNICODE_STRING userNameString,
	OUT PUNICODE_STRING syblnkString,
	OUT PULONG extensionSize)
{
 	UNICODE_STRING user_name,syb_name;
	NTSTATUS status = STATUS_SUCCESS;

#if DBG
//    _asm int 3
#endif

    // ��ʼ����������
    cfListInit();

  	// ȷ�������豸�����ֺͷ������ӡ�
	RtlInitUnicodeString(&user_name,L"crypt_file_cdo");
	RtlInitUnicodeString(&syb_name,L"crypt_file_cdo_syb");
	RtlCopyUnicodeString(userNameString,&user_name);
	RtlCopyUnicodeString(syblnkString,&syb_name);

	// ���ÿ����豸Ϊ�����û�����
	sfilterSetCdoAccessForAll();

    // ��ʼ���������ֲ���
    cfCurProcNameInit();


    return STATUS_SUCCESS;
}

VOID OnSfilterDriverUnload()
{
    // ûʲôҪ����...;
}

NTSTATUS OnSfilterCDODispatch(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN OnSfilterAttachPre(
		IN PDEVICE_OBJECT ourDevice,
		IN PDEVICE_OBJECT theDeviceToAttach,
		IN PUNICODE_STRING DeviceName,
		IN PVOID extension)
{
    // ֱ�ӷ���TRUE�������豸����
    return TRUE;
}

VOID OnSfilterAttachPost(
		IN PDEVICE_OBJECT ourDevice,
		IN PDEVICE_OBJECT theDeviceToAttach,
		IN PDEVICE_OBJECT theDeviceToAttached,
		IN PVOID extension,
		IN NTSTATUS status)
{
    // ����Ҫ��ʲô��
}
