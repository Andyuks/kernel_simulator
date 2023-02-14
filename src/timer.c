/* timer.c */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern pthread_mutex_t mtx_clock, mtx_pg, mtx_dispatcher;
extern pthread_cond_t cond, condbr, cond_pg, cond_dispatcher, cond_pg2,cond_dispatcher2;
extern int done;

/* Deals with the starting of pg (or loader) timer */
void timer_pg()
{	
	int freq_pg = 600000;
	int cont=0;
	printf("\033[0;36mTimer pg>>\033[0m Entered!\n");
	pthread_mutex_lock(&mtx_pg);
	pthread_mutex_lock(&mtx_clock);
	printf("\033[0;36mTimer pg>>\033[0m I got mtxs!\n");

	while(1){	
		done++;
		/* actions */
		cont ++;
		if(cont==freq_pg){
			// throw_exception 1 tick -> trigger pg/loader
			printf("\033[0;36mtimer_pg>>\033[0m trigger pg\n");
			pthread_cond_wait(&cond_pg, &mtx_pg);
			cont=0;
		}

		/* sincr */
		pthread_cond_broadcast(&cond);
		pthread_cond_wait(&condbr, &mtx_clock);
		pthread_cond_broadcast(&cond_pg2);
	}
}



/* Deals with the starting of dispatcher timer*/
void timer_dispatcher()
{	
	int cont=0;
	int freq_disp = 700000;
	printf("\033[0;36mTimer disp>>\033[0m Entered!\n");
	pthread_mutex_lock(&mtx_dispatcher);
	pthread_mutex_lock(&mtx_clock);
	printf("\033[0;36mTimer disp>>\033[0m I got mutexs!\n");
	while(1){
		done++;
		/* actions */
		cont ++;
		if(cont==freq_disp){
			// throw_exception 1 tick -> trigger dispatcher
			printf("\033[0;36mtimer_disp>\033[0m trigger dispatcher\n");
			pthread_cond_wait(&cond_dispatcher, &mtx_dispatcher);
			cont=0;
		}
		/*sincr*/
		pthread_cond_broadcast(&cond);
		pthread_cond_wait(&condbr, &mtx_clock);
		pthread_cond_broadcast(&cond_dispatcher2);
	}
}