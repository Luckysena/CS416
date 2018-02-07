// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu

#include "my_pthread_t.h"
#define MEM 64000
int is_scheduler_init = 0;


void init_scheduler(){
	/*
	Need to initialize the queues & mutexes, set mode bit, start clock
	*/
	Scheduler = malloc(sizeof(scheduler));
	mode_bit = 0;
	is_scheduler_init = 1;

	// context init
	getcontext(Scheduler->context);
	Scheduler->context->uc_link = 0; //this needs to be running program's main
	Scheduler->context->uc_stack.ss_sp = malloc(MEM);
	Scheduler->context->uc_stack.ss_size = MEM;
	Scheduler->context->uc_stack.ss_flags = 0;

	makecontext(Scheduler->context,(void*)&init_timer,NULL);
	return;
}

void init_timer(){
	// timer init
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = &clock_interrupt_handler;
	sigaction(SIGVTALRM, &sa, NULL);

	// config timer to set off every 25 ms
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 25000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 25000;

	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	return;
}

void clock_interrupt_handler(){
	/*
	Need to yield whatever thread we're on and give control to scheduler
	*/
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};
