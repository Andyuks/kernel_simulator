/* disk.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */

void add_to_ftl(struct disk_t * disk, struct ftl_t * ftl,  int num, int data);
int get_from_disk(struct disk_t * disk, struct ftl_t * ftl,  int num);