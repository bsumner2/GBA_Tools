#include "linked_list.h"

#ifndef _INT_LL_H_
#define _INT_LL_H_

LL_DECL(int, Int);

#define NEW_INT_LL() LL_INIT(Int)

void Int_LL_Enqueue(Int_LL_t *ll, int data);
int Int_LL_Dequeue(Int_LL_t *ll);
void Int_LL_Close(Int_LL_t *ll);

#endif  /* _INT_LL_H_ */

