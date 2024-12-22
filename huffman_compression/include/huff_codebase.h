#ifndef _HUFF_CODEBASE_H_
#define _HUFF_CODEBASE_H_

#include "binary_tree.h"
#include "huffman.h"
#include <stdint.h>

typedef struct s_codent {
  uint32_t data;
  uint32_t codelen;
  uint64_t code;  /// 64b for a code is probably overkill, but just in case
} CodeEntry_t;

typedef BST_t CodeBase_t;  // It's just a BST under the hood. Don't worry about it! 
                           // Just only call Huff_Codebase_XYZ(...) functions with it.
                           // Don't call any BST_XYZ(...) functions with it directly.
const char *Huff_Codebase_Strerror(void);
CodeBase_t *Huff_Codebase_Create(const HuffTree_t *tree);
CodeEntry_t *Huff_Codebase_Get_Code(CodeBase_t *codebase, uint32_t data);
void Huff_Codebase_Destroy(CodeBase_t *codebase);

#endif  /* _HUFF_CODEBASE_H_ */
