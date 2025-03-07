#include <string.h>
#include "cc_util/cc_lru.h"
#include "cc_util/cc_test_tools.h"

// Helper function to create a new LRU cache node
cc_lru_cache_node_t *create_lru_cache_node(const char *key) {
  cc_lru_cache_node_t *node = (cc_lru_cache_node_t *)malloc(sizeof(cc_lru_cache_node_t));
  assert(node != NULL);
  node->hash_node.key = strdup(key);
  node->list_node.prev = NULL;
  node->list_node.next = NULL;
  return node;
}

// Helper function to free a LRU cache node
void free_lru_cache_node(cc_lru_cache_node_t *node) {
  free(node->hash_node.key);
  free(node);
}

// Helper function to print LRU cache
void print_lru_cache(cc_lru_cache_t *cache) {
  printf("LRU Cache:\n");
  printf("Capacity: %zu\n", cache->capacity);
  printf("Size: %zu\n", cache->size);
  printf("Bucket Size: %zu\n", cache->bucket_size);
  printf("Doubly Linked List:\n");
  printf("Address\t\tprev\t\tnext\t\tKey\n");
  cc_dlist_node_t *current = cache->list_head.next;
  while (current != &(cache->list_head)) {
    cc_lru_cache_node_t *cache_node = cc_list_node_to_cache_node(current);
    printf("%p\t%p\t%p\t\t%s\n", (void *)current, (void *)current->prev, (void *)current->next,
           cache_node->hash_node.key);
    current = current->next;
  }
  printf("\n");
}

// Test cc_lru_cache_create
void test_cc_lru_cache_create(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(10, NULL, NULL);
  assert(cache != NULL);
  assert(cache->capacity == 10);
  assert(cache->size == 0);
  assert(cache->bucket_size == (size_t)((double)10 / ((double)CC_GROW_LOAD_FACTOR / 100)));
  print_lru_cache(cache);
  cc_lru_cache_free(cache);
}

// Test cc_lru_cache_insert
void test_cc_lru_cache_insert(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(3, NULL, NULL);
  cc_lru_cache_node_t *node1 = create_lru_cache_node("key1");
  cc_lru_cache_insert(cache, node1);
  assert(cache->size == 1);
  print_lru_cache(cache);

  cc_lru_cache_node_t *node2 = create_lru_cache_node("key2");
  cc_lru_cache_insert(cache, node2);
  assert(cache->size == 2);
  print_lru_cache(cache);

  cc_lru_cache_node_t *node3 = create_lru_cache_node("key3");
  cc_lru_cache_insert(cache, node3);
  assert(cache->size == 3);
  print_lru_cache(cache);

  cc_lru_cache_node_t *node4 = create_lru_cache_node("key4");
  cc_lru_cache_insert(cache, node4);
  assert(cache->size == 3);
  print_lru_cache(cache);

  cc_lru_cache_free(cache);
  free_lru_cache_node(node1);
  free_lru_cache_node(node2);
  free_lru_cache_node(node3);
  free_lru_cache_node(node4);
}

// Test cc_lru_cache_lookup
void test_cc_lru_cache_lookup(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(3, NULL, NULL);
  cc_lru_cache_node_t *node1 = create_lru_cache_node("key1");
  cc_lru_cache_insert(cache, node1);

  cc_lru_cache_node_t *node2 = create_lru_cache_node("key2");
  cc_lru_cache_insert(cache, node2);

  cc_lru_cache_node_t *node3 = create_lru_cache_node("key3");
  cc_lru_cache_insert(cache, node3);

  cc_lru_cache_node_t *found_node;
  int result = cc_lru_cache_lookup(cache, "key1", &found_node);
  assert(result == CC_HASH_OK);
  assert(strcmp(found_node->hash_node.key, "key1") == 0);
  print_lru_cache(cache);

  result = cc_lru_cache_lookup(cache, "key2", &found_node);
  assert(result == CC_HASH_OK);
  assert(strcmp(found_node->hash_node.key, "key2") == 0);
  print_lru_cache(cache);

  result = cc_lru_cache_lookup(cache, "key4", &found_node);
  assert(result == CC_HASH_ERR);
  print_lru_cache(cache);

  cc_lru_cache_free(cache);
  free_lru_cache_node(node1);
  free_lru_cache_node(node2);
  free_lru_cache_node(node3);
}

// Test cc_lru_cache_delete
void test_cc_lru_cache_delete(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(3, NULL, NULL);
  cc_lru_cache_node_t *node1 = create_lru_cache_node("key1");
  cc_lru_cache_insert(cache, node1);

  cc_lru_cache_node_t *node2 = create_lru_cache_node("key2");
  cc_lru_cache_insert(cache, node2);

  cc_lru_cache_node_t *node3 = create_lru_cache_node("key3");
  cc_lru_cache_insert(cache, node3);

  cc_lru_cache_delete(cache, "key1");
  assert(cache->size == 2);
  print_lru_cache(cache);

  cc_lru_cache_delete(cache, "key2");
  assert(cache->size == 1);
  print_lru_cache(cache);

  cc_lru_cache_delete(cache, "key3");
  assert(cache->size == 0);
  print_lru_cache(cache);

  cc_lru_cache_free(cache);
  free_lru_cache_node(node1);
  free_lru_cache_node(node2);
  free_lru_cache_node(node3);
}

// Test cc_lru_cache_delete_nonexistent_key
void test_cc_lru_cache_delete_nonexistent_key(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(3, NULL, NULL);
  cc_lru_cache_delete(cache, "key1");
  assert(cache->size == 0);
  print_lru_cache(cache);

  cc_lru_cache_free(cache);
}

// Test cc_lru_cache_free
void test_cc_lru_cache_free(void) {
  cc_lru_cache_t *cache = cc_lru_cache_create(3, NULL, NULL);
  cc_lru_cache_node_t *node1 = create_lru_cache_node("key1");
  cc_lru_cache_insert(cache, node1);

  cc_lru_cache_free(cache);
  // No assertions needed as the function should not crash
}

int main(void) {
  RUN_TEST_FUNC(test_cc_lru_cache_create);
  RUN_TEST_FUNC(test_cc_lru_cache_insert);
  RUN_TEST_FUNC(test_cc_lru_cache_lookup);
  RUN_TEST_FUNC(test_cc_lru_cache_delete);
  RUN_TEST_FUNC(test_cc_lru_cache_delete_nonexistent_key);
  RUN_TEST_FUNC(test_cc_lru_cache_free);
  return 0;
}