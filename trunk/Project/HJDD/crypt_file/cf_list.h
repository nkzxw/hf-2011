///
/// @file         cf_list.h
/// @author    crazy_chu
/// @date       2009-1-29
///

#ifndef _CF_LIST_HEADER_
#define _CF_LIST_HEADER_

void cfListInit();
BOOLEAN cfListInited();
void cfListLock();
void cfListUnlock();
// �������һ���ļ����ж��Ƿ��ڼ��������С�
BOOLEAN cfIsFileCrypting(PFILE_OBJECT file);
BOOLEAN cfFileCryptAppendLk(PFILE_OBJECT file);
BOOLEAN cfIsFileNeedCrypt(
    PFILE_OBJECT file,
    PDEVICE_OBJECT next_dev,
    ULONG desired_access,
    BOOLEAN *need_write_header);
// �����ļ���clean up��ʱ����ô˺����������鷢��
// FileObject->FsContext���б���
BOOLEAN cfCryptFileCleanupComplete(PFILE_OBJECT file);
NTSTATUS cfWriteAHeader(PFILE_OBJECT file,PDEVICE_OBJECT next_dev);

#endif // _CF_LIST_HEADER_