#include "binary_tree.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "
#define WARNING_PREFIX "\x1b[1;33m[Warning]:\x1b[0m "
#define errprintf(fmt, ...) fprintf(stderr, ERR_PREFIX fmt, __VA_ARGS__)
#define errputs(s) fputs(ERR_PREFIX s, stderr)
#define warnf(fmt, ...) fprintf(stderr, WARNING_PREFIX fmt, __VA_ARGS__)
#define warn(s) fputs(WARNING_PREFIX s, stderr)

struct bst {
  BST_Node_t *root;
  BST_Cmp_Cb_t cmp_cb;
  BST_Alloc_Cb_t alloc_cb;
  BST_Dealloc_Cb_t dealloc_cb;
  size_t data_len;
  int count;
} __attribute__ ((aligned(8)));

struct bst_info {
  BST_Cmp_Cb_t cmpcb;
  BST_Alloc_Cb_t alloccb;
  BST_Dealloc_Cb_t dealloc_cb;
  size_t len;
} __attribute__ ((aligned(8)));

#define NODE_HEIGHT(root) ((root!=NULL) ? ( root->height) : (-1))

#define NODE_BALANCE_FACT(root) ((root!=NULL) ? (NODE_HEIGHT(root->l) - NODE_HEIGHT(root->r)) : 0)


static void BST_Node_Recalc_Height(BST_Node_t *root) {
  int lh, rh;
  if (!root)
    return;
  lh = NODE_HEIGHT(root->l), rh = NODE_HEIGHT(root->r);
  root->height = 1 + ((lh > rh) ? lh : rh);
}

static BST_Node_t *BST_Node_LL_Rotate(BST_Node_t *root) {
  BST_Node_t *newroot = root->l;
  root->l = newroot->r;
  BST_Node_Recalc_Height(root);
  newroot->r = root;
  BST_Node_Recalc_Height(newroot);
  return newroot;
}

static BST_Node_t *BST_Node_LR_Rotate(BST_Node_t *root) {
  BST_Node_t *newroot = root->l->r;
  root->l->r = newroot->l;
  BST_Node_Recalc_Height(root->l);
  newroot->l = root->l;
  root->l = newroot->r;
  BST_Node_Recalc_Height(root);
  newroot->r = root;
  BST_Node_Recalc_Height(newroot);
  return newroot;
}

static BST_Node_t *BST_Node_RR_Rotate(BST_Node_t *root) {
  BST_Node_t *newroot = root->r;
  root->r = newroot->l;
  BST_Node_Recalc_Height(root);
  newroot->l = root;
  BST_Node_Recalc_Height(newroot);
  return newroot;
}

static BST_Node_t *BST_Node_RL_Rotate(BST_Node_t *root) {
  BST_Node_t *newroot = root->r->l;
  root->r->l = newroot->r;
  BST_Node_Recalc_Height(root->r);
  newroot->r = root->r;
  root->r = newroot->l;
  BST_Node_Recalc_Height(root);
  newroot->l = root;
  BST_Node_Recalc_Height(newroot);
  return newroot;
}

BST_t *BST_Init(BST_Cmp_Cb_t comparison_cb, BST_Alloc_Cb_t data_alloc_cb, BST_Dealloc_Cb_t data_dealloc_cb, size_t data_size) {
  if (NULL==comparison_cb) {
    return NULL;
  }
  BST_t *ret = malloc(sizeof(*ret));
  ret->cmp_cb = comparison_cb;
  ret->alloc_cb = data_alloc_cb;
  ret->dealloc_cb = data_dealloc_cb;
  ret->data_len = data_size;
  ret->root = NULL;
  ret->count = 0;
  return ret;
}



