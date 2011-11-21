///
/// @file         sflt_smpl.c
/// @author    crazy_chu
/// @date       2009-2-20
/// @brief      ʹ��sfilter.lib��һ�����ӡ� 
/// 
/// ��������
/// ������Ϊʾ�����롣δ���꾡���ԣ�����֤�ɿ��ԡ����߶�
/// �κ���ʹ�ô˴��뵼�µ�ֱ�Ӻͼ����ʧ�������Ρ�
/// 

#include <ntifs.h>
#include "..\inc\sfilter\sfilter.h"

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

	// �ҽ��������ļ����� FileObject�����ڵ����һ��passthru.
	if(file == NULL)
		return SF_IRP_PASS;

    KdPrint(("IRP: file name = %wZ major function = %x\r\n",&file->FileName));
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
    // ʲô��������
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

  	// ȷ�������豸�����ֺͷ������ӡ�
	RtlInitUnicodeString(&user_name,L"sflt_smpl_cdo");
	RtlInitUnicodeString(&syb_name,L"sflt_smpl_cdo_syb");
	RtlCopyUnicodeString(userNameString,&user_name);
	RtlCopyUnicodeString(syblnkString,&syb_name);

	// ���ÿ����豸Ϊ�����û�����
	sfilterSetCdoAccessForAll();

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
