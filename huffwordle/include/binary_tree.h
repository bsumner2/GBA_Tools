#ifndef _BINARY_TREE_H_
#define _BINARY_TREE_H_
#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#include <stdbool.h>
#endif  /* CXX Name mangler guard  */

typedef struct bst_node BST_Node_t;
typedef struct bst BST_t;
struct bst_node {
  void *data;
  BST_Node_t *l,*r;
  int height;
};
typedef int (*BST_Cmp_Cb_t)(const void*,const void*);
typedef void (*BST_Dealloc_Cb_t)(void*);
typedef void * (*BST_Alloc_Cb_t)(const void*);
/** 
 * @param comparison_cb   [REQUIRED] comparison callback data in node.
 * @param data_alloc_cb   [OPTIONAL] custom allocator callback to allocate data for new nodes
 * @param data_dealloc_cb [OPTIONAL] custom deallocator to call. If NULL, will assume data is static. If data is in heap, then just pass free if standard dealloc is fine.
 * @param data_size       \[[REQUIRED IFF want malloc'd memcpy of data AND data_alloc_cb IS NULL]\] If data_size is zero, then data added will be direct ptr value given to BST_Add.\
 *                        Otherwise, data added will be a malloc(data_size) ptr with memcpy(node->data, data, data_size) to transfer data from BST_Add.
 **/
BST_t *BST_Init(BST_Cmp_Cb_t comparison_cb, BST_Alloc_Cb_t data_alloc_cb, BST_Dealloc_Cb_t data_dealloc_cb, size_t data_size);
void BST_Add(BST_t *tree, void *data);
void BST_Remove(BST_t *tree, void *data);
void *BST_Remove_Minimum(BST_t *tree);

/**
 * @param tree [REQUIRED]
 * @param data [REQUIRED] key to retrieve data with.
 * @param custom_cmp_cb [OPTIONAL] If NULL, use BST's cmp callback. Otherwise use this param as cmp cb.
 **/
void *BST_Retrieve(BST_t *tree, void *data, BST_Cmp_Cb_t custom_cmp_cb);

bool BST_Contains(BST_t *tree, void *data, BST_Cmp_Cb_t custom_cmp_cb);
int BST_Element_Count(BST_t *tree);
/**
 * @summary Use for self-guided traversals. DO NOT edit nodes!! This will cause discrepancies such as discrepancies in tree's structure
 * or discrepancies between tree's internal node count field and actual node count.
 * @param tree [REQUIRED]
 * @param return_node_ct \[[OPTIONAL ; OUT]\] Returns amount of nodes in tree (from given root) ONLY IF return_node_ct is NOT NULL.
 * @return Tree's root node.
 **/
const BST_Node_t *BST_Get_Root(BST_t *tree, int *return_node_ct);
void BST_Close(BST_t *tree);

#ifdef __cplusplus
}
#endif  /*CXX Name Mangler Guard */

#endif  /* _BINARY_TREE_H_ */