static BST_Node_t *Node_Add(const struct bst_info *const info,  BST_Node_t *root, void *data, bool *insert_occurred) {
  int diff;
  if (!root) {
    root = calloc(1, sizeof(*root));
    if (info->alloccb) {
      root->data = (info->alloccb)(data);
    } else if (info->len) {
      root->data = malloc(info->len);
      memcpy(root->data, data, info->len);
    } else {
      root->data = data;
    }
    *insert_occurred = true;
    return root;
  }
  diff = (info->cmpcb)(data, root->data);
  if (!diff) {
    *insert_occurred = false;
    return root;
  } else if (0 > diff) {
    root->l = Node_Add(info, root->l, data, insert_occurred);
  } else {
    root->r = Node_Add(info, root->r, data, insert_occurred);
  }

  if (!(*insert_occurred)) {
    return root;
  }
  
  diff = NODE_BALANCE_FACT(root);
  if (diff > 1) {
    if ((info->cmpcb)(data, root->l->data) < 0) {
      root = BST_Node_LL_Rotate(root);
    } else {
      root = BST_Node_LR_Rotate(root);
    }
  } else if (diff < -1) {
    if ((info->cmpcb)(data, root->r->data) > 0) {
      root = BST_Node_RR_Rotate(root);
    } else {
      root = BST_Node_RL_Rotate(root);
    }
  } else {
    BST_Node_Recalc_Height(root);
  }

  return root;
}

static void Node_Dealloc(BST_Node_t *node, BST_Dealloc_Cb_t deallocator_cb) {
  if (!node)
    return;
  if (node->l || node->r) {
#ifndef _SUPPRESS_BST_WARNINGS_
    warn("Dealloc called on a non-leaf node!\n");
#endif
  }
  if (NULL==deallocator_cb) {  // if dealloc callback is NULL then either the data
                               // is assumed be static (stack-based) or the responsibility
                               // of freeing the data from heap falls on the caller
    free(node);
    return;
  }

  deallocator_cb(node->data);
  free(node);
}

static BST_Node_t *Node_Hibbard_Rebal(BST_Node_t *root) {
  int bal = NODE_BALANCE_FACT(root);
  if (bal > 1) {
    if (NODE_BALANCE_FACT(root->l)>=0) {
      return BST_Node_LL_Rotate(root);
    } else {
      return BST_Node_LR_Rotate(root);
    }
  } else if (bal < -1) {
    if (NODE_BALANCE_FACT(root->r) <= 0) {
      return BST_Node_RR_Rotate(root);
    } else {
      return BST_Node_RL_Rotate(root);
    }
  } else {
    BST_Node_Recalc_Height(root);
    return root;
  }
}

static BST_Node_t *Node_Hibbard_Delete(BST_Node_t *root, BST_Dealloc_Cb_t dealloc_cb) {
  if (!root)
    return root;  // this should never happen, duh...
  BST_Node_t *stack[root->height+2];
  BST_Node_t *tmp;
  int top = -1;
  {
    BST_Node_t *smallest_rsub = root->r;
    void *dswap;
    while ((tmp=smallest_rsub->l)) {
      stack[++top] = smallest_rsub;
      smallest_rsub = tmp;
    }
    dswap = root->data;
    root->data = smallest_rsub->data;
    smallest_rsub->data = dswap;
    tmp = smallest_rsub->r;

#ifndef _SUPPRESS_BST_WARNINGS_
    smallest_rsub->r = NULL;
#endif
    Node_Dealloc(smallest_rsub, dealloc_cb);
  }
  while (top > -1) {
    stack[top]->l = tmp;
    tmp = Node_Hibbard_Rebal(stack[top--]);
  }

  root->r = tmp;
  return Node_Hibbard_Rebal(root);
}

static BST_Node_t *Node_Rm(const struct bst_info *const info, BST_Node_t *root, void *data, bool *remove_occurred) {
  BST_Node_t *tnode;
  int diff;
  if (!root) {
    *remove_occurred = false;
    return root;
  }
  diff = (info->cmpcb)(data, root->data);
  if (diff < 0) {
    root->l = Node_Rm(info, root->l, data, remove_occurred);
  } else if (diff > 0) {
    root->r = Node_Rm(info, root->r, data, remove_occurred);
  } else {
    *remove_occurred = true;
    // Hibbard deletion. Handle easy cases first:
    if (!(root->l)) {  // if no left child, just replace with right child
      tnode = root->r;
#ifndef _SUPPRESS_BST_WARNINGS_
      root->r = NULL;
#endif
      Node_Dealloc(root, info->dealloc_cb);
      return tnode;
    } else if (!(root->r)) {
      tnode = root->l;
#ifndef _SUPPRESS_BST_WARNINGS_
      root->l = NULL;
#endif
      Node_Dealloc(root, info->dealloc_cb);
      return tnode;
    } else {
      return Node_Hibbard_Delete(root, info->dealloc_cb);
    }
  }
  if ((!*remove_occurred))
    return root;
  return Node_Hibbard_Rebal(root);
}


