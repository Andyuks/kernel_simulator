/* program_management.c*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

#ifndef MEMORY_H
	#define MEMORY_H
	#include "../include/memory.h"	
#endif
#include "../include/tlb.h"
#include "../include/disk.h"

#define mask_decode_op      0xF0000000
#define mask_decode_r1      0x0F000000
#define mask_decode_r2      0x00F00000
#define mask_decode_r3      0x000F0000
#define mask_decode_dir     0x00FFFFFF

#define ld      0
#define st      1
#define add     2
// former sub
#define rddisk     3 
// former mul
#define wrdisk     4 
#define div     5
#define and     6
#define or      7
#define xor     8
#define mov     9
#define cmp     10
#define b       11
#define beq     12
#define bgt     13
#define blt     14
#define exit    15

extern pthread_mutex_t    mtx_pm;
extern pthread_cond_t cond_pm,cond_pm2;

int fetch_decode(struct thread_t * thread, struct memory_t * memory);
int load_op(struct thread_t * thread);
void arithm(struct thread_t * thread, struct memory_t * memory, struct disk_t * disk, struct ftl_t * ftl);
void writeresults(struct thread_t * thread, int res);

int security_check_ld(struct thread_t * thread);
int security_check_st(struct thread_t * thread);
int security_checks_add(struct thread_t * thread);
int security_checks_rddisk(struct thread_t * thread);
int security_checks_wrdisk(struct thread_t * thread);

/* provides physical memory address */
int getphysical(struct thread_t * thread, struct memory_t * memory, int pc)
{
	int page, frame, pos;
	page = pc/16;
	printf("\033[0;36mget_phys>>\033[0m dir %X page %d\n", pc, page);
	frame = isinTLB(&thread->mmu.tlb, page);
	
	if (frame==-1)
	{
		printf("\033[0;36mget_phys>>\033[0mentry not in TLB\n");
		pos = 4*page + thread->PTBR;
		frame = get_content(memory, pos);
		printf("\033[0;36mget_phys>>\033[0m ptbr %X page %X pos %X frame %X\n", thread->PTBR, page, pos,frame);	
		add_to_TLB(&thread->mmu.tlb, page, frame);
	}
	return frame;
}


/* manage instruction execution */  
void *program_manager(struct param_t * param)
{
	struct machine_t * machine = param->machine;
	int i,j, k, ret;
	printf("\n\033[1;34mPm>>\033[0m Entered!\n");
	pthread_mutex_lock(&mtx_pm);

	while(1){	
		printf("\n\033[1;34mPm>>\033[0m Starting analysis... \n");
		for(i=0; i<machine->maxcpus; i++)
			for(j=0; j<machine->maxcores; j++)
				for(k=0; k<machine->maxthreads; k++)
				{
					if(machine->cpu[i].core[j].thread[k].cur_pcb->pid!=-1)
					{
						printf("\033[1;34mPm>>\033[0m Execute instr [%d:%d:%d]\n", i, j, k);
						ret = fetch_decode(&machine->cpu[i].core[j].thread[k],  machine->memory);
						
						if(ret==0)
						{
							ret = load_op(&machine->cpu[i].core[j].thread[k]);
							if(ret==0)
							{
								arithm(&machine->cpu[i].core[j].thread[k], machine->memory, machine->disk, machine->ftl);
								if(machine->cpu[i].core[j].thread[k].context->op == exit)
									machine->cpu[i].core[j].thread[k].cur_pcb->ttl=0;
							}
						} 
					}
					if (ret==-1) machine->cpu[i].core[j].thread[k].cur_pcb->ttl=0;
					printf("\033[1;34mPm>>\033[0m Finished with [%d:%d:%d]\n", i, j, k);
				}
		printf("\033[1;34mPm>>\033[0m Analysis ended\n");
		// timer related
		pthread_cond_broadcast(&cond_pm);
		pthread_cond_wait(&cond_pm2, &mtx_pm);
	}
}



// B/D L A E //

