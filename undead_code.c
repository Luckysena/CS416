// Author: John-Austen Francisco
// Date: 11 January 2018
//
// Preconditions: Appropriate C libraries, iLab machines
// Postconditions: Generates Segmentation Fault for
//                               signal handler self-hack

// Student name: Mykola Gryshko
// Ilab machine used:

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void segment_fault_handler(int signum)
{
	printf("I am slain!\n");

	int * p = &signum;
	//signumPtr += 0x61C;
	p += 0x4c-0x10; //Increment pointer down to the stored PC
	*(int *)p += 0x6; //Increment value at pointer by length of bad instruction

	//Use the signnum to construct a pointer to flag on stored stack
	//Increment pointer down to the stored PC
	//Increment value at pointer by length of bad instruction



}


int main()
{
	int r2 = 0;

	signal(SIGSEGV, segment_fault_handler);

	r2 = *( (int *) 0 );

	printf("I live again!\n");

	return 0;
}
