/* pg.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */

void *process_generation(struct param_t * param);
void * load_program(struct machine_t * machine);
int unload_program(struct memory_t * memory, struct thread_t * thread);