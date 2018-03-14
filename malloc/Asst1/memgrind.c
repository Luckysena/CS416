#include "mymalloc.h"

long int wlAmean = 0;
long int wlBmean = 0;
long int wlCmean = 0;
long int wlDmean = 0;
long int wlEmean = 0;
long int wlFmean = 0;

void printTime(char* message, long int mean){
  printf("The mean time for workload %s was %ld\n", message, mean);
}
long int getTime(struct timeval start, struct timeval end){
return ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec ));
}

void workloadA(){
  int i=0;
  struct timeval start, end;
  gettimeofday(&start, NULL);
  void * ptrs[3000];
  for(i = 0; i < 3000; i++){
    ptrs[i] = (char*)malloc(sizeof(char) * 1);
    }
  for(i = i - 1; i > -1; i--){
      if(ptrs[i] != NULL){
	free(ptrs[i]);}
    }
    gettimeofday(&end, NULL);
    wlAmean += getTime(start, end);

}



void workloadB(){
  struct timeval start, end;
  gettimeofday(&start, NULL);
  int i = 0;
  for(i=0;i<3000; i++){
    void * a = (char*)malloc(sizeof(char)); 
    free(a);
  }

  gettimeofday(&end, NULL);
  wlBmean = wlBmean + getTime(start, end);

}


void workloadC(){
  struct timeval start, end;
  gettimeofday(&start, NULL);
  int i;
  int ptrTrack = 0;
  int mallocItr = 0;
  char * ptrs[3000];
  for(i = 0; i < 6000; i++){
    int r = rand() % 2;
    //is malloc                                                                 
    if(r == 1){
      //if malloc has been called 3000 times                                    
      if(mallocItr == 3000){
        //free the pointers in array                                            
        for(i = ptrTrack-1; i < -1; i--){
	  free(ptrs[i]);
	}
      }
      //malloc has not been called 3000 times                                   
      ptrs[ptrTrack] = (char*)malloc(sizeof(char));
      mallocItr++;
      ptrTrack += 1;//onto next spot in array                                   
    }else{
      //if there is not a pointer in the array loop again                       
      if(ptrs[0] == NULL)
        continue;
      //else free the previous pointer
      ptrTrack--;                                          
      free(ptrs[ptrTrack]);
      ptrs[ptrTrack] = NULL;
    }
  }
  gettimeofday(&end, NULL);
  wlCmean += getTime(start,end);

}

void workloadD(){
  struct timeval start, end;
  gettimeofday(&start, NULL);
  int i;
  int ptrTrack = 0;
  int mallocItr = 0;
  char * ptrs[3000];
  for(i = 0; i < 6000; i++){
    int r = rand() % 2;
    //is malloc                                                                 
    if(r == 1){
      //if malloc has been called 3000 times                                    
      if(mallocItr == 3000){
        //free the pointers in array                                            
        for(i = ptrTrack-1; i < -1; i--){
	  free(ptrs[i]);
	}
      }
      //malloc has not been called 3000 times                                   
      ptrs[ptrTrack] = (char*)malloc(sizeof(char) * (rand() % 4999));
      if(!(ptrs[ptrTrack] == 0) && !(ptrs[ptrTrack] == NULL))
	ptrTrack +=1;
      mallocItr++;
     }else{
      //if there is not a pointer in the array loop again                       
      if(ptrs[0] == NULL || (ptrs[0] == 0))
        continue;
      //else free the previous pointer
      ptrTrack--;                                          
                        //pck += 1;rintf("ptr address %i and value %i\n", ptr, *ptr);
      free(ptrs[ptrTrack]);
      ptrs[ptrTrack] = NULL;
    }
  }
  gettimeofday(&end, NULL);
  wlDmean += getTime(start,end);

}

void workloadE(){
struct timeval start, end;
gettimeofday(&start,NULL);
void* ptrs[500];
int i = 0;
for(i=0;i<500;i++){
ptrs[i] = (char*)malloc(sizeof(char)*8);
}
for(i=i-1;i>-1;i--){
  free(ptrs[i]);
}
ptrs[0] = (char*)malloc(sizeof(char)*4998);
gettimeofday(&end, NULL);


wlEmean+=getTime(start,end);

}

void workloadF(){
struct timeval start, end;
gettimeofday(&start, NULL);

char * str1 = (char*)malloc(sizeof(char) * 4);
char * str2 = (char*)malloc(sizeof(char) * 4);
char * str3 = (char*)malloc(sizeof(char) * 4);

char * str4 = (char*)malloc(sizeof(char) * 4);

str1[0] = 'h'; str1[1] = 'e'; str1[2] = 'l'; str1[3] = 'p';
str2[0] = 'l'; str2[1] = 'o'; str2[2] = 'v'; str2[3] = 'e';
str3[0] = 'p'; str3[1] = 'o'; str3[2] = 'o'; str3[3] = 'p';
str4[0] = 't'; str4[1] = 'i'; str4[2] = 'm'; str4[3] = 'e';

printf("%s %s %s %s\n", str1, str2, str3, str4);

free(str4); 
free(str3);

char * str5 = (char*)malloc(sizeof(char) * 8);
str5[0] = 'b'; str5[1] = 'a'; str5[2] = 'n'; str5[3] = 'a'; str5[4] = 'n'; str5[5] = 'a';  str5[6] = 's'; str5[7] = '!';

printf("%s\n", str5);
 
}

void memGrind(){
  workloadA();                                                              
  workloadB();
  workloadC();
  workloadE();                                                              
  workloadF();
  
  printTime("A", wlAmean/100.0);                                              
  printTime("B", wlBmean/100.0);
  printTime("C", wlCmean/100.0);                                              
  printTime("E", wlEmean/100.0);
  printTime("F", wlFmean/100.0);                                              
}

int main(int argc, char ** argv){
  memGrind();
 return 0;
}
