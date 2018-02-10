// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu

#include "my_pthread_t.h"
#define MEM 64000
int is_scheduler_init = 0;
my_pthread_t tcbIDs[32];

tcb* initTCB(){
	int i;
	if(!is_scheduler_init){
		for(i = 0;i<32;i++){
			tcbIDs[i] = 0;
		}
	}

	tcb* newTCB = (tcb*)malloc(sizeof(tcb));
	getcontext(newTCB->context);   //may need to init stack and flags
	newTCB->context->uc_link = Scheduler->context;  //go to scheduler's context upon completion
	newTCB->context->uc_stack.ss_sp = malloc(MEM);
	newTCB->context->uc_stack.ss_size = MEM;
	newTCB->context->uc_stack.ss_flags = 0;
	newTCB->status = NEW;
	newTCB->runCount = 0;
	newTCB->priority = HIGH;

	for(i = 0; i<32; i++){
		if(!tcbIDs[i]){
			newTCB->tid = i;
			tcbIDs[i] = 1;
		}
	}
	newTCB->next = NULL;
	return newTCB;
}

queue* initQ(){
	queue* newQ = (queue*)malloc(sizeof(queue));
	newQ->num_threads = 0;
	newQ->head = NULL;
	newQ->tail = NULL;
}

int QisEmpty(queue* qq){
	if(qq->num_threads==0){
		return 1;
	}
	else return 0;
}

void enqueue(queue* qq,tcb* thread){
	if (qq->num_threads == 0){
		qq->head = thread;
		qq->tail = thread;
		qq->num_threads++;
	}
	else if (qq->num_threads == 1){
		qq->tail = thread;
		qq->head->next = thread;
		qq->num_threads++;
	}
	else{
		qq->tail->next = thread;
		qq->tail = thread;
		qq->num_threads++;
	}
}

tcb* dequeue(queue* qq){

		tcb* tmp ;

		if(qq->num_threads == 0){
			return NULL;
		}

		else if (qq->num_threads == 1){
			tmp = qq->head;
			tmp->next = NULL;
			qq->head = NULL;
			qq->tail = NULL;
			qq->num_threads = 0;
			return tmp;
		}
		else{
			tmp = qq->head;
			qq->head = tmp->next;
			qq->num_threads--;
			tmp->next = NULL;
			return tmp;
	}
}

MLPQ* initTasklist(){
	MLPQ* newMLPQ = (MLPQ*)malloc(sizeof(MLPQ));
	newMLPQ->L1.num_threads = 0;
	newMLPQ->L2.num_threads = 0;
	newMLPQ->L3.num_threads = 0;
	newMLPQ->L1.head = NULL;
	newMLPQ->L1.tail = NULL;
	newMLPQ->L2.head = NULL;
	newMLPQ->L2.tail = NULL;
	newMLPQ->L3.head = NULL;
	newMLPQ->L3.tail = NULL;
}

void init_scheduler(){
	/*
	Need to initialize the queues & mutexes, set mode bit
	*/


	Scheduler = (scheduler*)malloc(sizeof(scheduler));  //memory for scheduler
	Scheduler->mainContext = initTCB();
	Scheduler->runQ = initQ();
	Scheduler->waitQ = initQ();
	Scheduler->tasklist = initTasklist();
	is_scheduler_init = 1;

	// context for scheduler, might be unneeded tbh
	getcontext(Scheduler->context);
	Scheduler->context->uc_link = 0; //replace with cleanup program later
	Scheduler->context->uc_stack.ss_sp = malloc(MEM);
	Scheduler->context->uc_stack.ss_size = MEM;
	Scheduler->context->uc_stack.ss_flags = 0;
	makecontext(Scheduler->context,(void*)&schedulerfn, 0); //set up scheduler function

	// add main() to run queue
	enqueue(Scheduler->runQ,Scheduler->mainContext);

	// timer init
	signal(SIGVTALRM,clock_interrupt_handler);
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 25000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 25000;

	setitimer(ITIMER_VIRTUAL, &timer, NULL);
	return;
}

void clock_interrupt_handler(){

		// turn off timer
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 0;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);

		tcb* old_thread = Scheduler->runningContext;
		old_thread->runCount++;

}

void schedulerfn(){
	while(1){
		//run this bitch forever
		if(!QisEmpty(Scheduler->runQ)){
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = 25000;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = 25000;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
		}

	}
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
