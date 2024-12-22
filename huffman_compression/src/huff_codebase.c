#include "huff_codebase.h"
#include "binary_tree.h"
#include "huffman.h"
#include <stdlib.h>

enum e_huff_codebase_errno {
  HUFF_CODEBASE_ERROR_NONE=0,
  HUFF_CODEBASE_ERROR_MAX_CODELEN_REACHED,
  HUFF_CODEBASE_ERROR_BAD_HUFFTREE_GIVEN,
  HUFF_CODEBASE_ERROR_ALLOCATION_FAILED
};
static enum e_huff_codebase_errno huff_cberrno = HUFF_CODEBASE_ERROR_NONE;

#define ERRCASE(c) case c: return #c
const char *Huff_Codebase_Strerror(void) {
  switch (huff_cberrno) {
    case HUFF_CODEBASE_ERROR_NONE: 
      return "No error to report.";
    ERRCASE(HUFF_CODEBASE_ERROR_MAX_CODELEN_REACHED);
    ERRCASE(HUFF_CODEBASE_ERROR_BAD_HUFFTREE_GIVEN);
    ERRCASE(HUFF_CODEBASE_ERROR_ALLOCATION_FAILED);
    default: 
      return "Undefined state.";
  }
}

static int Data_Against_Codent_Cmp_Cb(const void *data, const void *codent) {
  return (*((const uint32_t*) data)) - (((const CodeEntry_t*) codent)->data);
}

static int Codent_Cmp_Cb(const void *a, const void *b) {
  const CodeEntry_t *c0=a, *c1=b;
  return (c0->data - c1->data);
}
#define warnf(fmt, ...) fprintf(stderr, WARN_PREFIX fmt, __VA_ARGS__)
#define warn(s) fputs(WARN_PREFIX s, stderr)
#define soft_assertf(expr, fmt, ...) do { \
    if (!expr) { \
      warn("Soft assertion for expression, " #expr "failed.\n"); \
      fprintf(stderr, fmt, __VA_ARGS__); \
    } \
  } while (0)

void Huff_Codebase_Fill(CodeBase_t *dst, HuffNode_t *root, 
    CodeEntry_t *const codent) {
  if (HUFF_NODE_IS_LEAF(root)) {
    codent->data = root->data[0];
    BST_Add(dst, codent);
    return;
  }
  ++(codent->codelen);
  if (1&(codent->code<<=1))
    codent->code &= ~1;
  Huff_Codebase_Fill(dst, root->l, codent);
  codent->code |= 1;
  Huff_Codebase_Fill(dst, root->r, codent);
  --(codent->codelen);
  codent->code>>=1;
}

CodeBase_t *Huff_Codebase_Create(const HuffTree_t *tree) {
  CodeEntry_t tmp = {0};
  CodeBase_t *ret;
  huff_cberrno = HUFF_CODEBASE_ERROR_BAD_HUFFTREE_GIVEN;
  if (!tree)
    return NULL;
  if (!tree->root)
    return NULL;
  if (HUFF_NODE_IS_LEAF(tree->root))
    return NULL;
  huff_cberrno = HUFF_CODEBASE_ERROR_NONE;
  ret = BST_Init(Codent_Cmp_Cb, NULL, free, sizeof(tmp));
  if (!ret) {
    huff_cberrno = HUFF_CODEBASE_ERROR_ALLOCATION_FAILED;
    return NULL;
  }
  if ((1+tree->root->height) > 64) {
    BST_Close(ret);
    huff_cberrno = HUFF_CODEBASE_ERROR_MAX_CODELEN_REACHED;
    return NULL;
  }
  Huff_Codebase_Fill(ret, tree->root, &tmp);
  return ret;
}

CodeEntry_t *Huff_Codebase_Get_Code(CodeBase_t *codebase, uint32_t data) {
  if (!codebase)
    return NULL;
  return BST_Retrieve(codebase, &data, Data_Against_Codent_Cmp_Cb);
}


void Huff_Codebase_Destroy(CodeBase_t *codebase) {
  if (!codebase)
    return;
  BST_Close(codebase);
}
