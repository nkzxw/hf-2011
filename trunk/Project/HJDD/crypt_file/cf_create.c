///
/// @file         cf_create.c
/// @author    crazy_chu
/// @date       2009-2-4
/// @brief      ʵ�ֶ�create irp�Ĵ��� 
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
#include "cf_list.h"
#include "cf_file_irp.h"

#define CF_FILE_HEADER_SIZE (1024*4)
#define CF_MEM_TAG 'cfct'

// ��create֮ǰ��ʱ�򣬻��������·����
ULONG
cfFileFullPathPreCreate(
						PFILE_OBJECT file,
                        PUNICODE_STRING path
						)
{
	NTSTATUS status;
	POBJECT_NAME_INFORMATION  obj_name_info = NULL;
	WCHAR buf[64] = { 0 };
	void *obj_ptr;
	ULONG length = 0;
	BOOLEAN need_split = FALSE;

	ASSERT( file != NULL );
	if(file == NULL)
		return 0;
	if(file->FileName.Buffer == NULL)
		return 0;

	obj_name_info = (POBJECT_NAME_INFORMATION)buf;
	do {

		// ��ȡFileNameǰ��Ĳ��֣��豸·�����߸�Ŀ¼·����
		if(file->RelatedFileObject != NULL)
			obj_ptr = (void *)file->RelatedFileObject;
		else
			obj_ptr= (void *)file->DeviceObject;
		status = ObQueryNameString(obj_ptr,obj_name_info,64*sizeof(WCHAR),&length);
		if(status == STATUS_INFO_LENGTH_MISMATCH)
		{
			obj_name_info = ExAllocatePoolWithTag(NonPagedPool,length,CF_MEM_TAG);
			if(obj_name_info == NULL)
				return STATUS_INSUFFICIENT_RESOURCES;
			RtlZeroMemory(obj_name_info,length);
			status = ObQueryNameString(obj_ptr,obj_name_info,length,&length);            
		}
		// ʧ���˾�ֱ����������
		if(!NT_SUCCESS(status))
			break;

		// �ж϶���֮���Ƿ���Ҫ��һ��б�ܡ�����Ҫ��������:
		// FileName��һ���ַ�����б�ܡ�obj_name_info���һ��
		// �ַ�����б�ܡ�
		if( file->FileName.Length > 2 &&
			file->FileName.Buffer[ 0 ] != L'\\' &&
			obj_name_info->Name.Buffer[ obj_name_info->Name.Length / sizeof(WCHAR) - 1 ] != L'\\' )
			need_split = TRUE;

		// ���������ֵĳ��ȡ�������Ȳ��㣬Ҳֱ�ӷ��ء�
		length = obj_name_info->Name.Length + file->FileName.Length;
		if(need_split)
			length += sizeof(WCHAR);
		if(path->MaximumLength < length)
			break;

		// �Ȱ��豸��������ȥ��
		RtlCopyUnicodeString(path,&obj_name_info->Name);
		if(need_split)
			// ׷��һ��б��
			RtlAppendUnicodeToString(path,L"\\");

		// Ȼ��׷��FileName
		RtlAppendUnicodeStringToString(path,&file->FileName);
	} while(0);

	// ���������ռ���ͷŵ���
	if((void *)obj_name_info != (void *)buf)
		ExFreePool(obj_name_info);
	return length;
}

