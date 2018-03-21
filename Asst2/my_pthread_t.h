// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H
#define _GNU_SOURCE
#define TOTAL_PAGE_NUM 2048
#define OS_PAGE_NUM 512

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
#include <malloc.h>
#include <signal.h>



typedef uint my_pthread_t;

typedef enum _modebit{
	LIBRARYREQ, THREADREQ
}modebit;

typedef enum _state{
	READY, RUNNING, WAITING, TERMINATED, NEW, INTERRUPTED, JOINING
}state;

typedef enum _priority{
	LOW, MED, HIGH
}Priority;

typedef enum _bool{
	FALSE, TRUE
}bool;

typedef struct threadControlBlock {
	ucontext_t *context;
	state status;
	int runCount;
	int * returnValue;
	Priority priority;
	my_pthread_t* tid;
 	struct threadControlBlock * next;
} tcb;

typedef struct _queue{
	/* simple FIFO queue */
	tcb * head;
	tcb * tail;
	int num_threads;
} queue;

typedef struct my_pthread_mutex_t {
	int mutexID;
	int state;			// 0 = unlocked, 1 = locked
	int wait_count;		// Number of waiting threads
	queue* mutexQ;		// Mutex Queue
	tcb * owner;
} my_pthread_mutex_t;

typedef struct _MLPQ{
	queue* L1;
	queue* L2;
	queue* L3;
}MLPQ;

typedef struct _scheduler{
	tcb *runningContext;
	tcb *mainContext;
	queue * terminatedQ;
	queue * runQ;
	MLPQ * tasklist;
	queue * joinQ;
	int runCount;
	int isWait;
	my_pthread_mutex_t ** Monitor;
}scheduler;

typedef struct _memEntry {
	int size;
	bool isFree;
	struct _memEntry *next;
	struct _memEntry *prev;
	int magicNum;
} memEntry;

typedef struct _PageTableEntry{
	bool validbit;
	bool OS_entry;
	int physLocation;
	void* head;
	my_pthread_t tid;
} PageTableEntry;

typedef struct _memBook{
	int numPages;
	int physLocation;
}memBook;


/* define your data structures here: */
scheduler* Scheduler;
struct itimerval timer;
PageTableEntry pageTable[TOTAL_PAGE_NUM];
static void* base_page;
static void* usr_space;

//unsure of this just yet
static void* swap_space;




/* Function Declarations: */

/* allocates a block of memory to caller */
void * myallocate(size_t size, char *file, int line, modebit req);

/* frees a given memory allocation block */
void mydeallocate(void *ptr, char *file, int line, modebit reg);

/* init a memEntry block */
void createMemEntry(size_t size, memEntry* pointer);

memEntry* findBestFit(size_t size);

void coalesce();

/* scheduler run function */
void schedulerfn();

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


#define USE_MY_PTHREAD 1

#ifdef USE_MY_PTHREAD
#define malloc(x) myallocate(x,__FILE__,__LINE__,THREADREQ)
#define free(x) mydeallocate(x,__FILE__,__LINE__,THREADREQ)
#define pthread_create my_pthread_create
#define pthread_yield my_pthread_yield
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif


#endif
