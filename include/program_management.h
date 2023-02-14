/* program_management.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */

void *program_manager(struct param_t * param);
int getphysical(struct thread_t * thread, struct memory_t * memory, int pc);