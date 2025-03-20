#ifndef __CC_UTIL_CC_HASH_H__
#define __CC_UTIL_CC_HASH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc_util/cc_buffer.h"

#define CC_HASH_OK (0)
#define CC_HASH_ERR (-1)
#define CC_HASH_NOT_FOUND (1)

#define CC_GROW_FACTOR (2)
#define CC_GROW_LOAD_FACTOR (75)
#define CC_MIN_BUCKET_SIZE (4)

typedef struct _cc_hash_node_s {
  cc_string_t key;
  struct _cc_hash_node_s *next;
} cc_hash_node_t;

typedef unsigned int cc_hash_func_t(const char *key, unsigned int key_size, size_t bucket_size);

typedef int cc_compare_func_t(const char *key, unsigned int key_size, const char *other_key,
                              unsigned int other_key_size);

typedef void cc_hash_destroy_node_func_t(cc_hash_node_t *node);

typedef struct _cc_hash_table_s {
  cc_hash_node_t **buckets;
  cc_hash_func_t *hash_func;
  cc_compare_func_t *compare_func;
  cc_hash_destroy_node_func_t *destroy_node_func;
  size_t node_size;
  size_t bucket_size;
} cc_hash_table_t;

typedef struct _cc_dynamic_hash_table_s {
  cc_hash_table_t *small_table;
  cc_hash_table_t *large_table;
  unsigned int transfer_index;
} cc_dynamic_hash_table_t;

// 创建哈希表
cc_hash_table_t *cc_hash_table_create(size_t bucket_size, cc_hash_func_t *hash_func, cc_compare_func_t *compare_func,
                                      cc_hash_destroy_node_func_t *destroy_node_func);

// 插入节点
void cc_hash_table_insert(cc_hash_table_t *table, cc_hash_node_t *node);

// 查找节点
int cc_hash_table_lookup(cc_hash_table_t *table, const char *key, unsigned int key_size, cc_hash_node_t **node);

// 删除节点
void cc_hash_table_delete(cc_hash_table_t *table, const char *key, unsigned int key_size);

// 释放哈希表
void cc_hash_table_free(cc_hash_table_t *table);

// 创建动态哈希表
cc_dynamic_hash_table_t *cc_dynamic_hash_table_create(size_t bucket_size, cc_hash_func_t *hash_func,
                                                      cc_compare_func_t *compare_func,
                                                      cc_hash_destroy_node_func_t *destroy_node_func);

// 插入节点
void cc_dynamic_hash_table_insert(cc_dynamic_hash_table_t *table, cc_hash_node_t *node);

// 查找节点
int cc_dynamic_hash_table_lookup(cc_dynamic_hash_table_t *table, const char *key, unsigned int key_size,
                                 cc_hash_node_t **node);

// 删除节点
void cc_dynamic_hash_table_delete(cc_dynamic_hash_table_t *table, const char *key, unsigned int key_size);

// 释放动态哈希表
void cc_dynamic_hash_table_free(cc_dynamic_hash_table_t *table);

#endif
