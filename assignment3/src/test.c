
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define BLOCK_SIZE 512       //2^9
#define FILESIZE 16777216    //2^24
#define INODE_COUNT 128      //2^7
#define MAX_FILE_SIZE 8192   //2^13
#define MAX_DATA_SIZE 1048576 //2^20 = inode * max file size
#define MAX_INODE_SIZE 65536 //2^16 = inode * 512
#define MAX_DATA_BLOCKS 16    //2^4 = 8192 / 512
#define MAX_FILE_NAME 256

typedef enum _bool{
  FALSE, TRUE
}bool;

typedef enum _blockType{
  ERROR, DIRECTORY, FILES
}blockType;

// 16 of them
typedef struct _inode{
  blockType type;
  struct stat s;
  int indexes[MAX_DATA_BLOCKS];
  char name[MAX_FILE_NAME];
  char bufSpace[40];
}inode;

// 3 of them pointing to 16 ea = 48
typedef struct _singleIndirectionInode{
  inode* list;
}singleIndirectionInode;

// 2 of them pointing to 2 single to 16 ea = 64
typedef struct _doubleIndirectionInode{
  singleIndirectionInode* list;
}doubleIndirectionInode;

typedef struct _dataBlock{
  int size;
  char data[BLOCK_SIZE - sizeof(int)];
}dataBlock;


typedef struct _bootBlock{
  int magicNum;
  int isinit;
  int superBlockIndex;
  char bufSpace[500];
}bootBlock;

typedef struct _superBlock{
  int partitionSize;
  int blocksize;
  inode* inodeList;
  singleIndirectionInode* singleList;
  doubleIndirectionInode* doubleList;
  bool* inode_bitmap;
  bool* data_bitmap;
  char bufSpace[464];
}superBlock;

int main(int argc, char** argv){
  printf("Size of bool struct is: %li\n",sizeof(bool));
  return 0;

}
