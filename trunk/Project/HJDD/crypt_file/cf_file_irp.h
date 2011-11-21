///
/// @file         cf_file_irp.h
/// @author    crazy_chu
/// @date       2009-1-29
/// @brief       ���ļ��Ĳ�����ֱ�ӷ���irp����������
/// 

#ifndef _CF_FILE_IRP_HEADER_
#define _CF_FILE_IRP_HEADER_

// �Է���SetInformation����.
NTSTATUS 
cfFileSetInformation( 
    DEVICE_OBJECT *dev, 
    FILE_OBJECT *file,
    FILE_INFORMATION_CLASS infor_class,
	FILE_OBJECT *set_file,
    void* buf,
    ULONG buf_len);

NTSTATUS
cfFileQueryInformation(
    DEVICE_OBJECT *dev, 
    FILE_OBJECT *file,
    FILE_INFORMATION_CLASS infor_class,
    void* buf,
    ULONG buf_len);

NTSTATUS 
cfFileReadWrite( 
    DEVICE_OBJECT *dev, 
    FILE_OBJECT *file,
    LARGE_INTEGER *offset,
	ULONG *length,
	void *buffer,
	BOOLEAN read_write);

NTSTATUS
cfFileGetStandInfo(
	PDEVICE_OBJECT dev,
	PFILE_OBJECT file,
	PLARGE_INTEGER allocate_size,
	PLARGE_INTEGER file_size,
	BOOLEAN *dir);

NTSTATUS
cfFileSetFileSize(
	DEVICE_OBJECT *dev,
	FILE_OBJECT *file,
	LARGE_INTEGER *file_size);

// ������
void cfFileCacheClear(PFILE_OBJECT pFileObject);

#endif // _CF_FILE_IRP_HEADER_