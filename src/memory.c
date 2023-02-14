/* memory.c */
#include <stdlib.h>
#include <stdio.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

/* provides the content of a memory position */
int get_content(struct memory_t * memory, int pos)
{
	int word; //4B
	word =          memory->data[pos + 3];
	word = word |  (memory->data[pos + 2] << 8); 
	word = word |  (memory->data[pos + 1] << 16);
	word = word |  (memory->data[pos + 0] << 24);
    return word;
}

/* sets the content of a memory position */
void set_content(struct memory_t * memory, int pos, int value)
{
	memory->data[pos + 3] = (value & 0x000000FF);
	memory->data[pos + 2] = ((value & 0x0000FF00) >> 8 );
	memory->data[pos + 1] = ((value & 0x00FF0000) >> 16);
	memory->data[pos + 0] = ((value & 0xFF000000) >> 24);
    //printf("Set data %X %X %X %X from pos %X on\n", memory->data[pos], memory->data[pos+1], memory->data[pos+2], memory->data[pos+3], pos);
}


/* add gap to gap queue: only gap of gap queue */
void gq_addonly(struct gapq_t * gapq, struct gap_t * gap)
{
    gapq->last = gap;
    gapq->first = gap;
    gapq->last->next = NULL;
    gapq->first->prev = NULL;
    printf("\033[0;36madd_gap>>\033[0m Gap (length %X) added (only one)\n", gap->length);
}


/* add gap to gap queue: beginning of gap queue */
void gq_addbeg(struct gapq_t * gapq, struct gap_t * gap)
{
    gap->next = gapq->first;
    gapq->first->prev = gap;
    gapq->first = gap;
    printf("\033[0;36madd_gap>>\033[0m Gap (length %X) added to queue (first pos)\n", gap->length);
}

/* add gap to gap queue: end of gap queue */
void gq_addlast(struct gapq_t * gapq, struct gap_t * gap)
{
    gapq->last->next = gap; // ... last -> gap
    gap->prev = gapq->last; // ... last <-> gap 
    gap->next = NULL; // ... prev <-> last -> null
    gapq->last = gap; // ... prev <-> last
    printf("\033[0;36madd_gap>>\033[0m  Gap (length %X) added to queue (last pos)\n", gap->length);
}


/* add gap to gap queue: middle of gap queue */
void gq_addmid(struct gap_t * aux, struct gap_t * gap)
{
    gap->next = aux->next;
    gap->prev = aux;
    aux->next->prev = gap;
    aux->next = gap;
    printf("\033[0;36madd_gap>>\033[0m Gap (length %X) added to queue (mid pos)\n", gap->length);
}


/* add gap to the gap queue: best-fit policy */
void pg_addgap (struct gapq_t * gapq, struct gap_t * gap){
    struct gap_t * aux;

    if (gapq->last == NULL) /* case first */
    {
        gq_addonly(gapq, gap);
        
    }
    else /* not first */
    {     

        if(gap->length < gapq->first->length) /* case first */
        {
            gq_addbeg(gapq, gap);
        }
        else if (gap->length >= gapq->last->length)/* case last */
        {
            gq_addlast(gapq, gap);
        }
        else /* case middle */
        {
            aux = gapq->first;
            
            while (aux->next!= NULL && aux->next->length<=gap->length)
            {
                aux = aux->next;
            }
            gq_addmid(aux, gap);
        }

    }
    printf("\033[0;36madd_gap>>\033[0mGap added to queue\n");
}

