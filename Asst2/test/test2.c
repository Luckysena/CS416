#include <stdlib.h>

typedef struct _memEntry {

	struct _memEntry *prev, *next;
	int isFree;
	int size;
	int mode;

} memEntry;

int main() {

	memEntry *memPointer;
	printf("size of pointer is: %zu\n", sizeof(memPointer));

	return 0;
}

