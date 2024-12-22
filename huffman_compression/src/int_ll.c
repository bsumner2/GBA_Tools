#include <assert.h>
#include "int_ll.h"
#include "linked_list.h"

void Int_LL_Enqueue(Int_LL_t *ll, int data) {
  LL_NODE_APPEND(Int, ll, data);
}

int Int_LL_Dequeue(Int_LL_t *ll) {
  int ret = 0;
  LL_HEAD_RM(Int, ll, ret);
  return ret;
}

void Int_LL_Close(Int_LL_t *ll) {
  LL_CLOSE(Int, ll);
}




