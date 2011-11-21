///
/// @file         cf_modify_irp.c
/// @author    crazy_chu wowocock
/// @date       2009-2-1
/// @brief      ʵ�ֶ�irp����ͽ�����޸� 
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
#include "cf_file_irp.h"
#include "cf_list.h"
#include "fat_headers/fat.h"
#include "fat_headers/nodetype.h"
#include "fat_headers/fatstruc.h"

#define CF_FILE_HEADER_SIZE (1024*4)
#define CF_MEM_TAG 'cfmi'

// ����Щset information�����޸ģ�ʹ֮��ȥǰ���4k�ļ�ͷ��
void cfIrpSetInforPre(
    PIRP irp,
    PIO_STACK_LOCATION irpsp)
{
    PUCHAR buffer = irp->AssociatedIrp.SystemBuffer;
    NTSTATUS status;

    ASSERT(irpsp->MajorFunction == IRP_MJ_SET_INFORMATION);
    switch(irpsp->Parameters.SetFile.FileInformationClass)
    {
    case FileAllocationInformation:
        {
		    PFILE_ALLOCATION_INFORMATION alloc_infor = 
                (PFILE_ALLOCATION_INFORMATION)buffer;
		    alloc_infor->AllocationSize.QuadPart += CF_FILE_HEADER_SIZE;        
            break;
        }
    case FileEndOfFileInformation:
        {
		    PFILE_END_OF_FILE_INFORMATION end_infor = 
                (PFILE_END_OF_FILE_INFORMATION)buffer;
		    end_infor->EndOfFile.QuadPart += CF_FILE_HEADER_SIZE;
            break;
        }
    case FileValidDataLengthInformation:
        {
		    PFILE_VALID_DATA_LENGTH_INFORMATION valid_length = 
                (PFILE_VALID_DATA_LENGTH_INFORMATION)buffer;
		    valid_length->ValidDataLength.QuadPart += CF_FILE_HEADER_SIZE;
            break;
        }
	case FilePositionInformation:
		{
			PFILE_POSITION_INFORMATION position_infor = 
				(PFILE_POSITION_INFORMATION)buffer;
			position_infor->CurrentByteOffset.QuadPart += CF_FILE_HEADER_SIZE;
			break;
		}
	case FileStandardInformation:
		((PFILE_STANDARD_INFORMATION)buffer)->EndOfFile.QuadPart += CF_FILE_HEADER_SIZE;
		break;
	case FileAllInformation:
		((PFILE_ALL_INFORMATION)buffer)->PositionInformation.CurrentByteOffset.QuadPart += CF_FILE_HEADER_SIZE;
		((PFILE_ALL_INFORMATION)buffer)->StandardInformation.EndOfFile.QuadPart += CF_FILE_HEADER_SIZE;
		break;

    default:
        ASSERT(FALSE);
    };
}

