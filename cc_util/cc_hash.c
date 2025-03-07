#include "cc_util/cc_hash.h"
#include "cc_util/cc_util.h"

static unsigned int cc_default_hash(const char *key, unsigned int size) {
  unsigned int hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
  }
  return hash % size;
}

static unsigned int cc_hash_table_load_factor(cc_hash_table_t *table) {
  if (table == NULL || table->bucket_size == 0) {
    return 0;
  }
  return (unsigned int)(((double)table->node_size / table->bucket_size) * 100);
}

// 创建哈希表
cc_hash_table_t *cc_hash_table_create(size_t bucket_size, cc_hash_func_t *hash_func,
                                      cc_hash_destroy_node_func_t *destroy_node_func) {
  cc_hash_table_t *table = (cc_hash_table_t *)cc_malloc(sizeof(cc_hash_table_t));
  if (!table) {
    return NULL;
  }
  table->node_size = 0;
  table->bucket_size = bucket_size < CC_MIN_BUCKET_SIZE ? CC_MIN_BUCKET_SIZE : bucket_size;
  table->hash_func = hash_func ? hash_func : cc_default_hash;
  table->destroy_node_func = destroy_node_func;
  table->buckets = (cc_hash_node_t **)cc_alloc(table->bucket_size, sizeof(cc_hash_node_t *));
  if (!table->buckets) {
    free(table);
    return NULL;
  }
  return table;
}

// 插入节点
void cc_hash_table_insert(cc_hash_table_t *table, cc_hash_node_t *node) {
  unsigned int index = table->hash_func(node->key, table->bucket_size);
  if (table->buckets[index] == NULL) {
    table->buckets[index] = node;
  } else {
    node->next = table->buckets[index];
    table->buckets[index] = node;
  }
  ++table->node_size;
}

// 查找节点
int cc_hash_table_lookup(cc_hash_table_t *table, const char *key, cc_hash_node_t **node) {
  unsigned int index = table->hash_func(key, table->bucket_size);
  cc_hash_node_t *current = table->buckets[index];
  while (current) {
    if (strcmp(current->key, key) == 0) {
      *node = current;
      return CC_HASH_OK;
    }
    current = current->next;
  }
  return CC_HASH_ERR;
}

// 删除节点
void cc_hash_table_delete(cc_hash_table_t *table, const char *key) {
  unsigned int index = table->hash_func(key, table->bucket_size);
  cc_hash_node_t *current = table->buckets[index];
  cc_hash_node_t *last = NULL;
  while (current) {
    if (strcmp(current->key, key) == 0) {
      if (last) {
        last->next = current->next;
      } else {
        table->buckets[index] = current->next;
      }
      --table->node_size;
      cc_hash_node_t *temp = current;
      if (table->destroy_node_func) {
        table->destroy_node_func(temp);
      }
      current = current->next;
    } else {
      last = current;
      current = current->next;
    }
  }
}

// 释放哈希表
void cc_hash_table_free(cc_hash_table_t *table) {
  for (size_t i = 0; i < table->bucket_size; ++i) {
    cc_hash_node_t *current = table->buckets[i];
    while (current) {
      cc_hash_node_t *temp = current->next;
      if (table->destroy_node_func) {
        table->destroy_node_func(current);
      }
      current = temp;
    }
  }
  cc_free(table->buckets);
  cc_free(table);
}

static void transfer_buckets(cc_dynamic_hash_table_t *table) {
  if (!table->large_table) {
    table->transfer_index = 0;
    table->large_table = cc_hash_table_create(table->small_table->bucket_size, table->small_table->hash_func,
                                              table->small_table->destroy_node_func);
    if (!table->large_table) {
      // warning
      return;
    }
  }
  if (table->transfer_index < table->small_table->bucket_size) {
    cc_hash_node_t *node = table->small_table->buckets[table->transfer_index];
    while (node) {
      cc_hash_node_t *next = node->next;
      cc_hash_table_insert(table->large_table, node);
      node = next;
    }
    table->transfer_index++;
  } else {
    table->small_table = table->large_table;
    table->large_table = NULL;
  }
}

// 创建动态哈希表
cc_dynamic_hash_table_t *cc_dynamic_hash_table_create(size_t bucket_size, cc_hash_func_t *hash_func,
                                                      cc_hash_destroy_node_func_t *destroy_node_func) {
  cc_dynamic_hash_table_t *table = (cc_dynamic_hash_table_t *)cc_malloc(sizeof(cc_dynamic_hash_table_t));
  if (!table) {
    return NULL;
  }
  table->transfer_index = 0;
  table->small_table = cc_hash_table_create(bucket_size, hash_func, destroy_node_func);
  if (!table->small_table) {
    cc_free(table);
    return NULL;
  }
  return table;
}

// 插入节点
void cc_dynamic_hash_table_insert(cc_dynamic_hash_table_t *table, cc_hash_node_t *node) {
  if (table->large_table) {
    cc_hash_table_insert(table->large_table, node);
  } else {
    cc_hash_table_insert(table->small_table, node);
    unsigned int load_factor = cc_hash_table_load_factor(table->small_table);
    if (load_factor > CC_GROW_LOAD_FACTOR) {
      transfer_buckets(table);
    }
  }
}

// 查找节点
int cc_dynamic_hash_table_lookup(cc_dynamic_hash_table_t *table, const char *key, cc_hash_node_t **node) {
  if (table->large_table) {
    if (cc_hash_table_lookup(table->large_table, key, node) == CC_HASH_OK) {
      return CC_HASH_OK;
    }
  }
  if (table->small_table) {
    if (cc_hash_table_lookup(table->small_table, key, node) == CC_HASH_OK) {
      return CC_HASH_OK;
    }
  }
  return CC_HASH_ERR;
}

// 删除节点
void cc_dynamic_hash_table_delete(cc_dynamic_hash_table_t *table, const char *key) {
  if (table->large_table) {
    cc_hash_table_delete(table->large_table, key);
  }
  if (table->small_table) {
    cc_hash_table_delete(table->small_table, key);
  }
  if (table->small_table && table->large_table) {
    transfer_buckets(table);
  }
}

// 释放动态哈希表
void cc_dynamic_hash_table_free(cc_dynamic_hash_table_t *table) {
  if (table->large_table) {
    cc_hash_table_free(table->large_table);
  }
  if (table->small_table) {
    cc_hash_table_free(table->small_table);
  }
  cc_free(table);
}