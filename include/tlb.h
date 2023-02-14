/* tlb.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */
int isinTLB(struct tlb_t * tlb, unsigned int page);
void add_to_TLB(struct tlb_t * tlb, unsigned int page, unsigned int frame);
void clean_TLB(struct tlb_t * tlb);