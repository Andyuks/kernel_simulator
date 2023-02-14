/* scheduler.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */
#include "../include/tlb.h"
#include "../include/pg.h"

extern pthread_mutex_t   mtx_dispatcher, mtx_pm;
extern pthread_cond_t cond_dispatcher,cond_dispatcher2, cond_pm, cond_pm2;

struct pcb_t * scheduler(struct processq_t * processq, struct pcb_t * cur, int opt);
struct pcb_t * scheduler_prio(struct processq_t * processq, struct pcb_t * cur);
struct pcb_t * scheduler_not_prio(struct processq_t * processq, struct pcb_t * cur);
int getfirstoc(struct cpu_t * cpu);
void ttl_management(struct machine_t * machine, struct thread_t * thread, struct pcb_t * pcb_null, int i, int j,int  k, int ismem);
int processq_expired_management(struct cpu_t * cpu, int * index, int * expindex);
struct pcb_t * scheduler_rr(struct processq_t * processq, struct pcb_t * cur);
void context_change(struct machine_t * machine, struct pcb_t * sel_pcb, int ismem, int opt, int index, int curindex, int i, int j,  int k);
void add_to_expired(struct machine_t * machine, struct pcb_t * pcb_null, int ismem, int opt, int curindex, int i, int j,  int k);
int getfirstexpoc(struct cpu_t * cpu);
int prio_processq_expired_management(struct cpu_t * cpu);

/* performs the change of context (dispatcher) */
void *dispatcher(struct param_t * param)
{
	int i, j, k, ret, index, expindex, curindex;
	int opt = param->opt;
	int ismem = param->ismem;
	struct pcb_t * sel_pcb;
	struct pcb_t * pcb_null;
	struct machine_t * machine = param->machine;

	printf("\n\033[1;33mDispatcher>>\033[0m Entered!\n");
	pthread_mutex_lock(&mtx_pm);
	printf("\n\033[1;33mDispatcher>>\033[0m I got mutx pm!\n");
	pthread_mutex_lock(&mtx_dispatcher);
	printf("\n\033[1;33mDispatcher>>\033[0m I got mutx disp!\n");

	while(1){
		printf("\n\033[1;33mDispatcher>>\033[0m  starting analysis... \n");
							
		for(i=0; i<machine->maxcpus; i++)
		{
			for(j=0; j<machine->maxcores; j++)
			{
				pcb_null = machine->cpu[i].core[j].pcb_null;
				for(k=0; k<machine->maxthreads; k++)
				{		
					

					/*** TTL MANAGEMENT ***/		
					if(machine->cpu[i].core[j].thread[k].cur_pcb->pid>0) /* process is being executed */
						ttl_management(machine, &machine->cpu[i].core[j].thread[k],  pcb_null, i,  j,  k, ismem);

					/*** GET INDEX ***/
					if(opt>3) /* priority queues */
					{
						index = getfirstoc(&machine->cpu[i]);
						expindex = getfirstexpoc(&machine->cpu[i]);
						curindex =  machine->cpu[i].core[j].thread[k].cur_pcb->prio;
					}
					else
					{
						index = 0; curindex=0;
					}
					printf("\033[1;33mDispatcher>>\033[0m curindex: %d - index: %d\n", curindex, index);
					
						
					/*** PROCESSES WAITING? ***/
					if (opt==0 || opt > 3)/* prio: check processq and expired */
					{
						if(machine->cpu[i].core[j].thread[k].cur_pcb->pid !=-1)
							add_to_expired(machine,pcb_null, ismem, opt, curindex,  i,  j,   k);				

						if(opt>3)
							ret = processq_expired_management(&machine->cpu[i], &index, &expindex);
						else
							ret = prio_processq_expired_management(&machine->cpu[i]);
						
						if(ret==-1)
							continue;
						
						printf("\033[1;33mDispatcher>>\033[0m curindex: %d - index: %d - expindex %d\n", curindex, index, expindex);
					}
					else /* check processq */
					{
						if (machine->cpu[i].processq[index].first==NULL)
						{
							printf("\033[1;32mScheduler>>\033[0m No processes in processq!\n");
							continue;
						}
					}
					

					/*** DUE PROCESS SELECTION ***/
					printf("\033[1;32mScheduler>>\033[0m Processes waiting! check pq %d\n", index);
					sel_pcb = scheduler(&machine->cpu[i].processq[index], machine->cpu[i].core[j].thread[k].cur_pcb, opt);
					
					/*** CONTEXT CHANGE ***/
					printf("\033[1;33mdispatcher>>\033[0m Selected process: %d - current:  %d!\n", sel_pcb->pid,  machine->cpu[i].core[j].thread[k].cur_pcb->pid);	
					if(sel_pcb->pid != machine->cpu[i].core[j].thread[k].cur_pcb->pid)
						context_change(machine, sel_pcb,  ismem, opt,  index,  curindex, i,  j,   k);
				}	
			}
		}

		if(ismem)
		{
			pthread_cond_broadcast(&cond_pm2);
			pthread_cond_wait(&cond_pm, &mtx_pm);	
		}
		printf("\033[1;33mdispatcher>>\033[0m  analysis ended \n\n");
		pthread_cond_broadcast(&cond_dispatcher);
		pthread_cond_wait(&cond_dispatcher2, &mtx_dispatcher);
		pthread_cond_broadcast(&cond_pm2);
	}
}
    