/* performs fetch/decode*/
int fetch_decode(struct thread_t * thread, struct memory_t * memory)
{
	int dir, pos, frame;
	dir = thread->context->pc;
	printf("\033[1;34mPm_BD>>\033[0m virtual dir %X\n", dir);
	if(dir < 0 || dir > (thread->cur_pcb->mm.end -4) || dir < thread->cur_pcb->mm.code || dir >= thread->cur_pcb->mm.data)
	{
		fprintf(stderr,"\033[1;31mSegmentation fault (B/D dir out of bounds) -  core dumped :>\033[0m\n");
		return -1;
	}

	frame = getphysical(thread, memory, dir);
	if(frame!=-1)	
	{
		pos = (frame*16) + (dir % 16);
		thread->context->ri = get_content(memory, pos);
		printf("\033[1;34mPm_BD>>\033[0m Pos %X Frame %X Content %X \n", pos, frame, thread->context->ri);
		thread->context->pc = thread->context->pc + 4;
		thread->context->op = (thread->context->ri & mask_decode_op) >> 28;
		printf("\033[1;34mPm_BD>>\033[0m pc %X -> %X, ri %08X, op %X\n", thread->context->pc -4, thread->context->pc, thread->context->ri, thread->context->op);
		return 0;
	}
	else
		return -1;
}



/*load operands*/
int load_op(struct thread_t * thread)
{
	int ret = 0;
	switch(thread->context->op)
	{
		case ld: //op:0 rd:reg ra:dir
			thread->rd = (thread->context->ri & mask_decode_r1) >> 24;
			thread->ra = (thread->context->ri & mask_decode_dir);
			printf("\033[1;34mPm_L>>\033[0m LD operands: rd %d, ra %X\n", thread->rd, thread->ra);

			/* security checks*/
			ret = security_check_ld(thread);
			break;

		case st://op:1 ra:reg rb:dir
			thread->ra = (thread->context->ri & mask_decode_r1) >> 24;
			thread->rb = (thread->context->ri & mask_decode_dir);
			printf("\033[1;34mPm_L>>\033[0m ST operands: ra %d, rb %X\n", thread->ra, thread->rb);

			/* security checks*/
			ret = security_check_st(thread);
			break;
		case add: //op:2 rd:reg_dest ra:reg1 rb:reg2
			thread->rd = (thread->context->ri & mask_decode_r1) >> 24;
			thread->ra = (thread->context->ri & mask_decode_r2) >> 20;
			thread->rb = (thread->context->ri & mask_decode_r3) >> 16;
			printf("\033[1;34mPm_L>>\033[0m ADD operands: rd %d, ra %d, rb %d\n", thread->rd, thread->ra, thread->rb);

			/* security checks */
			ret = security_checks_add(thread);
			
			break;
		case rddisk: //op:3 rd:reg_dest ra:regdisk
			thread->rd = (thread->context->ri & mask_decode_r1) >> 24;
			thread->ra = (thread->context->ri & mask_decode_r2) >> 20;
			printf("\033[1;34mPm_L>>\033[0m RD operands: rd %d, ra %d\n", thread->rd, thread->ra);
			
			/* security checks */
			ret = security_checks_rddisk(thread);


			break;
		case wrdisk: //op:4 ra:reg rb:regdisk
			thread->ra = (thread->context->ri & mask_decode_r1) >> 24;
			thread->rb = (thread->context->ri & mask_decode_r2) >> 20;
			printf("\033[1;34mPm_L>>\033[0m WR  operands: ra %d, rb %d\n", thread->ra, thread->rb);

			/* security checks */
			ret = security_checks_wrdisk(thread);

			break;
		case exit:
			printf("\033[1;34mPm_L>>\033[0m Exitting...\n");
			break;
		default:
			fprintf(stderr,"\033[1;31mError: illegal instruction found\033[0m\n");
			ret = -1;
			break;
	}
	return ret;
}

/* calculus and due operations */
void arithm(struct thread_t * thread, struct memory_t * memory, struct disk_t * disk, struct ftl_t * ftl)
{
	int frame, pos, result;
	switch(thread->context->op)
	{
		case ld: //op:0 rd:reg ra:dir
			frame = getphysical(thread, memory, thread->ra);
			if(frame!=-1)
			{
				pos = (frame*16)+ (thread->ra%16);
				result = get_content(memory, pos);
				printf("\033[1;34mPm_A>>\033[0m pos %X content %d\n", pos, result);
				writeresults(thread, result);
			} else
				fprintf(stderr,"\033[1;31mError: could not load data, wrong dir\033[0m\n");

			break;

		case st: //op:1 ra:reg rb:dir
			frame = getphysical(thread, memory, thread->rb);
			if(frame!=-1)
			{
				pos = (frame*16)+ (thread->rb%16);
				printf("\033[1;34mPm_A>>\033[0m pos %X content %d\n", pos, thread->context->br[thread->ra]);
				set_content(memory, pos, thread->context->br[thread->ra]);
			}
			else
				fprintf(stderr,"\033[1;31mError: could not store data, wrong dir\033[0m\n");
			break;

		case add:
			result = thread->context->br[thread->ra] + thread->context->br[thread->rb];
			writeresults(thread, result);
			break;

		case rddisk: //op:3 rd:reg_dest ra:regdisk
			result = get_from_disk(disk, ftl, thread->ra);

			if (result == -1)
				fprintf(stderr,"\033[1;31mError: page holds no value yet\033[0m\n");
			else
			{
				printf("\033[1;34mPm_A>>\033[0m result %d\n", result);
				writeresults(thread, result);
			}
			break;
		case wrdisk://op:4 ra:reg rb:regdisk	
			printf("\033[1;34mPm_A>>\033[0m %d -> disk p%d\n", thread->context->br[thread->ra], thread->rb);
			add_to_ftl(disk, ftl, thread->rb, thread->context->br[thread->ra]);
			
			break;
		case exit:
			printf("\033[1;34mPm_A>>\033[0m Exitting...\n");

			break;
		default:
			fprintf(stderr,"\033[1;31mError: illegal instruction found\033[0m\n");
			break;
	}	
}