void BST_Add(BST_t *tree, void *data) {
  bool tmp = false;
  tree->root = Node_Add(((struct bst_info*)(&tree->cmp_cb)), tree->root, data, &tmp);
  if (tmp)
    ++(tree->count);
}

void BST_Remove(BST_t *tree, void *data) {
  bool tmp = false;
  tree->root = Node_Rm((struct bst_info*)(&tree->cmp_cb), tree->root, data, &tmp);
  if (tmp)
    --(tree->count);
}

bool BST_Contains(BST_t *tree, void *data, BST_Cmp_Cb_t custom_cmp_cb) {
  BST_Node_t *root;
  int diff;
  if (!tree)
    return NULL;
  if (!(tree->root))
    return NULL;
  root = tree->root;
  if (custom_cmp_cb == NULL) {
    custom_cmp_cb = tree->cmp_cb;
  }
  while (root) {
    diff = custom_cmp_cb(data, root->data);
    if (!diff) {
      return true;
    } else if (diff < 0) {
      root = root->l;
    } else {
      root = root->r;
    }
  }

  return false;
}

static void Node_Free_Subtree(BST_Node_t *root, BST_Dealloc_Cb_t data_dealloc_cb) {
  if (!root)
    return;
  Node_Free_Subtree(root->l, data_dealloc_cb);
  Node_Free_Subtree(root->r, data_dealloc_cb);
#ifndef _SUPPRESS_BST_WARNINGS_
  root->l = root->r = NULL;
#endif
  Node_Dealloc(root, data_dealloc_cb);
}

void *BST_Remove_Minimum(BST_t *tree) {
  BST_Node_t *min;
  void *ret;
  if (!(tree->root))
    return NULL;
  if (tree->count == 1) {
    ret = tree->root->data;  // data being passed back to caller,
                             // so responsibility of dealloc'ing data 
                             // offloaded to caller.
#ifndef _SUPPRESS_BST_WARNINGS_
    if (tree->root->l || tree->root->r) {
      warnf("Tree count = 1 but tree root is not also a leaf.\n\t"
          "\x1b[1;31m(Still has %s).\x1b[0m\n",
          ((tree->root->l && tree->root->r) ? "left and right subtrees" : ((tree->root->l) ? "left subtree" : "right subtree")));
    }
#endif
    free(tree->root);
    tree->root = NULL;
    tree->count = 0;
    return ret;
  }
  min = tree->root;
  BST_Node_t *stack[min->height+2];
  int top = -1;
  while (min->l) {
    stack[++top] = min;
    min = min->l;
  }
  ret = min->data;
  if (min->r) {
    BST_Node_t *tmp = min->r;
    free(min);
    min = tmp;
  } else {
    free(min);
    min = NULL;
  }
  while (top > -1) {
    stack[top]->l = min;
    min = Node_Hibbard_Rebal(stack[top--]);
  }
  tree->root = min;
  --tree->count;
  return ret;
}

void BST_Close(BST_t *tree) {
  Node_Free_Subtree(tree->root, tree->dealloc_cb);
  free(tree);
}


int BST_Element_Count(BST_t *tree) {
  return tree->count;
}

void *BST_Retrieve(BST_t *tree, void *data, BST_Cmp_Cb_t custom_cmp_cb) {
  if (!tree)
    return NULL;
  if (!tree->root)
    return NULL;
  if (custom_cmp_cb == NULL)
    custom_cmp_cb = tree->cmp_cb;
  BST_Node_t *root = tree->root;
  int diff;
  while (root) {
    diff = custom_cmp_cb(data, root->data);
    if (!diff) {
      return root->data;
    } else if (diff < 0) {
      root = root->l;
    } else {
      root = root->r;
    }
  }

  return NULL;

}



const BST_Node_t *BST_Get_Root(BST_t *tree, int *return_node_ct) {
  if (!tree)
    return NULL;
  if (return_node_ct != NULL) {
    *return_node_ct = tree->count;
  }
  return tree->root;
}



