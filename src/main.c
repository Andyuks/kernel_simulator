/********************************
OPERATIVE SYSTEMS : MAIN PROGRAM 
********************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>

#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

#include "../include/clock.h"
#include "../include/timer.h"
#include "../include/scheduler.h"
#include "../include/pg.h"
#include "../include/program_management.h"
#define NTHREADS 6
void parse_arguments(int argc, char *argv[], int * maxcpus, int * maxcores, int * maxthreads, int * maxpids, int * opt, int * ismem);

/* main */
int main (int argc, char * argv[]){
	/* declare variables */
	int maxcpus, maxcores, maxthreads, maxpids, ismem, opt;
	struct machine_t machine;
	pthread_t threads[NTHREADS];
	struct param_t param;

	printf("\n\033[1;35m***************************\033[0m\n");
	printf("\033[1;35m*** SIMULADOR DE KERNEL ***\033[0m\n");
	printf("\033[1;35m***************************\033[0m\n");

	/* ask for data */
	parse_arguments(argc, argv, &maxcpus, &maxcores, &maxthreads, &maxpids, &opt, &ismem);

	/* check data */
	if(maxcpus<1 || maxcores<1 || maxthreads<1 || maxpids<1 || opt<0 || opt>6 || (ismem!=0 && ismem !=1)){
		fprintf(stderr,"\033[1;31mError: you must provide valid values.\033[0m\n");
		exit(1);
	}

	printf("\033[1;34m> CPUs: %d\033[0m\n", maxcpus);
	printf("\033[1;34m> COREs: %d\033[0m\n", maxcores);
	printf("\033[1;34m> THREADs: %d\033[0m\n", maxthreads);
	printf("\033[1;34m> PIDs: %d\033[0m\n", maxpids);
	printf("\033[1;34m> Option: %d \n\t0: prio\n\t1: FIFO\n\t2: SJF\n\t3: RR\n\t4: prio queues(fifo)\n\t5: prio queues (sjf)\n\t6: prio queues (rr)):\033[0m\n", opt);
	printf("\033[1;34m> Memory based? %d (based (1) not (0))\033[0m\n", ismem);
	
	/* initializations */
	initialize_machine(&machine, maxcpus, maxcores, maxthreads, maxpids, opt, ismem);
	init_mutex_cond();
	param.machine = &machine;
	param.opt= opt;
	param.ismem= ismem;

	/* call functions respectively */
	pthread_create(&threads[0], NULL, (void *)start_clock, NULL);
	pthread_create(&threads[1], NULL, (void *)timer_pg, NULL);
	pthread_create(&threads[2], NULL, (void *)timer_dispatcher,  NULL);
	if(ismem)
	{
		pthread_create(&threads[3], NULL, (void *)load_program, &param);
		pthread_create(&threads[4], NULL, (void *)program_manager, &param);
	}
	else	
	{
		pthread_create(&threads[3], NULL, (void *)process_generation, &param);
	}
	pthread_create(&threads[5], NULL, (void *)dispatcher, &param);

	/* join threads after function is completed */
	for (int i=0; i<NTHREADS; ++i) 
		pthread_join(threads[i], NULL);
	

	/* free memory and exit */
	free_machine(&machine, maxcpus, maxcores, maxthreads, opt, ismem);
	exit(0);
}

/* function to parse arguments */
void parse_arguments(int argc, char *argv[], int * maxcpus, int * maxcores, int * maxthreads, int * maxpids, int * opt, int * ismem)
{
	int o, long_index;
	static struct option long_options[] = {
        {"help",       no_argument,       0,  'h' },
        {"cpus",      required_argument, 0,  'c' },
        {"cores",      required_argument, 0,  'r' },
        {"threads",       required_argument, 0,  't' },
        {"pids",       required_argument, 0,  'p' },
        {"opt",   required_argument, 0,  'o' },
        {"ismem",       required_argument, 0,  'm' },
        {0,            0,                 0,   0  }
    };
	*maxcpus = 1;
	*maxcores = 1;
	*maxthreads = 1;
	*maxpids = 1;
	*opt = 1;
	*ismem = 1;

	while ((o = getopt_long(argc, argv,":h:c:r:t:p:o:m:", 
                        long_options, &long_index )) != -1) 
	{
		switch(o)
		{
			case 'h':
				printf("Use: %s [OPTIONS]\n", argv[0]);
				printf(" --h, --help \t\t Help\n");
				printf(" -c, --cpus=N \t\t CPU number\n");
				printf(" -r, --cores=N \t\t core number\n");
				printf(" -t, --threads=N \t\t thread number\n");
				printf(" -p, --pids=N \t\t max pids\n");
				printf(" -o, --opt=0-6 \t\t option\n");
				printf(" -m, --ismem=0|1 \t\t memory\n");
				exit(0);
			case 'c':
				*maxcpus=atoi(optarg);
				break;
			case 'r':
				*maxcores=atoi(optarg);
				break;
			case 't':
				*maxthreads=atoi(optarg);
				break;
			case 'p':
				*maxpids=atoi(optarg);
				break;
			case 'o':
				*opt=atoi(optarg);
				break;
			case 'm':
				*ismem=atoi(optarg);
				break;
			default:
				printf("Unknown argument option\n");
		}
	}
}
