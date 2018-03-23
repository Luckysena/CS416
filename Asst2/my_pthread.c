// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Mykola Gryshko, Joshua Kim
// username of iLab: mg1250, jbk91
// iLab Servers: vi.cs.rutgers.edu, factory.cs.rutgers.edu

#include "my_pthread_t.h"
#define MEM 64000
#define QUANTA 25000
#define L1THREADS 5
#define L2THREADS 5
#define L3THREADS 3
#define SYSPAGE (sysconf(_SC_PAGE_SIZE))
#define MEM_SIZE 8388608
#define OS_SIZE 2097152
#define SWAP_SIZE 16777216


/**********************************
					SIGSEV HANDLER
**********************************/
static void handler(int sig, siginfo_t *si, void* scrap){

}





/**********************************
					PAGE LIBRARY
**********************************/

void init_Mem(){
	base_page = memalign(SYSPAGE,MEM_SIZE);
	usr_space = base_page + OS_SIZE;


	/* init page table  */
	int i;

	/* OS space */

	for(i = 0; i < OS_PAGE_NUM; i++){
		pageTable[i].validbit = TRUE;
		pageTable[i].OS_entry = TRUE;
		pageTable[i].physLocation = i;
		pageTable[i].head = base_page + (i*SYSPAGE);
		MemBook[i].tid = 0;
		MemBook[i].isValid = FALSE;
	}

 	/* USR space */
	for(i = OS_PAGE_NUM; i < TOTAL_PAGE_NUM; i++){
		pageTable[i].validbit = TRUE;
		pageTable[i].OS_entry = FALSE;
		pageTable[i].physLocation = i;
		pageTable[i].head = usr_space + (i*SYSPAGE);
		MemBook[i].tid = -1;
		MemBook[i].isValid = FALSE;
	}

	/* signal handler for seg faults */
	struct sigaction sa;
 	sa.sa_flags = SA_SIGINFO;
 	sigemptyset(&sa.sa_mask);
 	sa.sa_sigaction = handler;

 	if (sigaction(SIGSEGV, &sa, NULL) == -1)
 	{
		printf("Fatal error setting up signal handler\n");
	 	exit(EXIT_FAILURE);    //explode!
 	}
	return;
}


/**********************************
					MEMORY LIBRARY
**********************************/