// ��IoCreateFileSpecifyDeviceObjectHint�����ļ���
// ����ļ���֮�󲻽�������������Կ���ֱ��
// Read��Write,���ᱻ���ܡ�
HANDLE cfCreateFileAccordingIrp(
   IN PDEVICE_OBJECT dev,
   IN PUNICODE_STRING file_full_path,
   IN PIO_STACK_LOCATION irpsp,
   OUT NTSTATUS *status,
   OUT PFILE_OBJECT *file,
   OUT PULONG information)
{
	HANDLE file_h = NULL;
	IO_STATUS_BLOCK io_status;
	ULONG desired_access;
	ULONG disposition;
	ULONG create_options;
	ULONG share_access;
	ULONG file_attri;
    OBJECT_ATTRIBUTES obj_attri;

    ASSERT(irpsp->MajorFunction == IRP_MJ_CREATE);

    *information = 0;

    // ��дobject attribute
    InitializeObjectAttributes(
        &obj_attri,
        file_full_path,
        OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    // ���IRP�еĲ�����
	desired_access = irpsp->Parameters.Create.SecurityContext->DesiredAccess;
	disposition = (irpsp->Parameters.Create.Options>>24);
	create_options = (irpsp->Parameters.Create.Options & 0x00ffffff);
	share_access = irpsp->Parameters.Create.ShareAccess;
	file_attri = irpsp->Parameters.Create.FileAttributes;

    // ����IoCreateFileSpecifyDeviceObjectHint���ļ���
    *status = IoCreateFileSpecifyDeviceObjectHint(
        &file_h,
        desired_access,
        &obj_attri,
        &io_status,
        NULL,
        file_attri,
        share_access,
        disposition,
        create_options,
        NULL,
        0,
        CreateFileTypeNone,
        NULL,
        0,
        dev);

    if(!NT_SUCCESS(*status))
        return file_h;

    // ��סinformation,��������ʹ�á�
    *information = io_status.Information;

    // �Ӿ���õ�һ��fileobject���ں���Ĳ������ǵ�һ��Ҫ���
    // ���á�
    *status = ObReferenceObjectByHandle(
        file_h,
        0,
        *IoFileObjectType,
        KernelMode,
        file,
        NULL);

    // ���ʧ���˾͹رգ�����û���ļ����������ʵ�����ǲ�
    // Ӧ�ó��ֵġ�
    if(!NT_SUCCESS(*status))
    {
        ASSERT(FALSE);
        ZwClose(file_h);
    }
    return file_h;
}

// д��һ���ļ�ͷ��
NTSTATUS cfWriteAHeader(PFILE_OBJECT file,PDEVICE_OBJECT next_dev)
{
    static WCHAR header_flags[CF_FILE_HEADER_SIZE/sizeof(WCHAR)] = {L'C',L'F',L'H',L'D'};
    LARGE_INTEGER file_size,offset;
    ULONG length = CF_FILE_HEADER_SIZE;
    NTSTATUS status;

    offset.QuadPart = 0;
    file_size.QuadPart = CF_FILE_HEADER_SIZE;
    // ���������ļ��Ĵ�СΪ4k��
    status = cfFileSetFileSize(next_dev,file,&file_size);
    if(status != STATUS_SUCCESS)
        return status;

    // Ȼ��д��8���ֽڵ�ͷ��
   return cfFileReadWrite(next_dev,file,&offset,&length,header_flags,FALSE);
}


// ��Ԥ����
ULONG cfIrpCreatePre(
    PIRP irp,
    PIO_STACK_LOCATION irpsp,
    PFILE_OBJECT file,
    PDEVICE_OBJECT next_dev)
{
    UNICODE_STRING path = { 0 };
    // ���Ȼ��Ҫ���ļ���·����
    ULONG length = cfFileFullPathPreCreate(file,&path);
    NTSTATUS status;
    ULONG ret = SF_IRP_PASS;
    PFILE_OBJECT my_file = NULL;
    HANDLE file_h;
    ULONG information = 0;
    LARGE_INTEGER file_size,offset = { 0 };
    BOOLEAN dir,sec_file;
    // ��ô򿪷���������
	ULONG desired_access = irpsp->Parameters.Create.SecurityContext->DesiredAccess;
    WCHAR header_flags[4] = {L'C',L'F',L'H',L'D'};
    WCHAR header_buf[4] = { 0 };
    ULONG disp;

    // �޷��õ�·����ֱ�ӷŹ����ɡ�
    if(length == 0)
        return SF_IRP_PASS;

    // ���ֻ�����Ŀ¼�Ļ���ֱ�ӷŹ�
    if(irpsp->Parameters.Create.Options & FILE_DIRECTORY_FILE)
        return SF_IRP_PASS;

    do {

        // ��path���仺����
        path.Buffer = ExAllocatePoolWithTag(NonPagedPool,length+4,CF_MEM_TAG);
        path.Length = 0;
        path.MaximumLength = (USHORT)length + 4;
        if(path.Buffer == NULL)
        {
            // �ڴ治�����������ֱ�ӹҵ�
            status = STATUS_INSUFFICIENT_RESOURCES;
            ret = SF_IRP_COMPLETED;
            break;
        }
        length = cfFileFullPathPreCreate(file,&path);

        // �õ���·����������ļ���
        file_h = cfCreateFileAccordingIrp(
            next_dev,
            &path,
            irpsp,
            &status,
            &my_file,
            &information);

        // ���û�гɹ��Ĵ򿪣���ô˵�����������Խ�����
        if(!NT_SUCCESS(status))
        {
            ret = SF_IRP_COMPLETED;
            break;
        }

        // �õ���my_file֮�������ж�����ļ��ǲ����Ѿ���
        // ���ܵ��ļ�֮�С�����ڣ�ֱ�ӷ���passthru����
        cfListLock();
        sec_file = cfIsFileCrypting(my_file);
        cfListUnlock();
        if(sec_file)
        {
            ret = SF_IRP_PASS;
            break;
        }

        // ������Ȼ�򿪣���������Ȼ������һ��Ŀ¼��������
        // �ж�һ�¡�ͬʱҲ���Եõ��ļ��Ĵ�С��
        status = cfFileGetStandInfo(
	        next_dev,
	        my_file,
	        NULL,
	        &file_size,
	        &dir);

        // ��ѯʧ�ܡ���ֹ�򿪡�
        if(!NT_SUCCESS(status))
        {
            ret = SF_IRP_COMPLETED;
            break;
        }

        // �������һ��Ŀ¼����ô�������ˡ�
        if(dir)
        {
            ret = SF_IRP_PASS;
            break;
        }

        // ����ļ���СΪ0������д�����׷�����ݵ���ͼ��
        // ��Ӧ�ü����ļ���Ӧ��������д���ļ�ͷ����Ҳ��Ψ
        // һ��Ҫд���ļ�ͷ�ĵط���
        if(file_size.QuadPart == 0 && 
            (desired_access & 
                (FILE_WRITE_DATA| 
		        FILE_APPEND_DATA)))
        {
            // �����Ƿ�ɹ���һ��Ҫд��ͷ��
            cfWriteAHeader(my_file,next_dev);
            // д��ͷ֮������ļ����ڱ�����ܵ��ļ�
            ret = SF_IRP_GO_ON;
            break;
        }

        // ����ļ��д�С�����Ҵ�СС��ͷ���ȡ�����Ҫ���ܡ�
        if(file_size.QuadPart < CF_FILE_HEADER_SIZE)
        {
            ret = SF_IRP_PASS;
            break;
        }

        // ���ڶ�ȡ�ļ����Ƚ������Ƿ���Ҫ���ܣ�ֱ�Ӷ���8��
        // �ھ��㹻�ˡ�����ļ��д�С�����ұ�CF_FILE_HEADER_SIZE
        // ������ʱ����ǰ8���ֽڣ��ж��Ƿ�Ҫ���ܡ�
        length = 8;
        status = cfFileReadWrite(next_dev,my_file,&offset,&length,header_buf,TRUE);
        if(status != STATUS_SUCCESS)
        {
            // ���ʧ���˾Ͳ������ˡ�
            ASSERT(FALSE);
            ret = SF_IRP_PASS;
            break;
        }
        // ��ȡ�����ݣ��ȽϺͼ��ܱ�־��һ�µģ����ܡ�
        if(RtlCompareMemory(header_flags,header_buf,8) == 8)
        {
            // ��������Ϊ�Ǳ�����ܵġ���������£����뷵��GO_ON.
            ret = SF_IRP_GO_ON;
            break;
        }

        // ������������ǲ���Ҫ���ܵġ�
        ret = SF_IRP_PASS;
    } while(0);

    if(path.Buffer != NULL)
        ExFreePool(path.Buffer);    
    if(file_h != NULL)
        ZwClose(file_h);
    if(ret == SF_IRP_GO_ON)
    {
        // Ҫ���ܵģ�������һ�»��塣�����ļ�ͷ�����ڻ����
        cfFileCacheClear(my_file);
    }
    if(my_file != NULL)
        ObDereferenceObject(my_file);

    // ���Ҫ������ɣ����������������ɡ���һ�㶼��
    // �Դ�����Ϊ��ֵġ�
    if(ret == SF_IRP_COMPLETED)
    {
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = information;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    // Ҫע��:
    // 1.�ļ���CREATE��ΪOPEN.
    // 2.�ļ���OVERWRITEȥ���������ǲ���Ҫ���ܵ��ļ���
    // ������������������Ļ�����������ͼ�����ļ��ģ�
    // ��������ļ��Ѿ������ˡ�������ͼ�����ļ��ģ���
    // ����һ�λ�ȥ������ͷ��
    disp = FILE_OPEN;
    irpsp->Parameters.Create.Options &= 0x00ffffff;
    irpsp->Parameters.Create.Options |= (disp << 24);
    return ret;
}
