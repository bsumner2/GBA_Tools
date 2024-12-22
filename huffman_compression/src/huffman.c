#include "huffman.h"
#include "binary_tree.h"
#include "huff_codebase.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum e_huffman_errno {
  HUFF_ERROR_NONE=0,
  HUFF_ERROR_UNSUPPORTED_FEATURE,
  HUFF_ERROR_INPUT_TOO_UNIFORM,
  HUFF_ERROR_INPUT_TOO_SHORT,
  HUFF_ERROR_SUBROOT_MISSING_DESCENDANT,
  HUFF_ERROR_FREQ_TREE_MISSING_NODES,
  HUFF_ERROR_UNEXPECTED_TREE_NODE_CT,
  HUFF_ERROR_DATA_GIVEN_IS_NULL,
  HUFF_ERROR_NO_TREE_SUPPLIED,
  HUFF_ERROR_CODEBASE_ERR,
  HUFF_ERROR_CODEBASE_MISSING_ENTRY,
  HUFF_ERROR_NO_HEADER_SUPPLIED,
  HUFF_ERROR_DATA_NOT_WORD_ALIGNABLE,
  HUFF_ERROR_GBA_TABLE_ENTRY_OFS_OVERFLOW,
}; 
static enum e_huffman_errno huff_errno=HUFF_ERROR_NONE;


#define WARN_PREFIX "\x1b[1;33m[Warning]:\x1b[0m "
#define warnf(fmt, ...) fprintf(stderr, WARN_PREFIX fmt, __VA_ARGS__)
#define warn(s) fputs(WARN_PREFIX s, stderr)

#define HUFF_ERR_CASE(enumval) case enumval: return #enumval

const char *Huff_Strerror(void) {
  switch (huff_errno) {
    case HUFF_ERROR_NONE: return "No error to report.";
    HUFF_ERR_CASE(HUFF_ERROR_UNSUPPORTED_FEATURE);
    HUFF_ERR_CASE(HUFF_ERROR_INPUT_TOO_UNIFORM);
    HUFF_ERR_CASE(HUFF_ERROR_INPUT_TOO_SHORT);
    HUFF_ERR_CASE(HUFF_ERROR_SUBROOT_MISSING_DESCENDANT);
    HUFF_ERR_CASE(HUFF_ERROR_FREQ_TREE_MISSING_NODES);
    HUFF_ERR_CASE(HUFF_ERROR_UNEXPECTED_TREE_NODE_CT);
    HUFF_ERR_CASE(HUFF_ERROR_DATA_GIVEN_IS_NULL);
    HUFF_ERR_CASE(HUFF_ERROR_NO_TREE_SUPPLIED);
    HUFF_ERR_CASE(HUFF_ERROR_CODEBASE_MISSING_ENTRY);
    HUFF_ERR_CASE(HUFF_ERROR_NO_HEADER_SUPPLIED);
    HUFF_ERR_CASE(HUFF_ERROR_DATA_NOT_WORD_ALIGNABLE);
    HUFF_ERR_CASE(HUFF_ERROR_GBA_TABLE_ENTRY_OFS_OVERFLOW);
    case HUFF_ERROR_CODEBASE_ERR: return Huff_Codebase_Strerror();
    default: return "Undefined error case.";
  }
}

static int Huff_Node_Cmp_Callback(const void *a, const void *b) {
  const HuffNode_t *t1 = a, *t2 = b;
  int diff=t1->freq-t2->freq;
  if (diff)
    return diff;
  // 0 > diff ? t1->dlen is minimum : t2->dlen is minimum
  diff = t1->dlen - t2->dlen;
  // given the way these huff trees nest, there should never be any erroneous,
  // cmp:=0 collisions from comparing data subroots, 
  // so just defaulting to returning a memcmp of nodes' data
  // streams won't be a problem
  return memcmp(t1->data, t2->data, 
      (0 > diff ? t1->dlen : t2->dlen)*sizeof(*(t1->data)));
}


#define MINIMUM(a,b) ((a < b) ? a : b)
#define MAXIMUM(a,b) ((a > b) ? a : b)

