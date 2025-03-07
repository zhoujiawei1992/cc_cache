#ifndef __CC_UTIL_CC_RBTREE_H__
#define __CC_UTIL_CC_RBTREE_H__

#include <stdlib.h>

typedef unsigned long cc_rbtree_key_t;
typedef int cc_rbtree_key_int_t;

typedef struct _cc_rbtree_node_s {
  cc_rbtree_key_t key;
  struct _cc_rbtree_node_s *left;
  struct _cc_rbtree_node_s *right;
  struct _cc_rbtree_node_s *parent;
  unsigned char color;
  unsigned char data;
} cc_rbtree_node_t;

typedef void (*cc_rbtree_insert_pt)(cc_rbtree_node_t *root, cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel);

typedef struct _cc_rbtree_s {
  cc_rbtree_node_t *root;
  cc_rbtree_node_t *sentinel;
  cc_rbtree_insert_pt insert;
} cc_rbtree_t;

#define cc_rbtree_init(tree, s, i) \
  cc_rbtree_sentinel_init(s);      \
  (tree)->root = s;                \
  (tree)->sentinel = s;            \
  (tree)->insert = i

#define cc_rbtree_data(node, type, link) (type *)((unsigned char *)(node) - offsetof(type, link))

void cc_rbtree_insert(cc_rbtree_t *tree, cc_rbtree_node_t *node);
void cc_rbtree_delete(cc_rbtree_t *tree, cc_rbtree_node_t *node);
void cc_rbtree_insert_value(cc_rbtree_node_t *root, cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel);
void cc_rbtree_insert_timer_value(cc_rbtree_node_t *root, cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel);
cc_rbtree_node_t *cc_rbtree_next(cc_rbtree_t *tree, cc_rbtree_node_t *node);

#define cc_rbt_red(node) ((node)->color = 1)
#define cc_rbt_black(node) ((node)->color = 0)
#define cc_rbt_is_red(node) ((node)->color)
#define cc_rbt_is_black(node) (!cc_rbt_is_red(node))
#define cc_rbt_copy_color(n1, n2) (n1->color = n2->color)

#define cc_rbtree_sentinel_init(node) cc_rbt_black(node)

static cc_rbtree_node_t *cc_rbtree_min(cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel) {
  while (node->left != sentinel) {
    node = node->left;
  }
  return node;
}
#endif