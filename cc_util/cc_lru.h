#ifndef __CC_UTIL_CC_LRU_H__
#define __CC_UTIL_CC_LRU_H__

#include "cc_util/cc_hash.h"
#include "cc_util/cc_list.h"
#include "cc_util/cc_util.h"

typedef struct _cc_lru_cache_node_s {
  cc_hash_node_t hash_node;
  cc_dlist_node_t list_node;
} cc_lru_cache_node_t;

typedef struct _cc_lru_cache_s {
  cc_dlist_node_t list_head;
  cc_hash_table_t *hash_table;
  size_t capacity;
  size_t bucket_size;
  size_t size;
} cc_lru_cache_t;

#define cc_list_node_to_cache_node(node) \
  (cc_lru_cache_node_t *)((unsigned char *)(node) - offsetof(cc_lru_cache_node_t, list_node))
#define cc_hash_node_to_cache_node(node) \
  (cc_lru_cache_node_t *)((unsigned char *)(node) - offsetof(cc_lru_cache_node_t, hash_node))

cc_lru_cache_t *cc_lru_cache_create(size_t capacity, cc_hash_func_t *hash_func,
                                    cc_hash_destroy_node_func_t *destroy_node_func);

int cc_lru_cache_lookup(cc_lru_cache_t *cache, const char *key, cc_lru_cache_node_t **node);

void cc_lru_cache_insert(cc_lru_cache_t *cache, cc_lru_cache_node_t *node);

void cc_lru_cache_delete(cc_lru_cache_t *cache, const char *key);

void cc_lru_cache_free(cc_lru_cache_t *cache);

#endif