static HuffNode_t *Huff_Node_Create_Subroot(HuffNode_t *l, HuffNode_t *r) {
  // node checklist: [X] data ; [X] data len ; [X] height
  // [X] freq ; [X] descendants
  HuffNode_t *ret;
  byte *buf;
  if (!l || !r) {
    huff_errno = HUFF_ERROR_SUBROOT_MISSING_DESCENDANT;
    return NULL;
  }
  
  // allocate
  ret = malloc(sizeof(*ret));
  
  // set descendant nodes
  int lh, rh;
  ret->l = l;
  ret->r = r;
 
  // set height
  lh = HUFF_NODE_HEIGHT(l), rh = HUFF_NODE_HEIGHT(r);
  ret->height = MAXIMUM(lh,rh) + 1;
  
  // set data len
  lh = l->dlen, rh = r->dlen;
  ret->dlen = lh+rh;
  
  // set data to concat(l->data, r->data)
  buf = ret->data = malloc(sizeof(*(ret->data))*((ret->dlen) + 1));
  memcpy(buf, l->data, lh);
  memcpy(&buf[lh], r->data, rh);
  
  // null-terminate data
  buf[ret->dlen] = '\0';

  // set freq combined freq of child nodes
  ret->freq = l->freq + r->freq;
  return ret;

}

static HuffNode_t *Huff_Node_Create_Leaf(int data, int freq) {
  // node checklist: [X] data ; [X] data len ; [X (set to 0 by calloc)] height
  // [X] freq ; [X (set to NULL by calloc)] descendants
  HuffNode_t *ret = calloc(1, sizeof(*ret));
  // calloc with 2 for arg0 because HuffNode_t.data should always be
  // null-terminated
  ret->data = calloc(2, sizeof(*(ret->data)));
  ret->data[0] = data;
  ret->dlen = 1;
  ret->freq = freq;
  return ret;

}

void Huff_Node_Dealloc(HuffNode_t *root) {
  if (!root)
    return;
  HuffNode_t *stack[1<<(root->height+1)];
  int top = -1;
  stack[++top] = root;
  do {
    root = stack[top--];
    if (!root) {
      continue;
    } else if (HUFF_NODE_IS_LEAF(root)) {
      free(root->data);
      free(root);
      continue;
    }
    free(root->data);
    stack[++top] = root->l;
    stack[++top] = root->r;
    free(root);
  } while (top > -1);
}

static void Huff_Node_BST_Dealloc_Callback(void *vp) {
  if (!vp)
    return;
  Huff_Node_Dealloc(vp);
}

static int Huff_Node_Get_Subroot_Node_Ct(HuffNode_t *root) {
  if (!root)
    return 0;
  HuffNode_t *stack[1<<(root->height+1)];
  int top = -1, ret=0;
  stack[++top] = root;
  do {
    root = stack[top--];
    if (!root) {
      continue;
    }
    stack[++top] = root->l;
    stack[++top] = root->r;
    ++ret;
  } while (top > -1);

  return ret;

}


static int Huff_Tree_Fill_4b(HuffTree_t *dst, const byte *data, int byte_ct) {
  int freq[16] = {0}, unique_vals[16] = {0};
  int unique_ct=0;
  byte currval, currpair;
  do {
    currpair = *data++;
    currval = currpair&15;
    if (!(freq[currval]++)) {
      unique_vals[unique_ct++] = currval;
    }
    currval = (currpair>>4)&15;
    if (!(freq[currval]++)) {
      unique_vals[unique_ct++] = currval;
    }

  } while (--byte_ct);

  if (unique_ct < 2) {
    huff_errno = HUFF_ERROR_INPUT_TOO_UNIFORM;
    return -1;
  }

  // NULL for alloc arg makes BST just insert data ptr passed to BST_Add calls
  // directly. Pass deallocator for proper deallocations when BST_Close is 
  // called during failure cases. Otherwise, if function is to return 
  // successfully, BST_Close will be called on empty tree.
  BST_t *valfreq_tree = BST_Init(Huff_Node_Cmp_Callback, NULL, 
      Huff_Node_BST_Dealloc_Callback, 0ULL);
  HuffNode_t *insert;
  for (int i = 0; i < unique_ct; ++i) {
    currval = unique_vals[i];
    insert = Huff_Node_Create_Leaf(currval, freq[currval]);
    if (!insert) {
      // no need to set errno. already set by create_leaf
      BST_Close(valfreq_tree);
      return -1;
    }
    BST_Add(valfreq_tree, insert);
  }
  if (BST_Element_Count(valfreq_tree) != unique_ct) {
    huff_errno = HUFF_ERROR_FREQ_TREE_MISSING_NODES;
    BST_Close(valfreq_tree);
    return -1;
  }
  HuffNode_t *node_1st_least, *node_2nd_least;
  int node_ct = unique_ct;  // node ct so far = count of huff tree's leaf nodes
  do {
    node_1st_least = BST_Remove_Minimum(valfreq_tree);
    node_2nd_least = BST_Remove_Minimum(valfreq_tree);
    // Given previous checks, we definitely know subroot created won't be NULL
    // so no need to check; can just pass it to BST_Add directly
    BST_Add(valfreq_tree, 
        Huff_Node_Create_Subroot(node_1st_least, node_2nd_least));
    // every time we nest existing nodes, we add 1 node to final resulting
    // hufftree, so increment node_ct with each loop.
    ++node_ct;
  } while (BST_Element_Count(valfreq_tree) > 1);
  
  if (!BST_Element_Count(valfreq_tree)) {
    huff_errno = HUFF_ERROR_FREQ_TREE_MISSING_NODES;
    BST_Close(valfreq_tree);
    return -1;
  }

  dst->root = BST_Remove_Minimum(valfreq_tree);
  
  BST_Close(valfreq_tree);

  if (node_ct != Huff_Node_Get_Subroot_Node_Ct(dst->root)) {
    huff_errno = HUFF_ERROR_UNEXPECTED_TREE_NODE_CT;
    Huff_Node_Dealloc(dst->root);
    dst->root = NULL;
    return -1;
  }

  dst->node_ct = node_ct;
  dst->data_unit_bitlen = E_DATA_UNIT_4_BITS;
  dst->leaf_ct = unique_ct;
  return 0;
}