memEntry* getHead(modebit req){
	// need to return the memEntry struct at the front of a free page
	int i;
	if(req == LIBRARYREQ){
		// the OS requests space
		for(i = 0; i < OS_PAGE_NUM; i++){
			// if its initialized
			if (MemBook[i].isValid == TRUE){
				return (memEntry*)pageTable[i].head;
			}
			//otherwise make it
			else{
				createMemEntry(SYSPAGE, pageTable[i].head, i);
				MemBook[i].isValid == TRUE;
				return (memEntry*)pageTable[i].head;
			}
		}
		if(i == OS_PAGE_NUM){
			fprintf(stderr, "ERROR: OS requested space but none left FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
	}
	else{
		// the user requests space
		for(i = OS_PAGE_NUM; i < TOTAL_PAGE_NUM; i++){
			if (MemBook[i].isValid == TRUE){
				return (memEntry*)pageTable[i].head;
			}
			else{
				createMemEntry(SYSPAGE, pageTable[i].head, i);
				MemBook[i].isValid == TRUE;
				return (memEntry*)pageTable[i].head;
			}
		}
		if(i == TOTAL_PAGE_NUM){
			fprintf(stderr, "ERROR: USR requested space but none left FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
	}

}

void* myallocate(size_t size, char *file, int line, modebit req) {

	if(size <= 0){
		fprintf(stderr,"ERROR: invalid request, zero or negative value. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	if(base_page == NULL){
		init_Mem();
	}

	memEntry* ptr = getHead(req);
	//getHead will always return the first memEntry after making sure its init

	return (void*)(findBestFit(size,ptr)+sizeof(memEntry));
}

void mydeallocate(void *ptr, char *file, int line, modebit reg){

	if(ptr = NULL) {
		fprintf(stderr,"ERROR: invalid free request, null pointer. FILE %s, LINE %d\n",__FILE__,__LINE__);
	}

}


void createMemEntry(size_t size, void* pointer, int pageTableValue){

		memEntry newHead;
		newHead.magicNum = 1409;
		newHead.isFree = TRUE;
		newHead.next = NULL;
		// this is the case where the page hasnt been init yet
		if(pageTableValue != -1){
			//create the struct locally then memcpy it into the location given by pointer
			newHead.size = (size - sizeof(memEntry));
			newHead.prev = NULL;
			memcpy(pointer, &newHead, sizeof(memEntry));
			return;
		}

		// other case is page has been init and now we need to create the next dude
		newHead.size = size;
		memcpy(pointer, &newHead,sizeof(memEntry));
		return;
}

memEntry* findBestFit(size_t size, memEntry* ptr){

	memEntry *tmp, *best;

	for(tmp = ptr; tmp != NULL; tmp = tmp->next){
		if((tmp->size >= size) && (tmp->isFree == TRUE)){
			if(best == NULL){
				best = tmp;
				continue;
			}
			else if(best->size > tmp->size){
				best = tmp;
				continue;
			}
		}
	}
	size_t overSize = best->size - sizeof(memEntry);
	if(size < overSize){
		// usual case where we can create a new mementry following it
		createMemEntry((overSize - size), (void*)(best)+size, -1);
		tmp = (best+size);
		best->next = tmp;
		tmp->prev = best;
	}
	best->isFree = FALSE;
	// otherwise its case where we have not enough space between mementries to create another memEntry
	return best;
}

void coalesce(){


}

/*********************************************
							THREAD LIBRARY
*********************************************/


int is_scheduler_init = 0;
tcb* tcbList[33];


tcb* initTCB(my_pthread_t * tid){
	int i;
	if(!is_scheduler_init){
		for(i = 0;i<33;i++){
			tcbList[i] = NULL;
		}
	}

	tcb* newTCB = (tcb*)malloc(sizeof(tcb));
	newTCB->context = (ucontext_t*)malloc(sizeof(ucontext_t));   //may need to init stack and flags
	if(!is_scheduler_init){
		newTCB->context->uc_link = 0;  //should go to scheduler upon completion, need to work on it
	}
	else{
		newTCB->context->uc_link = Scheduler->mainContext->context;
	}
	newTCB->context->uc_stack.ss_sp = malloc(MEM);
	newTCB->context->uc_stack.ss_size = MEM;
	newTCB->context->uc_stack.ss_flags = 0;
	getcontext(newTCB->context);
	newTCB->status = NEW;
	newTCB->runCount = 0;
	newTCB->priority = HIGH;
	newTCB->returnValue = (int*)malloc(sizeof(int));

	for(i = 0; i<33; i++){
		if(tcbList[i] == NULL){
			*tid = i;
			newTCB->tid = tid;
			tcbList[i] = newTCB;
			break;
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
	return newQ;
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
	newMLPQ->L1 = initQ();
	newMLPQ->L2 = initQ();
	newMLPQ->L3 = initQ();

	return newMLPQ;
}

void init_scheduler(){

	my_pthread_t * INIT = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	*INIT = 0;
	Scheduler = (scheduler*)malloc(sizeof(scheduler));  //memory for scheduler
	Scheduler->mainContext = initTCB(INIT);
	Scheduler->runQ = initQ();
	Scheduler->joinQ = initQ();
	Scheduler->terminatedQ = initQ();
	Scheduler->runningContext = Scheduler->mainContext;
	Scheduler->tasklist = initTasklist();
	Scheduler->runCount = 0;
	Scheduler->isWait = 0;
	int i;
	Scheduler->Monitor = malloc(sizeof(my_pthread_mutex_t)*32);
	for(i = 0; i<32; i++){
		Scheduler->Monitor[i] = NULL;

	}


	is_scheduler_init = 1;
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
			tcbList[*Scheduler->terminatedQ->head->tid] = NULL;
			free(dequeue(Scheduler->terminatedQ));
		}
	}


	// if it ran more than 10 times bump it to L1
	int sizeList = Scheduler->tasklist->L3->num_threads;
	for (i = 0;i < sizeList; i++)
	{
		if(Scheduler->tasklist->L3->head->runCount >= 10){
			tcb * ptr = dequeue(Scheduler->tasklist->L3);
			ptr->priority = HIGH;
			enqueue(Scheduler->tasklist->L1, ptr);
		}
	}
	return;
}

void clock_interrupt_handler(){

		// turn off timer
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 0;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);

		//establish that clock stopped this thread and that it didnt yield explicitly
	 	Scheduler->runningContext->status = INTERRUPTED;

		//call yield to handle the thread and run next item
		my_pthread_yield();
		return;

}

void schedulerfn(){
	int i;

		//maintence counter
		Scheduler->runCount++;
		if(Scheduler->runCount >= 10){
			maintence();
		}


		//this should always occur, runQ will be empty and we swap to scheduler to run its maintence
		if(QisEmpty(Scheduler->runQ)){
					int L1size = Scheduler->tasklist->L1->num_threads;
					int L2size = Scheduler->tasklist->L2->num_threads;
					int L3size = Scheduler->tasklist->L3->num_threads;
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
				 	for (i = 0; i < L1size; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L1));
				 	}
				 }
				 // Grab from L2
				 if(Scheduler->tasklist->L2->num_threads >= L2THREADS)
				 {
					for(i = 0; i < L2THREADS; i++)
					{
					 	enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L2));
					}
				 }
				 else
				 {
				 	for (i = 0; i < L2size; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L2));
				 	}
				 }
				 // Grab from L3
				 if(Scheduler->tasklist->L3->num_threads >= L3THREADS)
				 {
					for(i = 0; i < L3THREADS; i++)
					{
					 	enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L3));
					}
				 }
				 else
				 {
				 	for (i = 0; i < L3size; i++)
				 	{
				 		enqueue(Scheduler->runQ, dequeue(Scheduler->tasklist->L3));
				 	}
				 }

		}

		//then reset timer and swap context to first in runQ
		tcb* old_context = Scheduler->runningContext;
		if(Scheduler->runQ->num_threads != 0){
			Scheduler->runningContext = dequeue(Scheduler->runQ);
		}
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = QUANTA;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = QUANTA;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		swapcontext(old_context->context,Scheduler->runningContext->context);

		return;
}

