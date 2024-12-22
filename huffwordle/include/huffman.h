#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include <stdbool.h>
#include <stdint.h>


typedef uint8_t byte;
typedef enum e_data_size {
  E_DATA_UNIT_4_BITS=4,
  E_DATA_UNIT_8_BITS=8,
  E_DATA_UNIT_VALIDITY_MASK=12
} __attribute__ ((aligned(4))) DataSize_e;

typedef struct s_htnode {
  byte *data;  /// Should be null-terminated field.
  int dlen;
  int height;
  int freq;
  struct s_htnode *l, *r;  /// l corresponds to 0 and r to 1
} HuffNode_t;

typedef struct s_ht {
  HuffNode_t *root;
  int node_ct;
  int leaf_ct;
  DataSize_e data_unit_bitlen;
} HuffTree_t;

typedef struct s_hsr_gba {
  uint8_t descendants_ofs : 6;
  bool r_is_leaf : 1;
  bool l_is_leaf : 1;
} __attribute__ ((packed)) HuffSubroot_GBA_t;

typedef union u_ht_gba {
  HuffSubroot_GBA_t subroot;
  uint8_t leaf;
} __attribute__ ((packed)) HuffNode_GBA_t;

typedef struct s_hhdr_gba {
  DataSize_e data_unit_bitlen: 4;
  int id_reserved : 4;
  int decomp_data_size : 24;
} __attribute__ (( packed )) HuffHeader_GBA_t;


#define HUFF_NODE_IS_LEAF(node) ((node->l==NULL) && (node->r==NULL))
#define HUFF_NODE_HEIGHT(node) (node ? node->height : -1)

const char *Huff_Strerror(void);
void Huff_Node_Dealloc(HuffNode_t *node);
/**
 * @summary Create huff tree. Data on GBA gets decompressed in 32-byte chunks, so
 * make sure the data passed to this function has a byte count that's divisible by 4,
 * hence why param 2 is word_ct and not byte_ct.
 * */
HuffTree_t *Huff_Tree_Create(const void *data, int word_ct, DataSize_e edata_unit_bit_len);
void Huff_Tree_Destroy(HuffTree_t *tree);
uint32_t *Huff_Compress(const void *data, HuffTree_t *hufftree, int word_ct, int *return_word_ct);

int Huff_GBA_Header_Init(HuffHeader_GBA_t *dst, int decompressed_data_bytelen,
    DataSize_e data_unit_bitlen);
HuffNode_GBA_t *Huff_GBA_Huff_Table_Create(const HuffTree_t *tree, 
    int *return_table_size);


#endif  /* _HUFFMAN_H_ */
