#include "cc_util/cc_list.h"
#include "cc_util/cc_test_tools.h"

// 打印双向链表
void print_dlist(cc_dlist_node_t *head) {
  cc_dlist_node_t *current = head;
  printf("Doubly Linked List:\n");
  printf("Address\t\tprev\t\tnext\n");
  do {
    printf("%p\t%p\t%p\n", (void *)current, (void *)current->prev, (void *)current->next);
    current = current->next;
  } while (current != head);
  printf("\n");
}

// 打印单链表
void print_slist(cc_slist_node_t *head) {
  cc_slist_node_t *current = head;
  printf("Singly Linked List:\n");
  printf("Address\tnext\n");
  do {
    printf("%p\t%p\n", (void *)current, (void *)current->next);
    current = current->next;
  } while (current != NULL);
  printf("\n");
}

// Helper function to create a new dlist node
cc_dlist_node_t *create_dlist_node() {
  cc_dlist_node_t *node = (cc_dlist_node_t *)malloc(sizeof(cc_dlist_node_t));
  assert(node != NULL);
  node->prev = NULL;
  node->next = NULL;
  return node;
}

// Helper function to create a new slist node
cc_slist_node_t *create_slist_node() {
  cc_slist_node_t *node = (cc_slist_node_t *)malloc(sizeof(cc_slist_node_t));
  assert(node != NULL);
  node->next = NULL;
  return node;
}

// Helper function to free a dlist node
void free_dlist_node(cc_dlist_node_t *node) { free(node); }

// Helper function to free a slist node
void free_slist_node(cc_slist_node_t *node) { free(node); }

// Test cc_dlist_init
void test_cc_dlist_init(void) {
  cc_dlist_node_t head;
  cc_dlist_init(&head);
  assert(cc_dlist_empty(&head));

  print_dlist(&head);
}

// Test cc_dlist_insert_backward
void test_cc_dlist_insert_backward(void) {
  cc_dlist_node_t head;
  cc_dlist_init(&head);

  cc_dlist_node_t *node1 = create_dlist_node();
  cc_dlist_insert_backward(&head, node1);
  assert(head.next == node1);
  assert(head.prev == node1);
  assert(node1->next == &head);
  assert(node1->prev == &head);

  cc_dlist_node_t *node2 = create_dlist_node();
  cc_dlist_insert_backward(node1, node2);
  assert(head.next == node2);
  assert(head.prev == node1);
  assert(node2->next == node1);
  assert(node2->prev == &head);
  assert(node1->next == &head);
  assert(node1->prev == node2);

  print_dlist(&head);

  free_dlist_node(node1);
  free_dlist_node(node2);
}

static void prinf_node(void *node) {}

// Test cc_dlist_traverse_backward
void test_cc_dlist_traverse_backward(void) {
  cc_dlist_node_t head;
  cc_dlist_init(&head);

  cc_dlist_node_t *node1 = create_dlist_node();
  cc_dlist_insert_backward(&head, node1);

  cc_dlist_node_t *node2 = create_dlist_node();
  cc_dlist_insert_backward(node1, node2);

  cc_dlist_traverse_backward(&head, prinf_node);

  free_dlist_node(node1);
  free_dlist_node(node2);
}

// Test cc_slist_init
void test_cc_slist_init(void) {
  cc_slist_node_t head;
  cc_slist_init(&head);
  assert(cc_slist_empty(&head));

  print_slist(&head);
}

// Test cc_slist_insert
void test_cc_slist_insert(void) {
  cc_slist_node_t head;
  cc_slist_init(&head);

  cc_slist_node_t *node1 = create_slist_node();
  cc_slist_insert(&head, node1);
  assert(head.next == node1);

  cc_slist_node_t *node2 = create_slist_node();
  cc_slist_insert(node1, node2);
  assert(head.next == node1);
  assert(node1->next == node2);

  print_slist(&head);

  free_slist_node(node1);
  free_slist_node(node2);
}

// Test cc_slist_delete
void test_cc_slist_delete(void) {
  cc_slist_node_t head;
  cc_slist_init(&head);

  cc_slist_node_t *node1 = create_slist_node();
  cc_slist_insert(&head, node1);

  cc_slist_node_t *node2 = create_slist_node();
  cc_slist_insert(node1, node2);

  cc_slist_delete(node1);
  assert(head.next == node1);
  assert(node1->next == NULL);

  print_slist(&head);

  free_slist_node(node1);
  free_slist_node(node2);
}

// Test cc_slist_delete_node
void test_cc_slist_delete_node(void) {
  cc_slist_node_t head;
  cc_slist_init(&head);

  cc_slist_node_t *node1 = create_slist_node();
  cc_slist_insert(&head, node1);

  cc_slist_node_t *node2 = create_slist_node();
  cc_slist_insert(node1, node2);

  cc_slist_delete_node(&head, node1);
  assert(head.next == node2);

  cc_slist_delete_node(&head, node2);
  assert(cc_slist_empty(&head));

  print_slist(&head);

  free_slist_node(node1);
  free_slist_node(node2);
}

// Test cc_slist_traverse
void test_cc_slist_traverse(void) {
  cc_slist_node_t head;
  cc_slist_init(&head);

  cc_slist_node_t *node1 = create_slist_node();
  cc_slist_insert(&head, node1);

  cc_slist_node_t *node2 = create_slist_node();
  cc_slist_insert(node1, node2);

  cc_slist_traverse(&head, prinf_node);

  free_slist_node(node1);
  free_slist_node(node2);
}

int main(void) {
  RUN_TEST_FUNC(test_cc_dlist_init);
  RUN_TEST_FUNC(test_cc_dlist_insert_backward);
  RUN_TEST_FUNC(test_cc_dlist_traverse_backward);

  RUN_TEST_FUNC(test_cc_slist_init);
  RUN_TEST_FUNC(test_cc_slist_insert);
  RUN_TEST_FUNC(test_cc_slist_delete);
  RUN_TEST_FUNC(test_cc_slist_delete_node);
  RUN_TEST_FUNC(test_cc_slist_traverse);

  return 0;
}