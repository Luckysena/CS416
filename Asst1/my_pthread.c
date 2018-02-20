// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko
// username of iLab: mg1250
// iLab Server: vi.cs.rutgers.edu

#include "my_pthread_t.h"
#define MEM 64000
#define QUANTA 25000
#define L1THREADS 5
#define L2THREADS 5
#define L3THREADS 3

#define pthread_create my_pthread_create
#define pthread_yield my_pthread_yield
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy

int is_scheduler_init = 0;
tcb* tcbList[32];

tcb* initTCB(my_pthread_t * tid){
	int i;
	if(!is_scheduler_init){
		for(i = 0;i<32;i++){
			tcbList[i] = NULL;
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
		if(tcbList[i] == NULL){
			newTCB->tid = tid;
			tcbList[i] = newTCB;
		}
	}
	newTCB->next = NULL;
	return newTCB;
}

queue* initQ()
{
	queue* newQ = (queue*)malloc(sizeof(queue));
	newQ->num_threads = 0;
	newQ->head = NULL;
	newQ->tail = NULL;
}

int QisEmpty(queue* qq)
{
	if(qq->num_threads==0){
		return 1;
	}
	else return 0;
}

void enqueue(queue* qq,tcb* thread)
{
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

tcb* dequeue(queue* qq)
{
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


	my_pthread_t * INIT = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	*INIT = 1;
	Scheduler = (scheduler*)malloc(sizeof(scheduler));  //memory for scheduler
	Scheduler->mainContext = initTCB(INIT);
	Scheduler->runQ = initQ();
	Scheduler->tasklist = initTasklist();
	Scheduler->runCount = 0;
	Scheduler->isWait = 0;
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
	int i;
	//remove terminated threads as long as none are waiting
	if(Scheduler->isWait == 0){
		for(i = 0; i < Scheduler->terminatedQ->num_threads; i++){
			free(dequeue(Scheduler->terminatedQ));
		}
	}

	// Put everything into temp_list
	tcb * temp_list[32];
	for (i = 0; i < Scheduler->tasklist->L3->num_threads; i++)
	{
		temp_list[i] = dequeue(Scheduler->tasklist->L3);
	}

	quickSort(temp_list,0,Scheduler->tasklist->L3->num_threads);
	int list_size = Scheduler->tasklist->L3->num_threads;
	Scheduler->tasklist->L3->num_threads = 0;

	//take top half and put it back in L3
	for(i = 0; i < (list_size/2); i++)
	{
	 	enqueue(Scheduler->tasklist->L3, temp_list[i]);
	}

	list_size = list_size/2;
	//other half goes into L1
	for(i = 0; i < list_size; i++)
	{
	 	enqueue(Scheduler->tasklist->L1, temp_list[i]);
	}

}

void quickSort(tcb* list[], int l, int r){
	int j;

	if( l < r )
	{
	 	j = partition( list, l, r);
		quickSort(list, l, j-1);
		quickSort(list, j+1, r);
	}
}

int partition(tcb* list[], int l, int r) {
   int i, j;
	 tcb * t, pivot;
   pivot = list[l];
   i = l; j = r+1;

   while(1)
   {
   	do ++i; while( list[i]->runCount <= pivot->runCount && i <= r );
   	do --j; while( list[j]->runCount > pivot->runCount );
   	if( i >= j ){
			break;
		}
   	t = list[i]; list[i] = list[j]; list[j] = t;
   }
   t = list[l]; list[l] = list[j]; list[j] = t;
   return j;
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
	while(1)
	{

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

				 // Grab from L1
				 if(Scheduler->tasklist->L1->num_threads >= L1THREADS)
				 {
					for(i = 0; i < L1THREADS; i++)
					{
					 	enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
					}
				 }
				 else
				 {
				 	for (i = 0; i < Scheduler->tasklist->L1->num_threads; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
				 	}
				 }
				 // Grab from L2
				 if(Scheduler->tasklist->L2->num_threads >= L2THREADS)
				 {
					for(i = 0; i < L2THREADS; i++)
					{
					 	enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
					}
				 }
				 else
				 {
				 	for (i = 0; i < Scheduler->tasklist->L2->num_threads; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
				 	}
				 }
				 // Grab from L3
				 if(Scheduler->tasklist->L3->num_threads >= L3THREADS)
				 {
					for(i = 0; i < L3THREADS; i++)
					{
					 	enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
					}
				 }
				 else
				 {
				 	for (i = 0; i < Scheduler->tasklist->L3->num_threads; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
				 	}
				 }

		}

		//then reset timer and swap context to first in runQ
		Scheduler->runningContext = dequeue(Scheduler->runQ);

		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = QUANTA;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = QUANTA;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		setcontext(Scheduler->runningContext->context);

	}
}
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	if(!is_scheduler_init){
		init_scheduler();
	}

	tcb * newTCB = initTCB(thread);

	// add the function and arguments to the newTCB and then add it to queue L1
	makecontext((newTCB->context), (void*)function, 1, arg);
	enqueue(Scheduler->tasklist->L1, newTCB);

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	//kill timer
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	tcb* old_thread = Scheduler->runningContext;

	if ((old_thread->status != WAITING) || (old_thread->status != TERMINATED))
	{
			//inc run count
		old_thread->runCount++;

		//change priority and enqueue it into MLPQ (tasklist) only if it didn't call yield explicitly
		if(old_thread->status == INTERRUPTED){
			if(old_thread->priority == HIGH){
				old_thread->priority = MED;
				enqueue(Scheduler->tasklist->L2,old_thread);
				old_thread->status = READY;
			}
			else if(old_thread->priority == MED){
				old_thread->priority = LOW;
				enqueue(Scheduler->tasklist->L3,old_thread);
				old_thread->status = READY;
			}
		}
		//case where it yielded itself
		else{
			if(old_thread->priority == HIGH){
				enqueue(Scheduler->tasklist->L1,old_thread);
				old_thread->status = READY;
			}
			else if(old_thread->priority == MED){
				enqueue(Scheduler->tasklist->L2,old_thread);
				old_thread->status = READY;
			}
			else if(old_thread->priority == LOW){
				enqueue(Scheduler->tasklist->L3,old_thread);
				old_thread->status = READY;
			}
		}
	}
	else if(old_thread->status == TERMINATED){
		enqueue(Scheduler->terminatedQ, old_thread);
	}

	//check if runQ is empty, if its not then run next item in it based on its priority
	if(!QisEmpty(Scheduler->runQ)){
		tcb* new_thread = dequeue(Scheduler->runQ);
		Scheduler->runningContext = new_thread;

		if(new_thread->priority == HIGH){
			new_thread->status = RUNNING;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = QUANTA;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = QUANTA;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
			setcontext(new_thread->context);
		}

		else if(new_thread->priority == MED){
			new_thread->status = RUNNING;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = 5*QUANTA;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = 5*QUANTA;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
			setcontext(new_thread->context);
		}

		else if(new_thread->priority == LOW){
		new_thread->status = RUNNING;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 10*QUANTA;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10*QUANTA;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		setcontext(new_thread->context);
		}
	}
	//this is incase the runQ is finished, then we swap to scheduler to make a new one
	else{
		setcontext(Scheduler->context);
	}

	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {

	tcb* old_thread = Scheduler->runningContext;
	old_thread->status = TERMINATED;
	old_thread->returnValue = value_ptr;

	pthread_yield();

};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

	int i;
	tcb* joiningTCB = NULL;
	for(i = 0; i < 32; i++){
		if(*(tcbList[i]->tid) == thread){
			joiningTCB = tcbList[i];
			break;
		}
	}

	if(joiningTCB == NULL){
		//unlikely situation where scheduler cleaned term thread before parent called joiningTCB
		return -1;
	}
	// situation where terminated thread is cleaned up?
	while(joiningTCB->status != TERMINATED){
		Scheduler->isWait = 1;
		Scheduler->runningContext->status = WAITING;
		my_pthread_yield();
	}

	Scheduler->runningContext->status = READY;
	*value_ptr = joiningTCB->returnValue;
	Scheduler->isWait = 0;

	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	mutex = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t*));
	mutex->mutexQ = initQ();
	mutex->state = 0;
	mutex->wait_count = 0;

	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{

	if(mutex->owner == NULL){
		// then continue
	}
	else if(mutex->owner->tid == Scheduler->runningContext->tid){
		//owner tries to double lock
		return -1;
	}
	else if((mutex->state == 0) && (mutex->owner->tid != Scheduler->runningContext->tid)){
		// case where mutex owner isnt running context, need to add to queue
		enqueue(mutex->mutexQ, Scheduler->runningContext);
		mutex->wait_count++;
		Scheduler->runningContext->status = WAITING;

		my_pthread_yield();
	}

	// Only true if mutex is locked
	if (__atomic_exchange_n(&mutex->state, 1, __ATOMIC_SEQ_CST) == 1)
	{
		// Add to mutex waitQ
		enqueue(mutex->mutexQ, Scheduler->runningContext);
		mutex->wait_count++;

		// Set runQ state to WAITING
		Scheduler->runningContext->status = WAITING;
		my_pthread_yield();
	}
	else{
		// we locked and gave an owner
		mutex->owner = Scheduler->runningContext;
	}

	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
	//only unlock if its the owner
	if(mutex->owner->tid == Scheduler->runningContext->tid){
		__atomic_store_n(&mutex->state, 0, __ATOMIC_SEQ_CST);
	}

	//check waitQ
	if(QisEmpty(mutex->mutexQ)){
		mutex->owner = NULL;
		return 0;
	}
	else{
		mutex->owner = dequeue(mutex->mutexQ);
		mutex->wait_count--;
		mutex->owner->status = READY;
		mutex->owner->priority = HIGH;
		enqueue(Scheduler->tasklist->L1,mutex->owner);
	}

	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
	//check if mutex is unlocked, this inherently makes sure queue is empty
	if(mutex->state == 1){
		return -1;
	}

	//otherwise free it
	free(mutex);

	return 0;
};
