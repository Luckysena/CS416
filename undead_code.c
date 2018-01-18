// Author: John-Austen Francisco
// Date: 11 January 2018
//
// Preconditions: Appropriate C libraries, iLab machines
// Postconditions: Generates Segmentation Fault for
//                               signal handler self-hack

// Student name: Mykola Gryshko
// Ilab machine used: vi.cs.rutgers.edu

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void segment_fault_handler(int signum)
{
	printf("I am slain!\n");

	void * signumPtr =(void *) &signum;
	signumPtr += 0x0000003C;
	*(int *)signumPtr += 0x00000006;

}


int main()
{
	int r2 = 0;

	signal(SIGSEGV, segment_fault_handler);

	r2 = *( (int *) 0 );

	printf("I live again!\n");

	return 0;
}
