#include "cc_util/cc_rbtree.h"

void cc_rbtree_left_rotate(cc_rbtree_node_t **root, cc_rbtree_node_t *sentinel, cc_rbtree_node_t *node);
void cc_rbtree_right_rotate(cc_rbtree_node_t **root, cc_rbtree_node_t *sentinel, cc_rbtree_node_t *node);

void cc_rbtree_insert(cc_rbtree_t *tree, cc_rbtree_node_t *node) {
  cc_rbtree_node_t **root, *temp, *sentinel;
  root = &tree->root;
  sentinel = tree->sentinel;
  if (*root == sentinel) {
    node->parent = NULL;
    node->left = sentinel;
    node->right = sentinel;
    cc_rbt_black(node);
    *root = node;
    return;
  }
  tree->insert(*root, node, sentinel);

  while (node != *root && cc_rbt_is_red(node->parent)) {
    if (node->parent == node->parent->parent->left) {
      temp = node->parent->parent->right;
      if (cc_rbt_is_red(temp)) {
        cc_rbt_black(node->parent);
        cc_rbt_black(temp);
        cc_rbt_red(node->parent->parent);
        node = node->parent->parent;
      } else {
        if (node == node->parent->right) {
          node = node->parent;
          cc_rbtree_left_rotate(root, sentinel, node);
        }
        cc_rbt_black(node->parent);
        cc_rbt_red(node->parent->parent);
        cc_rbtree_right_rotate(root, sentinel, node->parent->parent);
      }
    } else {
      temp = node->parent->parent->left;
      if (cc_rbt_is_red(temp)) {
        cc_rbt_black(node->parent);
        cc_rbt_black(temp);
        cc_rbt_red(node->parent->parent);
        node = node->parent->parent;
      } else {
        if (node == node->parent->left) {
          node = node->parent;
          cc_rbtree_right_rotate(root, sentinel, node);
        }
        cc_rbt_black(node->parent);
        cc_rbt_red(node->parent->parent);
        cc_rbtree_left_rotate(root, sentinel, node->parent->parent);
      }
    }
  }
  cc_rbt_black(*root);
}

void cc_rbtree_insert_value(cc_rbtree_node_t *temp, cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel) {
  cc_rbtree_node_t **p;
  for (;;) {
    p = (node->key < temp->key) ? &temp->left : &temp->right;
    if (*p == sentinel) {
      break;
    }
    temp = *p;
  }
  *p = node;
  node->parent = temp;
  node->left = sentinel;
  node->right = sentinel;
  cc_rbt_red(node);
}

void cc_rbtree_insert_timer_value(cc_rbtree_node_t *temp, cc_rbtree_node_t *node, cc_rbtree_node_t *sentinel) {
  cc_rbtree_node_t **p;
  for (;;) {
    p = ((cc_rbtree_key_int_t)(node->key - temp->key) < 0) ? &temp->left : &temp->right;
    if (*p == sentinel) {
      break;
    }
    temp = *p;
  }
  *p = node;
  node->parent = temp;
  node->left = sentinel;
  node->right = sentinel;
  cc_rbt_red(node);
}

