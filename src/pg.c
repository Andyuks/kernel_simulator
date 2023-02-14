/* pg.c */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#ifndef MEMORY_H
	#define MEMORY_H
	#include "../include/memory.h"	
#endif

#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */
#include "../include/program_management.h"

extern pthread_mutex_t   mtx_pg;
extern pthread_cond_t cond,  cond_pg,   cond_pg2;

/* create a process for each CPU */  
void *process_generation(struct param_t * param)
{
	int opt = param->opt;
	struct machine_t * machine = param->machine;
	int i, index, prio=0;
	srand (time(NULL));

	printf("\n\033[1;36mPg>>\033[0m Entered!\n");
	pthread_mutex_lock(&mtx_pg);
	while(1){
		printf("\033[1;36mPg>>\033[0m starting analysis...\n");
		for(i=0; i<machine->maxcpus; i++){
			
			prio = rand()%140; //priority: 0-139
			if(opt>3)
				index = prio;
			else 
				index = 0;
		
			/* check if can create more processes */
			if (machine->cpu[i].processq[index].numpid == machine->cpu[i].processq[index].maxpid)
				printf(" \033[1;36mPg>>\033[0m cannot create more processes for cpu %d - processq %d (%d/%d)\n", i, index, machine->cpu[i].processq[index].numpid, machine->cpu[i].processq[index].maxpid );
			
			else
			{
				struct pcb_t *pcb = malloc(sizeof(struct pcb_t));
				pcb->prio = prio;
				pcb->pid=machine->cpu[i].nextpid;
				pcb->ttl = (rand()%5) + 1; //ttl: 1-5
				printf("\033[1;36mPg>>\033[0m process %d dir %p prio %d ttl %d \n", pcb->pid, pcb, pcb->prio, pcb->ttl);
				pq_addPCB(&machine->cpu[i].processq[index], pcb, opt);
				printf("\033[1;36mPg>>\033[0m process queue last %d dir %p prio %d ttl %d\n", machine->cpu[i].processq[index].last->pid, machine->cpu[i].processq[index].last, machine->cpu[i].processq[index].last->prio, machine->cpu[i].processq[index].last->ttl);
				printf("\033[1;36mPg>>\033[0m process queue first %d dir %p prio %d ttl %d \n", machine->cpu[i].processq[index].first->pid, machine->cpu[i].processq[index].first, machine->cpu[i].processq[index].first->prio, machine->cpu[i].processq[index].first->ttl);
				machine->cpu[i].nextpid++;
				machine->cpu[i].processq[index].numpid++;
				printf("\033[1;36mPg>>\033[0m process %d generated for cpu %d and processq %d\n", pcb->pid, i, index);
			}		
		}
		printf("\033[1;36mPg>>\033[0m Analysis ended...\n\n");
		pthread_cond_broadcast(&cond_pg);
		pthread_cond_wait(&cond_pg2, &mtx_pg);
	}
}