static int Huff_Tree_Fill_8b(HuffTree_t *dst, const byte *data, int byte_ct) {
  int freq[256] = {0}, unique_vals[256] = {0};
  int unique_ct=0;
  byte currval;
  do {
    currval = *data++;
    if (!(freq[currval]++)) {
      unique_vals[unique_ct++] = currval;
    }
  } while (--byte_ct);

  if (unique_ct < 2) {
    huff_errno = HUFF_ERROR_INPUT_TOO_UNIFORM;
    return -1;
  }

  // NULL for alloc arg makes BST just insert data ptr passed to BST_Add calls
  // directly. Pass deallocator for proper deallocations when BST_Close is 
  // called during failure cases. Otherwise, if function is to return 
  // successfully, BST_Close will be called on empty tree.
  BST_t *valfreq_tree = BST_Init(Huff_Node_Cmp_Callback, NULL, 
      Huff_Node_BST_Dealloc_Callback, 0ULL);
  HuffNode_t *insert;
  for (int i = 0; i < unique_ct; ++i) {
    currval = unique_vals[i];
    insert = Huff_Node_Create_Leaf(currval, freq[currval]);
    if (!insert) {
      // no need to set errno. already set by create_leaf
      BST_Close(valfreq_tree);
      return -1;
    }
    BST_Add(valfreq_tree, insert);
  }
  if (BST_Element_Count(valfreq_tree) != unique_ct) {
    huff_errno = HUFF_ERROR_FREQ_TREE_MISSING_NODES;
    BST_Close(valfreq_tree);
    return -1;
  }
  HuffNode_t *node_1st_least, *node_2nd_least;
  int node_ct = unique_ct;  // node ct so far = count of huff tree's leaf nodes
  do {
    node_1st_least = BST_Remove_Minimum(valfreq_tree);
    node_2nd_least = BST_Remove_Minimum(valfreq_tree);
    // Given previous checks, we definitely know subroot created won't be NULL
    // so no need to check; can just pass it to BST_Add directly
    BST_Add(valfreq_tree, 
        Huff_Node_Create_Subroot(node_1st_least, node_2nd_least));
    // every time we nest existing nodes, we add 1 node to final resulting
    // hufftree, so increment node_ct with each loop.
    ++node_ct;
  } while (BST_Element_Count(valfreq_tree) > 1);
  
  if (!BST_Element_Count(valfreq_tree)) {
    huff_errno = HUFF_ERROR_FREQ_TREE_MISSING_NODES;
    BST_Close(valfreq_tree);
    return -1;
  }

  dst->root = BST_Remove_Minimum(valfreq_tree);
  
  BST_Close(valfreq_tree);

  if (node_ct != Huff_Node_Get_Subroot_Node_Ct(dst->root)) {
    huff_errno = HUFF_ERROR_UNEXPECTED_TREE_NODE_CT;
    Huff_Node_Dealloc(dst->root);
    dst->root = NULL;
    return -1;
  }

  dst->node_ct = node_ct;
  dst->data_unit_bitlen = E_DATA_UNIT_8_BITS;
  dst->leaf_ct = unique_ct;
  return 0;
}

