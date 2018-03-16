#ifndef _MY_ALLOCATE_H
#define _MY_ALLOCATE_H

#define _GNU_SOURCE

#define THREADREQ 0
#define LIBRARYREQ 1

#define MEMSIZE 16

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

#define malloc(x) mymalloc(x,__FILE__,__LINE__,LIBRARYREQ)
#define free(x) myfree(x,__FILE__,__LINE__,LIBRARYREQ)

static char memblock[8000000];

typedef struct memoryEntry {
	int size;
	int isFree;	
	struct memoryEntry *nextMemBlock;
} memEntry;

/* function calls for allocating memory */
void * myallocate(size_t size, char *file, int line, int req);

void myfree(void *ptr, char *file, int line, int reg);

memEntry *createBlock(size_t size, int isFree, int magicNum);

void addBlock(memEntry *block); 

void findBestFit(size_t size);

#endif