/* load a program in  memory */  
void * load_program(struct param_t * param)
{
	int i, l, off, offkernel, filesize, pagenum, tablesize, index, prio, frame,  k=0;
	unsigned int code_start, data_start, data;
	int opt = param->opt;
	struct machine_t * machine = param->machine;
	FILE *fp;
	char line[8];

	srand (time(NULL));
	pthread_mutex_lock(&mtx_pg);
	while(1){
		printf("\033[1;36mLoader>>\033[0m starting analysis...\n");
		for(i=0; i<machine->maxcpus; i++){
			
			prio = rand()%140; //priority: 0-139
			if(opt>3)
				index = prio;
			else 
				index = 0;
			
			/* check if can create more processes */
			printf("\033[1;36mLoader>>\033[0m index: %d (%d - %d)\n", index, machine->cpu[i].processq[index].numpid, machine->cpu[i].processq[index].maxpid);
			if (machine->cpu[i].processq[index].numpid == machine->cpu[i].processq[index].maxpid)
				printf("\033[1;36mLoader>>\033[0m cannot create more processes for cpu %d\n", i);
				
			else 
			{
				
				/* create pcb */
				struct pcb_t *pcb = malloc(sizeof(struct pcb_t));
				pcb->context = malloc(sizeof(struct context_t) );
				sprintf(pcb->context->prog_name, "../programs/prog%03d.elf", k);
				k = (k+1) % MAX_PROGRAMS;
				pcb->prio = prio; //priority: 0-139
				pcb->pid=machine->cpu[i].nextpid;

				printf(" \033[1;36mLoader>>\033[0mLoading program %s...\n", pcb->context->prog_name);

				/* open file to read program*/
				fp = fopen(pcb->context->prog_name, "r");
				if(fp==NULL)
				{
					fprintf(stderr,"\033[1;31mError: file could not be opened.\033[0m\n");
				}
				else 
				{
					/* get file size*/
					fseek (fp, 0, SEEK_END);
					filesize = ftell(fp);
					filesize = (filesize -26) /9;
					pcb->mm.end = filesize*4;
					fseek (fp, 0, SEEK_SET);

					/* read text and data addr*/
					if(fscanf(fp, "%s %X", line, &code_start)!=2)
					{
						printf("\033[1;31mError:\033[0m Wrong file format :<\n");
						fclose(fp);
						continue;
					}
					if(fscanf(fp, "%s %X", line, &data_start)!=2)
					{
						printf("\033[1;31mError:\033[0m Wrong file format :<\n");
						fclose(fp);
						continue;
					}
					printf("\033[1;36mLoader>>\033[0m code_start %X, data_start %X, end %X\n", code_start, data_start, pcb->mm.end);

					
					/* get size to be reserved and due offset for the program */
					printf("\033[1;36mLoader>>\033[0m word amount in file %d\n", filesize);
					if((filesize)%4!=0)
						filesize = (((filesize/4)+1)*4);
					filesize = 4*filesize; // each instr occupies 4 positions of mem
					pagenum = filesize/16;
					printf("\033[1;36mLoader>>\033[0m Reserve %d pos (%d pag)\n", filesize, pagenum);
					off = get_free_pos_memory(machine->memory->gapq, filesize);
					frame = off / 16;

					/* get page table size and due offset for the page table*/
					if((pagenum)%4!=0)
						tablesize = ((((pagenum)/4)+1)*4);
					tablesize = 4*tablesize; // each instr occupies 4 positions of mem
					offkernel = get_free_pos_memory(machine->memory->kernel_gapq, tablesize);

					/* check if enough space */
					if (off == -1 || offkernel == -1)
					{
						fprintf(stderr,"\033[1;31mError: Not enough memory.\033[0m\n");	
						fclose(fp);

						if(off!=-1)
						{
							struct gap_t * gap = malloc(sizeof(struct gap_t));
							gap->dir=off;
							gap->length = filesize;
							pg_addgap(machine->memory->gapq, gap);
						}
						if(offkernel!=-1)
						{
							struct gap_t * gap = malloc(sizeof(struct gap_t));
							gap->dir=offkernel;
							gap->length = tablesize;
							pg_addgap(machine->memory->kernel_gapq, gap);
						}	
						// free(pcb); // memory corruption
						continue;
					}

					/* set due data */
					printf("\033[1;36mLoader>>\033[0m Load at %X (frame %X)\n", off, frame);
					pcb->mm.code = code_start;
					pcb->mm.data = data_start;
					pcb->context->pc = pcb->mm.code;
					pcb->ttl = data_start - code_start; // num of code lines
					pcb->mm.pgb = offkernel;
					pcb->mm.table_entries = pagenum;

					/* set content in memory */
					while(fscanf(fp, "%08X", &data)==1)
					{	
						set_content(machine->memory, off, data);
						off = off + 4;
					}

					printf("\033[1;36mLoader>>\033[0m Load pagetable at %X\n", offkernel);
					/* set page table in memory */
					for(l=0; l<pagenum; l++)
					{	
						set_content(machine->memory, offkernel, l + frame);
						offkernel = offkernel + 4;
					}

					/* close file */
					fclose(fp);
					printf("\033[1;36mLoader>>\033[0m File closed\n");
					/* add process to processq */
					printf(" \033[1;36mPg>>\033[0m process %d dir %p prio %d ttl %d  pq %d\n", pcb->pid, pcb, pcb->prio, pcb->ttl, index);
					pq_addPCB(&machine->cpu[i].processq[index], pcb, opt);
					printf("\033[1;36mLoader>>\033[0m pcb added\n");
					machine->cpu[i].nextpid++;
					machine->cpu[i].processq[index].numpid++;
				}
				
			}	
		}
		pthread_cond_broadcast(&cond_pg);
		pthread_cond_wait(&cond_pg2, &mtx_pg);
	}
}

/* Remove a program from  memory */  
int unload_program(struct memory_t * memory, struct thread_t * thread)
{
	/* declaration of variables */
	FILE *fp;
	int i=0, frame, dir_code, size_prog, filesize, tablesize;
	char text[16], dataaddr[16], data[10];

	/* initialize */
	size_prog = thread->cur_pcb->mm.end - thread->cur_pcb->mm.code;
	frame = getphysical(thread, memory, thread->cur_pcb->mm.code);
	dir_code = frame * 16 + (thread->cur_pcb->mm.code % 16);


	printf(" \033[1;36mLoader>>\033[0m Removing program %s... frame %X dir %X\n", thread->cur_pcb->context->prog_name, frame, dir_code);

	/* open file to write program back*/
	fp = fopen(thread->cur_pcb->context->prog_name, "w");
	if(fp==NULL)
	{
		fprintf(stderr," \033[1;31mError: file could not be opened.\033[0m\n");
		return(1);
	}

	/* write text and data addr*/
	sprintf(text, ".text %06X\n", thread->cur_pcb->mm.code); /* code start virtual addr*/
    sprintf(dataaddr, ".data %06X\n", thread->cur_pcb->mm.data); /* data start virtual addr, aft code*/
	fwrite( text,1,strlen(text), fp);
    fwrite( dataaddr,1,strlen(dataaddr), fp);

	/* write content */
	while(i  < size_prog)
	{
		sprintf(data, "%08X\n", get_content(memory, dir_code + i ));
		fwrite(data,1,strlen(data), fp);
		i = i+4;
	}

	
	/* close file */
    fclose(fp);

	/* get measures */
	filesize = size_prog / 4;
	if((filesize)%4!=0)
		filesize = (((filesize/4)+1)*4);
	filesize = 4*filesize; // each instr occupies 4 positions of mem
	
	if((thread->cur_pcb->mm.table_entries)%4!=0)
		tablesize = ((((thread->cur_pcb->mm.table_entries)/4)+1)*4);
	tablesize = 4*tablesize;

	/* free memory */
	printf(" \033[1;36mLoader>>\033[0m Set void %X - %X\n", dir_code, dir_code + filesize -1);
	set_void(memory->gapq, dir_code, dir_code + filesize);
	printf(" \033[1;36mLoader>>\033[0m Set void %X - %X\n",  thread->cur_pcb->mm.pgb, thread->cur_pcb->mm.pgb + tablesize -1);
	set_void(memory->kernel_gapq, thread->cur_pcb->mm.pgb, thread->cur_pcb->mm.pgb + tablesize);
	printf("\033[1;36mLoader>>\033[0m Program removed!\n");
	return(0);
}	