// Even 8 is kind of ridiculously small tho tbh. We'll be generous here.
#define DATA_LEN_MINIMUM 8


HuffTree_t *Huff_Tree_Create(const void *data, int word_ct, DataSize_e edata_unit_bit_len) {
  HuffTree_t *ret=NULL;
  if (!data) {
    huff_errno = HUFF_ERROR_DATA_GIVEN_IS_NULL;
    return NULL;
  }
  int outcome;
  if (word_ct < DATA_LEN_MINIMUM) {
    huff_errno = HUFF_ERROR_INPUT_TOO_SHORT;
    return NULL;

  }

  switch (edata_unit_bit_len) {
  case E_DATA_UNIT_4_BITS:
    outcome = Huff_Tree_Fill_4b(ret=calloc(1,sizeof(*ret)), data, word_ct*4);
    break;
  case E_DATA_UNIT_8_BITS:
    outcome = Huff_Tree_Fill_8b(ret=calloc(1,sizeof(*ret)), data, word_ct*4);
    break;
  default:
    huff_errno = HUFF_ERROR_UNSUPPORTED_FEATURE;
    return NULL;
  }
  if (0 > outcome) {
    // if we're here, huff_errno will have already been set by Huff_Tree_Fill_8b
    free(ret);
    return NULL;
  }
  return ret;
}

void Huff_Tree_Destroy(HuffTree_t *tree) {
  if (!tree)
    return;
  Huff_Node_Dealloc(tree->root);
  free(tree);
}

static uint32_t *Huff_Compress_4B(const byte *data, HuffTree_t *tree, 
    int byte_ct, int *return_word_ct) {
  CodeBase_t *codebase = Huff_Codebase_Create(tree);
  if (!codebase) {
    huff_errno = HUFF_ERROR_CODEBASE_ERR;
    *return_word_ct = 0;
    return NULL;
  }
  int max_words = (tree->root->height+1)*(byte_ct*2);
  if (31&max_words) {
    max_words/=32;
    ++max_words;
  } else {
    max_words/=32;
  }
  uint64_t code;
  uint32_t *ret, sbuf[max_words], *bufcursor = sbuf, curr_word=0, word_ct=0, 
           tmp, tmpcode;
  int curr_b=31, codelen=0;
  CodeEntry_t *codent = NULL;
  byte currpair, currval, sides_done;
  memset(sbuf, 0, sizeof(sbuf));
  do {
    currpair = *data++;
    for (currval = 15&currpair, sides_done = 0; 
        sides_done^3;
        currval = (currpair>>4)&15, sides_done |= (sides_done&1) ? 2 : 1) {
      
      codent = Huff_Codebase_Get_Code(codebase, currval);
      if (!codent) {
        Huff_Codebase_Destroy(codebase);
        huff_errno = HUFF_ERROR_CODEBASE_MISSING_ENTRY;
        *return_word_ct = 0;
        return NULL;
      }
      code = codent->code;
      codelen = codent->codelen;
      while ((curr_b - codelen) < -1) {
        tmp = codelen;
        codelen -= curr_b;
        tmp -= --codelen;
        tmpcode = code>>codelen;  // shift out all code bits that won't be in 
                                  // this byte, but the next byte.
        for (int i = tmp - 1; i > -1; --i)
          if (tmpcode&(1<<i))
            curr_word |= (1<<curr_b--);
          else
            curr_word &= ~(1<<curr_b--);
 
        curr_b = 31;
        *bufcursor++ = curr_word;
        curr_word = 0;
        ++word_ct;
      }
 
      for (int i = codelen - 1; i > -1; --i)
        if (code&(1<<i))
          curr_word |= (1<<curr_b--);
        else
         curr_word &= ~(1<<curr_b--);
    }
  } while (--byte_ct);

  if (bufcursor < (&sbuf[max_words]) && curr_b != 31) {
    *bufcursor++ = curr_word;
    ++word_ct;
  }
  ret = malloc(sizeof(*ret)*word_ct);
  memcpy(ret, sbuf, sizeof(*ret)*word_ct);
  *return_word_ct = word_ct;
  Huff_Codebase_Destroy(codebase);
  return ret;

}


