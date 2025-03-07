#ifndef __CC_UTIL_CC_LIST_H__
#define __CC_UTIL_CC_LIST_H__

#include <stdlib.h>

typedef struct _cc_dlist_node_s {
  struct _cc_dlist_node_s *prev;
  struct _cc_dlist_node_s *next;
} cc_dlist_node_t;

typedef struct _cc_slist_node_s {
  struct _cc_slist_node_s *next;
} cc_slist_node_t;

#define cc_list_data(node, type, link) (type *)((unsigned char *)(node) - offsetof(type, link))

#define cc_dlist_init(head) \
  do {                      \
    (head)->prev = (head);  \
    (head)->next = (head);  \
  } while (0)

#define cc_dlist_empty(head) ((head)->next == (head))

#define cc_dlist_insert_forward(target, node) \
  do {                                        \
    (node)->next = (target);                  \
    (node)->prev = (target)->prev;            \
    (target)->prev->next = (node);            \
    (target)->prev = (node);                  \
  } while (0)

#define cc_dlist_insert_backward(target, node) \
  do {                                         \
    (node)->prev = (target);                   \
    (node)->next = (target)->next;             \
    (target)->next->prev = (node);             \
    (target)->next = (node);                   \
  } while (0)

#define cc_dlist_delete(head, node)       \
  do {                                    \
    if ((head) != (node)) {               \
      (node)->prev->next = (node)->next;  \
      (node)->next->prev = (node)->prev;  \
      (node)->next = (node)->prev = NULL; \
    }                                     \
  } while (0)

#define cc_dlist_traverse_forward(head, func) \
  do {                                        \
    cc_dlist_node_t *current = (head)->next;  \
    while (current != (head)) {               \
      func(current);                          \
      current = current->next;                \
    }                                         \
  } while (0)

#define cc_dlist_traverse_backward(head, func) \
  do {                                         \
    cc_dlist_node_t *current = (head)->prev;   \
    while (current != (head)) {                \
      func(current);                           \
      current = current->prev;                 \
    }                                          \
  } while (0)

#define cc_slist_init(head) \
  do {                      \
    (head)->next = NULL;    \
  } while (0)

#define cc_slist_empty(head) ((head)->next == NULL)

#define cc_slist_insert(target, node) \
  do {                                \
    (node)->next = (target)->next;    \
    (target)->next = (node);          \
  } while (0)

#define cc_slist_delete(target)               \
  do {                                        \
    if ((target)->next != NULL) {             \
      cc_slist_node_t *temp = (target)->next; \
      (target)->next = temp->next;            \
    }                                         \
  } while (0)

#define cc_slist_delete_node(head, node)                       \
  do {                                                         \
    cc_slist_node_t *current = (head);                         \
    while (current->next != NULL && current->next != (node)) { \
      current = current->next;                                 \
    }                                                          \
    if (current->next == (node)) {                             \
      current->next = (node)->next;                            \
    }                                                          \
  } while (0)

#define cc_slist_traverse(head, func)        \
  do {                                       \
    cc_slist_node_t *current = (head)->next; \
    while (current != NULL) {                \
      func(current);                         \
      current = current->next;               \
    }                                        \
  } while (0)

#endif