void cfIrpQueryInforPost(PIRP irp,PIO_STACK_LOCATION irpsp)
{
    PUCHAR buffer = irp->AssociatedIrp.SystemBuffer;
    ASSERT(irpsp->MajorFunction == IRP_MJ_QUERY_INFORMATION);
    switch(irpsp->Parameters.QueryFile.FileInformationClass)
    {
    case FileAllInformation:
        {
            // ע��FileAllInformation���������½ṹ��ɡ���ʹ���Ȳ�����
            // ��Ȼ���Է���ǰ����ֽڡ�
            //typedef struct _FILE_ALL_INFORMATION {
            //    FILE_BASIC_INFORMATION BasicInformation;
            //    FILE_STANDARD_INFORMATION StandardInformation;
            //    FILE_INTERNAL_INFORMATION InternalInformation;
            //    FILE_EA_INFORMATION EaInformation;
            //    FILE_ACCESS_INFORMATION AccessInformation;
            //    FILE_POSITION_INFORMATION PositionInformation;
            //    FILE_MODE_INFORMATION ModeInformation;
            //    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
            //    FILE_NAME_INFORMATION NameInformation;
            //} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;
            // ������Ҫע����Ƿ��ص��ֽ����Ƿ������StandardInformation
            // �������Ӱ���ļ��Ĵ�С����Ϣ��
            PFILE_ALL_INFORMATION all_infor = (PFILE_ALL_INFORMATION)buffer;
            if(irp->IoStatus.Information >= 
                sizeof(FILE_BASIC_INFORMATION) + 
                sizeof(FILE_STANDARD_INFORMATION))
            {
                ASSERT(all_infor->StandardInformation.EndOfFile.QuadPart >= CF_FILE_HEADER_SIZE);
				all_infor->StandardInformation.EndOfFile.QuadPart -= CF_FILE_HEADER_SIZE;
                all_infor->StandardInformation.AllocationSize.QuadPart -= CF_FILE_HEADER_SIZE;
                if(irp->IoStatus.Information >= 
                    sizeof(FILE_BASIC_INFORMATION) + 
                    sizeof(FILE_STANDARD_INFORMATION) +
                    sizeof(FILE_INTERNAL_INFORMATION) +
                    sizeof(FILE_EA_INFORMATION) +
                    sizeof(FILE_ACCESS_INFORMATION) +
                    sizeof(FILE_POSITION_INFORMATION))
                {
                    if(all_infor->PositionInformation.CurrentByteOffset.QuadPart >= CF_FILE_HEADER_SIZE)
                        all_infor->PositionInformation.CurrentByteOffset.QuadPart -= CF_FILE_HEADER_SIZE;
                }
            }
            break;
        }
    case FileAllocationInformation:
        {
		    PFILE_ALLOCATION_INFORMATION alloc_infor = 
                (PFILE_ALLOCATION_INFORMATION)buffer;
            ASSERT(alloc_infor->AllocationSize.QuadPart >= CF_FILE_HEADER_SIZE);
		    alloc_infor->AllocationSize.QuadPart -= CF_FILE_HEADER_SIZE;        
            break;
        }
    case FileValidDataLengthInformation:
        {
		    PFILE_VALID_DATA_LENGTH_INFORMATION valid_length = 
                (PFILE_VALID_DATA_LENGTH_INFORMATION)buffer;
            ASSERT(valid_length->ValidDataLength.QuadPart >= CF_FILE_HEADER_SIZE);
		    valid_length->ValidDataLength.QuadPart -= CF_FILE_HEADER_SIZE;
            break;
        }
    case FileStandardInformation:
        {
            PFILE_STANDARD_INFORMATION stand_infor = (PFILE_STANDARD_INFORMATION)buffer;
            ASSERT(stand_infor->AllocationSize.QuadPart >= CF_FILE_HEADER_SIZE);
            stand_infor->AllocationSize.QuadPart -= CF_FILE_HEADER_SIZE;            
            stand_infor->EndOfFile.QuadPart -= CF_FILE_HEADER_SIZE;
            break;
        }
    case FileEndOfFileInformation:
        {
		    PFILE_END_OF_FILE_INFORMATION end_infor = 
                (PFILE_END_OF_FILE_INFORMATION)buffer;
            ASSERT(end_infor->EndOfFile.QuadPart >= CF_FILE_HEADER_SIZE);
		    end_infor->EndOfFile.QuadPart -= CF_FILE_HEADER_SIZE;
            break;
        }
	case FilePositionInformation:
		{
			PFILE_POSITION_INFORMATION PositionInformation =
				(PFILE_POSITION_INFORMATION)buffer; 
            if(PositionInformation->CurrentByteOffset.QuadPart > CF_FILE_HEADER_SIZE)
			    PositionInformation->CurrentByteOffset.QuadPart -= CF_FILE_HEADER_SIZE;
			break;
		}
    default:
        ASSERT(FALSE);
    };
}

// �����󡣽�ƫ����ǰ�ơ�
void cfIrpReadPre(PIRP irp,PIO_STACK_LOCATION irpsp)
{
    PLARGE_INTEGER offset;
    PFCB fcb = (PFCB)irpsp->FileObject->FsContext;
	offset = &irpsp->Parameters.Read.ByteOffset;
    if(offset->LowPart ==  FILE_USE_FILE_POINTER_POSITION &&  offset->HighPart == -1)
	{
        // ���±�������������������
        ASSERT(FALSE);
	}
    // ƫ�Ʊ����޸�Ϊ����4k��
    offset->QuadPart += CF_FILE_HEADER_SIZE;
    KdPrint(("cfIrpReadPre: offset = %8x\r\n",
        offset->LowPart));
}

// �������������Ҫ���ܡ�
void cfIrpReadPost(PIRP irp,PIO_STACK_LOCATION irpsp)
{
    // �õ���������Ȼ�����֮�����ܼܺ򵥣�����xor 0x77.
    PUCHAR buffer;
    ULONG i,length = irp->IoStatus.Information;
    ASSERT(irp->MdlAddress != NULL || irp->UserBuffer != NULL);
	if(irp->MdlAddress != NULL)
		buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);
	else
		buffer = irp->UserBuffer;

    // ����Ҳ�ܼ򵥣�xor 0x77
    for(i=0;i<length;++i)
        buffer[i] ^= 0X77;
    // ��ӡ����֮�������
    KdPrint(("cfIrpReadPost: flags = %x length = %x content = %c%c%c%c%c\r\n",
        irp->Flags,length,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]));
}

// ����һ��MDL������һ������Ϊlength�Ļ�������
PMDL cfMdlMemoryAlloc(ULONG length)
{
    void *buf = ExAllocatePoolWithTag(NonPagedPool,length,CF_MEM_TAG);
    PMDL mdl;
    if(buf == NULL)
        return NULL;
    mdl = IoAllocateMdl(buf,length,FALSE,FALSE,NULL);
    if(mdl == NULL)
    {
        ExFreePool(buf);
        return NULL;
    }
    MmBuildMdlForNonPagedPool(mdl);
    mdl->Next = NULL;
    return mdl;
}