static uint32_t *Huff_Compress_8B(const byte *data, HuffTree_t *tree, 
    int byte_ct, int *return_word_ct) {
  CodeBase_t *codebase = Huff_Codebase_Create(tree);
  if (!codebase) {
    huff_errno = HUFF_ERROR_CODEBASE_ERR;
    *return_word_ct = 0;
    return NULL;
  }
  int max_words = (tree->root->height+1)*(byte_ct);
  if (31&max_words) {
    max_words/=32;
    ++max_words;
  } else {
    max_words/=32;
  }
  uint64_t code;
  uint32_t *ret, sbuf[max_words], *bufcursor = sbuf, curr_word=0, word_ct=0, 
           tmp, tmpcode;
  int curr_b=31, codelen=0;
  CodeEntry_t *codent = NULL;
  memset(sbuf, 0, sizeof(sbuf));
  do {
    codent = Huff_Codebase_Get_Code(codebase, *data++);
    if (!codent) {
      Huff_Codebase_Destroy(codebase);
      huff_errno = HUFF_ERROR_CODEBASE_MISSING_ENTRY;
      *return_word_ct = 0;
      return NULL;
    }
    code = codent->code;
    codelen = codent->codelen;
    while ((curr_b - codelen) < -1) {
      tmp = codelen;
      codelen -= curr_b;
      tmp -= --codelen;
      tmpcode = code>>codelen;  // shift out all code bits that won't be in 
                                // this byte, but the next byte.
      for (int i = tmp - 1; i > -1; --i)
        if (tmpcode&(1<<i))
          curr_word |= (1<<curr_b--);
        else
          curr_word &= ~(1<<curr_b--);

      curr_b = 31;
      *bufcursor++ = curr_word;
      curr_word = 0;
      ++word_ct;
    }

    for (int i = codelen - 1; i > -1; --i)
      if (code&(1<<i))
        curr_word |= (1<<curr_b--);
      else
       curr_word &= ~(1<<curr_b--);
  } while (--byte_ct);

  if (bufcursor < (&sbuf[max_words]) && curr_b != 31) {
    *bufcursor++ = curr_word;
    ++word_ct;
  }
  ret = malloc(sizeof(*ret)*word_ct);
  memcpy(ret, sbuf, sizeof(*ret)*word_ct);
  *return_word_ct = word_ct;
  Huff_Codebase_Destroy(codebase);
  return ret;

}

uint32_t *Huff_Compress(const void *data, HuffTree_t *hufftree, int word_ct, 
    int *return_word_ct) {
  if (!data) {
    huff_errno = HUFF_ERROR_DATA_GIVEN_IS_NULL;
    *return_word_ct = 0;
    return NULL;
  }

  if (word_ct < DATA_LEN_MINIMUM) {
    huff_errno = HUFF_ERROR_INPUT_TOO_SHORT;
    *return_word_ct = 0;
    return NULL;
  }
  
  if (!hufftree) {
    huff_errno = HUFF_ERROR_NO_TREE_SUPPLIED;
    *return_word_ct = 0;
    return NULL;
  }
  uint32_t *ret = NULL;
  switch (hufftree->data_unit_bitlen) {
  case E_DATA_UNIT_4_BITS:
    ret = Huff_Compress_4B(data, hufftree, word_ct*4, return_word_ct);
    break;
  case E_DATA_UNIT_8_BITS:
    ret = Huff_Compress_8B(data, hufftree, word_ct*4, return_word_ct);
    break;
  default:
    huff_errno = HUFF_ERROR_UNSUPPORTED_FEATURE;
    break;
  }
  if (ret==NULL) {
    // if here, then huff_errno must have been set by a called function in
    // switch statement
    *return_word_ct = 0;
    return NULL;
  }

  return ret;
}


#define HUFF_GBA_NODE_OFS(cur, next) \
  ((((uintptr_t)next) - 2 - (((uintptr_t)cur) & (~1ULL))) / 2) 

#define soft_assert(expr) do if (!(expr)) {\
    fputs("\x1b[1;32m[Warning]:\x1b[2;34m Soft assertion, \x1b[1;33m\"" #expr "\",\x1b[2;31m failed.\x1b[0m\n", stderr); \
  } while (0)

#define UNITS_MASK(unit_bit_ct) (((1<<(unit_bit_ct))-1))



