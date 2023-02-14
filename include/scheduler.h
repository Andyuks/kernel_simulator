/* scheduler.h */
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "data_struct.h"
#endif /* DATA_STRUCT_H */

struct pcb_t scheduler(struct processq_t * processq, struct pcb_t * cur, int opt);
struct pcb_t scheduler_prio(struct processq_t * processq, struct pcb_t * cur);
struct pcb_t scheduler_not_prio(struct processq_t * processq, struct pcb_t * cur);
void *dispatcher(struct param_t * param);