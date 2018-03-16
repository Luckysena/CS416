#include "myallocate.h"

memEntry *head = NULL;
memEntry *tmpBestFit = NULL;

static int init = 0;
static char *memptr = &memblock[0];

void *myallocate(size_t size, char *file, int line, int req) {

	if(size >= 3980) {
		fprintf(stderr, "error: exceeded 4kb page size\n");
		return NULL;
	}

	memEntry *mBlock = createBlock(size, 0, 0);

	if(!init) {
		head = mBlock;
		return (void *) memptr; // will return the address of wherever memptr is left off 
	} else {
		findBestFit(size); // finds the block where the next best fit is
	}
			
};

/* need to determine what magicNum does */
memEntry* createBlock(size_t size, int isFree, int magicNum) {

	memEntry *mBlock = (memEntry *) memptr;
	mBlock->next = NULL;
	mBlock->size = size; 
	mBlock->isFree = 0;
	memptr += 16; // each memory entry block is 16 bytes
	return mBlock;
};

void addBlock(memEntry *block) {

};

void findBestFit(size_t size) {

	memEntry *tmp = head;
	while(tmp != NULL) {
		
		if ((tmp->size >= size) && (tmp->isFree == 0)) {
			if (tmpBestFit != NULL) {
				tmpBestFit = tmp;
				memptr = &tmpBestFit;
			} else {
				if(tmp->size <= tmpBestFit->size) {
					tmpBestFit = tmp;
					memptr = &tmpBestFit;
				}
			}
		}
	}

};