/* manages ttl*/
void ttl_management(struct machine_t * machine, struct thread_t * thread, struct pcb_t * pcb_null, int i, int j,int  k, int ismem)
{		
	/* process has finished execution*/
	if(thread->cur_pcb->ttl==0)
	{		
		/* process finishes */
		printf("\033[1;33mdispatcher>>\033[0m process %d of (%d:%d:%d) finished execution\n", thread->cur_pcb->pid, i, j, k);
		thread->cur_pcb->state = PCB_STATE_FINISHED;
		
		if(ismem)
		{
			unload_program( machine->memory, &machine->cpu[i].core[j].thread[k]);
			printf("\033[1;33mdispatcher>>\033[0m program removed! \n");
		}

		//free(cur_pcb);

		/* thread management */
		thread->cur_pcb = pcb_null;
	}
	/* process has not finished yet!*/
	else if	(ismem == 0)
	{
		thread->cur_pcb->ttl --;		
		printf("\033[1;33mdispatcher>>\033[0m process %d of (%d:%d:%d) ttl reduced: %d\n", thread->cur_pcb->pid, i, j, k, thread->cur_pcb->ttl);
	}
		
}


/* manages expired and processq: change pointers if required (case prio queues) */
int processq_expired_management(struct cpu_t * cpu, int * index, int * expindex)
{
	int tmp;
	struct processq_t * intermediate;

	/* no processes pq n expired*/
	if (*index==-1 && *expindex==-1)
	{
		printf("\033[1;32mScheduler>>\033[0m No processes in processq or expired!\n");
		return -1;
	}	
	/* no processes pq */
	else if (*index==-1)
	{
		printf("\033[1;32mScheduler>>\033[0m Processes in expired! (exp dir %p pq dir %p)\n", cpu->expired, cpu->processq);
		intermediate = cpu->processq;
		cpu->processq = cpu->expired;
		cpu->expired = intermediate;
		printf("\033[1;33mdispatcher>>\033[0m Pointers changed! (exp dir %p pq dir %p)\n", cpu->expired, cpu->processq);
		tmp = *index;
		*index = *expindex;
		*expindex = tmp;
	}
	return 0;
}

/* manages expired and processq: change pointers if required (case prio)*/
int prio_processq_expired_management(struct cpu_t * cpu)
{
	struct processq_t * intermediate;

	/* no processes pq n expired*/
	if ((cpu->processq[0].first==NULL) && (cpu->expired[0].first==NULL))
	{
		printf("\033[1;32mScheduler>>\033[0m No processes in processq or expired!\n");
		return -1;
	}	
	/* no processes pq */
	else if (cpu->processq[0].first==NULL)
	{
		printf("\033[1;32mScheduler>>\033[0m Processes in expired! (exp dir %p pq dir %p)\n", cpu->expired, cpu->processq);
		intermediate = cpu->processq;
		cpu->processq = cpu->expired;
		cpu->expired = intermediate;
		printf("\033[1;32mScheduler>>\033[0m Pointers exchanged! (exp dir %p pq dir %p)\n", cpu->expired, cpu->processq);
	}
	return 0;

}

/* implements policy: prioritized */
struct pcb_t * scheduler_prio(struct processq_t * processq, struct pcb_t * cur)
{
	struct pcb_t * chosen;
	if(cur->prio <= processq->first->prio)
	{
		chosen = cur;
		printf("\033[1;32mScheduler>>\033[0m Maintain pcb: %d vs %d\n", chosen->pid, cur->pid);	
	}
	else
	{
		chosen = processq->first;
		printf("\033[1;32mScheduler>>\033[0m New pcb: %d\n", chosen->pid);
	}
	return chosen;
}


/* implements policy: no priorities */
struct pcb_t * scheduler_not_prio(struct processq_t * processq, struct pcb_t * cur)
{
	struct pcb_t *  chosen;
	if(cur->pid != -1)
	{
		chosen = cur;
		printf("\033[1;32mScheduler>>\033[0m Maintain pcb: %d\n", chosen->pid);	
	}
	else
	{
		chosen = processq->first;
		printf("\033[1;32mScheduler>>\033[0m New pcb: %d\n", chosen->pid);
	}
	return chosen;
}


/* implements policy: no priorities case round robin*/
struct pcb_t * scheduler_rr(struct processq_t * processq, struct pcb_t * cur)
{
	struct pcb_t * chosen = processq->first;
	printf("\033[1;32mScheduler>>\033[0m New pcb: %d\n", chosen->pid);	
	return chosen;
}



