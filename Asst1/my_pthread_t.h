// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>

typedef uint my_pthread_t;

typedef enum state{
	READY, RUNNING, WAITING, TERMINATED, NEW
}state;

typedef struct threadControlBlock {
	ucontext_t *context;
	state status;
	int runTime;
	int priority;
	my_pthread_t tid;
 	struct threadControlBlock * next;
} tcb;


/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
} my_pthread_mutex_t;


typedef struct _queue{
	/* temporary simple FIFO queue */
	tcb * head;
	tcb * end;
	int num_threads;
} queue;


typedef struct _scheduler{
	tcb *runningContext;
	tcb *mainContext;
	int state;
	queue * runQ;
	queue * waitQ;
}scheduler;




/* define your data structures here: */
scheduler * Scheduler;
int mode_bit;
struct itimerval timer;

/* Function Declarations: */

/* timer init */
void init_timer();

/* init for scheduler */
void init_scheduler();

/* clock handler */
void clock_interrupt_handler();

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