void cc_rbtree_delete(cc_rbtree_t *tree, cc_rbtree_node_t *node) {
  unsigned char red;
  cc_rbtree_node_t **root, *sentinel, *subst, *temp, *w;
  root = &tree->root;
  sentinel = tree->sentinel;
  if (node->left == sentinel) {
    temp = node->right;
    subst = node;
  } else if (node->right == sentinel) {
    temp = node->left;
    subst = node;
  } else {
    subst = cc_rbtree_min(node->right, sentinel);
    temp = subst->right;
  }
  if (subst == *root) {
    *root = temp;
    cc_rbt_black(temp);
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;
    return;
  }

  red = cc_rbt_is_red(subst);
  if (subst == subst->parent->left) {
    subst->parent->left = temp;
  } else {
    subst->parent->right = temp;
  }
  if (subst == node) {
    temp->parent = subst->parent;
  } else {
    if (subst->parent == node) {
      temp->parent = subst;
    } else {
      temp->parent = subst->parent;
    }
    subst->left = node->left;
    subst->right = node->right;
    subst->parent = node->parent;
    cc_rbt_copy_color(subst, node);

    if (node == *root) {
      *root = subst;
    } else {
      if (node == node->parent->left) {
        node->parent->left = subst;
      } else {
        node->parent->right = subst;
      }
    }
    if (subst->left != sentinel) {
      subst->left->parent = subst;
    }
    if (subst->right != sentinel) {
      subst->right->parent = subst;
    }
  }

  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
  node->key = 0;
  if (red) {
    return;
  }

  while (temp != *root && cc_rbt_is_black(temp)) {
    if (temp == temp->parent->left) {
      w = temp->parent->right;
      if (cc_rbt_is_red(w)) {
        cc_rbt_black(w);
        cc_rbt_red(temp->parent);
        cc_rbtree_left_rotate(root, sentinel, temp->parent);
        w = temp->parent->right;
      }
      if (cc_rbt_is_black(w->left) && cc_rbt_is_black(w->right)) {
        cc_rbt_red(w);
        temp = temp->parent;
      } else {
        if (cc_rbt_is_black(w->right)) {
          cc_rbt_black(w->left);
          cc_rbt_red(w);
          cc_rbtree_right_rotate(root, sentinel, w);
          w = temp->parent->right;
        }
        cc_rbt_copy_color(w, temp->parent);
        cc_rbt_black(temp->parent);
        cc_rbt_black(w->right);
        cc_rbtree_left_rotate(root, sentinel, temp->parent);
        temp = *root;
      }

    } else {
      w = temp->parent->left;
      if (cc_rbt_is_red(w)) {
        cc_rbt_black(w);
        cc_rbt_red(temp->parent);
        cc_rbtree_right_rotate(root, sentinel, temp->parent);
        w = temp->parent->left;
      }
      if (cc_rbt_is_black(w->left) && cc_rbt_is_black(w->right)) {
        cc_rbt_red(w);
        temp = temp->parent;
      } else {
        if (cc_rbt_is_black(w->left)) {
          cc_rbt_black(w->right);
          cc_rbt_red(w);
          cc_rbtree_left_rotate(root, sentinel, w);
          w = temp->parent->left;
        }
        cc_rbt_copy_color(w, temp->parent);
        cc_rbt_black(temp->parent);
        cc_rbt_black(w->left);
        cc_rbtree_right_rotate(root, sentinel, temp->parent);
        temp = *root;
      }
    }
  }
  cc_rbt_black(temp);
}

void cc_rbtree_left_rotate(cc_rbtree_node_t **root, cc_rbtree_node_t *sentinel, cc_rbtree_node_t *node) {
  cc_rbtree_node_t *temp;
  temp = node->right;
  node->right = temp->left;
  if (temp->left != sentinel) {
    temp->left->parent = node;
  }
  temp->parent = node->parent;
  if (node == *root) {
    *root = temp;
  } else if (node == node->parent->left) {
    node->parent->left = temp;
  } else {
    node->parent->right = temp;
  }
  temp->left = node;
  node->parent = temp;
}

void cc_rbtree_right_rotate(cc_rbtree_node_t **root, cc_rbtree_node_t *sentinel, cc_rbtree_node_t *node) {
  cc_rbtree_node_t *temp;
  temp = node->left;
  node->left = temp->right;
  if (temp->right != sentinel) {
    temp->right->parent = node;
  }
  temp->parent = node->parent;
  if (node == *root) {
    *root = temp;
  } else if (node == node->parent->right) {
    node->parent->right = temp;
  } else {
    node->parent->left = temp;
  }
  temp->right = node;
  node->parent = temp;
}

cc_rbtree_node_t *cc_rbtree_next(cc_rbtree_t *tree, cc_rbtree_node_t *node) {
  cc_rbtree_node_t *root, *sentinel, *parent;
  sentinel = tree->sentinel;
  if (node->right != sentinel) {
    return cc_rbtree_min(node->right, sentinel);
  }
  root = tree->root;
  for (;;) {
    parent = node->parent;
    if (node == root) {
      return NULL;
    }
    if (node == parent->left) {
      return parent;
    }
    node = parent;
  }
}