/* return the process to execute (scheduler) */
struct pcb_t * scheduler(struct processq_t * processq, struct pcb_t * cur, int opt)
{
	struct pcb_t * pcb;
	switch (opt){
		case 0:
		case 4:
		case 5:
		case 6: pcb = scheduler_prio(processq, cur);
		break;
		case 3:
			pcb = scheduler_rr(processq, cur);
		break;
		default: pcb =scheduler_not_prio(processq, cur);
	}
	return pcb;
}

/* return the index of first processq that has pcbs */
int getfirstoc(struct cpu_t * cpu)
{
	int i=0, flag =0;
	while (i<140 && flag==0)
	{
		if(cpu->processq[i].first!=NULL)
			flag = 1;
		else
			i++;
	}
	if(flag==0) return -1;
	else return i;
}

/* return the index of first expired-processq that has pcbs */
int getfirstexpoc(struct cpu_t * cpu)
{
	int i=0, flag =0;
	while (i<140 && flag==0)
	{
		if(cpu->expired[i].first!=NULL)
			flag = 1;
		else
			i++;
	}

	if(flag==0) return -1;
	else return i;
}


/* adds a process to expired */
void add_to_expired(struct machine_t * machine, struct pcb_t * pcb_null, int ismem, int opt, int curindex, int i, int j,  int k)
{
	/* add to expired */
	printf("\033[1;33mdispatcher>>\033[0m Analyzing expired processess...\n");
	if(machine->cpu[i].expired[curindex].numpid < machine->cpu[i].expired[curindex].maxpid)
	{
		if(ismem)
			machine->cpu[i].core[j].thread[k].cur_pcb->context =  machine->cpu[i].core[j].thread[k].context;
		machine->cpu[i].core[j].thread[k].cur_pcb->state = PCB_STATE_IDLE;
		printf("\033[1;33mdispatcher>>\033[0m Room in expired! (queue %d: (%d / %d))\n", curindex,  machine->cpu[i].expired[curindex].numpid, machine->cpu[i].expired[curindex].maxpid);
		pq_addPCB(&machine->cpu[i].expired[curindex], machine->cpu[i].core[j].thread[k].cur_pcb, opt);
		machine->cpu[i].expired[curindex].numpid++;

	} else /* no room -> free  pcb*/
	{
		printf("\033[1;33mdispatcher>>\033[0m No room in expired, del proc...\n");
		machine->cpu[i].core[j].thread[k].cur_pcb->state = PCB_STATE_DEAD;
		if(ismem)
		{
			unload_program( machine->memory, &machine->cpu[i].core[j].thread[k]);
			printf("\\033[1;33mdispatcher>>\033[0m program removed! \n");
		}
		//free(machine->cpu[i].core[j].thread[k].cur_pcb);
	}
	machine->cpu[i].core[j].thread[k].cur_pcb = pcb_null;
}

/* performs the context management when a process is being executed */
void context_management_pcb(struct machine_t * machine, struct pcb_t * sel_pcb, int ismem, int opt, int curindex, int i, int j,  int k)
{
	printf("\033[1;33mdispatcher>>\033[0m process: %d extracted (%d:%d:%d)\n", machine->cpu[i].core[j].thread[k].cur_pcb->pid, i, j, k);
	machine->cpu[i].core[j].thread[k].cur_pcb->state=PCB_STATE_IDLE; //set current pcb to idle
	
	if(ismem)/* context management*/
		machine->cpu[i].core[j].thread[k].cur_pcb->context =  machine->cpu[i].core[j].thread[k].context;	

	/* return to queue: RR */	
	pq_addPCB(&machine->cpu[i].processq[curindex], machine->cpu[i].core[j].thread[k].cur_pcb, opt); // return cur to queue
	machine->cpu[i].processq[curindex].numpid++;
}


/* performs the context change */
void context_change(struct machine_t * machine, struct pcb_t * sel_pcb, int ismem, int opt, int index, int curindex, int i, int j,  int k)
{
	if ((sel_pcb->pid != machine->cpu[i].core[j].thread[k].cur_pcb->pid) &&  (machine->cpu[i].core[j].thread[k].cur_pcb->pid!=-1))
		context_management_pcb(machine, sel_pcb,  ismem, opt, curindex,  i,  j,   k); /*case RR */

	machine->cpu[i].core[j].thread[k].cur_pcb = sel_pcb; // put selected in thread
	machine->cpu[i].core[j].thread[k].cur_pcb->state=PCB_STATE_RUNNING; //set new current to running

	/* update num of processes*/
	pq_delPCB(&machine->cpu[i].processq[index], sel_pcb);
	machine->cpu[i].processq[index].numpid--;

	if(ismem)
	{
		clean_TLB(&machine->cpu[i].core[j].thread[k].mmu.tlb);
		machine->cpu[i].core[j].thread[k].context = machine->cpu[i].core[j].thread[k].cur_pcb->context;
		machine->cpu[i].core[j].thread[k].PTBR = machine->cpu[i].core[j].thread[k].cur_pcb->mm.pgb;
		machine->cpu[i].core[j].thread[k].table_entries = sel_pcb->mm.table_entries;
	}
	printf("\033[1;33mdispatcher>>\033[0m process %d on (%d:%d:%d)\n\n", sel_pcb->pid, i, j, k);	

}