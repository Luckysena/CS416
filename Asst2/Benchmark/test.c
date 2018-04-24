#include <stdlib.h>
#include <pthread.h>

#include "../my_pthread_t.h"

void func(pthread_t* tid){
	int *ptr = (int*)malloc(sizeof(int) * 10);
	printf("[TID: %i] location is: %u\n", tid, ptr);
	*ptr = 691;
	printf("[TID: %i] number is: %i\n", tid, *ptr);
	return;
}

int main() {
	int *ptr1 = (int*)malloc(sizeof(int) * 10);
	printf("[MAIN] location is: %u\n", ptr1);
	*ptr1 = 69;
	printf("[MAIN] number is: %i\n", *ptr1);



	pthread_t *thread = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(&thread, NULL, &func, NULL);
	pthread_join(thread, NULL);


	return 0;
}