HuffNode_GBA_t *Huff_GBA_Huff_Table_Create(const HuffTree_t *tree, 
    int *return_table_size) {
  const HuffNode_t *root = tree->root;
  HuffNode_GBA_t *ret, *cur, *next_free;
  const int datamask = UNITS_MASK(tree->data_unit_bitlen);
  int size;
  *return_table_size = 0;

  if (datamask > 255) {
    // TODO: Add support for data units larger than byte
    huff_errno = HUFF_ERROR_UNSUPPORTED_FEATURE;
    return NULL;
  }
  if (!tree) {
    huff_errno = HUFF_ERROR_NO_TREE_SUPPLIED;
    return NULL;
  }
    
  size = tree->node_ct + 1;
  if (size < 4) {
    huff_errno = HUFF_ERROR_UNEXPECTED_TREE_NODE_CT;
    return NULL;
  }
  if (size&3) {
    // aka if size will cause compressed data portion to dealign from word 
    // boundary, then pad it so that size is word-alignable
    size &= ~3;  // round down to nearest multiple of four.
    size += 4;  // then add 4, essentially doing a base 4 ceil on size.
  }
  ret = malloc(sizeof(*ret)*size);
  ret[0].leaf = size/2-1;  // first entry in table is, in reality, is not really 
                           // a hufftree leaf, but, in fact, just another 
                           // header field, which is equal to a mathematical 
                           // expression, ofs/2 - 1. In this expr, the var,
                           // ofs := the offset of the compressed bitstream from
                           // just after huffman header, which is exactly equal
                           // to size of data region malloc'd for ret.
  cur = &ret[1], next_free = &ret[2];
  cur->subroot = (HuffSubroot_GBA_t) { 
    .descendants_ofs = HUFF_GBA_NODE_OFS(cur, next_free),
    .r_is_leaf = HUFF_NODE_IS_LEAF(tree->root->r),
    .l_is_leaf = HUFF_NODE_IS_LEAF(tree->root->l)
  };
  {
    const HuffNode_t *in_stack[tree->node_ct+1];
    HuffNode_GBA_t *out_stack[tree->node_ct+1];
    int top = -1;

    in_stack[++top] = root->l;
    out_stack[top] = next_free++;
    in_stack[++top] = root->r;
    out_stack[top] = next_free++;
    do {
      root = in_stack[top];
      cur = out_stack[top--];
      if (HUFF_NODE_IS_LEAF(root)) {
        cur->leaf = root->data[0]&datamask;
        continue;
      }
      cur->subroot = (HuffSubroot_GBA_t) {
        .descendants_ofs = HUFF_GBA_NODE_OFS(cur, next_free),
        .r_is_leaf = HUFF_NODE_IS_LEAF(root->r),
        .l_is_leaf = HUFF_NODE_IS_LEAF(root->l),
      };
      if (cur->subroot.descendants_ofs != HUFF_GBA_NODE_OFS(cur, next_free)) {
        huff_errno = HUFF_ERROR_GBA_TABLE_ENTRY_OFS_OVERFLOW;
        free(ret);
        return NULL;
      }
      in_stack[++top] = root->l;
      out_stack[top] = next_free++;
      in_stack[++top] = root->r;
      out_stack[top] = next_free++;
    } while (top > -1);
  }
  
  *return_table_size = size;
  return ret;
}

#define HUFF_HEADER_GBA_COMPRESSION_TYPE_ID 0x02
int Huff_GBA_Header_Init(HuffHeader_GBA_t *dst, int decompressed_data_bytelen,
    DataSize_e data_unit_bitlen) {
  if (!dst) {
    huff_errno = HUFF_ERROR_NO_HEADER_SUPPLIED;
    return -1;
  }

  if (decompressed_data_bytelen&3) {
    huff_errno = HUFF_ERROR_DATA_NOT_WORD_ALIGNABLE;
    return -1;
  }

  switch (data_unit_bitlen) {
  case E_DATA_UNIT_4_BITS:
  case E_DATA_UNIT_8_BITS:
    break;
  default:
    huff_errno = HUFF_ERROR_UNSUPPORTED_FEATURE;
    return -1;
  }

  dst->data_unit_bitlen = data_unit_bitlen;
  dst->id_reserved = HUFF_HEADER_GBA_COMPRESSION_TYPE_ID;
  dst->decomp_data_size = decompressed_data_bytelen;
  return 0;
}
