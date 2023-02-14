/* data_struct.c*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

/**********
* MACHINE *
***********/

/* initialize machine */
void initialize_machine(struct machine_t * machine, int ncpu, int ncore, int nthreads, int maxpid, int opt, int ismem){
    int i, j, k;
    /* initialize specs*/
    machine->maxcpus = ncpu;
    machine->maxcores = ncore;
    machine->maxthreads = nthreads;

    /* initialize cpu - core - processq - expired*/
    machine->cpu = malloc(ncpu * sizeof(struct cpu_t));
    for(i=0; i<ncpu; i++){
        machine->cpu[i].nextpid=1;
        machine->cpu[i].core =  malloc(ncore * sizeof(struct core_t));

        /* initialize process queue n expired */
        if(opt>3) j=140;
        else      j=1;

        machine->cpu[i].processq = malloc(j * sizeof(struct processq_t));
        if(opt==0 || opt>3)  machine->cpu[i].expired = malloc(j * sizeof(struct processq_t));

        k=0;
        while(k<j){
            initialize_pq(&machine->cpu[i].processq[k], maxpid);
            if (opt==0 || opt>3)
                initialize_pq(&machine->cpu[i].expired[k], maxpid);
            k++;
        }
        printf("\033[0;36minit>>\033[0m Process queue initialized \n");
            

        /* initialize threads and pcb_null*/
        for(j=0;j<ncore;j++)
        {
            machine->cpu[i].core[j].thread = malloc(nthreads * sizeof(struct thread_t));
            machine->cpu[i].core[j].pcb_null = malloc(sizeof(struct pcb_t));
            machine->cpu[i].core[j].pcb_null->prio = 140;
            machine->cpu[i].core[j].pcb_null->pid = -1;
            if(ismem)
            {
                for (k=0;k<nthreads;k++)
                    initialize_tlb( &machine->cpu[i].core[j].thread[k].mmu.tlb);
            }
        }
        
        printf("\033[0;36minit>>\033[0m CPUs Cores and threads ready!\n");
    }

    init_cur(machine);
	
    /* initialize memory - disk - ftl if needed*/
    if(ismem)
    {
        machine->memory =  malloc(sizeof(struct memory_t));
        machine->memory->data = malloc (1 << 24); 
        machine->memory->gapq =  malloc(sizeof(struct gapq_t));
        machine->memory->kernel_gapq =  malloc(sizeof(struct gapq_t));
        initialize_gq(machine->memory->gapq);   
        initialize_kernel_gq(machine->memory->kernel_gapq);   
        machine->disk =  malloc(sizeof(struct disk_t));
        machine->ftl =  malloc(sizeof(struct ftl_t));
        init_ftl(machine->ftl);
        init_disk(machine->disk);
    }
    
    printf("\033[0;36minit>>\033[0m machine initialized\n");
}


/* free space reserved for the machine */
void free_machine(struct machine_t * machine, int ncpu, int ncore, int nthreads, int opt,  int ismem)
{
    int i, j, k;
	
    for(i=0; i<ncpu; i++)
	{
        for(j=0;j<ncore;j++)
		{
			for(k=0;k<nthreads;k++)
				free(machine->cpu[i].core[j].thread[k].mmu.tlb.entry);
			
            free(machine->cpu[i].core[j].thread);
			free(machine->cpu[i].core[j].pcb_null);
			
        }
		free(machine->cpu[i].processq);
        if(opt==0 || opt>3) free(machine->cpu[i].expired);
        free(machine->cpu[i].core);
    }
	
    free(machine->cpu);
    if (ismem)
	{
		free(machine->memory->data);
		free(machine->memory->gapq);
		free(machine->memory->kernel_gapq);
		free(machine->ftl);
		free(machine->disk);
		free(machine->memory);
	}
		
    printf("\033[0;36mshutdown>>\033[0m machine freed\n");
}

/* initialize tlb */
void initialize_tlb(struct tlb_t * tlb)
{
    tlb->entry = malloc(sizeof(struct gap_t) * TAM_TLB);
    tlb->index=0;
    tlb->maxentries = TAM_TLB;
}

/* initialize disk */
void init_disk(struct disk_t * disk)
{
    int i, j;
    for(i=0; i<MAX_BLOCKS; i++)
        for(j=0;j<BLOCK_PAGES; j++)
        {
            disk->block[i].page[j].num = -1;
            disk->block[i].page[j].data = -1;
            disk->block[i].page[j].garbage = 0;
        }
}

/* initialize ftl */
void init_ftl(struct ftl_t * ftl)
{
    int i;
    for(i=0; i<MAX_FTL; i++)
        ftl->page[i].dir = -1;
}


/************
* GAP QUEUE *
*************/

/* initialize gap queue */
void initialize_gq (struct gapq_t * gapq)
{
    struct gap_t * gap = malloc(sizeof(struct gap_t));
    gap->dir = 0x400000;
    gap->length = 0xFFFFFF - 0x400000;
    gap->next = NULL;
    gap->prev = NULL;
    gapq->first = gap;
    gapq->last = gap;
    printf("\033[0;36minit>>\033[0m Gap queue initialized: dir %X len %X \n", gap->dir, gap->length);
}

