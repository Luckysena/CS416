// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu

#include "my_pthread_t.h"
#define MEM 64000
#define QUANTA 25000
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
	newTCB->context->uc_link = 0;  //should go to scheduler upon completion, need to work on it
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
	newMLPQ->L1 = (queue*)malloc(sizeof(queue));
	newMLPQ->L2 = (queue*)malloc(sizeof(queue));
	newMLPQ->L3 = (queue*)malloc(sizeof(queue));
	newMLPQ->L1->num_threads = 0;
	newMLPQ->L2->num_threads = 0;
	newMLPQ->L3->num_threads = 0;
	newMLPQ->L1->head = NULL;
	newMLPQ->L1->tail = NULL;
	newMLPQ->L2->head = NULL;
	newMLPQ->L2->tail = NULL;
	newMLPQ->L3->head = NULL;
	newMLPQ->L3->tail = NULL;
}

void init_scheduler(){
	/*
	Need to initialize the queues & mutexes
	*/


	Scheduler = (scheduler*)malloc(sizeof(scheduler));  //memory for scheduler
	Scheduler->mainContext = initTCB();
	Scheduler->runQ = initQ();
	Scheduler->waitQ = initQ();
	Scheduler->tasklist = initTasklist();
	Scheduler->runCount = 0;
	is_scheduler_init = 1;

	// context for scheduler
	getcontext(Scheduler->context);
	Scheduler->context->uc_link = 0; //replace with cleanup program later
	Scheduler->context->uc_stack.ss_sp = malloc(MEM);
	Scheduler->context->uc_stack.ss_size = MEM;
	Scheduler->context->uc_stack.ss_flags = 0;
	makecontext(Scheduler->context,(void*)&schedulerfn, 0); //set up scheduler function

	// add main() to Level 1
	enqueue(Scheduler->tasklist->L1,Scheduler->mainContext);

	// timer init
	signal(SIGVTALRM,clock_interrupt_handler);
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = QUANTA;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = QUANTA;

	setitimer(ITIMER_VIRTUAL, &timer, NULL);
	return;
}

void maintence(){
	// here we will clean up terminated threads from terminated Q and reorganize the threads in MLPQ
}

void clock_interrupt_handler(){

		// turn off timer
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 0;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);

		//establish that clock stopped this thread and that it didnt yield explicitly
		tcb* current_thread = Scheduler->runningContext;
		current_thread->status = INTERRUPTED;

		//call yield to handle the thread and run next item
		my_pthread_yield();
		return;

}

void schedulerfn(){
	//run this forever
	int i;
	while(1){

		//maintence counter
		Scheduler->runCount++;
		if(Scheduler->runCount >= 10){
			maintence();
		}


		//this should always occur, runQ will be empty and we swap to scheduler to run its maintence
		if(QisEmpty(Scheduler->runQ)){
			/* Here we will select the next runQ from the MLPQ
			   I(Mykola) think we should choose up to 60 QUANTA
				 for the runQ, we can readjust as needed, up to 5 from L1, up to 5 from L2, up to 3 from L3 */

				 //grab 5 if availbile
				 if(Scheduler->tasklist->L1->num_threads >= 5){
					 for(i = 0; i<5; i++){
					 	enqueue(Scheduler->runQ, Scheduler->tasklist->L1->head);
						}
				 }
				 //otherwise add as many as we can
				 else{

				 }

		}



		//then reset timer and swap context to first in runQ

	}
}
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	if(!is_scheduler_init){
		init_scheduler();
	}

	tcb * newTCB = initTCB();
	// gotta add the function and arguments to the newTCB and then add it to queue L1
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	tcb* old_thread = Scheduler->runningContext;
	
/* You should implement checking if the thread stopped due to a mutex lock wait here(by checking its status)
and add it to the waiting queue for that particular mutex. this should be done first before we re-order it's spot in
the tasklist since its being blocked */



	//inc run count
	old_thread->runCount++;

	//change priority and enqueue it into MLPQ (tasklist) only if it didn't call yield explicitly
	if(old_thread->status == INTERRUPTED){
		if(old_thread->priority == HIGH){
			old_thread->priority = MED;
			enqueue(Scheduler->tasklist->L2,old_thread);
		}
		else if(old_thread->priority == MED){
			old_thread->priority = LOW;
			enqueue(Scheduler->tasklist->L3,old_thread);
		}
	}

	//check if runQ is empty, if its not then run next item in it based on its priority
	if(!QisEmpty(Scheduler->runQ)){
		tcb* new_thread = dequeue(Scheduler->runQ);

		if(new_thread->priority == HIGH){
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = QUANTA;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = QUANTA;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
			swapcontext(old_thread->context,new_thread->context);
		}

		else if(new_thread->priority == MED){
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = 5*QUANTA;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = 5*QUANTA;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
			swapcontext(old_thread->context,new_thread->context);
		}

		else if(new_thread->priority == LOW){
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 10*QUANTA;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10*QUANTA;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		swapcontext(old_thread->context,new_thread->context);
		}
	}
	//this is incase the runQ is finished, then we swap to scheduler to make a new one
	else{
		swapcontext(old_thread->context,Scheduler->context);
	}

	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {

	tcb* old_thread = Scheduler->runningContext;
	old_thread->status = TERMINATED;
	enqueue(Scheduler->terminatedQ, old_thread);

	//Here we would include if/else block in my_pthread_yield
	//to swap context?
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