// �ͷŵ�����MDL�Ļ�������
void cfMdlMemoryFree(PMDL mdl)
{
    void *buffer = MmGetSystemAddressForMdlSafe(mdl,NormalPagePriority);
    IoFreeMdl(mdl);
    ExFreePool(buffer);
}

// д���������ġ���Ϊд�������ָ�ԭ����irp->MdlAddress
// ����irp->UserBuffer�����Բ���Ҫ��¼�����ġ�
typedef struct CF_WRITE_CONTEXT_{
    PMDL mdl_address;
    PVOID user_buffer;
} CF_WRITE_CONTEXT,*PCF_WRITE_CONTEXT;

// д������Ҫ���·��仺�����������п���ʧ�ܡ����ʧ��
// �˾�ֱ�ӱ����ˡ�����Ҫ��һ�����ء�TRUE��ʾ�ɹ�����
// �Լ���GO_ON��FALSE��ʾʧ���ˣ������Ѿ���ã�ֱ��
// ��ɼ���
BOOLEAN cfIrpWritePre(PIRP irp,PIO_STACK_LOCATION irpsp, PVOID *context)
{
    PLARGE_INTEGER offset;
    ULONG i,length = irpsp->Parameters.Write.Length;
    PUCHAR buffer,new_buffer;
    PMDL new_mdl = NULL;

    // ��׼��һ��������
    PCF_WRITE_CONTEXT my_context = (PCF_WRITE_CONTEXT)
        ExAllocatePoolWithTag(NonPagedPool,sizeof(CF_WRITE_CONTEXT),CF_MEM_TAG);
    if(my_context == NULL)
    {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        return FALSE;
    }
  
    // ������õ�������м��ܡ�Ҫע�����д����Ļ�����
    // �ǲ�����ֱ�Ӹ�д�ġ��������·��䡣
    ASSERT(irp->MdlAddress != NULL || irp->UserBuffer != NULL);
	if(irp->MdlAddress != NULL)
    {
		buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);
        new_mdl = cfMdlMemoryAlloc(length);
        if(new_mdl == NULL)
            new_buffer = NULL;
        else
            new_buffer = MmGetSystemAddressForMdlSafe(new_mdl,NormalPagePriority);
    }
	else
    {
		buffer = irp->UserBuffer;
        new_buffer = ExAllocatePoolWithTag(NonPagedPool,length,CF_MEM_TAG);
    }
    // �������������ʧ���ˣ�ֱ���˳����ɡ�
    if(new_buffer == NULL)
    {
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        ExFreePool(my_context);
        return FALSE;
    }
    RtlCopyMemory(new_buffer,buffer,length);

    // ��������һ���ɹ������������������ˡ�
    my_context->mdl_address = irp->MdlAddress;
    my_context->user_buffer = irp->UserBuffer;
    *context = (void *)my_context;

    // ��irpָ���е�mdl�������֮���ٻָ�������
    if(new_mdl == NULL)
        irp->UserBuffer = new_buffer;
    else
        irp->MdlAddress = new_mdl;

	offset = &irpsp->Parameters.Write.ByteOffset;
    KdPrint(("cfIrpWritePre: fileobj = %x flags = %x offset = %8x length = %x content = %c%c%c%c%c\r\n",
        irpsp->FileObject,irp->Flags,offset->LowPart,length,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]));

    // ����Ҳ�ܼ򵥣�xor 0x77
    for(i=0;i<length;++i)
        new_buffer[i] ^= 0x77;

    if(offset->LowPart ==  FILE_USE_FILE_POINTER_POSITION &&  offset->HighPart == -1)
	{
        // ���±�������������������
        ASSERT(FALSE);
	}
    // ƫ�Ʊ����޸�Ϊ����4KB��
    offset->QuadPart += CF_FILE_HEADER_SIZE;
    return TRUE;
}

// ��ע�����۽����Σ����������WritePost.���������޷��ָ�
// Write�����ݣ��ͷ��ѷ���Ŀռ�������
void cfIrpWritePost(PIRP irp,PIO_STACK_LOCATION irpsp,void *context)
{
    PCF_WRITE_CONTEXT my_context = (PCF_WRITE_CONTEXT) context;
    // ����������Իָ�irp�������ˡ�
    if(irp->MdlAddress != NULL)
        cfMdlMemoryFree(irp->MdlAddress);
    if(irp->UserBuffer != NULL)
        ExFreePool(irp->UserBuffer);
    irp->MdlAddress = my_context->mdl_address;
    irp->UserBuffer = my_context->user_buffer;
    ExFreePool(my_context);
}
