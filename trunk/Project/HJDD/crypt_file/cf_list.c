///
/// @file         cf_list.c
/// @author    crazy_chu
/// @date       2009-1-29
/// @brief      ʵ��һ�����������������ڼ��ܴ��ŵ��ļ���
///                ��ע��ֻ֧��WindowsXP�µ�FastFat�ļ�ϵͳ��
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
#include "fat_headers/fat.h"
#include "fat_headers/nodetype.h"
#include "fat_headers/fatstruc.h"

#define CF_MEM_TAG 'cfls'
#define CF_FILE_HEADER_SIZE (1024*4)

typedef struct {
    LIST_ENTRY list_entry;
    FCB *fcb;
} CF_NODE,*PCF_NODE;

static LIST_ENTRY s_cf_list;
static KSPIN_LOCK s_cf_list_lock;
static KIRQL s_cf_list_lock_irql;
static BOOLEAN s_cf_list_inited = FALSE;
BOOLEAN cfListInited()
{
    return s_cf_list_inited;
}
 
void cfListLock()
{
    ASSERT(s_cf_list_inited);
    KeAcquireSpinLock(&s_cf_list_lock,&s_cf_list_lock_irql);
}

void cfListUnlock()
{
    ASSERT(s_cf_list_inited);
    KeReleaseSpinLock(&s_cf_list_lock,s_cf_list_lock_irql);
}

void cfListInit()
{
    InitializeListHead(&s_cf_list);
    KeInitializeSpinLock(&s_cf_list_lock);
    s_cf_list_inited = TRUE;
}

// �������һ���ļ����ж��Ƿ��ڼ��������С��������û������
BOOLEAN cfIsFileCrypting(PFILE_OBJECT file)
{
    PLIST_ENTRY p;
    PCF_NODE node;
   for(p = s_cf_list.Flink; p != &s_cf_list; p = p->Flink)
    {
	    node = (PCF_NODE)p;
        if(node->fcb == file->FsContext)
        {
            //KdPrint(("cfIsFileCrypting: file %wZ is crypting. fcb = %x \r\n",&file->FileName,file->FsContext));
            return TRUE;
        }
    } 
    return FALSE;
}

// ׷��һ������ʹ�õĻ����ļ�����������м�������ֻ֤����һ
// ���������ظ����롣
BOOLEAN cfFileCryptAppendLk(PFILE_OBJECT file)
{
    // �ȷ���ռ�
    PCF_NODE node = (PCF_NODE)
        ExAllocatePoolWithTag(NonPagedPool,sizeof(CF_NODE),CF_MEM_TAG);
    node->fcb = (PFCB)file->FsContext;

    cfFileCacheClear(file);

    // ���������ң�����Ѿ����ˣ�����һ�������Ĵ���ֱ�ӱ����ɡ�
    cfListLock();
    if(cfIsFileCrypting(file))
    {
        ASSERT(FALSE);
        return TRUE;
    }
    else if(node->fcb->UncleanCount > 1)
    {
        // Ҫ�ɹ��ļ��룬����Ҫ����һ������������FCB->UncleanCount <= 1.
        // �����Ļ�˵��û�����������������ļ�������Ļ�������һ����
        // ͨ���̴���������ʱ���ܼ��ܡ����ؾܾ��򿪡�
        cfListUnlock();
        // �ͷŵ���
        ExFreePool(node);
        return FALSE;
    }

    // ����Ļ�����������뵽�����
    InsertHeadList(&s_cf_list, (PLIST_ENTRY)node);
    cfListUnlock();

    //cfFileCacheClear(file);
    return TRUE;
}


// �����ļ���clean up��ʱ����ô˺����������鷢��
// FileObject->FsContext���б���
BOOLEAN cfCryptFileCleanupComplete(PFILE_OBJECT file)
{
    PLIST_ENTRY p;
    PCF_NODE node;
    FCB *fcb = (FCB *)file->FsContext;

    KdPrint(("cfCryptFileCleanupComplete: file name = %wZ, fcb->UncleanCount = %d\r\n",
        &file->FileName,fcb->UncleanCount));

    // �����������ļ����塣Ȼ���ٴ��������Ƴ�������Ļ����建
    // ��ʱ��д�����Ͳ�������ˡ�
    if(fcb->UncleanCount <= 1 || (fcb->FcbState & FCB_STATE_DELETE_ON_CLOSE) )
        cfFileCacheClear(file);
    else
        return FALSE;

    cfListLock();
   for(p = s_cf_list.Flink; p != &s_cf_list; p = p->Flink)
   {
	    node = (PCF_NODE)p;
        if(node->fcb == file->FsContext && 
            (node->fcb->UncleanCount == 0 ||
            (fcb->FcbState & FCB_STATE_DELETE_ON_CLOSE)))
        {
            // ���������Ƴ���
            RemoveEntryList((PLIST_ENTRY)node);
            cfListUnlock();
            //  �ͷ��ڴ档
            ExFreePool(node);
            return TRUE;
        }
    } 
    cfListUnlock();
   return FALSE;
}
