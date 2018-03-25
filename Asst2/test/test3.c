#include <stdlib.h>

int main() {

	void *ptr = malloc(sizeof(char) * 10);
	printf("location is: %i\n", ptr);

	return 0;
}

