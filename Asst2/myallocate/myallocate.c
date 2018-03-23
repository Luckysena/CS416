#include "myallocate.h"

static void handler(int sig, siginfo_t *si, void *unused) {

	return;
}

void initMemLib() {

	memblock = (char *) memalign(SYSPAGE,MEMORY_SIZE+SWAP_SIZE); 	
	base_page = (char *) memblock;
	user_space = (char *) memblock + SYSPAGE * OS_PAGE_NUM;
	swap_space = (char *) memblock + ;	
		
	// initialize page table
	int i;

	for(i = 0; i < OS_PAGE_NUM; i++) {
		pageTable[i].location = i;
		pageTable[i].isOS = TRUE;
		pageTable[i].isUsed = FALSE;
		pageTable[i].tid = -1;	
	}

	for(i = OS_PAGE_NUM; i < TOTAL_PAGE_NUM; i++) {
		pageTable[i].location = i;
		pageTable[i].isOS = FALSE;
		pageTable[i].isUsed = FALSE;
		pageTable[i].tid = -1;
	}

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handler;
	
	if(sigaction(SIGSEGV, &sa, NULL) == -1) {
		printf("Fatal error setting up signal handler\n");
		exit(EXIT_FAILURE);
	}	

	return;
}

void *myallocate(size_t size, char *file, int line, int req) {

	if(base_page == NULL) {
		initMemLib();
	}

	if(size <= 0) {
		fprintf(stderr,"ERROR: invalid request, must be greater than 0. FILE: %s, LINE %d\n",__FILE__,__LINE__);
		return NULL;
	}

		

};

