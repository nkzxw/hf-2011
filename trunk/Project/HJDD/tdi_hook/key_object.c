#include "key_object.h"

#include <ntddk.h>
#define tsock_alloc_mm(size) ExAllocatePoolWithTag(NonPagedPool, (size), 'kobj')
#define tsock_free_mm(p) ExFreePool(p)

struct object_hash_item{
        struct object_hash_item* next;          /* 单链表 */
        struct object_hash_item** entry;       /* hash表入口头 */
        unsigned long key;
        void*   object;
};

struct object_hash_pool{
        int hash_size;
        struct object_hash_item** hash_table;   /* hash表 */
        int pool_size;                          /* object数量 */
        struct object_hash_item* object_pool;   /* object池 */
        int pool_stick;                         /* object轮转分配位置 */
};

/*
 * 初始化一个hash_pool, pool_size <=0 表明不使用object池，不限制object数量
 */
void* alloc_object_hash_pool(int hash_size, int pool_size)
{
        struct object_hash_pool* pool;
        if (hash_size <= 0)
                return NULL;

        pool = tsock_alloc_mm(sizeof(struct object_hash_pool));
        if (!pool)
                return NULL;

        /* 初始化数据结构 */
        memset(pool, 0, sizeof(struct object_hash_pool));
        
        /* 动态分配hash表 */
        pool->hash_size = hash_size;
        pool->hash_table = tsock_alloc_mm(hash_size * sizeof(struct object_hash_item*));
        if (!pool->hash_table){
                free_object_hash_pool(pool);
                return NULL;
        }

        memset(pool->hash_table, 0, hash_size * sizeof(struct object_hash_item*));
        
        /* 预分配object池 */        
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
 * 释放object_hash_pool
 */
void free_object_hash_pool(void* pool_handle)
{
        int i;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_head;
        struct object_hash_item* hash_item;
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;

        if (pool){
                /* 释放object */
                if (pool->object_pool){
                        tsock_free_mm(pool->object_pool);
                } else if (pool->hash_table) {
                        /* 遍历hash表，依次释放object */
                        for (i = 0; i < pool->hash_size; i++){
                                hash_head = pool->hash_table[i];
                                for (enum_item = hash_head; enum_item;){
                                        hash_item = enum_item;
                                        enum_item = hash_item->next;
                                        tsock_free_mm(hash_item);
                                }
                        }
                }
                
                /* 释放hash表 */
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
        
        /* 计算待插入的hash链表头 */
        hash_index = key % pool->hash_size;
        hash_head = pool->hash_table[hash_index];
        
        /* head->自己*/
        for (enum_item = hash_head; enum_item; enum_item = enum_item->next){
                if (enum_item->key == key)
                        return enum_item;
        }
        
        return NULL;
}

/*
 * 插入一个key到hash表,key不允许重复
 */
int add_object_hash_pool(void* pool_handle, unsigned long key, void* object)
{
        int hash_index;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_item;
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;

        if (!pool)
                return -1;

        /* 计算待插入的hash链表头 */
        hash_index = key % pool->hash_size;

        /* 检查是否key冲突 */
        if (key_to_hash_object(pool, key))
                return -1;

        /* 申请一个object */
        if (pool->object_pool){
                pool->pool_stick++;
                pool->pool_stick = pool->pool_stick % pool->pool_size;
                hash_item = pool->object_pool + pool->pool_stick;
        } else {
                hash_item = tsock_alloc_mm(sizeof(struct object_hash_item));
                memset(hash_item, 0, sizeof(struct object_hash_item));
        }

        /* 从原来的hash队列中删除 */
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
                /* 挂入队列尾 */
                for (enum_item = *hash_item->entry; enum_item->next; enum_item = enum_item->next);
                
                enum_item->next = hash_item;        
        } else {
                *hash_item->entry = hash_item;
        }

        return 0;
}

/*
 * 通过key快速检索到一个object
 */
void* get_object_hash_pool(void* pool_hanle, unsigned long key)
{
        struct object_hash_item* hash_item = key_to_hash_object((struct object_hash_pool*)pool_hanle, key);
        return hash_item ? hash_item->object : NULL;
}

/*
 * 删除指定key对应的object
 */
int del_object_hash_pool(void* pool_handle, unsigned long key)
{
        struct object_hash_pool* pool = (struct object_hash_pool*)pool_handle;
        struct object_hash_item* enum_item;
        struct object_hash_item* hash_item = key_to_hash_object(pool, key);

        /* 从链表中删除该object */
        for (enum_item = *(hash_item->entry); enum_item && enum_item->next != hash_item;)
                enum_item = enum_item->next;

        /* 删除该节点 - enum为hash_item上一个节点 */
        if (enum_item && enum_item->next == hash_item)
                enum_item->next = hash_item->next;
        else if(!enum_item){
                /* 如果没有上一节点说明当前节点位于链表头 */
                *(hash_item->entry) = hash_item->next;
        }

        if (!pool->object_pool)
                tsock_free_mm(hash_item);

        return 0;
}
