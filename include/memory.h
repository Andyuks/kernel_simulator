/* memory.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */

int get_content(struct memory_t * memory, int pos);
void set_content(struct memory_t * memory, int pos, int value);
int get_free_pos_memory(struct gapq_t * gapq, int tam);
void set_void(struct gapq_t * gapq,int dirstart, int dirend);
void pg_addgap (struct gapq_t * gapq, struct gap_t * gap);
void pg_delgap(struct gapq_t * gapq, struct gap_t * gap);