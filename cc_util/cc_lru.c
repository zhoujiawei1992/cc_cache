#include "cc_util/cc_lru.h"

cc_lru_cache_t *cc_lru_cache_create(size_t capacity, cc_hash_func_t *hash_func, cc_compare_func_t *compare_func,
                                    cc_hash_destroy_node_func_t *destroy_node_func) {
  cc_lru_cache_t *cache = (cc_lru_cache_t *)cc_malloc(sizeof(cc_lru_cache_t));
  if (!cache) {
    return NULL;
  }
  size_t bucket_size = (size_t)((double)capacity / ((double)CC_GROW_LOAD_FACTOR / 100));
  cache->hash_table = cc_hash_table_create(bucket_size, hash_func, compare_func, destroy_node_func);
  if (!cache->hash_table) {
    cc_free(cache);
    return NULL;
  }
  cache->capacity = capacity;
  cache->bucket_size = bucket_size;
  cache->size = 0;
  cc_dlist_init(&(cache->list_head));
  return cache;
}

int cc_lru_cache_lookup(cc_lru_cache_t *cache, const char *key, unsigned int key_size, cc_lru_cache_node_t **node) {
  cc_hash_node_t *hash_node;
  if (cc_hash_table_lookup(cache->hash_table, key, key_size, &hash_node) == CC_HASH_OK) {
    *node = cc_hash_node_to_cache_node(hash_node);
    cc_dlist_delete(&(cache->list_head), &((*node)->list_node));
    cc_dlist_insert_backward(&(cache->list_head), &((*node)->list_node));
    return CC_HASH_OK;
  }
  return CC_HASH_ERR;
}

void cc_lru_cache_insert(cc_lru_cache_t *cache, cc_lru_cache_node_t *node) {
  if (cache->size >= cache->capacity) {
    cc_dlist_node_t *last_node = cache->list_head.prev;
    cc_dlist_delete(&(cache->list_head), last_node);
    cc_lru_cache_node_t *last_cache_node = cc_list_node_to_cache_node(last_node);
    cc_hash_table_delete(cache->hash_table, last_cache_node->hash_node.key.data, last_cache_node->hash_node.key.size,
                         NULL);
    cache->size--;
  }
  cc_hash_table_insert(cache->hash_table, &(node->hash_node));
  cc_dlist_insert_backward(&(cache->list_head), &(node->list_node));
  cache->size++;
}

void cc_lru_cache_delete(cc_lru_cache_t *cache, const char *key, unsigned int key_size) {
  cc_hash_node_t *hash_node;
  if (cc_hash_table_lookup(cache->hash_table, key, key_size, &hash_node) == CC_HASH_OK) {
    cc_lru_cache_node_t *node = cc_hash_node_to_cache_node(hash_node);
    cc_dlist_delete(&(cache->list_head), &(node->list_node));
    cc_hash_table_delete(cache->hash_table, key, key_size, NULL);
    cache->size--;
  }
}

void cc_lru_cache_free(cc_lru_cache_t *cache) {
  cc_hash_table_free(cache->hash_table);
  cc_free(cache);
}