/*
 * ͨ��hash�㷨����key��object�Ŀ���ӳ���ϵ
 * �Ա�ͨ��key���ټ�����object
 */

#ifndef __KEY_OBJECT_H__
#define __KEY_OBJECT_H__

void    free_object_hash_pool(void* pool);
void*   alloc_object_hash_pool(int hash_size, int pool_size);
void*   get_object_hash_pool(void* pool_handle, unsigned long key);
int     add_object_hash_pool(void* pool_handle, unsigned long key, void* object);
int     del_object_hash_pool(void* pool_handle, unsigned long key);

#endif