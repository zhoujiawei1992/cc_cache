#include "cc_util/cc_hash.h"
#include "cc_util/cc_test_tools.h"

#include <stdio.h>
#include <string.h>

// 打印普通哈希表
void print_hash_table(cc_hash_table_t *table) {
  printf("Hash Table:\n");
  printf("Bucket\tNode Address\tKey\tNext Node Address\n");
  for (size_t i = 0; i < table->bucket_size; i++) {
    cc_hash_node_t *current = table->buckets[i];
    while (current != NULL) {
      printf("%zu\t%p\t%s\t%p\n", i, (void *)current, current->key, (void *)current->next);
      current = current->next;
    }
  }
  printf("\n");
}

// 打印动态哈希表
void print_dynamic_hash_table(cc_dynamic_hash_table_t *table) {
  printf("Dynamic Hash Table:\n");

  printf("Small Table:\n");
  print_hash_table(table->small_table);

  if (table->large_table != NULL) {
    printf("Large Table:\n");
    print_hash_table(table->large_table);
  } else {
    printf("Large Table: Not yet created\n");
  }

  printf("Transfer Index: %zu\n", table->transfer_index);
  printf("\n");
}
void test_cc_hash_table_create(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  assert(table != NULL);
  assert(table->bucket_size == 10);
  assert(table->node_size == 0);
  cc_hash_table_free(table);
}

void test_cc_hash_table_insert(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_hash_table_insert(table, node1);
  assert(table->node_size == 1);

  cc_hash_node_t *node2 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node2->key = strdup("key2");
  node2->next = NULL;

  cc_hash_table_insert(table, node2);
  assert(table->node_size == 2);

  print_hash_table(table);

  cc_hash_table_free(table);
  free(node1->key);
  free(node1);
  free(node2->key);
  free(node2);
}

void test_cc_hash_table_lookup(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_hash_table_insert(table, node1);

  cc_hash_node_t *found_node;
  int result = cc_hash_table_lookup(table, "key1", &found_node);
  assert(result == CC_HASH_OK);
  assert(strcmp(found_node->key, "key1") == 0);

  result = cc_hash_table_lookup(table, "key2", &found_node);
  assert(result == CC_HASH_ERR);

  cc_hash_table_free(table);
  free(node1->key);
  free(node1);
}

void test_cc_hash_table_delete(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_hash_table_insert(table, node1);

  cc_hash_table_delete(table, "key1");
  assert(table->node_size == 0);

  print_hash_table(table);

  cc_hash_table_free(table);
  free(node1->key);
  free(node1);
}

void test_cc_hash_table_delete_nonexistent_key(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  cc_hash_table_delete(table, "key1");
  assert(table->node_size == 0);

  cc_hash_table_free(table);
}

void test_cc_hash_table_free(void) {
  cc_hash_table_t *table = cc_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_hash_table_insert(table, node1);

  cc_hash_table_free(table);
  // No assertions needed as the function should not crash
}

void test_cc_dynamic_hash_table_create(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  assert(table != NULL);
  assert(table->small_table != NULL);
  assert(table->large_table == NULL);
  assert(table->transfer_index == 0);
  cc_dynamic_hash_table_free(table);
}

void test_cc_dynamic_hash_table_insert(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_dynamic_hash_table_insert(table, node1);
  assert(table->small_table->node_size == 1);

  for (int i = 0; i < 75; i++) {
    char key[10];
    snprintf(key, 10, "key%d", i + 2);
    cc_hash_node_t *node = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
    node->key = strdup(key);
    node->next = NULL;
    cc_dynamic_hash_table_insert(table, node);
  }

  print_dynamic_hash_table(table);

  assert(table->large_table != NULL);
  cc_dynamic_hash_table_free(table);
}

void test_cc_dynamic_hash_table_lookup(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_dynamic_hash_table_insert(table, node1);

  cc_hash_node_t *found_node;
  int result = cc_dynamic_hash_table_lookup(table, "key1", &found_node);
  assert(result == CC_HASH_OK);
  assert(strcmp(found_node->key, "key1") == 0);

  result = cc_dynamic_hash_table_lookup(table, "key2", &found_node);
  assert(result == CC_HASH_ERR);

  cc_dynamic_hash_table_free(table);
  free(node1->key);
  free(node1);
}

void test_cc_dynamic_hash_table_delete(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_dynamic_hash_table_insert(table, node1);

  cc_dynamic_hash_table_delete(table, "key1");
  assert(table->small_table->node_size == 0);

  print_dynamic_hash_table(table);

  cc_dynamic_hash_table_free(table);
  free(node1->key);
  free(node1);
}

void test_cc_dynamic_hash_table_delete_nonexistent_key(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  cc_dynamic_hash_table_delete(table, "key1");
  assert(table->small_table->node_size == 0);

  cc_dynamic_hash_table_free(table);
}

void test_cc_dynamic_hash_table_free(void) {
  cc_dynamic_hash_table_t *table = cc_dynamic_hash_table_create(10, NULL, NULL);
  cc_hash_node_t *node1 = (cc_hash_node_t *)malloc(sizeof(cc_hash_node_t));
  node1->key = strdup("key1");
  node1->next = NULL;

  cc_dynamic_hash_table_insert(table, node1);

  cc_dynamic_hash_table_free(table);
  // No assertions needed as the function should not crash
}

int main(void) {
  RUN_TEST_FUNC(test_cc_hash_table_create);
  RUN_TEST_FUNC(test_cc_hash_table_insert);
  RUN_TEST_FUNC(test_cc_hash_table_lookup);
  RUN_TEST_FUNC(test_cc_hash_table_delete);
  RUN_TEST_FUNC(test_cc_hash_table_delete_nonexistent_key);
  RUN_TEST_FUNC(test_cc_hash_table_free);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_create);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_insert);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_lookup);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_delete);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_delete_nonexistent_key);
  RUN_TEST_FUNC(test_cc_dynamic_hash_table_free);
  return 0;
}