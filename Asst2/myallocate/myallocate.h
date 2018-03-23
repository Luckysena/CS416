#ifndef MY_ALLOCATE_H
#define MY_ALLOCATE_H

#define _GNU_SOURCE

#define THREADREQ 0
#define LIBRARYREQ 1

#define SYSPAGE (sysconf(_SC_PAGE_SIZE))
#define OS_SIZE 2097152
#define MEMORY_SIZE 8388608
#define SWAP_SIZE 16777216
#define OS_PAGE_NUM 512
#define TOTAL_PAGE_NUM 2048

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
#include "my_pthread_t.h"


#define malloc(x) mymalloc(x,__FILE__,__LINE__,LIBRARYREQ)
#define free(x) myfree(x,__FILE__,__LINE__,LIBRARYREQ)

typedef uint my_pthread_t;

typedef enum _bool {
	FALSE, TRUE
} bool;

/* malloc metadata struct definition */
typedef struct _memoryEntry {	
	int size;
	bool isFree;
	struct _memEntry *prev, *next;
} memEntry;

// page table entry attributes need more work
typedef struct _pageTableEntry {
	int location;
	bool isOS, isUsed;
	my_pthread_t tid;	
} pageTableEntry;

/* function calls for allocating memory */
void initMemLib();

void * myallocate(size_t size, char *file, int line, int req);

void myfree(void *ptr, char *file, int line, int reg);

memEntry *createBlock(size_t size, int isFree, int magicNum);

void addBlock(memEntry *block); 

void findBestFit(size_t size);

/* memory variables */
char *memblock;
void *base_page;
pageTableEntry pageTable[TOTAL_PAGE_NUM];
static void *usr_space;
static void *swap_space;


#endif