/* initialize kernel gap queue */
void initialize_kernel_gq (struct gapq_t * gapq)
{
    struct gap_t * gap = malloc(sizeof(struct gap_t));
    gap->dir = 0x000000;
    gap->length = 0x3FFFFF;
    gap->next = NULL;
    gap->prev = NULL;
    gapq->first = gap;
    gapq->last = gap;
    printf("\033[0;36minit>>\033[0m Kernel gap queue initialized: dir %X len %X \n", gap->dir, gap->length);
}



/****************
* PROCESS QUEUE *
*****************/
/* set current processess in execution as pcb_null*/
void init_cur(struct machine_t * machine)
{
	int i,j,k;
	for(i=0; i<machine->maxcpus; i++)
		for(j=0; j<machine->maxcores; j++)
        {
			for(k=0; k<machine->maxthreads; k++)
                machine->cpu[i].core[j].thread[k].cur_pcb = machine->cpu[i].core[j].pcb_null;
        }
}            

/* initialize process queue */
void initialize_pq (struct processq_t * processq, int maxpid){
    processq->first = NULL;
    processq->last = NULL;
    processq->numpid = 0;
    processq->maxpid = maxpid;
}

/* add pcb to process queue: only process of process queue */
void pq_addonly(struct processq_t * processq, struct pcb_t * pcb)
{
    processq->last = pcb;
    processq->first = pcb;
    processq->last->next = NULL;
    processq->first->prev = NULL;
    printf("\033[0;36madd_pcb>>\033[0m Process %d added (only one)\n", pcb->pid);
}

/* add pcb to process queue: beginning of process queue */
void pq_addbeg(struct processq_t * processq, struct pcb_t * pcb)
{
    pcb->next = processq->first;
    processq->first->prev = pcb;
    processq->first = pcb;
    printf("\033[0;36madd_pcb>>\033[0m Process %d added to queue (first pos)\n", pcb->pid);
}

/* add pcb to process queue: end of process queue */
void pq_addlast(struct processq_t * processq, struct pcb_t * pcb)
{
    processq->last->next = pcb; // ... last -> pcb
    pcb->prev = processq->last; // ... last <-> pcb 
    pcb->next = NULL; // ... prev <-> last -> null
    processq->last = pcb; // ... prev <-> last
    printf("\033[0;36madd_pcb>>\033[0m Process %d added to queue (last pos)\n", pcb->pid);
}


/* add pcb to process queue: middle of process queue */
void pq_addmid(struct pcb_t * aux, struct pcb_t * pcb)
{
    pcb->next = aux->next;
    pcb->prev = aux;
    aux->next->prev = pcb;
    aux->next = pcb;
    printf("\033[0;36madd_pcb>>\033[0m Process %d added to queue (mid pos)\n", pcb->pid);
}


/* add pcb to process queue: sjf based */
void pq_addsjf (struct processq_t * processq, struct pcb_t * pcb){
    struct pcb_t * aux;

    if(pcb->ttl<processq->first->ttl) /* case first */
        pq_addbeg(processq, pcb);

    else if (pcb->ttl>=processq->last->ttl)/* case last */
        pq_addlast(processq, pcb);

    else /* case middle */
    {
        aux = processq->first;
        
        while (aux->next!= NULL && aux->next->ttl<=pcb->ttl)
            aux = aux->next;
        
        pq_addmid(aux, pcb);
    }
}

/* add pcb to process queue: priority based*/
void pq_addprio (struct processq_t * processq, struct pcb_t * pcb){
    struct pcb_t * aux;

    if(pcb->prio < processq->first->prio) /* case first */
    {
        pq_addbeg(processq, pcb);
    }
    else if (pcb->prio >= processq->last->prio)/* case last */
    {
        pq_addlast(processq, pcb);
    }
    else /* case middle */
    {
        aux = processq->first;
        
        while (aux->next!= NULL && aux->next->prio<=pcb->prio)
        {
            aux = aux->next;
        }
        pq_addmid(aux, pcb);
    }
}

/* add pcb to process queue according to option */
void pq_addPCB (struct processq_t * processq, struct pcb_t * pcb, int opt)
{
    if (processq->last == NULL) /* case first */
    {
        pq_addonly(processq, pcb);
    }
    else /* not first */
    {
        switch (opt)
        {
            case 0: pq_addprio(processq, pcb); /* prio */
            break;
            
            case 1:
            case 3:
            case 4:
            case 6: pq_addlast(processq, pcb); /* fifo, rr */
            break;

            case 2:
            case 5: pq_addsjf(processq, pcb); /* sjf */
            break;

            default: printf("future cases...\n");
        }        
    }
}



/* delete pcb from process queue*/
void pq_delPCB(struct processq_t * processq,struct pcb_t * pcb){
    if (pcb->pid == processq->last->pid && pcb->pid == processq->first->pid)  /* case 1 elem */
    {
        processq->first = NULL;
        processq->last = NULL;
    }
    else if (pcb->pid == processq->last->pid) /* last elem, not first */
    {
        processq->last = pcb->prev;
        processq->last->next = NULL;
    } 
    else if (pcb->pid == processq->first->pid) /* first elem, not last */
    {
        processq->first = pcb->next;
        processq->first->prev = NULL;
    }else  /* middle */
    {
        pcb->next->prev = pcb->prev;
        pcb->prev->next = pcb->next;
    }
    printf("\033[0;36mdel_pcb>>\033[0m Process %d deleted from queue\n", pcb->pid);
}