/* write results */
void writeresults(struct thread_t * thread, int res)
{
	thread->context->br[thread->rd] = res;
	printf("\033[1;34mPm_W>>\033[0m r%d = %d\n", thread->rd, res);
}



/* security checks (ld) */
int security_check_ld(struct thread_t * thread)
{
	int ret=0;
	//op:0 rd:reg ra:dir
	if(thread->rd < 0 || thread->rd  > 15) // register
	{
		fprintf(stderr,"\033[1;31mError:\033[0m register out of bounds (rd:%d)\n",thread->rd);
		ret = -1;
	} else if(thread->ra < thread->cur_pcb->mm.data|| thread->ra > (thread->cur_pcb->mm.end - 4)) // memory direction
	{
		fprintf(stderr,"\033[1;31mSegmentation fault (ra %X out of bounds %X %X) -  core dumped :>\033[0m\n", thread->ra,thread->cur_pcb->mm.data, thread->cur_pcb->mm.end-4);
		ret= -1;
	}
	return ret;
}

/* security checks (st) */
int security_check_st(struct thread_t * thread)
{
	int ret = 0;
	//op:1 ra:reg rb:dir
	if(thread->ra < 0 || thread->ra  > 15) // register
	{
		fprintf(stderr,"\033[1;31mError:\033[0m register out of bounds (ra:%d)\n",thread->ra);
		ret = -1;
	}else if(thread->rb < thread->cur_pcb->mm.data|| thread->rb > (thread->cur_pcb->mm.end - 4)) // memory direction
	{
		printf("\033[1;31mSegmentation fault (rb %X out of bounds %X %X) -  core dumped :>\033[0m\n", thread->rb,thread->cur_pcb->mm.data, thread->cur_pcb->mm.end-4);
		ret = -1;
	}
	return ret;
}

/* security checks (add) */
int security_checks_add(struct thread_t * thread)
{
	int ret = 0;
	//op:2 rd:reg_dest ra:reg1 rb:reg2
	if(thread->ra < 0 || thread->ra  > 15 || thread->rb < 0 || thread->rb  > 15 || thread->rd < 0 || thread->rd  > 15 ) // registers
	{
		fprintf(stderr,"\033[1;31mError:\033[0m register out of bounds (rd:%d ra:%d rb:%d)\n", thread->rd, thread->ra, thread->rb);
		ret = -1;
	}
	return ret;
}

/* security checks (rddisk) */
int security_checks_rddisk(struct thread_t * thread)
{
	int ret = 0;
	//op:3 rd:reg_dest ra:regdisk
	if(thread->rd < 0 || thread->rd  > 15 ) // register
	{
		fprintf(stderr,"\033[1;31mError:\033[0m register out of bounds (rd:%d)\n", thread->rd);
		ret = -1;
	} else if(thread->ra < 0 || thread->ra  >= MAX_FTL)
	{
		fprintf(stderr,"\033[1;31mError:\033[0m index of page out of bounds %d\n", thread->ra);
		ret = -1;
	}
	return ret;
}

/* security checks (wrdisk) */
int security_checks_wrdisk(struct thread_t * thread)
{
	int ret = 0;
	//op:4 ra:reg rb:regdisk
	if(thread->ra < 0 || thread->ra  > 15 ) // register
	{
		fprintf(stderr,"\033[1;31mError:\033[0m register out of bounds (ra:%d)\n", thread->ra);
		ret = -1;
	} else if(thread->rb < 0 || thread->rb  >= MAX_FTL)
	{
		fprintf(stderr,"\033[1;31mError:\033[0m index of page out of bounds %d\n", thread->rb);
		ret = -1;
	}
	return ret;
}
