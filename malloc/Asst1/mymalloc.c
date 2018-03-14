#include "mymalloc.h"

static char myblock[5000];
static char * start = &myblock[0];

static char * prev = &myblock[0];
static char * curr = &myblock[0];
static char * next = &myblock[0];
static int remaining = 5000;
static int first_free_call = 1;
static int only_one_malloc = 1;

void * mymalloc(unsigned short int size, char * file, int line){
  // error check for valid requeste size
  if((size <=0) || (size > 4998)) {
  	printf("Requested invalid number of bytes in %s line %i\n", file, line);
	return NULL; 
  }

  unsigned short int *p = &size;
  make_even(p); // makes the requested size the next greatest even integer

  prev = curr;
  curr = next;
  next += (size + 2);

  int dummy_size;
  if(first_free_call == 0) { // if free was called before
  	curr = start; // reset curr to the beginning
  }

  if(curr == start) {
  	if(remaining == 5000){ // first time malloc is ever called to allocate onto myblock
		
		only_one_malloc = 1;
		remaining -= (size + 2);

		set_allocated(p); // change the LSB of the size to set allocated flag
		*((int *) curr) = size;

		//printf("1 >> prev: %i, curr: %i, next: %i, remain: %i, size: %i\n", prev, curr, next, remaining, size);

		return curr+2;
	
	} else if(remaining != 5000){
		only_one_malloc = 0; // curr was reset to the beginning
		while(curr <= (prev+size+2)) { // needs to traverse to see if available free block exists
			//printf("is allocated? : %i and *curr vs size: %i vs %i\n", is_allocated((unsigned short int *)curr), *curr, size);
			unsigned short int sizee = *curr; 
			dummy_size = get_size(sizee);
			//printf("dummy size is %i\n", dummy_size);
			if((!is_allocated((unsigned short int *)curr)) && ((*curr) >= size)){
				set_allocated((unsigned short int *) curr);

				//printf("3 >> prev: %i, curr: %i, next: %i, remain: %i, size: %i\n", prev, curr, next, remaining, size);
				return curr+2;
			} else {
				curr += dummy_size+2;
				//printf("curr is updated to %i\n", curr);
			}
		}
	
		return (prev+size+2); 
	}
  } 

  if(curr != start) {
  	if(remaining <= (size + 2)) {
	//	printf("Requested invalid number of bytes in %s line %i\n", file, line);
		return NULL;
	} 	

	only_one_malloc = 0; // cleared to 0 since this is the nth malloc call	
	remaining -= (size + 2);

	set_allocated(p); 
	*((int *) curr) = size;
    	//printf("2 >> prev: %i, curr: %i, next: %i, remain: %i, size: %i\n", prev, curr, next, remaining, size);	

	return curr+2; 
  }

  return NULL;
}

void myfree(void * p, char * file, int line){
	
  char * ptr = (char *) p;
  ptr  = ptr - 2; // ptr is the location of where the meta data starts
  
  //printf("Attempting to free address %i with value/character %c\n", ptr, *(ptr+2));

  if(!is_allocated((unsigned short int *) ptr)){
  	fprintf(stderr, "Error: requested to free unallocated pointer in %s line %i\n", file, line);
  }
  
  get_real_size((unsigned short int *) ptr);

  char * orig_ptr = ptr; // ptr to the location of freeing metadata
  int size = *orig_ptr;
  ptr += (*ptr + 2); // ptr to the location of next metadata

  if(first_free_call == 1){ // first free call
  	if(only_one_malloc == 1){ // total of only one malloc call
		while(orig_ptr < ptr){
			*orig_ptr = 0; // resets each index of myblock to 0
			orig_ptr += 1; 
		} 
		
		first_free_call = 0;
		curr = start;
		remaining += size; 
		return;
	} else if(only_one_malloc == 0){ // malloc has been called before
		if(*ptr == 0){ // if *ptr == 0, that means we are trying to free a block at the end (since no meta data block size is 0)
			//printf("ptr address %i and value %i\n", ptr, *ptr);
			//printf("orig add %i and value %i\n", orig_ptr, *orig_ptr);
			while(orig_ptr <= ptr){
				//printf("%c\n", *orig_ptr);
				*orig_ptr = 0;
				orig_ptr += 1;
			}

		  		
			first_free_call = 0;
			curr = start;
			remaining += (size+2);
			//printf("remaining is: %i\n", remaining);
			return;
		} else {
			//printf("ptr address %i and value %i\n", ptr, *ptr);
			//printf("orig add %i and value %i\n", orig_ptr, *orig_ptr);
			set_free((unsigned short int *)orig_ptr);
			//printf("now free: %i\n", *orig_ptr);
			orig_ptr += 2;
			while(orig_ptr < ptr){
				//printf("%c\n", *(orig_ptr));
				*orig_ptr = 0;
				orig_ptr += 1;
			}

			first_free_call = 0;
			curr = start;
			remaining += (size+2);
			return;
		}
	}
  } else if(first_free_call == 0){ // free has been called before so this one needs to check next neighbor and append if it is free
  	if(*ptr == 0){	
		while(orig_ptr <= ptr){
			*orig_ptr = 0;
			orig_ptr += 1;

		}

		first_free_call = 0;
		curr = start;
		remaining += (size+2);

		return;
	} else if(!(is_allocated((unsigned short int *)ptr))){	// if the next block is unallocated, need to combine with the current freeing block
		get_real_size((unsigned short int *) ptr);
		*((int *) orig_ptr) = (*orig_ptr) + (*ptr) + 2; // updated the unallocated block size
		//printf("updated is %i\n", *orig_ptr); 
		orig_ptr += 2;
		int i; 
		for(i = 0; i < *orig_ptr; i++){
			*orig_ptr = 0;
		}

		first_free_call = 0;
		curr = start;
		remaining += (*orig_ptr + 2);
		return;
	}
  }	
 
}


void make_even(unsigned short int * p){
  if((*p % 2) != 0){
	*p += 1;
  }
}

void set_allocated(unsigned short int * p){
  *p |= 1;
}

void set_free(unsigned short int * p){
  *p &= ~1;
}

void get_real_size(unsigned short int * p){
  (*p) &= ~1;
}

int get_size(unsigned short int size){
  return (size &= ~1);
}

int is_allocated(unsigned short int * p){
  if((*p) & 1){
	return 1;
  } else {
        return 0;
  }
}


