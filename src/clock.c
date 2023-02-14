/* clock.c */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef DATA_STRUCT_H
	#define DATA_STRUCT_H
	#include "../include/data_struct.h"
#endif /* DATA_STRUCT_H */

pthread_mutex_t mtx_clock, mtx_pg, mtx_dispatcher, mtx_pcb, mtx_pm;
pthread_cond_t cond, condbr, cond_pg, cond_dispatcher, cond_pg2,cond_dispatcher2, cond_pm, cond_pm2;
int done = 0;


/* Initializes the mutex and the conditions */
void init_mutex_cond(){
    pthread_mutex_init(&mtx_clock, NULL);
	pthread_mutex_init(&mtx_pg, NULL);
	pthread_mutex_init(&mtx_dispatcher, NULL);
	pthread_mutex_init(&mtx_pcb, NULL);
	pthread_mutex_init(&mtx_pm, NULL);

    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&condbr, NULL);
	pthread_cond_init(&cond_pg, NULL);
	pthread_cond_init(&cond_pg2, NULL);
	pthread_cond_init(&cond_dispatcher2, NULL);
	pthread_cond_init(&cond_dispatcher, NULL);
	pthread_cond_init(&cond_pm2, NULL);
	pthread_cond_init(&cond_pm, NULL);
	

}

/* Starts the clock */
void start_clock(){
	int timer_num;
	timer_num = 2;


	pthread_mutex_lock(&mtx_clock);
	while(1){
		while(done<timer_num){
			pthread_cond_wait(&cond, &mtx_clock);
			//...
		}
		done=0;
        pthread_cond_broadcast(&condbr);
	}
}
