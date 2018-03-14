#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#define malloc(x)	mymalloc( x, __FILE__, __LINE__)
#define free(x)		myfree( x, __FILE__, __LINE__)

void * mymalloc(unsigned short int size, char * file, int line);

void myfree(void * p, char * file, int line);

void make_even(unsigned short int * p);

void set_allocated(unsigned short int * p);

void set_free(unsigned short int * p);

void get_real_size(unsigned short int * p);

int get_size(unsigned short int size);

int is_allocated(unsigned short int * p);

void printTime(char* message, long int mean);

long int getTime(struct timeval start, struct timeval end);

void workloadA();

void memGrind();