/* delete gap from the gap queue */
void pg_delgap(struct gapq_t * gapq, struct gap_t * gap)
{
    if (gap->dir == gapq->last->dir && gap->dir == gapq->first->dir)  /* case 1 elem */
    {
        gapq->first = NULL;
        gapq->last = NULL;
    }
    else if (gap->dir == gapq->last->dir) /* last elem, not first */
    {
        gapq->last = gap->prev;
        gapq->last->next = NULL;
    } 
    else if (gap->dir == gapq->first->dir) /* first elem, not last */
    {
        gapq->first = gap->next;
        gapq->first->prev = NULL;
    }else  /* middle */
    {
        gap->next->prev = gap->prev;
        gap->prev->next = gap->next;
    }
    printf("\033[0;36mdel_gap>>\033[0m gap  (%d) deleted from queue\n", gap->dir);
    free(gap);
}


/* reserves a memory section: best-fit policy*/
int get_free_pos_memory(struct gapq_t * gapq, int tam){
    struct gap_t * gap = gapq->first;
    int pos = -1;
    while (gap->next!=NULL && pos==-1){
        printf("\033[0;36msearch_mem_pos>>\033[0m dir: %X; length: %X \n", gap->dir, gap->length);
        /* check if fits in gap */
        if (gap->length>=tam){
            pos = gap->dir;
            
            /* update / erase gap */
            if(gap->length==tam)
                pg_delgap(gapq, gap);
            else {
                gap->dir = gap->dir + tam;
                gap->length = gap->length - tam;
                printf("\033[0;36mupdate_gap>>\033[0m dir: %X->%X; length: %X->%X\n", pos, gap->dir, (gap->length + tam), gap->length);
            }
        }
        /* not fit in gap -> check next */
        else 
            gap=gap->next;
    }
    /* try last element */
    if(pos==-1)
        printf("\033[0;36msearch_mem_pos>>\033[0m dir: %X; length: %X \n", gap->dir, gap->length);
    if (pos==-1 && gap->length>=tam){
        pos = gap->dir;
        /* update / erase gap */
        if(gap->length==tam)
            pg_delgap(gapq, gap);
        else {
            gap->dir = gap->dir + tam;
            gap->length = gap->length - tam;
            printf("\033[0;36mupdate_gap>>\033[0m dir: %X->%X; length: %X->%X\n", pos, gap->dir, (gap->length + tam), gap->length);
        }
    }
    return pos;
}

/* sets a memory section as void  */
void set_void(struct gapq_t * gapq, int dirstart, int dirend){
    struct gap_t * gap = gapq->first;
    int done = -1;
    while (gap->next!=NULL && done==-1){
        /* check if update required */
        if (gap->dir==dirend)
        {
            printf("\033[0;36mupdate_gap>>\033[0m dir: %X -> %X; length: %X -> %X\n", gap->dir, dirstart,  gap->length, gap->length + (dirend - dirstart));
            gap->dir = dirstart;
            gap->length = gap->length + (dirend - dirstart);
            done=0;
        }
        else if ((gap->dir + gap->length)==dirstart)
        {
            printf("\033[0;36mupdate_gap>>\033[0m dir: %X; length: %X -> %X\n", gap->dir, gap->length, gap->length + (dirend - dirstart));
            gap->length = gap->length + (dirend - dirstart);
            done=0;
        }
        /*  check next */
        else 
            gap=gap->next;
    }
    /* try last element */
    if (done == -1 && gap->dir==(dirend))
    {
        printf("\033[0;36mupdate_gap>>\033[0m dir: %X -> %X; length: %X -> %X\n", gap->dir, dirstart,  gap->length, gap->length + (dirend - dirstart));
        gap->dir = dirstart;
        gap->length = gap->length + (dirend - dirstart);
    }
    else if(done == -1 && (gap->dir + gap->length)==dirstart)    
    {
        printf("\033[0;36mupdate_gap>>\033[0m dir: %X; length: %X -> %X\n", gap->dir, gap->length, gap->length + (dirend - dirstart));
        gap->length = gap->length + (dirend - dirstart);
    }    
	/*create new gap*/		
    else if (done == -1){
        gap = malloc(sizeof(struct gap_t));
        gap->dir=dirstart;
        gap->length = dirend - dirstart;
        pg_addgap(gapq, gap);
    }
}



