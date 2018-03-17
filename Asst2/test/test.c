#include "../my_pthread_t.h"


int main(int argc, char** argv){
  bool test = FALSE;

  printf("test is: %i\n", sysconf(_SC_PAGE_SIZE));
  return 0;
}
