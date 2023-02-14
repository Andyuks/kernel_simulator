/* tlb.c */
#include <stdio.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

/* gets if a page traduction resides in the tlb*/
int isinTLB(struct tlb_t * tlb, unsigned int page){
    int i=0;
    int flag = 0;
    int frame = -1;
    while(flag==0 && i < tlb->maxentries)
    {
        //printf("tlb> ite %d page %X frame %X\n", i, tlb->entry[i].page, tlb->entry[i].frame );
        if(tlb->entry[i].page == page)
        {
            printf("\033[0;36mTLB>>\033[0m page %X frame %X\n",tlb->entry[i].page, tlb->entry[i].frame );
            frame = tlb->entry[i].frame;
            flag = 1;
        }
        else
            i++;
    }
    if(flag == 1) return frame;
    else return -1;
}

/* Add page number and frame number to TLB */
void add_to_TLB(struct tlb_t * tlb, unsigned int page, unsigned int frame){
	tlb->entry[tlb->index].page = page;
    tlb->entry[tlb->index].frame = frame;
    tlb->index = (tlb->index + 1) % tlb->maxentries; 
    printf("\033[0;36mTLB>>\033[0m Added to TLB! page %X frame %X\n", page, frame);
}

/* Clean TLB (change of context) */
void clean_TLB(struct tlb_t * tlb){
    int i;
    for (i = 0; i < tlb->maxentries; i++)
    {
        tlb->entry[i].page = -1;
        tlb->entry[i].frame = -1;
    }
    tlb->index=0;
}