#include "key_object.h"

#include <ntddk.h>
#define tsock_alloc_mm(size) ExAllocatePoolWithTag(NonPagedPool, (size), 'kobj')
#define tsock_free_mm(p) ExFreePool(p)

struct object_hash_item{
        struct object_hash_item* next;          /* ������ */
        struct object_hash_item** entry;       /* hash�����ͷ */
        unsigned long key;
        void*   object;
};

struct object_hash_pool{
        int hash_size;
        struct object_hash_item** hash_table;   /* hash�� */
        int pool_size;                          /* object���� */
        struct object_hash_item* object_pool;   /* object�� */
        int pool_stick;                         /* object��ת����λ�� */
};

/*
 * ��ʼ��һ��hash_pool, pool_size <=0 ������ʹ��object�أ�������object����
 */
void* alloc_object_hash_pool(int hash_size, int pool_size)
{
        struct object_hash_pool* pool;
        if (hash_size <= 0)
                return NULL;

        pool = tsock_alloc_mm(sizeof(struct object_hash_pool));
        if (!pool)
                return NULL;

        /* ��ʼ�����ݽṹ */
        memset(pool, 0, sizeof(struct object_hash_pool));
        
        /* ��̬����hash�� */
        pool->hash_size = hash_size;
        pool->hash_table = tsock_alloc_mm(hash_size * sizeof(struct object_hash_item*));
        if (!pool->hash_table){
                free_object_hash_pool(pool);
                return NULL;
        }

        memset(pool->hash_table, 0, hash_size * sizeof(struct object_hash_item*));
        
        /* Ԥ����object�� */        
        if (pool_size > 0){
                pool->pool_size = pool_size;
                pool->object_pool = tsock_alloc_mm(pool_size * sizeof(struct object_hash_item));
                if (!pool->object_pool){
                        free_object_hash_pool(pool);
                        return NULL;
                }
                memset(pool->object_pool, 0, pool_size * sizeof(struct object_hash_item));
        }

        return pool;
}

/*
 * �ͷ�object_hash_pool
 */
void free_object_hash_pool(void* pool_handle)
{
        int i;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_head;
        struct object_hash_item* hash_item;
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;

        if (pool){
                /* �ͷ�object */
                if (pool->object_pool){
                        tsock_free_mm(pool->object_pool);
                } else if (pool->hash_table) {
                        /* ����hash�������ͷ�object */
                        for (i = 0; i < pool->hash_size; i++){
                                hash_head = pool->hash_table[i];
                                for (enum_item = hash_head; enum_item;){
                                        hash_item = enum_item;
                                        enum_item = hash_item->next;
                                        tsock_free_mm(hash_item);
                                }
                        }
                }
                
                /* �ͷ�hash�� */
                if (pool->hash_table)
                        tsock_free_mm(pool->hash_table);

                memset(pool, 0, sizeof(struct object_hash_pool));
                tsock_free_mm(pool);

        }
}

static struct object_hash_item* key_to_hash_object(struct object_hash_pool* pool, unsigned long key)
{
        int hash_index;
        struct object_hash_item* hash_head;
        struct object_hash_item* enum_item;
        
        if (!pool)
                return NULL;
        
        /* ����������hash����ͷ */
        hash_index = key % pool->hash_size;
        hash_head = pool->hash_table[hash_index];
        
        /* head->�Լ�*/
        for (enum_item = hash_head; enum_item; enum_item = enum_item->next){
                if (enum_item->key == key)
                        return enum_item;
        }
        
        return NULL;
}

/*
 * ����һ��key��hash��,key�������ظ�
 */
int add_object_hash_pool(void* pool_handle, unsigned long key, void* object)
{
        int hash_index;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_item;
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;

        if (!pool)
                return -1;

        /* ����������hash����ͷ */
        hash_index = key % pool->hash_size;

        /* ����Ƿ�key��ͻ */
        if (key_to_hash_object(pool, key))
                return -1;

        /* ����һ��object */
        if (pool->object_pool){
                pool->pool_stick++;
                pool->pool_stick = pool->pool_stick % pool->pool_size;
                hash_item = pool->object_pool + pool->pool_stick;
        } else {
                hash_item = tsock_alloc_mm(sizeof(struct object_hash_item));
                memset(hash_item, 0, sizeof(struct object_hash_item));
        }

        /* ��ԭ����hash������ɾ�� */
        if (hash_item->entry && *hash_item->entry){
                if (*hash_item->entry == hash_item){
                        *hash_item->entry = hash_item->next;
                } else {
                        for (enum_item = *hash_item->entry; enum_item; enum_item = enum_item->next){
                                if (enum_item->next == hash_item){
                                        enum_item->next = hash_item->next;
                                        break;
                                }
                        }
                }
        }
        
        hash_item->entry = pool->hash_table + hash_index;
        hash_item->next = NULL;
        hash_item->key = key;
        hash_item->object = object;
        
        if (*hash_item->entry){
                /* �������β */
                for (enum_item = *hash_item->entry; enum_item->next; enum_item = enum_item->next);
                
                enum_item->next = hash_item;        
        } else {
                *hash_item->entry = hash_item;
        }

        return 0;
}

/*
 * ͨ��key���ټ�����һ��object
 */
void* get_object_hash_pool(void* pool_hanle, unsigned long key)
{
        struct object_hash_item* hash_item = key_to_hash_object((struct object_hash_pool*)pool_hanle, key);
        return hash_item ? hash_item->object : NULL;
}

/*
 * ɾ��ָ��key��Ӧ��object
 */
int del_object_hash_pool(void* pool_handle, unsigned long key)
{
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_item = key_to_hash_object(pool, key);

        /* ��������ɾ����object */
        for (enum_item = *(hash_item->entry); enum_item && enum_item->next != hash_item;)
                enum_item = enum_item->next;

        /* ɾ���ýڵ� - enumΪhash_item��һ���ڵ� */
        if (enum_item && enum_item->next == hash_item)
                enum_item->next = hash_item->next;
        else if(!enum_item){
                /* ���û����һ�ڵ�˵����ǰ�ڵ�λ������ͷ */
                *(hash_item->entry) = hash_item->next;
        }

        if (!pool->object_pool)
                tsock_free_mm(hash_item);

        return 0;
}
