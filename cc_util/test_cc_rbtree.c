#include "cc_util/cc_rbtree.h"
#include "cc_util/cc_test_tools.h"

// 中序遍历打印红黑树，使用缩进来展示树状结构
void print_tree_inorder(cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel, int level) {
  if (node != sentinel) {
    print_tree_inorder(node->right, sentinel, level + 1);
    for (int i = 0; i < level; i++) {
      printf("    ");
    }
    printf("Key: %lu, Color: %s\n", node->key, cc_rbt_is_red(node) ? "Red" : "Black");
    print_tree_inorder(node->left, sentinel, level + 1);
  }
}

void test_insert_and_check() {
  cc_rbtree_t tree;
  cc_rbtree_node_t sentinel;
  cc_rbtree_init(&tree, &sentinel, cc_rbtree_insert_value);

  cc_rbtree_node_t nodes[10] = {{10, NULL, NULL, NULL, 0, 0}, {20, NULL, NULL, NULL, 0, 0},
                                {5, NULL, NULL, NULL, 0, 0},  {15, NULL, NULL, NULL, 0, 0},
                                {25, NULL, NULL, NULL, 0, 0}, {3, NULL, NULL, NULL, 0, 0},
                                {7, NULL, NULL, NULL, 0, 0},  {12, NULL, NULL, NULL, 0, 0},
                                {18, NULL, NULL, NULL, 0, 0}, {30, NULL, NULL, NULL, 0, 0}};

  for (int i = 0; i < 10; i++) {
    cc_rbtree_insert(&tree, &nodes[i]);
  }
  print_tree_inorder(tree.root, &sentinel, 0);

  assert(cc_rbt_is_black(&nodes[0]));
  assert(cc_rbt_is_red(&nodes[1]));
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_black(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_red(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_red(&nodes[9]));
}

void test_delete_and_check() {
  cc_rbtree_t tree;
  cc_rbtree_node_t sentinel;
  cc_rbtree_init(&tree, &sentinel, cc_rbtree_insert_value);

  cc_rbtree_node_t nodes[10] = {{10, NULL, NULL, NULL, 0, 0}, {20, NULL, NULL, NULL, 0, 0},
                                {5, NULL, NULL, NULL, 0, 0},  {15, NULL, NULL, NULL, 0, 0},
                                {25, NULL, NULL, NULL, 0, 0}, {3, NULL, NULL, NULL, 0, 0},
                                {7, NULL, NULL, NULL, 0, 0},  {12, NULL, NULL, NULL, 0, 0},
                                {18, NULL, NULL, NULL, 0, 0}, {30, NULL, NULL, NULL, 0, 0}};

  for (int i = 0; i < 10; i++) {
    cc_rbtree_insert(&tree, &nodes[i]);
  }

  cc_rbtree_delete(&tree, &nodes[1]);  // 删除 key 为 20 的节点
  print_tree_inorder(tree.root, &sentinel, 0);

  // 检查删除后的节点颜色
  assert(cc_rbt_is_black(&nodes[0]));
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_red(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_red(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_black(&nodes[9]));

  cc_rbtree_delete(&tree, &nodes[0]);  // 删除 key 为 10 的节点
  print_tree_inorder(tree.root, &sentinel, 0);

  // 检查删除后的节点颜色
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_red(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_black(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_black(&nodes[9]));
}

void test_insert_timer_value_and_check() {
  cc_rbtree_t tree;
  cc_rbtree_node_t sentinel;
  cc_rbtree_init(&tree, &sentinel, cc_rbtree_insert_timer_value);

  cc_rbtree_node_t nodes[10] = {{10, NULL, NULL, NULL, 0, 0}, {20, NULL, NULL, NULL, 0, 0},
                                {5, NULL, NULL, NULL, 0, 0},  {15, NULL, NULL, NULL, 0, 0},
                                {25, NULL, NULL, NULL, 0, 0}, {3, NULL, NULL, NULL, 0, 0},
                                {7, NULL, NULL, NULL, 0, 0},  {12, NULL, NULL, NULL, 0, 0},
                                {18, NULL, NULL, NULL, 0, 0}, {30, NULL, NULL, NULL, 0, 0}};

  for (int i = 0; i < 10; i++) {
    cc_rbtree_insert(&tree, &nodes[i]);
  }

  print_tree_inorder(tree.root, &sentinel, 0);

  // 检查插入后的节点颜色
  assert(nodes[0].key == 10);
  assert(cc_rbt_is_black(&nodes[0]));
  assert(cc_rbt_is_red(&nodes[1]));
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_black(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_red(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_red(&nodes[9]));

  // printf("Insert timer value and check test passed.\n");
}

void test_delete_timer_value_and_check() {
  cc_rbtree_t tree;
  cc_rbtree_node_t sentinel;
  cc_rbtree_init(&tree, &sentinel, cc_rbtree_insert_timer_value);

  cc_rbtree_node_t nodes[10] = {{10, NULL, NULL, NULL, 0, 0}, {20, NULL, NULL, NULL, 0, 0},
                                {5, NULL, NULL, NULL, 0, 0},  {15, NULL, NULL, NULL, 0, 0},
                                {25, NULL, NULL, NULL, 0, 0}, {3, NULL, NULL, NULL, 0, 0},
                                {7, NULL, NULL, NULL, 0, 0},  {12, NULL, NULL, NULL, 0, 0},
                                {18, NULL, NULL, NULL, 0, 0}, {30, NULL, NULL, NULL, 0, 0}};

  for (int i = 0; i < 10; i++) {
    cc_rbtree_insert(&tree, &nodes[i]);
  }

  cc_rbtree_delete(&tree, &nodes[1]);  // 删除 key 为 20 的节点
  print_tree_inorder(tree.root, &sentinel, 0);

  // 检查删除后的节点颜色
  assert(cc_rbt_is_black(&nodes[0]));
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_red(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_red(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_black(&nodes[9]));

  cc_rbtree_delete(&tree, &nodes[0]);  // 删除 key 为 10 的节点
  print_tree_inorder(tree.root, &sentinel, 0);

  // 检查删除后的节点颜色
  assert(cc_rbt_is_black(&nodes[2]));
  assert(cc_rbt_is_black(&nodes[3]));
  assert(cc_rbt_is_red(&nodes[4]));
  assert(cc_rbt_is_red(&nodes[5]));
  assert(cc_rbt_is_red(&nodes[6]));
  assert(cc_rbt_is_black(&nodes[7]));
  assert(cc_rbt_is_red(&nodes[8]));
  assert(cc_rbt_is_black(&nodes[9]));
}

int main() {
  RUN_TEST_FUNC(test_insert_and_check);
  RUN_TEST_FUNC(test_delete_and_check);
  RUN_TEST_FUNC(test_insert_timer_value_and_check);
  RUN_TEST_FUNC(test_delete_timer_value_and_check);
  return 0;
}