void wrapper_thread_function(void *(*function)(void *), void *arg) {
  void *retVal = function(arg);
  if (retVal != NULL) {
    my_pthread_exit(retVal);
  } else {
    my_pthread_exit(NULL);
  }
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	if(!is_scheduler_init){
		init_scheduler();
	}

	tcb * newTCB = initTCB(thread);

	// add the function and arguments to the newTCB and then add it to queue L1
	makecontext((newTCB->context), (void*)wrapper_thread_function, 2,function, arg);
	enqueue(Scheduler->tasklist->L1, newTCB);

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	//kill timer
	if(!is_scheduler_init){
		init_scheduler();
	}

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	tcb* old_thread = Scheduler->runningContext;
	old_thread->runCount++;

	if ((old_thread->status != WAITING) && (old_thread->status != TERMINATED))
	{
			//inc run count


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
			else if(old_thread->priority == LOW){
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
			swapcontext(old_thread->context,new_thread->context);
		}

		else if(new_thread->priority == MED){
			new_thread->status = RUNNING;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = 5*QUANTA;
			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = 5*QUANTA;
			setitimer(ITIMER_VIRTUAL, &timer, NULL);
			swapcontext(old_thread->context,new_thread->context);
		}

		else if(new_thread->priority == LOW){
		new_thread->status = RUNNING;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 10*QUANTA;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10*QUANTA;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		swapcontext(old_thread->context,new_thread->context);
		}
	}
	//this is incase the runQ is finished, then we swap to scheduler to make a new one
	else{ //when we do this we killed main()
		schedulerfn();
	}

	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {

	if(!is_scheduler_init){
		init_scheduler();
	}

	tcb* old_thread = Scheduler->runningContext;
	old_thread->status = TERMINATED;
	old_thread->returnValue = (int*)value_ptr;

	my_pthread_yield();

};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

	if(!is_scheduler_init){
		init_scheduler();
	}


	int i;
	tcb* joiningTCB = NULL;
	for(i = 0; i < 32; i++){
		if(tcbList[i]->tid != NULL){
			if(*(tcbList[i]->tid) == thread){
				joiningTCB = tcbList[i];
				break;
			}
		}
	}

	if(joiningTCB == NULL){
		//unlikely situation where scheduler cleaned term thread before parent called joiningTCB
		//or invalid tid was given
		return -1;
	}
	// situation where terminated thread is cleaned up?
	while(joiningTCB->status != TERMINATED){
		Scheduler->isWait = 1;
		Scheduler->runningContext->status = JOINING;
		my_pthread_yield();
	}
	Scheduler->runningContext->status = READY;
	Scheduler->isWait = 0;

	if(value_ptr == NULL){
		return 0;
	}

	*value_ptr = (int*)joiningTCB->returnValue;


	return 0;


};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr){
	if(mutex==NULL){
		return -1;
	}

	if(!is_scheduler_init){
		init_scheduler();
	}
	int i;
	for(i=0;i<32;i++){
		if(Scheduler->Monitor[i] == NULL){
			Scheduler->Monitor[i] = mutex;
			break;
		}
	}
	mutex->mutexQ = initQ();
	mutex->state = 0;
	mutex->wait_count = 0;


	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){
	if(!is_scheduler_init){
		init_scheduler();
	}

	if(mutex==NULL){
		printf("ERROR: mutex is not initialized");
		return -1;
	}


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
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){
	if(!is_scheduler_init){
		init_scheduler();
	}

	if(mutex==NULL){
		printf("ERROR: mutex is not initialized");
		return -1;
	}

	//only unlock if its the owner
	if(mutex->owner->tid == Scheduler->runningContext->tid){
		__atomic_store_n(&mutex->state, 0, __ATOMIC_SEQ_CST);
	}


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
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){
	if(!is_scheduler_init){
		init_scheduler();
	}

	if(mutex==NULL){
		printf("ERROR: mutex is not initialized");
		return -1;
	}

	//check if mutex is unlocked, this inherently makes sure queue is empty
	if(mutex->state == 1){
		return -1;
	}

	//otherwise free it
	free(mutex->mutexQ);

	return 0;
};
